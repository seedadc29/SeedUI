@echo off
setlocal
set CFG=%1
if "%CFG%"=="" set CFG=Debug
set MSBUILD=E:\Visual estudio roxo\MSBuild\Current\Bin\MSBuild.exe
"%MSBUILD%" SeedNotas.vcxproj /p:Configuration=%CFG% /p:Platform=x64 /m /v:m /nologo
