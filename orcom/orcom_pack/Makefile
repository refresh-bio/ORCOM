.PHONY: orcom_pack

all: orcom_pack

ifndef CXX
	CXX = g++
endif
CXX_FLAGS += -O3 -DNDEBUG -flto -fwhole-program
CXX_FLAGS += -m64 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -std=c++11
CXX_FLAGS += -DDISABLE_GZ_STREAM

CXX_OBJS =  DnarchModule.o \
	DnarchOperator.o \
	DnarchFile.o \
	BinFileExtractor.o \
	DnaCompressor.o \
	../orcom_bin/BinFile.o \
	../orcom_bin/DnaPacker.o \
	../orcom_bin/DnaParser.o \
	../orcom_bin/FileStream.o

PPMD_OBJS = ../ppmd/PPMd.o \
	../ppmd/Model.o

CXX_LIBS += -lpthread

.cpp.o:
	$(CXX) $(CXX_FLAGS) -c $< -o $@

orcom_pack: $(CXX_OBJS) $(PPMD_OBJS)
	$(CXX) $(CXX_FLAGS) -o $@ main.cpp $(CXX_OBJS) $(RLE_OBJS) $(PPMD_OBJS) $(CXX_LIBS)
	strip $@

clean:
	-rm -f $(CXX_OBJS)
	-rm -f $(PPMD_OBJS)
	-rm -f orcom_pack
