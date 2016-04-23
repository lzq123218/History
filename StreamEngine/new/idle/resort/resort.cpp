// resort.cpp : Defines the entry point for the console application.
//


#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "windows.h"

#include "..\inc\engbase.h"
#include "..\inc\viruslib_in.h"
#include "..\inc\virusimg.h"

DWORD gArrayNum = 0;
DWORD gTotal = 0;


void FastArray2Dcode( void* aMemBase, DWORD aTotal, void* aRealBase, void* aDataBase ){

	FastArray2 *lCurr = NULL;

	DWORD lSize = 0;
	DWORD lTotal = 0;

	DWORD i = 0;
	DWORD lLen = 0;
	DWORD lFastBase = 0;

	DWORD lTimes = 0;

	if ( aMemBase == 0 )return;
	
	lFastBase = (DWORD)(aRealBase);
	lTotal = aTotal;

	lCurr = (FastArray2*)(aMemBase);

	while( lSize < lTotal ){

		lTimes++;

		lLen = sizeof(FastArray2) + sizeof(ElementInfo2)* lCurr->count;

		lCurr->pData = (DWORD*)((DWORD)(lCurr->pData) + (DWORD)aDataBase);
	
		for( i=0 ; i< lCurr->count; i++){

			if ( lCurr->info[i].sub == (FastArray2*)IMG_NULL ){

				lCurr->info[i].sub = (FastArray2*)NULL;

			}else{

				lCurr->info[i].sub = (FastArray2*)((DWORD)(lCurr->info[i].sub) + (DWORD)lFastBase);
			}		
		}

		lSize += lLen;
		lCurr = (FastArray2*)((DWORD)lCurr + lLen);
	}

	gArrayNum = lTimes;

}

FastArray2 *gFastArray2 = NULL;

