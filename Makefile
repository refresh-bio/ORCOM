.PHONY: bin pack gen_fastq

all: bin pack gen_fastq

BIN_DIR = bin

bin:
	cd orcom/orcom_$@ && make clean orcom_$@
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)	
	cp orcom/orcom_$@/orcom_$@ $(BIN_DIR)/

pack:
	cd orcom/orcom_$@ && make clean orcom_$@
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)	
	cp orcom/orcom_$@/orcom_$@ $(BIN_DIR)/

gen_fastq:
	cd tools/gen_fastq && make
	cp tools/gen_fastq/$@ $(BIN_DIR)/

clean:
	cd orcom/orcom_bin/ && make clean
	cd orcom/orcom_pack/ && make clean
	cd tools/gen_fastq/ && make clean
	-rm -rf $(BIN_DIR)
