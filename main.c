#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "main.h"
#include "fileLoading.h"
#include "portopt.h"

/* When using the replace option, -R, there is a possibility that if the 
 * program were to be terminated or crash during the overwriting of the input 
 * file data could be lost.  One could write out to a .swp file and then rename
 * it to the original but this would depend upon platform specific code */

extern SDC_BOOL      verbose_output;
extern SDC_BOOL      inplace_conv;
extern enum dataType float_out;

void printHelp(void);

static enum dataType getFloatType(const char *str)
{
	if (strcmp(str, "F32") == 0)
	{
		return FLOAT_32;
	}
	else if (strcmp(str, "F16") == 0)
	{
		return FLOAT_16;
	}
	else if (strcmp(str, "BF16") == 0)
	{
		return BFLOAT_16;
	}
	else
	{
		return DTYPE_UNKNOWN;
	}
}

int main(int argc, char **argv)
{
	const struct portoptVerboseOpt opts[] =
	{
		{'R', "replace",    PORTOPT_FALSE},
		{'f', "float-type", PORTOPT_TRUE},
		{'i', "input",      PORTOPT_TRUE},
		{'o', "output",     PORTOPT_TRUE},
		{'v', "verbose",    PORTOPT_FALSE},
		{'h', "help",       PORTOPT_FALSE}
	};
	const size_t num_opts = sizeof(opts) / sizeof(opts[0]);
	const size_t lenc = (size_t) argc;
	char *file_path = NULL;
	char *out_path  = "output.safetensors";
	size_t ind = 0;
	int flag;

	while ((flag = portoptVerbose(lenc, argv, opts, num_opts, &ind)) != -1)
	{
		switch (flag)
		{
			/* 'replace' instead of 'inplace' to avoid users 
			 * inputting -I when they mean -i and losing data */
			case 'R':
				inplace_conv = SDC_TRUE;
				break;
			case 'f':
				float_out = getFloatType(portoptGetArg(lenc, 
					argv, &ind));
				break;
			case 'i':
				file_path = portoptGetArg(lenc, argv, &ind);
				break;
			case 'o':
				out_path = portoptGetArg(lenc, argv, &ind);
				break;
			case 'v':
				fputs("Enabling verbose output\n", stdout);
				verbose_output = SDC_TRUE;
				break;
			case 'h':
				printHelp();
				return SDC_SUCCESS;
			case '?':
			default: /* fallthrough */
				fputs("Unknown switch\n", stderr);
				break;
		}
	}

	if (float_out == DTYPE_UNKNOWN)
	{
		fputs("Invalid argument for --float-type, valid options are:\n"
			"'F32'  : C language float type (default)\n"
			"'F16'  : IEEE Specfication Half precision float\n"
			"'BF16' : 'Brain' Half precision float\n", stdout);

		return SDC_FAILURE;
	}
	else if (float_out != FLOAT_32)
	{
		verbosePrintf("WARNING: conversion to half precision or brain"
			" floats relies on bit fiddling and as such may not "
			"work as expected for non-IEEE compliant systems\n");
	}

	if (file_path == NULL)
	{
		fputs("Please provide a valid safetensors file path "
			"to act upon with -i or --input. Pass in -h or "
			"--help for additional information\n", stderr);

		return SDC_FAILURE;
	}

	/* XXX: This will not guard against using two different paths to 
	 * reference the same file, that would be a good thing to guard 
	 * against but it should fail with fopen */
	if (strcmp(file_path, out_path) == 0)
	{
		fputs("input and output path are identical. If in-place "
		"conversion is desired please run with the -R, --replace, "
		"command line switch\n", stderr);

		return SDC_FAILURE;
	}

	if (porteggIsLittle() == PORTEGG_FALSE)
	{
		fputs("WARNING: safetensors files are encoded to be little "
			"endian and this program was designed and tested on "
			"a little endian system. Mitigating measures have "
			"been put in place but may still be buggy on big "
			"endian systems", stdout);
	}

	return convertSafetensorFile(file_path, out_path);
}

void printHelp(void)
{
	fputs("SDC, Safetensor Dtype Converter\n\n"
		"-R, --replace                    :"
			" Replaces input file with output\n"
		"-f, --float-out {F32, F16, BF16} :"
			" Float output type, default F32\n"
		"-i, --input  <FILE PATH>         :"
			" Safetensor file to be converted\n"
		"-o, --output <FILE PATH>         :"
			" Desired output file name\n"
		"-v, --verbose                    :"
			" Enables additional logging\n"
		"-h, --help                       :"
			" Prints this help message\n\n"
		"Example invocation:\n"
		"./sdc -i ~/.models/foo.safetensors -o bar.safetensors\n",
		stdout);
}
