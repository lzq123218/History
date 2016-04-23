// addlib.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdlib.h"
#include "stdio.h"

#include "windows.h"

#include "..\inc\viruslib_ex.h"

DWORD g_id = 0;

HANDLE hLib = (HANDLE) -1;

BOOL OpenLibFile( CHAR* path ){

	hLib = CreateFile(
		
		path,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hLib == (HANDLE)-1 ){

		return FALSE;
	}

	return TRUE;
}

BOOL CloseLibFile(){

	if ( hLib == (HANDLE) -1 ) return TRUE;

	CloseHandle(hLib);
	return TRUE;
}

CHAR name[8] ={'c','a','s','e','y','z','h','a'};

BOOL ReadLibHead( LibHead* pHead ){

	DWORD num = 0;
	BOOL result = FALSE;

	SetFilePointer(hLib, 0, NULL, FILE_BEGIN);

	result= ReadFile( hLib, pHead, sizeof(LibHead), &num, NULL);

	if ( result && ( num == sizeof(LibHead) ) ){
		
		return TRUE;
	}else{
		return FALSE;
	}
}

BOOL WriteLibHead( LibHead* pHead ){
	
	DWORD num = 0;
	BOOL result = FALSE;

	memcpy( pHead->name, name, 8);

	SetFilePointer(hLib, 0, NULL, FILE_BEGIN);

	result= WriteFile( hLib, pHead, sizeof(LibHead), &num, NULL);
		
	return TRUE;
}

BOOL AddLibRec( PLibRec pRec, PLibHead pLib ){

	DWORD num = 0;
	BOOL result = FALSE;

	SetFilePointer(hLib, 0, NULL, FILE_END);

	result= WriteFile( hLib, pRec, sizeof(LibRec), &num, NULL);

	if( result && (num == sizeof(LibRec) ) ){

		pLib->total++;
		return TRUE;	
	}

	return FALSE;
}

BOOL GenerateLibRec( PCHAR filename, PLibRec pRec ){

	HANDLE hFile;
	DWORD value;
	DWORD offset = 32;
	BOOL result;
	DWORD num;

	hFile = CreateFile(
		
		filename,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == (HANDLE)-1 ){

		return FALSE;
	}
	
	pRec->num = 0;
	pRec->id = ++g_id;
	
	result = SetFilePointer( hFile, offset, NULL, FILE_BEGIN);
	
	if( !result ){ CloseHandle( hFile); return FALSE ;}

	result = ReadFile( hFile, &value, sizeof(DWORD), &num, NULL);

	while( 
		result
		&&
		(num == sizeof(DWORD))
		&& (pRec->num < 8)
	){

		pRec->arry[pRec->num].offset = offset;
		pRec->arry[pRec->num].value =value;

		pRec->num ++;

		offset *= 2;

		result = SetFilePointer( hFile, offset, NULL, FILE_BEGIN);
	
		if( !result ) break;

		result = ReadFile( hFile, &value, sizeof(DWORD), &num, NULL);
		
	}

	CloseHandle( hFile);
	return TRUE;
}

DWORD GetRealOffset( DWORD offset ){

	DWORD realoffset;

	realoffset  = offset % 3;

	if ( realoffset == 0 ){
		
		realoffset = offset;
	}else{

		realoffset = offset + 3 - realoffset;
	}
	return realoffset;
}

#define _T 
HRESULT Mime64Encode(LPBYTE pData, UINT cbData, LPTSTR pchData)
{
    static TCHAR const alphabet[] = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

    int shift = 0;
    int save_shift = 0;
    unsigned long accum = 0;
    TCHAR blivit;
    unsigned long value;
    BOOL quit = FALSE;

    while ( ( cbData ) || (shift != 0) )
    {
        if ( ( cbData ) && ( quit == FALSE ) )
        {
            blivit = *pData++;
            --cbData;
        }
        else
        {
            quit = TRUE;
            save_shift = shift;
            blivit = 0;
        }

        if ( (quit == FALSE) || (shift != 0) )
        {
            value = (unsigned long)blivit;
            accum <<= 8;
            shift += 8;
            accum |= value;
        }

        while ( shift >= 6 )
        {
            shift -= 6;
            value = (accum >> shift) & 0x3Fl;

            blivit = alphabet[value];

            *pchData++ = blivit;

            if ( quit != FALSE )
            {
                shift = 0;
            }
        }
    }

    if ( save_shift == 2 )
    {
        *pchData++ = _T('=');
        *pchData++ = _T('=');
    }
    else if ( save_shift == 4 )
    {
        *pchData++ = _T('=');
    }

    *pchData++ = 0;

    return(NOERROR);
}

void Mime64Encode2( unsigned char* in, DWORD len, PCHAR out){

	const CHAR cv[] = ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	CHAR a[4];

	a[0] = in[0]>>2;

	a[1] = ((in[0] & 0x3)<<4)+((in[1] & 0xf0)>>4);

	a[2] = ((in[1] & 0xf)<<2)+((in[2] & 0xc0)>>6);

	a[3] = in[2]&((CHAR)0x3f);

	out[0] = cv[ a[0] ];
	out[1] = cv[ a[1] ];
	out[2] = cv[ a[2] ];
	out[3] = cv[ a[3] ];

}


