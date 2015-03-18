#include <windows.h>
#include <stdio.h>

HHOOK hook_handle = 0;

LRESULT PREVENT_KEY_PRESS = 1;
const int SC_SPACE = 0x39;
const int SC_CONTROL = 0x1d;

BOOL is_space_down = FALSE;
DWORD space_pressed_time = 0;

BOOL is_something_was_pressed = FALSE;

const int DELAY_TIME_MS = 70;
const int LONG_THINK_TIMEOUT_MS = 500;

#define MAX_DELAYED_COUNT 20
KBDLLHOOKSTRUCT delayed_keys[ MAX_DELAYED_COUNT ];
int delayed_keys_count = 0;

void reset_delayed_keys();
void add_to_delayed_keys( PKBDLLHOOKSTRUCT key_info );
void resolve_delays_keys_as_normal();
void resolve_delays_keys_with_ctrl();

void press_space( BOOL down );
void press_ctrl( BOOL down );

void process_space_down( PKBDLLHOOKSTRUCT key_info );
void process_space_up( PKBDLLHOOKSTRUCT key_info );
BOOL process_not_space_down( PKBDLLHOOKSTRUCT key_info );

LRESULT CALLBACK hook( int nCode, WPARAM wParam, LPARAM lParam );

BOOL is_need_delay( DWORD time );
BOOL is_down( PKBDLLHOOKSTRUCT key_info );

int CALLBACK WinMain( 
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow 
)
{
    MSG msg;

    hook_handle = SetWindowsHookEx(
        WH_KEYBOARD_LL, 
        (HOOKPROC)hook, 
        hInstance, 
        0
    );

    while( 1 ) 
    {
        PeekMessage( &msg, 0, 0, 0, PM_REMOVE );
        TranslateMessage( &msg );
        DispatchMessage( &msg );

        if ( is_space_down && delayed_keys_count && !is_need_delay( GetTickCount() ) )
        {
            resolve_delays_keys_with_ctrl();
        }

        Sleep( 10 );
    }

    UnhookWindowsHookEx( hook_handle );

    return 0;
}

LRESULT CALLBACK hook( int nCode, WPARAM wParam, LPARAM lParam )
{
    PKBDLLHOOKSTRUCT key_info = 0;

    if ( nCode < 0 ) 
    {
        return CallNextHookEx( hook_handle, nCode, wParam, lParam );
    }
    
    key_info = (PKBDLLHOOKSTRUCT)lParam;
    if ( key_info->vkCode == VK_SPACE )
    {
        if ( is_down( key_info ) )
        {
            process_space_down( key_info );
        }
        else
        {
            process_space_up( key_info );   
        }
        return PREVENT_KEY_PRESS;
    }
    else if ( is_down( key_info ) && is_space_down )
    {
        if ( process_not_space_down( key_info ) )
        {
            return PREVENT_KEY_PRESS;
        }
    }

    return CallNextHookEx( hook_handle, nCode, wParam, lParam );    
}

void process_space_down( PKBDLLHOOKSTRUCT key_info )
{
    if ( !is_space_down )
    {
        is_space_down = TRUE;
        space_pressed_time = key_info->time;
    }   
}

void process_space_up( PKBDLLHOOKSTRUCT key_info )
{
    if ( 0 != delayed_keys_count )
    {
        resolve_delays_keys_as_normal();
    }
    else if ( is_something_was_pressed )
    {
        press_ctrl( FALSE );
    }
    else
    {
        if ( ( key_info->time - space_pressed_time ) < LONG_THINK_TIMEOUT_MS)
        {
            press_space( TRUE );
            press_space( FALSE );
        }
    }
    is_space_down = FALSE;
    is_something_was_pressed = FALSE;
}

BOOL process_not_space_down( PKBDLLHOOKSTRUCT key_info )
{
    BOOL need_prevent = FALSE;
    if ( is_need_delay( key_info->time ) )
    {
        add_to_delayed_keys( key_info );
        need_prevent = TRUE;
    }
    else if ( !is_something_was_pressed )
    {
        is_something_was_pressed = TRUE;
        press_ctrl( TRUE );
    }  
    return need_prevent;
}

BOOL is_down( PKBDLLHOOKSTRUCT key_info )
{
    return ( key_info->flags & LLKHF_UP ) == 0;   
}

BOOL is_need_delay( DWORD time )
{
    return ( time - space_pressed_time ) < DELAY_TIME_MS;
}

void init_input( INPUT *input )
{
    input->type = INPUT_KEYBOARD;
    input->ki.time = 0;
    input->ki.dwFlags = 0;
    input->ki.dwExtraInfo = 0;
}

void set_down( INPUT *input, BOOL down )
{
    if ( !down )
    {
        input->ki.dwFlags |= KEYEVENTF_KEYUP;  
    }   
}

void key2input( KBDLLHOOKSTRUCT *key, INPUT *input)
{
    KEYBDINPUT *kbd = &input->ki;
    kbd->wVk = (WORD)key->vkCode;
    kbd->wScan = (WORD)key->scanCode;
    kbd->dwFlags = 0;
    if ( key->flags & LLKHF_EXTENDED )
    {
        kbd->dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    if ( key->flags & LLKHF_UP )
    {
        kbd->dwFlags |= KEYEVENTF_KEYUP;
    }
    kbd->dwExtraInfo = key->dwExtraInfo;
}

void press( INPUT* input )
{
    SendInput( 1, input, sizeof( INPUT ) );
}


void press_space( BOOL down )
{
    INPUT input;
    init_input( &input );
    input.ki.wVk = VK_SPACE;
    input.ki.wScan = SC_SPACE;
    set_down( &input, down );
    press( &input );
}

void press_ctrl( BOOL down )
{
    INPUT input;
    init_input( &input );
    input.ki.wVk = VK_CONTROL;
    input.ki.wScan = SC_CONTROL;
    set_down( &input, down );
    press( &input );  
}

void reset_delayed_keys()
{
    delayed_keys_count = 0;
}

void add_to_delayed_keys( PKBDLLHOOKSTRUCT key_info )
{
    if ( delayed_keys_count < MAX_DELAYED_COUNT )
    {
        delayed_keys[ delayed_keys_count ] = *key_info;
        delayed_keys_count += 1;
    }
}

void resolve_delays_keys()
{
    int i = 0;
    for ( i = 0; i < delayed_keys_count; i += 1 )
    {
        INPUT input;
        init_input( &input );
        key2input( &delayed_keys[ i ], &input );
        press( &input );
    }
    reset_delayed_keys();
}

void resolve_delays_keys_with_ctrl()
{
    press_ctrl( TRUE );
    resolve_delays_keys();
    is_something_was_pressed = TRUE;
}

void resolve_delays_keys_as_normal()
{
    press_space( TRUE );
    press_space( FALSE );
    resolve_delays_keys();
    is_something_was_pressed = FALSE;
}