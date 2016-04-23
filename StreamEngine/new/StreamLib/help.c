
#include "RSStreamLib.h"

/* function used to check whether a pe is valid. if valid get its infomation */

BOOL VerifyPE( HANDLE hFile , PeInfoFromFile* pInfo ){

	IMAGE_DOS_HEADER mz;
	IMAGE_NT_HEADERS nthd;

	DWORD num = 0;
	DWORD offset = 0;
	BOOL result = FALSE;

	DWORD i = 0;
	DWORD size = 0;

	DWORD lMinimal = 0xffffffff;

	PIMAGE_SECTION_HEADER pSectionInfo = NULL;
	PIMAGE_SECTION_HEADER pSection = NULL;

	memset( pInfo, 0, sizeof(PeInfoFromFile) );

	SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
	result = ReadFile( hFile, &mz, sizeof( IMAGE_DOS_HEADER), &num, NULL);

	if ( (!result) || (num != sizeof(IMAGE_DOS_HEADER)) ){
	
		return FALSE;
	}

	if( mz.e_magic != 0x5a4d ){

		//printf("not PE\n");
		return FALSE;
	}

	offset = mz.e_lfanew;
	memset( &nthd, 0, sizeof(IMAGE_NT_HEADERS) );

	SetFilePointer( hFile, offset, NULL, FILE_BEGIN );
	result = ReadFile( hFile, &nthd, sizeof(IMAGE_NT_HEADERS), &num, NULL);

	if ( (!result) || (num != sizeof(IMAGE_NT_HEADERS)) ){
	
		return FALSE;
	}

	if( nthd.Signature != 0x4550l ){

		//printf("not PE\n");
		return FALSE;
	}

	offset = nthd.OptionalHeader.SizeOfHeaders;

	//added 2005-01-12

	pInfo->Machine = nthd.FileHeader.Machine;
	pInfo->SectionNum = nthd.FileHeader.NumberOfSections;

	pInfo->SizeOfCode  = nthd.OptionalHeader.SizeOfCode;	
	pInfo->EntryOffset = nthd.OptionalHeader.AddressOfEntryPoint;

	pInfo->SizeOfImage  = nthd.OptionalHeader.SizeOfImage;
	pInfo->SizeOfHeaders = nthd.OptionalHeader.SizeOfHeaders;

	size = sizeof(IMAGE_SECTION_HEADER)* pInfo->SectionNum;

	pSection = ( pSectionInfo = (PIMAGE_SECTION_HEADER)malloc( size ) );

	offset = mz.e_lfanew + 0x18l + nthd.FileHeader.SizeOfOptionalHeader;

	SetFilePointer( hFile, offset, NULL, FILE_BEGIN );

	result = ReadFile( hFile, pSectionInfo, size, &num, NULL);

	if( (result == FALSE)||(size != num) ){

		free( pSection );
		return FALSE;
	}

	for( i = 0 ; i < pInfo->SectionNum ; i++, pSectionInfo++ ){

		if ( lMinimal > pSectionInfo->VirtualAddress ){
		
			lMinimal = pSectionInfo->VirtualAddress;
		}

		if ( 
			( pInfo->EntryOffset >= pSectionInfo->VirtualAddress )
			&&
			( pInfo->EntryOffset < (pSectionInfo->VirtualAddress+ pSectionInfo->SizeOfRawData) )	
		){

			pInfo->EntryFound = ( i+1 ) ;
			
			pInfo->VirtualAddress = pSectionInfo->VirtualAddress;

			if ( pSectionInfo->SizeOfRawData && pSectionInfo->PointerToRawData ){

				pInfo->RawDataSize = pSectionInfo->SizeOfRawData;
				pInfo->RawDataOffset = pSectionInfo->PointerToRawData;
			}
			break;	
		}

		if ( pSectionInfo->SizeOfRawData ){
		
			if ( (i ==1)&&(pInfo->EntryOffset < 0x200l) ){

				pInfo->RawDataSize = pSectionInfo->SizeOfRawData;
				pInfo->RawDataOffset = pSectionInfo->PointerToRawData;
				break;
			}
		}
		
		if ( pSectionInfo->SizeOfRawData ){

			if ( (i == 0)&&( pInfo->EntryOffset < pSectionInfo->VirtualAddress) ){
			
				if ( pInfo->EntryOffset >= 0x200l ){

					pInfo->RawDataSize = pSectionInfo->SizeOfRawData;
					pInfo->RawDataOffset = pSectionInfo->PointerToRawData;
					break;	
				}
			}		
		}

	}

	free( pSection );
	pSection = NULL;

	if ( pInfo->EntryOffset < lMinimal ){

	//	printf( "\nthere is a hole\n" );		
	}

	if ( pInfo->EntryFound == pInfo->SectionNum ){
	
	//	if ( pInfo->VirtualAddress != lMinimal ) printf( " may be a virus\n\n");
	}

	if ( pInfo->RawDataSize == 0 ) return FALSE;

	
	return TRUE;
}

