#include "stdio.h"
#include "stdlib.h"

#include "windows.h"

//#include "..\addlib2\viruslib_ex.c"

#include "verifype.h"


DWORD CvrtVirtual2Real( DWORD aVirtual, PIMAGE_SECTION_HEADER apSec, DWORD aSecCnt ){

	DWORD i, ret =0;

	for( i =0; i< aSecCnt; i++, apSec++){

		if ( apSec->SizeOfRawData == 0 )continue;

		if ( (apSec->VirtualAddress<= aVirtual)
			&&
			 ((apSec->VirtualAddress+apSec->SizeOfRawData) > aVirtual )
		){
			ret = aVirtual - apSec->VirtualAddress + apSec->PointerToRawData;
		}

	}
	return ret;
}

PIMAGE_NT_HEADERS VerifyPe( LPVOID aStart ){

	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)aStart;
	PIMAGE_NT_HEADERS pNt =  NULL;

	if ( pDos == NULL ) return NULL;

	if ( pDos->e_magic == 0x5a4d ){
	
		pNt = (PIMAGE_NT_HEADERS)( pDos->e_lfanew + (DWORD)aStart );
	}

	if ( pNt->Signature == 0x4550 ){
	
		return pNt;
	}
	return NULL;
}

DWORD ImportFun( LPVOID aStart, PIMAGE_IMPORT_DESCRIPTOR pDes, PIMAGE_SECTION_HEADER apSec, DWORD aSecCnt){

	PIMAGE_IMPORT_BY_NAME *ppItem = NULL;

	PCHAR p = NULL;
	DWORD total = 0;

	DWORD offset = 0;

	if ( pDes->Name == 0 ) return 0;

	p = (PCHAR)((DWORD)aStart + CvrtVirtual2Real(pDes->Name, apSec, aSecCnt) );

	printf( "Module: %s\n", p ); 

#if 0
	if ( pDes->OriginalFirstThunk ){
	
		offset = pDes->OriginalFirstThunk;
	}else{
	
		if ( pDes->FirstThunk ){

			offset =  pDes->FirstThunk;
		}else{
			
			return 0;
		}
	}
#endif
	offset =  pDes->FirstThunk;

	ppItem = (PIMAGE_IMPORT_BY_NAME*)((DWORD)aStart + CvrtVirtual2Real(offset, apSec, aSecCnt));

	while( *ppItem ){

		total++;

		if ( ( IMAGE_ORDINAL_FLAG32 & (DWORD)(*ppItem) )== 0 ){

			p = (PCHAR)((DWORD)aStart + CvrtVirtual2Real( (*ppItem)->Name, apSec, aSecCnt));
			printf( "     %s\n", p );
		}

		ppItem ++;	
	}
	
	return total;
}

void importProcess(  LPVOID aStart, PIMAGE_DATA_DIRECTORY pDD, PIMAGE_SECTION_HEADER apSec, DWORD aSecCnt ){

	PIMAGE_DATA_DIRECTORY pImport = pDD +1;
	PIMAGE_IMPORT_DESCRIPTOR pDes = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)aStart + CvrtVirtual2Real( pImport->VirtualAddress, apSec, aSecCnt ));

	IMAGE_IMPORT_DESCRIPTOR DesNULL;

	DWORD total = 0;

	memset( &DesNULL, 0, sizeof(IMAGE_IMPORT_DESCRIPTOR) );

	while( memcmp( pDes, &DesNULL, sizeof(IMAGE_IMPORT_DESCRIPTOR)) ){

		total += ImportFun( aStart, pDes, apSec, aSecCnt );
	
		pDes++;
	}
	
	printf( "Import total = %d\n", total );	
}

BOOL GetPeInfo( LPVOID aStart , PPeInfo apInfo ){

	PIMAGE_NT_HEADERS pNt = NULL;
	PIMAGE_SECTION_HEADER pSec = NULL, peSec = NULL;

	PIMAGE_DATA_DIRECTORY pDD = NULL;

	DWORD i = 0;

	pNt = VerifyPe( aStart );

	if ( pNt == NULL ) return FALSE;

	apInfo->Machine = pNt->FileHeader.Machine;

	apInfo->SectionNum = pNt->FileHeader.NumberOfSections;

	apInfo->CodeSize  = pNt->OptionalHeader.SizeOfCode;

	apInfo->EntryOffset = pNt->OptionalHeader.AddressOfEntryPoint;

	pDD = pNt->OptionalHeader.DataDirectory;

	peSec = (PIMAGE_SECTION_HEADER)(pNt + 1);

	importProcess( aStart, pDD, peSec, apInfo->SectionNum );

	for( i = 0, pSec= peSec ; i < apInfo->SectionNum ; i++,pSec ++ ){

		if ( 
			( apInfo->EntryOffset >= pSec->VirtualAddress )
			&&
			( apInfo->EntryOffset <= (pSec->VirtualAddress+ pSec->SizeOfRawData) )	
		){
		
			apInfo->RawDataSize = pSec->SizeOfRawData;
			apInfo->RawDataOffset = pSec->PointerToRawData;
			apInfo->VirtualAddress = pSec->VirtualAddress;
			
		}	
	}
	
	return TRUE;
}

BOOL GetData( LPVOID aStart, DWORD aOffset, DWORD aSize ){
	
	HANDLE hFile;
	DWORD res = 0, num = 0;

	hFile = CreateFile(

		"data.bin",
		GENERIC_READ|GENERIC_WRITE, 
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == INVALID_HANDLE_VALUE ) return FALSE;

	res = WriteFile( hFile, (PVOID)((DWORD)aStart +aOffset), aSize, &num, NULL );

	CloseHandle( hFile );

	return TRUE;
}

void MapPE( PCHAR aName ){

	HANDLE hFile, hMap;
	LPVOID MapBase = NULL;
	PeInfo info;

	hFile = CreateFile(
		
		aName,
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == INVALID_HANDLE_VALUE ){
	
		return ;
	}
	
	hMap = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );

	if ( hMap == INVALID_HANDLE_VALUE ){
	
		return;
	}

	MapBase = MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );

	if ( MapBase ){

		GetPeInfo( MapBase, &info );
		GetData( MapBase, info.EntryOffset- info.VirtualAddress +info.RawDataOffset , 1024 );

		UnmapViewOfFile( MapBase );
	}

	CloseHandle( hMap );
	CloseHandle( hFile );
}





char buff[128];

int main( int argc, char* argv[] ){
	

	if ( argc != 2 ){ return 0 ; }

	//printf( "%x\n", sizeof(IMAGE_NT_HEADERS32) );

	//printf( "%x\n", sizeof(IMAGE_SECTION_HEADER) );

	MapPE( argv[1] );


	return 0;
}