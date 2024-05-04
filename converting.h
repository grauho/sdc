#ifndef CONVERTING_H 
#define CONVERTING_H 

#include "main.h"

#define SDC_DTYPE_IS_FLOAT(type) (((type) < SIGNED_64) ? SDC_TRUE : SDC_FALSE)

static struct
{
	const enum dataType dtype;
	const size_t size;
	const char *name;
} dtype_info[] =
{
	{FLOAT_64,   sizeof(double),  "F64"},
	{FLOAT_32,   sizeof(float),   "F32"},
	{FLOAT_16,   sizeof(int16_t), "F16"},
	{BFLOAT_16,  sizeof(int16_t), "BF16"},
	{SIGNED_64,  sizeof(int64_t), "I64"},
	{SIGNED_32,  sizeof(int32_t), "I32"},
	{SIGNED_16,  sizeof(int16_t), "I16"},
	{SIGNED_8,   sizeof(int8_t),  "I8"},
	/* I don't believe that there is proper handling for either of the 
	 * below types */
	{UNSIGNED_8, sizeof(uint8_t), "U8"},
	{BOOLEAN,    sizeof(char),    "BOOL"}
};

static const size_t dtype_info_len 
	= sizeof(dtype_info) / sizeof(dtype_info[0]);

char* downConvertDTypes(char *in, const size_t len, 
	const enum dataType in_type, enum dataType *out_type);
void dumpTypeInfo(void);

#endif /* CONVERTING_H */

