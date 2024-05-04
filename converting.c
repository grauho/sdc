#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "converting.h"
#include "fileLoading.h" /* for verbosePrintf */

#define SDC_MIN(x, y) ((x) < (y) ? (x) : (y))
#define SDC_MAX(x, y) ((x) > (y) ? (x) : (y))
#define SDC_CLAMP(min, x, max) (SDC_MIN(SDC_MAX(x, min), max))

#define SDC_INT_TO_I64(type, in, out, len)                     \
do                                                             \
{                                                              \
	size_t iti_i;                                          \
	                                                       \
	for (iti_i = 0; iti_i < len; iti_i++)                  \
	{                                                      \
		((int64_t *) out)[iti_i]                       \
			= (int64_t) (((type *) in)[iti_i]);    \
	}                                                      \
} while (0)

#define SDC_DOWNCONVERT(in_type, in, out_type, out, max, min, len)     \
do                                                                     \
{                                                                      \
	size_t dc_i;                                                   \
	                                                               \
	for (dc_i = 0; dc_i < len; dc_i++)                             \
	{                                                              \
		if (((in_type *) in)[dc_i] < min)                      \
		{                                                      \
			((out_type *) out)[dc_i] = min;                \
		}                                                      \
		else if (((in_type *) in)[dc_i] > max)                 \
		{                                                      \
			((out_type *) out)[dc_i] = max;                \
		}                                                      \
		else                                                   \
		{                                                      \
			((out_type *) out)[dc_i]                       \
				= (out_type) (((in_type *) in)[dc_i]); \
		}                                                      \
	}                                                              \
} while (0)

enum dataType float_out = FLOAT_32;

enum
{
	INCOMING = 0,
	OUTGOING
};

struct
{
	size_t type[2][NUM_DATA_TYPE];
} conversion_info = {0};

/* IEEE double-precision float */
/* 1 sign bit
 * 11 exponent bits
 * 52 fraction bits */

/* IEEE single-precision floating point */
/* 1 sign bit
 * 8 exponent bits
 * 23 fraction bits */

/* IEEE half-precision float */
/* 1 sign bit
 * 5 exponent bits
 * 10 fraction bits  */

/* Brain float*/
/* 1 sign bit
 * 8 exponent bits
 * 7 fraction bits */

void dumpTypeInfo(void)
{
	size_t i;

	verbosePrintf("\nData type conversion information\n");

	for (i = 0; i < UNSIGNED_8; i++)
	{
		verbosePrintf("%s:\t%-4lu -> %lu\n",
			dtype_strs[i], 
			conversion_info.type[INCOMING][i],
			conversion_info.type[OUTGOING][i]);
	}
}

/* Helper function to get around strict aliasing */
static uint32_t asU32(const float in) 
{
	uint32_t out = 0;

	memcpy(&out, &in, sizeof(float));

	return out;
}

static float asF32(const uint32_t in)
{
	float out = 0;

	memcpy(&out, &in, sizeof(float));

	return out;
}

/* Adapted from Maratyszcza's FP16 header library */
static uint16_t fltToHlf(const float in)
{
	const float scale_to_inf  = asF32(0x77800000);
	const float scale_to_zero = asF32(0x08800000);
	const uint32_t u32_in     = asU32(in);
	const uint32_t shl1       = u32_in << 1;
	const uint32_t sign       = u32_in & 0x80000000;
	uint32_t bias             = SDC_MAX((shl1 & 0xFF000000), 0x71000000);
	float base = ((((in < 0.f) ? -in : in) * scale_to_inf) * scale_to_zero)
		+ asF32((bias >> 1) + 0x07800000);
	uint32_t nonsign = ((asU32(base) >> 13) & 0x00007C00) 
		+ (asU32(base) & 0x00000FFF);

	return (uint16_t) ((sign >> 16) | ((shl1 > 0xFF000000) 
		? 0x7E00 : nonsign));
}

static uint16_t dblToBft(const double in)
{
	const float flt = ((float) in) * 1.001957f;

	return (uint16_t) (asU32(flt) >> 16);
}

static void doubleToHalf(const double *in, uint16_t *out, const size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
	{
		out[i] = fltToHlf((float) in[i]);
	}
}

static void signed64ToHalf(const int64_t *in, uint16_t *out, const size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
	{
		out[i] = fltToHlf((float) in[i]);
	}
}

