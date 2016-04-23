// engine.cpp : Defines the entry point for the console application.

/*NOTATION*/
/*used only in windows.*/

#include "stdio.h"
#include "windows.h"
#include "string.h"

#include "eng.h"


BOOL Display = FALSE;

#define BLKSIZE 2048//12288

BYTE buff[BLKSIZE];

FILE *g_log, *g_log2;

FILE *g_log4 = NULL;

HANDLE gLog3 = INVALID_HANDLE_VALUE;

DWORD ScanTimes = 0;
DWORD MaxTimeUsed = 0 ;

LONG StreamScan( HANDLE hFile, DWORD protocol, DWORD *aRet ){

	BOOL result = FALSE;
	BOOL FirstRead = TRUE;
	DWORD actual;
	DWORD context = 0;
	LONG value = 0;

	DWORD vId = 0;

	RSConnectEvent( protocol, &context );//changed 2004-10-28

	ScanTimes = 0;

	do{
		result = ReadFile( hFile, buff, BLKSIZE, &actual, NULL);

		if (  result && FirstRead ){

			FirstRead = FALSE;
			//if ( *((PSHORT)buff) != (SHORT)0x5a4d ) break;		
		}

		value = RSDataScan( context, buff, actual, &vId );

		ScanTimes++;

		if ( (value != -1)&&( protocol == 0 ) )break;

		if ( (value != -1)&&( protocol == 9) ){

			if ( value == 0 ){

				if ( g_log ) fprintf( g_log, "Attachment: NO Virus\n" );
			
			}else if ( value > 0){
				
				if ( g_log ) fprintf( g_log, "Attachment: Found Virus id = %d\n", value );
			}
		}

	}while( actual == BLKSIZE );


	RSDisconnectEvent( context );

	if ( vId > 0  ) *aRet = value;
	return vId;
}

DWORD VirusFound = 0;


DWORD NoName( const PCHAR pName, DWORD vid, DWORD protocol ){

	HANDLE hFile;
	LONG res = 0;
	DWORD TimeUsed = 0;
	WORD head = 0;
	DWORD num = 0;
	DWORD index = 0;

	hFile = CreateFile(
		
		pName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if( hFile == (HANDLE)-1){
		printf( "CAN NOT OPNE FILE: %s\n",pName );
		return 0;
	}

	if ( Display ) printf( "%s\n", pName );

	ResetComparedTimes();

	TimeUsed = GetTickCount();

	ReadFile( hFile, &head, 2, &num, NULL );

	SetFilePointer( hFile, 0, NULL, FILE_BEGIN );

	if( head == 0x5a4d ){

		res = StreamScan( hFile, 0 , &index );

		TimeUsed = GetTickCount() - TimeUsed;

		if ( TimeUsed > MaxTimeUsed ){

			MaxTimeUsed = TimeUsed;	
		}

		if ( res > 0 ){

			if ( g_log4 &&( res == UNKNOWN_VIRUS)){

				fprintf( g_log4, "%d %s\n", res, pName );
				
			} 

			if ( g_log ){

				fprintf( g_log, "filename: %s\n", pName );
				fprintf( g_log, "Virus Found: id = %d\n", res );

				fprintf( g_log, "index = %u\n", index );
				//fprintf( g_log, "Time used is %d millisecond\n", TimeUsed );
				
				//fprintf( g_log, "ScanTimes = %d\n", ScanTimes );
				//fprintf( g_log, "ComparedTimes = %d\n", GetComparedTimes() );
			}

			if ( (gLog3 != INVALID_HANDLE_VALUE) && index ){

				MistakeRec tmp ;
				DWORD num = 0;

				memset( &tmp, 0, sizeof(MistakeRec) );

				tmp.IndexValue = index;

				if ( vid ){
				
					tmp.vid = vid;
				}

				strcpy( &tmp.name, pName );

				WriteFile( gLog3, &tmp, sizeof(MistakeRec), &num, NULL );
	
			}

			VirusFound ++;

		}else{

			if ( g_log2 ){

				//fprintf( g_log2, "ScanTimes = %d\n", ScanTimes );
				//fprintf( g_log2, "ComparedTimes = %d\n", GetComparedTimes() );
				fprintf( g_log2, "not_found: %s\n", pName );
			}
		}

	}else{

		//res = StreamScan( hFile, 9 );
	}

	CloseHandle( hFile );
	return 0;
}



DWORD ScanFileList( PCHAR filelist ){


typedef struct _LogHead{

	DWORD size;
	DWORD id;

}LogHead;

	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD num = 0, res = 0, total = 0;

	LogHead loginfo;

	static CHAR buff[256];


	hFile = CreateFile(
		
		filelist,
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == INVALID_HANDLE_VALUE ) return FALSE;


	while( ReadFile( hFile, &loginfo, sizeof(LogHead), &num, NULL )  ){

		if ( num != sizeof(LogHead) ) break;

		memset( buff, 0 , 256 );

		res = ReadFile( hFile, buff, (loginfo.size - sizeof(LogHead)), &num, NULL );

		loginfo.id;

		NoName( buff, loginfo.id, 0 );

		total++;

	}


	CloseHandle( hFile );
	return TRUE;
}


#define BROWSE_BUFF_SIZE 2048

DWORD BrowseDirErr = 0;

DWORD total_file = 0;
DWORD total_dir = 0;
DWORD total_pe = 0;

#define  ERR_MEM_ALLOC       119
#define  ERR_FILE_OPEN       117
#define  ERR_INVALID_HANDLE  115
#define  ERR_INVALID_PARAM   113
#define  ERR_BUFF_SMALL      111

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

		total_file ++;
		NoName( name, 0, 0 );
	}

	FindClose( hFind);
	free( name );
}

