#include "ntddk.h"


#define ZHANG_ATT


PDRIVER_OBJECT gDriver = NULL;

PDEVICE_OBJECT gSelfControl= NULL;

PDEVICE_OBJECT gFilter = NULL;


NTSTATUS
ZAttachVol(PDRIVER_OBJECT aDriver);

VOID
ZDettachVol( PDEVICE_OBJECT aDevice );

NTSTATUS
ZCreate( IN PDEVICE_OBJECT aDeviceObject, IN PIRP Irp );

NTSTATUS
ZClose( IN PDEVICE_OBJECT aDeviceObject, IN PIRP Irp );

NTSTATUS
ZCleanup( IN PDEVICE_OBJECT aDeviceObject, IN PIRP Irp );

NTSTATUS
ZRead( IN PDEVICE_OBJECT aDeviceObject, IN PIRP Irp );

VOID
DriverUnload( IN PDRIVER_OBJECT DriverObject ){

#ifdef ZHANG_ATT

	if( gFilter ){

		ZDettachVol( gFilter);
		gFilter = NULL;
	}
	
#endif

	IoDeleteDevice( gSelfControl );
	
	gSelfControl = NULL;

	gDriver = NULL;

}

NTSTATUS
DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath ){


	UNICODE_STRING functionName;
	UNICODE_STRING nameString;
	
	ULONG lAddress = 0;
	NTSTATUS lStatus;


	RtlInitUnicodeString( &functionName, L"NtCreateFile" );

	lAddress = (ULONG)MmGetSystemRoutineAddress( &functionName );

	DbgPrint("address %x\n", lAddress );

	gDriver = DriverObject;

	RtlInitUnicodeString( &nameString, L"\\Device\\ZHANG" );

	lStatus = IoCreateDevice(
		
			DriverObject,
			0,                      //has no device extension
			&nameString,
			FILE_DEVICE_DISK_FILE_SYSTEM,
			FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&gSelfControl
	);

	if (!NT_SUCCESS( lStatus )) {
           
            return lStatus;
	}

	DriverObject->DriverUnload = DriverUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = ZCreate;	
	DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = NULL;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = ZCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = ZClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = ZRead;
	
#ifdef ZHANG_ATT

	lStatus = ZAttachVol( gDriver );

	if (!NT_SUCCESS( lStatus )) {

		IoDeleteDevice( gSelfControl );		
		return lStatus;
	}

#endif

	return STATUS_SUCCESS;	
}


NTSTATUS
ZAttachVol( PDRIVER_OBJECT aDriver ){

	UNICODE_STRING lVolName;
	NTSTATUS lStatus = 0;
	PFILE_OBJECT lFileObj = NULL;
	
	PDEVICE_OBJECT lVolDevice = NULL;
	PDEVICE_OBJECT lNew = NULL;
	PDEVICE_OBJECT lTop = NULL;
	

	RtlInitUnicodeString( &lVolName, L"\\DosDevices\\D:" );

	lStatus= IoGetDeviceObjectPointer( &lVolName, FILE_READ_ATTRIBUTES, &lFileObj, &lVolDevice );

	if(!NT_SUCCESS( lStatus ) ){

		DbgPrint( "status error = %x\n", lStatus );
		return lStatus;
		
	}else{
	
		ObDereferenceObject( lFileObj );
		lFileObj = NULL;

	}

	lStatus = IoCreateDevice(
		
			aDriver ,
			sizeof(PDEVICE_OBJECT),
			NULL, FILE_DEVICE_DISK,
			0,
			FALSE,
			&lNew
	);

	if(!NT_SUCCESS( lStatus ) ){

		DbgPrint( " can not create device\n" );
		DbgPrint( "status error = %x\n", lStatus );
		
		return lStatus;
	}

	lTop = IoAttachDeviceToDeviceStack( lNew, lVolDevice );

	if( lTop == NULL ){

		DbgPrint( "can not attach\n" );
		DbgPrint( "status error = %x\n", lStatus );
		
		IoDeleteDevice( lNew );
		
		return STATUS_NO_SUCH_DEVICE;
	}

	{	

		*( (PDEVICE_OBJECT*)(lNew->DeviceExtension) ) = lTop;

		//DbgPrint( "device = %x\n", lTop );
		
		gFilter = lNew;
		
	}
	
	return STATUS_SUCCESS;
	
}

VOID
ZDettachVol( PDEVICE_OBJECT aDevice ){

	PDEVICE_OBJECT *pTmp = aDevice->DeviceExtension;

	PDEVICE_OBJECT lDevice = NULL;

	if( aDevice->DriverObject != gDriver ){ return; }

	if( pTmp ){
		
		lDevice = *pTmp; 
	}

	if( lDevice ){ IoDetachDevice(lDevice); }

	IoDeleteDevice( aDevice );


}

NTSTATUS
ZCreate( IN PDEVICE_OBJECT aDeviceObject, IN PIRP Irp ){

	PIO_STACK_LOCATION lIrpStk = NULL;
	PFILE_OBJECT lFileObj = NULL;

	if ( gSelfControl == aDeviceObject ){
	
		IoCompleteRequest( Irp, IO_NO_INCREMENT );

		return STATUS_SUCCESS;		
	}

	lIrpStk = IoGetCurrentIrpStackLocation( Irp );

	lFileObj = lIrpStk->FileObject;

	if ( lFileObj ){
		
		CHAR buff[512];
		
		STRING a;
		
		a.Length  = 0;
		a.MaximumLength = 512;
		a.Buffer = buff;

		RtlUnicodeStringToAnsiString( &a, &(lFileObj->FileName), FALSE );
		
		DbgPrint( "name length = %d (10)", lFileObj->FileName.Length );

		DbgPrint( "file name :%s\n", lFileObj->FileName.Buffer );

	}

	DbgPrint( "CreateFile\n" );

	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return STATUS_SUCCESS;

}

NTSTATUS
ZClose( IN PDEVICE_OBJECT aDeviceObject, IN PIRP Irp ){

	if ( gSelfControl == aDeviceObject ){
	
		IoCompleteRequest( Irp, IO_NO_INCREMENT );

		return STATUS_SUCCESS;		
	}

	DbgPrint( "CloseFile\n" );

	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return STATUS_SUCCESS;
}

NTSTATUS
ZCleanup( IN PDEVICE_OBJECT aDeviceObject, IN PIRP Irp ){

	PIO_STACK_LOCATION lIrpStk = NULL;
	PFILE_OBJECT lFileObj = NULL;

	if ( gSelfControl == aDeviceObject ){
	
		IoCompleteRequest( Irp, IO_NO_INCREMENT );

		return STATUS_SUCCESS;		
	}

		lIrpStk = IoGetCurrentIrpStackLocation( Irp );

	lFileObj = lIrpStk->FileObject;

	if ( lFileObj ){
		
		CHAR buff[512];
		
		STRING a;
		
		a.Length  = 0;
		a.MaximumLength = 512;
		a.Buffer = buff;

		RtlUnicodeStringToAnsiString( &a, &(lFileObj->FileName), FALSE );

		DbgPrint( "file name :%s\n", a.Buffer );

	}
	
	DbgPrint( "Cleanup\n" );

	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return STATUS_SUCCESS;
	
}

NTSTATUS
ZRead( IN PDEVICE_OBJECT aDeviceObject, IN PIRP Irp ){

	if ( gSelfControl == aDeviceObject ){
	
		IoCompleteRequest( Irp, IO_NO_INCREMENT );

		return STATUS_SUCCESS;		
	}

	DbgPrint( "ReadFile\n" );

	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return STATUS_SUCCESS;
}