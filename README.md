# UefiDiskAccess
Simple Demo of Accessing Disks with Block I/O Protocol

## Introduction
This project is a simple demo of accessing disks using Block I/O Protocol in UEFI Environment.

## Requirement
To run this project, a working computer with UEFI firmware is required.

## Build
If it is your first time to build this project, you should run `build_prep.bat` to prepare for compilation. <br>
The required compiler for this project is Win64 LLVM. You may download it from [GitHub](https://github.com/llvm/llvm-project/releases). <br>
The required library for this project is TianoCore EDK II. You may download it from [GitHub](https://github.com/tianocore/edk2/releases). <br>
You are required to pre-compile the EDK II library. The method to compile is unofficial. You may use the script from [EDK II Library](https://github.com/MickeyMeowMeowHouse/EDK-II-Library) to build EDK II library. For detailed steps to pre-compile, consult that repository to proceed.

## Test
Setup a USB flash stick with GUID Partition Table (GPT). Construct a partition and format it into FAT32 file system. <br>
Copy the compiled `bootx64.efi` to `\EFI\BOOT\bootx64.efi` in your flash stick. <br>
Enter your firmware settings. Set the device option prior to the operating system. Disable Secure Boot unless you can sign the executable.

## License
This repository is under the MIT license.