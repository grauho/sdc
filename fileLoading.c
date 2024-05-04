#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <float.h>

#include "converting.h"
#include "fileLoading.h"
#include "cJSON.h"

/* TODO:
 * 	- Better float bounds checking 
 */

SDC_BOOL verbose_output = SDC_FALSE;
SDC_BOOL inplace_conv   = SDC_FALSE;
extern enum dataType float_out;

void verbosePrintf(const char *fmt, ...)
{
	if (verbose_output == SDC_TRUE)
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
		fprintf(stderr, "%s: Bad file handle argument\n", __func__);

		return NULL;
	}

	if ((slurp = malloc(sizeof(char) * header_len)) == NULL)
	{
		fprintf(stderr, "%s: Failure to allocated header buffer\n",
			__func__);

		return NULL;
	}

	if ((bytes_read = fread(slurp, sizeof(char), header_len, fhandle))
		!= header_len)
	{
		fprintf(stderr, "%s: Incomplete read, %lu / %lu\n",
			__func__, bytes_read, header_len);
		free(slurp);

		return NULL;
	}

	verbosePrintf("%s: %lu bytes read\n", __func__, bytes_read);

	return slurp;
}

static enum dataType extractDataType(const struct cJSON *target)
{
	size_t i;

	for (i = 0; i < dtype_info_len; i++)
	{
		if (strcmp(dtype_info[i].name, target->valuestring) == 0)
		{
			return dtype_info[i].dtype;
		}
	}

	return DTYPE_UNKNOWN;
}

static char* extractRawData(FILE *fhandle, const struct cJSON *src,
	const size_t binary_start, size_t *data_range)
{
	static SDC_BOOL large_seek_warned = SDC_FALSE;
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
		fprintf(stderr, "%s: Failure determining tensor data range\n",
			__func__);

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
		fprintf(stderr, "%s: Malloc failure for data\n", __func__);

		return NULL;
	}

	if ((binary_start + data_range[0] > LONG_MAX)
	&& (large_seek_warned == SDC_FALSE))
	{
		fprintf(stderr, "%s: Attempting to seek to a range outside of "
			"the historical capacity of fseek, if output doesn't "
			"work this is the place to check first\n", __func__);

		large_seek_warned = SDC_TRUE;
	}

	if (fseek(fhandle, binary_start + data_range[0], SEEK_SET) != 0)
	{
		fprintf(stderr, "%s: Bad file seek\n", __func__);
		free(arr);

		return NULL;
	}

	if (fread(arr, sizeof(char), data_len, fhandle) != data_len)
	{
		fprintf(stderr, "%s: Bad read from file\n", __func__);
		free(arr);

		return NULL;
	}

	return arr;
}

static char* rawDataArrayEndianness(char *data, const size_t len, 
	const enum dataType dtype, SDC_BOOL is_outgoing)
{
	size_t i;

	if (dtype > dtype_info_len)
	{
		fprintf(stderr, "Unknown dtype %d\n", dtype);

		return data;
	}

	for (i = 0; i < len; i++)
	{
		if (is_outgoing == SDC_TRUE)
		{
			PORTEGG_SYS_TO_LE_RAW(dtype_info[dtype].size, 
				data + (i * dtype_info[dtype].size));	
		}
		else
		{
			PORTEGG_LE_TO_SYS_RAW(dtype_info[dtype].size, 
				data + (i * dtype_info[dtype].size));	
		}
	}

	return data;
}

static SDC_STAT loadTensorFromToken(FILE *fhandle, FILE *data_file, 
	const size_t binary_start, size_t *write_cursor, 
	const struct cJSON *json_cursor)
{
	enum dataType dtype     = DTYPE_UNKNOWN;
	enum dataType out_dtype = DTYPE_UNKNOWN;
	struct cJSON *data_obj  = NULL;
	struct cJSON *dtype_obj = NULL;
	struct cJSON *arr_obj   = NULL;
	char *data              = NULL;
	size_t data_range[2]    = {0};
	size_t data_len         = 0;
	size_t bytes_out, write_tmp;

	if ((fhandle == NULL) || (data_file == NULL) || (json_cursor == NULL))
	{
		fprintf(stderr, "%s: Invalid Arguments\n", __func__);

		return SDC_FAILURE;
	}

	if (((dtype_obj = cJSON_GetObjectItemCaseSensitive(
		json_cursor, "dtype")) == NULL)
	|| ((data_obj = cJSON_GetObjectItemCaseSensitive(
		json_cursor, "data_offsets")) == NULL))
	{
		fprintf(stderr, "%s: Cannot access tensor object members\n",
			__func__);

		return SDC_FAILURE;
	}

	dtype = extractDataType(dtype_obj);
	data = extractRawData(fhandle, data_obj, binary_start, data_range);

	if (data == NULL)
	{
		fprintf(stderr, "%s: Failure to extract raw tensor data\n",
			__func__);

		return SDC_FAILURE;
	}

	data_len = data_range[1] - data_range[0];
	data = rawDataArrayEndianness(data, data_len, dtype, SDC_FALSE);
	
	/*
	if (dtype == FLOAT_64 || dtype == SIGNED_64)
	*/
	if ((dtype < UNSIGNED_8) /* ie: all floats, all signed ints */
	&& (dtype != float_out))
	{
		const size_t num_items = data_len / dtype_info[dtype].size;
		void *tmp = downConvertDTypes(data, num_items, dtype, 
			&out_dtype);

		if (tmp == NULL)
		{
			fprintf(stderr, "Bad down conversion\n");
			free(data);

			return SDC_FAILURE;
		}

		free(data);
		data = (char *) tmp;
		data_len = num_items * dtype_info[out_dtype].size;
		cJSON_SetValuestring(dtype_obj, dtype_info[out_dtype].name);
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
		fprintf(stderr, "%s: Cannot access tensor data_range array\n",
			__func__);
		free(data);

		return  SDC_FAILURE;
	}

	porteggSysToLeCopy(size_t, *write_cursor, write_tmp);
	cJSON_SetNumberValue(arr_obj, write_tmp);

	if ((arr_obj = cJSON_GetArrayItem(data_obj, 1)) == NULL)
	{
		fprintf(stderr, "%s: Cannot access tensor data_range array\n",
			__func__);
		free(data);

		return  SDC_FAILURE;
	}

	porteggSysToLeCopy(size_t, *write_cursor + data_len, write_tmp);
	cJSON_SetNumberValue(arr_obj, write_tmp);
	data = rawDataArrayEndianness(data, data_len, dtype, SDC_TRUE);

	if ((bytes_out = fwrite(data, sizeof(char), data_len, data_file)) 
		!= data_len)
	{
		fprintf(stderr, "%s: Incomplete write to file (%lu / %lu)\n", 
			__func__, bytes_out, data_len);
		free(data);

		return SDC_FAILURE;
	}

	*write_cursor += data_len;
	free(data);

	return SDC_SUCCESS;
}


