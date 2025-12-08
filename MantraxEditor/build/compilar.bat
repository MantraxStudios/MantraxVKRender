@echo off
cmake -G Ninja ..

cd ..

cmake --build build
pause