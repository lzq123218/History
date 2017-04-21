
#include	"predef.h"
#include	"ioincpt.h"

extern ULONG ulIoCreateCount;

BOOLEAN filterByName( PUNICODE_STRING pName){

	LONG Result, i;
	
#define FILTERNUM  7

	UNICODE_STRING name[FILTERNUM];

	if ( pName == NULL ) return FALSE;

	RtlInitUnicodeString( &( name[0] ), L"\\??\\e:\\supervise\\log.txt");
	RtlInitUnicodeString( &( name[1] ), L"\\??\\c:\\Drive information\\wkNtFsLdf.dat");
	RtlInitUnicodeString( &( name[2] ), L"\\??\\d:\\Drive information\\wkNtFsLdf.dat");
	RtlInitUnicodeString( &( name[3] ), L"\\??\\e:\\Drive information\\wkNtFsLdf.dat");
	RtlInitUnicodeString( &( name[4] ), L"\\??\\timer");
	RtlInitUnicodeString( &( name[5] ), L"\\??\\E:\\supervise\\index.dat");
	RtlInitUnicodeString( &( name[6] ), L"\\??\\E:\\supervise\\data.dat" );

	i = 0;
	
	do{

		Result = RtlCompareUnicodeString( pName, &( name[i] ), TRUE );

		if( Result == 0 ){
			
			return TRUE;
		}
		i++;

	}while( Result && ( i < FILTERNUM ) );
	
	return FALSE;
}

VOID suIoCreateFileEx(
	
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG Disposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength,
    IN CREATE_FILE_TYPE CreateFileType,
    IN PVOID ExtraCreateParameters OPTIONAL,
    IN ULONG Options
){

	_asm push eax;

	ulIoCreateCount++;
	
/**
**	in kernel mode , handle value 0 means  request unsuccessfully
**	
**	i do not care unsuccessful request
**/
	{
		PMGROBJ pObj = NULL;
		BOOLEAN FirstCreated;
	
		if ( *FileHandle ){

			if ( FILE_DIRECTORY_FILE & CreateOptions ){
#if 0
				DbgPrint("Open Directory : ");
				ShowUniC( ObjectAttributes->ObjectName);
#endif 					
			}else if ( FILE_OPEN_NO_RECALL & CreateOptions ){
#if 0
				DbgPrint("Open Pipe : ");
				ShowUniC( ObjectAttributes->ObjectName);
#endif 
			}else if ( filterByName( ObjectAttributes->ObjectName ) ){

				DbgPrint("File Filter : ");				
				ShowUniC( ObjectAttributes->ObjectName);
				
			}else if( FILE_NON_DIRECTORY_FILE & CreateOptions ){
			
				DbgPrint( "@suIoCreateFileEx : FileHandle = %x\n",				
					*FileHandle 
				);

				pObj = mgrGetObj( *FileHandle, ObjectAttributes, &FirstCreated );

				if ( pObj ){
					
					DbgPrint( "mgrGetObj = %x\n", pObj );

					if ( FirstCreated ){
	/**
	**	HERE : BROWSE ISOLATED FILES
	**/
					}
	#if 0				
					DbgPrint( "CreateOptions = %x; Options = %x\n",
						CreateOptions, Options
					);
	#endif				
				}
				
			}
			
		}
		
	}	

	_asm pop eax;
 
}

VOID ContainerBH(){

	_asm{
		
		add esp , 4*3;
		jmp suIoCreateFileEx;
		//ret 0x38;
		//	must use jmp,
		//	because eip have existed already		
	}	
}

VOID ContainerIOCreateFileEx(){

#define  NEWSTACKSIZE   ( 0x38 + 4 + 4*3)
	
	_asm{

		pushfd;
		push eax;
		push ecx;
	
		sub esp, NEWSTACKSIZE;
	
		mov ecx, NEWSTACKSIZE;
	des:
		mov al, [ esp +ecx + NEWSTACKSIZE -1];
		mov byte ptr [esp + ecx -1], al;
		loop des;

		mov ecx, 4;
		mov eax,  ContainerBH;		//	be careful here
	des1:
		rol eax, 8;		
		mov byte ptr[esp + 4*3 + ecx-1], al;
		loop des1;

		pop ecx;
		pop eax;
		popfd;
		
/**
**	instruments being changed
**/
		push ebp;
		mov ebp, esp;
		nop;
		nop;
/**
**	jmp near back to function being intercepted
**/
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
				
	}
}

VOID SysInterceptIoRoutineEx( PVOID des, PVOID container ){
	
	UCHAR* pChar = (UCHAR*)container;
	ULONG offset = 0L;
	ULONG i = 0L;

	if ( !des || !container) return;

	FillWithNop( container, 3);

/**
**	added  2004.05.23
**/

	pChar =( UCHAR*)container + 0x1d;
	*pChar = 0xb8;
	
/**
**	add push ff ; 0x6a ,0xff
**/
	pChar = (UCHAR*)container + 0x31;
	*pChar++ = 0x6a;
	*pChar = 0xff;

/**
**	add jmp near 
**/
	
	pChar = container;

	pChar += 0x33;
	*pChar =0xe9;	pChar++;
	
	offset = ( UCHAR*)des -( UCHAR*)container -0x33;
	FillOffset( pChar, offset, sizeof( ULONG) );
	
/**
**	change des instrument
**/
	
#if 1
	
	pChar = des;
	*pChar = 0xe9;	pChar++;

	offset = ( ULONG)container -( ULONG)des -5;
	FillOffset( pChar, offset, sizeof( ULONG) );
	
#endif 	
	
}

VOID SysRestoreEx(){

	UCHAR* pDes = ( UCHAR*)IoCreateFile;
	UCHAR* pChar = NULL;
	USHORT i ;
	UCHAR ucData[5] = { 
		
		0x55,					//	push ebp
		0x8b, 0xec,				//	mov ebp, esp
		0x6a, 0xff				//	push ff
	};

	for ( i = 0, pChar = ucData; i < 5 ; i++ ){

		*pDes = *pChar;
		
		pDes++;	pChar++;
	}

}

VOID IoIntercept(){

	FillWithNop( ( UCHAR*)ContainerBH, 6 );
	
	SysInterceptIoRoutineEx( 
		
		IoCreateFile, 
		(PVOID)ContainerIOCreateFileEx
	);

}

VOID IoRestore(){

	SysRestoreEx();

}
