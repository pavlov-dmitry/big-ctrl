mkdir bin
call stop.bat
pushd bin
cl -Zi ../src/big_ctrl.c user32.lib
popd
devenv bin/big_ctrl.exe