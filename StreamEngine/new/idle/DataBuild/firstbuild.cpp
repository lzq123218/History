
#include "RSStreamLib.h"
#include "BuildDataBase.h"

/* function used to check whether a pe is valid. if valid get its infomation */

extern DWORD gOffsetNum, gOffsetSize;

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


struct _Parameter{

	HANDLE hLib;
	DWORD total;
	FILE* log;
	LibHead* libhd;
	OUTPUT output;
	PVOID  pdata;
};

long ProcessAFile( PCHAR aName, void* aPara ){

		struct _Parameter *p = (struct _Parameter*)aPara;

		LibRec record;
		
		DWORD res;

		memset( &record, 0, sizeof(LibRec));

		if ( GenerateLibRec2( aName, &record , gOffsetNum, gOffsetSize ) == FALSE ) {

			fprintf( p->log, "%s\n", aName );
		}

		record.VID = ++p->total;
		strcpy( (PCHAR)record.FullName, aName );

		res = AddLibRec( p->hLib, &record, p->libhd );

		if( p->output && p->pdata ) p->output( p->pdata, aName, res );

		return res;
}

int FirstBuild( OUTPUT aFun, PVOID aPv, PCHAR aSourceDir, PCHAR aWorkingDir ){

	HANDLE hLib;
	FILE* log = NULL;
	LibHead head;
	CHAR lFileName[512];
	CHAR tSourceDir[512];

	struct _Parameter ProcessParameter;

	gOffsetNum = 7; gOffsetSize = 1;

	InitLibHead( &head);

	sprintf( lFileName, "%s\\test2.lib", aWorkingDir );

	hLib = CreateLibFile( lFileName );//"f:\\zlib\\data\\test2.lib"

	
	sprintf( lFileName, "%s\\addlib_log.txt", aWorkingDir );

	log = fopen( lFileName, "w" );

	fprintf( log, "----------------\n" );
	fprintf( log, "------start\n" );
	fprintf( log, "----------------\n" );

	ProcessParameter.hLib = hLib;
	ProcessParameter.libhd = &head;
	ProcessParameter.log = log;
	ProcessParameter.total = 0;
	ProcessParameter.output = aFun;
	ProcessParameter.pdata = aPv;

	sprintf( tSourceDir, "%s\\", aSourceDir );
	BrowseDir( tSourceDir, ProcessAFile, &ProcessParameter );


	CloseLibFile( hLib, &head );	
	
	fprintf( log, "----------------\n" );
	fprintf( log, "------end\n" );
	fprintf( log, "----------------\n" );

	fclose( log );

	return 0;
}
