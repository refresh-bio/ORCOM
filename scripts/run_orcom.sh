#!/bin/bash

bin_tmp_prefix=__tmp.bin

orcom_bin=$1/orcom_bin
orcom_pack=$1/orcom_pack

in_fastq=$2
out_orcom=$3


####
#
# main
#
if [[ ! $# -eq 3 ]]; then
	echo "Usage: ./$( basename $0 ) <orcom_binaries_path> <in_fastq_file> <out_orcom_archive>"
	exit 1
fi

echo "Binning..."
$orcom_bin e -i$in_fastq -o$bin_tmp_prefix

echo "Packing..."
$orcom_pack e -i$bin_tmp_prefix -o$out_orcom

echo "Cleaning..."
rm $bin_tmp_prefix*

echo "Done!"
