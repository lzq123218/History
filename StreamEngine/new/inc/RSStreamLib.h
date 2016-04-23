
#ifndef RS_STREAMLIB

#define RS_STREAMLIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windows.h"

#include "engbase.h"

#include "viruslib_ex.h"

typedef struct _PeInfoFromFile{

	DWORD  Machine;

	DWORD  SectionNum;

	DWORD  SizeOfCode;
	DWORD  EntryOffset;
	DWORD  SizeOfImage;
	DWORD  SizeOfHeaders;

	DWORD  VirtualAddress;
	DWORD  RawDataSize;
	DWORD  RawDataOffset;

	DWORD  EntryFound;

} PeInfoFromFile;


BOOL VerifyPE( HANDLE hFile , PeInfoFromFile* pInfo );

BOOL GenerateLibRec2( PCHAR filename, PLibRec pRec, DWORD aTotal, DWORD aOffsetSize );


typedef struct _MemAlloc{

	CHAR* pMemBase;

	DWORD BytesAlloced;
	DWORD BytesTotal;

	DWORD AllocTag;
	
	struct statistic{

		DWORD allocBytes;
		DWORD allocTimes;

	}info;

}MemAlloc, *PMemAlloc;


void InitMemAlloc( PMemAlloc aPtr );

BOOL BuildMemAlloc( PMemAlloc aPtr );
void DestroyMemAlloc( PMemAlloc aPtr );

void* MallocMem( PMemAlloc aPtr, DWORD aSize );
void FreeMem( PMemAlloc aPtr, void* aMem );


#define MAX_FILE_LEN	256
typedef long (*FUNPROCESSFILE)( char*, void* );

void BrowseDir( PCHAR aName, FUNPROCESSFILE aFun, void* aPara );
BOOL MakeFullPath( const PCHAR aSrc, DWORD aSize, PCHAR aDes );

#endif