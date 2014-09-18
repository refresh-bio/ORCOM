#!/bin/bash

script=`basename $0`

if [[ $# -ne 2 ]]; then
    echo "Usage: ./$script <input_fastq_file> <output_dna_file>"
    exit 1
fi

awk '0 == (NR + 2) % 4' <$1 >$2
