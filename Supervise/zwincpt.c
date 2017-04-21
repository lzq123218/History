
#include	"predef.h"
#include	"zwincpt.h"


HANDLE hZwLock = NULL;
HANDLE hIoLock = NULL;

ULONG ulOpenCount = 0L;
ULONG ulCloseCount = 0L;
ULONG ulIoCreateCount = 0L;


PVOID gpvZwFun = NULL;
PVOID gpvIoFun = NULL;
PVOID gpvCreateFun = NULL;


#define EXTRASIZE   26

UCHAR extracode[ EXTRASIZE ] = {

	0xb8, 0x64, 0x00, 0x00, 0x00,	//	mov eax, 0x0000 0064 ;
	
	0x60, 						//	pushad
	0x9c,          					//	pushfd
	
	0x90, 0x90, 0x90, 0x90,		//	reserve positions for  call
	0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90,
	
	0x9d,          					//	popfd	
	0x61,  						//	popad
	
	0xc3						//	return near

};

#define OFFSET	24

UCHAR extracode3[ EXTRASIZE + OFFSET ] = {

	0xb8, 
	0x18, 0x00, 0x00, 0x00,		//	mov eax, 0x0000 0018 ;
	
	0x60, 						//	pushad
	0x9c,          					//	pushfd

#if 1

	0x81, 0xec,					//	sub esp, xxxx
	0x04, 0x00, 0x00, 0x00,

	0xb9,						//	mov ecx, xxxx
	0x04, 0x00, 0x00, 0x00,	
								//DES :

	0x8a, 0x84, 0x0c, 				//	mov al, [ esp + ecx + xxxx -1]
	0x2f, 0x00, 0x00, 0x00,

	0x88, 0x44, 0x0c, 0xff,		//	mov byte ptr [ esp + ecx -1 ], al

	0xe2, 0xf3,					//	loop DES
	
#endif 

	0x90, 0x90, 0x90, 0x90,		//	reserve positions for  call
	0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90,
	
	0x9d,          					//	popfd
	0x61,  						//	popad
	
	0xc3						//	return near
};

#define 	EXTRACODE2SIZE  25

UCHAR extracode2[EXTRACODE2SIZE] = {

	0x55,					//	push ebp
	0x8b, 0xec,				//	mov ebp, esp
	0x6a, 0xff,				//	push ff

	0x60,					//	pusha
	0x9c,					//	pushfd

	0x90, 0x90, 0x90, 0x90,	//	call
	0x90, 0x90,
	
	0x9d,					//	popfd
	0x61,					//	popad

	
	0x90,					
	0x90,					

	0x90, 0x90, 0x90, 0x90,	//	JMP 
	0x90, 0x90, 0x90, 0x90
};

#define	EXTRACODE4SIZE    1

UCHAR extracode4[EXTRACODE4SIZE] = {

	//	pushfd
	//	push eax
	//	push ecx
	
	//	sub esp, 0x38 + 4 + 12
	
	//	mov ecx, 0x38 + 4 + 12
	//des:
	//	mov al, [ esp +ecx +0x38 +4 + 12 ]
	//	mov byte ptr [esp + ecx ], al
	//	loop des

	
	//	mov eax,  function address;
	
	//	mov byte ptr[esp -12 ], al
	//	mov byte ptr[esp -12 + 1], ah
	
	//	bswap eax
	//	mov byte ptr[esp -12 + 2], ah
	//	mov byte ptr[esp -12 + 3], al

	//	pop ecx
	//	pop eax
	//	popfd

	//	nop;

	//	jmp 
	
	0x90
};

VOID SysIntercept( PVOID des, PVOID exec , PVOID container, UCHAR input[]);
VOID SysRestore( PVOID des, UCHAR input[] );

VOID SysInterceptIoRoutine( PVOID des, PVOID exec , PVOID container);
VOID SysRestoreIoRoutine();

VOID ContainerOpen();
VOID ContainerClose();
VOID ContainerIoCreateFile();

VOID suOpen();
VOID suClose( HANDLE hHandle );
VOID suIoCreateFile();

/**
  *	this function intercept  syscall  ZwXXX 
  *	argument : 
  *
  *		des: function pointer to ZwXXX
  *
  *		exec:  function executed  when syscall being intercepted
  *		
  *		container: 
**/

VOID SysIntercept( PVOID des, PVOID exec , PVOID container , UCHAR input[] ){

	UCHAR* fun1 =  NULL;
	UCHAR* pChar = input;
	ULONG   address = 0L ;
	USHORT reg_cs = 0 ;
	ULONG  offset = 0;
	int i ;

	if ( !des||!exec || !container ){

		return ;
	}

//	DbgPrint( "des address = %x\n", (ULONG)des );
//	DbgPrint( "container address = %x\n", (ULONG)container );

/**
**	construct a near call instrument 
**
**/

	input[31] =  0xe8;
	pChar = input + 32;
	
	offset = ( ULONG)exec -( ULONG)container -5 -31;	
	FillOffset( pChar, offset, sizeof( ULONG) );

/**
**	change container function content
**
**/
	
	fun1 = ( UCHAR*)container;
	pChar = input;
	
	for ( i =0; i < ( EXTRASIZE + OFFSET ) ; i++){ 
		
		*fun1 = *pChar ;

		fun1++;	pChar++;		
	}						

/**
**	following code is used to redirect  ZwOpenFile to code writen by timer
**	
**/
#if 1
		fun1 = ( UCHAR*) des;

		*fun1 = 0xe8;
		fun1++;
		
		offset = ( ULONG)container- ( ULONG)des -5;
		FillOffset( fun1, offset, sizeof(ULONG) );
		
#endif		
//	hZwLock = MmLockPagableCodeSection( container);


}


