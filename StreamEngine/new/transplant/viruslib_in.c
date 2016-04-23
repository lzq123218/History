
#ifdef __KERNEL__

	#define printf printk

#else

	#include <stdio.h>
	#include <stdlib.h>

#ifdef FOR_WINDOWS

	#include "windows.h"
#else 

	#include <fcntl.h>
	#include <unistd.h>

#endif

#endif


#include "eng.h"
#include "hashtab.h"
#include "virusimg.h"


//#define HASH_IMPLEMENT

//PTree global_root = NULL;
//PTree global_root2 = NULL;

//Tree VirusTreeRoot = { NULL, NULL, NULL };


/* following code added 2005-04-19 */
/* By now I have worked in Rising for one year */

/* hash tables are used  in some nodes that have many subnodes for acceleration */

#ifdef HASH_IMPLEMENT

static unsigned int zyhash(struct hashtab *h, void *key){

	DWORD value = *((DWORD*)key);

	value = value & 0x00000fff;
	value = value % h->size;

	return value;
}

static int zycmp(struct hashtab *h, void *key1, void *key2){

	DWORD value1 = *((DWORD*)key1);
	DWORD value2 = *((DWORD*)key2);

	h = h;

	if( value1 == value2 ){	return 0;

	}else{	return 1;	}
}
#endif

static char* gMemBase = NULL;

PeerArray* gPeerArrayRoot = NULL;


#ifndef __KERNEL__

#ifdef HASH_IMPLEMENT

BOOL BuildHashForPeerArray( PeerArray* aPtr ){

	struct hashtab *lHash = NULL;
	DWORD lTmp = 0;
	DWORD lRes = 0;

	if ( aPtr ){
	
		if ( (aPtr->count > 400)&& (aPtr->hash == NULL) ){

			lTmp = 379;
			lHash = hashtab_create( zyhash, zycmp, lTmp );

			if ( lHash ){
			
				DWORD i;

				for( i = 0; i < aPtr->count; i++ ){

					lRes = hashtab_insert(
						
						lHash, 
						&(aPtr->content[i].value), 
						&(aPtr->content[i])
					);

					if ( lRes ){
						//destroy hashtab
						hashtab_destroy( lHash );
						lHash = NULL;
						break;
					}				
				}
				aPtr->hash = lHash;
				return TRUE;
			}
			return FALSE;
		}

		{	DWORD i;

			for( i = 0; i< aPtr->count; i++ ){

				if( aPtr->content[i].sub ){
				
					BuildHashForPeerArray( aPtr->content[i].sub );
				}			
			}
		}
		
	}
	return FALSE;
}

void DestroyHashForPeerArray( PeerArray* aPtr ){

	if ( aPtr ){

		if ( aPtr->hash ){

			hashtab_destroy( aPtr->hash );
			aPtr->hash = NULL;
		}
		
		{	DWORD i;

			for( i = 0; i< aPtr->count; i++ ){

				if( aPtr->content[i].sub ){
				
					DestroyHashForPeerArray( aPtr->content[i].sub );
				}			
			}
		}
	}

}

#endif

#endif

void IndexDecode( PIndex start, DWORD aSize ){

	DWORD lBase = (DWORD)start;

	PIndex p = start;

	DWORD count = 0, i = 0 , size = 0;

	while( count < aSize ){

		for( i =0; i < p->count ; i++ ){

			if ( p->array[i].type == NEXT_TYPE_INDEX ){

				p->array[i].RangeOrIndex.next = (DWORD)(p->array[i].RangeOrIndex.next) + lBase;
			}	
		}

		size = (sizeof(Index) + (p->count -1)*sizeof(IndexRecord));
		count += size;

		p = (PIndex)((DWORD)p + size );
	}

}


/* this function is used to decode imagefile of PeerArray */

