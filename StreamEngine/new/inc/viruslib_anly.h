
#ifndef VIRUS_LIB_ANALY
#define VIRUS_LIB_ANALY

#include "RSStreamLib.h"

#include "hashtab.h"

#include "viruslib_ex.h"
#include "viruslib_in.h"



DWORD  LoadVirusLib2(const char* pLibPath, DWORD Protocol, DWORD max );
DWORD  UnloadVirusLib();


void BuildTotalPeerArray();
void DestroyTotalPeerArray();

BOOL ZBuildImgForPeerArray( PMemAlloc aPtr, CHAR* aDir );



void BuildTotalFastArray2();
void DestroyTotalFastArray2();

BOOL ZBuildImgForFastArray2( PMemAlloc aFast, PMemAlloc aData, CHAR* aDir );

#endif

