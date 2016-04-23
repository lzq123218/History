
#ifndef WHITESCAN
#define WHITESCAN

#include "windows.h"

#ifdef __cplusplus
extern "C"{
#endif

__declspec(dllexport) LONG ScanFile( const PCHAR pName, DWORD *aRet );

__declspec(dllexport) BOOL Load( PCHAR aName );
__declspec(dllexport) BOOL UnLoad();

__declspec(dllexport) DWORD ScanBegin( DWORD aPrtcl );
__declspec(dllexport) DWORD ScanEnd( DWORD aCtxt );
__declspec(dllexport) LONG ScanPiece( DWORD aCtxt, PCHAR aBuff, DWORD aLen );

#ifdef __cplusplus
}
#endif

#endif