DWORD B64ToValue( PCHAR str){
	
	DWORD value = 0;
	PCHAR des = (PCHAR)&value;
	PCHAR src = str;
	DWORD i;

	for( i = 0; i < 4; i++ ){

		*des = *src;
		des++; src++;
	}
	return value;
}

BOOL GenerateLibRecB64( PCHAR filename, PLibRec pRec ){

	HANDLE hFile;

#define B64BUFF 3

	UCHAR buff[B64BUFF];
	CHAR tmp[5];
	DWORD offset = 32;
	BOOL result;
	DWORD num;
	DWORD value;

	hFile = CreateFile(
		
		filename,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == (HANDLE)-1 ){

		return FALSE;
	}
	
	pRec->num = 0;
	pRec->id = ++g_id;
	
	result = SetFilePointer( hFile, GetRealOffset(offset), NULL, FILE_BEGIN);
	
	if( !result ){ CloseHandle( hFile); return FALSE ;}

	result = ReadFile( hFile, buff, B64BUFF, &num, NULL);

	while( 
		result
		&&
		( num == B64BUFF )
		&& (pRec->num < 8)
	){

		Mime64Encode2( (PUCHAR)buff, 3, tmp );
		value = B64ToValue(tmp);

		pRec->arry[pRec->num].offset = (GetRealOffset(offset)*4 / 3);

		pRec->arry[pRec->num].value = value;//

		pRec->num ++;

		offset *= 2;

		result = SetFilePointer( hFile, GetRealOffset(offset), NULL, FILE_BEGIN);
	
		if( !result ) break;

		result = ReadFile( hFile, buff, B64BUFF, &num, NULL);
		
	}

	CloseHandle( hFile);
	return TRUE;
}

void promote(){

	printf("please input command like this:\n");
	printf("addlib -s source path   -d destination path  [b64]\n");
	printf("for example: addlib -s e:\\casey\\ -d e:\\casey\\lib\\  b64\n\n");
}

void makepath( char* des, const char* src ){

	int len = strlen(src);
	int i = 0;

	while( i < len ){

		if (*src =='\\'){

			*des = *src; des++;
			*des = *src;
			des++; src++;
		
		}else{

			*des = *src;
			des++; src++;
		}
		i++;
	}
}

//CHAR sample[] = "e:\\dev\\addlib\\exesample\\";
CHAR source[128];
CHAR destination[128];
BOOL B64 = FALSE;	

int main(int argc, char* argv[])
{

	LibHead head;
	LibRec record;

	WIN32_FIND_DATAA info;
	HANDLE hFind;
	CHAR name[128];
	BOOL result = FALSE;

	if ( argc ){

		int i;

		if ( argc < 5 ){

			promote();	
			return 0;
		}else{

			for( i = 0; i < argc; i++){

				printf("argument %d %s\n", i, argv[i]);		
			}
			if ( !strcmp("-s", argv[1]) ){

				makepath( source, argv[2] );
			}
			if ( !strcmp("-d", argv[3]) ){

				makepath( destination, argv[4] );
			}
			if ( argc == 6 ){
			
				if ( !strcmp( "b64", argv[5]) ){
					
					B64 = TRUE;
				}
			}
		}	
	}

//	strcpy( name, "E:\\dev\\addlib\\lib\\");
	strcpy( name, destination );

	if ( B64 ){
		strcat( name, "casey.B64" );
	}else{
		strcat( name, "casey.zha" );
	}

	if( !OpenLibFile( name ) ){

		printf("CAN NOT OPEN LIB FILE\n");
		return 0;
	}

	if ( !ReadLibHead( &head) ){// head does not exist.

		head.date = 0;
		head.total = 0;
		WriteLibHead( &head);
	}

	SetFilePointer(hLib, sizeof(LibHead), NULL, FILE_BEGIN);

	//process all files in a directory 

	strcpy( name, source);
	strcat( name, "*.*");

	hFind = FindFirstFile( name, &info);

	if ( hFind == (HANDLE)-1 ){
	
		CloseLibFile();
	}

	while ( FindNextFile( hFind, &info) ){
		
		if ( !strcmp( info.cFileName, "..") ){

			continue;
		}
		
		strcpy( name, source);
		strcat( name, info.cFileName);

		if ( B64 ){

			result = GenerateLibRecB64( name, &record );
		}else{
			result = GenerateLibRec( name, &record );
		}

		if ( result ){

			AddLibRec( &record, &head);
#if _DEBUG			
			{
				DWORD i; 
					
				printf("Add file %s to viruslib\n", name);

				printf("ID = %d\n", record.id );
				for( i = 0; i < record.num; i++ ){

					printf( "offset = %d : value = %d\n",
						record.arry[i].offset,
						record.arry[i].value
					);
				}
				printf("\n");			
			}
#endif
		}
	}

	//CloseHandle( hFind);

	WriteLibHead( &head);

	CloseLibFile();
	return 0;
}

