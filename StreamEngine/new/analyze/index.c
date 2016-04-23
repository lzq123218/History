

#include "string.h"
#include "viruslib_anly.h"


MemAlloc gIndex;


PIndex BuildI( DWORD count , PInfoNode first, DWORD aBase ){

	DWORD num  = 0;
	DWORD size = 0;
	DWORD i = 0;

	PIndex tmp = NULL;

	DWORD start = 0, end = 0;

	PIndex p = NULL;

	if ( count ==0 || first == NULL ) return NULL;

	if ( count <= NEED_BUILD_INDEX ) return NULL;

	size = sizeof(Index) + (INDEX_COUNT_PER_LEVEL-1)*sizeof(IndexRecord);

	p = (PIndex)MallocMem( &gIndex, size );

	memset( p, 0, size );

	size = count/4;// must adjust size 
	size++;
	
	p->count = INDEX_COUNT_PER_LEVEL;

	for( i = 0; i < INDEX_COUNT_PER_LEVEL ; i ++ ){

		start = i*size;

		if ( (start + size) < count ){
		
			end = start +size -1;
		}else{
		
			end = count -1;
		}

		p->array[i].first = (first+start)->Index;
		p->array[i].last = (first+end)->Index;

		tmp = BuildI( (end-start+1),(first+start), aBase );

		if ( tmp ){
			
			p->array[i].type = NEXT_TYPE_INDEX;
			p->array[i].RangeOrIndex.next = tmp;

		}else{
		
			p->array[i].type = NEXT_TYPE_RANGE;
// think about here
			p->array[i].RangeOrIndex.Range.postion = (DWORD)(first+start)- aBase;
			p->array[i].RangeOrIndex.Range.count = (DWORD)(end-start +1);
		}		
	}

	return p;
}

void DestroyI( PIndex  p ){

	DWORD i = 0;

	if ( p == NULL ) return;

	for( i = 0; i < p->count; i++ ){

		if ( p->array[i].type == NEXT_TYPE_RANGE );//printf( "%u ~ %u\n", p->array[i].first, p->array[i].last );

		if ( p->array[i].type == NEXT_TYPE_INDEX ) DestroyI( p->array[i].RangeOrIndex.next );
	}
	
	FreeMem( &gIndex, p);
}

void IndexEncode( PIndex start, DWORD aSize ){

	DWORD lBase = (DWORD)start;

	PIndex p = start;

	DWORD count = 0, i = 0 , size = 0;

	while( count < aSize ){

		for( i =0; i < p->count ; i++ ){

			if ( p->array[i].type == NEXT_TYPE_INDEX ){

				p->array[i].RangeOrIndex.next = (PIndex)( (DWORD)(p->array[i].RangeOrIndex.next) - lBase );
			}	
		}

		size = (sizeof(Index) + (p->count -1)*sizeof(IndexRecord));
		count += size;

		p = (PIndex)((DWORD)p + size );
	}

}

