
#include "stdio.h"
#include "stdlib.h"

#include "windows.h"

#include "viruslib_ex.h"

#include "RSStreamLib.h"

DWORD gCount = 0;

DWORD gOffsetNum = SECTION_OFFSET_NUM;

DWORD gOffsetSize = SIZE_Z;

#if 0

DWORD g_offset1[32] ={

59,63, 217,221, 257,261, 277,281,

297,301, 337,341, 377,381, 461,465,

561,565, 791,795, 1771,1775, 1977,1981,//801,831, 

4691,4695, 4699,4703, 4795,4799, 5013,5017


};

DWORD g_offset[20] ={

59, 217, 257, 277,

297, 337, 377, 461,

561, 791, 1771, 1977,//801,831, 

4691, 4697, 4795, 5013,

7699, 9812, 115200, 135023

};


DWORD g_offset2[20] ={

217, 257,277,

297, 355,337,383,397, 591,//561,791,

2179, 4691, 4697,// first is 2179
//791,2599,3755, 3997,

4795, 5013,

5397,5598,//,5628//,5798//,

5798, 6913,
7699,8177//6933

};


BOOL GenerateLibRec( PCHAR filename, PLibRec pRec ){

	HANDLE hFile;
	DWORD value;
	DWORD i = 0;
	DWORD offset = g_offset[i]; 
	DWORD result;
	DWORD num;
	DWORD start = 0; //added 2004-12-01;
	DWORD OffsetSize = 0;

	PeInfo info;
	
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

	memset( &info, 0, sizeof(PeInfo) );//added 2005-03-11

	//added 2004-12-01;...
	if ( VerifyPE( hFile, &info ) == FALSE ){
		
		CloseHandle( hFile ); return FALSE;
	}


	{//new process methord added here 2005-03-11

		DWORD count = gOffsetNum;

		OffsetSize = gOffsetSize;

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

		pRec->id = ++gCount;
			
	}


	if ( pRec->num == 0 ){

		//Sleep(1000);
	}

	CloseHandle( hFile);
	return TRUE;
}

#endif


void InitLibHead( LibHead* pHead ){

	memset( pHead, 0, sizeof(LibHead) );
	memcpy( pHead->Sig, "ZHALFA", 6 );

	pHead->LibType = BASELINELIB;
	pHead->RecordSize = sizeof(LibRec);

}

BOOL WriteLibHead( HANDLE hLib, LibHead* pHead ){
	
	DWORD num = 0;
	BOOL result = FALSE;

	SetFilePointer(hLib, 0, NULL, FILE_BEGIN);

	result= WriteFile( hLib, pHead, sizeof(LibHead), &num, NULL);
		
	return TRUE;
}

HANDLE CreateLibFile( CHAR* path ){

	HANDLE hLib;
	LibHead head;

	hLib = CreateFile(
		
		path,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hLib == (HANDLE)-1 ){  return hLib; }

	InitLibHead( &head );
	WriteLibHead( hLib, &head );
	return hLib;
}

BOOL AddLibRec( HANDLE hLib, PLibRec pRec, PLibHead pLib ){

	DWORD num = 0;
	BOOL result = FALSE;

	SetFilePointer(hLib, 0, NULL, FILE_END);

	result= WriteFile( hLib, pRec, sizeof(LibRec), &num, NULL);

	if( result && (num == sizeof(LibRec) ) ){

		pLib->Count++;
		return TRUE;	
	}

	return FALSE;
}

BOOL CloseLibFile( HANDLE hLib, LibHead* pHead ){

	if ( hLib == (HANDLE) -1 ) return FALSE;

	pHead->Reserved[0] = 4;
	pHead->Reserved[1] = gOffsetNum;
	pHead->Reserved[2] = gOffsetSize;
	
	WriteLibHead( hLib, pHead);

	CloseHandle(hLib);
	return TRUE;
}

