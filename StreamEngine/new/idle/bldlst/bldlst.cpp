// bldlst.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "stdio.h"
#include "stdlib.h"
#include "windows.h"

typedef struct _LogHead{

	DWORD size;
	DWORD id;

}LogHead;

#define NVDIR "nv"
#define VDIR  "v"

void readdata( PCHAR iName, FILE*log, DWORD id ){

	HANDLE hFile;

	DWORD Data[3]; DWORD num = 0, res = 0;

	DWORD offset = 0; DWORD val = 0;

	hFile = CreateFile(
		
		iName,
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == INVALID_HANDLE_VALUE ){

		fprintf( log, "can not open %s\n", iName );
		return;	
	}

	offset = 0x3f0;

	SetFilePointer( hFile, offset, NULL, FILE_BEGIN );

	memset( Data, 0, sizeof(DWORD)*3 );

	res = ReadFile( hFile, Data, sizeof(DWORD)*3, &num, NULL );

	if ( res && (num == sizeof(DWORD)*3) ){

		offset = Data[0] +Data[1] -123;
		SetFilePointer( hFile, offset, NULL, FILE_BEGIN );

		ReadFile( hFile, Data, sizeof(DWORD)*3, &num, NULL );

		if (log ){
		
			fprintf( log, "%.8x  %.8x  %.8x ->%u:%s\n", Data[0], Data[1], Data[2], id, iName );	
		}	
	}
	CloseHandle( hFile);
}

void GetName( const char* in, char* out ){

	CHAR lName[256];
    CHAR lDir[256];

	DWORD len  = 0, cout = 0, size = 0;

	const char* p = NULL;


	len = strlen( in );

	p = &( in[len-1]);

	while( (*p != '\\') && (cout<len) ){
	
		p--; cout++; size++;
	}

	if ( cout < len ){

		memset( lName, 0, 256 );
		memcpy( lName, p+1, size );	

	}else{
	
		return;
	}
	strcpy( out, lName );
}

BOOL GetRecord( HANDLE hFile, PCHAR pName, DWORD BuffSize, DWORD *pVid, DWORD *pIndex ){

typedef struct _MistakeRec{

	DWORD IndexValue;
	DWORD vid;

	CHAR name[374];

}MistakeRec;

	MistakeRec mistake;
	DWORD num, res, len;


	memset( &mistake, 0, sizeof(MistakeRec) );
	memset( pName, 0, BuffSize );

	res = ReadFile( hFile, &mistake, sizeof(MistakeRec), &num, NULL );

	if( res && ( num == sizeof(MistakeRec) ) ){
	
		*pVid = mistake.vid;
		*pIndex = mistake.IndexValue;

		memcpy( pName, mistake.name, strlen(mistake.name) );

		return TRUE;

	}else

	return FALSE;
	
}

CHAR gCurDir[256];

int main(int argc, char* argv[])
{
	FILE *in = NULL, *out = NULL;

	HANDLE hFile, hFileIn;

	CHAR fname[256];
	CHAR filename[256];
	CHAR nname[256];
	DWORD vid = 0;
	DWORD index;
	DWORD nlen = 0, num = 0 , cnt = 0;

	PCHAR SubDir = NULL;

	DWORD total = 0;

	LogHead lghead;

	memset( gCurDir, 0, 256 );

	GetCurrentDirectory( 256, gCurDir );

	//in = fopen( "a.txt", "r" );

	hFileIn = CreateFile(
		
		"mistake.log",
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	out = fopen( "out.txt", "w" );

	if ( hFileIn == INVALID_HANDLE_VALUE ) return 0;

	hFile = CreateFile(
		
		"scanresult.log",
		GENERIC_WRITE, 
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	CreateDirectory( VDIR, NULL );
	CreateDirectory( NVDIR, NULL );

	while ( GetRecord( hFileIn, fname, 256, &vid, &index ) ){

		CHAR dirname[512];

		if ( vid ){

			SubDir = VDIR;		
		}else{

			SubDir = NVDIR;
		}

		sprintf( dirname, "%s\\%.8u", SubDir, index );

		CreateDirectory( dirname, NULL );

		memset( filename, 0, 256 );
		GetName( fname, filename );

		sprintf( nname, "%s\\%s\\%s", gCurDir, dirname, filename );

		if ( CopyFile( fname, nname, TRUE )== TRUE ){

			total++;
		}else{

			DWORD err, i =0; BOOL res;

			err = GetLastError();

			if ( err == 80 ){

				do{

					sprintf( nname, "%s\\%s\\%.8u_%s", gCurDir, dirname, i, filename );

					res = CopyFile( fname, nname, TRUE );
					i++;
				
				}while( res == FALSE );

				if ( res ) total++;
			
			}			

		}

		if ( vid ){// build list

			memset( &lghead, 0, sizeof(LogHead) );

			lghead.id = vid;

			nlen = strlen( nname );
			lghead.size = nlen + sizeof(LogHead);

			WriteFile( hFile, &lghead, sizeof(LogHead), &num, NULL );
			WriteFile( hFile, fname, nlen, &num, NULL );		
		
		}

		//readdata( nname, out, vid );

	}

	printf( "copy %d files\n", total );

	CloseHandle( hFile );
	CloseHandle( hFileIn );
	fclose( out );

	return 0;
}