void PeerDecode( void* aMemBase, DWORD aBytesTotal ){

	PeerArray* lCurr = (PeerArray*)aMemBase;

	DWORD lPeerSize = 0;
	DWORD lSize = 0 ;
	DWORD i;

	while( lSize < aBytesTotal ){

		lPeerSize = sizeof(PeerArray) + sizeof(Element)*lCurr->count;
		lSize += lPeerSize;

		if ( lCurr->hash != (struct hashtab*)IMG_NULL ){
			
			lCurr->hash = (struct hashtab*)((DWORD)lCurr->hash + (DWORD)aMemBase);
		}else{
			lCurr->hash = (struct hashtab*)NULL;
		} 

		for( i = 0; i< lCurr->count; i++ ){

			if ( lCurr->content[i].sub != (PeerArray*)IMG_NULL ){

				lCurr->content[i].sub = (PeerArray*)((DWORD)lCurr->content[i].sub + (DWORD)aMemBase);

			}else{
				lCurr->content[i].sub = (PeerArray*)NULL;
			}		
		}

		lCurr = (PeerArray*)((DWORD)lCurr + lPeerSize);		
	}

}

/* this function is used to decode imagefile of FastArray */

void ZDecodeFast( void* aMemBase, DWORD aMemOffset, DWORD aBytesTotal ){

	FastArray* lCurr = (FastArray*)aMemBase;

	DWORD lPeerSize = 0;
	DWORD lSize = 0 ;
	DWORD i;

	ElementInfo *pElementInfo = NULL;

	while( lSize < aBytesTotal ){

		lPeerSize = sizeof(FastArray) + (sizeof(ElementInfo)+sizeof(DWORD))*lCurr->count;
		lSize += lPeerSize;

		if ( lCurr->hash != (struct hashtab*)IMG_NULL ){
			
			lCurr->hash = (struct hashtab*)((DWORD)lCurr->hash + (DWORD)aMemOffset);
		}else{
			lCurr->hash = (struct hashtab*)NULL;
		}
		
		pElementInfo = (ElementInfo*)( (DWORD)lCurr + sizeof(FastArray)+ lCurr->count* sizeof(DWORD) );

		for( i = 0; i< lCurr->count; i++, pElementInfo++ ){

			if ( pElementInfo->sub != (FastArray*)IMG_NULL ){

				pElementInfo->sub = (FastArray*)((DWORD)pElementInfo->sub + (DWORD)aMemOffset);

			}else{
				pElementInfo->sub = (FastArray*)NULL;
			}		
		}

		lCurr = (FastArray*)((DWORD)lCurr + lPeerSize);		
	}

}

extern DWORD gPENum, gOffsetNum, gOffsetSize;

PeerAddress gPeerAddress;

#ifndef __KERNEL__


