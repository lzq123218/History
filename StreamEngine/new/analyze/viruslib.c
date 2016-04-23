
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "viruslib_anly.h"

#include "virusimg.h"

DWORD gFeatureSame = 0;

extern FILE *g_log;
extern FILE *g_log_err;
extern FILE *g_log_over;


//PTree global_root = NULL;
//PTree global_root2 = NULL;

Tree VirusTreeRoot = { NULL, NULL, NULL};


DWORD gMallocTimes = 0;

PCHAR EngineMalloc( DWORD size ){

	gMallocTimes++;

	return (PCHAR)malloc(size);
}

BOOL EngineMfree( PCHAR p ){

	free( p ); gMallocTimes--;

	return TRUE;
}


static
BOOL ReadLibHead( LibHead* pHead, HANDLE hLib ){

	DWORD num = 0;
	BOOL result = FALSE;

	SetFilePointer(hLib, 0, NULL, FILE_BEGIN);

	result= ReadFile( hLib, pHead, sizeof(LibHead), &num, NULL);

	if ( result && ( num == sizeof(LibHead) ) ){

		if ( memcmp( pHead->Sig, "ZHALFA", 6 ) ) return FALSE;

		if ( pHead->Count == 0 ) return FALSE;
		
		return TRUE;

	}else{
		return FALSE;
	}

}

PItem ItemCreateAndInit( PLibRec pRec, DWORD pos ){

	PItem tmp = NULL;
	BOOL dupname = FALSE;

	DWORD len = 0;

	if ( pRec->num == pos+1 ) dupname = TRUE;

	if ( dupname ) len = strlen( pRec->FullName ); 

	tmp = (PItem)EngineMalloc( sizeof(Item) + len );

	if ( tmp == NULL ){ 
		
		printf("fatal error 2\n"); 
		return NULL; 
	}

	memset( tmp, 0, sizeof(Item) + len );

	INITTREE( &(tmp->node))

	tmp->id = pRec->id;
	tmp->VID = pRec->VID;

	tmp->offset = pRec->arry[pos].offset;
	tmp->value = pRec->arry[pos].value;

	tmp->hash = NULL;

	if ( dupname ) memcpy( tmp->FileName, pRec->FullName, len );

	return tmp;
}

static
DWORD FindNode2( PItem* ret, PLibRec pRec ){

	PTree pHigh = &VirusTreeRoot;
	PItem pPeer = (PItem)pHigh->left;
	PItem pPrior = NULL;
	PItem curr = NULL;
	PItem LastMatch = NULL;

	DWORD i = 0;

	if( pPeer == NULL ){
		
		//directly create item and add it into tree

			curr = ItemCreateAndInit( pRec, i );
			
			pHigh->left = (PTree) &(curr->node);
			i++;

	}

	// added by zhangyuan 2005-04-08
	// i belive that there are some error in following code

	// conditon changed by zhangyuan 2005-04-08
	while ( pPeer /* &&(pPeer->offset == pRec->arry[i].offset) */ ){

		if ( pPeer->value == pRec->arry[i].value ){
		
			LastMatch = pPeer;

			pHigh = &(pPeer->node);
			pPrior = NULL;
			pPeer = (PItem)pHigh->left;
			i++;

			if ( i == pRec->num ){

				if ( pRec->VID != LastMatch->VID ){

					if ( g_log )
					fprintf( g_log, "%d <-> %d same \n%s\n%s\n\n",

						pRec->VID, LastMatch->VID, pRec->FullName, LastMatch->FileName 
					);
				}
				pPrior = NULL;
				break;
			}

			continue;
		}

		if ( pPeer->value < pRec->arry[i].value ){

			pHigh = NULL;
			pPrior = pPeer;
			pPeer = (PItem) (pPeer->node.right);

			continue;		
		}

		// add condition 2005-04-11

		if (  /*(i < pRec->num)
			&&*/
			(pPeer->value > pRec->arry[i].value) 
		){

			// create item and insert
			curr = ItemCreateAndInit( pRec, i ); 
							
			// insert to link that exist already
			if ( pPeer && pPrior ){

				curr->node.right = (PTree) &(pPeer->node);
				pPrior->node.right = (PTree) &(curr->node);
				i++;
			
			}else if ( (pPrior== NULL)&& pPeer ){
			
				if ( pHigh == NULL ){
					// error hanppens ,demalloc item
					EngineMfree( (PCHAR)curr );
					printf( "error hanppens\n" );

				}else{

					pHigh->left = (PTree) &(curr->node);
					curr->node.right = (PTree) &(pPeer->node);
					curr->node.parent = pHigh;
					i++;
				}

			}//if 

			break;
		
		}/* if */

	}//while

	if (  (pPeer== NULL)
		  && pPrior 
		  && ( pPrior->offset == pRec->arry[i].offset )
	){

		if ( i < pRec->num ){

			// create item and insert to the tail of link

			curr = ItemCreateAndInit( pRec, i ); 

			pPrior->node.right = (PTree) &(curr->node);
			i++;

		}

	}

	*ret = curr;
	return i;

}

