
#include "unistd.h"
#include "sys/wait.h"
#include "dirent.h"
#include "sys/stat.h"
#include "string.h"

#include <iostream>

#include "archiveDetector.h"

using namespace CCAArchiveDetector;

ArchiveFile::ArchiveFile(){

}

ArchiveFile::~ArchiveFile(){

	if ( mDecompressor ) delete mDecompressor;
}

RetVal
ArchiveFile::SetDecompressor( Decompressor* aDecompressor ){


	RetVal ret = notDefined;

	if ( aDecompressor ){
		
		mDecompressor = aDecompressor;

		ret = successful;
		return ret;

	}
	
	return ret;
}

RetVal
ArchiveFile::SetFilename( string& aFilename ){

	RetVal ret = notDefined;

	mFilename = aFilename;
	ret = successful;
	return ret;
}

RetVal
ArchiveFile::SetSpaceMgr( SpaceMgr *aSpMgr ){

	if ( aSpMgr ){
		
		mSpaceMgr = aSpMgr;
	}

	return successful;
}

RetVal
ArchiveFile::SetCallback( Callback *aCb ){

	if ( aCb ){
		
		mCallback = aCb;
	}

	return successful;
}


RetVal
ArchiveFile::DetectMalicious( list< ResultRecord* >& aRecordList ){

	RetVal ret = notDefined;

	string resDir;
	list< ArchiveFile* > archList;

	if( mDecompressor ){
		
		mSpaceMgr->CreateDir( resDir );
		mDecompressor->Decompress( mFilename, resDir );

		ret = BrowseFiles( resDir, aRecordList, archList );
			
		mSpaceMgr->Alert();
		//for each in archList

		list< ArchiveFile*>::iterator it;	

		if( archList.size() && ret == notMalicious ){

			ret = mSpaceMgr->IncreaseLayer();
			
			if ( ret == successful ){

				for ( it = archList.begin(); it != archList.end(); it++ ){
				
					ArchiveFile *tmp = *it;
					ret = tmp->DetectMalicious( aRecordList );

					if ( ret == malicious || ret == terminated ) break;
				}
			}
		}

		for ( it = archList.begin(); it != archList.end(); it++ ){
		
			ArchiveFile *tmp = *it;
			Recognizer::DestroyArchiveFile( tmp );
		}
		
		//cleanDir
	}
	
	return ret;
}

RetVal
ArchiveFile::BrowseFiles( string &aDir, list<ResultRecord*>& aResults, list<ArchiveFile*>& aArchList ){

	RetVal ret = notMalicious;

	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	long res = 0;
    

	if( ( dp = opendir( aDir.c_str() ) ) == NULL) {

		return hasError;
	}

	while( (entry = readdir(dp)) != NULL ) {

		if( strcmp( ".", entry->d_name) == 0 || strcmp( "..", entry->d_name) == 0 )
			continue;


		string filename = aDir;

		filename += "/";
		filename += entry->d_name;

		memset( &statbuf, 0, sizeof( struct stat ) );
		res = lstat( filename.c_str(), &statbuf );

		if( S_ISDIR(statbuf.st_mode) ) {

			ret = BrowseFiles( filename, aResults, aArchList );
			if ( ret != notMalicious ) break;

		}else if ( res == 0 ) {
			
			
			ret = mSpaceMgr->IncreaseFileCount();
			if ( ret != successful ) break;
			
			ret = mSpaceMgr->IncreaseUsedspace( statbuf.st_size );
			if ( ret != successful ) break;

			cout << filename << endl;

			ArchiveFile* arch = Recognizer::Recognize( filename, mCallback, mSpaceMgr );
			
			if ( arch ){
				
				aArchList.push_back( arch );
			}else{
				//call analytics

				ret = mCallback->Scan( filename );
				if ( ret == malicious ) break;

			}

		}
	}

	closedir(dp);
	
	if ( ret == successful ) ret = notMalicious;
	return ret;
}

