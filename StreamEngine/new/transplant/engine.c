
#ifdef __KERNEL__

	#define __NO_VERSION__

	#include "kernel_inc.h"

	#define printf printk

#else

	#include <stdio.h>
	#include <stdlib.h>

	#include <string.h>

#endif

#include "eng.h"
#include "hashtab.h"


#define VERSION_H	0
#define VERSION_L	1



DWORD RSGetEngineVersion(){

	return ( (VERSION_H << 8) + VERSION_L );
}

DWORD ComparedTimes = 0 ;

void ResetComparedTimes(){

	ComparedTimes = 0 ;
}

DWORD GetComparedTimes(){

	return ComparedTimes;
}

void PreProcess(PStreamObject pObj, const BYTE* pbDataBuff, DWORD Length){

	pbDataBuff = pbDataBuff;

//	pObj->back.need = FALSE;

	if ( pObj->recvdlen ){
		
		pObj->recvdlen += Length;
		pObj->currentstart = pObj->nextstart;
		pObj->nextstart += Length;
	
	}else{// first receive data,som data must be inited
		
		pObj->recvdlen = Length;
		pObj->back.on = FALSE; // if on == false, must not retrospect data;

		pObj->currentstart = 0;
		pObj->nextstart = Length;

		//pObj->startpos = 0;
	}

}

void PostProcess(PStreamObject pObj, const BYTE* pbDataBuff, DWORD Length){

	PCHAR des = NULL;
	PCHAR src = NULL;
	DWORD i;
/*
	if( pObj->back.need == FALSE ){
		
		pObj->back.on = FALSE; return;
	}
*/
	if ( pObj->back.on == FALSE ){
	
		pObj->back.on = TRUE;
	}

	src = ( (PCHAR) & (pbDataBuff[Length - 1]) );
	des = ( (PCHAR) & (pObj->back.Last9[9-1]) );
	
	for ( i = 0; i < 9 ; i ++ ){

		*des = *src;
		des --; src --;	
	}

}

DWORD gCount = 0 ;

PCHAR EngineMalloc( DWORD size ){
	
	gCount++;

#ifdef __KERNEL__

	return (PCHAR)kmalloc( size, GFP_KERNEL);
#else
	
	return (PCHAR)malloc( size );
#endif
}

BOOL EngineMfree( PCHAR start){

#ifdef __KERNEL__

	kfree( start );
#else

	free(start);
#endif

	gCount--;
	return TRUE;
}

List ExistingObj = { NULL, NULL };

void LockInsertObj(){}

void UnlockInsertObj(){}

void (*gReport)( DWORD aId, void* aArg ) = NULL;
void *gReportArg = NULL;

DWORD RSRegisterCallback( void(*aFun)( DWORD , void* ), void* aArg ){

	gReport = aFun;
	gReportArg = aArg;
	
	if ( gReport && aArg ){

		return TRUE;
	}
	return FALSE;
}

void RSDeregisterCallback( void ){

	gReport = NULL;
	gReportArg = NULL;
}

void EngineAddObj( PStreamObject pObj ){

	PList head = &ExistingObj;
	PList node = NULL;

	if ( pObj ){

		node = &(pObj->objlist);

		LockInsertObj();
		INSERTLISTTAIL( head, node );
		UnlockInsertObj();
	}
}

void EngineRemoveObj( PStreamObject pObj ){

	PList node = NULL;

	if( pObj ){
		
		node = &(pObj->objlist);

		LockInsertObj();
		REMOVEFROMLIST( node );
		UnlockInsertObj();
	}
}


PStreamObject EngineFindObj( DWORD context ){

	PStreamObject pObj = (PStreamObject)context;

	if ( pObj == NULL ) return pObj;
	
	LockInsertObj();

	if ( (pObj->type == OBJTYPE)&&(pObj->size == sizeof(StreamObject)) ){
		
	}else{

		pObj = NULL;	
	}

	UnlockInsertObj();
	return pObj;
}

void ResetStreamObj( PStreamObject pObj ){

	if ( (pObj->type == OBJTYPE)&&(pObj->size == sizeof(StreamObject)) ){
	
		pObj->scanlist.pre = NULL;
		pObj->scanlist.next = NULL;

		pObj->scanTree = NULL;
		//pObj->viruslib = global_root;

		pObj->viruslib2 = (PTree)gPeerArrayRoot;

		pObj->virusfound = FALSE;
		pObj->virusindex = 0;
		pObj->usinglibtype = 0;

		pObj->startpos = 0;
		pObj->recvdlen = 0;

		pObj->currentstart = 0;
		pObj->nextstart = 0;

		pObj->other =0;

		pObj->back.on = FALSE;

		memset( &(pObj->data), 0, sizeof(PeInfo) );
		memset( &(pObj->b64), 0, sizeof(B64Info) );
			
	}
}

