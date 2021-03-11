/*
  UefiDiskAccess - Simple demo of Accessing Disk I/O

  Copyright 2021 Zero Tang. All rights reserved.

  This program is distributed in the hope that it will be useful, but
  without any warranty (no matter implied warranty or merchantability
  or fitness for a particular purpose, etc.).

  File Location: /src/efimain.c
*/

#include <Uefi.h>
#include <IndustryStandard/Mbr.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <intrin.h>
#include "efimain.h"

CHAR16 BlockUntilKeyStroke(IN CHAR16 Unicode)
{
	EFI_INPUT_KEY InKey;
	do
	{
		UINTN fi=0;
		gBS->WaitForEvent(1,&StdIn->WaitForKey,&fi);
		StdIn->ReadKeyStroke(StdIn,&InKey);
	}while(InKey.UnicodeChar!=Unicode && Unicode);
	return InKey.UnicodeChar;
}

INTN EfiCompareGuid(EFI_GUID *Guid1,EFI_GUID *Guid2)
{
    if(Guid1->Data1>Guid2->Data1)
        return 1;
    else if(Guid1->Data1<Guid2->Data1)
        return -1;
    if(Guid1->Data2>Guid2->Data2)
        return 1;
    else if(Guid1->Data2<Guid2->Data2)
        return -1;
    if(Guid1->Data3>Guid2->Data3)
        return 1;
    else if(Guid1->Data3<Guid2->Data3)
        return -1;
    for(UINT8 i=0;i<8;i++)
    {
        if(Guid1->Data4[i]>Guid2->Data4[i])
            return 1;
        else if(Guid1->Data4[i]<Guid2->Data4[i])
            return -1;
    }
    return 0;
}

void SetConsoleModeToMaximumRows()
{
	UINTN MaxHgt=0,OptIndex;
	for(UINTN i=0;i<StdOut->Mode->MaxMode;i++)
	{
		UINTN Col,Row;
		EFI_STATUS st=StdOut->QueryMode(StdOut,i,&Col,&Row);
		if(st==EFI_SUCCESS)
		{
			if(Row>MaxHgt)
			{
				OptIndex=i;
				MaxHgt=Row;
			}
		}
	}
	StdOut->SetMode(StdOut,OptIndex);
	StdOut->ClearScreen(StdOut);
	StdOut->SetAttribute(StdOut,EFI_BACKGROUND_BLACK|EFI_LIGHTGRAY);
}

void DisplaySize(IN UINT64 Size,OUT CHAR16 *Buffer,IN UINTN Limit)
{
	if(Size<LimitKiB)
		UnicodeSPrint(Buffer,Limit,L"%u Bytes",Size);
	else if(Size>=LimitKiB && Size<LimitMiB)
		UnicodeSPrint(Buffer,Limit,L"%u KiB",Size>>10);
	else if(Size>=LimitMiB && Size<LimitGiB)
		UnicodeSPrint(Buffer,Limit,L"%u MiB",Size>>20);
	else
		UnicodeSPrint(Buffer,Limit,L"%u GiB",Size>>30);
}

