// reprocess.cpp : Defines the entry point for the console application.
//

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "reprocessh.h"


List gHead;
List gHead2;

DWORD Index2Name( DWORD aValue, PCHAR aRet, DWORD *aVid );
PInfoNode Index2node( DWORD aValue );


BB* Find( PList aHead , DWORD IndexValue ){

	BB *p0 = NULL;
	PList p = aHead->next;

	BB *tmp = NULL;

	while( p != aHead ){

		p0 = (BB *)((DWORD)p - 8 );

		if ( p0->IndexValue == IndexValue ) return p0;

		if ( p0->IndexValue < IndexValue){
		
			p = p->next; continue;
		}

		if ( p0->IndexValue > IndexValue ){

			return NULL;
		}			
	}

	return NULL;
}

BB* FindOrBuild(  PList aHead, AA* aRec ){

	BB *p0 = NULL;
	PList p = aHead->next;

	BB *tmp = NULL;

	while( p != aHead ){

		p0 = (BB *)((DWORD)p - 8 );

		if ( p0->IndexValue == aRec->IndexValue ) return p0;

		if ( p0->IndexValue < aRec->IndexValue){
		
			p = p->next; continue;
		}

		if ( p0->IndexValue > aRec->IndexValue ){

			tmp = (BB*) malloc( sizeof(BB) );
			memset( tmp, 0, sizeof(BB) );

			tmp->IndexValue = aRec->IndexValue;

			INITLISTHEAD( &(tmp->b) );

			//insert before p0
			{
				PList b = &tmp->a;	
				INSERTLISTTAIL( p, b );
			}
			
			return tmp;
		}			
	}

	if ( p == aHead ){

		tmp = (BB*) malloc( sizeof(BB) );
		memset( tmp, 0, sizeof(BB) );

		tmp->IndexValue = aRec->IndexValue;
		INITLISTHEAD( &(tmp->b) );
		//insert after  p
		{
			PList b = &tmp->a;
			INSERTLISTHEAD( (p->pre), b );
		}
			
		return tmp;
	} 

	return NULL;
}