void InitStreamObj( PStreamObject pObj ){

	pObj->type = OBJTYPE;
	pObj->size = sizeof(StreamObject);

	pObj->protocol = 0;

	pObj->status = OBJSTATUS_WAITTING_DATA;

	pObj->objlist.pre = NULL;
	pObj->objlist.next = NULL;

	ResetStreamObj( pObj);

//	pObj->hash = NULL;

}

DWORD RSEngineInit(){

	INITLISTHEAD( &ExistingObj );

	//INITTREE( &(VirusTreeRoot));

	gReport = NULL;
	gReportArg = NULL;

	return TRUE;
}

DWORD RSEngineUninit(){

	return 1;
}

int RSConnectEvent( DWORD aProtocol, DWORD *aRet ){

	PStreamObject pObj = NULL;

	pObj = (PStreamObject) EngineMalloc( sizeof(StreamObject) );

	if( pObj == NULL ) return -1;

	InitStreamObj( pObj );

	pObj->protocol = aProtocol;

	EngineAddObj(pObj);

	*aRet = (DWORD)pObj;
	return 0;
}

int RSDisconnectEvent( DWORD Context ){

	PStreamObject pObj = NULL;

	pObj = EngineFindObj( Context);

	if ( pObj ){

		EngineRemoveObj( pObj );

		EngineMfree( (PCHAR)pObj );
	}
	return 0;
}

int RSResetEvent( DWORD context ){

	PStreamObject pObj = (PStreamObject)context;

	if ( pObj == NULL ) return -1;
	
	LockInsertObj();

	ResetStreamObj( pObj );

	UnlockInsertObj();

	return 0;
}

/*

DWORD OffsetInRange( DWORD offset, DWORD low, DWORD high ){

#define OFFSET_SIZE 4

	if (
		( (offset + OFFSET_SIZE) <=high )&&( offset >= low )
	){
		return 1;
	}
	
	if (
		( offset < low )&&( (offset + OFFSET_SIZE) > low )
	){
		return 2;
	}

	return 0;
}

DWORD GetOffsetValue( PStreamObject pObj, DWORD offset, const BYTE* buff ){
	
	DWORD value = 0;
	DWORD condition;
	PCHAR src = NULL, des = NULL;
	DWORD i, j;

	condition = OffsetInRange( offset, pObj->currentstart, pObj->nextstart );
	
	if ( condition == 1 ){

		src = (PCHAR)( buff + ( offset - pObj->currentstart ));
		des = (PCHAR)(&value);

		for( i = 0; i < 4 ; i++ ){

			*des = *src; des ++; src ++;
		}
	
	}else if ( condition == 2 ){

		if( !(pObj->back.on) ) return 0;		

		i = pObj->currentstart - offset;

		src = (PCHAR) ( &( pObj->back.Last9[9-i] ) );
		des = (PCHAR) ( &value );

		for ( j = 0; j < i; j++ ){

			*des = *src ; des++; src++;
		}

		i = 4- i;// 4== feature size
		src = (PCHAR)buff;

		for ( j =0; j < i; j++ ){

			*des = *src; des++; src++;		
		}
		
	}

	return value;
}

*/
#define OFFSET_SIZE 4

BOOL OffsetInRange2( DWORD offset, DWORD low, DWORD high, DWORD *p ){

	if (
		( (offset + OFFSET_SIZE) <= high )&&( offset >= low )
	){
		*p = 1; return TRUE;
	}
	
	if (
		( offset < low )&&( (offset + OFFSET_SIZE) >low )
	){
		*p = 2; return TRUE;
	}

	if ( (offset + OFFSET_SIZE) > high ){

		*p = ERR_DATA_NOT_READY;
		
	}else if ( (offset+OFFSET_SIZE) <= low ){

		*p = ERR_DATA_PASSED; 
	}

	return FALSE;
}

