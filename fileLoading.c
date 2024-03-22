#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <float.h>

#include "fileLoading.h"
#include "cJSON.h"

/* TODO:
 * 	- Better float bounds checking 
 */

STL_BOOL verbose_output = STL_FALSE;

static void verbosePrintf(const char *fmt, ...)
{
	if (verbose_output == STL_TRUE)
	{
		va_list args;
		va_start(args, fmt);
		vfprintf(stdout, fmt, args);
		va_end(args);
	}
}

static char* slurpHeader(FILE *fhandle, const size_t header_len)
{
	size_t bytes_read = 0;
	char *slurp = NULL;

	if (fhandle == NULL)
	{
		fprintf(stderr, "Bad file handle for slurping\n");

		return NULL;
	}

	if ((slurp = malloc(sizeof(char) * header_len)) == NULL)
	{
		fprintf(stderr, "Failure to allocated header buffer\n");

		return NULL;
	}

	if ((bytes_read = fread(slurp, sizeof(char), header_len, fhandle))
		!= header_len)
	{
		fprintf(stderr, "Incomplete read, %lu / %lu\n",
			bytes_read, header_len);
		free(slurp);

		return NULL;
	}

	verbosePrintf("%s: %lu bytes read\n", __func__, bytes_read);

	return slurp;
}

static enum dataType extractDataType(const struct cJSON *target)
{
	const struct
	{
		const char *name;
		const enum dataType type;
	} lookup[] =
	{
		{"F64",  FLOAT_64},
		{"F32",  FLOAT_32},
		{"F16",  FLOAT_16},
		{"BF16", BFLOAT_16},
		{"I64",  SIGNED_64},
		{"I32",  SIGNED_32},
		{"I16",  SIGNED_16},
		{"I8",   SIGNED_8},
		{"U8",   UNSIGNED_8},
		{"BOOL", BOOLEAN}
	};
	const size_t table_len = sizeof(lookup) / sizeof(lookup[0]);
	size_t i;

	for (i = 0; i < table_len; i++)
	{
		if (strcmp(lookup[i].name, target->valuestring) == 0)
		{
			return lookup[i].type;
		}
	}

	return DTYPE_UNKNOWN;
}

static char* extractRawData(FILE *fhandle, const struct cJSON *src,
	const size_t binary_start, size_t *data_range)
{
	static STL_BOOL large_seek_warned = STL_FALSE;
	struct cJSON *start_cur = NULL;
	struct cJSON *end_cur   = NULL;
	char *arr               = NULL;
	size_t data_len;

	if ((fhandle == NULL) || (src == NULL) || (data_range == NULL))
	{
		fprintf(stderr, "%s: bad args\n", __func__);

		return NULL;
	}

	start_cur = cJSON_GetArrayItem(src, 0);
	end_cur   = cJSON_GetArrayItem(src, 1);

	if ((start_cur == NULL) || (end_cur == NULL))
	{
		fprintf(stderr, "Failure determining tensor data range\n");

		return NULL;
	}

	/* TODO: Figure out what the maximum integer value a double can 
	 * represent and make sure that this doesn't exceed that because 
	 * losing precision here could spoil everything but exact floating
	 * point behavior is a can of worms I don't want to deal with right
	 * now */
	data_range[0] = (size_t) start_cur->valuedouble;
	data_range[1] = (size_t) end_cur->valuedouble;
	data_range[0] = PORTEGG_LE_TO_SYS(size_t, data_range[0]);
	data_range[1] = PORTEGG_LE_TO_SYS(size_t, data_range[1]);

	data_len = data_range[1] - data_range[0];

	if ((arr = malloc(sizeof(char) * (data_len))) == NULL)
	{
		fprintf(stderr, "Malloc failure in RawData\n");

		return NULL;
	}

	if ((binary_start + data_range[0] > LONG_MAX)
	&& (large_seek_warned == STL_FALSE))
	{
		fprintf(stderr, "Attempting to seek to a range outside of the "
			"historical capacity of fseek, if output doesn't work "
			"this is the place to check first\n");

		large_seek_warned = STL_TRUE;
	}

	if (fseek(fhandle, binary_start + data_range[0], SEEK_SET) != 0)
	{
		fprintf(stderr, "Bad file seek\n");
		free(arr);

		return NULL;
	}

	if (fread(arr, sizeof(char), data_len, fhandle) != data_len)
	{
		fprintf(stderr, "Bad read from file\n");
		free(arr);

		return NULL;
	}

	return arr;
}

/* Unused, needed it to create test models from models I knew already worked */
static char* convertF32toF64(char *data, const size_t len)
{
	const float *tmp = (float *) data;
	double *ret;
	size_t i;

	if ((data == NULL)
	|| ((ret = malloc(sizeof(double) * len)) == NULL))
	{
		return NULL;
	}

	for (i = 0; i < len; i++)
	{
		ret[i] = (double) tmp[i];
	}

	free(data);

	return (char *) ret;
}

