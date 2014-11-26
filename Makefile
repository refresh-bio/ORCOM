.PHONY: cpp11 boost gen_fastq

all: cpp11

BIN_DIR = bin

cpp11:
	cd orcom/orcom_bin && make clean orcom_bin
	cd orcom/orcom_pack && make clean orcom_pack
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)	
	mv orcom/orcom_bin/orcom_bin $(BIN_DIR)/
	mv orcom/orcom_pack/orcom_pack $(BIN_DIR)/

boost:
	cd orcom/orcom_bin && make -f Makefile.boost clean orcom_bin
	cd orcom/orcom_pack && make -f Makefile.boost clean orcom_pack
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)	
	mv orcom/orcom_bin/orcom_bin $(BIN_DIR)/
	mv orcom/orcom_pack/orcom_pack $(BIN_DIR)/

gen_fastq:
	cd tools/gen_fastq && make
	mv tools/gen_fastq/$@ $(BIN_DIR)/

clean:
	cd orcom/orcom_bin/ && make clean
	cd orcom/orcom_pack/ && make clean
	cd tools/gen_fastq/ && make clean
	-rm -rf $(BIN_DIR)
