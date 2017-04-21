
#ifndef	TIMER_PROC

#define TIMER_PROC

VOID BeforExecute( 
	IN PUNICODE_STRING  FullImageName,
	IN HANDLE  ProcessId, // where image is mapped
	IN PIMAGE_INFO  ImageInfo
);

VOID BeforProcessCreate( 
	IN HANDLE  ParentId,
	IN HANDLE  ProcessId,
	IN BOOLEAN  Create
);


#endif