DWORD GetOffsetValue2( PStreamObject pObj, DWORD offset, const BYTE* buff, DWORD *pErr ){

	DWORD value = 0;
	DWORD status = 0;
	PCHAR src = NULL, des = NULL;
	DWORD i, j;

	*pErr = 0;

	if ( OffsetInRange2( offset, pObj->currentstart, pObj->nextstart, &status) ){
	
		if ( status == 1 ){

			src = (PCHAR)( buff + ( offset - pObj->currentstart ));
			des = (PCHAR)(&value);

			for( i = 0; i < 4 ; i++ ){

				*des = *src; des ++; src ++;
			}
		
		}else if ( status == 2 ){

			if( !(pObj->back.on) ){ 
				
				//printf("*****************************************\n");
				*pErr = 1; return 0;		
			}

			i = pObj->currentstart - offset;

			src = (PCHAR) ( &( pObj->back.Last9[9-i] ) );
			des = (PCHAR) ( &value );

			for ( j = 0; j < i; j++ ){

				*des = *src ; des++; src++;
			}

			i = 4- i;// 4== feature size
			src = (PCHAR)buff;

			for ( j =0; j < i; j++ ){

				*des = *src; des++; src++;		
			}
			
		}

	}else{
/*
		if ( status == ERR_DATA_NOT_READY ){

			if ( offset < pObj->nextstart  ){
			
				pObj->back.need = TRUE;
			}
		}
*/
		*pErr = status;	
	}

	return value;

}

DWORD gPENum = 0; gOffsetNum =0; gOffsetSize = 0;

DWORD GetRealOffset( DWORD offset, PPeInfo pInfo ){

	DWORD tmp = 0;

	if( (offset & SECTION_OFFSET_PREFIX) == SECTION_OFFSET_PREFIX ){

		tmp = offset & (~SECTION_OFFSET_PREFIX);

		//tmp = pInfo->RawData + tmp * (pInfo->SizeOfRawData / SECTION_OFFSET_NUM );
		tmp = pInfo->RawData +
		(pInfo->SizeOfRawData / (gOffsetNum / gOffsetSize))*( tmp / gOffsetSize)+( tmp % gOffsetSize )*4;	


		return tmp;
	}

	return 0;
}
DWORD GetRealValue( PStreamObject pObj, DWORD offset, const BYTE* buff, DWORD* p ){

	PPeInfo pInfo = (PPeInfo) &(pObj->data);
	DWORD tmp = 0;
	
	if ( pInfo->status !=PE_STATUS_PE_SECTION_READY ){ *p = 8; return 0;}
	// PE_INFO_OFFSET
	if( (offset & PE_INFO_OFFSET_PREFIX) == PE_INFO_OFFSET_PREFIX ){

		tmp = offset & (~PE_INFO_OFFSET_PREFIX);
		
		*p = 0;
	
		switch( tmp ){

		case PE_INFO_NUMBER_OF_SECTIONS:
				return pInfo->sectionsnum;

		case PE_INFO_ADDRESS_OF_ENTRY:
				return pInfo->entryoffset;

		case PE_INFO_SIZE_OF_RAWDATA:
				return pInfo->SizeOfRawData;

		case PE_INFO_POINTER_TO_RAWDATA:
				return pInfo->RawData;

		default:
			*p = 9;
			return 0 ;
		}
	
	}
	// SECTION_OFFSET
	if( (offset & SECTION_OFFSET_PREFIX) == SECTION_OFFSET_PREFIX ){

		return GetOffsetValue2( pObj, GetRealOffset( offset, pInfo), buff, p);	
	}

	return 0;
}


extern void InternalScan3( PStreamObject pObj, const BYTE* buff );
extern void InternalScanMisc( PStreamObject pObj, const BYTE* buff );

void InternalScanlib( PStreamObject pObj, const BYTE* buff ){

	if( pObj->usinglibtype == 0 ){

		InternalScan3( pObj, buff );
		InternalScanMisc( pObj, buff );
	
	}else if( pObj->usinglibtype == 1 ){
	
		InternalScanMisc( pObj, buff );
	} 

}

//Different scan methord is implemented in following function
//it used related offset other than absolute offset

