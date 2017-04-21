#include "predef.h"
#include "supobj.h"


static LIST_ENTRY  HeadUsing;
static ULONG CountUsing;

static LIST_ENTRY  HeadFree;
static ULONG CountFree;

LIST_ENTRY HeadScan;
static ULONG CountScan;

static LIST_ENTRY HeadScanning;
static ULONG CountScanning;

KMUTANT MutexAlloc;
KMUTANT MutexUsing;
KMUTANT MutexFree;

KMUTANT MutexScan;
KMUTANT MutexScanning;

HANDLE hNotifyEvent = (HANDLE)NULL;

MGRSTATICS mgrInfo = { 0L, 0L };

#define mgrLock()
#define mgrUnlock()


VOID mgrStatics(){

	DbgPrint( "ulAlloc =%x; ulFree =%x\n",
		mgrInfo.ulAlloc,
		mgrInfo.ulFree
	);

	DbgPrint( "CountUsing = %x; CountFree = %x\n",
		CountUsing,
		CountFree
	);

}


/**
**
**		COUNT 100
**__________________
**	R ->	...
**	E ->		...
**	S ->	...
**	O ->	...
**	U ->	...
**	R ->	...
**	C ->	...
**	E ->		...
**	S ->	...
**	I  ->	...
**	Z ->	...
**	E ->		...
**__________________
**	32
**
**/


#define	COL_COUNT  32
#define	LIN_COUNT	100

LONG ResourceIndex = -1;

ULONG NumAllocated = 0;

PVOID Resource[ COL_COUNT ];

static BOOLEAN mgrAllocRow(){

	PVOID pMm = NULL;

	if ( ResourceIndex < ( COL_COUNT -1 ) ){
	
		pMm = ExAllocatePool( NonPagedPool , sizeof( MGROBJ) *LIN_COUNT );

		if ( pMm == NULL ) {
			
			DbgPrint( "Allocate memory errror\n");
			
			return FALSE;

		}else{

			ResourceIndex ++;
			Resource[ ResourceIndex] = pMm;
			NumAllocated = 0;
			
			return TRUE;
		}
		
	}else{
	
		DbgPrint( "Reach Max Size\n");
		return FALSE;
	}

}

static BOOLEAN mgrFirstAlloc(){

	ULONG i;

	for ( i = 0; i < COL_COUNT; i++ ){

		Resource[i] = NULL;
	}
	
	ResourceIndex = -1;

	return mgrAllocRow();

}

VOID mgrInit(){

	ULONG i;
	
	if ( !mgrFirstAlloc() ){
		
		DbgPrint( "supervise fata error\n");
	}

	CountUsing = 0L;
	InitializeListHead( &HeadUsing);

	CountFree = 0L;
	InitializeListHead( &HeadFree);

	CountScan = 0L;
	InitializeListHead( &HeadScan);

	CountScanning = 0L;
	InitializeListHead( &HeadScanning);
	

	KeInitializeMutex( &MutexAlloc, MUTEX_LEVEL_STREAMS_SUBSYS );
	KeInitializeMutex( &MutexUsing, MUTEX_LEVEL_STREAMS_SUBSYS );
	KeInitializeMutex( &MutexFree, MUTEX_LEVEL_STREAMS_SUBSYS );
	
	KeInitializeMutex( &MutexScan, MUTEX_LEVEL_STREAMS_SUBSYS );
	KeInitializeMutex( &MutexScanning, MUTEX_LEVEL_STREAMS_SUBSYS );
	
}

VOID mgrFreeRes(){

	ULONG i;

	for ( i = 0; i < COL_COUNT; i++ ){

		if ( Resource[i] ){

			ExFreePool( Resource[i] );
		}
	}
	
}

static VOID mgrInitObj( IN PMGROBJ pMgrObj, IN HANDLE hHandle ){

	if ( pMgrObj == NULL ) return ;

	pMgrObj->hFile = hHandle;
	pMgrObj->ulType;
	pMgrObj->ulRefer = 0L;
	
	pMgrObj->ulReadCount = 0L;
	pMgrObj->ulWriteCount = 0L;
	
	pMgrObj->pOperation = NULL;

	RtlFillBytes( &(pMgrObj->uncFileName), sizeof(UNICODE_STRING), (UCHAR)0 );

	pMgrObj->bScaned = FALSE;
	pMgrObj->EventWait = (HANDLE)NULL;

}

static VOID mgrAddToUsingList( IN PMGROBJ pObj ){
	
	PLIST_ENTRY pEntry = NULL;
	NTSTATUS status;

	if ( pObj == NULL ) return;

	pEntry = ( PLIST_ENTRY)&(pObj->ListItem);

	mgrLock();
	
	status = KeWaitForMutexObject(

		&MutexUsing,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS( status) ){
	
		InsertHeadList( &HeadUsing, pEntry );
		InterlockedIncrement( &CountUsing);
		
		KeReleaseMutex( &MutexUsing, FALSE );
		
	}else{

		DbgPrint( "FATAL ERROR :1\n");
	}
	
	mgrUnlock();

}

