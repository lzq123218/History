
#include "archiveDetector.h"
#include <iostream>

using namespace std;
using namespace CCAArchiveDetector;


class AnalyticsCaller : public Callback {

	public:
		AnalyticsCaller();
		~AnalyticsCaller();

		virtual RetVal Scan( string& aFilename );
};

AnalyticsCaller::AnalyticsCaller(){

}

AnalyticsCaller::~AnalyticsCaller(){

}

RetVal
AnalyticsCaller::Scan( string& aFilename ){
	
	cout << aFilename;

	return notMalicious;
}

AnalyticsCaller gAnalyticsCaller;

int
main(){

	unsigned long long maxDiskSpace = 0x40000000;
	unsigned long long maxSpaceUsed = 1199159;

	ConfigMgr::SetWorkingDir( "/var/tmp", maxDiskSpace );
	ConfigMgr::SetSingleFileOptions( 20, 0x400, 600, maxSpaceUsed );
	
	SpaceMgr spaceMgr;
	ConfigMgr::SetSpaceMgr( &spaceMgr );

	string filename( "/var/tmp/7z/7z465_extra.7z" );


	ArchiveFile *arc = Recognizer::Recognize( filename, &gAnalyticsCaller, &spaceMgr );

	if ( arc ){
		
		list< ResultRecord* > result;

		arc->DetectMalicious( result );
		Recognizer::DestroyArchiveFile( arc );
	}	

	return 0;
}
