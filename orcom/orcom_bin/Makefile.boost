.PHONY: orcom_bin

all: orcom_bin

CXX = g++
CXX_FLAGS += -O3 -DNDEBUG -flto -fwhole-program
CXX_FLAGS += -m64 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -static
CXX_FLAGS += -DUSE_BOOST_THREAD

CXX_OBJS = BinModule.o \
	BinOperator.o \
	BinFile.o \
	DnaPacker.o \
	DnaCategorizer.o \
	DnaParser.o \
	FastqStream.o \
	FileStream.o

CXX_LIBS += -lz
CXX_LIBS += -lboost_thread -lboost_system -lpthread

.cpp.o:
	$(CXX) $(CXX_FLAGS) -c $< -o $@

orcom_bin: $(CXX_OBJS)
	$(CXX) $(CXX_FLAGS) -o $@ main.cpp $(CXX_OBJS) $(CXX_LIBS)
	strip $@

clean:
	-rm $(CXX_OBJS)
	-rm orcom_bin