static char* convertF64toF32(char *data, const size_t len)
{
	const double *tmp = (double *) data;
	float *ret;
	size_t i;

	if ((data == NULL)
	|| ((ret = malloc(sizeof(float) * len)) == NULL))
	{
		return NULL;
	}

	for (i = 0; i < len; i++)
	{
		/* to avoid introducing "inf" but this isn't really the right
		 * way to check these particular bounds */
		if ((tmp[i] < FLT_MAX) && (tmp[i] > -FLT_MAX))
		{
			ret[i] = (float) tmp[i];
		}
		else
		{
			if (tmp[i] < 0)
			{
				ret[i] = -FLT_MAX;
			}
			else
			{
				ret[i] = FLT_MAX;
			}
		}
	}

	free(data);

	return (char *) ret;
}

static char* convertI64toF32(char *data, const size_t len)
{
	const uint64_t *tmp = (uint64_t *) data;
	float *ret;
	size_t i;

	if ((data == NULL)
	|| ((ret = malloc(sizeof(float) * len)) == NULL))
	{
		return NULL;
	}

	for (i = 0; i < len; i++)
	{
		ret[i] = (float) tmp[i];
	}

	free(data);

	return (char *) ret;
}

static STL_STAT loadTensorFromToken(FILE *fhandle, FILE *data_file, 
	const size_t binary_start, size_t *write_cursor, 
	const struct cJSON *json_cursor)
{
	enum dataType dtype     = DTYPE_UNKNOWN;
	struct cJSON *data_obj  = NULL;
	struct cJSON *dtype_obj = NULL;
	struct cJSON *arr_obj   = NULL;
	char *data              = NULL;
	size_t data_range[2]    = {0};
	size_t data_len         = 0;
	size_t write_tmp;

	if ((fhandle == NULL) || (data_file == NULL) || (json_cursor == NULL))
	{
		fprintf(stderr, "Failure to extract raw tensor data\n");

		return STL_FAILURE;
	}

	if (((dtype_obj = cJSON_GetObjectItemCaseSensitive(
		json_cursor, "dtype")) == NULL)
	|| ((data_obj = cJSON_GetObjectItemCaseSensitive(
		json_cursor, "data_offsets")) == NULL))
	{
		fprintf(stderr, "Failure to access tensor object members\n");

		return STL_FAILURE;
	}

	dtype = extractDataType(dtype_obj);
	data = extractRawData(fhandle, data_obj, binary_start, data_range);

	if (data == NULL)
	{
		fprintf(stderr, "Failure to extract raw tensor data\n");

		return STL_FAILURE;
	}

	data_len = data_range[1] - data_range[0];
	data = PORTEGG_LE_TO_SYS_RAW(data_len, data);
	
	if (dtype == FLOAT_64)
	{
		const size_t num_items = data_len / sizeof(double);

		assert(data_len % sizeof(double) == 0);

		if ((data = convertF64toF32(data, num_items)) == NULL)
		{
			fprintf(stderr, "Bad F64 -> F32 Conversion\n");
			free(data);

			return STL_FAILURE;
		}

		data_len = num_items * sizeof(float);
		cJSON_SetValuestring(dtype_obj, "F32");
	}
	else if (dtype == SIGNED_64)
	{
		const size_t num_items = data_len / sizeof(uint64_t);

		assert(data_len % sizeof(uint64_t) == 0);

		if ((data = convertI64toF32(data, num_items)) == NULL)
		{
			fprintf(stderr, "Bad I64 -> F32 Conversion\n");
			free(data);

			return STL_FAILURE;
		}

		data_len = num_items * sizeof(float);
		cJSON_SetValuestring(dtype_obj, "F32");
	}

	/* Because the locations of the raw data will change because some 
	 * tensors get converted each tensor will need to have their data 
	 * range updated in the new file regardless of dtype */
	/*
	verbosePrintf("(%lu <-> %lu) -> (%lu <-> %lu)\n",
		data_range[0], data_range[1], 
		*write_cursor, *write_cursor + data_len);
	*/

	if ((arr_obj = cJSON_GetArrayItem(data_obj, 0)) == NULL)
	{
		fprintf(stderr, "Cannot access tensor data_range array\n");
		free(data);

		return  STL_FAILURE;
	}

	porteggSysToLeCopy(size_t, *write_cursor, write_tmp);
	cJSON_SetNumberValue(arr_obj, write_tmp);

	if ((arr_obj = cJSON_GetArrayItem(data_obj, 1)) == NULL)
	{
		fprintf(stderr, "Cannot access tensor data_range array\n");
		free(data);

		return  STL_FAILURE;
	}

	porteggSysToLeCopy(size_t, *write_cursor + data_len, write_tmp);
	cJSON_SetNumberValue(arr_obj, write_tmp);
	
	data = PORTEGG_SYS_TO_LE_RAW(data_len, data);

	if (fwrite(data, sizeof(char), data_len, data_file) != data_len)
	{
		fprintf(stderr, "Bad write to file\n");
		free(data);

		return STL_FAILURE;
	}

	*write_cursor += data_len;
	free(data);

	return STL_SUCCESS;
}