/* function used to generate a lib-record for an pe */

BOOL GenerateLibRec2( PCHAR filename, PLibRec pRec, DWORD aTotal, DWORD aOffsetSize ){

	HANDLE hFile;
	DWORD value;
	DWORD i = 0;
	DWORD offset = 0; 
	DWORD result;
	DWORD num;
	DWORD start = 0; //added 2004-12-01;
	DWORD OffsetSize = 0;

	PeInfoFromFile info;

	memset( pRec, 0, sizeof(LibRec) );
	
	hFile = CreateFile(
		
		filename,
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == (HANDLE)-1 ){	return FALSE; }

	memset( &info, 0, sizeof(PeInfoFromFile) );//added 2005-03-11

	//added 2004-12-01;...
	if ( VerifyPE( hFile, &info ) == FALSE ){
		
		CloseHandle( hFile ); return FALSE;
	}


	{//new process methord added here 2005-03-11

		DWORD count = aTotal;

		OffsetSize = aOffsetSize;

		pRec->num = 0;


		pRec->arry[pRec->num].value = info.SectionNum;
		pRec->arry[pRec->num].offset = (PE_INFO_OFFSET_PREFIX | PE_INFO_NUMBER_OF_SECTIONS);
		pRec->num ++;


		pRec->arry[pRec->num].value = info.EntryOffset;
		pRec->arry[pRec->num].offset = (PE_INFO_OFFSET_PREFIX | PE_INFO_ADDRESS_OF_ENTRY);
		pRec->num ++;


		pRec->arry[pRec->num].value = info.RawDataSize;
		pRec->arry[pRec->num].offset = (PE_INFO_OFFSET_PREFIX | PE_INFO_SIZE_OF_RAWDATA);
		pRec->num ++;


		pRec->arry[pRec->num].value = info.RawDataOffset;
		pRec->arry[pRec->num].offset = (PE_INFO_OFFSET_PREFIX | PE_INFO_POINTER_TO_RAWDATA);
		pRec->num ++;


		for( i = 0; i < count; i++ ){

			//offset = info.RawDataOffset + (info.RawDataSize / count)* i;

			offset = info.RawDataOffset
			+ (info.RawDataSize / (count / OffsetSize))*( i / OffsetSize)+( i % OffsetSize )*4;

			result = SetFilePointer( hFile, offset, NULL, FILE_BEGIN );

			result = ReadFile( hFile, &value, sizeof(DWORD), &num, NULL);

			if ( (result == FALSE) || (num != sizeof(DWORD)) ){
				break;
			}

			pRec->arry[pRec->num].value = value;

			pRec->arry[pRec->num].offset =(SECTION_OFFSET_PREFIX | i);// offset need special process

			pRec->num ++;

		}
			
	}

	if ( pRec->num == 0 ){

		//Sleep(1000);
	}

	CloseHandle( hFile);
	return TRUE;
}

/* functions used to build a continuous datablock */

void InitMemAlloc( PMemAlloc aPtr ){

	if ( aPtr == NULL )return;

	aPtr->pMemBase = NULL;
	aPtr->BytesAlloced = 0;
	aPtr->BytesTotal = 0;

	aPtr->AllocTag = 0;
	aPtr->info.allocBytes = 0;
	aPtr->info.allocTimes = 0;

}

