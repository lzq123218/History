#ifndef ZFUNCTION

#define ZFUNCTION



#ifdef __cplusplus
extern "C" {
#endif

long BuildCompressedFile( char *aInputFile, char *aOutputFile );

long DecompressFile( char *aInputFile, void* *aRet );



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif