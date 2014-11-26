#!/usr/bin/python

import sys

if len(sys.argv) != 3 or sys.argv[2] == sys.argv[1]:
	print "usage: dna_to_fastq.py <input_dna_filename> <output_fastq_filename>"
	exit(1)

outfile = open(sys.argv[2], 'w')

rec_count = 0
for dna in open(sys.argv[1]):
	qua = "H" * (len(dna) - 1)
	outfile.write("@TAG.%d\n%s+\n%s\n" % (rec_count, dna, qua))
	rec_count += 1

print "Done!"
print "Written %d FASTQ records" % rec_count
