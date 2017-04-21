
#include	"predef.h"
#include	"openclose.h"


typedef struct _A{

	int a;
}A;

struct B{
	A;
};

VOID ShowOffset();
VOID Showoffset2();

NTSTATUS CreateOpen(	IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp ){

	DbgPrint( "Device Opened\n" );
	
	//ShowIRP( Irp );
	ShowCurrentStack( Irp );
			
	return  STATUS_SUCCESS ;
}

NTSTATUS Close( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp ){
	
	DbgPrint( "Device Closed\n" );
	
	Showoffset2();
	ShowOffset();
	return STATUS_SUCCESS ;
}

VOID ShowIRP( IN PIRP Irp ){

	DbgPrint("ApcEnvironment = %x ; StackCount = %x ; UserBuffer = %x ; Flags = %x ;\n",

	Irp->ApcEnvironment , Irp->StackCount , Irp->UserBuffer, Irp->Flags

	);

}
VOID ShowCurrentStack( IN PIRP Irp ){

	PIO_STACK_LOCATION pIrpStack = NULL ;
	PFILE_OBJECT pFileObj = NULL ;

	pIrpStack = IoGetCurrentIrpStackLocation(Irp) ;
	DbgPrint( "pIrpStack = %x ;\n", pIrpStack );

	pFileObj = pIrpStack->FileObject ;
	DbgPrint( "pFileObj = %x ;\n", pFileObj );
	
	DbgPrint("offset = %x\n", (ULONG)(&(pFileObj->FileName) )-(ULONG)pFileObj );
	//ShowFileObj( pFileObj);

}


VOID ShowOffset(){

	DbgPrint( "Type : 0x%x\n", &(((PIRP)0)->Type));
	DbgPrint( "Size : 0x%x\n", &(((PIRP)0)->Size));

	DbgPrint( "MdlAddress : 0x%x\n", &(((PIRP)0)->MdlAddress));
	DbgPrint( "Flags : 0x%x\n", &(((PIRP)0)->Flags));
	DbgPrint( "AssociatedIrp : 0x%x\n", &(((PIRP)0)->AssociatedIrp));

	DbgPrint( "ThreadListEntry : 0x%x\n", &(((PIRP)0)->ThreadListEntry));
	DbgPrint( "IoStatus : 0x%x\n", &(((PIRP)0)->IoStatus));
	DbgPrint( "RequestorMode : 0x%x\n", &(((PIRP)0)->RequestorMode));
	DbgPrint( "PendingReturned : 0x%x\n", &(((PIRP)0)->PendingReturned));
	DbgPrint( "StackCount : 0x%x\n", &(((PIRP)0)->StackCount));
	DbgPrint( "CurrentLocation : 0x%x\n", &(((PIRP)0)->CurrentLocation));

	DbgPrint( "Cancel : 0x%x\n", &(((PIRP)0)->Cancel));
	DbgPrint( "CancelIrql : 0x%x\n", &(((PIRP)0)->CancelIrql));
	DbgPrint( "ApcEnvironment : 0x%x\n", &(((PIRP)0)->ApcEnvironment));
	DbgPrint( "AllocationFlags : 0x%x\n", &(((PIRP)0)->AllocationFlags));
	DbgPrint( "UserIosb : 0x%x\n", &(((PIRP)0)->UserIosb));

	DbgPrint( "UserEvent : 0x%x\n", &(((PIRP)0)->UserEvent));
	
	DbgPrint( "Overlay : 0x%x\n", &(((PIRP)0)->Overlay));
	//
	DbgPrint( "CancelRoutine : 0x%x\n", &(((PIRP)0)->CancelRoutine));
	DbgPrint( "UserBuffer : 0x%x\n", &(((PIRP)0)->UserBuffer));
	
	DbgPrint( "Tail : 0x%x\n", &(((PIRP)0)->Tail));
	//
	DbgPrint( "Tail.Overlay.ListEntry : 0x%x\n", &(((PIRP)0)->Tail.Overlay.ListEntry));
	DbgPrint( "Tail.Overlay.CurrentStackLocation : 0x%x\n", &(((PIRP)0)->Tail.Overlay.CurrentStackLocation));
	DbgPrint( "Tail.Overlay.OriginalFileObject : 0x%x\n", &(((PIRP)0)->Tail.Overlay.OriginalFileObject));
	

}

VOID Showoffset2(){
	
	DbgPrint("MajorFunction : 0x%x\n", &(((PIO_STACK_LOCATION)0)->MajorFunction));
	DbgPrint("MinorFunction : 0x%x\n", &(((PIO_STACK_LOCATION)0)->MinorFunction));
	DbgPrint("Flags : 0x%x\n", &(((PIO_STACK_LOCATION)0)->Flags));
	DbgPrint("Control : 0x%x\n", &(((PIO_STACK_LOCATION)0)->Control));
	
	DbgPrint("Parameters : 0x%x\n", &(((PIO_STACK_LOCATION)0)->Parameters));
	//DbgPrint("Parameters.DeviceIoControl : 0x%x\n", &(((PIO_STACK_LOCATION)0)->Parameters.DeviceIoControl));
	
	DbgPrint("Parameters.DeviceIoControl.OutputBufferLength : 0x%x\n", &(((PIO_STACK_LOCATION)0)->Parameters.DeviceIoControl.OutputBufferLength));
	DbgPrint("Parameters.DeviceIoControl.InputBufferLength : 0x%x\n", &(((PIO_STACK_LOCATION)0)->Parameters.DeviceIoControl.InputBufferLength));
	DbgPrint("Parameters.DeviceIoControl.IoControlCode : 0x%x\n", &(((PIO_STACK_LOCATION)0)->Parameters.DeviceIoControl.IoControlCode));
	DbgPrint("Parameters.DeviceIoControl.Type3InputBuffer : 0x%x\n", &(((PIO_STACK_LOCATION)0)->Parameters.DeviceIoControl.Type3InputBuffer));

	DbgPrint("DeviceObject : 0x%x\n", &(((PIO_STACK_LOCATION)0)->DeviceObject));
	DbgPrint("FileObject : 0x%x\n", &(((PIO_STACK_LOCATION)0)->FileObject));
	DbgPrint("CompletionRoutine : 0x%x\n", &(((PIO_STACK_LOCATION)0)->CompletionRoutine));
	DbgPrint("Context : 0x%x\n", &(((PIO_STACK_LOCATION)0)->Context));

}

