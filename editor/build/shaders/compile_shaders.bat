@echo off
setlocal enabledelayedexpansion
title Compilador GLSL → SPIR-V

echo ======================================
echo     COMPILADOR GLSL → SPIR-V
echo ======================================
echo.

:: --- Comprobar glslc ---
where glslc >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: No se encontro glslc en PATH.
    echo Asegurate de que VulkanSDK esta instalado y la ruta agregada al PATH.
    pause
    exit /b 1
)

:: --- Pedir nombre del shader ---
set /p shaderName=Nombre base del shader (sin extension): 

if "%shaderName%"=="" (
    echo ERROR: No puedes dejar el nombre vacio.
    pause
    exit /b 1
)

echo.
echo Compilando shaders: %shaderName%.vert y %shaderName%.frag
echo --------------------------------------

:: --- Comprobar archivos ---
if not exist "%shaderName%.vert" (
    echo ERROR: No existe el archivo "%shaderName%.vert"
    pause
    exit /b 1
)

if not exist "%shaderName%.frag" (
    echo ERROR: No existe el archivo "%shaderName%.frag"
    pause
    exit /b 1
)

:: --- Compilar vertex ---
echo Compilando %shaderName%.vert...
glslc "%shaderName%.vert" -o "%shaderName%.vert.spv"
if %errorlevel% neq 0 (
    echo ERROR al compilar el vertex shader.
    pause
    exit /b 1
)

:: --- Compilar fragment ---
echo Compilando %shaderName%.frag...
glslc "%shaderName%.frag" -o "%shaderName%.frag.spv"
if %errorlevel% neq 0 (
    echo ERROR al compilar el fragment shader.
    pause
    exit /b 1
)

echo.
echo ======================================
echo ✓ Shaders compilados correctamente!
echo Se generaron:
echo   • %shaderName%.vert.spv
echo   • %shaderName%.frag.spv
echo ======================================
echo.

pause
exit /b 0