BOOL VirusRecProcess4( PLibRec pRec){

	DWORD i;
	PItem NewNode = NULL;
	PItem curr = NULL;
	PItem tmp = NULL;

	i = FindNode2( &NewNode, pRec );

	if ( i&& NewNode&& (i < pRec->num) ){

		curr = NewNode;

		while ( i < pRec->num ){

			tmp = ItemCreateAndInit( pRec, i ); 

			tmp->node.parent = &(curr->node);

			curr->node.left= &(tmp->node);

			curr= tmp;
			i++;
		}
		return TRUE;

	}else if( i == pRec->num ){
	//this branch added 2004-11-18
	//Same feature exists already

		gFeatureSame++;
		
		return TRUE;
	}

	return FALSE;
}


DWORD g_less = 0;
DWORD g_err = 0;

DWORD gPENum = 4;
DWORD gOffsetNum = SECTION_OFFSET_NUM;
DWORD gOffsetSize = SIZE_Z;

DWORD  LoadVirusLib2(const char* pLibPath, DWORD Protocol, DWORD max ){

	HANDLE  hLib;
	LibHead head;
	LibRec  record;
	BOOL    result;
	DWORD   num;
	DWORD   i;

	Protocol = Protocol;

	gFeatureSame = 0;

	hLib = CreateFile(
		
		pLibPath,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hLib == (HANDLE)-1 ){

		return FALSE;
	}

	if ( !ReadLibHead( &head, hLib) ){

		CloseHandle(hLib);
		return FALSE;
	}

	gPENum = head.Reserved[0];
	gOffsetNum = head.Reserved[1];
	gOffsetSize = head.Reserved[2];
	
	max = gPENum + gOffsetNum;
		
	SetFilePointer(hLib, sizeof(LibHead), NULL, FILE_BEGIN);

	for ( i = 0; i < head.Count; i++ ){
	//read all virus records sequently,then add to virus tree;
		result = ReadFile( hLib, &record, sizeof(LibRec), &num, NULL );
	
		if ( result &&  (num == sizeof(LibRec) ) ){

			//add 2004-11-28
			if ( record.num < max ){

				if( g_log_err )
				fprintf( g_log_err, "less: %d %s\n", record.num, record.FullName );

				g_less++; continue;
			}


			if ( VirusRecProcess4( &record ) == FALSE){

#if _DEBUG
			if ( g_log_err )
			{
				printf( "! add record %d error !\n", record.id );

				fprintf( g_log_err, "err item begin\n");
				fprintf( g_log_err, "id = %d ; num = %d\n", record.id, record.num );

				{	DWORD i = 0;

					for ( i =0 ; i < record.num ; i++ ){

						fprintf( g_log_err, "offset = %d ; value = %d\n",
							record.arry[i].offset,
							record.arry[i].value
						);
					}					
				}

				fprintf( g_log_err, "err item end");
				g_err ++;
			}
#endif
				continue;
				break;
			}

		}else{
		
			printf("virus tree build error\n");
			CloseHandle(hLib);
			return FALSE;			
		}
	}

	//printf( "\nadd to mem %d records\n", i);
	//printf( "overlapped records = %d\n", gFeatureSame );
	//printf( "less that %d : error = %d\n", g_less, g_err );

	g_less = 0;
	g_err = 0;
	
	CloseHandle(hLib);
	return TRUE;
}


void Unloadlib2( PTree root){

	PItem tmp = (PItem)root;

	if ( tmp == NULL ) return;

	if ( tmp->node.left )Unloadlib2( tmp->node.left);

	if ( tmp->node.right )Unloadlib2( tmp->node.right);

//	printf( "%d\n", tmp->id );
	EngineMfree((PCHAR)tmp);
}

DWORD  UnloadVirusLib(){
		
	if ( VirusTreeRoot.left ){
		
		Unloadlib2( VirusTreeRoot.left );
		VirusTreeRoot.left = NULL;
	}

	return TRUE;
}

 
struct _BuildInfoNode{

	DWORD total;
	
	DWORD VNSize;
	DWORD FNSize;

} gBuildInfoNode;


List gInfoHead;

