
#ifndef	TIMER_UTILITY

#define	TIMER_UTILITY

typedef enum _OBJECT_INFORMATION_CLASS {

	ObjectBasicInformation,
	ObjectNameInformation,
	ObjectTypeInformation,
	ObjectAllTypesInformation,
	ObjectHandleInformation
} OBJECT_INFORMATION_CLASS;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryObject(
	IN HANDLE ObjectHandle,
	IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
	OUT PVOID ObjectInformation,
	IN ULONG ObjectInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);

VOID ShowInfo();

VOID ShowObjAttr( IN POBJECT_ATTRIBUTES pObjAttr );

VOID ShowUniC( IN PUNICODE_STRING pSrc );

VOID ShowContext();

VOID ShowFileObj( PFILE_OBJECT pFileObj );

VOID ShowHandleInfo( HANDLE hHandle );

VOID FillWithNop( PVOID des, ULONG size );

VOID FillOffset( UCHAR* pChar, ULONG ulValue, ULONG ulCount );

VOID Display( UCHAR* puc, int count );

BOOLEAN noname( IN HANDLE hHandle );


#endif