void ScanLib3( PStreamObject pObj, DWORD low, DWORD high, const BYTE* buff){

	WORD first = 0;
	PPeInfo pInfo = NULL;
	DWORD err = 0, tmp = 0;

	pInfo = (PPeInfo) &(pObj->data);


	if ( low == 0  ){
		
		pInfo->status = PE_STATUS_WAITING_MZ;
	}
	
	// logically it should be put in the end of this function
	// but added here  only for accelaration
	if ( pInfo->status == PE_STATUS_PE_SECTION_READY ){
			
		InternalScanlib( pObj, buff );
		//InternalScan5( pObj, buff );
		return;		
	}
	
	
	if ( pInfo->status == PE_STATUS_WAITING_MZ ){
		
		//this branch is supposed that the first four bytes can be always gotten
		//so that i do not care about the situation that less than four bytes
		//arrive in the first time
		//that means that following GetOffsetValue function never fail. 
		
		first = (WORD)GetOffsetValue2( pObj, 0, buff, &err );
		
		if ( first == (WORD)0x5a4d){// MZ head
			
			pInfo->status = PE_STATUS_MZ_RECEIVED;
			
		}else{// added by zhangyuan 2005-04-06
			
			pObj->viruslib2 = NULL;
			return;
		}
	}
	
	if (  
		(pInfo->status == PE_STATUS_MZ_RECEIVED)
		&&
		(pInfo->overlay.peoffset == 0 )	 
	){
		
		//pInfo->overlay.peoffset = GetOffsetValue( pObj, 0x3c, buff);
#if 1
		tmp = GetOffsetValue2( pObj, 0xc, buff, &err);
		
		if ( tmp == 0x4550l ){

			pInfo->overlay.pebegin = 0xcl;
			pInfo->status = PE_STATUS_PE_RECEIVED;

			pInfo->PHtype = PE_HEAD_COMPRESSED;

			goto PE_RECEIVED;
		
		}else{
		
			if ( err == ERR_DATA_NOT_READY ){
				return;
			}
			if ( err == ERR_DATA_PASSED ){
			
			}  
		}

#endif
		
		//changed by zhangyuan 2005-04-01 to accelerate
		if ( pInfo->overlay.peoffset = GetOffsetValue2( pObj, 0x3c, buff, &err) ){			
#if 0
			if ( pInfo->overlay.peoffset < 64){
				
				pObj->viruslib2 = NULL;
				pObj->virusfound = UNKNOWN_VIRUS;
				return;			
			}
#endif		
		}else{

			if ( err == ERR_NO_ERR ){
				
				pObj->viruslib2 = NULL;
			}
			return;		
		}
		
	}
	
	if (
		(pInfo->status == PE_STATUS_MZ_RECEIVED)
		&&
		(pInfo->overlay.peoffset )
	){

		tmp = GetOffsetValue2( pObj, pInfo->overlay.peoffset, buff, &err );
		
		if ( tmp == 0x4550l ){// PE head
			
			pInfo->overlay.pebegin ;
			
			pInfo->status = PE_STATUS_PE_RECEIVED;

			pInfo->PHtype = PE_HEAD_NORMAL;
			
			//get sectionstable offset;		
		}else{

			if( err == ERR_DATA_NOT_READY ){

				return ;
				
			}else{
				pObj->viruslib2 = NULL;
				return ;
			}
		}		
		
	}

PE_RECEIVED:
	
	if ( pInfo->status == PE_STATUS_PE_RECEIVED ){ //now collecting PE infomation begins
		
		//be careful about the sequence of PE info	
		//get fileheader->Marchine
		
		//get fileheader->NumberOfSections
		if( pInfo->sectionsnum == 0 ){
			
			tmp = GetOffsetValue2( pObj, (pInfo->overlay.pebegin + 4), buff, &err );
			
			if ( tmp ){

				tmp = tmp>>16;

				if ( tmp ) {

					pInfo->sectionsnum = tmp;
					
				}else{
					pObj->viruslib2 = NULL;
					return;
				}
				
			}else{

				if ( err == ERR_DATA_NOT_READY ){

					return ;
				}else{
					pObj->viruslib2 = NULL;
					return;		
				}		
			}			
		}

		if ( pInfo->sectionsnum && (pInfo->SizeOfOptionalHeader == 0) ){

			tmp = GetOffsetValue2( pObj, (pInfo->overlay.pebegin + 20), buff, &err );

			if ( tmp ){

				tmp = tmp&0xffffl;

				if ( tmp ){

					pInfo->SizeOfOptionalHeader = tmp;
					pInfo->substatus = PE_SUBSTATUS_WAITING_AddressOfEntryPoint;

				}else{

					pObj->viruslib2 = NULL;
					return;		
				}
			
			}else{

				if ( err == ERR_DATA_NOT_READY ){

					return ;
				}else{
					pObj->viruslib2 = NULL;
					return;		
				}		
			}		
		}
		
		//get optionalheader->SizeOfCode
		
		//because some Pe codesize is zero, this condition may block scan
		//so i delete some code processing this condition

		
		//get optionalheader->AddressOfEntryPoint
		
		if (  pInfo->substatus == PE_SUBSTATUS_WAITING_AddressOfEntryPoint ){

			tmp = GetOffsetValue2( pObj, (pInfo->overlay.pebegin + 0x28), buff, &err );		

			if( tmp ){
				
				pInfo->entryoffset = tmp;

				pInfo->substatus = PE_SUBSTATUS_WAITING_SizeOfImage;

			}else{
				
				if ( err == ERR_DATA_NOT_READY ){

					return ;

				}else{

					if ( err == ERR_NO_ERR ){

						pInfo->entryoffset = 0;
						pInfo->substatus = PE_SUBSTATUS_WAITING_SizeOfImage;
					
					}else{

						pObj->viruslib2 = NULL;
						return;
					}
				}
			}
							
		}
		
		if ( pInfo->substatus == PE_SUBSTATUS_WAITING_SizeOfImage ){

			// more needed here
			// to collect section info is needed
			// Get SizeOfImage 0x50  SizeOfHeaders 0x54

			tmp = GetOffsetValue2( pObj, (pInfo->overlay.pebegin + 0x50), buff, &err );		

			if( err == 0 ){

				pInfo->SizeOfImage = tmp;

				pInfo->status = PE_STATUS_PE_INFO_READY;			
				pInfo->substatus = PE_SUBSTATUS_WAITING_VIRTUAL_ADDRESS;
							
				//adjust offset to begin of the sections table
				pInfo->overlay.sectionoffset = pInfo->overlay.pebegin + 0x18l+ pInfo->SizeOfOptionalHeader;

			}else{
				
				if ( err == ERR_DATA_NOT_READY ){

					return ;

				}else{

					pObj->viruslib2 = NULL;
					return;		
				}
			}
/*			
			pInfo->status = PE_STATUS_PE_INFO_READY;			
			pInfo->substatus = PE_SUBSTATUS_WAITING_VIRTUAL_ADDRESS;
						
			//adjust offset to begin of the sections table
			pInfo->overlay.sectionoffset = pInfo->overlay.pebegin + 0xf8;
*/
		}
		
	}//( pInfo->status == PE_STATUS_PE_RECEIVED )
		
	
	if ( pInfo->status == PE_STATUS_PE_INFO_READY ){
		
		do{
			
			if ( pInfo->currentsection > pInfo->sectionsnum ){
				// this is an error status;
				// some PE file may be broken
				
				pObj->viruslib2 = NULL;
				return;
			}
			
			//Section->VirtualAddress
			if( pInfo->substatus == PE_SUBSTATUS_WAITING_VIRTUAL_ADDRESS ){

				tmp = GetOffsetValue2( pObj, (pInfo->overlay.sectionoffset + 0xc), buff, &err );

				if ( err == 0 ){
					
					pInfo->VirtualAddress = tmp;
					pInfo->substatus = PE_SUBSTATUS_WAITING_SizeOfRawData;			
					// added by zhangyuan 2005-04-11
					
				}else{
					
					//some process may be added here later
					if( err == ERR_DATA_PASSED ){

						pObj->viruslib2 = NULL; return;
					
					}else if( err == ERR_DATA_NOT_READY ){
						
						return;
					}
				}
					
			}
			
			//Section->SizeOfRawData
			if( 
				(pInfo->substatus == PE_SUBSTATUS_WAITING_SizeOfRawData)
				&& 
				(pInfo->SizeOfRawData == 0) 
			){
				
				tmp = GetOffsetValue2( pObj, (pInfo->overlay.sectionoffset + 0x10), buff, &err );
				
				if ( err == 0 ){

					if ( tmp ){
						
						pInfo->SizeOfRawData = tmp;
						
						pInfo->substatus = PE_SUBSTATUS_CAN_COMPARE;
						
					}else{
						//because there is a situation where SizeOfRawData is zero
						//special process is needed here
						
						// process next item of sections table
						
						pInfo->currentsection ++;
						pInfo->overlay.sectionoffset = pInfo->overlay.sectionoffset + 0x28;
						// reset value;
						pInfo->VirtualAddress = 0;
						pInfo->SizeOfRawData = 0;
						
						pInfo->substatus = PE_SUBSTATUS_WAITING_VIRTUAL_ADDRESS;
						
					}
				}else{

					if( err == ERR_DATA_PASSED ){

						pObj->viruslib2 = NULL; return;
					
					}else if( err == ERR_DATA_NOT_READY ){
						
						return;
					}		
				}
				
			}
			
			//Section->PointerToRawData
			if( pInfo->substatus == PE_SUBSTATUS_CAN_COMPARE ){

				if ( pInfo->SizeOfRawData && (pInfo->currentsection == 0)&&(pInfo->entryoffset < pInfo->VirtualAddress) ){

					if ( pInfo->entryoffset >= 0x200l ){
						
						tmp = GetOffsetValue2( pObj, (pInfo->overlay.sectionoffset + 0x14), buff, &err );
						
						if ( tmp ){
								
							// find section where entrypoint exists within 
							pInfo->RawData = tmp;
							pInfo->status = PE_STATUS_PE_SECTION_READY;
								
							// some more process may be needed here
							//
							//TO DO:

							if( tmp < low ){

								pObj->viruslib2 = NULL;
								pObj->virusfound = 55555;
								return ;	
							}								

							//ShowPeInfo( pInfo );								
							break;
								
						}else{
							// current need not to be scaned 
							// maybe PE file is broken 
							//pObj->viruslib2 = NULL;
							break;
						}					
					}
				}
			
				if( pInfo->SizeOfRawData && (pInfo->currentsection == 1)&&(pInfo->entryoffset < 0x200l) ){

					tmp = GetOffsetValue2( pObj, (pInfo->overlay.sectionoffset + 0x14), buff, &err );
					
					if ( tmp ){
							
						// find section where entrypoint exists within 
						pInfo->RawData = tmp;
						pInfo->status = PE_STATUS_PE_SECTION_READY;
							
						// some more process may be needed here
						//
						//TO DO:

						if( tmp < low ){

							pObj->viruslib2 = NULL;
							pObj->virusfound = 55555;
							return ;	
						}								

						//ShowPeInfo( pInfo );								
						break;
							
					}else{
						// current need not to be scaned 
						// maybe PE file is broken 
						//pObj->viruslib2 = NULL;
						break;
					}

				}				

				
				if (  ( pInfo->entryoffset >= pInfo->VirtualAddress )
					&&
					( pInfo->entryoffset < (pInfo->VirtualAddress + pInfo->SizeOfRawData) )	//delete = 2006 05 19	
				){
	
					tmp = GetOffsetValue2( pObj, (pInfo->overlay.sectionoffset + 0x14), buff, &err );
						
					if ( tmp ){
							
						// find section where entrypoint exists within 
						pInfo->RawData = tmp;
						pInfo->status = PE_STATUS_PE_SECTION_READY;
							
						// some more process may be needed here
						//
						//TO DO:

						if( tmp < low ){

							pObj->viruslib2 = NULL;
							pObj->virusfound = 55555;
							return ;	
						}								

						//ShowPeInfo( pInfo );								
						break;
							
					}else{
						// current need not to be scaned 
						// maybe PE file is broken 
						//pObj->viruslib2 = NULL;
						break;
					}		
					
				}else{
					
					// Scan next item of sections table
					pInfo->currentsection ++;
					pInfo->overlay.sectionoffset = pInfo->overlay.sectionoffset + 0x28;
					// reset value;
					pInfo->VirtualAddress = 0;
					pInfo->SizeOfRawData = 0;
					
					//added by zhangyuan 2005-04-11
					pInfo->substatus = PE_SUBSTATUS_WAITING_VIRTUAL_ADDRESS;
				}		
			}
			
	}while( pInfo->status != PE_STATUS_PE_SECTION_READY );

	
		}//( pInfo->status == PE_STATUS_PE_INFO_READY )
		
		
		if ( pInfo->status == PE_STATUS_PE_SECTION_READY ){
			
			//compare value of each feature here
			//InternalScan( pObj, buff );
			
			//InternalScan2( pObj, buff );
			
			InternalScanlib( pObj, buff );
			//InternalScan5( pObj, buff );
		}		
		
}

