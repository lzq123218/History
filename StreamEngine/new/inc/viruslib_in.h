
#ifndef VIRUS_LIB_INTERNEL
#define VIRUS_LIB_INTERNEL

#include "engbase.h"

#include "hashtab.h"


/* scan methord 1: dynamic mem alloc*/

#define FEATURES 18

typedef struct _VirusRec{

	Tree node;
	DWORD id;
	PCHAR name;
	DWORD total;

	Feature arry[FEATURES];

}VirusRec, *PVirusRec;

typedef struct _ScanItem{
	
	Tree node;
	PVirusRec VirusInfo;
	DWORD CurrFe;

}ScanItem, *PScanItem;

#define ITEM_NAME_LEN  128
/* scan methord 2 static mem alloc */
typedef struct {

	Tree node;

	DWORD offset;
	DWORD value;

	DWORD id;
	DWORD VID;

	struct hashtab* hash;

	/* added by zhangyuan 2004-04-19 */
	CHAR FileName[1];
	

}Item, *PItem;


/* the third scan methord */

typedef struct _Item3{

#if 0

	void* next; //指向下一个特征的指针
	void* equalnext;//指向同一级的指针

#endif

	Tree node;
	//left: 指向下一个特征的指针
	//right: 指向同一级的指针
	//parent: nothing

	unsigned char num;//指示是第几个特征 ，从 0 开始
	unsigned long id;
	unsigned long offset; //该特征的绝对偏移，根据需要可能会该为相对pe的代码节偏移
	unsigned long value; // 特征的值

} Item3, *PItem3;


struct _peerArray;

typedef struct _element{

	DWORD value;
	//DWORD id;

	struct _peerArray *sub;

}Element;

typedef struct _peerArray{

	DWORD count;
	DWORD offset;
	struct hashtab *hash;

	Element content[0];

}PeerArray;

struct _FastArray;

typedef struct _ElementInfo{

	DWORD id;
	struct _FastArray *sub;

}ElementInfo;

typedef struct _FastArray{
	
	DWORD count;
	DWORD offset;
	struct hashtab *hash;

	DWORD data[0];

}FastArray;

struct _FastArray2;

typedef struct _ElementInfo2{

	DWORD id;
	struct _FastArray2 *sub;

}ElementInfo2;

typedef struct _FastArray2{

	DWORD count;
	DWORD offset;

	//struct hashtab *hash;
	DWORD *pData;

	ElementInfo2 info[0];

}FastArray2;


DWORD RSGetLibVersion( void );

DWORD RSLoadLib( const char* pLibPath, DWORD Protocol );

DWORD RSUnloadLib();


DWORD LoadLibFromImgPeerArray( PCHAR aLibName );
void UnloadImgPeerArray();

DWORD LoadLibFromImgFastArray( PCHAR aLibName );
void UnloadImgFastArray();

typedef struct _PeerAddress{

	void*		base;

	PeerArray	*aa;

	PIndex		pIndex;

	PInfoNode	pInfoBlock;
	DWORD		InfoCount;

	void*		pMisc;

	PCHAR		pNameBlock;

}PeerAddress;


typedef struct _MistakeRec{

	DWORD IndexValue;
	DWORD vid;

	CHAR name[374];

}MistakeRec;


#endif

