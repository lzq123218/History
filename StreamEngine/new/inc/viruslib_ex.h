
#ifndef VIRUS_LIB

#define VIRUS_LIB

#include "engbase.h"


#define FEATURE_COUNT 32
#define VALID_COUNT   16

typedef struct _LibRec{

	DWORD id;
	DWORD num;
	Feature arry[ FEATURE_COUNT ];

	DWORD VID;	//virus id defined by Rising Ltd
	DWORD Reserved;
	UCHAR FullName[256];	//includes driver ,path and filename;

}LibRec, *PLibRec;

typedef StdHead LibHead, *PLibHead;


/* virus process */

void InitLibHead( LibHead* pHead );

HANDLE CreateLibFile( CHAR* path );

BOOL GenerateLibRec( PCHAR filename, PLibRec pRec );

BOOL AddLibRec( HANDLE hLib, PLibRec pRec, PLibHead pLib );

BOOL CloseLibFile( HANDLE hLib, LibHead* pHead );



typedef struct _ExtHeadPeer{

	DWORD TotalSize;

	DWORD OffsetOfPeer;
	DWORD SizeOfPeer;

	DWORD OffsetOfIndex;
	DWORD SizeOfIndex;

	DWORD OffsetOfInfo;
	DWORD SizeOfInfo;

	DWORD OffsetOfMisc;
	DWORD SizeOfMisc;

	DWORD OffsetOfName;
	DWORD SizeOfName;


}ExtHeadPeer,*PExtHeadPeer;



#endif