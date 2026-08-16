@echo off
title Seed Engine - Logica Orbital 3D
color 0b

echo ===================================================
echo     SEED STUDIO - SUITE DE LOGICA ORBITAL 3D
echo ===================================================
echo.
echo [1/2] Iniciando servidor de desenvolvimento...
cd /d "%~dp0OrbitalLogicPrototype"

echo [2/2] Abrindo navegador em http://localhost:5175/
start http://localhost:5175/

npm run dev
pause
