@ECHO OFF
REM This batch file generates a new Kodiak app project for Visual Studio 2022.
REM It is expected that python.exe is on your path and is version 3.0 or above.
python.exe Programs\AppGen\MakeNewApp.py %1