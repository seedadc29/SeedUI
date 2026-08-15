@echo off
title Stove 3D Studio - Iniciando...
cd /d "%~dp0\Stove3D"
echo ============================================================
echo      STOVE 3D STUDIO - MINI-EDITOR 3D (OFFLINE / 60 FPS)
echo ============================================================
echo.
echo Iniciando servidor local ultraleve...
echo.
start http://localhost:5173
npx vite --host localhost --port 5173
pause
