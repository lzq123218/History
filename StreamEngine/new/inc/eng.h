/*
**	This headfile includes structs and macros that stream engine needs
*/
#ifndef STREAM_ENGINE
#define STREAM_ENGINE

#ifdef __cplusplus

extern "C"
{
#endif

#include "engbase.h"

#include "viruslib_ex.h"
#include "viruslib_in.h"

#include "hashtab.h"



typedef struct _LookBack{

//	BOOL need;

	BOOL on;
	CHAR Last9[9];

}LookBack;


#define OBJTYPE ((DWORD) 0x44448888)

#define OBJSTATUS_WAITTING_DATA          4
#define OBJSTATUS_FINDING_REALDATA     1
#define OBJSTATUS_SCANVIRUS_PENDING   2
#define OBJSTATUS_NOVIRUS_FOUND          3
#define OBJSTATUS_VIRUS_FOUND               5

#define PROTOCOL_MAIL 9


#define PE_HEAD_NORMAL			0xc00000
#define PE_HEAD_COMPRESSED		0xd00000

typedef struct _PeInfo{

	SHORT status;
	SHORT substatus;

	DWORD PHtype;

	union{
		
		DWORD peoffset;
		DWORD codeoffset;
		DWORD pebegin;
		DWORD sectionoffset;

	}overlay;

	DWORD sectionsnum; //NumberOfSections
	DWORD SizeOfOptionalHeader;
	DWORD entryoffset; //AddressOfEntryPoint
	DWORD SizeOfImage; //SizeOfImage

//->Marchine
	DWORD currentsection;

	DWORD VirtualAddress;
	DWORD SizeOfRawData;
	DWORD RawData;

}PeInfo, *PPeInfo;

typedef struct _B64Info{

	DWORD count;
	CHAR buff[4];

}B64Info;


typedef struct _StreamObject{

/*
** Head used to protect this structure
*/
	DWORD type;
	DWORD size;
/*
** 
*/
	DWORD protocol;
	BOOL status;
	List objlist;
	List scanlist;//changed 2004-10-28,because never used
	PTree scanTree;// used to store scan infomation
	PTree viruslib;
	PTree viruslib2;//added 2004-10-29

/*
** used to record stream info
*/
	DWORD startpos;// data position which we are actually interested
	DWORD recvdlen;// length of data that we have received already

	DWORD currentstart;// it records current data received range;
	DWORD nextstart;

	DWORD other;
	DWORD virusfound;
	DWORD virusindex;
	DWORD usinglibtype;

	LookBack back;
/*
	LONG64 connect;
	LONG64 disconnect;
*/
	PeInfo data;
	// added 2005-03-08 
	// this field extend to store more infomation of stream such as
	// PE infomation

	B64Info b64;
	//struct hashtab* hash;

}StreamObject, *PStreamObject;


/*
#define INITITEMHEAD(P) ( (PItemHead) P )->name = NULL;
*/

#define	VLIB_ORIGINAL	0x00000001
#define VLIB_BASE64		0x00000002


PCHAR EngineMalloc( DWORD size );
BOOL EngineMfree( PCHAR start );


//初始化引擎，在最开始使用的时候调用一次

DWORD RSEngineInit();

//反初始化引擎，在最后关闭的时候调用一次

DWORD RSEngineUninit();


//在一个流开始的时候调用
//参数：
//	Protocol:协议类型，这里只能指定一种。
//返回值: Context,用于表示这个流的上下文句柄
// 0xFFFFFFFF为错误，可能的情况主要是病毒库没有加载。
int RSConnectEvent( DWORD aProtocol, DWORD *aRet );

//在一个流结束的时候调用
//参数：
//	Context:流上下文
int RSDisconnectEvent( DWORD Context );

int RSResetContext( DWORD Context );

//将数据传入，进行扫描
//参数：
//	Context:流上下文
//	pbDataBuff:本次传入流的数据
//	Length:本次传入数据的长度
//返回值:
//	>0	为病毒ID
//	==0	确认没有病毒
//	-1	继续等待数据
//	<-1	其他错误情况，将来定义
LONG RSDataScan( DWORD Context, const BYTE* pbDataBuff, DWORD Length, DWORD *aRet );

//根据病毒ID得到名称
//参数：
//	ID:病毒Id
//返回值:
//	病毒名称
DWORD RSGetEngineVersion( void );

DWORD RSRegisterCallback( void(*aFun)( DWORD , void* ), void* aArg );

void RSDeregisterCallback( void );

extern PTree global_root;
extern PTree global_root2;//added 2004-10-29
extern DWORD gCount;


#define PE_STATUS_WAITING_MZ         0
#define PE_STATUS_MZ_RECEIVED	     1
#define PE_STATUS_WAITING_PE         2	 
#define PE_STATUS_PE_RECEIVED        3
#define PE_STATUS_PE_INFO_READY      4
#define PE_STATUS_PE_SECTION_READY   5

#define PE_SUBSTATUS_WAITING_VIRTUAL_ADDRESS		1
#define PE_SUBSTATUS_WAITING_SizeOfRawData			2

#define PE_SUBSTATUS_CAN_COMPARE					3

#define PE_SUBSTATUS_WAITING_AddressOfEntryPoint	4
#define PE_SUBSTATUS_WAITING_SizeOfImage			5
#define PE_SUBSTATUS_WAITING_SizeOfHeaders			6		


#define UNKNOWN_VIRUS	44444


void ResetComparedTimes();
DWORD GetComparedTimes();

extern Tree VirusTreeRoot;


extern PeerArray* gPeerArrayRoot;



#ifdef __cplusplus

} // extern "C"
#endif

#endif