void VirusScan( PStreamObject pObj, const BYTE* pBuff ){

	PTree lib = pObj->viruslib2;

	if ( lib ){

		//added 2005-03-08
		ScanLib3( pObj, pObj->currentstart, pObj->nextstart, pBuff );

	}
}

LONG GeneralDataScan( PStreamObject pObj, const BYTE* pbDataBuff, DWORD Length, DWORD *aRet ){
	
	PreProcess( pObj, pbDataBuff, Length);

	if (OBJSTATUS_WAITTING_DATA == pObj->status){

		pObj->status = OBJSTATUS_SCANVIRUS_PENDING;
	}

	if (OBJSTATUS_SCANVIRUS_PENDING == pObj->status){
		
		VirusScan( pObj, pbDataBuff );

		if ( pObj->virusfound ){/* NOT COMPLETED */

			pObj->status = OBJSTATUS_SCANVIRUS_PENDING;

			if ( gReport ){
				
				gReport( pObj->virusfound, gReportArg );
			}

			*aRet = pObj->virusindex ;

			return pObj->virusfound;//911
			
		}else if ( pObj->viruslib2 ){

			goto end;

		}else if( pObj->viruslib2==NULL ){

			return 0;// no virus found
		}
	}

end:
	PostProcess(pObj, pbDataBuff, Length);
	return -1;
}

