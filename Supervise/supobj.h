
#ifndef TIMER_SUPOBJ

#define TIMER_SUPOBJ


NTSTATUS NtCreateEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSYSAPI NTSTATUS NTAPI
ZwWaitForSingleObject(
	IN HANDLE Handle,
	IN BOOLEAN Alertable,
	IN PLARGE_INTEGER Timeout OPTIONAL
);

NTSYSAPI NTSTATUS NTAPI
ZwSetEvent(
	IN HANDLE EventHandle,
	OUT PULONG PreviousState OPTIONAL
);

typedef struct _MGROBJ  *PMGROBJ;

typedef struct _MGROBJOPERATION{

/**
**	ulType : used to indentify file type such as PE and other data filetype
**/

	ULONG ulType;

/**
**	different file care different  file operations,
**	as an abstract object 
**	all operations that may be used musht list bellow
**	parameters of operations varys ,according to its implement
**
**/
	
	VOID ( *ObjRead)( IN PMGROBJ pObj );
	
	VOID ( *ObjWrite)( IN PMGROBJ pObj );
	
	VOID ( *ObjStart)( IN PMGROBJ pObj );
	
	VOID ( *ObjClose)( IN PMGROBJ pObj );
	
	VOID ( *ObjOther)( IN PMGROBJ pObj );

} MGROBJOPERATION, *PMGROBJOPERATION;

typedef struct _MGROBJ{

	HANDLE hFile;
	HANDLE hPID;
	
	LIST_ENTRY ListItem;
	
	ULONG ulAlertLevl;
	ULONG ulReadCount; 
	ULONG ulWriteCount;
	
	ULONG ulRefer;
	ULONG ulType;
	PMGROBJOPERATION pOperation;
	UNICODE_STRING uncFileName;

	PRKEVENT EventWait;
	LIST_ENTRY ListScan;
	BOOLEAN bScaned;

	LARGE_INTEGER StartTime;
	LARGE_INTEGER CloseTime;
	
	LARGE_INTEGER InQueue;
	LARGE_INTEGER OutQueue;

	LARGE_INTEGER BeginScan;
	LARGE_INTEGER EndScan;
	
} MGROBJ, *PMGROBJ;

typedef struct _MGRSTATICS{

	ULONG ulAlloc;
	ULONG ulFree;
	
} MGRSTATICS;


extern HANDLE hNotifyEvent;

PMGROBJ mgrGetObj(
	
	HANDLE hHandle , 
	POBJECT_ATTRIBUTES pObjectAttributes,
	BOOLEAN * pbFirstCreated 
);
VOID mgrFreeObj( PMGROBJ pMgrObj );

PMGROBJ mgrGetMgrObjByHandle( HANDLE hHandle );

VOID mgrInit();
VOID mgrFreeRes();

VOID mgrStatics();

VOID mgrShowObj( PMGROBJ pObj );

VOID mgrOpRead( PMGROBJ pMgrObj );

VOID mgrOpWrite( PMGROBJ pMgrObj );

ULONG CacheAndNotify();

NTSTATUS mgrNotifyScan( PMGROBJ pObj );

PMGROBJ mgrGetObjForScan();
VOID mgrPurgeScan();

VOID mgrAddScanning( PMGROBJ pObj );

VOID mgrRemvScanning( PMGROBJ pObj );

#endif

