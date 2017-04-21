
#include	"predef.h"
#include	"cache.h"

HANDLE hIndexFile;

PIndexEntry NewIndexEntry(){

	return ( PIndexEntry) NULL;
}

PIndexEntry GetIndexEntryAnsi( STRING *pStr ){

	return ( PIndexEntry) NULL;
}

PIndexEntry GetIndexEntryUnc( UNICODE_STRING *pUnc ){

	return ( PIndexEntry) NULL;
}

NTSTATUS RemoveIndexEntry(){

	return STATUS_SUCCESS;
}

NTSTATUS DeleteAllIndexEntry(){

	return STATUS_SUCCESS;
}

NTSTATUS SaveIndexFile(){

	return STATUS_SUCCESS;
}

NTSTATUS LoadIndexFile(){

	
	
	return STATUS_SUCCESS;
}

NTSTATUS CacheInit(){

	UNICODE_STRING IndexFileName;
	UNICODE_STRING DataFileName;
	NTSTATUS status;
	HANDLE hHandle;
	HANDLE hData;
	IO_STATUS_BLOCK IoBlk;
	OBJECT_ATTRIBUTES attrib;

	RtlInitUnicodeString( &IndexFileName, L"\\??\\E:\\supervise\\index.dat" );
	RtlInitUnicodeString( &DataFileName, L"\\??\\E:\\supervise\\data.dat" );

	InitializeObjectAttributes( &attrib, &IndexFileName, OBJ_INHERIT, 0 ,NULL);
	status = ZwOpenFile(		
		&hHandle, 
		FILE_READ_DATA,
		&attrib,
		&IoBlk,
		FILE_SHARE_READ,
		FILE_NON_DIRECTORY_FILE
	);
	
	if ( !NT_SUCCESS(status) ){

		DbgPrint("Open Cache Error\n");
		return status;
	}

	InitializeObjectAttributes( &attrib, &DataFileName, OBJ_INHERIT, 0 ,NULL);
	status = ZwOpenFile(	
		&hData, 
		FILE_READ_DATA,
		&attrib,
		&IoBlk,
		FILE_SHARE_READ,
		FILE_NON_DIRECTORY_FILE
	);

	if ( !NT_SUCCESS(status) ){

		ZwClose( hHandle);
		DbgPrint("Open Cache Error\n");
		return status;
	}
/**
**	here construct cache in memory
**/

	ZwClose( hData);
	ZwClose( hHandle);
	return STATUS_SUCCESS;
	
}