#if 0
void PatternPrint( const PCHAR patt, DWORD p){

	DWORD i;

	if ( ( patt != NULL )&&( p != 0 ) ){
		
		for ( i = 0; i < p ; i++ ){
			
			printf("%c", patt[i]);
		}
		printf( "\n");	
	}
}

PCHAR PatternMarch( const PCHAR patt, DWORD p, const BYTE* des, DWORD d, const PCHAR back, DWORD b ){

	DWORD i, j;
	
	if ( 
		( patt == NULL )||( p ==0 )||( des== NULL )||( d ==0 ) 
	){
	
		printf( "PARAMETER ERROR\n");
		return NULL;
	}
	if ( d < p ) return NULL;

	if ( ( back == NULL )&&( b == 0 ) ){

		for( i =0; i < (d-p); i++){

			for ( j =0; j < p; j ++){

				if( des[i+j] != patt[j] )break;
			}
			
			if ( j == p){
			// have found pattern
				//PatternPrint( patt, p );
				return (PCHAR) ( &(des[i]) );
			}
		}
	}

	if ( ( back != NULL)&&( b != 0 ) ){
	
	}
//	printf( "NOT FIND\n");
	return NULL;
}

void B64C4TO3( PUCHAR Buff, PUCHAR pRet ){

	UCHAR out[3];
	DWORD i;

	for( i =0; i <4 ; i++ ){
		
		if ( (Buff[i]>= 'A')&&( Buff[i]<= 'Z') ){

			Buff[i] = (UCHAR)( Buff[i] - 'A' );

		}else if ( (Buff[i]>= 'a')&&(Buff[i]<='z')){

			Buff[i] = (UCHAR)( Buff[i] - 'a' +26 );
		
		}else if ( (Buff[i]>= '0')&&(Buff[i]<='9')){

			Buff[i] = (UCHAR)( Buff[i] - '0' +52 );

		}else if ( Buff[i]== '+' ){
			
			Buff[i]= 62;
		
		}else if ( Buff[i]=='/' ){
		
			Buff[i]= 63;
		}else{
		
			//printf( "can not decode base64 :%d\n", Buff[i]);
		}

	}

	out[0] = (UCHAR)( ((Buff[0]<<2) &0xfc)|( (Buff[1] >>4)&0x3) );
	out[1] = (UCHAR)( ((Buff[1]<<4) &0xf0)|( (Buff[2] >>2)&0xf) );
	out[2] = (UCHAR)( ((Buff[2]<<6) &0xc0)|( Buff[3]&0x3f) );

	memcpy( pRet, out, 3 );

}