VOID SysRestore( PVOID des, UCHAR input[]){

	UCHAR* func = ( UCHAR*)des;
	UCHAR* pChar = input;
	int i;

	for ( i = 0; i < 5; i++ ){

		*func = *pChar ;
		func++;	pChar++;

	}

	if ( hZwLock !=(HANDLE) -1 ){

//		MmUnlockPagableImageSection( hZwLock );
	}
	
}

VOID SysInterceptIoRoutine( PVOID des, PVOID exec , PVOID container){

	UCHAR* pChar = extracode2;
	UCHAR* fun1 = NULL;
	ULONG  offset = 0;
	int i = 0;

	if ( !des||!exec || !container ){

		return ;
	}

	offset = ( ULONG)exec -( ULONG)container -5 -7;

	extracode2[7] =  0xe8;		//	add call near
	pChar += 8;
	fun1 =( UCHAR*)( &offset );
	for ( i = 0; i < 4 ; i++ ){

		*pChar = *fun1;
		
		fun1++;	pChar++;

	}

	extracode2[15] = 0xe9;		//	add JMP NEAR

	offset = ( ULONG)des - ( ULONG) container  -15 ;
	pChar= extracode2;
	pChar+= 16 ;
	fun1 =( UCHAR*)( &offset );
	for ( i = 0; i < 4 ; i++ ){

		*pChar = *fun1;
		
		fun1++;	pChar++;	

	}
	pChar = extracode2;			//	chang JMP002 
	fun1 = container;

	for ( i = 0; i < EXTRACODE2SIZE; i++ ){

		*fun1 = *pChar ;
		
		fun1++;	pChar++;

	}

	offset = ( ULONG)container -( ULONG)des -5;
	pChar = ( UCHAR*)&offset ;
		
	fun1 = ( UCHAR*)des;
	*fun1 = 0xe9;
	fun1++;

	for ( i =0; i < 4 ; i++ ){

		*fun1 = *pChar;
		
		fun1++;	pChar++;

	}
	hIoLock = MmLockPagableCodeSection( container);

}

VOID SysRestoreIoRoutine(){

	UCHAR* func = ( UCHAR*)gpvIoFun;
	UCHAR* pChar = extracode2;
	int i;

	for ( i =0 ; i < 5 ; i++ ){

		*func = *pChar ;
		
		func++;	pChar++;

	}

	if ( hIoLock!=(HANDLE) -1 ){

		MmUnlockPagableImageSection( hIoLock );
	}
	
}

VOID suOpen(){

	ulOpenCount ++;

}

VOID suClose( HANDLE hHandle ){

	OBJECT_HANDLE_INFORMATION ObjInfo;

	NTSTATUS status;
	PVOID pUnknown;

	ulCloseCount ++;
	
	ShowContext();
	
	DbgPrint( "ZwClose( 0x%x )\n", hHandle);
	
	status = ObReferenceObjectByHandle(

		hHandle,
		STANDARD_RIGHTS_ALL,
		NULL,
		KernelMode,
		&pUnknown,
		&ObjInfo
	);

	if ( !NT_SUCCESS( status) ){
		
		DbgPrint( "reference error\n");
		
	}else{
	
/**
**	add code here
**/
		DbgPrint( "HandleAttr = %x;AccessMask = %x\n",
		
			ObjInfo.HandleAttributes,
			ObjInfo.GrantedAccess
		);
		
		ObDereferenceObject ( pUnknown);

	}
	
}

VOID suIoCreateFile(){

	ShowContext();
	
}

VOID ContainerIoCreateFile(){

	DbgPrint( "SuIoCreateFile\n");

	_asm {

		nop; 
		nop;
		nop; 
		nop; 
		nop;
		nop; 
		nop; 
		nop; 
		nop; 
		nop;
		
		nop;
		nop;
		nop; 
		nop;
		nop;
		nop;
		nop;
		nop; 
		nop;
		nop;
		
		nop; 
		nop; 
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

VOID ContainerOpen(){
	
	DbgPrint("SuOpen\n");

	_asm {					//	only for position 

		nop; 
		nop; 
		nop; 
		nop; 
		nop;
		nop; 
		nop; 
		nop; 
		nop; 
		nop;
		
		nop; 
		nop; 
		nop; 
		nop; 
		nop;
		nop; 
		nop; 
		nop; 
		nop; 
		nop;
		
		nop; 
		nop; 
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

VOID ContainerClose(){

	DbgPrint( "SuClose\n");

	_asm {
		
		nop; 
		nop;
		nop; 
		nop; 
		nop;
		nop; 
		nop; 
		nop; 
		nop; 
		nop;
		
		nop;
		nop;
		nop; 
		nop;
		nop;
		nop;
		nop;
		nop; 
		nop;
		nop;
		
		nop; 
		nop; 
		nop; 
		nop; 
		nop;
		nop; 
		nop; 
		nop; 
		nop; 
		nop;

		nop; 
		nop; 
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

VOID ZwIntercept(){

	//SysIntercept( ZwOpenFile, suOpen, ContainerOpen, extracode);
	
	SysIntercept( ZwClose, suClose, ContainerClose, extracode3 );
	
	gpvIoFun = IoCreateFile;
	SysInterceptIoRoutine( gpvIoFun, suIoCreateFile, ContainerIoCreateFile );

}

VOID ZwRestore(){
	
	SysRestore( ZwClose, extracode3);
	SysRestoreIoRoutine();
}
