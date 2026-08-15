@echo off
title Blender Pro 3D (Blender 4.1 UI)
cd /d "%~dp0\BlenderPro3D"
echo ========================================================
echo   Iniciando Blender Pro 3D (Interface Blender 4.1 Pura)
echo ========================================================
echo.
start http://localhost:5175/
call npx vite --port 5175 --open
pause