DWORD Decodeb64( PStreamObject pObj, const BYTE* pbDataBuff, DWORD Length, PCHAR ret){

	UCHAR Buff[4];
	DWORD size = 0, count = 0, decode = 0;
	PUCHAR p1 = NULL, p2 = Buff;
	PUCHAR pRet = (PUCHAR) ret;

	if ( pObj->b64.count ){

		DWORD i;

		p1 = (PUCHAR) &(pObj->b64.buff);

		for( i =0; i < pObj->b64.count; i++ ){

			if ( (*p1 != 0xa)&&(*p1 != 0xd) ){

				*p2 = *p1; size++;
				p2++; p1++;	
			
			}else{

				p1++;
			}
		}

		pObj->b64.count = 0;
	}

	p1 = (PUCHAR) pbDataBuff;

	while( ( Length >= count)&&((size +Length- count) >=4) ){

		if ( (*p1 ==0xa)||(*p1 ==0xd) ){
		
			p1++; count ++;
			
			continue;
		}

		// put buff
		if( size < 4 ){

			*p2 = *p1;	p2++; p1++;
			size++; count++;
			
			continue;
		}

		// decode base64
		if( size == 4 ){

			B64C4TO3( Buff, pRet );

			pRet +=3; decode +=3;
			size = 0;p2 = Buff;
		}

	}

	if( (size ==0)&&( (Length- count)< 4 ) ){

		DWORD i = Length - count;

		pObj->b64.count = i;

		if ( i >0 ){

			memcpy( pObj->b64.buff, &(pbDataBuff[count]), i);		
		}	
	}

	return decode;
}

