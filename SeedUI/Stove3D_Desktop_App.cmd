@echo off
title Stove 3D Studio - Desktop App
cd /d "%~dp0\Stove3D"

echo ============================================================
echo      STOVE 3D STUDIO - DESKTOP STANDALONE APP (60 FPS)
echo ============================================================
echo.
echo [1/2] Iniciando servico interno ultraleve...

:: Inicia o Vite em segundo plano se ainda nao estiver rodando
start /min cmd /c "npx vite --host localhost --port 5173"

:: Aguarda 1.5 segundo para a porta abrir
timeout /t 2 /nobreak >nul

echo [2/2] Abrindo Janela Nativa Standalone...
echo.

:: Abre diretamente em modo de aplicativo standalone (sem barra de URL do navegador)
start msedge --app="http://localhost:5173" --window-size=1366,768 || start chrome --app="http://localhost:5173" --window-size=1366,768 || start http://localhost:5173

exit
