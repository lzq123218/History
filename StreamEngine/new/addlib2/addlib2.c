// addlib2.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include "viruslib_ex.h"

#include "RSStreamlib.h"

#if 0

int main(int argc, char* argv[]){

	HANDLE hFile;
	HANDLE hLib;
	HANDLE hName;
	CHAR buff[256];

	DWORD total = 0;
	DWORD num = 0;
	BOOL  res = FALSE;

	LibHead head;
	LibRec record;
	NameHead nHead;
	NameRec  nRecord;

	CHAR lFileName[256];
	CHAR lPath[256];

	if( argc != 2 ){
		printf( "commandline: addlib2 e:\\lib\\" );
		return 0;
	}

	InitLibHead( &head);

	memset( lFileName, 0, 256 );
	strcpy( lFileName, argv[1] );
	strcat( lFileName, "statics\\pe_all.zha");

	hFile = CreateFile(
		
		lFileName,
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == (HANDLE)-1 ){
		printf( "Can not find intermediate file\n" );
		return 0;
	}

	memset( lPath, 0, 256 );
	strcpy( lPath, argv[1] );
	strcat( lPath, "data\\");

	memset( lFileName, 0, 256 );
	strcpy( lFileName, lPath );
	strcat( lFileName, "test2.lib");

	hLib = CreateLibFile( lFileName );//"f:\\zlib\\data\\test2.lib"

	memset( lFileName, 0, 256 );
	strcpy( lFileName, lPath );
	strcat( lFileName, "test.nam");

	hName = CreateNameLib( lFileName ); //"f:\\zlib\\data\\test2.nam"

	if ( hLib == (HANDLE)-1 ){
		
		CloseHandle( hFile);
		return 0;
	}

	res = ReadFile( hFile, buff, 256, &num, NULL );

	while( res && (num == 256) ){

		printf( "%s\n", buff ); total++;

		memset( &record, 0, sizeof(LibRec));

		if ( GenerateLibRec( buff, &record) == FALSE ) 
			break;

		memset( &nRecord, 0, sizeof(NameRec) );

		nRecord.id = record.id;
		strcpy( nRecord.name, buff);

		if ( AddLibRec( hLib, &record, &head)== FALSE ) 
			break;

		if ( AddName( hName, &nRecord )== FALSE ) 
			break;
		
		res = ReadFile( hFile, buff, 256, &num, NULL );
	}

	CloseNameLib( hName, &nHead );
	CloseLibFile( hLib, &head );	
	CloseHandle( hFile);

	printf("total =%d\n",total);
	return 0;
}

#else

typedef struct _LogHead{

	DWORD size;
	DWORD id;

}LogHead;

extern DWORD gOffsetNum, gOffsetSize;

int main(int argc, char* argv[]){

	HANDLE hFile = NULL;
	HANDLE hLib;

	CHAR buff[256];

	FILE* log = NULL;

	LogHead loginfo;

	DWORD total = 0;
	DWORD num = 0;
	BOOL  res = FALSE;

	LibHead head;
	LibRec record;

	CHAR lFileName[256];
	CHAR lPath[256];

	DWORD a, b;

	if( argc != 5 ){
		printf( "commandline: addlib2 e:\\lib\\" );
		return 0;
	}

	a = atol( argv[3] ); b = atol( argv[4] );

	gOffsetNum = a; gOffsetSize = b;

	InitLibHead( &head);

	memset( lFileName, 0, 256 );
	strcpy( lFileName, argv[1] );
	strcat( lFileName, "scanresult.log");

	hFile = CreateFile(

		lFileName,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == NULL ){
		printf( "Can not find intermediate file\n" );
		return 0;
	}

	memset( lPath, 0, 256 );
	strcpy( lPath, argv[2] );


	memset( lFileName, 0, 256 );
	strcpy( lFileName, lPath );
	strcat( lFileName, "test2.lib");

	hLib = CreateLibFile( lFileName );//"f:\\zlib\\data\\test2.lib"

	memset( lFileName, 0, 256 );
	strcpy( lFileName, lPath );
	strcat( lFileName, "test.nam");

	strcpy( lFileName, lPath );
	strcat( lFileName, "addlib_log.txt" );

	log = fopen( lFileName, "a+" );

	fprintf( log, "----------------\n" );
	fprintf( log, "------start\n" );
	fprintf( log, "----------------\n" );


	while( ReadFile( hFile, &loginfo, sizeof(LogHead), &num, NULL )  ){

		if ( num != sizeof(LogHead) ) break;

		memset( buff, 0 , 256 );

		res = ReadFile( hFile, buff, (loginfo.size - sizeof(LogHead)), &num, NULL );

		memset( &record, 0, sizeof(LibRec));

		if ( GenerateLibRec2( buff, &record , gOffsetNum, gOffsetSize ) == FALSE ) {

			fprintf( log, "%s\n", buff );

			continue;
		}

		record.VID = loginfo.id;
		strcpy( record.FullName, buff );


		if ( AddLibRec( hLib, &record, &head)== FALSE ) 
			break;

		total++;
		printf("id = %u : %s\n", loginfo.id, buff );
	}

	CloseLibFile( hLib, &head );	
	CloseHandle( hFile);

	fprintf( log, "----------------\n" );
	fprintf( log, "------end\n" );
	fprintf( log, "----------------\n" );

	fclose( log );

	return 0;
}

#endif

