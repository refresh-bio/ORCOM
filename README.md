# ORCOM
ORCOM - Overlapping Reads COmpression with Minimizers is a set of 2 tools designed for an effective and high-performance compression of DNA sequencing reads, which consist of:
* _orcom\_bin_ - performing a DNA reads clusterization into bins,
* _orcom\_pack_ - performing compression of the DNA reads stored in bins.

Currently, ORCOM works with FASTQ files and supports only compression of DNA stream, discarding the read names and qualities. By compressing only the DNA stream, it can reduce the H. sapiens reads (ERA015743) composed of 1.34G reads of lengths 100–102 from 136GB to 5.5GB, achieving compression ratio of 0.327 bits per base.

For more information please check out the [official website](http://sun.aei.polsl.pl/orcom/).


# Building

## Pre-built binaries

Pre-built binaries for Linux platform can be downloaded from the [official website](http://sun.aei.polsl.pl/orcom/download.html).


## Build prerequisites

ORCOM currently provides Makefiles for building on Linux platform, however it should also be able to be compiled on Windows and/or MacOSX platforms. The only one prequisite is the availability of the _zlib_ library in the system.

ORCOM binaries can be compiled in two ways - depending on the selection of multithreading support library, where for each a different makefile file is provided in subproject directories. In the first (default) case, threading support from _C++11_ standard will be used (g++ version >= 4.8). In the second case the _boost::threads_ library will be used, which is needed to be present on the build system.

By default, binaries are compiled using _g++_, however compiling using _Clang_ or _Intel icpc_ should be also possible.


## Compiling

To compile ORCOM using _g++_ >= 4.8 with _C++11_ standard and dynamic linking, in the main directory type:
    
    make

To compile ORCOM using _boost::threads_ with static linking:

    make boost

The resulting _orcom\_bin_ and _orcom\_pack_ binaries will be placed in _bin_ subdirectory.


However, to compile each subprogram separately, use the makefile files provided in each of subprograms directory.


# Usage

DNA stream compression using ORCOM is a 2 stage process, consisting of running _orcom\_bin_ and _orcom\_pack_ subprograms in chain. However, to decompress the DNA stream, only running _orcom\_pack_ is needed.

## _orcom\_bin_

_orcom\_bin_ performs DNA records clustering into separate bins representing signatures. As an input it takes a single or a set of FASTQ files and stores the output to two separate files with binned records: `*.bdna` file, containing encoded DNA stream, and `*.bmeta file`, containing archive meta-information.

### Command line
_orcom\_bin_ is run from the command prompt:

    orcom_bin <e|d> [options]

in one of the two modes:
* `e` - encoding,
* `d` - decoding,

with available options:
* `-i<file>` - input file,
* `-f"<f1> <f2> ... <fn>"` - input file list,
* `-g` - input compressed in `.gz` format,
* `-o<f>` - output files prefix,
* `-p<n>` - signature length, default: `8`,
* `-s<n>` - skip-zone length, default: `12`,
* `-b<n>` - FASTQ input buffer size (in MB), default: `256`,
* `-t<n>` -  worker threads number, default: `8`,
* `-v` - verbose mode, default: `false`.


The parameters `-p<value>` and `-s<value>` concern the records clusterization process and signature selection. The parameter `-b<value>` concern the bins sizes before and after clusterization — the FASTQ buffer size should be set as large as possible in order to achieve best ratio (at the cost of large memory consumption). The parameter `-t<value>` sets total number of processing threads (not including two I/O threads).


### Examples
Encode (cluster) reads from `NA19238.fastq` file, using signtature length of `6` and skip-zone length of `6`, `4` processing threads with `256` MB FASTQ block buffer, saving output to `NA19238.bin` bin files:

    orcom_bin e -iNA19238.fastq -oNA19238.bin -t4 -b256 -p6 -s6 

Encode reads from `NA19238_1.fastq` and `NA19238_2.fastq` files saving output to `NA19238.bin` bin files:

    orcom_bin e -f”NA19238_1.fastq NA19238_2.fastq” -oNA19238.bin 
    
Encode reads from gzip-compressed FASTQ files (`-g`) in the current directory using `8` processing threads and saving the output to `NA19238.bin` bin files:

    orcom_bin e -f”$( ls *.fastq.gz )” -oNA19238.bin -g -t8
    
Decode reads from `NA19238.bin` bin files and save the DNA reads to `NA19238.dna` file.

    orcom_bin d -iNA19238.bin -oNA19238.dna



## _orcom\_pack_

_orcom\_pack_ performs DNA records compression. As an input it takes files produced by _orcom\_bin_: `*.bdna` and `*.bmeta` and it generates two output files: `*.cdna` file, containing compressed streams and `*.cmeta` file, containing archive meta-information.

### Command line

_orcom\_pack_ is run from the command prompt:

    orcom_pack <e|d> [options]

in one of the two modes:
* `e` - encoding,
* `d` - decoding,

with available options:
* `-i<file>` - _orcom\_bin_ generated bin files prefix,
* `-o<file>` - output files prefix,
* `-e<n>` - encode threshold value, default: `0` (0 - auto),
* `-m<n>` - mismatch cost, default: `2`,
* `-s<n>` - insert cost, default: `1`,
* `-t<n>` - threads count, default: `8`,
* `-v` - verbose mode, default: `false`.


The parameters `-e<value>`, `-m<value>` and `-s<value>` concern the records internal encoding step, where encoding threshold value should be adapted to the dataset records’ length. The parameter `-t<value>` sets total number of processing threads (not including two I/O threads).


## Examples

Encode (compress) clustered reads from `NA19238.bin` bin files using `4` processing threads and save the output to `NA19238.orcom` archive files:

    orcom_pack e -iNA19238.bin -oNA19238.orcom -t4
    
Encode clustered reads from `NA19238.bin` bin files setting the read matching parameters of insert const to `2`, mismatch cost to `1` and encoding threshold to `40`, and saving the result to `NA19238.orcom` archive files:

    orcom_pack e -iNA19238.bin -oNA19238.orcom -s2 -m1 -e40 

Decode (decompress) reads from `NA19238.orcom` archive saving the DNA reads to `NA19238.dna` file:

    orcom_pack d -iNA19238.orcom -oNA19238.dna
