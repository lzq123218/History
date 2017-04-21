
#include "predef.h"
#include "ioctl.h"

#include "cache.h"

extern LIST_ENTRY HeadScan;

NTSTATUS DevIoctl(	IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp ){

	PIO_STACK_LOCATION pIrpStack = NULL ;
	NTSTATUS status = STATUS_SUCCESS;

	if ( !DeviceObject || !Irp ) return STATUS_INVALID_PARAMETER;

	pIrpStack = IoGetCurrentIrpStackLocation(Irp);

	if ( pIrpStack == NULL ) return STATUS_INVALID_PARAMETER;

	switch( pIrpStack->Parameters.DeviceIoControl.IoControlCode ){

		case NOTIFYDRIVERSTART:
			CacheInit();
			status = NotifyDriverStart( Irp, pIrpStack );
			break;

		case NOTIFYDRIVERSTOP:
			status = NotifyDriverStop( Irp, pIrpStack );
			break;
			
		case GETINFOFROMDRIVER:
			
			status = GetScanInfo( Irp, pIrpStack );
			break;
		case RETURNRESULTTODRIVER:
			
			status = ScanComplete(Irp, pIrpStack );
			break;
		default:
			;
	}
	
	Irp->IoStatus.Status = status;
	IoCompleteRequest( Irp, 0 );

	return status;
	
}

NTSTATUS NotifyDriverStart( IN PIRP Irp, IN PIO_STACK_LOCATION pIrpStack ){
			
	EventHandle param;
	EventHandle *pInput;
	
	PVOID pObj = NULL;
	NTSTATUS status;

	DbgPrint( "NotifyDriverStart\n");
	
	if ( !Irp || !pIrpStack ) return STATUS_INVALID_PARAMETER;

	if ( pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(EventHandle) ){
			
		return STATUS_BUFFER_TOO_SMALL;	
	}

	pInput = (EventHandle*)(Irp->AssociatedIrp.SystemBuffer);
	param = *pInput;
	
	if ( param.hEventHandle == 0 ) return STATUS_INVALID_PARAMETER;
	
	//DbgPrint( "param.hEventHandle =%x\n", param.hEventHandle);

	status = ObReferenceObjectByHandle(
		
		param.hEventHandle,
		0,
		NULL,
		KernelMode,
		&pObj,
		NULL
	);

	if ( !NT_SUCCESS( status) ){

		return status;
	}

	hNotifyEvent = (HANDLE)pObj;
		
	return STATUS_SUCCESS;
	
}

NTSTATUS NotifyDriverStop( IN PIRP Irp, IN PIO_STACK_LOCATION pIrpStack ){

	DbgPrint( "NotifyDriverStop\n");
	
	if ( !Irp || !pIrpStack ) return STATUS_INVALID_PARAMETER;

	if ( hNotifyEvent != NULL){
		
		ObDereferenceObject( hNotifyEvent );
		
		hNotifyEvent = NULL;
	}
	
	mgrPurgeScan();

	return STATUS_SUCCESS;
}