MemAlloc gPeer;
MemAlloc gInfoLst;


MemAlloc gInfoBlock;
MemAlloc gFNBlock;
//MemAlloc gInfoIndex;
/*
void BuildIndexBlock(){

	struct _aa{

		DWORD count;
		DWORD datasize;
		
		DWORD nameblocksize;
	
	}head;

	HANDLE hFile;
	DWORD num = 0;


	head.count = gBuildInfoNode.total;

	head.datasize = gInfoBlock.BytesTotal;

	head.nameblocksize = gFNBlock.BytesTotal;

	hFile = CreateFile(
		
		"off_info",
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);


	WriteFile( hFile, &head, sizeof(struct _aa), &num, NULL );

	WriteFile( hFile, gInfoBlock.pMemBase, gInfoBlock.BytesTotal, &num, NULL );

	WriteFile( hFile, gFNBlock.pMemBase, gFNBlock.BytesTotal, &num, NULL );

	CloseHandle( hFile );

}
*/

void BuildInfoAndNameBlock( PList ph ){

	PList p = ph->next;
	PInfoLst tmp = NULL;

	PInfoNode pi = NULL;
	CHAR	*pname = NULL;

	InitMemAlloc( &gInfoBlock );

	gInfoBlock.info.allocBytes = gBuildInfoNode.total* sizeof(InfoNode);

	BuildMemAlloc( &gInfoBlock);


	InitMemAlloc( &gFNBlock );

	gFNBlock.info.allocBytes = gBuildInfoNode.FNSize;

	BuildMemAlloc( &gFNBlock);

	while ( p != ph ){

		tmp = (PInfoLst)p;	
		p = p->next;

		pi = (PInfoNode) MallocMem( &gInfoBlock, sizeof(InfoNode) );

		memcpy( pi, &(tmp->node), sizeof(InfoNode) );

		pname = (char*)MallocMem( &gFNBlock, tmp->node.u.v.VNlen );

		memcpy( (PVOID)pname, (PVOID)tmp->node.u.v.VName, tmp->node.u.v.VNlen );

		pi->u.v.VName = (DWORD)pname - (DWORD)(gFNBlock.pMemBase);

		FreeMem( &gInfoLst, tmp );
	}

	memset( &gInfoHead, 0, sizeof(List) );	
}

extern MemAlloc gIndex;

PIndex BuildI( DWORD count , PInfoNode first, DWORD aBase );

void DestroyI( PIndex  p );

void BrowseInfoList( PList ph ){

	PList p = ph->next;
	PInfoLst tmp = NULL;
	PIndex index = NULL;

	memset( &gBuildInfoNode, 0, sizeof(struct _BuildInfoNode) );

	while ( p != ph ){
	
		tmp = (PInfoLst)p;	
		p = p->next;

		gBuildInfoNode.total ++;
		gBuildInfoNode.FNSize += tmp->node.u.v.VNlen;

	}

	//printf( "info node count %d\n", gBuildInfoNode.total );
	//printf( "info node memory %d\n", 
	//	gBuildInfoNode.total* sizeof(InfoNode) + gBuildInfoNode.FNSize + gBuildInfoNode.VNSize
	//);

	BuildInfoAndNameBlock( &gInfoHead );

	InitMemAlloc( &gIndex );
	
	index = BuildI( gBuildInfoNode.total, (PInfoNode)gInfoBlock.pMemBase, (DWORD)gInfoBlock.pMemBase );
	DestroyI( index );

	index = NULL;

	BuildMemAlloc( &gIndex );
	index = BuildI( gBuildInfoNode.total, (PInfoNode)gInfoBlock.pMemBase, (DWORD)gInfoBlock.pMemBase );

	//DestroyMemAlloc( &gIndex );
}

