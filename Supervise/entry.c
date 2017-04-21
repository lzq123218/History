

/**++
  *  author: timer chan 
  *  
  *  this driver is used to supervise file operations in Win2k 
  *
--**/

#include	"predef.h"
#include	"openclose.h"
#include	"ioctl.h"

#include	"ntincpt.h"
#include	"ioincpt.h"
#include	"zwincpt.h"


#define DEVICE_OBJECT_NAME_LENGTH 128

PDEVICE_OBJECT pDevice = NULL ;

HANDLE hSuperviseEvent ;
HANDLE hDir;

KSPIN_LOCK LockInstall;

extern ULONG ulOpenCount;
extern ULONG ulCloseCount;
extern ULONG ulIoCreateCount;


VOID Unload( IN PDRIVER_OBJECT DriverObject );

NTSTATUS MakeDir();

NTSTATUS DriverEntry(
	
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
){
           
	UNICODE_STRING DeviceName;
	UNICODE_STRING EventName;

	PDEVICE_OBJECT pNewDevObj = NULL;
	PKEVENT pEvent = NULL;
	HANDLE hEvent;
	
	NTSTATUS status = 0;
	KIRQL OldIrql;

	
	KeInitializeSpinLock( &LockInstall );

	mgrInit();
	
	//KeGetCurrentIrql();
	KeAcquireSpinLock( &LockInstall, &OldIrql );
	
	//ZwIntercept();	
	IoIntercept();
	NtIntercept();

	KeReleaseSpinLock( &LockInstall, OldIrql );

	//KeStallExecutionProcessor( 0xefffffff );
	
	RtlInitUnicodeString( &DeviceName, L"\\DosDevices\\timer" );
		
	status = IoCreateDevice( 
		
		DriverObject, 
		sizeof(long),
		&DeviceName,
		FILE_DEVICE_FILE_SYSTEM, 
		FILE_DEVICE_SECURE_OPEN, 
		TRUE, 
		& pNewDevObj 
	);

	if ( !NT_SUCCESS(status) ){

		return status ;
	}
	
#if 0
	RtlInitUnicodeString( &EventName, L"\\SuperviseEvent" );
	
	pEvent = IoCreateNotificationEvent( &EventName, &hEvent );

	if ( pEvent == NULL ){

		//IoDeleteDevice( pNewDevObj);

		return STATUS_ACCESS_DENIED;
	}
	
	DbgPrint( "EventHandle = %x\n", hEvent);
	
	hSuperviseEvent = hEvent;
#endif

 	pDevice = pNewDevObj;
 	
	DriverObject->DriverUnload                          = Unload;
	DriverObject->MajorFunction[IRP_MJ_CREATE]          = CreateOpen;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]           = Close;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = DevIoctl;

#if 0

//   DriverObject->DriverExtension->AddDevice            = AddDevice;

//   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = Flush;
	DriverObject->MajorFunction[IRP_MJ_WRITE]           = Write;
	DriverObject->MajorFunction[IRP_MJ_READ]            = Read;
	
//   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
//      = InternalIoControl;

//   DriverObject->MajorFunction[IRP_MJ_CLEANUP]         = Cleanup;
//   DriverObject->MajorFunction[IRP_MJ_PNP]             = PnpDispatch;
//   DriverObject->MajorFunction[IRP_MJ_POWER]           = PowerDispatch;

//   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]
//      = QueryInformationFile;
//   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]
//      = SetInformationFile;

//   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]
//      = SystemControlDispatch;

#endif 

	return STATUS_SUCCESS ;

}

VOID Unload( IN PDRIVER_OBJECT DriverObject ){

	//ZwRestore();

	IoRestore();
	NtRestore();
	
	mgrFreeRes();
	mgrStatics();

	DbgPrint( "Result OpenCount =%x\n", ulOpenCount );
	DbgPrint( "Result CloseCount =%x\n", ulCloseCount );
	DbgPrint( "Result IOCreateCount =%x\n", ulIoCreateCount );
	
	if ( pDevice ){

		IoDeleteDevice( pDevice );	
	}

	if ( hSuperviseEvent != (HANDLE) -1 ){

		ZwClose( hSuperviseEvent);
	}
	
	if ( hDir != (HANDLE) -1 ){

		ZwClose( hDir);
	}

	if ( hNotifyEvent ){

		ObDereferenceObject( hNotifyEvent );
		hNotifyEvent = NULL;
	}

}

NTSTATUS MakeDir(){

	OBJECT_ATTRIBUTES ObjAtt ;
	UNICODE_STRING DirName ;
	NTSTATUS status ;

	RtlInitUnicodeString( &DirName, L"\\casey" );

	InitializeObjectAttributes(
		
		&ObjAtt,
		&DirName,
		OBJ_INHERIT,
		NULL,
		NULL 
	);

	status = ZwCreateDirectoryObject(
		
		&hDir,
		DIRECTORY_ALL_ACCESS,
		&ObjAtt
	); 

	switch ( status ){

	case STATUS_SUCCESS:
		
		DbgPrint("hDir =%x\n", hDir );
		break;
		
	case STATUS_ACCESS_DENIED:
		
		DbgPrint( "STATUS_ACCESS_DENIED\n" );
		break;
		
	case STATUS_ACCESS_VIOLATION:

		DbgPrint( "STATUS_ACCESS_VIOLATION\n" );
		break;
		
	case STATUS_DATATYPE_MISALIGNMENT:
		
		DbgPrint( "STATUS_DATATYPE_MISALIGNMENT\n" );
		break;

	default: 
		DbgPrint("error unknown\n");
	
	}

 	return status;
 	
}



