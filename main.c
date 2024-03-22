#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "fileLoading.h"
#include "portopt.h"

extern STL_BOOL verbose_output;

void printHelp(void);

int main(int argc, char **argv)
{
	const struct portoptVerboseOpt opts[] =
	{
		{'i', "input",   PORTOPT_TRUE},
		{'o', "output",  PORTOPT_TRUE},
		{'v', "verbose", PORTOPT_FALSE},
		{'h', "help",    PORTOPT_FALSE}
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
			case 'i':
				file_path = portoptGetArg(lenc, argv, &ind);
				break;
			case 'o':
				out_path = portoptGetArg(lenc, argv, &ind);
				break;
			case 'v':
				fputs("Enabling verbose output\n", stdout);
				verbose_output = STL_TRUE;
				break;
			case 'h':
				printHelp();
				return STL_SUCCESS;
			case '?':
			default: /* fallthrough */
				fputs("Unknown switch\n", stderr);
				break;
		}
	}

	if (file_path == NULL)
	{
		fputs("Please provide a valid safetensors file path "
			"to act upon with -i or --input. Pass in -h or "
			"--help for additional information\n", stderr);

		return STL_FAILURE;
	}

	if (strcmp(file_path, out_path) == 0)
	{
		fputs("In-place conversion is not currently supported, "
			"--input and --output arguments must be different\n",
			stderr);

		return STL_FAILURE;
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
		"-i, --input <FILE PATH>  : Safetensor file to be converted\n"
		"-o, --output <FILE PATH> : Desired output file name\n"
		"-v, --verbose            : Enables additional logging\n"
		"-h, --help               : Prints this help message\n\n"
		"Example invocation:\n"
		"./sdc -i ~/.models/foo.safetensors -o bar.safetensors\n",
		stdout);
}
