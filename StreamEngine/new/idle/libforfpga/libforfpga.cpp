// libforfpga.cpp : Defines the entry point for the console application.
//

#include "stdio.h"
#include "stdlib.h"

#include "windows.h"


#include "..\inc\engbase.h"
#include "..\inc\viruslib_ex.h"
#include "..\inc\viruslib_in.h"
#include "..\inc\virusimg.h"


int main(int argc, char* argv[])
{
	VirusImgHead lImgHead;
	PeerArray lDataHead;

	HANDLE lhLib;
	HANDLE lhFile;

	DWORD lNum = 0;
	DWORD lTotal = 0;

	DWORD lReadSize = 0;
	DWORD lAllocSize = 0;

	Element* pArray = NULL;

	BOOL lRes = FALSE;


	printf("Lib For Fpga!\n");

	lhLib = CreateFile(
		
		"e:\\lib\\PeerArray.img",
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	lRes = ReadFile( lhLib, &lImgHead, sizeof(VirusImgHead), &lNum, NULL);

	if ( !lRes || (lNum != sizeof(VirusImgHead)) ) return 0;

	if ( lImgHead.size != 0 ){

		lhFile = CreateFile(
		
			"e:\\lib\\NewPeerArray.img",
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_WRITE | FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		WriteFile( lhFile, &lImgHead, sizeof(VirusImgHead), &lNum, NULL );

		while( lImgHead.size > lReadSize ){

			ReadFile( lhLib, &lDataHead, sizeof(PeerArray), &lNum, NULL );
			
			if( lNum == sizeof(PeerArray) ){
			
				lReadSize += lNum;
			}else{
			
				break;
			}

			WriteFile( lhFile, &lDataHead, sizeof(PeerArray), &lNum, NULL );
			
			lAllocSize = lDataHead.count * sizeof(Element);

			pArray = (Element*)malloc( lAllocSize );

			ReadFile( lhLib, pArray, lAllocSize, &lNum, NULL );

			if( lNum == lAllocSize ){
			
				lReadSize += lNum;
			}else{
			
				break;
			}

			{	DWORD i = 0;
				Element* pElement = pArray;


				for ( i =0; i < lDataHead.count; i++, pElement++ ){

					WriteFile( lhFile, &(pElement->value), sizeof(DWORD), &lNum, NULL );				
				}

				pElement = pArray;

				for ( i =0; i < lDataHead.count; i++, pElement++ ){

					WriteFile( lhFile, &(pElement->id), sizeof(ElementInfo), &lNum, NULL );	
				}
						
			}
			
			free( pArray );
		
		}


		CloseHandle( lhFile );
	
	}

	CloseHandle( lhLib );
	return 0;
}

