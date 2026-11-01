@echo off
rem Compila el programa, pasa las pruebas y lo instala para el usuario actual (ver compilar.ps1, que es quien lo
rem hace). Se puede abrir con doble clic. Admite los mismos parametros, por ejemplo:
rem   compilar.bat -Qt C:\Qt\6.8.3\mingw_64 -SinEscritorio
rem   compilar.bat -Desinstalar
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0compilar.ps1" %*
set resultado=%errorlevel%
rem Con doble clic, la ventana se cerraria sin dejar leer el resultado.
echo %cmdcmdline% | find /i "%~0" >nul && pause
exit /b %resultado%