DWORD LoadLibFromImgPeerArray( PCHAR aLibName ){

	StdHead libHead;
	ExtHeadPeer ExtHead;

	HANDLE lhLib;
	int fd = 0;

	DWORD lNum = 0;
	DWORD lTotal = 0;

	BOOL lRes = FALSE;

	memset( &gPeerAddress, 0, sizeof(PeerAddress) );

#ifdef FOR_WINDOWS

	lhLib = CreateFile(
		
		aLibName,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( lhLib == (HANDLE)-1 ) return FALSE;
#else

	fd = open( aLibName, O_RDONLY );
	
	if ( fd == 0 ) return FALSE;
#endif

#ifdef FOR_WINDOWS

	lRes = ReadFile( lhLib, &libHead, sizeof(StdHead), &lNum, NULL);

	if ( !lRes || (lNum != sizeof(StdHead)) ) goto ERR_END;

#else

	lNum = read( fd, &libHead, sizeof(StdHead) );

	if ( lNum != sizeof(StdHead) ) goto ERR_END;

#endif

	gPENum		= libHead.Reserved[0];
	gOffsetNum	= libHead.Reserved[1];
	gOffsetSize	= libHead.Reserved[2];

#ifdef FOR_WINDOWS

	lRes = ReadFile( lhLib, &ExtHead, sizeof(ExtHead), &lNum, NULL );

	if ( !lRes || (lNum != sizeof(ExtHead)) ) goto ERR_END;

#else

	lNum = read( fd, &ExtHead, sizeof(ExtHead) );

	if ( lNum != sizeof(ExtHead) ) goto ERR_END;

#endif

	gPeerAddress.base = malloc( ExtHead.TotalSize );

	if ( gPeerAddress.base == NULL ) goto ERR_END;	
	

#ifdef FOR_WINDOWS

	lRes = ReadFile( lhLib, gPeerAddress.base, ExtHead.TotalSize, &lNum, NULL );

	if ( !lRes || (lNum != ExtHead.TotalSize ) ){
	
		free( gPeerAddress.base );
		goto ERR_END;	
	}

	CloseHandle( lhLib );

#else

	lNum = read( fd, gPeerAddress.base, ExtHead.TotalSize );

	if ( lNum != ExtHead.TotalSize ) {

		free( gPeerAddress.base );
		goto ERR_END;
	}

	close( fd );

#endif

	lTotal = ExtHead.SizeOfPeer;

	//decode;
	PeerDecode( gPeerAddress.base, lTotal );

	if ( ExtHead.SizeOfIndex ){

		gPeerAddress.pIndex = (PIndex)(ExtHead.OffsetOfIndex +(DWORD)gPeerAddress.base);
	}

	IndexDecode( gPeerAddress.pIndex, ExtHead.SizeOfIndex );


	gPeerAddress.pInfoBlock = (PInfoNode)((DWORD)gPeerAddress.base + ExtHead.OffsetOfInfo);

	gPeerAddress.InfoCount = ExtHead.SizeOfInfo/ sizeof(InfoNode);

	if( ExtHead.SizeOfMisc ){

		gPeerAddress.pMisc = (void*)((DWORD)gPeerAddress.base + ExtHead.OffsetOfMisc);
	}
	//may add code for decoding in the future
	
	if ( ExtHead.SizeOfName ){

		gPeerAddress.pNameBlock = (PCHAR)((DWORD)gPeerAddress.base + ExtHead.OffsetOfName);
	}

	gPeerArrayRoot = (PeerArray*)( gPeerAddress.base );

	// hashtable build
#ifdef HASH_IMPLEMENT
	BuildHashForPeerArray( gPeerArrayRoot );
#endif

	return TRUE;

ERR_END:

#ifdef FOR_WINDOWS
	CloseHandle( lhLib );
#else
	close( fd );
#endif

	return FALSE;
}

void UnloadImgPeerArray(){

	if ( gMemBase){

		//destroy hashtable
#ifdef HASH_IMPLEMENT
		DestroyHashForPeerArray( gPeerArrayRoot );
#endif
		free( gMemBase );
		gMemBase = NULL;
		gPeerArrayRoot = NULL;
	}
}

#include "function.h"
PVOID gFileData = NULL;

DWORD LoadlibFromImgZ( PCHAR aName ){

	DWORD Ret = FALSE, size = 0, lTotal = 0;

	StdHead *phd;
	ExtHeadPeer *pehd;

	gFileData = NULL;

	size = DecompressFile( aName, &gFileData );

	if( size ){

		phd = (StdHead*)gFileData;

		gPENum		= phd->Reserved[0];
		gOffsetNum	= phd->Reserved[1];
		gOffsetSize	= phd->Reserved[2];

		pehd = (ExtHeadPeer*)(phd+1);

		gPeerAddress.base = (PVOID)(pehd+1);


		lTotal = pehd->SizeOfPeer;

		//decode;
		PeerDecode( gPeerAddress.base, lTotal );

		if ( pehd->SizeOfIndex ){

			gPeerAddress.pIndex = (PIndex)(pehd->OffsetOfIndex +(DWORD)gPeerAddress.base);
		}

		IndexDecode( gPeerAddress.pIndex, pehd->SizeOfIndex );


		gPeerAddress.pInfoBlock = (PInfoNode)((DWORD)gPeerAddress.base + pehd->OffsetOfInfo);

		gPeerAddress.InfoCount = pehd->SizeOfInfo/ sizeof(InfoNode);

		if( pehd->SizeOfMisc ){

			gPeerAddress.pMisc = (void*)((DWORD)gPeerAddress.base + pehd->OffsetOfMisc);
		}
		//may add code for decoding in the future
		
		if ( pehd->SizeOfName ){

			gPeerAddress.pNameBlock = (PCHAR)((DWORD)gPeerAddress.base + pehd->OffsetOfName);
		}

		gPeerArrayRoot = (PeerArray*)( gPeerAddress.base );

		Ret = TRUE;
	}
	return Ret;
}




#endif //__KERNEL__

#ifndef __KERNEL__

DWORD RSGetLibVersion( void ){
	
	return TRUE;
}

DWORD RSLoadLib( const char* pLibPath, DWORD Protocol ){

	Protocol = Protocol;
	return LoadLibFromImgPeerArray( pLibPath );
}

DWORD RSUnloadLib(){

	UnloadImgPeerArray();
	return TRUE;
}

#endif


