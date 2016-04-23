
#include "windows.h"

#include "function.h"


typedef struct _ZHead{

	unsigned long ItemCount;
	unsigned long blocksize;
	unsigned long orignalsize;
	long rsv[6];

}ZHead;

typedef struct _ZItem{

	unsigned long status;
	unsigned long actsize;
	unsigned long chksum;

}ZItem;


#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS);



InternalCompressFile( HANDLE aInput, char* aOutput, unsigned long aSize ){

	unsigned long bufsize = 64*1024, size = 0, readlen = 0, comprsedlen = 0, writelen =0;
	PVOID inBuff = NULL, outBuff = NULL;
	HANDLE hOut;

	ZHead hd;
	ZItem item;

	if ( lzo_init() == LZO_E_OK ){

		inBuff = malloc( bufsize );
		outBuff = malloc( bufsize );

		hOut = CreateFile( 
			
				aOutput,
				GENERIC_READ | GENERIC_WRITE, 
				FILE_SHARE_WRITE | FILE_SHARE_READ,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
		);

		hd.blocksize = bufsize;
		hd.orignalsize = aSize;
		hd.ItemCount = aSize/bufsize + (aSize%bufsize ? 1:0 );

		WriteFile( hOut, &hd, sizeof(ZHead), &writelen, NULL );

		while ( size < aSize ){

			if ( ReadFile( aInput, inBuff, bufsize, &readlen, NULL ) ){

				size += readlen;

				item.status = lzo1x_1_compress( inBuff, readlen, outBuff, &comprsedlen, wrkmem );

				if ( LZO_E_OK == item.status ){
				
					item.actsize = comprsedlen;
					item.chksum = 0;

					WriteFile( hOut, &item, sizeof(ZItem), &writelen, NULL );
					WriteFile( hOut, outBuff, comprsedlen, &writelen, NULL );

				}else{
		
					item.actsize = readlen;
					item.chksum = 0;

					WriteFile( hOut, &item, sizeof(ZItem), &writelen, NULL );
					WriteFile( hOut, inBuff, readlen, &writelen, NULL );
				}
			
			}	
		}

		CloseHandle( hOut );

		free( inBuff ); free( outBuff );
	}

}


long BuildCompressedFile( char *aInputFile, char *aOutputFile ){

	long ret = 0;
	unsigned long fsize;
	HANDLE hIn;

	hIn = CreateFile(
		
		aInputFile,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if( hIn != INVALID_HANDLE_VALUE ){

		fsize = GetFileSize( hIn, NULL );

		if ( fsize ){
		
			InternalCompressFile( hIn, aOutputFile, fsize );
		}

		CloseHandle( hIn );
		
	}else{
	
		ret = -1;
	}

	return ret;
}

long DecompressFile( char *aInputFile, void* *aRet ){

	HANDLE hIn;
	ZHead hd;
	ZItem item;
	long ret = -1;
	unsigned long readlen = 0, bufsize = 0, i = 0, uncprslen = 0;
	PVOID inBuff = NULL, outBuff = NULL, retBuff = NULL, curBuff = NULL;

	hIn = CreateFile(
		
		aInputFile,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hIn != INVALID_HANDLE_VALUE ){

		ReadFile( hIn, &hd, sizeof(ZHead), &readlen, NULL );

		bufsize = hd.blocksize;
		retBuff = malloc( hd.orignalsize );

		ret = lzo_init();

		if ( ret == LZO_E_OK ){

			inBuff = malloc( bufsize );
			outBuff = malloc( bufsize );

			for ( i = 0; i < hd.ItemCount; i++ ){

				curBuff = (PVOID)((unsigned long)retBuff + hd.blocksize*i);

				ReadFile( hIn, &item, sizeof(ZItem), &readlen, NULL );

				ReadFile( hIn, inBuff, item.actsize, &readlen, NULL );

				if( item.status == LZO_E_OK ){

					ret = lzo1x_decompress( inBuff, item.actsize, outBuff, &uncprslen, wrkmem );
					
					if ( uncprslen > hd.blocksize ) break;

					if ( ret == LZO_E_OK ) {
		
						memcpy( curBuff, outBuff, uncprslen );
					}else{
					
						memcpy( curBuff, inBuff, item.actsize );
					}
				}
			}
		}

		CloseHandle( hIn );
		free( inBuff ); free( outBuff );
		*aRet = retBuff;
	}

	return ret< 0 ?	ret : hd.orignalsize;
}