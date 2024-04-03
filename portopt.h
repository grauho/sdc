/* License information at EOF */
#ifndef PORTOPT_H 
#define PORTOPT_H

/* Both freestanding C90 compatable */
#include <stddef.h> /* for NULL and size_t */
#include <stdarg.h>

#define PORTOPT_BOOL  unsigned char
#define PORTOPT_TRUE  1
#define PORTOPT_FALSE 0

/* Checks if a given ASCII encoded character is a number */
#define PORTOPT_IS_NUM(x) \
	((((x) >= '0') && ((x) <= '9')) ? PORTOPT_TRUE : PORTOPT_FALSE)

#ifndef PORTOPT_DISABLE_LOGGING
/* Not freestanding C90 compatable, but disablable */
#include <stdio.h>
#endif

static void portoptLog(const char *fmt, ...)
{
#ifndef PORTOPT_DISABLE_LOGGING
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
#else
	(void) fmt;
#endif
}

struct portoptVerboseOpt
{
	const char abrv_opt;
	const char *verbose_opt;
	const PORTOPT_BOOL takes_arg;
};

static int portoptCmp(const char *left, const char *right)
{
	for (; (*left) == (*right) && (*left != '\0'); left++, right++);

	return (* (unsigned char *)left) - (* (unsigned char *)right);
}

static PORTOPT_BOOL portoptValidateNumber(char *str)
{
	do
	{
		/* don't worry, this will fail immediately for '\0' */
		if (PORTOPT_IS_NUM(*str) == PORTOPT_FALSE)
		{
			return PORTOPT_FALSE;
		}
	} while (*(++str) != '\0');

	return PORTOPT_TRUE;
}

/* Returns NULL so should the user want to crash on a particular switch not
 * getting that arg that they expect they can */
static char* portoptGetArg(const size_t argc, char **argv, size_t *ind)
{
	if ((argv == NULL) || (ind == NULL) || (*ind >= argc))
	{
		portoptLog("PORTOPT: Bad arguments passed to portoptGetArg\n");

		return NULL;
	}

	return ((argv[*ind][0] == '-') 
	&& (portoptValidateNumber(&(argv[*ind][1])) == PORTOPT_FALSE))
		? NULL : argv[(*ind)++];
}

static PORTOPT_BOOL portoptCmpVerbose(const char *foo, 
	const struct portoptVerboseOpt opt)
{
	return (portoptCmp(&foo[2], opt.verbose_opt) == 0) 
		? PORTOPT_TRUE : PORTOPT_FALSE;
}

static PORTOPT_BOOL portoptCmpAbrv(const char *foo, 
	const struct portoptVerboseOpt opt)
{
	return (foo[1] == opt.abrv_opt) ? PORTOPT_TRUE : PORTOPT_FALSE;
}

static int portoptVerbose(const size_t argc, char **argv, 
	const struct portoptVerboseOpt *options, const size_t num_opts, 
	size_t *ind)
{
	size_t i, j;
	PORTOPT_BOOL (*CmpFunc)
		(const char*, const struct portoptVerboseOpt);

	if ((ind != NULL) && (argv != NULL) && (options != NULL))
	{
		for (i = *ind; i < argc; i++)
		{
			if ((argv[i] == NULL) || (argv[i][0] != '-'))
			{
				continue;
			}

			*ind = i + 1;
			CmpFunc = (argv[i][1] == '-') 
				? portoptCmpVerbose 
				: portoptCmpAbrv;

			for (j = 0; j < num_opts; j++)
			{
				if (CmpFunc(argv[i], options[j]) 
					== PORTOPT_TRUE)
				{
					return options[j].abrv_opt;
				}
			}

			portoptLog("PORTOPT: Unknown switch '%s'\n", argv[i]);

			return '?';
		}
	}

	return -1;
}

static int portopt(const size_t argc, char **argv, const char *options, 
	size_t *ind)
{
	size_t i, j;

	if ((ind != NULL) && (argv != NULL) && (options != NULL))
	{
		for (i = *ind; i < argc; i++)
		{
			if ((argv[i] != NULL) 
			&& ((argv[i][0] == '-') && (argv[i][1] != ':')))
			{
				*ind = i + 1;

				for (j = 0; options[j] != '\0'; j++)
				{
					if ((argv[i][1] == options[j])
					&& (argv[i][2] == '\0'))
					{
						return options[j];
					}
				}

				portoptLog("PORTOPT: Unknown switch '%s'\n", 
					argv[i]);

				return '?';
			}
		}
	}

	return -1;
}

#endif /* PORTOPT_H */

/*
BSD 4-Clause License
Copyright (c) 2024, grauho <grauho@proton.me> All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice, 
    this list of conditions and the following disclaimer in the documentation 
    and/or other materials provided with the distribution.

    All advertising materials mentioning features or use of this software must 
    display the following acknowledgement: This product includes software 
    developed by the <copyright holder>.

    Neither the name of the <copyright holder> nor the names of its 
    contributors may be used to endorse or promote products derived from this 
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> AS IS AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