CHAR DATAPATH[512];

CHAR Options[64];



int main( int argc, char* argv[] ){

	DWORD res = 0, start = 0, version = 0;
	DOUBLE time = 0;

	if ( argc != 4 ){
		
		printf( "usage example: transplant  f:\\scannow\\  f:\\\n");
		printf( "               transplant  f:\\scannow\\  all\n" );
		return 0;	
	}

	memset( DATAPATH, 0, 512 );
	strcpy( DATAPATH, argv[1] );

	strcpy( Options, argv[3] );
	
	if ( strchr( Options, 'd') ) Display = TRUE;

	if ( strchr( Options, 'l') ) {

		if ( strchr( Options, 'f') ){

			memset( DATAPATH, 0, 512);
			strcpy( DATAPATH, argv[1] );
			strcat( DATAPATH, "scan_result.log");

			g_log = fopen( DATAPATH, "w" );
		}
		if ( strchr( Options, 'n') ){

			memset( DATAPATH, 0, 512 );
			strcpy( DATAPATH, argv[1] );
			strcat( DATAPATH, "not_found.log");

			g_log2 = fopen( DATAPATH, "w" );
		}
#if 1

		memset( DATAPATH, 0, 512 );
		strcpy( DATAPATH, argv[1] );
		strcat( DATAPATH, "44444.log");

		g_log4 = fopen( DATAPATH, "w" );
#endif

	}
	if ( strchr( Options, 'm') ){

		memset( DATAPATH, 0, 512 );
		strcpy( DATAPATH, argv[1] );
		strcat( DATAPATH, "mistake.log");
	
	
		gLog3 = CreateFile(
			
			DATAPATH,
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
	}

	version = RSGetEngineVersion();

	printf( "Stream Engine V %d.%d\n", version>>8, version&0xff );
	//printf( "%d\n", sizeof(struct _StreamObject) );

	RSEngineInit();

	//RSRegisterCallback( fun, &time );
	strcpy( DATAPATH, argv[1] );			
	strcat( DATAPATH, "PeerArray.img" );
	res = RSLoadLib( DATAPATH , 0 );

	if ( res == 0 ){

		printf( "LIB LOAD ERROR\n" );
		return 0;
	}

	printf( "Virus Scanning ...\n" );
	start = GetTickCount();

	if ( strchr( Options, 'z' ) ){

		ScanFileList( argv[2] );
	
	}else{

		if( strcmp( argv[2], "all" ) == 0 ){

			BrowseDir( "L:\\", 0);
			BrowseDir( "K:\\", 0);
			BrowseDir( "J:\\", 0);
			BrowseDir( "I:\\", 0);
			BrowseDir( "H:\\", 0);

			BrowseDir( "g:\\", 0);
			BrowseDir( "f:\\", 0);
			BrowseDir( "e:\\", 0);
			BrowseDir( "d:\\", 0);
			BrowseDir( "c:\\", 0);

		}else{

			memset( DATAPATH, 0, 512 );
			strcpy( DATAPATH, argv[2] );

			BrowseDir( DATAPATH, 0 );	
		}
	}

	time = ( GetTickCount() - start )/(1000.00);

	{
		if ( g_log ){

			fprintf( g_log, "VirusFound = %d\n", VirusFound );

			fprintf( g_log, "time used = %f\n", time );
			fprintf( g_log, "file scaned = %d\n", total_file );
			fprintf( g_log, "Max Time Used In Scaning A Single file is %d milliseconds\n", MaxTimeUsed);
		
			fclose( g_log );
		}

		if ( g_log2 ) fclose( g_log2 );

		if ( gLog3 != INVALID_HANDLE_VALUE ) CloseHandle( gLog3 );

		if ( g_log4 ) fclose( g_log4 );
	}

	RSUnloadLib();

	printf("result = %d\n", gCount);

	//RSDeregisterCallback();
	RSEngineUninit();
	return 0;
}
