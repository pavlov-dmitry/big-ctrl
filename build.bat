mkdir bin
call stop.bat
pushd bin
cl /O2 ../src/big_ctrl.c user32.lib
popd