static VOID mgrRemvFromUsingList( IN PMGROBJ pObj ){

	PLIST_ENTRY pEntry = NULL;
	NTSTATUS status;

	if ( pObj == NULL ) return ;
	
	pEntry = &(pObj->ListItem);

	mgrLock();

	status = KeWaitForMutexObject(

		&MutexUsing,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS( status) ){
	
		RemoveEntryList( pEntry);
		InterlockedDecrement( &CountUsing);
		
/**
**	added in 2004 06 10
**/
		mgrInitObj( pObj, (HANDLE)0 );

		KeReleaseMutex( &MutexUsing, FALSE );
		
	}else{

		DbgPrint( "FATAL ERROR :2\n");
	}
	
	mgrUnlock();

}

static VOID mgrAddToFreeList( IN PMGROBJ pObj ){

	PLIST_ENTRY pEntry = NULL;
	NTSTATUS status;

	if ( pObj == NULL ) return;
	
	pEntry = ( PLIST_ENTRY)&(pObj->ListItem);

	mgrLock();
	
	status = KeWaitForMutexObject(

		&MutexFree,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS( status) ){
	
		InsertHeadList( &HeadFree, pEntry );
		InterlockedIncrement( &CountFree);

		KeReleaseMutex( &MutexFree, FALSE );
		
	}else{
		
		DbgPrint( "FATAL ERROR :3\n");
	}
	
	mgrUnlock();
	
}

static VOID mgrAddSearchTree( HANDLE hHandle, PMGROBJ pMgrObj ){

}

/**
**	mgrAllocObj  first scan freelist , if get an obj then return pointer to the object
**	if there is no object in free list  then  get  a block of nopage memory from 
**	system . and construct an object , and return
**
**/
static PMGROBJ mgrAllocObj(){

	PLIST_ENTRY pEntry = NULL;
	PMGROBJ pObj = NULL;
	NTSTATUS status;

	status = KeWaitForMutexObject(

		&MutexFree,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS( status) ){

		if ( IsListEmpty( &HeadFree) ){
			
			KeReleaseMutex( &MutexFree, FALSE );
			
			status = KeWaitForMutexObject(

				&MutexAlloc,
				Executive,
				KernelMode,
				FALSE,
				NULL
			);

			if ( NT_SUCCESS( status) ){

				if ( NumAllocated >= LIN_COUNT ){
					
					if ( !mgrAllocRow() ) {
						
						KeReleaseMutex( &MutexAlloc, FALSE );
						
						return NULL;
					}	
				}
				
				pObj = ( PMGROBJ)Resource[ResourceIndex] + NumAllocated ;

				NumAllocated++;
				
				KeReleaseMutex( &MutexAlloc, FALSE );

				return pObj;
				
			}else{
			
				DbgPrint( "FATAL ERROR :6\n");
				
				return NULL;
			}
				
		}else{

			pEntry = RemoveHeadList( &HeadFree );
			
			InterlockedDecrement( &CountFree);
			
			KeReleaseMutex( &MutexFree, FALSE );
			
			pObj =  CONTAINING_RECORD( pEntry, MGROBJ, ListItem );

			return pObj;
		}	
		
	}else{
		
		DbgPrint( "FATAL ERROR :5\n");
		return NULL;
	}
	
}

PMGROBJ mgrGetObj( 
	
	HANDLE hHandle, 
	POBJECT_ATTRIBUTES pObjectAttributes, 
	BOOLEAN * pbFirstCreated 
){

	PMGROBJ pTmp = NULL;

	if (	( hHandle == 0 )
		||
		( pObjectAttributes == NULL )
		||
		( pbFirstCreated ==NULL )
	)
		return NULL;


/**
**	look up whether  handle already exist
**	that is , there is an object  allocated
**	if object have existed ,increase reference count
**
**/

	pTmp = mgrGetMgrObjByHandle( hHandle);

	if ( pTmp ){

		DbgPrint( "Handle already exists\n");

		*pbFirstCreated = FALSE;
		InterlockedIncrement( &( pTmp->ulRefer) );
		return pTmp;	
	}

/**
**	constuct an new object
**/
	pTmp = mgrAllocObj();

	if ( pTmp ){

		mgrInitObj( pTmp, hHandle );
		*pbFirstCreated = TRUE;
		
		{
			UNICODE_STRING *pSrc =pObjectAttributes->ObjectName;
			UNICODE_STRING *pDes = &(pTmp->uncFileName);

			if( pObjectAttributes->RootDirectory != 0){
				
				DbgPrint( "RootDirectory = %x\n", 
					pObjectAttributes->RootDirectory
				);
				//noname( hHandle);
				/* added 2004 06 22 */
			}/* added 2004 06 18 */
			
			//noname( hHandle);
			/* added  2004 06 23 */
			if ( pSrc ){

				if ( pSrc->Length ){

					pDes->Length = pSrc->Length;
					pDes->MaximumLength = pDes->Length;
					pDes->Buffer = ExAllocatePool( NonPagedPool, pDes->Length);

					if ( pDes->Buffer ){

						RtlCopyUnicodeString( pDes, pSrc );
						ShowUniC( pSrc);
					}
				}
			}
			
			//noname( hHandle);
			/* added  2004 06 24 */
		}
		
		InterlockedIncrement( &( pTmp->ulRefer) );
		
		mgrAddToUsingList( pTmp);
		//mgrAddSearchTree( hHandle, pTmp);
		
		mgrInfo.ulAlloc ++;
		return pTmp;
	}

	return NULL;
}