STL_STAT convertSafetensorFile(const char *filepath, const char *outpath)
{
	FILE *fhandle             = NULL;
	FILE *out_file		  = NULL;
	FILE *data_file           = NULL;
	struct cJSON *json_tree   = NULL;
	struct cJSON *cursor      = NULL;
	char *header              = NULL;
	uint64_t header_len       = 0;
	size_t tensors_loaded     = 0;
	size_t tensors_total      = 0;
	size_t write_cursor       = 0;
	STL_STAT ret_code = STL_SUCCESS;

	if (((fhandle  = fopen(filepath, "rb")) == NULL)
	|| ((out_file  = fopen(outpath,  "w"))  == NULL)
	|| ((data_file = tmpfile()) == NULL))
	{
		fprintf(stderr, "Failure to open file %s\n", filepath);
		ret_code = STL_FAILURE;

		goto CLEANUP;
	}

	if (fread(&header_len, sizeof(uint64_t), 1, fhandle) != 1)
	{
		fprintf(stderr, "Failure to read from file\n");
		ret_code = STL_FAILURE;

		goto CLEANUP;
	}

	header_len = PORTEGG_LE_TO_SYS(uint64_t, header_len);
	verbosePrintf("Reading header of length: %lu\n", header_len);

	if ((header = slurpHeader(fhandle, header_len)) == NULL)
	{
		fprintf(stderr, "Failure to slurp header\n");
		ret_code = STL_FAILURE;

		goto CLEANUP;
	}

	if ((json_tree = cJSON_ParseWithLength(header, header_len)) == NULL)
	{
		fprintf(stderr, "Failure to initialize JSON tree\n");
		ret_code = STL_FAILURE;

		goto CLEANUP;
	}

	for (cursor = json_tree->child; cursor != NULL; cursor = cursor->next)
	{
		if (strcmp(cursor->string, "__metadata__") == 0)
		{
			continue;
		}

		tensors_total++;

		if (loadTensorFromToken(fhandle, data_file, 
			header_len + sizeof(uint64_t), &write_cursor, 
			cursor) == STL_FAILURE)
		{
			fprintf(stderr, "Failure to load %s into tensor\n",
				cursor->string);

			continue;
		}

		tensors_loaded++;
	}

	if (tensors_loaded != tensors_total)
	{
		fprintf(stderr, "Bad load, only %lu of %lu tensors loaded\n", 
			tensors_loaded, tensors_total);
		ret_code = STL_FAILURE;
	}
	else
	{
		char *tmp = cJSON_PrintUnformatted(json_tree);
		uint64_t tmp_len = (uint64_t) strlen(tmp);
		uint64_t write_len;
		size_t i;

		porteggSysToLeCopy(uint64_t, tmp_len, write_len);
		verbosePrintf("%lu of %lu tensors loaded successfully\n",
			tensors_loaded, tensors_total);

		/* Write out the length of the header, taking into account
		 * the potentially new values in the various data_range arrays
		 * and how that affects the byte-length */

		if (fwrite(&write_len, sizeof(uint64_t), 1, out_file) != 1)
		{
			fprintf(stderr, 
				"Failure to write out header length\n");
			ret_code = STL_FAILURE;
		}

		if (fwrite(tmp, sizeof(char), tmp_len, out_file) != tmp_len)
		{
			fprintf(stderr, 
				"Failure to write out entire header\n");
			ret_code = STL_FAILURE;
		}

		rewind(data_file);

		for (i = 0; i < write_cursor; i++)
		{
			fputc(getc(data_file), out_file);
		}

		verbosePrintf("%lu bytes written to output file\n",
			write_cursor + tmp_len + sizeof(uint64_t));
		free(tmp);
	}

CLEANUP:
	if (header != NULL)
	{
		free(header);
	}

	if (fhandle != NULL)
	{
		fclose(fhandle);
	}

	if (out_file != NULL)
	{
		fclose(out_file);
	}

	if (data_file != NULL)
	{
		fclose(data_file);
	}

	if (json_tree != NULL)
	{
		cJSON_Delete(json_tree);
	}

	return ret_code;
}
