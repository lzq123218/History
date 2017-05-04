#ifndef DECOMPRESSION
#define DECOMPRESSION

#include <string>
#include <list>

using namespace std;

namespace CCAArchiveDetector {

class Decompressor;

class ArchiveRecognizer;
class ArchiveFile;
class Callback;
class SpaceMgr;
class ResultRecord;

enum RetVal{

	notDefined,

	successful,

	terminated,

	hasError,

	notArchive,

	notMalicious,
	malicious,

	exceedingMaxLayers,
	exceedingMaxInternalFileCount,
	exceedingMaxSpace
};


class Decompressor{

	public:
		virtual RetVal Decompress( string& aFile, string& aDesDir );
};

class ArchiveFile{

	public:
		explicit ArchiveFile();
		~ArchiveFile();

		RetVal SetSpaceMgr( SpaceMgr* );
		RetVal SetCallback( Callback* );

		RetVal SetDecompressor( Decompressor* );
		RetVal SetFilename( string& );
		RetVal DetectMalicious( list< ResultRecord* >& );

	private:
		RetVal BrowseFiles( string &aDir, list<ResultRecord*>& aResults,  list<ArchiveFile*>& aArchList );

		string mFilename;
		Decompressor *mDecompressor;
		SpaceMgr *mSpaceMgr;
		Callback *mCallback;
};

class Recognizer{

	public:
		static  bool IsArchive( string& aFilename );
		static  ArchiveFile* Recognize( string& aFilename, Callback* aCb, SpaceMgr* aSpaceMgr );
		static  void DestroyArchiveFile( ArchiveFile* aArchFile);
};

class Callback{

	public:
		Callback(){};
		virtual ~Callback(){};

		virtual RetVal Scan( string& aFilename ) = 0; 
};

class SpaceMgr{

	public:
		SpaceMgr();
		~SpaceMgr();
		
		RetVal IncreaseFileCount();
		RetVal IncreaseUsedspace( unsigned long aSize );
		RetVal IncreaseLayer();

		void SetMaxValue( unsigned aMaxLayers,  unsigned long long aMaxSpaceUsed, unsigned aMaxInternalFileCount );

		RetVal Alert();
		RetVal CreateDir( string& aDir );

	private:
		string mRootDir;
		
		unsigned long long mMaxSpace;
		unsigned long long mSpaceUsed;

		unsigned mMaxInternalFileCount;
		unsigned mInternalFileCount;

		unsigned mMaxLayers;
		unsigned mLayers;
};

class ResultRecord{

	public:
		RetVal show();
};


class ConfigMgr{
	
	public:
		static void SetWorkingDir( const char* aPath, unsigned long long aSize );

		static void SetSingleFileOptions(

			unsigned aMaxLayers,
			unsigned long aMaxSingleFileSize,
			unsigned long aMaxInternalFileSize,
			unsigned long long aMaxSpaceUsed
		);
		
		static void SetSpaceMgr( SpaceMgr* aSpaceMgr );
		static unsigned long GetMaxSingleFileSize();
		
	private:

		static string workingDir;
		static unsigned long long maxDiskSpace;
		
		static unsigned maxLayers;
		static unsigned long maxSingleFileSize;
		static unsigned long maxInternalFileCount;
		static unsigned long long maxSpaceUsed;

};

};
#endif