VOID mgrFreeObj( PMGROBJ pMgrObj ){

	NTSTATUS status;

	mgrInfo.ulFree ++;
	
	if ( pMgrObj == NULL ) return;
	if ( pMgrObj->ulRefer == 0 ) return ;

	InterlockedDecrement( &( pMgrObj->ulRefer) );

	if ( pMgrObj->ulRefer == 0 ){

/**
**	the file will be closed indeed, so it is needed to scan
**
**/
		if ( pMgrObj->ulWriteCount ){

			status = mgrNotifyScan( pMgrObj);
		}
			
		if ( pMgrObj->uncFileName.Buffer ){
			
			ShowUniC( &( pMgrObj->uncFileName));
			ExFreePool( pMgrObj->uncFileName.Buffer);
			pMgrObj->uncFileName.Buffer = NULL;
		}
		
		mgrLock();
		mgrRemvFromUsingList( pMgrObj );
		mgrAddToFreeList( pMgrObj);
		mgrUnlock();
	}
	
}
/**
**	mgrGetMgrObjByHandle 
**
**	used to find obj by handle , if object indexed by handle 
**	do not exist, return NULL
**
**	here must use fast search methord to improve performance
**
**/

PMGROBJ mgrGetMgrObjByHandle( HANDLE hHandle ){

	PLIST_ENTRY pEntry = NULL;
	PMGROBJ pObj = NULL;
	NTSTATUS status;

	mgrLock();

	status = KeWaitForMutexObject(
		
		&MutexUsing,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS( status) ){
		
		if ( IsListEmpty( &HeadUsing) ){

			KeReleaseMutex( &MutexUsing, FALSE );
			mgrUnlock();
			
			return NULL;
		}

		pEntry = HeadUsing.Flink;

		while( pEntry != (&HeadUsing) ){

			pObj = CONTAINING_RECORD( pEntry, MGROBJ, ListItem );

			if ( pObj->hFile == hHandle ){

				KeReleaseMutex( &MutexUsing, FALSE );
				mgrUnlock();
				
				return pObj;
				
			}else{

				pEntry = pEntry->Flink;
			}
				
		}

		KeReleaseMutex( &MutexUsing, FALSE );
		mgrUnlock();
		
		return NULL;
		
	}else{

		DbgPrint( "FATAL ERROR :4\n");
	}
	
	mgrUnlock();
	return NULL;
	
}

VOID mgrShowObj( PMGROBJ pObj ){

	if ( pObj == NULL ){

		DbgPrint( "ERROR: pObj == NULL\n");
		return;
	}

	DbgPrint( "Handle =%x ; ReadCount = %x ; WriteCount = %x\n",

		pObj->hFile, pObj->ulReadCount, pObj->ulWriteCount
	);

}

VOID mgrOpRead( PMGROBJ pMgrObj ){

	NTSTATUS status;

	if ( pMgrObj == NULL ) return ;

	InterlockedIncrement( &( pMgrObj->ulReadCount) );

	if ( pMgrObj->ulReadCount == 1 ){
		
/**
**	first time reading an opened file , 
**	lookup cache history ,decide whether the file need to be scan
**	if need ,notify engine to scan
**
**/
//		status = mgrNotifyScan( pMgrObj );

	}

	if ( pMgrObj->pOperation == NULL ) return;

	if ( pMgrObj->pOperation->ObjRead == NULL ) return;

	(pMgrObj->pOperation->ObjRead)( pMgrObj);

}

VOID mgrOpWrite( PMGROBJ pMgrObj ){

	if ( pMgrObj == NULL ) return ;

	InterlockedIncrement( &( pMgrObj->ulWriteCount) );

	if ( pMgrObj->pOperation == NULL ) return;

	if ( pMgrObj->pOperation->ObjWrite == NULL ) return;

	(pMgrObj->pOperation->ObjWrite)( pMgrObj);
	
}

