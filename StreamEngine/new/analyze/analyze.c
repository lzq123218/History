// analyze.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include "viruslib_anly.h"

FILE *g_log = NULL;
FILE *g_log_err = NULL;
FILE *g_log_over = NULL;


extern void BrowseInfoList( PList ph );
extern void IndexEncode( PIndex start, DWORD aSize );
extern void PeerEncode( void* aStart, DWORD aSize );

CHAR lLogDir[256];

int main( int argc, char* argv[] ){

	BOOL success = FALSE;
	
	CHAR lFileName[256];


	if ( argc!= 2 ){
	
		printf("commandline: analyze e:\\lib\\");
		return 0;
	}

	memset( lLogDir, 0, 256 );
	strcpy( lLogDir, argv[1] );
	//strcat( lLogDir, "log\\");

	strcpy( lFileName, lLogDir );
	strcat( lFileName, "analyze_same.log");
	g_log      = fopen( lFileName, "w");//"f:\\zlib\\analyze.log"


	strcpy( lFileName, lLogDir );
	strcat( lFileName, "analyze_err.log");
	g_log_err  = fopen( lFileName, "w");//"f:\\zlib\\err.log"

#if 0
	strcpy( lFileName, lLogDir );
	strcat( lFileName, "over.log");
	g_log_over = fopen( lFileName, "w");//"f:\\zlib\\over.log"


	strcpy( lFileName, lLogDir );
	strcat( lFileName, "overlapped.log");

	hFile = CreateFile( //"f:\\zlib\\overlapped.log"
		
		lFileName,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	CloseHandle( hFile);

	strcpy( lFileName, lLogDir );
	strcat( lFileName, "lessthan.log");

	hFile = CreateFile(// "f:\\zlib\\lessthan.log"
		
		lFileName,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	CloseHandle( hFile);
#endif

	printf("analyzing virus lib ...\n\n");
	//LoadVirusLib2( "e:\\dev\\casey\\lib\\casey.zha", 0 );

	memset( lFileName, 0, 256 );
	strcpy( lFileName, argv[1] );
	strcat( lFileName, "test2.lib");

/*
	{
		printf( "gMallocTimes = %d\n", gMallocTimes);

		LoadVirusLib2( lFileName, 0 , 25);//"f:\\zlib\\data\\test2.lib"

		Times = gMallocTimes;
		
		printf( "MallocTimes = %d\n", Times );
		
		UnloadVirusLib();
		
		printf( "gMallocTimes = %d\n", gMallocTimes);
		
		BuildMem( Times );
		gMethord = 1;
		LoadVirusLib2( lFileName, 0 , 25);//"f:\\zlib\\data\\test2.lib"
		
		BuildImage( Times );
		FreeImage();

		UnloadVirusLib();

		gMethord = 0;
	}
*/

#if 1

	LoadVirusLib2( lFileName, 0 , 25);//"f:\\zlib\\data\\test2.lib"

	{	
		extern MemAlloc gPeer, gInfoLst, gIndex, gInfoBlock, gFNBlock;
		extern List gInfoHead;
		extern DWORD gPENum, gOffsetNum, gOffsetSize;

		InitMemAlloc( &gPeer );
		InitMemAlloc( &gInfoLst );
		INITLISTHEAD( &gInfoHead );
			
		BuildTotalPeerArray();
		
		printf( "gPeer.info.allocTimes = %d\n", gPeer.info.allocTimes );
		
		DestroyTotalPeerArray();

		printf( "gPeer.info.allocTimes = %d\n", gPeer.info.allocTimes );
		printf( "gPeer.info.allocBytes = %d\n", gPeer.info.allocBytes );

		success = BuildMemAlloc( &gPeer);
		
		BuildTotalPeerArray();
		//
		PeerEncode( gPeer.pMemBase, gPeer.BytesTotal);

		BrowseInfoList( &gInfoHead );
		//
		IndexEncode( (PIndex)gIndex.pMemBase, gIndex.BytesTotal );
		{

			DWORD total = 0;
			StdHead LibHead;
			ExtHeadPeer ExtHead;

			HANDLE hFile;
			DWORD num = 0;
			CHAR lFileName[256];



			memset( &ExtHead, 0, sizeof(ExtHeadPeer) );
			memset( &LibHead, 0, sizeof(StdHead) );

			ExtHead.OffsetOfPeer = 0;
			ExtHead.SizeOfPeer = gPeer.BytesTotal;
			total += gPeer.BytesTotal;

			ExtHead.OffsetOfIndex = total;
			ExtHead.SizeOfIndex = gIndex.BytesTotal;
			total += gIndex.BytesTotal;

			ExtHead.OffsetOfInfo = total;
			ExtHead.SizeOfInfo = gInfoBlock.BytesTotal;
			total += gInfoBlock.BytesTotal;

			ExtHead.OffsetOfMisc = 0;
			ExtHead.SizeOfMisc = 0;

			ExtHead.OffsetOfName = total;
			ExtHead.SizeOfName = gFNBlock.BytesTotal;
			total += gFNBlock.BytesTotal;

			ExtHead.TotalSize = total;

			memcpy( LibHead.Sig, "ZHALFA", 6 );

			LibHead.LibType = OPTIMIZED;
			LibHead.ExtOffset = sizeof(StdHead);
			LibHead.ExtSize = sizeof(ExtHeadPeer);

			LibHead.RecordSize = 0;

			LibHead.Count = ExtHead.SizeOfInfo / sizeof(InfoNode);

			LibHead.Reserved[0] = gPENum;
			LibHead.Reserved[1] = gOffsetNum;
			LibHead.Reserved[2] = gOffsetSize;


			strcpy( lFileName, argv[1] );
			strcat( lFileName, "PeerArray.img" );

			hFile = CreateFile( //"PeerArray.img"
			
				lFileName,
				GENERIC_READ | GENERIC_WRITE, 
				FILE_SHARE_WRITE | FILE_SHARE_READ,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			WriteFile( hFile, &LibHead, sizeof(StdHead), &num, NULL );
			WriteFile( hFile, &ExtHead, sizeof(ExtHeadPeer), &num, NULL );
			
			WriteFile( hFile, gPeer.pMemBase, gPeer.BytesTotal, &num, NULL );

			WriteFile( hFile, gIndex.pMemBase, gIndex.BytesTotal, &num, NULL );
			WriteFile( hFile, gInfoBlock.pMemBase, gInfoBlock.BytesTotal, &num, NULL );
			WriteFile( hFile, gFNBlock.pMemBase, gFNBlock.BytesTotal, &num, NULL);				

			CloseHandle( hFile );

		}
		//ZBuildImgForPeerArray( &gPeer, argv[1] );

		DestroyMemAlloc( &gPeer );
		DestroyMemAlloc( &gIndex );

		DestroyMemAlloc( &gInfoBlock );
		DestroyMemAlloc( &gFNBlock );

	}

#endif

#if 0
	{

		extern MemAlloc gFast;
		extern MemAlloc gData;

		InitMemAlloc( &gFast );
		InitMemAlloc( &gData );


		BuildTotalFastArray2();
		DestroyTotalFastArray2();

		BuildMemAlloc( &gFast);
		BuildMemAlloc( &gData);

		BuildTotalFastArray2();
		
		ZBuildImgForFastArray2( &gFast, &gData, argv[1] );

		DestroyMemAlloc( &gFast );
		DestroyMemAlloc( &gData );	
	}
#endif

	UnloadVirusLib();

	fclose( g_log );
	fclose( g_log_err );
	//fclose( g_log_over);
	
	return 0;
}