DWORD LoadLibFastArray2( PCHAR aLibName ){

	FastImgHead lImgHead;

	HANDLE lhLib;

	DWORD lNum = 0;
	DWORD lTotal = 0;

	BOOL lRes = FALSE;
	PCHAR lRealBase = NULL;

	lhLib = CreateFile(
		
		aLibName,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( lhLib == (HANDLE)-1 ) return 0;


	lRes = ReadFile( lhLib, &lImgHead, sizeof(FastImgHead), &lNum, NULL);

	if ( !lRes || (lNum != sizeof(FastImgHead)) ) return 0;

	lTotal = lImgHead.fastSize + lImgHead.dataSize;

	lRealBase = (PCHAR)malloc( lTotal );

	if ( lRealBase == NULL ){
		
		CloseHandle( lhLib );
		return 0;	
	}

	lRes = ReadFile( lhLib, lRealBase, lTotal, &lNum, NULL );

	if ( !lRes || (lNum != lTotal ) ){
	
		free( lRealBase );
		CloseHandle( lhLib );
		return 0;	
	}

	CloseHandle( lhLib );

	FastArray2Dcode( lRealBase,lImgHead.fastSize, lRealBase, (lRealBase+lImgHead.fastSize) );

	gFastArray2 = (FastArray2*)lRealBase;
	gTotal = lImgHead.fastSize;

	return TRUE;
}

struct  virus{

	void *data;
	int len;

	FastArray2 * pFastArray;
	void* newaddr;
};

struct virus* gIndex = NULL;

DWORD gZ = 0;

void BuildIndex(){

	FastArray2* lCurr = NULL;
	struct virus* curr = NULL;

	DWORD lTotal = gTotal;
	DWORD lLen = 0;

	DWORD lSize = sizeof(struct virus)* gArrayNum;

	gIndex = (struct virus*)malloc(lSize);

	if ( gIndex == NULL )return;

	lSize = 0;

	gZ = 0;
	curr = gIndex;
	lCurr = gFastArray2;

	while( lSize < lTotal ){

		lLen = sizeof(FastArray2) + sizeof(ElementInfo2)* lCurr->count;

		curr->data = lCurr->pData;
		curr->len = lCurr->count;
		curr->pFastArray = lCurr;

		gZ += curr->len;

		lSize += lLen;
		lCurr = (FastArray2*)((DWORD)lCurr + lLen);
		curr++;
	}

}

typedef struct _MemAlloc{

	CHAR* pMemBase;

	DWORD BytesAlloced;
	DWORD BytesTotal;

	DWORD AllocTag;
	
	struct statistic{

		DWORD allocBytes;
		DWORD allocTimes;

	}info;

}MemAlloc, *PMemAlloc;

void FastArray2Encode( PMemAlloc aFast, void* aDataBase ){

	FastArray2 *lCurr = NULL;

	DWORD lSize = 0;
	DWORD lTotal = 0;

	DWORD i = 0;
	DWORD lLen = 0;
	DWORD lFastBase = 0;
	DWORD lTimes = 0;

	if ( aFast == NULL )return ;

	lFastBase = (DWORD)(aFast->pMemBase);
	lTotal = aFast->BytesTotal;

	lCurr = (FastArray2*)(lFastBase);
	
	while( lSize < lTotal ){
		
		lTimes++;

		lLen = sizeof(FastArray2) + sizeof(ElementInfo2)* lCurr->count;

		lCurr->pData = (DWORD*)((DWORD)(lCurr->pData) - (DWORD)aDataBase);
	
		for( i=0 ; i< lCurr->count; i++){

			if ( lCurr->info[i].sub ){

				lCurr->info[i].sub = (FastArray2*)((DWORD)(lCurr->info[i].sub) - (DWORD)lFastBase);

			}else{

				lCurr->info[i].sub = (FastArray2*)IMG_NULL;
			}		
		}

		lSize += lLen;
		lCurr = (FastArray2*)((DWORD)lCurr + lLen);
	}

	printf( "FastArray2Encode : %d\n", lTimes );

}

extern "C" int defrag( unsigned char *dest, struct virus *vdb, int num);
extern "C" unsigned long build( void* addr, unsigned int num );

int main(int argc, char* argv[]){

	struct virus* p = gIndex +( gArrayNum -1 );
	HANDLE hFile;
	DWORD lNum = 0;
	DWORD lSize = 0;
	PCHAR pBase = NULL;

	LoadLibFastArray2( "e:\\lib\\FastArray2.img");

	BuildIndex();

	printf( "gZ = %d\n", gZ );

	printf( "gArrayNum = %d\n", gArrayNum );
	printf( "gIndex = %x\n", gIndex );

//	defrag( (unsigned char*)100000, gIndex, gArrayNum );

	lSize = build( gIndex, gArrayNum );

	printf( "lSize = %d\n", lSize );

	pBase = (PCHAR)malloc( lSize* sizeof(DWORD));

	printf( "lSize = %d\n", lSize);

	if ( pBase ){

		DWORD i;
		FastArray2 *pTmp = NULL;
		

		for( i = 0; i < gArrayNum; i++ ){

			pTmp = gIndex[i].pFastArray;

			if ( gIndex[i].len > 1024 ){
			
				printf( ">1024 : %x", gIndex[i].newaddr );
			}

			memcpy( 

				(void*)((DWORD)(pBase)+(DWORD)gIndex[i].newaddr),
				(void*)(gIndex[i].data),
				gIndex[i].len*sizeof(DWORD) 
			);

			pTmp->pData = (DWORD*)((DWORD)(gIndex[i].newaddr)+(DWORD)pBase);
		
		}

		//free( pBase);
		printf( "OK\n");
	}

	{
		FastImgHead lHead;
		MemAlloc tmp;

		lHead.fastSize = gTotal;
		lHead.dataSize = lSize *sizeof(DWORD);
		lHead.root = 0;

		tmp.pMemBase = (PCHAR)gFastArray2 ;
		tmp.BytesTotal = gTotal;

		FastArray2Encode( &tmp, pBase );

		hFile = CreateFile(
		
			"e:\\lib\\resort.a",
			GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		WriteFile( hFile, &lHead, sizeof(FastImgHead), &lNum, NULL);

		WriteFile( hFile, tmp.pMemBase, tmp.BytesTotal, &lNum, NULL);
		
		WriteFile( hFile, pBase, lSize*sizeof(DWORD), &lNum, NULL );

		printf( "lSize = %d\n", lSize);
	
		CloseHandle( hFile);
	}





#if 0
	hFile = CreateFile(
		
		"e:\\lib\\info.a",
		GENERIC_READ|GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	WriteFile( hFile, gIndex, sizeof(struct virus)*gArrayNum, &lNum, NULL );

	CloseHandle( hFile);

	printf( "filesize = %d\n", sizeof(struct virus)*gArrayNum );
#endif
	
	return 0;
}

