// WhiteScan.cpp : Defines the entry point for the DLL application.
//

#include "windows.h"
#include "stdio.h"
#include "eng.h"
#include "RSStreamLib.h"

#include "WhiteScan.h"

#define BLKSIZE 4096

BYTE buff[BLKSIZE];

LONG StreamScan( HANDLE hFile, DWORD protocol, DWORD *aRet ){

	BOOL result = FALSE;
	BOOL FirstRead = TRUE;
	DWORD actual;
	DWORD context = 0;
	LONG value = 0;

	DWORD ext = 0;

	RSConnectEvent( protocol, &context );

	do{
		result = ReadFile( hFile, buff, BLKSIZE, &actual, NULL);

		value = RSDataScan( context, buff, actual, &ext );

		if ( (value != -1)&&( protocol == 0 ) )break;

	}while( actual == BLKSIZE );

	RSDisconnectEvent( context );

	if ( value > 0  ) *aRet = ext;

	return value;
}


LONG ScanFile( const PCHAR pName, DWORD *aRet ){

	HANDLE hFile;
	LONG res = 0;
	WORD head = 0;
	DWORD num = 0, index = 0;

	hFile = CreateFile(
		
		pName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if( hFile == INVALID_HANDLE_VALUE ){

		printf( "CAN NOT OPNE FILE: %s\n",pName );
		return 0;
	}

	ReadFile( hFile, &head, 2, &num, NULL );

	SetFilePointer( hFile, 0, NULL, FILE_BEGIN );

	if( head == 0x5a4d ){

		res = StreamScan( hFile, 0 , &index );
	}

	CloseHandle( hFile );
	return res;
}

DWORD LoadlibFromImgZ( PCHAR aName );

DWORD LoadLibFromImgPeerArray( PCHAR aLibName );

BOOL Load( PCHAR aName ){

	RSEngineInit();
	return  LoadlibFromImgZ( aName );
}


BOOL UnLoad(){

	RSUnloadLib();
	RSEngineUninit();
	return TRUE;
}


DWORD ScanBegin( DWORD aPrtcl ){

	DWORD context = 0;

	RSConnectEvent( aPrtcl, &context );
	return context;
}


DWORD ScanEnd( DWORD aCtxt ){

	RSDisconnectEvent(aCtxt);
}


LONG ScanPiece( DWORD aCtxt, PCHAR aBuff, DWORD aLen ){

	DWORD ext = 0;
	return RSDataScan( aCtxt, aBuff, aLen, &ext );
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

