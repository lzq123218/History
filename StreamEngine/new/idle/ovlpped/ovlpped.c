// ovlpped.cpp : Defines the entry point for the console application.
//

#include "stdio.h"
#include "stdlib.h"

#include "windows.h"

CHAR gBuff[512];


int main(int argc, char* argv[])
{

	HANDLE hFile;
	DWORD num = 0;
	BOOL res = FALSE;


	hFile = CreateFile(
		
		"a.exe",
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == (HANDLE)-1 ){
	
		printf( "can not open file\n");
		return 0;
	}


	res = ReadFile( hFile, gBuff, 512,&num, NULL);


	CloseHandle( hFile);

	return 0;
}
