
#ifndef ENGBASE
#define ENGBASE


#ifndef _WINDEF_

typedef char CHAR, BYTE, *PCHAR;
typedef unsigned char  UCHAR, *PUCHAR;

typedef long LONG;
typedef unsigned long DWORD;


typedef unsigned char BOOL;

typedef void *HANDLE;
typedef void *PVOID;
typedef short SHORT;
typedef short WORD ;

typedef struct _long64{

	DWORD low;
	DWORD hig;

}LONG64;

#endif

#ifndef NULL

#define NULL ( 0 )

#endif

#define TRUE   1
#define FALSE  0


typedef struct _list{

	struct _list* pre;
	struct _list* next;

}List, *PList;

#define INITLIST(p) ( (PList)(p) )->pre = NULL;( (PList)(p) )->next = NULL;

#define INITLISTHEAD(p) ( (PList)(p) )->pre = p;( (PList)(p) )->next = p;


#define INSERTLISTHEAD( head, node ) { PList tmp;\
tmp = head->next; head->next = node; node->pre = head; node->next= tmp; tmp->pre = node;}

#define INSERTLISTTAIL( head, node ) { PList tmp;\
tmp = head->pre; tmp->next = node; node->pre = tmp; node->next = head; head->pre = node;}

#define REMOVEFROMLIST( node ) { PList pre = node->pre; PList next = node->next;\
pre->next = next; next->pre = pre;}



typedef struct _tree{

	struct _tree* left;
	struct _tree* right;
	struct _tree* parent;

}Tree, *PTree;

#define INITTREE(p) ( (PTree)(p) )->left = NULL;\
( (PTree)(p) )->right = NULL;\
( (PTree)(p) )->parent = NULL;


typedef struct _feature{
	
	DWORD offset;
	DWORD value;
}Feature;


#define PE_INFO_OFFSET_PREFIX  ( (unsigned long)0xf0000000 )

#define SECTION_OFFSET_PREFIX  ( (unsigned long)0xe0000000 )

#define PE_INFO_NUMBER_OF_SECTIONS  1
#define PE_INFO_ADDRESS_OF_ENTRY    2
#define PE_INFO_SIZE_OF_RAWDATA     3
#define PE_INFO_POINTER_TO_RAWDATA  4
	

#define SECTION_OFFSET_NUM    21

#define SIZE_Z  3


#define ERR_NO_ERR          0
#define ERR_DATA_NOT_READY  3
#define ERR_DATA_PASSED     4


#define BASELINELIB		0x1l
#define INCREMENTAL		0x2l

#define OPTIMIZED		0x3l

#define RUNONFPGA		0x4l


typedef struct _StdHead{

	UCHAR Sig[8];	//signature always "ZHALFA"

	DWORD LibType;
	DWORD ReleaseVersion;	//if a virus library will be released ,here is releaseversion
							//otherwise is zero
	DWORD ToolsVersion;		//version of tools which generate library

	DWORD Date[4];			//year - month - day - hour
	DWORD Count;			//total records

	DWORD RecordSize;		//may be zero

	DWORD ExtOffset;		//may be zero
	DWORD ExtSize;			//may be zero

	DWORD Reserved[4];

	DWORD CheckSum;


}StdHead, *PStdHead;


typedef struct _VirusInfo{

	DWORD VID;
	DWORD VNlen;
	DWORD VName;

}VirusInfo, *PVirusInfo;

typedef struct _InfoNode{

	DWORD Index;	//return by engine to get real infomation
	DWORD Type;

	union{
		VirusInfo v;

		void *p;	
	}u;

}InfoNode, *PInfoNode;

typedef struct _IndexElement{
	
	DWORD val;
	VirusInfo vi;

}IndexElement;

typedef struct _IndexElementArray{

	DWORD offset;
	DWORD count;

	IndexElement ie[1];

}IndexElementArray, *PIndexElementArray;

typedef struct _InfoLst{

	List lst;
	InfoNode node;

}InfoLst,*PInfoLst;



struct _Index;

typedef struct _IndexRecord{

		DWORD first; // first index value in array
		DWORD last;  // last index value in array

		DWORD type; 

		union{

			struct{
					
				DWORD postion; //offset relative to beginning positon of InfoNode array
				DWORD count;
			}Range;

			struct _Index *next;
				
		}RangeOrIndex;


}IndexRecord, *PIndexRecord;

#define NEXT_TYPE_RANGE  4
#define NEXT_TYPE_INDEX  44


typedef struct _Index{
	
		DWORD count;

		IndexRecord array[1];

}Index, *PIndex;

#define NEED_BUILD_INDEX		256
#define INDEX_COUNT_PER_LEVEL	4


#endif


