
#include "predef.h"
#include "ntincpt.h"

extern ULONG ulCloseCount;

VOID SetContainer( PVOID des, PVOID container){

	UCHAR *pChar = NULL;
	ULONG offset = 0L;

	if ( !container ) {

		DbgPrint( "setConatainer: NULL\n");
		return ;
	}

	FillWithNop( container, 3);
	
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
	

}

VOID SysInterceptNtRoutine(
	
	PVOID des,
	PVOID container,
	PVOID containerBH
){

	UCHAR *pChar = NULL;
	ULONG offset = 0L;

	if ( !des || !container || !containerBH ) return ;

	SetContainer( des, container );

	FillWithNop( containerBH, 6 );

#if 1
	
	pChar = des;
	*pChar = 0xe9;	pChar++;

	offset = ( ULONG)container -( ULONG)des -5;
	FillOffset( pChar, offset, sizeof( ULONG) );
	
#endif 	

}

VOID SysRestoreNt( PVOID des ){

	UCHAR* pDes = NULL;
	UCHAR* pChar = NULL;
	USHORT i ;
	UCHAR ucData[5] = { 
		
		0x55,					//	push ebp
		0x8b, 0xec,				//	mov ebp, esp
		0x6a, 0xff				//	push ff
	};

	if ( !des ) return ;
	
	pDes = (UCHAR*)des;
	
	for ( i = 0, pChar = ucData; i < 5 ; i++ ){

		*pDes = *pChar;
		
		pDes++;	pChar++;
	}

}

VOID suNtReadFile(
	
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
){

	_asm push eax;
	
	{
		PMGROBJ pObj = NULL;
		
		//DbgPrint( "suNtReadFile\n");
		pObj = mgrGetMgrObjByHandle( FileHandle);

		if ( pObj ){
			
			mgrOpRead( pObj);	
		}
	}
	
	_asm pop eax;
	
}

VOID suNtWriteFile(
	
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
){

	_asm push eax;
	
	{
		PMGROBJ pObj = NULL;
		
		//DbgPrint( "suNtWriteFile\n");
		pObj = mgrGetMgrObjByHandle( FileHandle);

		if ( pObj ){
			
			mgrOpWrite( pObj);
		}
		
	}
	
	_asm pop eax;

}

VOID suNtDeviceIoControlFile(
	
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
){
	_asm push eax;
	
	{
		PMGROBJ pObj = NULL;
		
		//DbgPrint( "suNtDeviceIoControlFile\n");
		pObj = mgrGetMgrObjByHandle( FileHandle);

		if ( pObj ){
			
		}
		
	}
	
	_asm pop eax;
	
}

VOID suNtFlushBuffersFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
){
	_asm push eax;
	
	{
		
	}

	_asm pop eax;
}

VOID suNtClose(
	
	IN HANDLE hHandle
){	
	_asm push eax;

	ulCloseCount++;

	{
		PMGROBJ pObj = NULL;
		
		//DbgPrint( "NtClose = %x\n", hHandle );
		pObj = mgrGetMgrObjByHandle( hHandle);

		if ( pObj ){
			
			DbgPrint( "mgrGetMgrObjByHandle =%x\n", pObj );	
			mgrShowObj(  pObj);			
			
			DbgPrint( "mgrFreeObj = %x\n", pObj );
			mgrFreeObj( pObj);
		}
		
	}

	_asm pop eax;
	
}

VOID ContainerBHNtReadFile(){

	_asm{
		
		add esp , 4*3;
		jmp suNtReadFile;		
	}
	
}

