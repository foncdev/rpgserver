@echo off
echo Building MMORPG Server System for Windows...

:: Visual Studio 빌드 환경 설정
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
if errorlevel 1 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
)

:: 빌드 디렉토리 생성
if not exist "build\windows" mkdir build\windows
cd build\windows

:: CMake 구성
cmake -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_INSTALL_PREFIX=../../dist/windows ^
      ../..

if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

::