SDC_STAT convertSafetensorFile(const char *file_path, const char *out_path)
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
	SDC_STAT ret_code = SDC_SUCCESS;

	if (((fhandle  = fopen(file_path, "rb")) == NULL)
	|| ((inplace_conv == SDC_FALSE) 
		&& ((out_file  = fopen(out_path,  "w"))  == NULL))
	|| ((data_file = tmpfile()) == NULL))
	{
		fprintf(stderr, "%s: Failure to open file '%s'\n", 
			__func__, file_path);
		ret_code = SDC_FAILURE;

		goto CLEANUP;
	}

	if (fread(&header_len, sizeof(uint64_t), 1, fhandle) != 1)
	{
		fprintf(stderr, "%s: Failure to read from file '%s'\n", 
			__func__, file_path);
		ret_code = SDC_FAILURE;

		goto CLEANUP;
	}

	header_len = PORTEGG_LE_TO_SYS(uint64_t, header_len);
	verbosePrintf("Reading header of length: %lu\n", header_len);

	if ((header = slurpHeader(fhandle, header_len)) == NULL)
	{
		fprintf(stderr, "%s: Failure to slurp header\n", __func__);
		ret_code = SDC_FAILURE;

		goto CLEANUP;
	}

	if ((json_tree = cJSON_ParseWithLength(header, header_len)) == NULL)
	{
		fprintf(stderr, "%s: Failure to initialize JSON tree\n",
			__func__);
		ret_code = SDC_FAILURE;

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
			cursor) == SDC_FAILURE)
		{
			fprintf(stderr, "%s: Failure to load %s into tensor\n",
				__func__, cursor->string);

			continue;
		}

		tensors_loaded++;
	}

	if (tensors_loaded != tensors_total)
	{
		fprintf(stderr, 
			"%s: Incomplete load, (%lu / %lu) tensors loaded\n", 
			__func__, tensors_loaded, tensors_total);
		ret_code = SDC_FAILURE;
	}
	else
	{
		FILE *tmp_handle = NULL;
		char *tmp = cJSON_PrintUnformatted(json_tree);
		uint64_t tmp_len = (uint64_t) strlen(tmp);
		uint64_t write_len;
		size_t i;

		porteggSysToLeCopy(uint64_t, tmp_len, write_len);
		verbosePrintf("%lu of %lu tensors loaded successfully\n",
			tensors_loaded, tensors_total);

		if (inplace_conv == SDC_FALSE)
		{
			tmp_handle = out_file;
		}
		else
		{
			if (fclose(fhandle) == EOF) 
			{
				/* To prevent another attempt which is UB */
				fhandle = NULL;
				fprintf(stderr, "Bad close on input file\n");

				goto CLEANUP;
			}

			fhandle = NULL;

			if ((tmp_handle = fopen(file_path, "w")) == NULL)
			{
				fprintf(stderr, "Failed to open input file "
					"'%s' for overwriting\n", file_path);

				goto CLEANUP;
			}
		}

		/* Write out the length of the header, taking into account
		 * the potentially new values in the various data_range arrays
		 * and how that affects the byte-length */
		if (fwrite(&write_len, sizeof(uint64_t), 1, tmp_handle) != 1)
		{
			fprintf(stderr, 
				"%s: Failure to write out header length\n",
				__func__);
			ret_code = SDC_FAILURE;
		}

		if (fwrite(tmp, sizeof(char), tmp_len, tmp_handle) != tmp_len)
		{
			fprintf(stderr, 
				"%s: Failure to write out entire header\n",
				__func__);
			ret_code = SDC_FAILURE;
		}

		rewind(data_file);

		for (i = 0; i < write_cursor; i++)
		{
			fputc(getc(data_file), tmp_handle);
		}

		verbosePrintf("%lu bytes written to output file\n",
			write_cursor + tmp_len + sizeof(uint64_t));

		if (inplace_conv == SDC_TRUE)
		{
			fclose(tmp_handle);
		}

		free(tmp);
	}

	dumpTypeInfo();

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

