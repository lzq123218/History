#ifndef	TIMER_IOCTL

#define	TIMER_IOCTL


#define NOTIFYDRIVERSTART CTL_CODE(\
FILE_DEVICE_NULL, 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define NOTIFYDRIVERSTOP CTL_CODE(\
FILE_DEVICE_NULL, 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define GETINFOFROMDRIVER	 CTL_CODE(\
FILE_DEVICE_NULL, 0x08, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define RETURNRESULTTODRIVER CTL_CODE(\
FILE_DEVICE_NULL, 0x09, METHOD_BUFFERED, FILE_ANY_ACCESS )




typedef struct _EventHandle{

	HANDLE hEventHandle;
	
} EventHandle, *PEventHandle;

typedef struct _EvNotifyInfo{

	HANDLE hFile;
	PVOID  pObjAddr;
	
/**
**	bFormation : 
**			TRUE  name formation in UNICODE
**
**			FALSE name formation in ANSICODE
**
**/

	BOOLEAN  bFormation;
	
	UNICODE_STRING uniName;
	STRING ansiName;

} EvNotifyInfo, *PEvNotifyInfo;

typedef struct _ScanResult{

	HANDLE hFile;
	PVOID pObjAddr;

	BOOLEAN bAlarm;
	ULONG ulLevel;

} ScanResult, *PScanResult;


NTSTATUS DevIoctl(	IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );

NTSTATUS NotifyDriverStart( IN PIRP Irp, IN PIO_STACK_LOCATION pIrpStack );

NTSTATUS NotifyDriverStop( IN PIRP Irp, IN PIO_STACK_LOCATION pIrpStack );

NTSTATUS GetScanInfo( IN PIRP Irp, IN PIO_STACK_LOCATION pIrpStack );

NTSTATUS ScanComplete( IN PIRP Irp, IN PIO_STACK_LOCATION pIrpStack );

#endif 
