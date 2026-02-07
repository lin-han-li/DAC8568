@echo off
chcp 65001 > nul
setlocal EnableDelayedExpansion

:: 获取脚本所在目录（即项目根目录）
set "PROJECT_ROOT=%~dp0"
set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

:: 定义源目录和目标目录（相对于项目根目录）
set "SOURCE_DIR=%PROJECT_ROOT%\FreeRTOS替换文件"
set "TARGET_DIR=%PROJECT_ROOT%\Middlewares\Third_Party\FreeRTOS\Source\portable\RVDS\ARM_CM4F"

:: 需要复制的文件列表
set "FILE1=port.c"
set "FILE2=portmacro.h"

echo.
echo ============================================
echo    FreeRTOS Portable 文件替换脚本
echo ============================================
echo.
echo 项目根目录: %PROJECT_ROOT%
echo 源目录: %SOURCE_DIR%
echo 目标目录: %TARGET_DIR%
echo.

:: 检查源目录是否存在
if not exist "%SOURCE_DIR%" (
    echo [错误] 源目录不存在: %SOURCE_DIR%
    echo 请确保项目根目录下存在 "FreeRTOS替换文件" 文件夹
    goto :error
)

:: 检查目标目录是否存在
if not exist "%TARGET_DIR%" (
    echo [错误] 目标目录不存在: %TARGET_DIR%
    echo 请确保项目结构正确
    goto :error
)

:: 检查源文件是否存在
if not exist "%SOURCE_DIR%\%FILE1%" (
    echo [错误] 源文件不存在: %SOURCE_DIR%\%FILE1%
    goto :error
)
if not exist "%SOURCE_DIR%\%FILE2%" (
    echo [错误] 源文件不存在: %SOURCE_DIR%\%FILE2%
    goto :error
)

:: 复制文件
echo 正在复制文件...
echo.

copy /Y "%SOURCE_DIR%\%FILE1%" "%TARGET_DIR%\%FILE1%"
if errorlevel 1 (
    echo [错误] 复制 %FILE1% 失败！
    goto :error
)
echo [成功] %FILE1% 已复制到目标目录

copy /Y "%SOURCE_DIR%\%FILE2%" "%TARGET_DIR%\%FILE2%"
if errorlevel 1 (
    echo [错误] 复制 %FILE2% 失败！
    goto :error
)
echo [成功] %FILE2% 已复制到目标目录

echo.
echo ============================================
echo    所有文件替换成功！
echo ============================================
echo.
pause
exit /b 0

:error
echo.
echo ============================================
echo    替换过程中发生错误，请检查上述信息
echo ============================================
echo.
pause
exit /b 1