NTSTATUS GetScanInfo( IN PIRP Irp, IN PIO_STACK_LOCATION pIrpStack ){
	
	EvNotifyInfo info;
	PMGROBJ pObj = NULL;
	
	ULONG ulBufferLength;
	PVOID pInput;

	DbgPrint( "GetScanInfo\n");

	if ( !Irp || !pIrpStack ) return STATUS_INVALID_PARAMETER;

	ulBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

//	DbgPrint( "BufferLength = %x\n", ulBufferLength );

	if ( ulBufferLength < sizeof(EvNotifyInfo) ) return STATUS_BUFFER_TOO_SMALL;

	pInput = (PVOID)(Irp->AssociatedIrp.SystemBuffer);

	pObj = mgrGetObjForScan();

	DbgPrint( "pObj = %x\n", pObj );

	if ( pObj == NULL ){
/**
**	 there is no file that need being scaned
**/
		if ( hNotifyEvent ){
			
			KeResetEvent( hNotifyEvent);
		}
		
		info.hFile = 0;
		info.pObjAddr = NULL;
		RtlZeroBytes( &(info.uniName), sizeof(UNICODE_STRING) );
		RtlZeroBytes( &(info.ansiName), sizeof(STRING) );
		
		RtlCopyBytes( pInput, &info, sizeof(EvNotifyInfo) );
		Irp->IoStatus.Information = sizeof(EvNotifyInfo);
		
		return STATUS_SUCCESS;
	}

	if ( pObj->uncFileName.Length + sizeof(EvNotifyInfo) > ulBufferLength ){
		
		DbgPrint( "STATUS_BUFFER_TOO_SMALL\n");
		return STATUS_BUFFER_TOO_SMALL;
	}

//	mgrAddScanning( pObj );

	info.hFile = pObj->hFile;
	info.pObjAddr = pObj;
	
	if ( 0 ){
/**
**	return UNICODE FORMATION
**/
		UCHAR *pChar = (PUCHAR)pInput +sizeof(EvNotifyInfo);
		RtlZeroBytes( &(info.ansiName), sizeof(STRING) );
#if 0
		RtlCopyBytes(
			pChar,
			pObj->uncFileName.Buffer,
			pObj->uncFileName.Length
		);
#endif 		

//memcopy;

		Irp->IoStatus.Information = sizeof(EvNotifyInfo) + info.uniName.Length;

	}else{

/**
**	return ANSICODE FORMATION
**/
//memcopy
		UCHAR *pChar = (PUCHAR)pInput +sizeof(EvNotifyInfo);
		STRING tmp;
		NTSTATUS status = 0;
		RtlZeroBytes( &(info.uniName), sizeof(UNICODE_STRING) );
		
		status = RtlUnicodeStringToAnsiString(
			&tmp,
			&(pObj->uncFileName),
			TRUE
		);

		if ( NT_SUCCESS(status) ){
			
			info.ansiName.Length = tmp.Length;
			info.ansiName.MaximumLength = tmp.MaximumLength;
			RtlCopyBytes( pChar, tmp.Buffer, tmp.Length);
			
			Irp->IoStatus.Information = sizeof(EvNotifyInfo)+tmp.Length;
			RtlFreeAnsiString(&tmp);
				
		}else{

			Irp->IoStatus.Information = 0;
		}
		
	}
	
	RtlCopyBytes( pInput, &info, sizeof(EvNotifyInfo) );

	return STATUS_SUCCESS;
	
}

NTSTATUS ScanComplete( IN PIRP Irp, IN PIO_STACK_LOCATION pIrpStack ){

	PMGROBJ pObj = NULL;
	ScanResult Result;
	
	ULONG ulBufferLength;
	PVOID pInput;

	DbgPrint( "ScanComplete\n");
	
	if ( !Irp || !pIrpStack ) return STATUS_INVALID_PARAMETER;

	ulBufferLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;

	if ( ulBufferLength < sizeof(ScanResult) ){

		DbgPrint( "STATUS_BUFFER_TOO_SMALL\n");
		return STATUS_BUFFER_TOO_SMALL;
	}

	pInput = Irp->AssociatedIrp.SystemBuffer;

	Result = *( (PScanResult)pInput );

	pObj = Result.pObjAddr;
	
	if ( !MmIsAddressValid(pObj) ){
		
		DbgPrint( "PARAMETER: ERROR\n");
		return STATUS_INVALID_PARAMETER;
	}

	if ( (pObj->hFile != Result.hFile)
		|| 
		( Result.hFile == (HANDLE)0)
	){
		
		DbgPrint( "PARAMETER: ERROR\n");
		return STATUS_INVALID_PARAMETER;
	}

//	DbgPrint( "pObj = %x\n", pObj );
	DbgPrint( "pObj->hFile = %x\n", pObj->hFile );
	
//	mgrRemvScanning( pObj);
	pObj->bScaned = TRUE;
	KeSetEvent( pObj->EventWait, 0, FALSE );

	if ( hNotifyEvent ){
		
		KeResetEvent( hNotifyEvent);

		if ( !IsListEmpty( &HeadScan) ){

			KeSetEvent(hNotifyEvent, 0, FALSE);
		}
		
	}
	return STATUS_SUCCESS;
	
}


