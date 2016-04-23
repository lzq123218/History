
#ifndef _REPROCESSH

#define _REPROCESSH


#include "RSStreamlib.h"


typedef struct _BB {

	DWORD IndexValue;

	DWORD Count;

	List  a;

	List  b;

	CHAR n[1];

}BB;

#define LIKBUFF 3

typedef struct _FileInfo{

	HANDLE hFile;

	DWORD  offset;
	DWORD  res;


	DWORD Buff[LIKBUFF];


}FileInfo;

typedef struct _DD{

	List b;
	DWORD vid;

	FileInfo i;

	CHAR n[1];

}DD;

typedef struct _AA{

	DWORD IndexValue;
	DWORD vid;

	CHAR name[376];

}AA;


typedef struct _SecSmry{

	DWORD fStart;
	DWORD fSize;

	DWORD rsv;

}SecSmry, *PSecSmry;


#endif