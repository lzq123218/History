// classify.cpp : Defines the entry point for the console application.
//

#include "stdio.h"
#include "stdlib.h"

#include "windows.h"

#include "..\inc\help.h"

//PCHAR start = "e:\\risinglib\\virus\\pe\\";

extern FILE* gOut;

extern CHAR PATHNAME[256];

int main(int argc, char* argv[])
{
	CHAR dir[512];

	if( argc != 3 ){
	
		printf( "command line like this : classify  e:\\lib\\statics\\  f:\\" );
		return 0;
	}

	memset( PATHNAME, 0, 256 );
	strcpy( PATHNAME, argv[1] );

	memset( dir, 0, 512);
	//strcpy( dir, start);
	strcpy( dir, argv[2] );

	printf( "Browse dir: %s and its subdirectory\n", dir );
	printf( "please wait .....\n");

	EmptyFile();

	{
		char name[256];
		strcpy( name, PATHNAME );
		strcat( name, "log.txt" );
		
		gOut = fopen( name, "w" );	
	}

	BrowseDir( dir, NULL );

	fclose( gOut );

	ShowResult();
	return 0;
}
