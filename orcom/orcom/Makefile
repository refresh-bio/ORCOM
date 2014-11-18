.PHONY: orcom

all: orcom

CXX = g++
CXX_FLAGS += -O3 -DNDEBUG -flto -fwhole-program
CXX_FLAGS += -m64 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE

CXX_LIBS += -lpthread -lz

# BOOST is needed in order to compile ORCOM statically
CXX_FLAGS += -DUSE_BOOST_THREAD -static
CXX_LIBS += -lboost_thread -lboost_system

CXX_OBJS = ../orcom_bin/BinModule.o \
	../orcom_bin/BinOperator.o \
	../orcom_bin/BinFile.o \
	../orcom_bin/DnaPacker.o \
	../orcom_bin/DnaCategorizer.o \
	../orcom_bin/DnaParser.o \
	../orcom_bin/FastqStream.o \
	../orcom_bin/FileStream.o \
	../orcom_pack/DnarchModule.o \
	../orcom_pack/DnarchOperator.o \
	../orcom_pack/DnarchFile.o \
	../orcom_pack/BinFileExtractor.o \
	../orcom_pack/DnaCompressor.o \

PPMD_OBJS = ../ppmd/PPMd.o \
	../ppmd/Model.o

.cpp.o:
	$(CXX) $(CXX_FLAGS) -c $< -o $@

orcom: $(CXX_OBJS) $(PPMD_OBJS)
	$(CXX) $(CXX_FLAGS) -o $@ main.cpp $(CXX_OBJS) $(RLE_OBJS) $(PPMD_OBJS) $(CXX_LIBS)
	strip $@

clean:
	-rm $(CXX_OBJS)
	-rm $(PPMD_OBJS)
	-rm orcom
