
#include "stdio.h"
#include "stdlib.h"

#include "windows.h"

#include "..\inc\help.h"

/**
**  record current last error when BrowseDir function execute
**/

DWORD BrowseDirErr = 0;
DWORD g_abnormal = 0;

#define BROWSE_BUFF_SIZE 2048

DWORD total_file = 0;
DWORD total_dir = 0;
DWORD total_pe = 0;

void ShowResult(){

	printf( "total_file : %d\n", total_file );
	printf( "total_dir : %d\n", total_dir );
	printf( "total_pe : %d\n", total_pe );

	printf( "abnormal PE file:%d\n", g_abnormal );

	printf( "BrowseDirErr = %d\n", BrowseDirErr );

}

//#define PATHNAME "f:\\zlib\\statics\\"

CHAR PATHNAME[256];

PCHAR surf[] = {

	"not_pe.zha",
	"pe_all.zha",
	"pe_big.zha",
	"pe_1k.zha",
	"pe_2k.zha",
	"pe_nk.zha",
	"abnormal.zha"

};

void EmptyFile(){

	int i = 0;
	HANDLE hFile;
	CHAR filename[128];

	for( i= 0; i < 7; i++ ){
		
		memset( filename, 0, 128 );
		strcpy( filename, PATHNAME );
		strcat( filename, surf[i] );

		hFile = CreateFile(
			
			filename,
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		CloseHandle( hFile );	
	}

}

static
void noName( const CHAR* name, DWORD SizeL, DWORD SizeH ){

	CHAR filename[128];
	HANDLE hFile;
	DWORD num = 0;
	
	memset( filename, 0, 128 );
	
	strcpy( filename, PATHNAME );

	if ( SizeH ){
		
		strcat( filename, surf[2]);

	}else{
	
		if ( SizeL <= 4096 ){

			strcat( filename, surf[3]);
		
		}else if( (SizeL >= 4096)&&( SizeL <= 8192) ){

			strcat( filename, surf[4]);
		
		}else{

			strcat( filename, surf[5]);
		}	
	}

	hFile = CreateFile(
		
		filename,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	SetFilePointer( hFile, 0, NULL, FILE_END);

	WriteFile( hFile, name, 256, &num, NULL );

	CloseHandle( hFile );


	memset( filename, 0, 128 );
	strcpy( filename, PATHNAME );
	strcat( filename, surf[1]);

	hFile = CreateFile(
		
		filename,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	SetFilePointer( hFile, 0, NULL, FILE_END);

	WriteFile( hFile, name, 256, &num, NULL );

	CloseHandle( hFile );
}

FILE* gOut = NULL;

#define LEN 60

void classify( const CHAR* name, DWORD SizeL, DWORD SizeH ){

	HANDLE htmp;
	DWORD start = 0;
	DWORD num = 0 ;

	PeInfo info;

	htmp = CreateFile(
		
		name,
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	memset( &info, 0, sizeof(PeInfo) );

	if ( VerifyPE( htmp, &info ) ){

		DWORD offset = 0 ;		
		UCHAR buff[LEN];

		// this is orignal
		total_pe++;
		noName( name, SizeL, SizeH);


		offset = info.RawDataOffset + (info.EntryOffset - info.VirtualAddress  );
		//offset = info.EntryOffset;

		num = SetFilePointer( htmp, offset, NULL, FILE_BEGIN );
		
		memset( buff, 0, LEN );

		ReadFile( htmp, buff, LEN ,&num, NULL );

		if ( 
			( buff[0] != 0x55 )|| ( buff[1] != 0x8b )
			||
			( buff[2] != 0xec ) 			
		){
		
			CHAR filename[128];
			HANDLE hFile;

			memset( filename, 0, 128 );
			strcpy( filename, PATHNAME );
			strcat( filename, surf[6]);

			hFile = CreateFile(
				
				filename,
				GENERIC_READ | GENERIC_WRITE, 
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			SetFilePointer( hFile, 0, NULL, FILE_END);

			WriteFile( hFile, name, 256, &num, NULL );

			CloseHandle( hFile );


		if ( gOut ){

			DWORD i = 0;

			fprintf( gOut , "%s\n\n", name );

			for( i= 0; i< LEN; i++ ){

				if ( buff[i] > 0xf ){

					fprintf( gOut, "%x", buff[i] );
				}else{
					fprintf( gOut, "0%x", buff[i] );
				
				}
			}
			fprintf( gOut, "\n" );

				if ( info.EntryFound == info.SectionNum ){
	
					 fprintf( gOut, " may be a virus\n\n");
				}

		}


			g_abnormal++;
		}

	}else{

		HANDLE hFile;
		CHAR filename[128];
		DWORD num = 0;

		memset( filename, 0, 128 );
		strcpy( filename, PATHNAME );
		strcat( filename, surf[0]);

		hFile = CreateFile(
			
			filename,
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		SetFilePointer( hFile, 0, NULL, FILE_END);

		WriteFile( hFile, name, 256, &num, NULL );

		CloseHandle( hFile );

		//printf("file is not PE\n");
	}

	CloseHandle( htmp );
}

void BrowseDir( PCHAR pName, void* para ){

	WIN32_FIND_DATAA info;
	HANDLE hFind;

	CHAR* name = NULL;
	CHAR* newdir = NULL;

	name = (CHAR*)malloc( BROWSE_BUFF_SIZE );

	if ( name == NULL ){ BrowseDirErr = ERR_MEM_ALLOC; return; }

	memset( name, 0, BROWSE_BUFF_SIZE );
	strcpy( name, pName );
	strcat( name, "*.*" );

	hFind = FindFirstFile( name, &info);

	if ( hFind == (HANDLE)-1 ){	
		
		BrowseDirErr = ERR_FILE_OPEN;
		free( name ); return;
	}

	total_dir ++;

	while ( FindNextFile( hFind, &info) ){
		
		if ( !strcmp( info.cFileName, "..") ){  continue; }

		if ( 
			info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
		){
		
			newdir = (CHAR*)malloc( BROWSE_BUFF_SIZE );

			if( newdir == NULL ){  BrowseDirErr = ERR_MEM_ALLOC;  break; }

			memset( newdir, 0, BROWSE_BUFF_SIZE );
			strcpy( newdir, pName );
			strcat( newdir, info.cFileName );
			strcat( newdir, "\\" );

			BrowseDir( newdir, para );

			free( newdir );
			newdir = NULL;
			continue;
		}

		memset( name, 0, 1024);
		strcpy( name, pName );
		strcat( name, info.cFileName);

//added new code here
		printf( " %s\n", name);
		total_file ++;

		classify( name, info.nFileSizeLow, info.nFileSizeHigh );

	}

	FindClose( hFind);
	free( name );
}

void MakePath( PCHAR des, const CHAR* src ){

	int len = strlen(src);
	int i = 0;

	while( i < len ){

		if (*src =='\\'){

			*des = *src; des++;
			*des = *src;
			des++; src++;
		
		}else{

			*des = *src;
			des++; src++;
		}
		i++;
	}
}


BOOL VerifyPE( HANDLE hFile , PeInfo* pInfo ){

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

	memset( pInfo, 0, sizeof(PeInfo) );

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

	ReadFile( hFile, pSectionInfo, size, &num, NULL);

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

		if ( pInfo->EntryOffset < 0x200 ){
		
			if ( (i ==1)&& pSectionInfo->SizeOfRawData ){

				pInfo->RawDataSize = pSectionInfo->SizeOfRawData;
				pInfo->RawDataOffset = pSectionInfo->PointerToRawData;
				break;
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