DWORD GeneralDataScan2( PStreamObject pObj, const BYTE* pbDataBuff, DWORD Length){
	
	PCHAR pTmp = EngineMalloc( Length);

	DWORD res = (DWORD)-1;
	DWORD len = 0;
	DWORD ret = 0;
	
	if( pTmp ){

		len = Decodeb64( pObj, pbDataBuff, Length, pTmp );

		res = GeneralDataScan( pObj, pTmp, len, &ret );

		EngineMfree( pTmp);
	}
	// need change
	return res;
}

CHAR att[10] = {'a','t','t','a','c','h','m','e','n','t'};
CHAR TV[2] = {'T','V'};

#define OTHER_FIND_ATT     99  //find("attachment")	
#define OTHER_FIND_TV      88  //find("TV");

DWORD MailDataScan( PStreamObject pObj, const BYTE* pbDataBuff, DWORD Length){

	PCHAR found = NULL;// TV OR ATTACHMENT founed position in buffer
	DWORD foundsize = 0;


	if ( OBJSTATUS_WAITTING_DATA == pObj->status ){

		pObj->status = OBJSTATUS_FINDING_REALDATA;
		pObj->other = OTHER_FIND_ATT;
	}

realfind:

	if ( OBJSTATUS_FINDING_REALDATA == pObj->status ){

		if ( OTHER_FIND_ATT == pObj->other ){

			//may need back
			found = PatternMarch( att, 10, pbDataBuff, Length, NULL, 0 );

			if ( !found ) { goto final; }//return -1;

			pObj->other = OTHER_FIND_TV;

			foundsize = 10;
			//printf("%x\n", (DWORD) found );

		}

		if ( OTHER_FIND_TV == pObj->other ){

			if ( found ){
				//must not back

				found = PatternMarch(

					TV , 2, 
					(found+foundsize), (Length - ( found - pbDataBuff + foundsize )),
					NULL, 0
				);
				
			}else{
				// may need back
				found = PatternMarch( TV, 2, pbDataBuff, Length, NULL, 0 );
			}

			if ( !found ) { goto final; }//return -1; //need more data

			foundsize = 2;
			//printf("%x\n", (DWORD) found );
			// real begin point is found

			pObj->status = OBJSTATUS_SCANVIRUS_PENDING;

			/*add new code here*/
		}		
	}

	if ( OBJSTATUS_SCANVIRUS_PENDING == pObj->status ){

		LONG scanresult =0;
		
		if (found){

			ResetStreamObj( pObj );
			printf("attachment scan\n");
			scanresult = GeneralDataScan2( pObj,found, ( Length -(found - pbDataBuff) ));

		}else{
			scanresult = GeneralDataScan2( pObj,pbDataBuff, Length );
		
		}

#if 1
		if ( scanresult == -1 ) goto final;

		pObj->status = OBJSTATUS_FINDING_REALDATA;
		pObj->other = OTHER_FIND_ATT;

		if ( scanresult == 0 ){

			printf( "Attachment : No Virus\n");
		}
		
		if ( scanresult > 0 ){

			printf( "Attachment has virus id: %d\n", scanresult );

			pObj->virusfound = 0;
		}	

#endif

		if ( found ){

			found = PatternMarch( att, 10, //  //changed 2004-10-28  TV,2 is old
				(found+foundsize), (Length - ( found - pbDataBuff + foundsize )),
				NULL, 0
			);

		}else{

			found = PatternMarch( att, 10, pbDataBuff, Length, NULL, 0 );
		}

		if ( found ){/*{ goto final; }*///return -1;}// need more data

			pObj->status = OBJSTATUS_FINDING_REALDATA;
			pObj->other = OTHER_FIND_TV;
			goto realfind;

		}

		return scanresult;
	}

final:
	

	return (DWORD)-1;
}
#endif

#define INVALID_PARAMETER  7
#define INVALID_CONTEXT    8

LONG RSDataScan(DWORD Context, const BYTE* pbDataBuff, DWORD Length, DWORD *aRet )
{
	PStreamObject pObj = NULL;

	if ( ( pbDataBuff == NULL )||( Length == 0 ) ||(aRet ==NULL) )return (DWORD)-INVALID_PARAMETER;

	pObj = EngineFindObj( Context);

	if ( pObj == NULL) return (DWORD)-INVALID_CONTEXT;

	switch ( pObj->protocol ){

	case PROTOCOL_MAIL:
		
		return 0; //MailDataScan( pObj, pbDataBuff, Length );
		
	default:	
		return GeneralDataScan( pObj, pbDataBuff, Length, aRet );
	}

}



/* new request add 2005-01-31 */
/* Data sent to engine need relocated */
/* new code add here */