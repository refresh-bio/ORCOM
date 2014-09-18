#!/bin/bash

script=`basename $0`
if [[ $# -ne 2 ]]; then
    echo "Usage: ./$script <dir_with_gzipped_fastq_files> <output_dna_file>"
    exit 1
fi


gz_dir=$1
dna_file=$2

if [ -e $dna_file ]; then
    rm $dna_file
fi

for f in $gz_dir/*.fastq.gz;do
	gunzip -c $f | awk '0 == (NR + 2) % 4' >> $dna_file
done
