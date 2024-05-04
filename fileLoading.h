#ifndef FILE_LOADING_H
#define FILE_LOADING_H
	
#include "main.h"

SDC_STAT convertSafetensorFile(const char *filepath, const char *outpath);
void verbosePrintf(const char *fmt, ...);

#endif /* FILE_LOADING_H */

