#include	"predef.h"
#include	"utility.h"


VOID ShowContext(){

	HANDLE pid,  tid;

	pid = PsGetCurrentProcessId();	
	tid = PsGetCurrentThread();

	DbgPrint( "CONTEXT : pid = %x ; tid = %x \n", pid, tid );

}
VOID ShowObjAttr( IN POBJECT_ATTRIBUTES pObjAttr ){

	if ( pObjAttr == NULL ) return ;

	pObjAttr->Length;
	pObjAttr->RootDirectory;
	pObjAttr->Attributes;
	pObjAttr->ObjectName;
	ShowUniC( pObjAttr->ObjectName);
	pObjAttr->SecurityDescriptor;
	pObjAttr->SecurityQualityOfService;

}

BOOLEAN noname( IN HANDLE hHandle ){
	
	NTSTATUS status;
	ULONG Returned;
	POBJECT_NAME_INFORMATION pInfo = NULL;
	
	UCHAR buffer[2048];
	
	status = ZwQueryObject(
		
		hHandle,
		ObjectNameInformation,
		buffer,
		2048,
		&Returned
	);

	if ( !NT_SUCCESS(status) ){
		
		return FALSE;
	}

	pInfo = (POBJECT_NAME_INFORMATION) buffer;
	ShowUniC( &(pInfo->Name) );

	return TRUE;
	
}

VOID ShowUniC( IN PUNICODE_STRING pSrc ){
	
	STRING strTmp;
	NTSTATUS status;

	if ( pSrc == NULL ) return;
	if ( pSrc->Length == 0 ) return;

	status = RtlUnicodeStringToAnsiString( &strTmp, pSrc, TRUE );

	if ( NT_SUCCESS( status) ){

		DbgPrint( "%s\n",strTmp.Buffer );

		RtlFreeAnsiString( &strTmp);
	}

}

VOID ShowFileObj( PFILE_OBJECT pFileObj ){

	if ( pFileObj->Type == (CSHORT)5 ){

		DbgPrint( "Type = %x ;\n", pFileObj->Type );
		//DbgPrint( "Size = %x ;\n", pFileObj->Size );
		//DbgPrint( "DeviceObject = %x ;\n", pFileObj->DeviceObject );
		//DbgPrint( "Vpb = %x ;\n", pFileObj->Vpb );
		//DbgPrint( "FsContext = %x ;\n", pFileObj->FsContext );
		//DbgPrint( "FsContext2 = %x ;\n", pFileObj->FsContext2 );

		ShowUniC( &( pFileObj->FileName) );
	}

}

VOID ShowHandleInfo( HANDLE hHandle ){
	
	FILE_OBJECT *pObj = NULL ;
	NTSTATUS status ;
	
	status = ObReferenceObjectByHandle(
		
		hHandle, 
		FILE_READ_DATA , 
		NULL, 
		KernelMode, 
		&pObj,
		NULL
	);
	
	if ( NT_SUCCESS( status ) ){

		DbgPrint( "reference ++\n" );

		ShowFileObj( pObj);

		ObDereferenceObject( pObj);
	}

}

VOID FillWithNop( PVOID des, ULONG size ){

	UCHAR* pChar =( UCHAR*)des;
	ULONG i ;

	if ( size == 0 || des == NULL) return;

	for ( i = 0; i < size; i++ ){

		*pChar =0x90;	pChar++;
	}

}

/**
**	this function used to set offset value of instrument  such as jmp
**	
**	pChar : pointer to the lowest byte of offset 
**
**	ulValue : offset value to be set
**
**	ulCount : the size of  offset in byte
**/

VOID FillOffset( UCHAR* pChar, ULONG ulValue, ULONG ulCount ){

	UCHAR* pTmp = ( UCHAR*)&ulValue;
	ULONG i;

	if ( pChar == NULL ) return ;

	for ( i = 0; i < ulCount; i++ ){

		*pChar = *pTmp;

		pChar++;	pTmp++;
	}

}

VOID Display( UCHAR* puc, int count ){

	int  i ;
	UCHAR* pChar = puc;

#define TIMER_SAVE

#ifdef 	TIMER_SAVE

	HANDLE hFile;
	UCHAR ucBuffer[1024];
	IO_STATUS_BLOCK blk;
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING name;
		
#endif 

	DbgPrint( "DISPLAY START: 0x%x\n", puc );

	for ( i = 0; i < count ; i++, pChar++ ){
		
#ifdef	TIMER_SAVE

		ucBuffer[i] = *pChar ;
#else

		DbgPrint( "@ %d = %x\n", i , *pChar );
#endif 

	}

#ifdef	TIMER_SAVE

	RtlInitUnicodeString( &name, L"\\??\\E:\\test\\zhang.bin" );

	InitializeObjectAttributes( 
		
		&ObjectAttributes, 
		&name, 
		OBJ_KERNEL_HANDLE , 
		NULL,
		NULL 
	);

	status = ZwCreateFile(
		
		&hFile,
		FILE_APPEND_DATA,
		&ObjectAttributes,
		&blk,
		0, 
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_WRITE,
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,     
		0 
	);

	if ( NT_SUCCESS( status) ){

		DbgPrint( "@Create success@\n");

		status = ZwWriteFile(
			
			hFile, 
			NULL,
			NULL,
			NULL,
			&blk,
			ucBuffer,
			( ULONG)count,
			0,
			NULL
		);

		ZwClose( hFile);

	}else{

		DbgPrint( " error status = %x\n", status );
	}

#endif

}
