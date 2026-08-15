@echo off
title Seed3D Studio Launcher
echo ========================================================
echo        INICIANDO SEED3D STUDIO (BLENDER 4.3 ENGINE)
echo ========================================================
echo.
cd /d "%~dp0"
if exist "Stove3D" (
    cd /d "%~dp0\Stove3D"
)

echo Abrindo o navegador em http://localhost:5173 ...
start http://localhost:5173

echo Iniciando o servidor Vite...
call npm run dev
pause
