// BldScanlst.cpp : Defines the entry point for the console application.
//

#include "windows.h"

#include "RSStreamLib.h"

typedef struct _LogHead{

	DWORD size;
	DWORD id;

}LogHead;

typedef struct _Parameter{

	HANDLE h;
	DWORD cnt;

}Parameter, *PPara;

long Add2Lst( char* aName, void* aP ){

	LogHead lgh;
	PPara p = (PPara)aP;
	DWORD len = 0, ret = 0;

	if ( aName && p ){

		len = strlen(aName );
		p->cnt++;

		lgh.id = p->cnt;
		lgh.size = sizeof(LogHead) + len ;

		WriteFile( p->h, &lgh, sizeof(LogHead), &ret, NULL );
		WriteFile( p->h, aName, len, &ret, NULL );
	}

	return 0;
}

char gBrsDir[512];

int main(int argc, char* argv[])
{
	Parameter para;
	DWORD len = 0;

	if ( argc != 2 )return 0;

	memset( gBrsDir, 0, 512 );

	//strcpy( gBrsDir, argv[1] );
	MakeFullPath( argv[1], 512, gBrsDir );

	para.cnt = 0;

	para.h = CreateFile(
		
		"scanresult.log",
		GENERIC_WRITE, 
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);


	if ( para.h != INVALID_HANDLE_VALUE ){
	
		BrowseDir( gBrsDir, Add2Lst, (void*)(&para) );

		CloseHandle( para.h );
	}

	return 0;
}