static PeerArray* BuildPeerArray( PItem root ){

	PItem lStart = (PItem)root->node.left;
	PItem lCurr = lStart;

	DWORD lCount = 0;
	DWORD size = 0;
	DWORD i = 0 ;

	PeerArray* tmp = NULL;

	if ( lStart == NULL )return NULL;

	while( lCurr ){

		lCount ++;
		lCurr = (PItem)lCurr->node.right;	
	}

	size = sizeof(PeerArray)+ sizeof(Element)*lCount;

	tmp = (PeerArray*)MallocMem( &gPeer, size );

	if ( tmp == NULL){ printf("Can not allocate Mem\n"); return NULL; }

	lCurr = lStart;

	tmp->count = lCount;
	tmp->offset = lCurr->offset;
	tmp->hash = NULL;

	for ( i = 0; i < lCount; i++ ){

		//tmp->content[i].id = lCurr->VID;
		tmp->content[i].value = lCurr->value;

		tmp->content[i].sub = BuildPeerArray( lCurr );

		if ( (tmp->content[i].sub == NULL) && gPeer.AllocTag ){

			PInfoLst p = (PInfoLst)MallocMem( &gInfoLst, sizeof(InfoLst)+ strlen(lCurr->FileName) );

			if ( p ){
				memset( p, 0, sizeof(InfoLst) );
				INITLIST( &(p->lst) );

				p->node.Index = (DWORD)(&(tmp->content[i])) - (DWORD)gPeer.pMemBase;
				p->node.Type  = 0;

				p->node.u.v.VID = lCurr->VID;

				p->node.u.v.VName = (DWORD)p + sizeof(InfoLst);
				p->node.u.v.VNlen = strlen( lCurr->FileName );

				memcpy( (PVOID)p->node.u.v.VName, lCurr->FileName, strlen(lCurr->FileName) );

				INSERTLISTTAIL( (&gInfoHead), (&p->lst) );
			}
		}

		lCurr = (PItem)lCurr->node.right;
	
	}

	return tmp;
}

PeerArray* gPeerArrayRoot = NULL;

void BuildTotalPeerArray(){

	Item lVirtualNode;

	memset( &lVirtualNode, 0, sizeof(Item));

	lVirtualNode.node.left = VirusTreeRoot.left;

	gPeerArrayRoot = BuildPeerArray( &lVirtualNode );

}

void DestroyPeerArray( PeerArray* aRoot ){

	DWORD lCount = 0;

	if ( aRoot == NULL )return;

	lCount = aRoot->count;

	{	DWORD i;
		
		for( i = 0; i < lCount; i++ ){
		
			DestroyPeerArray( aRoot->content[i].sub );
			aRoot->content[i].sub = NULL;
		}	
	}
//  Zfree(aRoot);
	FreeMem( &gPeer, aRoot );
}

void DestroyTotalPeerArray(){

	DestroyPeerArray( gPeerArrayRoot );
	gPeerArrayRoot = NULL;
}

void PeerEncode( void* aStart, DWORD aSize ){

	PeerArray* lCurr = (PeerArray*)aStart;

	DWORD lPeerSize = 0;
	DWORD lSize = 0 ;
	DWORD i;

	while( lSize < aSize ){

		lPeerSize = sizeof(PeerArray) + sizeof(Element)*lCurr->count;
		lSize += lPeerSize;

		if ( lCurr->hash ){
			
			lCurr->hash = (struct hashtab*)((DWORD)lCurr->hash - (DWORD)aStart);
		}else{
			lCurr->hash = (struct hashtab*)IMG_NULL;
		} 

		for( i = 0; i< lCurr->count; i++ ){

			if ( lCurr->content[i].sub ){

				lCurr->content[i].sub = (PeerArray*)((DWORD)lCurr->content[i].sub - (DWORD)aStart);

			}else{
				lCurr->content[i].sub = (PeerArray*)IMG_NULL;
			}		
		}

		lCurr = (PeerArray*)((DWORD)lCurr + lPeerSize);		
	}
	
}

/*

void ZDecode(){

	PeerArray* lCurr = (PeerArray*)gMemBase;

	DWORD lPeerSize = 0;
	DWORD lSize = 0 ;
	DWORD i;

	while( lSize < gBytesTotal ){

		lPeerSize = sizeof(PeerArray) + sizeof(Element)*lCurr->count;
		lSize += lPeerSize;

		if ( lCurr->hash != (struct hashtab*)IMG_NULL ){
			
			lCurr->hash = (struct hashtab*)((DWORD)lCurr->hash + (DWORD)gMemBase);
		}else{
			lCurr->hash = (struct hashtab*)NULL;
		} 

		for( i = 0; i< lCurr->count; i++ ){

			if ( lCurr->content[i].sub != (PeerArray*)IMG_NULL ){

				lCurr->content[i].sub = (PeerArray*)((DWORD)lCurr->content[i].sub + (DWORD)gMemBase);

			}else{
				lCurr->content[i].sub = (PeerArray*)NULL;
			}		
		}

		lCurr = (PeerArray*)((DWORD)lCurr + lPeerSize);		
	}

}

*/

