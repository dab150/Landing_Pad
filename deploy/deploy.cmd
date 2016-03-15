setlocal enabledelayedexpansion

set username=
set buildnum=
set filename=BT_Causeway


for /f "delims=;" %%i in (.\deploy\build_number.txt) do set buildnum=%%i
for /f "delims=;" %%i in (.\deploy\user_name.txt) do set username=%%i

mkdir ..\..\hex\builds
copy dist\default\production\%filename%.X.production.hex ..\..\hex\builds\%filename%___build.%buildnum%.%username%.hex

@echo.
@echo.
@echo.
@echo ===========================================================================
@echo Created new firmware version: %filename%____build.%buildnum%.%username%.hex
@echo ===========================================================================

set /a buildnum=%buildnum% + 1

@echo %buildnum%;> .\deploy\build_number.txt