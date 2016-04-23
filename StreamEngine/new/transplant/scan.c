
#include "eng.h"

extern DWORD GetRealValue( PStreamObject pObj, DWORD offset, const BYTE* buff, DWORD* p );
extern DWORD GetOffsetValue2( PStreamObject pObj, DWORD offset, const BYTE* buff, DWORD *pErr );

extern PeerAddress gPeerAddress;


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

					pInfo = (PInfoNode)((DWORD)(gPeerAddress.pInfoBlock) + p->RangeOrIndex.Range.postion);

					return HalfFind( aValue, pInfo, p->RangeOrIndex.Range.count );			
				}
			
			}		
		}

		if ( i == curr->count ) return NULL;
	}

	return NULL;
}

PInfoNode FindInfoNode( DWORD aValue ){

	if ( gPeerAddress.pIndex ) return FindFromIndex( aValue, gPeerAddress.pIndex );
	else
	return HalfFind( aValue, gPeerAddress.pInfoBlock, gPeerAddress.InfoCount );

}

/* this function is only called by scanlib3 */

static Element* FindElementFromPeerArray( PeerArray* aNode, DWORD aValue ){

	Element* lSub = NULL;
	long lLow, lHigh, lMid;

	if( aNode == NULL ) return NULL;

	if( aNode->count == 1 ){

		if ( aNode->content[0].value == aValue ){
			
			return aNode->content;
		}else{
			return NULL;
		}

	}else{

		lLow = 0; 
		lHigh = aNode->count -1;

		while( lLow <= lHigh ){
		
			lMid = ( lLow + lHigh )/2;

			if ( aNode->content[lMid].value == aValue ){

				lSub = &( aNode->content[lMid] );
				break;

			}else if ( aNode->content[lMid].value > aValue ){

				lHigh = lMid -1;

			}else{ 

				lLow = lMid +1;
			}
		
		}
	}

	return lSub;
}

/* this function is only called by ScanLib3*/

void InternalScan3( PStreamObject pObj, const BYTE* buff ){

	PeerArray* curr = (PeerArray*)pObj->viruslib2;
	Element* lFound = NULL;

	DWORD lValue = 0;
	DWORD lErr = 0;

	if ( pObj->usinglibtype != 0 )return;

	do{

		if ( curr == NULL ) break;
	
		lValue = GetRealValue( pObj, curr->offset, buff, &lErr );

		if ( lErr == ERR_DATA_NOT_READY ){

			break;

		}else if ( lErr == ERR_DATA_PASSED ){ 

			curr = NULL;break; 
		}

		lFound = FindElementFromPeerArray( curr, lValue );

		if ( lFound ){

			curr = lFound->sub;
			
			if ( curr == NULL ){
				
				//pObj->virusfound = lFound->id;
				PInfoNode tmp = NULL;
				
				pObj->virusindex = (DWORD)lFound - (DWORD)gPeerAddress.base;


				tmp = FindInfoNode( pObj->virusindex );

				if (tmp){
					
					if ( tmp->Type == 0 )pObj->virusfound = tmp->u.v.VID;
					
					if ( tmp->Type ==1 ){

						//change usinglibtype 
					
						pObj->usinglibtype = 1;

						curr =  (PeerArray*)((DWORD)tmp->u.p + (DWORD)gPeerAddress.pMisc);

						break;
					}
				}				
			} 
		
		}else{
		
			curr = NULL;
		}

	}while( curr );

	pObj->viruslib2 = (PTree)curr;

}

IndexElement* FindIeFromArray( PIndexElementArray p, DWORD aValue ){

	DWORD count = p->count;
	IndexElement* pIe = p->ie;
	IndexElement* ret = NULL;

	LONG lLow, lHigh, lMid;


	lLow = 0; 
	lHigh = count -1;

	while( lLow <= lHigh ){
	
		lMid = ( lLow + lHigh )/2;

		if ( pIe[lMid].val == aValue ){

			ret = pIe + lMid;
			break;

		}else if ( pIe[lMid].val > aValue ){

			lHigh = lMid -1;

		}else{ 

			lLow = lMid +1;
		}

	}

	return ret;
}

void InternalScanMisc( PStreamObject pObj, const BYTE* buff ){

	PIndexElementArray curr = (PIndexElementArray)pObj->viruslib2;

	DWORD lValue = 0;
	DWORD lErr = 0;

	IndexElement *pIe = NULL;

	if ( pObj->usinglibtype != 1 ) return;

	do{
		lValue = GetOffsetValue2( pObj, curr->offset, buff, &lErr );

		if ( lErr == ERR_DATA_NOT_READY ){
			
			break;
		}else if ( lErr == ERR_DATA_PASSED ){ 

			curr = NULL;
		}else{

			pIe = FindIeFromArray( curr, lValue );

			if ( pIe ){
			
				pObj->virusfound = pIe->vi.VID;
			}

			curr = NULL;
		}

	}while( curr );

	pObj->viruslib2 = (PTree)curr;

}