/**
**	CacheAndNotify used to communicate with cache subsystem 
**	and engine subsystem.
**
**	if the file have been scaned and not modified
**
**	it is not needed to notify engine
**	
**	if find virus, return 1, othervise return 0
**/

ULONG CacheAndNotify(){

	return 0;
}

/**
**	Scanlist is used to communicate with USERMODE Engine
**
**	Engine use DeviceIoControl() to get infomation.
**/

VOID mgrAddToScanList( PMGROBJ pObj ){
	
	PLIST_ENTRY pEntry = NULL;
	NTSTATUS status;
	
	if ( pObj == NULL ) return ;

	pEntry = &(pObj->ListScan);

	status = KeWaitForMutexObject(
				
		&MutexScan,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS(status) ){

		InterlockedIncrement( &CountScan);
		InsertHeadList( &HeadScan, pEntry);
		
		KeReleaseMutex( &MutexScan, FALSE );
		
	}

}

PMGROBJ mgrGetObjForScan(){

	PLIST_ENTRY pEntry = NULL;
	PMGROBJ pObj = NULL;
	NTSTATUS status;

	status = KeWaitForMutexObject(
				
		&MutexScan,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS(status) ){
		
	       if ( IsListEmpty( &HeadScan) ){

	       	KeReleaseMutex( &MutexScan, FALSE );
			return NULL;
	       }

	       pEntry = RemoveHeadList( &HeadScan );
		InterlockedDecrement( &CountScan );

		KeReleaseMutex( &MutexScan, FALSE );
		
		pObj = CONTAINING_RECORD( pEntry, MGROBJ, ListScan );

		return pObj;

	}
	
	return NULL;
}

VOID mgrPurgeScan(){

	PLIST_ENTRY pEntry = NULL;
	PMGROBJ pObj = NULL;
	NTSTATUS status;

	status = KeWaitForMutexObject(
				
		&MutexScan,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);
	
	if ( NT_SUCCESS(status) ){
		
	       while ( !IsListEmpty( &HeadScan) ){
	       	
		       pEntry = RemoveHeadList( &HeadScan );
			InterlockedDecrement( &CountScan );
			
	  		pObj = CONTAINING_RECORD( pEntry, MGROBJ, ListScan );
	  		
			pObj->bScaned = FALSE;
	  		KeSetEvent( pObj->EventWait, 0, FALSE);
	       }
	
		KeReleaseMutex( &MutexScan, FALSE );

	}

}

NTSTATUS mgrNotifyScan( PMGROBJ pObj ){

	NTSTATUS status = STATUS_SUCCESS;
	KEVENT Event;
		
	DbgPrint( "MGR : mgrNotifyScan\n");

	if ( pObj == NULL ) return STATUS_INVALID_PARAMETER; 
	
	if ( hNotifyEvent == (HANDLE)NULL ){

		return STATUS_INVALID_PARAMETER;
	}

	KeInitializeEvent( &Event, SynchronizationEvent, FALSE );
		
//	DbgPrint( "NtCreateEvent = %x\n", &Event);
	
	pObj->EventWait = &Event ;

	mgrAddToScanList( pObj );
	
//    resetevent
	KeSetEvent( hNotifyEvent, 0, FALSE );
	DbgPrint( "Wake up Ring 3 and Sleep\n");

//	KeWaitForSingleObject();

	status = KeWaitForSingleObject(

		&Event,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS(status) ){
		
		DbgPrint( "MGR : wake up\n");
	}
	
	pObj->EventWait = (ULONG)0L;

	return status;	

}

VOID mgrAddScanning( PMGROBJ pObj ){

	PLIST_ENTRY pEntry = NULL;
	NTSTATUS status = STATUS_SUCCESS;

	if ( pObj == NULL ) return ;

	pEntry = &( pObj->ListScan);

	status = KeWaitForMutexObject(
		
		&MutexScanning,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS(status) ){

		InsertHeadList( &HeadScanning, pEntry );
		InterlockedIncrement( &CountScanning );

		KeReleaseMutex( &MutexScanning, 0 );
	}

}

VOID mgrRemvScanning( PMGROBJ pObj ){

	PLIST_ENTRY pEntry = NULL;
	NTSTATUS status = STATUS_SUCCESS;

	if ( pObj == NULL ) return ;

	pEntry = &( pObj->ListScan);

	status = KeWaitForMutexObject(
		
		&MutexScanning,
		Executive,
		KernelMode,
		FALSE,
		NULL
	);

	if ( NT_SUCCESS(status) ){

		RemoveEntryList( pEntry );
		InterlockedDecrement( &CountScanning );

		KeReleaseMutex( &MutexScanning, 0 );
	}
	
}