BOOL BuildMemAlloc( PMemAlloc aPtr ){
	
	if ( aPtr == NULL )return FALSE;

	if( ( aPtr->info.allocBytes != 0 )&& (aPtr->info.allocTimes == 0) ){
	
		aPtr->pMemBase = (CHAR*)malloc( aPtr->info.allocBytes );

		if( aPtr->pMemBase ){
		
			aPtr->BytesTotal = aPtr->info.allocBytes;
			aPtr->AllocTag = 1;

			return TRUE;
		}
	}
	return FALSE;
}

void DestroyMemAlloc( PMemAlloc aPtr ){

	if ( aPtr == NULL )return;

	if ( aPtr->pMemBase ){ free(aPtr->pMemBase); }

	aPtr->pMemBase = NULL;
	aPtr->BytesAlloced = 0;
	aPtr->BytesTotal = 0;

	aPtr->AllocTag = 0;
	aPtr->info.allocBytes = 0;
	aPtr->info.allocTimes = 0;
}

void* MallocMem( PMemAlloc aPtr, DWORD aSize ){

	void* p = NULL;

	if ( aPtr == NULL) return NULL;

	if ( aPtr->AllocTag == 0 ){
	
		p = malloc( aSize );

		if ( p ){
		
			aPtr->info.allocBytes += aSize;
			aPtr->info.allocTimes++;
		}

	}else{

		if ( aPtr->pMemBase && (aPtr->BytesAlloced < aPtr->BytesTotal) ){
		
			p = (void*)(aPtr->pMemBase + aPtr->BytesAlloced);
			aPtr->BytesAlloced += aSize;
		}
	
	}

	return p;
}

void FreeMem( PMemAlloc aPtr, void* aMem ){

	if ( aPtr == NULL ) return;

	if ( aPtr->AllocTag == 0 ){ 
		
		free(aMem); 
		aPtr->info.allocTimes--;

	}else{
				
	}
}

void BrowseDir( PCHAR aName, FUNPROCESSFILE aFun, void* aPara ){
	
	DWORD len = 0; CHAR* name = NULL;

	WIN32_FIND_DATAA info;
	HANDLE hFind;

	if ( aName == NULL ){ return; }

	len = strlen( aName ) + MAX_FILE_LEN + 6;

	name = (CHAR*)malloc( len );

	if ( name == NULL ){  return; }

	sprintf( name, "%s%s", aName, "*.*" );

	hFind = FindFirstFile( name, &info);

	if ( hFind == INVALID_HANDLE_VALUE ){

		free( name ); return;
	}

	do{

		if ( !strcmp( info.cFileName, ".") ){  continue; }
		if ( !strcmp( info.cFileName, "..") ){  continue; }

		if ( 
			info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
		){
		
			sprintf( name, "%s%s\\", aName, info.cFileName );

			BrowseDir( name, aFun, aPara );
			continue;
		}

		sprintf( name, "%s%s", aName, info.cFileName );
		//printf( "%s\n", name );

		if ( aFun ){

			aFun( name, aPara );
		}

	}while ( FindNextFile( hFind, &info) );

	FindClose( hFind);
	free( name );
}

int BuildRaw( PCHAR aSourceDir, PCHAR aWorkingDir ){


}

BOOL MakeFullPath( const PCHAR aSrc, DWORD aSize, PCHAR aDes ){

	DWORD len = 0; BOOL ret = FALSE;
	
	if ( aSrc && aDes && aSize ){

		memset( aDes, 0, aSize );

		len = strlen( aSrc );
		
		if ( aSrc[len-1] == '\\' ){

			if ( aSize > len ) {

				strcpy( aDes, aSrc ); ret = TRUE;
			}

		}else{

			if ( aSize > len+1 ){

				strcpy( aDes, aSrc); strcat( aDes, "\\" );
				ret = TRUE;
			}
		}
	
	}

	return ret;	
}

