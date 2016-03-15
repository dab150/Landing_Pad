setlocal enabledelayedexpansion

set username=
set buildnum=


for /f "delims=;" %%i in (.\deploy\build_number.txt) do set buildnum=%%i
for /f "delims=;" %%i in (.\deploy\user_name.txt) do set username=%%i

@echo.
@echo.
@echo.
@echo ===========================
@echo Compiling build #%buildnum%
@echo ===========================


@echo #define __BUILD %buildnum%> .\build.h
@echo #define __BUILD_USER "%username%"> .\build_user.h
