@echo off

title Project: UefiDiskAccess, Preparation, UEFI (AMD64 Architecture)
echo Project: UefiDiskAccess
echo Platform: Unified Extensible Firmware Interface
echo Preset: Directory Build
echo Powered by zero.tangptr@gmail.com
pause.

echo Starting Compilation Preparations
mkdir ..\bin

echo Making Directories for UefiDiskAccess Checked Build, 64-Bit UEFI
mkdir ..\bin\compchk_uefix64
mkdir ..\bin\compchk_uefix64\Intermediate

echo Making Directories for UefiDiskAccess Free Build, 64-Bit UEFI
mkdir ..\bin\compfre_uefix64
mkdir ..\bin\compfre_uefix64\Intermediate

echo Preparation Completed!
pause.