VOID ContainerNtReadFile(){

#define  NTREADFILESTACKSIZE   ( 0x24 + 4 + 4*3)
	
	_asm{

		pushfd;
		push eax;
		push ecx;
	
		sub esp, NTREADFILESTACKSIZE;
	
		mov ecx, NTREADFILESTACKSIZE;
	des:
		mov al, [ esp +ecx + NTREADFILESTACKSIZE -1];
		mov byte ptr [esp + ecx -1], al;
		loop des;

		mov ecx, 4;
		mov eax,  ContainerBHNtReadFile;		//	be careful here
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

VOID ContainerBHNtWriteFile(){

	_asm{
		
		add esp , 4*3;
		jmp suNtWriteFile;		
	}
	
}

VOID ContainerNtWriteFile(){

#define  NTWRITEFILESTACKSIZE   ( 0x24 + 4 + 4*3)
	
	_asm{

		pushfd;
		push eax;
		push ecx;
	
		sub esp, NTWRITEFILESTACKSIZE;
	
		mov ecx, NTWRITEFILESTACKSIZE;
	des:
		mov al, [ esp +ecx + NTWRITEFILESTACKSIZE -1];
		mov byte ptr [esp + ecx -1], al;
		loop des;

		mov ecx, 4;
		mov eax,  ContainerBHNtWriteFile;		//	be careful here
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

VOID ContainerBHNtDeviceIoControlFile(){

	_asm{
		
		add esp , 4*3;
		jmp suNtDeviceIoControlFile;		
	}	
}

VOID ContainerNtDeviceIoControlFile(){

#define  NTDEVICEIOCONTROLFILESTACKSIZE   ( 0x28 + 4 + 4*3)
	
	_asm{

		pushfd;
		push eax;
		push ecx;
	
		sub esp, NTDEVICEIOCONTROLFILESTACKSIZE;
	
		mov ecx, NTDEVICEIOCONTROLFILESTACKSIZE;
	des:
		mov al, [ esp +ecx + NTDEVICEIOCONTROLFILESTACKSIZE -1];
		mov byte ptr [esp + ecx -1], al;
		loop des;

		mov ecx, 4;
		mov eax,  ContainerBHNtDeviceIoControlFile;	//	be careful here
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

VOID ContainerBHNtFlushBuffersFile(){

	_asm{
		
		add esp , 4*3;
		jmp suNtFlushBuffersFile;		
	}	
}

VOID ContainerNtFlushBuffersFile(){

#define  NTFLUSHBUFFERSFILESTACKSIZE   ( 0x8 + 4 + 4*3)
	
	_asm{

		pushfd;
		push eax;
		push ecx;
	
		sub esp, NTFLUSHBUFFERSFILESTACKSIZE;
	
		mov ecx, NTFLUSHBUFFERSFILESTACKSIZE;
	des:
		mov al, [ esp +ecx + NTFLUSHBUFFERSFILESTACKSIZE -1];
		mov byte ptr [esp + ecx -1], al;
		loop des;

		mov ecx, 4;
		mov eax,  ContainerBHNtFlushBuffersFile;	//	be careful here
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

/**
**	use intercepting methord differnet from others
**
**/

VOID ContainerNtClose(){

#define NTCLOSESTACKSIZE ( 1*4 )

	_asm{

		pushfd;
		pushad;

		sub esp, NTCLOSESTACKSIZE;
		mov ecx, NTCLOSESTACKSIZE;
	des:
		mov al, [ esp + 0x2c + ecx -1 ];
		mov byte ptr [ esp + ecx -1 ], al;
		loop des;

		call suNtClose;

		popad;
		popfd;

		push ebp;
		mov ebp, esp;
		
		nop;
		nop;

		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
	}
}

VOID InterceptNtClose( PVOID des, PVOID container ){

	UCHAR *pChar = NULL;
	ULONG offset = 0L;

	if ( !des || !container ) return;

	FillWithNop( container , 6 );

/**
**	add push ff ; 0x6a ,0xff
**/
	pChar = (UCHAR*)container + 0x24;
	*pChar++ = 0x6a;
	*pChar = 0xff;

/**
**	add jmp near 
**/
	
	pChar = container;

	pChar += 0x26;
	*pChar =0xe9;	pChar++;
	
	offset = ( UCHAR*)des -( UCHAR*)container -0x26;
	FillOffset( pChar, offset, sizeof( ULONG) );

#if 1
	
	pChar = des;
	*pChar = 0xe9;	pChar++;

	offset = ( ULONG)container -( ULONG)des -5;
	FillOffset( pChar, offset, sizeof( ULONG) );
	
#endif 	

}

#define TIMER_NT_READ

#define TIMER_NT_WRITE

//#define TIMER_NT_FLUSH

//#define TIMER_NT_DEVICECTL


VOID NtIntercept(){
	
#ifdef TIMER_NT_READ

	SysInterceptNtRoutine(
		
		NtReadFile,
		ContainerNtReadFile,
		ContainerBHNtReadFile
	);
#endif

#ifdef TIMER_NT_WRITE

	SysInterceptNtRoutine(

		NtWriteFile,
		ContainerNtWriteFile,
		ContainerBHNtWriteFile
	);
#endif 

#ifdef TIMER_NT_DEVICECTL

	SysInterceptNtRoutine(

		NtDeviceIoControlFile,
		ContainerNtDeviceIoControlFile,
		ContainerBHNtDeviceIoControlFile
	);

#endif

#ifdef TIMER_NT_FLUSH

	SysInterceptNtRoutine(

		NtFlushBuffersFile,
		ContainerNtFlushBuffersFile,
		ContainerBHNtFlushBuffersFile
	);

#endif

	InterceptNtClose( NtClose, ContainerNtClose );
		
}

VOID NtRestore(){

#ifdef TIMER_NT_READ

	SysRestoreNt( NtReadFile);

#endif

#ifdef TIMER_NT_WRITE

	SysRestoreNt( NtWriteFile);

#endif

#ifdef TIMER_NT_DEVICECTL

	SysRestoreNt( NtDeviceIoControlFile);

#endif

	SysRestoreNt( NtClose );

}



