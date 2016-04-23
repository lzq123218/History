
#ifndef VIRUS_IMG

#define VIRUS_IMG

#include ".\engbase.h"


typedef struct _VirusImgHead{

	DWORD signature;
	DWORD size;
	DWORD root;

}VirusImgHead;

#define IMG_NULL 0xffffffff

typedef struct _FastImgHead{

	DWORD fastSize;

	DWORD dataSize;

	DWORD root;

}FastImgHead;


#endif