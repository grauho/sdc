#ifndef MAIN_H 
#define MAIN_H

#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <string.h>

#include "cJSON.h"
#include "portegg.h"

#define SDC_BOOL    char
#define SDC_STAT    char
#define SDC_TRUE    1
#define SDC_FALSE   0
#define SDC_SUCCESS 0
#define SDC_FAILURE 1

/* Structure of a .safetensor file */
/*
 * A UTF-8 encoded JSON file:
 *
 * 8 bytes containing a unsigned 64-bit integer, represents the N-byte size of
 * 	the header before the data section
 * N byte header containing tensor information corresponding to the tensors
 * 	stored in the data section, a JSON of the form:
 *
 * 	"tensor-name" : 
 * 	{
 * 		"dtype" : <type>		        eg: "F16"
 * 		"shape" : list of <int> 	        eg: [1, 5, 23]
 * 		"data_offsets" : [<start>:<end>]	eg: [69, 420]
 * 	}
 *
 * 	Also potentially a __metadata__ key which "stores a free form 
 * 	text-to-text map" according to the spec
 *
 * 	Allowable dtypes: F64, F32, F16, BF16, I64, I32, I16, I8, U8, BOOL
 * 	All of these are self explainatory except for BF16
 *
 * Note that .safetensors files are little-endian encoded 
 */

/* All possible .safetensors dtypes according to the standard */
enum dataType
{
	FLOAT_64 = 0,
	FLOAT_32,
	FLOAT_16,
	BFLOAT_16,
	SIGNED_64,
	SIGNED_32,
	SIGNED_16,
	SIGNED_8,
	UNSIGNED_8,
	BOOLEAN,
	DTYPE_UNKNOWN,
	NUM_DATA_TYPE = DTYPE_UNKNOWN,
	DTYPE_INT_OFF = SIGNED_64
};

static const char * const dtype_strs[] =
{
	"float_64",
	"float_32",
	"float_16",
	"bfloat_16",
	"signed_64",
	"signed_32",
	"signed_16",
	"signed_8",
	"unsigned_8",
	"boolean"
};

static const size_t data_str_len = sizeof(dtype_strs) / sizeof(dtype_strs[0]);

#endif /* MAIN_H */