static void doubleToBrain(const double *in, uint16_t *out, const size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
	{
		out[i] = dblToBft(in[i]);
	}
}

static void signed64ToBrain(const int64_t *in, uint16_t *out, const size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
	{
		out[i] = dblToBft((double) in[i]);
	}
}

/* All float values can be represented accurately as a double so this rather
 * trivial */
static void floatToDouble(const float *in, double *out, const size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
	{
		out[i] = (double) in[i];
	}

	return;
}

char* downConvertDTypes(char *in, const size_t len, 
	const enum dataType in_type, enum dataType *out_type)
{
	/* Both int64_t and double should be eight bytes long */
	char *tmp_arr = calloc(len, 8);
	char *out_arr = NULL;

	conversion_info.type[INCOMING][in_type]++;

	switch (in_type)
	{
		case FLOAT_64:
			memcpy(tmp_arr, in, len * sizeof(double));
			break;
		case FLOAT_32:
			/* SDC_FLT_TO_F64(float, in, tmp_arr, len); */
			floatToDouble((float *) in, (double *) tmp_arr, len);
			break;
		/* If the input is either F16 or BF16 already there isn't 
		 * much reason to touch them at the moment */
		case FLOAT_16:
			*out_type = FLOAT_16;
			out_arr = malloc(sizeof(uint16_t) * len);
			memcpy(out_arr, in, len * sizeof(uint16_t));
			conversion_info.type[OUTGOING][*out_type]++;
			free(tmp_arr);
			return out_arr;
		case BFLOAT_16:
			*out_type = BFLOAT_16;
			out_arr = malloc(sizeof(uint16_t) * len);
			memcpy(out_arr, in, len * sizeof(uint16_t));
			conversion_info.type[OUTGOING][*out_type]++;
			free(tmp_arr);
			return out_arr;
		case SIGNED_64:
			memcpy(tmp_arr, in, len * sizeof(int64_t));
			break;
		case SIGNED_32:
			SDC_INT_TO_I64(int32_t, in, tmp_arr, len);
			break;
		case SIGNED_16:
			SDC_INT_TO_I64(int16_t, in, tmp_arr, len);
			break;
		case SIGNED_8:
			SDC_INT_TO_I64(int8_t, in, tmp_arr, len);
			break;
		case UNSIGNED_8:
			SDC_INT_TO_I64(uint8_t, in, tmp_arr, len);
			break;
		case BOOLEAN: /* fallthrough */
		default:
			fprintf(stderr, "Unsupported dtype\n");
			free(tmp_arr);
			return NULL;
	}

	*out_type = float_out;
	conversion_info.type[OUTGOING][*out_type]++;
	out_arr = malloc(dtype_info[*out_type].size * len);

	/* down-convert to target float */
	switch (*out_type)
	{
		case FLOAT_32:
			if (SDC_DTYPE_IS_FLOAT(in_type) == SDC_TRUE)
			{
				SDC_DOWNCONVERT(double, tmp_arr, 
					float, out_arr, 
					FLT_MAX, -FLT_MAX, len);
			}
			else
			{
				SDC_DOWNCONVERT(int64_t, tmp_arr, 
					float, out_arr, 
					FLT_MAX, -FLT_MAX, len);
			}

			break;
		case FLOAT_16:  
			if (SDC_DTYPE_IS_FLOAT(in_type) == SDC_TRUE)
			{
				doubleToHalf((double *) tmp_arr, 
					(uint16_t *) out_arr, len);
			}
			else
			{
				signed64ToHalf((int64_t *) tmp_arr, 
					(uint16_t *) out_arr, len);
			}

			break;
		case BFLOAT_16: 
			if (SDC_DTYPE_IS_FLOAT(in_type) == SDC_TRUE)
			{
				doubleToBrain((double *) tmp_arr, 
					(uint16_t *) out_arr, len);
			}
			else
			{
				signed64ToBrain((int64_t *) tmp_arr,
					(uint16_t *) out_arr, len);
			}

			break;
		default:
			fprintf(stderr, "Bad output type: %s\n",
				dtype_info[*out_type].name);
			free(tmp_arr);
			free(out_arr);

			return NULL;
	}

	free(tmp_arr);

	return out_arr;
}