EFI_STATUS EnumDiskPartitions(IN EFI_BLOCK_IO_PROTOCOL *BlockIoProtocol)
{
	EFI_STATUS st=EFI_DEVICE_ERROR;
	if(!BlockIoProtocol->Media->LogicalPartition)
	{
		MASTER_BOOT_RECORD MBRContent;
		st=BlockIoProtocol->ReadBlocks(BlockIoProtocol,BlockIoProtocol->Media->MediaId,0,sizeof(MASTER_BOOT_RECORD),&MBRContent);
		if(st==EFI_SUCCESS)
		{
			if(MBRContent.Signature!=MBR_SIGNATURE)
				StdOut->OutputString(StdOut,L"Invalid MBR Signature! MBR might be corrupted!\r\n");
			for(UINT8 i=0;i<MAX_MBR_PARTITIONS;i++)
			{
				MBR_PARTITION_RECORD *Partition=&MBRContent.Partition[i];
				if(Partition->OSIndicator)
				{
					UINT32 StartLBA=*(UINT32*)Partition->StartingLBA;
					UINT32 SizeInLBA=*(UINT32*)Partition->SizeInLBA;
					CHAR16 ScaledStart[32],ScaledSize[32];
					DisplaySize(__emulu(StartLBA,BlockIoProtocol->Media->BlockSize),ScaledStart,sizeof(ScaledStart));
					DisplaySize(__emulu(SizeInLBA,BlockIoProtocol->Media->BlockSize),ScaledSize,sizeof(ScaledSize));
					Print(L"MBR-Defined Partition %d: OS Type: 0x%02X  Start Position: %s  Partition Size: %s\n",i,Partition->OSIndicator,ScaledStart,SizeInLBA==0xFFFFFFFF?L"Over 2TiB":ScaledSize);
					if(Partition->OSIndicator==PMBR_GPT_PARTITION || Partition->OSIndicator==EFI_PARTITION)
					{
						EFI_PARTITION_TABLE_HEADER *GptHeader=AllocatePool(BlockIoProtocol->Media->BlockSize);
						if(GptHeader)
						{
							st=BlockIoProtocol->ReadBlocks(BlockIoProtocol,BlockIoProtocol->Media->MediaId,StartLBA,BlockIoProtocol->Media->BlockSize,GptHeader);
							if(st==EFI_SUCCESS)
							{
								if(GptHeader->Header.Signature!=EFI_PTAB_HEADER_ID)
									StdOut->OutputString(StdOut,L"Improper GPT Header Signature!");
								else
								{
									UINT32 PartitionEntrySize=GptHeader->SizeOfPartitionEntry*GptHeader->NumberOfPartitionEntries;
									VOID* PartitionEntries=AllocatePool(PartitionEntrySize);
									Print(L"Disk GUID: {%g}  Partition Array LBA: %u  Number of Partitions: %u\n",&GptHeader->DiskGUID,GptHeader->PartitionEntryLBA,GptHeader->NumberOfPartitionEntries);
									if(PartitionEntries)
									{
										st=BlockIoProtocol->ReadBlocks(BlockIoProtocol,BlockIoProtocol->Media->MediaId,GptHeader->PartitionEntryLBA,PartitionEntrySize,PartitionEntries);
										if(st==EFI_SUCCESS)
										{
											for(UINT32 j=0;j<GptHeader->NumberOfPartitionEntries;j++)
											{
												EFI_PARTITION_ENTRY *PartitionEntry=(EFI_PARTITION_ENTRY*)((UINTN)PartitionEntries+j*GptHeader->SizeOfPartitionEntry);
												if(EfiCompareGuid(&PartitionEntry->PartitionTypeGUID,&gEfiPartTypeUnusedGuid))
												{
													DisplaySize(MultU64x32(PartitionEntry->StartingLBA,BlockIoProtocol->Media->BlockSize),ScaledStart,sizeof(ScaledStart));
													DisplaySize(MultU64x32(PartitionEntry->EndingLBA-PartitionEntry->StartingLBA+1,BlockIoProtocol->Media->BlockSize),ScaledSize,sizeof(ScaledSize));
													Print(L"GPT-Defined Partition %u: Start Position: %s Partition Size: %s\n",j,ScaledStart,ScaledSize);
													Print(L"Partition Type GUID:    {%g}\n",&PartitionEntry->PartitionTypeGUID);
													Print(L"Unique Partition GUID:  {%g}\n",&PartitionEntry->UniquePartitionGUID);
												}
											}
										}
										FreePool(PartitionEntries);
									}
								}
							}
							else
								Print(L"Failed to read GPT Header! Status=0x%p\n",st);
							FreePool(GptHeader);
						}
					}
				}
			}
		}
	}
	return st;
}

