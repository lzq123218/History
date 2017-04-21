
#ifndef TIMER_CACHE

#define TIMER_CACHE


typedef struct _IndexHead{

	LARGE_INTEGER Count;

	LARGE_INTEGER DataOffset;

} IndexHead;


typedef struct _IndexEntry{

	UNICODE_STRING UncName;
	
	LARGE_INTEGER offset;

	STRING AnsiName;
	
	LARGE_INTEGER Info;
	
	LARGE_INTEGER LastModifiedDate;

	BOOLEAN  ScanCompleted;


} IndexEntry, *PIndexEntry;


PIndexEntry NewIndexEntry();

PIndexEntry GetIndexEntryAnsi( STRING *pStr );

PIndexEntry GetIndexEntryUnc( UNICODE_STRING *pUnc );

NTSTATUS RemoveIndexEntry();

NTSTATUS DeleteAllIndexEntry();

NTSTATUS SaveIndexFile();

NTSTATUS LoadIndexFile();

NTSTATUS CacheInit();

#endif

