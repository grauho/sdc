# SDC, Safetensors Dtype Converter

This is a simple utility for converting a .safetensors file that contains
types not handled by GGML, eg: 64-bit floats or 64-bit integers, into a file
where those same values are converted and re-encoded to a smaller float type.
By default a 32-bit "full precision" floating point type is used but IEEE 
16-bit "half precision" floats or 16-bit "Brain" floats may be used instead.

This is a quick and dirty solution to the problem and shouldn't be relied upon
for critical applications.

## Building

An at least C99 compliant compiler is required to build this software, or for 
the adventurous the program can be compiled in C90/C89 compliant mode if 
alternative definitions for the stdint types are provided along with other
minor adjustments.

Currently only a POSIX makefile is included for automated building:

``` shell
make
```

Should all else fail one may simply build the object files and link them 
manually like in the bad old days:

``` shell
cc -Wall -pedantic -O2 -Wno-unused-function -c -o main.o main.c
cc -Wall -pedantic -O2 -Wno-unused-function -c -o cJSON.o cJSON.c
cc -Wall -pedantic -O2 -Wno-unused-function -c -o fileLoading.o fileLoading.c
cc -Wall -pedantic -O2 -Wno-unused-function -c -o converting.o converting.c
cc -Wall -pedantic -O2 -Wno-unused-function -o sdc main.o cJSON.o fileLoading.o converting.o
```

Notes:

Should you find you get the "Big-Endian" warning despite knowing you are on a
little-endian system you may force the portegg header library to recognize this
by either defining PORTEGG\_LITTLE\_ENDIAN\_SYSTEM either through a compile 
time argument or simply in the source code before portegg.h is included in 
main.h.

## Command Line Options

    -R, --replace                    : Converts file in-place, -o is ignored
    -f, --float-out {F32, F16, BF16} : Desired float dtype output type
    -i, --input  <FILE PATH>         : The safetensors file to be converted
    -o, --output <FILE PATH>         : The desired output file 
    -v, --verbose                    : Prints more logging information
    -h, --help                       : Prints a help message much like this one

Notes: 

* Should no output file be specified the results will be outputted to a new
filed named output.safetensors

* be sure to include .safetensors at the end of your desired output name
if you want it to be automatically recognized by other programs as such

* When using the replace option, -R, there is a possibility that if the 
program were to be terminated or crash during the overwriting of the input file 
data could be lost. One could write out to a .swp file and then rename it to 
the original but this would depend upon platform specific code

* Non-C language float data types, F16 and BF16, rely on some bit fiddling to 
convert down into, as such if running on a system that does not use the IEEE 
standardized number of bits for the fraction, mantissa, and exponent the 
resulting values may be meaningless. 

## Example Invocation

``` shell
./sdc -i ../../foo.safetensors -o bar.safetensors --verbose -f F32
```

## Bugs

* There is handling for endianess via the portegg header but this program was
developed and tested on a little-endian system so there is no guarantee that
it will work as expected.

* Especially large files may run into issues with fseek and it's historical
limitations, although in the authors experience it seems to manage on my 
Linux machine alright. The program will warn you if this is a possibility.

* If the same file is given for both --input and --output using different 
relative paths bad things may happen, but it should just die with a fread
failure.

## License

License information can be found in detail in LICENSE.txt but in short this
program is licensed by me, grauho, under the BSD 4-Clause license while cJSON 
is licensed by it's authors under the MIT license, copies of both in their 
entirety may be found in the aforementioned file.