void EnumAllDiskPartitions()
{
	for(UINTN i=0;i<NumberOfDiskDevices;i++)
	{
		// Skip absent media and partition media.
		if(DiskDevices[i].BlockIo->Media->MediaPresent && !DiskDevices[i].BlockIo->Media->LogicalPartition)
		{
			CHAR16 *DiskDevicePath=ConvertDevicePathToText(DiskDevices[i].DevicePath,FALSE,FALSE);
			if(DiskDevicePath)
			{
				StdOut->OutputString(StdOut,L"=============================================================================\r\n");
				Print(L"Partition Info of Device Path: %s\n",DiskDevicePath);
				FreePool(DiskDevicePath);
				Print(L"Block Size: %d bytes. I/O Alignment: 0x%X. Last LBA: 0x%llX.\n",DiskDevices[i].BlockIo->Media->BlockSize,DiskDevices[i].BlockIo->Media->IoAlign,DiskDevices[i].BlockIo->Media->LastBlock);
				EnumDiskPartitions(DiskDevices[i].BlockIo);
			}
		}
	}
	StdOut->OutputString(StdOut,L"=============================================================================\r\n");
}

EFI_STATUS InitializeDiskIoProtocol()
{
	UINTN BuffCount=0;
	EFI_HANDLE *HandleBuffer=NULL;
	// Locate all devices that support Disk I/O Protocol.
	EFI_STATUS st=gBS->LocateHandleBuffer(ByProtocol,&gEfiBlockIoProtocolGuid,NULL,&BuffCount,&HandleBuffer);
	if(st==EFI_SUCCESS)
	{
		DiskDevices=AllocateZeroPool(sizeof(DISK_DEVICE_OBJECT)*BuffCount);
		if(DiskDevices)
		{
			NumberOfDiskDevices=BuffCount;
			for(UINTN i=0;i<BuffCount;i++)
			{
				DiskDevices[i].DevicePath=DevicePathFromHandle(HandleBuffer[i]);
				gBS->HandleProtocol(HandleBuffer[i],&gEfiBlockIoProtocolGuid,&DiskDevices[i].BlockIo);
				if(HandleBuffer[i]==CurrentImage->DeviceHandle)
				{
					CHAR16* DevPath=ConvertDevicePathToText(DiskDevices[i].DevicePath,FALSE,FALSE);
					if(DevPath)
					{
						Print(L"Image was loaded from Disk Device: %s\r\n",DevPath);
						FreePool(DevPath);
					}
					CurrentDiskDevice=&DiskDevices[i];
				}
			}
		}
		else
		{
			st=EFI_OUT_OF_RESOURCES;
			StdOut->OutputString(StdOut,L"Failed to build list of Disk Devices!\r\n");
		}
		FreePool(HandleBuffer);
	}
	else
		Print(L"Failed to locate Disk I/O handles! Status=0x%p\n",st);
	return st;
}

EFI_STATUS EFIAPI EfiInitialize(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable)
{
	UefiBootServicesTableLibConstructor(ImageHandle,SystemTable);
	UefiRuntimeServicesTableLibConstructor(ImageHandle,SystemTable);
	UefiLibConstructor(ImageHandle,SystemTable);
	DevicePathLibConstructor(ImageHandle,SystemTable);
	StdIn=SystemTable->ConIn;
	StdOut=SystemTable->ConOut;
	return gBS->HandleProtocol(ImageHandle,&gEfiLoadedImageProtocolGuid,&CurrentImage);
}

EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS st=EfiInitialize(ImageHandle,SystemTable);
	if(st==EFI_SUCCESS)
	{
		UINT16 RevHi=(UINT16)(SystemTable->Hdr.Revision>>16);
		UINT16 RevLo=(UINT16)(SystemTable->Hdr.Revision&0xFFFF);
		SetConsoleModeToMaximumRows();
		StdOut->OutputString(StdOut,L"UefiDiskAccess Demo - Simple Demo of Accessing Disks in UEFI\r\n");
		StdOut->OutputString(StdOut,L"Powered by zero.tangptr@gmail.com, Copyright Zero Tang, 2021, All Rights Reserved.\r\n");
		Print(L"UEFI Firmware Vendor: %s Revision: %d.%d\n",SystemTable->FirmwareVendor,RevHi,RevLo);
		st=InitializeDiskIoProtocol();
		if(st==EFI_SUCCESS)
		{
			EnumAllDiskPartitions();
			FreePool(DiskDevices);
		}
		StdOut->OutputString(StdOut,L"Press Enter key to continue...\r\n");
		BlockUntilKeyStroke(L'\r');
	}
	return st;
}