BOOL FileContentNotSame( PCHAR n1, PCHAR n2 ){

	HANDLE h1,h2;
	DWORD s1, s2, num;
	BOOL ret = TRUE;

	PCHAR b1,b2;


	h1 = CreateFile(
		
		n1,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	h2 = CreateFile(
		
		n2,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	s1 = GetFileSize( h1, NULL );
	s2 = GetFileSize( h2, NULL );

	if ( s1 == s2 ){

		b1 = (PCHAR)malloc( s1 );
		b2 = (PCHAR)malloc( s2 );

		ReadFile( h1, b1, s1, &num, NULL );
		ReadFile( h2, b2, s1, &num, NULL );

		if( memcmp( b1+512, b2+512, s1-512) == 0 ){
			
			ret = FALSE;
		}

		free(b1); free(b2);
	}
	
	CloseHandle(h1);
	CloseHandle(h2);
	return ret;
}

BOOL FileNotSame( PCHAR n1, PCHAR n2, BOOL aDeep ){

	if ( !aDeep ) return strcmp( n1, n2 );

	else return FileContentNotSame( n1, n2 );

}

void Add2List( PList aHead, AA* aRec ){

	BB* pB = NULL;
	DD* pD = NULL;

	DWORD size = 0;

	PList pL = NULL;


	pB = FindOrBuild( aHead, aRec );

	if ( pB){

		pL = pB->b.next;

		while(  pL != &(pB->b) ){

			pD = (DD*)pL;

			if ( FileNotSame( pD->n, aRec->name, TRUE) ){
				
				pL = pL->next;
				continue;
			}else{
				
				return;
			}		
		}

		if ( pL == &(pB->b)){

			size = sizeof(DD) + strlen(aRec->name);
			
			pD = (DD*)malloc( size );

			memset( pD, 0, size );

			pD->vid = aRec->vid;

			strcpy( pD->n, aRec->name );

			INSERTLISTTAIL( (&(pB->b)), (&(pD->b)) );

			pB->Count++;
		}
		
	}
}


DWORD classifyMistake( PList aHead, PCHAR aFileName ){

	DWORD count = 0;
	DWORD ret = 0;
	HANDLE hFile = 0;
	DWORD num = 0;
	DWORD RetV = 0;
	
	AA tmp ;

	FILE *log = NULL;

//	INITLISTHEAD( &gHead );

	hFile = CreateFile(
		
		aFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile != INVALID_HANDLE_VALUE ){

		//log = fopen( "noindex.txt", "w" );

		do{
 
			ret = ReadFile( hFile, &tmp, sizeof(AA), &num, NULL );

			if ( num == sizeof(AA) ){

				count++;

				if ( tmp.IndexValue == 0 ) {
					
					//fprintf( log, "%s\n", tmp.name );
					continue;
				}
				Add2List( aHead, &tmp );

			}else{
				RetV = 0; break;
			}

		}while( 1);

		//fclose( log );

		CloseHandle( hFile );
	}

	return RetV;
}

DWORD classifyVirus( PList aHead, PList aHead2, PCHAR aFileName ){

	DWORD count = 0;
	DWORD ret = 0;
	HANDLE hFile = 0;
	DWORD num = 0;
	DWORD RetV = 0;
	
	AA tmp ;

	FILE *log = NULL;

//	INITLISTHEAD( &gHead );

	hFile = CreateFile(
		
		aFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile != INVALID_HANDLE_VALUE ){

		//log = fopen( "noindex.txt", "w" );

		do{
 
			ret = ReadFile( hFile, &tmp, sizeof(AA), &num, NULL );

			if ( num == sizeof(AA) ){

				count++;

				if ( tmp.IndexValue == 0 ) {
					
					//fprintf( log, "%s\n", tmp.name );
					continue;
				}
				if ( Find( aHead2, tmp.IndexValue) )Add2List( aHead, &tmp );

			}else{
				RetV = 0; break;
			}

		}while( 1);

		//fclose( log );

		CloseHandle( hFile );
	}

	return RetV;
}




void LikOpenFile( PList aHead ){

	DD *aRec;
	FileInfo *pi= NULL;
	PList p = NULL;

	p = aHead->next;

	while( p != aHead ){

		aRec = (DD*)p;
		p = p->next;

		pi = &aRec->i;

		memset( pi, 0 , sizeof(FileInfo) );

		pi->hFile = CreateFile(
			
			aRec->n,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if ( pi->hFile != INVALID_HANDLE_VALUE ){
		
		}
	}
	
}

void LikCloseFile( PList aHead ){

	DD *aRec = NULL;

	FileInfo *pi= NULL;

	PList p;

	if ( aHead == NULL ) return;

	p = aHead->next;

	while ( p != aHead ){

		aRec = (DD*)p;

		p = p->next;

		if( aRec == NULL ) return;

		pi = &aRec->i;

		if ( pi->hFile != INVALID_HANDLE_VALUE ){

			CloseHandle( pi->hFile );
			memset( pi, 0 , sizeof(FileInfo) );
		}

	}
}

void LikReadFile( PList aHead, DWORD aOffset ){

	FileInfo *pi= NULL;
	DWORD res = 0, num = 0;

	PList p = NULL;
	DD *aRec = NULL;

	p = aHead->next;

	while( p != aHead ){

		aRec = (DD*)p;
		p = p->next;


		pi = &aRec->i;

		if ( pi->hFile != INVALID_HANDLE_VALUE ){

			memset( pi->Buff, 0, sizeof(DWORD)*LIKBUFF );

			SetFilePointer( pi->hFile, aOffset, NULL, FILE_BEGIN );
		
			res = ReadFile( pi->hFile, pi->Buff, sizeof(DWORD), &num, NULL );

			if ( res && (num == sizeof(DWORD) ) ){
			
				pi->offset = aOffset;
				pi->res = res;

			}else{

				memset( pi->Buff, 0, sizeof(DWORD)*LIKBUFF);
			}

			pi->Buff[1] = res;
			pi->Buff[2] = num;
		}
	}

}

BOOL comparemem( PDWORD* aHead, DWORD aCount, PDWORD aPvalue, DWORD aSize ){

	DWORD i;

	if ( aCount == 0 ) return FALSE;

	for( i=0; i<aCount; i++ ){

		if ( memcmp( (void*)aHead[i], aPvalue, sizeof(DWORD)*aSize )==0 ){
			
			return TRUE;
		}	
	}

	return FALSE;
}		

DWORD LikCompare( PList aHead, DWORD aCount, PDWORD **aRet ){

	PList p = NULL;
	DWORD total =0;

	DD* tmp = NULL;

	PDWORD *parry = (PDWORD*)malloc(sizeof(PDWORD)* aCount);

	memset( parry, 0, sizeof(PDWORD)* aCount );

	p = aHead->next;

	while( p != aHead ){

		tmp = (DD*)p;
		p = p->next;

		if ( comparemem( parry, total, tmp->i.Buff, LIKBUFF ) == FALSE ){

			parry[total] = tmp->i.Buff; total++;
		}
	}
	
	*aRet = parry;
	return total;
}


DWORD GetSectionTbl( HANDLE hFile, PIMAGE_SECTION_HEADER pSct, DWORD aSize ){

	IMAGE_DOS_HEADER mz;
	IMAGE_NT_HEADERS nthd;

	DWORD result = 0, num = 0, offset= 0, size = 0;


	SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
	result = ReadFile( hFile, &mz, sizeof( IMAGE_DOS_HEADER), &num, NULL);

	if ( (!result) || (num != sizeof(IMAGE_DOS_HEADER)) ){
	
		return FALSE;
	}

		if( mz.e_magic != 0x5a4d ){

		//printf("not PE\n");
		return FALSE;
	}

	offset = mz.e_lfanew;
	memset( &nthd, 0, sizeof(IMAGE_NT_HEADERS) );

	SetFilePointer( hFile, offset, NULL, FILE_BEGIN );
	result = ReadFile( hFile, &nthd, sizeof(IMAGE_NT_HEADERS), &num, NULL);

	if ( (!result) || (num != sizeof(IMAGE_NT_HEADERS)) ){
	
		return FALSE;
	}

	if( nthd.Signature != 0x4550l ){

		//printf("not PE\n");
		return FALSE;
	}

	size = sizeof(IMAGE_SECTION_HEADER)* nthd.FileHeader.NumberOfSections;

	if ( size > aSize ) return FALSE;

	result = ReadFile( hFile, pSct, size, &num, NULL);

	if ( (!result) || (num != size) ){
	
		return FALSE;
	}


	return TRUE;
}

DWORD GetRange( PCHAR aName, PeInfoFromFile *aPe , PSecSmry aSmry){

	HANDLE hFile = INVALID_HANDLE_VALUE;

	PIMAGE_SECTION_HEADER pSct = NULL, pSct0 = NULL;

	DWORD fSize = 0;

	DWORD TblSize = 0;
	DWORD ret = 0;

	hFile = CreateFile(
		
		aName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	
	if ( hFile == INVALID_HANDLE_VALUE ) return FALSE;

	fSize = GetFileSize( hFile, NULL );

	VerifyPE( hFile, aPe );

	TblSize = sizeof(IMAGE_SECTION_HEADER)* aPe->SectionNum;

	pSct = (PIMAGE_SECTION_HEADER)malloc( TblSize );

	if ( GetSectionTbl( hFile, pSct, TblSize ) == FALSE ){
	
		return FALSE;
	}

	pSct0 =  pSct;

	{
		DWORD i,j =0;

		for( i = 0; i < aPe->SectionNum; i++, pSct0++ ){
		
			if ( pSct0->SizeOfRawData &&(pSct0->PointerToRawData > aPe->RawDataOffset) ){

				aSmry[j].fStart = pSct0->PointerToRawData;
				aSmry[j].fSize = pSct0->SizeOfRawData;

				j++;

				if ( j== 8)break;
			
			}
		}

		ret = j ;		
	}

	if ( ret >= 1){

		if ( fSize - (aSmry[ret-1].fStart + aSmry[ret-1].fSize)  > 16 ){

			aSmry[ret].fStart = (aSmry[ret-1].fStart + aSmry[ret-1].fSize);		
			aSmry[ret].fSize = fSize - aSmry[ret].fStart +1;

			ret++;
		}

	}else{

		if ( fSize - (aPe->RawDataOffset + aPe->RawDataSize) > 16 ){
		
			aSmry[ret].fStart = (aPe->RawDataOffset + aPe->RawDataSize);
			aSmry[ret].fSize = fSize - aSmry[ret].fStart +1;
			
			ret++;
		}	
	}
	
	CloseHandle( hFile );
	free( pSct);

	return ret;
}

BOOL LikExclusive( PList aHead, PDWORD *parry, DWORD aCount, DWORD aOffset ){

	DD *tmp = NULL; 
	PList p = NULL;

	BOOL ret = TRUE;

	LikOpenFile( aHead );
	LikReadFile( aHead, aOffset );

	p = aHead->next;

	while( p != aHead ){

		tmp = (DD*)p;
		p = p->next;

		if ( comparemem( parry, aCount, tmp->i.Buff, LIKBUFF ) == TRUE ){
			
			ret = FALSE; break;
		}
	}
	
	LikCloseFile( aHead );

	return ret;
}

DWORD FileSize( PCHAR aName ){

	DWORD ret = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	hFile = CreateFile(
		
		aName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	
	if ( hFile == INVALID_HANDLE_VALUE ) return ret;

	ret = GetFileSize( hFile, NULL );

	CloseHandle( hFile );

	return ret;
}

PCHAR FindSmallestFileName( PList aHead ){

	DD *tmp = NULL; 
	PList p = NULL;

	PCHAR ret = NULL;
	DWORD fSize = 0xffffffffl, a = 0;

	p = aHead->next;

	while( p != aHead ){

		tmp = (DD*)p;
		p = p->next;

		a = FileSize( tmp->n );

		if (  a < fSize ){
		
			fSize = a; ret = tmp->n;
		}
	}
	return ret;
}


DWORD GetBestOffset( BB* nvHead, BB* vHead ){

	DWORD nvCount =0, vCount= 0;

	PeInfoFromFile Pe;
	SecSmry smry[8];

	DWORD res;
	PCHAR pName = ((DD*)(vHead->b.next))->n;
	PList lHead = NULL, p =NULL;

	DWORD i, j, offset, count;

	PDWORD *ret;

	DWORD secnum = 0;

	DWORD RetVal = 0;

	memset( smry, 0, sizeof(SecSmry)*8 );

	pName = FindSmallestFileName( &vHead->b);

	secnum = GetRange( pName , &Pe, smry );

	if ( secnum == 0 ) {

		printf( "group:%d NO Range\n", vHead->IndexValue );

		return RetVal;
	}


	printf( "group: %d  NotVirus= %d Virus == %d\n", vHead->IndexValue, nvHead->Count, vHead->Count );

	if ( vHead->Count >1 ){


			lHead =  &vHead->b;
			count = vHead->Count;

			LikOpenFile( lHead );

			for( i =0; i < secnum; i++ ){

				for ( j =0; j < smry[i].fSize -8; j+=4 ){
				// read

					offset  = smry[i].fStart+j;
					LikReadFile( lHead, offset );	

					res = LikCompare( lHead, count, &ret );

					if ( res == count ){

						if ( LikExclusive( &nvHead->b, ret, res, offset ) == TRUE){

							printf( "offset = %d  Vnum= %d\n", offset, res );

							RetVal = offset;

							free( ret);
							goto endloop2;
						}
					}			
					free( ret );
				}
			}
		
			for( i =0; i < secnum; i++ ){

				for ( j =0; j < smry[i].fSize -8; j+=4 ){
				// read

					offset  = smry[i].fStart+j;
					LikReadFile( lHead, offset );	

					res = LikCompare( lHead, count, &ret );

					if ( LikExclusive( &nvHead->b, ret, res, offset ) == TRUE){

						printf( "offset = %d  Vnum= %d\n", offset, res );

						RetVal = offset;

						free( ret);
						goto endloop2;
					}
					
					free( ret );
				}
			}

endloop2:
			LikCloseFile( lHead );
		

	}else{//vHead->Count == 1


		if ( nvHead->Count >1 ){

			lHead =  &nvHead->b;
			count = nvHead->Count;

			LikOpenFile( lHead );
		
			for( i =0; i < secnum; i++ ){

				for ( j =0; j < smry[i].fSize -8; j+=4 ){
				// read

					offset  = smry[i].fStart+j;
					LikReadFile( lHead, offset );	

					res = LikCompare( lHead, count, &ret );

					
					if ( LikExclusive( &vHead->b, ret, res, offset ) == TRUE){

						printf( "offset = %d  Vnum= %d\n", offset, res );

						RetVal = offset;

						free( ret);
						goto endloop3;
					}
					
					free( ret ); ret = NULL;
				}
			}

endloop3:
			LikCloseFile( lHead );
		
		}else{//nvHead->Count ==1 and vHead->Count ==1

			PList p1 = NULL;
					
			p1 = vHead->b.next;

			REMOVEFROMLIST( p1 );
			INSERTLISTTAIL( (&(nvHead->b)), p1 );

			lHead =  &nvHead->b;
			count = 2;

			LikOpenFile( lHead );
		
			for( i =0; i < secnum; i++ ){

				for ( j =0; j < smry[i].fSize -8; j+=4 ){
				// read

					offset  = smry[i].fStart+j;
					LikReadFile( lHead, offset );	

					res = LikCompare( lHead, count, &ret );

					
					if ( res == count ){

						printf( "offset = %d  Vnum= %d\n", offset, res );

						RetVal = offset;

						free( ret);
						goto endloop4;
					}
					
					free( ret ); ret = NULL;
				}
			}

endloop4:
			LikCloseFile( lHead );


			REMOVEFROMLIST( p1 );
			INSERTLISTTAIL( (&(vHead->b)), p1 );

		}

	}

	printf( "offset$ = %d\n", RetVal );

	return RetVal;

}

BOOL GetFileData( PCHAR aName, DWORD aOffset, DWORD *aRet ){

	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD val = 0, num = 0;
	BOOL  res = FALSE;

	*aRet = 0;

	hFile = CreateFile(
		
		aName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile != INVALID_HANDLE_VALUE ){

		if ( SetFilePointer( hFile, aOffset, NULL, FILE_BEGIN ) == aOffset ){

			res =  ReadFile( hFile, &val, sizeof(DWORD), &num, NULL );

			CloseHandle( hFile );

			if ( res && (num == sizeof(DWORD)) ){
				
				*aRet = val;
				return TRUE;

			}else{
			
				return FALSE;
			}
		
		}else{

			CloseHandle( hFile );
			return FALSE;
		}

	}else{
	
		return FALSE;
	}
}

void set_value( IndexElement* p, DWORD aVal, DD* pvi ){

	p->val = aVal;
	p->vi.VID = pvi->vid;

	{ 
	  PCHAR pm = NULL;
	  DWORD len = 0;

	  len = strlen( pvi->n );

	  pm = (PCHAR)malloc( len );

	  memcpy( pm, pvi->n, len );

	  p->vi.VName = (DWORD)pm;
	  p->vi.VNlen = len;
	
	}
}


DWORD InsertRec( PIndexElementArray aP, DWORD aValue, DD* pvi){

	IndexElement* p = aP->ie;
	
	int i = 0;

	if ( aP->count == 0 ){
	
		set_value( p, aValue, pvi ); 
		
		aP->count ++;
		
		return aP->count;
	}

	if ( aP->count ){

		for( i = 0; i < aP->count; i++ ){

			if ( p[i].val < aValue ) continue;
			else break;	
		}

		if( i == aP->count ){

			set_value( p+i, aValue, pvi );
			
			aP->count ++;

			return aP->count;		
		}
		
		if ( (i < aP->count)&&(p[i].val != aValue) ){

			int j = 0;

			for( j = aP->count -1; j >= i; j-- ){

				memcpy( &(p[j+1]), &(p[j]), sizeof(IndexElement) );	
			}
		
			set_value( p+i, aValue, pvi );
			
			aP->count ++;
		}

	}

	return aP->count;
}

void build_rec(  BB* vHead, DWORD offset, PInfoNode pNode ){

	DWORD count = vHead->Count;

	PList lHead = &vHead->b;

	PList p = NULL;

	DD* tmp = NULL;

	DWORD size = 0;
	DWORD val = 0;

	PIndexElementArray pa = NULL;

	size = sizeof(IndexElementArray) + (count-1)* sizeof(IndexElement);

	pa = (PIndexElementArray)malloc( size );

	if ( pa ){
		
		memset( pa, 0, size );
		
		pa->offset = offset;

		p = lHead->next;

		while( p != lHead ){
		
			tmp = (DD*)p;
			p = p->next;

			if ( GetFileData( tmp->n, offset, &val ) ){

				InsertRec( pa, val, tmp );		
			}		
		}

		if ( pa->count ){

			pNode->Type = 1;
			pNode->u.p = pa;

		}else{
			free( pa ); pa = NULL;
		}

#if 1
		if ( pa ){

			DWORD i = 0;

			for( i = 0; i < pa->count ; i++){

				printf( " %u : %u ;", pa->ie[i].vi.VID, pa->ie[i].val );		
			}
			printf( "\n");	
		}
#endif

	}

}


CHAR gBuff[512];

void enmu( BB* aHead, FILE* aLog ){

	PList p = NULL;
	PList p1 = NULL;

	PList lHead = &aHead->b;
	DD *tmp = NULL;

	BB* vHead = NULL;

	DWORD id = 0, offset = 0;

	PInfoNode pNode = NULL;

	fprintf( aLog, "group: %u\n", aHead->IndexValue );

	memset( gBuff, 0, 512 );
	Index2Name( aHead->IndexValue, gBuff, &id );

	vHead = Find( &gHead2, aHead->IndexValue );

	if ( aHead->IndexValue == 3280496 ){
		printf("timer\n");
	}

	fprintf( aLog, "%d sample: %s\n", id, gBuff );

	offset = GetBestOffset( aHead, vHead );

	pNode = Index2node( vHead->IndexValue );

	if ( offset != 0 ){

		build_rec( vHead, offset, pNode );
	}
	
	p = lHead->next;

	while( p != lHead ){
	
		tmp = (DD*)p;
		p = p->next;

		{
			fprintf( aLog, "%d %s\n" , tmp->vid, tmp->n );
			free( tmp );
		}	
	}

	if ( vHead ){

		lHead = &vHead->b;
		p1 = lHead->next;

		while( p1 != lHead ){
	
			tmp = (DD*)p1;
			p1 = p1->next;

			{
				fprintf( aLog, "%d %s\n" , tmp->vid, tmp->n );
			}
		}
	
	}

	fprintf( aLog, "\n" );


	free( aHead );
}

void qq( FILE* aLog ){

	DWORD total = 0;
	PList p = gHead.next;
	BB *p0 = NULL;

	while( p != &gHead ){

		p0 = (BB *)((DWORD)p - 8 );

		REMOVEFROMLIST(p);
		p = gHead.next;

		{
			total += p0->Count;

			if ( p0->Count > 0){

				enmu( p0, aLog );
			}
		}

	}

	printf( "%u\n", total );

}

void IndexDecode( PIndex start, DWORD aSize ){

	DWORD lBase = (DWORD)start;

	PIndex p = start;

	DWORD count = 0, i = 0 , size = 0;

	while( count < aSize ){

		for( i =0; i < p->count ; i++ ){

			if ( p->array[i].type == NEXT_TYPE_INDEX ){

				p->array[i].RangeOrIndex.next = (PIndex)((DWORD)(p->array[i].RangeOrIndex.next) + lBase);
			}	
		}

		size = (sizeof(Index) + (p->count -1)*sizeof(IndexRecord));
		count += size;

		p = (PIndex)((DWORD)p + size );
	}

}

void* gBase = NULL;

PIndex gIndex = 0;

PInfoNode gInfoBlock = NULL;
DWORD gInfoCount = 0;

PCHAR gName = 0;

void LoadFile( PCHAR name ){

	HANDLE hFile = INVALID_HANDLE_VALUE;

	StdHead std;
	ExtHeadPeer ext;

	DWORD num, offset, count;


	hFile = CreateFile(
		
		name,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	ReadFile( hFile, &std, sizeof(StdHead), &num, NULL );

	if ( std.ExtSize !=  sizeof(ExtHeadPeer) ) return;

	ReadFile( hFile, &ext, sizeof(ExtHeadPeer), &num, NULL );

	offset = sizeof(StdHead) + sizeof(ExtHeadPeer) + ext.SizeOfPeer;

	count = ext.TotalSize - ext.SizeOfPeer;


	gBase = malloc( count );

	if ( gBase ){

		SetFilePointer( hFile, offset, NULL, FILE_BEGIN );

		ReadFile( hFile, gBase, count, &num, NULL );

		if ( count == num ){

			gIndex = (PIndex)gBase;

			IndexDecode( gIndex, ext.SizeOfIndex );

			gInfoBlock = (PInfoNode)((DWORD)gBase + ext.SizeOfIndex);

			gInfoCount = ext.SizeOfInfo / sizeof(InfoNode);

			gName = (PCHAR)((DWORD)gBase + ext.SizeOfIndex + ext.SizeOfInfo);

		}
	}

	CloseHandle( hFile );
}


#if 1

PInfoNode HalfFind(  DWORD aValue, PInfoNode begin, DWORD count ){

	LONG lLow, lHigh, lMid;

	PInfoNode ret = NULL;

	lLow = 0; 
	lHigh = count -1;


	while( lLow <= lHigh ){
	
		lMid = ( lLow + lHigh )/2;

		if ( begin[lMid].Index == aValue ){

			ret = begin + lMid;
			break;

		}else if ( begin[lMid].Index > aValue ){

			lHigh = lMid -1;

		}else{ 

			lLow = lMid +1;
		}

	}

	return ret;
}

PInfoNode FindFromIndex( DWORD aValue, PIndex pTop ){

	DWORD i = 0;
	PIndex curr = pTop;
	PIndexRecord p = NULL;

	PInfoNode pInfo = NULL;

	while( curr ){

		for( i = 0; i < curr->count; i++ ){

			p = &(curr->array[i]);

			if ( ( p->first <= aValue )&&( p->last >= aValue) ){

				if( p->type == NEXT_TYPE_INDEX ){
				
					curr = p->RangeOrIndex.next;
					break;

				}else if ( p->type == NEXT_TYPE_RANGE ){

					pInfo = (PInfoNode)((DWORD)gInfoBlock + p->RangeOrIndex.Range.postion);

					return HalfFind( aValue, pInfo, p->RangeOrIndex.Range.count );			
				}
			
			}		
		}

		if ( i == curr->count ) return NULL;
	}

	return NULL;
}

#endif

DWORD Index2Name( DWORD aValue, PCHAR aRet, DWORD *aVid ){

	PInfoNode p =  NULL;

	PCHAR from = NULL;

	DWORD size = 0;

	if ( aRet == NULL || aVid == NULL ) return 0;

	*aVid = 0;

	p = FindFromIndex( aValue , gIndex );

	if ( p &&  (p->Index == aValue )){

		from = gName + p->u.v.VName;

		size = p->u.v.VNlen;

		memcpy( aRet, from, size );
		
		*aVid = p->u.v.VID;

		return size;	
	}

	return 0;
}

PInfoNode Index2node( DWORD aValue ){

	PInfoNode p =  NULL;

	p = FindFromIndex( aValue , gIndex );

	if ( p &&  (p->Index == aValue )){
	
		return p;
	}

	return NULL;
}

DWORD CountNameSize( PInfoNode p, DWORD count ){

	DWORD i;
	DWORD total = 0;

	for( i= 0; i< count; i++ ){

		if ( p[i].Type == 0 ){

			total += p[i].u.v.VNlen;	
		}

		if ( p[i].Type == 1 ){

			PIndexElementArray  pa = (PIndexElementArray)(p[i].u.p);
			IndexElement *pie = pa->ie;

			DWORD j = 0;

			for ( j = 0; j < pa->count; j++ ){
				
				total += pie[j].vi.VNlen;
			}
		}	
	}

	return total;
}

DWORD CountMiscSize( PInfoNode p, DWORD count ){

	DWORD i;
	DWORD total = 0;
	DWORD size = 0;

	for( i= 0; i< count; i++ ){

		if( p[i].Type == 1 ){

			PIndexElementArray  pa = (PIndexElementArray)(p[i].u.p);

			size = sizeof(IndexElementArray) + (pa->count-1)* sizeof(IndexElement);

			total += size;		
		}
	}

	return total;
}

MemAlloc gMisc, gName2;

void rebuildlib(){

	DWORD size1 = 0, size2 = 0;

	DWORD i = 0;

	PInfoNode p = NULL ;//gInfoBlock

	size1 = CountMiscSize( gInfoBlock, gInfoCount );
	size2 =	CountNameSize( gInfoBlock, gInfoCount );

	printf( "%d\n", size1 );
	printf( "%d\n", size2 );

	InitMemAlloc( &gMisc );
	gMisc.info.allocBytes = size1;
	BuildMemAlloc( &gMisc );

	InitMemAlloc( &gName2 );
	gName2.info.allocBytes = size2;
	BuildMemAlloc( &gName2 );

	for( i = 0; i < gInfoCount; i++ ){

		p=  i + gInfoBlock;

		if ( p->Type == 0 ){

			DWORD len = p->u.v.VNlen;

			PCHAR pn =  p->u.v.VName +gName;

			PCHAR pnew =  NULL;

			pnew = (PCHAR)MallocMem( &gName2, len );

			memcpy( pnew, pn, len );

			p->u.v.VName = (DWORD)pnew - (DWORD)gName2.pMemBase;		
		
		}

		if ( p->Type == 1 ){

			DWORD j = 0; PCHAR pchr = NULL, pchr1 = NULL;

			PIndexElementArray  pa = (PIndexElementArray)(p->u.p);

			DWORD size = sizeof(IndexElementArray) + (pa->count-1)* sizeof(IndexElement);

			PIndexElementArray pnewa = (PIndexElementArray)MallocMem( &gMisc, size );

			memcpy( pnewa, pa, size );

			free( pa ); pa = NULL;

			for ( j = 0; j<pnewa->count; j++ ){

				DWORD len = pnewa->ie[j].vi.VNlen;

				pchr = (PCHAR)(pnewa->ie[j].vi.VName);

				pchr1 = (PCHAR)MallocMem( &gName2, len );

				memcpy( pchr1, pchr, len );

				pnewa->ie[j].vi.VName = (DWORD)pchr1 - (DWORD)gName2.pMemBase;
			
			}

			p->u.p = (void*)((DWORD)pnewa - (DWORD)gMisc.pMemBase);
		
		}	
	}
}

void RefreshLib( PCHAR aOldLib, PCHAR aNewLib ){

	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hFile2 = INVALID_HANDLE_VALUE;

	StdHead std;
	ExtHeadPeer ext;

	ExtHeadPeer ExtNew;

	DWORD num, offset, count;	DWORD size = 0; PCHAR pBuff = NULL;

	if ( aOldLib == NULL ) return;
	if ( aNewLib == NULL ) return;

	hFile = CreateFile(
		
		aOldLib,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if ( hFile == INVALID_HANDLE_VALUE ) return;

	ReadFile( hFile, &std, sizeof(StdHead), &num, NULL );

	if ( std.ExtSize !=  sizeof(ExtHeadPeer) ) return;

	ReadFile( hFile, &ext, sizeof(ExtHeadPeer), &num, NULL );

	if ( num != sizeof(ExtHeadPeer) ) return;

	memset( &ExtNew, 0, sizeof(ExtHeadPeer) );

	ExtNew.OffsetOfPeer = ext.OffsetOfPeer;
	ExtNew.SizeOfPeer = ext.SizeOfPeer;

	ExtNew.OffsetOfIndex = ext.OffsetOfIndex;
	ExtNew.SizeOfIndex = ext.SizeOfIndex;

	// copy

	ExtNew.OffsetOfInfo = ext.OffsetOfInfo;
	ExtNew.SizeOfInfo = ext.SizeOfInfo;

	ExtNew.OffsetOfMisc = ExtNew.SizeOfPeer + ExtNew.SizeOfIndex + ExtNew.SizeOfInfo;
	ExtNew.SizeOfMisc = gMisc.BytesTotal;

	ExtNew.OffsetOfName = ExtNew.OffsetOfMisc + ExtNew.SizeOfMisc;
	ExtNew.SizeOfName = gName2.BytesTotal;

	ExtNew.TotalSize = 

		ExtNew.SizeOfPeer + ExtNew.SizeOfIndex + ExtNew.SizeOfInfo
		+ ExtNew.SizeOfMisc + ExtNew.SizeOfName;

	hFile2 = CreateFile(
		
		aNewLib,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	WriteFile( hFile2, &std, sizeof(StdHead), &num, NULL );
	WriteFile( hFile2, &ExtNew, sizeof(ExtHeadPeer), &num, NULL );

	size = ext.SizeOfPeer + ext.SizeOfIndex;

	pBuff = (PCHAR)malloc( size );

	if ( pBuff ){
	
		ReadFile( hFile, pBuff, size, &num, NULL );

		if( num == size )WriteFile( hFile2, pBuff, size, &num, NULL );

		free( pBuff);
	}

	CloseHandle( hFile );

	WriteFile( hFile2, gInfoBlock, gInfoCount* sizeof(InfoNode), &num, NULL );

	WriteFile( hFile2, gMisc.pMemBase, gMisc.BytesTotal, &num, NULL );

	WriteFile( hFile2, gName2.pMemBase, gName2.BytesTotal, &num, NULL );

	CloseHandle( hFile2 );

}

int main(int argc, char* argv[])
{

	FILE *log = NULL;

	CHAR FullName[512];
	CHAR NewLib[512];

	// build a link using records

	// read a record 

	// compare files  related record

	// add change into record and log

	if ( argc != 2 ) return 0;

	strcpy( FullName, argv[1] );
	strcat( FullName, "peerarray.img" );

	LoadFile( FullName );

	{
		INITLISTHEAD( &gHead );

		strcpy( FullName, argv[1] );
		strcat( FullName, "mistake.log" );

		classifyMistake( &gHead, FullName );

		INITLISTHEAD( &gHead2 );

		strcpy( FullName, argv[1] );
		strcat( FullName, "virus.log" );

		classifyVirus( &gHead2, &gHead, FullName );
	}

	strcpy( FullName, argv[1] );
	strcat( FullName, "group.txt" );

	log = fopen( FullName, "w" );

	qq( log );


	rebuildlib();

	strcpy( FullName, argv[1] );
	strcat( FullName, "peerarray.img" );

	strcpy( NewLib, argv[1] );
	strcat( NewLib, "NewLib.img" );

	RefreshLib( FullName, NewLib );
	
	fclose( log );
	
	return 0;
}
