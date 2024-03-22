/* License information at EOF */
#ifndef PORTEGG_H 
#define PORTEGG_H 
/* Have you read Gulliver's Travels? */

/* If you are using a truly exotic endian format like Honeywell you are on your
 * own for the time being, sorry */

#include <stddef.h> /* for size_t */

#define PORTEGG_BOOL    char
#define PORTEGG_TRUE    1
#define PORTEGG_FALSE   0

/* No promises that this works on all systems, can be forced if neccessary */
static PORTEGG_BOOL porteggIsLittle(void)
{
#ifdef PORTEGG_BIG_ENDIAN_SYSTEM
	return PORTEGG_FALSE;
#else
#ifdef PORTEGG_LITTLE_ENDIAN_SYSTEM
	return PORTEGG_TRUE;
#else
	int num = 1;

	return (*((char *) &num) == 1) ? PORTEGG_TRUE : PORTEGG_FALSE;
#endif /* little check */
#endif /* big check */
}

static char* porteggReverseBytes(const size_t len, char *bytes)
{
	size_t i;

	for (i = 0; i < len >> 1; i++)
	{
		const char tmp = bytes[len - 1 - i];

		bytes[len - 1 - i] = bytes[i];
		bytes[i] = tmp;
	}

	return bytes;
}

/* As below but acts upon byte buffers of discrete length */
#define PORTEGG_LE_TO_SYS_RAW(len, bytes)              \
	((porteggIsLittle() == PORTEGG_TRUE)           \
		? (bytes)                              \
		: porteggReverseBytes((len), (bytes)))

#define PORTEGG_BE_TO_SYS_RAW(len, bytes)              \
	((porteggIsLittle() == PORTEGG_FALSE)          \
		? (bytes)                              \
		: porteggReverseBytes((len), (bytes)))

#define PORTEGG_SYS_TO_LE_RAW(len, bytes)              \
	((porteggIsLittle() == PORTEGG_TRUE)           \
		? (bytes)                              \
		: porteggReverseBytes((len), (bytes))) 

#define PORTEGG_SYS_TO_BE_RAW(len, bytes)              \
	((porteggIsLittle() == PORTEGG_FALSE)          \
		? (bytes)                              \
		: porteggReverseBytes((len), (bytes))) 

/* These may be called directly but they will modify the variable in place 
 * in memory, for this reason they also need to act upon lvalues */
#define PORTEGG_SYS_TO_LE(type, val) \
	(*((type *) (PORTEGG_SYS_TO_LE_RAW(sizeof(type), (char *) &(val)))))

#define PORTEGG_SYS_TO_BE(type, val) \
	(*((type *) (PORTEGG_SYS_TO_BE_RAW(sizeof(type), (char *) &(val)))))

#define PORTEGG_LE_TO_SYS(type, val) \
	(*((type *) (PORTEGG_LE_TO_SYS_RAW(sizeof(type), (char *) &(val)))))

#define PORTEGG_BE_TO_SYS(type, val) \
	(*((type *) (PORTEGG_BE_TO_SYS_RAW(sizeof(type), (char *) &(val)))))

/* These create a local copy that will then be copied to out, it will not 
 * modify "in" in-place and in may be an r-value */
#define porteggLeToSysCopy(type, in, out)             \
do                                                    \
{                                                     \
	type portegg_tmp = (in);                      \
	(out) = PORTEGG_LE_TO_SYS(type, portegg_tmp); \
} while (0)

#define porteggBeToSysCopy(type, in, out)             \
do                                                    \
{                                                     \
	type portegg_tmp = (in);                      \
	(out) = PORTEGG_BE_TO_SYS(type, portegg_tmp); \
} while (0)

#define porteggSysToLeCopy(type, in, out)             \
do                                                    \
{                                                     \
	type portegg_tmp = (in);                      \
	(out) = PORTEGG_SYS_TO_LE(type, portegg_tmp); \
} while (0)

#define porteggSysToBeCopy(type, in, out)             \
do                                                    \
{                                                     \
	type portegg_tmp = (in);                      \
	(out) = PORTEGG_SYS_TO_BE(type, portegg_tmp); \
} while (0)

#endif /* PORTEGG_H */

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
