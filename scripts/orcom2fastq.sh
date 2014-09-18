#/bin/bash

# toolset config
orcom_pack=./orcom_pack
dna2fastq="python dna2fastq.py"
tmp_dna_file=__tmp.dna

# params
in_orcom_file=$1
out_fastq_file=$2


####
#
# main
#
if [[ $# -ne 3 ]]; then
	echo "Usage: ./$( basename $0 ) <input_orcom_filenames_prefix> <output_fastq_filename>"
	exit 1
fi

# decompress ORCOM packed DNA stream
$orcom_pack d $in_orcom_file $tmp_dna_file

# generate FASTQ file with decompressed DNA stream
$dna2fastq $tmp_dna_file $out_fastq_file

# cleanup
if [ -e $tmp_dna_file ]; then
	rm $tmp_dna_file
fi
