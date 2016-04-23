

#ifndef VERIFY_PE

#define VERIFY_PE

#include <stdio.h>
#include <stdlib.h>

#include "windows.h"



typedef struct _PeInfo{

	DWORD  Machine;

	DWORD  SectionNum;
	DWORD  EntryOffset;
	
	DWORD  CodeSize;

	DWORD RawDataSize;
	DWORD RawDataOffset;
	DWORD VirtualAddress;

	DWORD RawEntry;

} PeInfo, *PPeInfo;



BOOL VerifyPE2( HANDLE hFile , PeInfo* pInfo , PIMAGE_DATA_DIRECTORY pArg );


#endif