RetVal
Decompressor::Decompress( string& aFilename, string& aResultDir ){

	//call 7z to decompress
	string command( "7z x -bd -y -o\"" );
	command +=  aResultDir + "\" " + aFilename;

	system( command.c_str() );
}


bool
Recognizer::IsArchive( string& aFilename ){
	
	return true;
}

ArchiveFile*
Recognizer::Recognize( string& aFilename, Callback* aCb, SpaceMgr* aSpaceMgr ){

	ArchiveFile* arch = NULL;
	

	if ( aCb && aSpaceMgr && IsArchive( aFilename ) ){

		arch = new ArchiveFile();

		if ( arch ){

			arch->SetFilename( aFilename );	
			arch->SetDecompressor( new Decompressor() );
			
			arch->SetSpaceMgr( aSpaceMgr );
			arch->SetCallback( aCb );
		}
	}

	return arch;
}

void
Recognizer::DestroyArchiveFile( ArchiveFile* aArch){

	if ( aArch ) delete aArch;	
}

string ConfigMgr::workingDir;
unsigned long long ConfigMgr::maxDiskSpace;

unsigned ConfigMgr::maxLayers;
unsigned long ConfigMgr::maxSingleFileSize;
unsigned long ConfigMgr::maxInternalFileCount;
unsigned long long ConfigMgr::maxSpaceUsed;

void
ConfigMgr::SetWorkingDir( const char* aPath, unsigned long long aSize ){

}

void
ConfigMgr::SetSingleFileOptions(

	unsigned aMaxLayers,
	unsigned long aMaxSingleFileSize,
	unsigned long aMaxInternalFileCount,
	unsigned long long aMaxSpaceUsed
){

	maxLayers = aMaxLayers;
	maxSingleFileSize = aMaxSingleFileSize;
	maxInternalFileCount = aMaxInternalFileCount;
	maxSpaceUsed = aMaxSpaceUsed;
}

void
ConfigMgr::SetSpaceMgr( SpaceMgr* aSpaceMgr ){
	
	if ( aSpaceMgr ){
		
		aSpaceMgr->SetMaxValue( maxLayers, maxSpaceUsed, maxInternalFileCount );
	}
}

SpaceMgr::SpaceMgr(){

	mRootDir = "/var/tmp/";
	mLayers = 1;
	mInternalFileCount = 0;
	mSpaceUsed = 0;
}

SpaceMgr::~SpaceMgr(){

}

RetVal
SpaceMgr::CreateDir( string& aDir){

	char randNum[100];

	sprintf( randNum, "%d", rand() );
	aDir = mRootDir + randNum;

	//mkdir();
	return successful;
}

RetVal
SpaceMgr::Alert(){

	return successful;
}

RetVal
SpaceMgr::IncreaseFileCount(){

	RetVal ret = successful;

	if ( mInternalFileCount + 1 > mMaxInternalFileCount ){

		ret = exceedingMaxInternalFileCount;
	}else{
		mInternalFileCount++;	
	}

	return ret;
}

RetVal
SpaceMgr::IncreaseUsedspace( unsigned long aSize ){
	
	RetVal ret = successful;

	if ( mSpaceUsed + aSize > mMaxSpace ){

		ret = exceedingMaxSpace;
	}else{
		mSpaceUsed += aSize;	
	}

	return ret;
}

RetVal
SpaceMgr::IncreaseLayer(){
	
	RetVal ret = successful;

	if ( mLayers+1 > mMaxLayers ){

		ret = exceedingMaxLayers;
	}else{
		mLayers++;	
	}

	return ret;
}

void
SpaceMgr::SetMaxValue( unsigned aMaxLayers, unsigned long long aMaxSpaceUsed, unsigned aMaxInternalFileCount ){

	mMaxLayers = aMaxLayers;
	mMaxSpace = aMaxSpaceUsed;
	mMaxInternalFileCount = aMaxInternalFileCount;
}