BOOL ZBuildImgForPeerArray( PMemAlloc aPtr, CHAR* aDir ){

	HANDLE hFile;
	DWORD num = 0;
	CHAR lFileName[256];

	VirusImgHead ImgHead;

	if ( aPtr == NULL ) return FALSE;

	ImgHead.signature = 0xffffffff;
	ImgHead.size = aPtr->BytesTotal;
	ImgHead.root = (DWORD)gPeerArrayRoot - (DWORD)(aPtr->pMemBase);

	strcpy( lFileName, aDir );
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

	WriteFile( hFile, &ImgHead, sizeof(VirusImgHead),&num, NULL );	

	WriteFile( hFile, aPtr->pMemBase, ImgHead.size, &num, NULL );

	CloseHandle( hFile);	

	return TRUE;

}


MemAlloc gFast;
MemAlloc gData;

static FastArray2* BuildFastArray2( PItem root ){

	PItem lStart = (PItem)root->node.left;
	PItem lCurr = lStart;

	DWORD lCount = 0;
	DWORD size = 0;
	DWORD i = 0 ;

	FastArray2* tmp = NULL;
	DWORD *tmp2 = NULL;

	if ( lStart == NULL )return NULL;

	while( lCurr ){

		lCount ++;
		lCurr = (PItem)lCurr->node.right;	
	}

	size = sizeof(FastArray2)+ sizeof(ElementInfo)*lCount;

	tmp = (FastArray2*)MallocMem( &gFast, size );

	tmp2 = (DWORD*)MallocMem( &gData, sizeof(DWORD)*lCount );

	if ( tmp == NULL){ return NULL; }

	lCurr = lStart;

	tmp->count = lCount;
	tmp->offset = lCurr->offset;
//	tmp->hash = NULL;
	tmp->pData = tmp2;

	for ( i = 0; i < lCount; i++ ){

		tmp->info[i].id = lCurr->VID;
		tmp->pData[i] = lCurr->value;

		tmp->info[i].sub = BuildFastArray2( lCurr );

		lCurr = (PItem)lCurr->node.right;
	
	}

	return tmp;
}

FastArray2 *gFastArray2Root = NULL;
 
void BuildTotalFastArray2(){

	Item lVirtualNode;

	memset( &lVirtualNode, 0, sizeof(Item));

	lVirtualNode.node.left = VirusTreeRoot.left;

	gFastArray2Root = BuildFastArray2( &lVirtualNode );

}

void DestroyFastArray2( FastArray2* aRoot ){

	DWORD lCount = 0;

	if ( aRoot == NULL )return;

	lCount = aRoot->count;

	if ( aRoot->pData ){
	
		FreeMem( &gData, aRoot->pData);	aRoot->pData = NULL;
	}

	{	DWORD i;
		
		for( i = 0; i < lCount; i++ ){
		
			DestroyFastArray2( (FastArray2*)(aRoot->info[i].sub) );
			aRoot->info[i].sub = NULL;
		}	
	}

	FreeMem( &gFast, aRoot);
}

void DestroyTotalFastArray2(){

	DestroyFastArray2( gFastArray2Root );
	gFastArray2Root = NULL;
}

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

void FastArray2Dcode( void* aMemBase, DWORD aTotal, void* aRealBase, void* aDataBase ){

	FastArray2 *lCurr = NULL;

	DWORD lSize = 0;
	DWORD lTotal = 0;

	DWORD i = 0;
	DWORD lLen = 0;
	DWORD lFastBase = 0;

	if ( aMemBase == 0 )return;
	
	lFastBase = (DWORD)(aRealBase);
	lTotal = aTotal;

	lCurr = (FastArray2*)(aMemBase);

	while( lSize < lTotal ){

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

}

BOOL ZBuildImgForFastArray2( PMemAlloc aFast, PMemAlloc aData, CHAR* aDir ){

	HANDLE hFile;
	DWORD num = 0;
	CHAR lFileName[256];

	FastImgHead ImgHead;

	if ( (aFast == NULL)||(aData == NULL) ) return FALSE;

	ImgHead.fastSize = aFast->BytesTotal;
	ImgHead.dataSize = aData->BytesTotal;
	ImgHead.root = (DWORD)gFastArray2Root - (DWORD)(aFast->pMemBase);

	strcpy( lFileName, aDir );
	strcat( lFileName, "FastArray2.img" );

    hFile = CreateFile(
		
		lFileName,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	WriteFile( hFile, &ImgHead, sizeof(FastImgHead),&num, NULL );

	FastArray2Encode( &gFast, gData.pMemBase );

	//FastArray2Dcode( gFast.pMemBase, gFast.BytesTotal, gFast.pMemBase, gData.pMemBase );

	WriteFile( hFile, aFast->pMemBase, aFast->BytesTotal, &num, NULL );

	WriteFile( hFile, aData->pMemBase, aData->BytesTotal, &num, NULL );

	CloseHandle( hFile);	

	return TRUE;

}