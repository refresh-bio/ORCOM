#!/bin/bash

orcom_bin=./orcom_bin
orcom_pack=./orcom_pack

bin_tmp_prefix=__tmp.bin

bin_threads=8
sig_len=8
skip_zone=12
block_size=256

pack_threads=8
enc_th=50

in_fastq=$1
out_orcom=$2


if [[ ! $# -eq 2 ]]; then
	echo "Usage: ./$( basename $0 )  <in_fastq_file> <out_orcom_file>"
	exit 1
fi

echo "Binning..."
$orcom_bin e -i$in_fastq -o$bin_tmp_prefix -p$sig_len -s$skip_zone -b$block_size -t$bin_threads

echo "Packing..."
$orcom_pack e -i$bin_tmp_prefix -o$out_orcom -e$enc_th -t$pack_threads

echo "Cleaning..."
rm $bin_tmp_prefix*

echo "Done!"
