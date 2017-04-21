
#include	"predef.h"
#include	"proc.h"


VOID BeforExecute(
	IN PUNICODE_STRING  FullImageName,
	IN HANDLE  ProcessId, // where image is mapped
	IN PIMAGE_INFO  ImageInfo
){

	PUNICODE_STRING name = FullImageName;
	STRING ascllname;

	DbgPrint( "length = 0x%x ; maxlength = 0x%x\n", 
				name->Length, name->MaximumLength );

	RtlUnicodeStringToAnsiString( &ascllname, name, TRUE );

	DbgPrint( "%s \n", ascllname.Buffer );
	
}


VOID BeforProcessCreate(
	IN HANDLE  ParentId,
	IN HANDLE  ProcessId,
	IN BOOLEAN  Create
){
	PVOID pObj = NULL;
	NTSTATUS status ;

	status = ObReferenceObjectByHandle(
		
		ProcessId,
		FILE_READ_DATA,
		NULL,
		KernelMode,
		&pObj,
		NULL
	);
	
	if ( NT_SUCCESS( status) ){
		
		DbgPrint( "obj = %x\n", pObj );
	}else{

		DbgPrint( "error : status = %x\n", status );
	}

}

VOID suProcess(){

	NTSTATUS status = 0L;
	
	status = PsSetCreateProcessNotifyRoutine( BeforProcessCreate, 0 );

	if ( !NT_SUCCESS( status) ){
		
		DbgPrint( "error for register\n");
	}
	
}

