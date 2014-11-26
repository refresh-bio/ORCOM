#/bin/bash

dna2fastq="python dna_to_fastq.py"
tmp_dna_file=__tmp.dna

orcom_pack=$1/orcom_pack
in_orcom_file=$2
out_fastq_file=$3


####
#
# main
#
if [[ $# -ne 3 ]]; then
	echo "Usage: ./$( basename $0 ) <orcom_binaries_path> <in_orcom_archive> <out_fastq>"
	exit 1
fi

echo "Unpacking DNA..."
$orcom_pack d -i$in_orcom_file -o$tmp_dna_file

echo "Creating FASTQ file..."
$dna2fastq $tmp_dna_file $out_fastq_file

# cleanup
if [ -e $tmp_dna_file ]; then
	rm $tmp_dna_file
fi
