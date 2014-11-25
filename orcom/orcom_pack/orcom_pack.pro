TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DDISABLE_GZ_STREAM
#QMAKE_CXXFLAGS += -DUSE_BOOST_THREAD
QMAKE_CXXFLAGS += -std=c++0x

LIBS += -lpthread
#LIBS += -lboost_thread -lboost_system


HEADERS += \
    ../orcom_bin/utils.h \
    ../orcom_bin/Globals.h \
    ../orcom_bin/FileStream.h \
    ../orcom_bin/DnaParser.h \
    ../orcom_bin/DnaPacker.h \
    ../orcom_bin/DataStream.h \
    ../orcom_bin/Buffer.h \
    ../orcom_bin/BitMemory.h \
    ../orcom_bin/BinFile.h \
    BinFileExtractor.h \
    DnaCompressor.h \
    DnarchFile.h \
    DnarchModule.h \
    DnarchOperator.h \
    ../ppmd/PPMd.h \
    ../ppmd/Coder.hpp \
    ../ppmd/PPMd.h \
    ../ppmd/PPMdType.h \
    ../ppmd/Stream.hpp \
    ../ppmd/SubAlloc.hpp \
    ../rle/RleEncoder.h \
    ../rc/RangeCoder.h \
    ../rc/SymbolCoderRC.h \
    ../rc/ContextEncoder.h \
    CompressedBlockData.h \
    Params.h \
    main.h

SOURCES += \
    ../orcom_bin/FileStream.cpp \
    ../orcom_bin/DnaParser.cpp \
    ../orcom_bin/DnaPacker.cpp \
    ../orcom_bin/BinFile.cpp \
    BinFileExtractor.cpp \
    main.cpp \
    DnaCompressor.cpp \
    DnarchFile.cpp \
    DnarchModule.cpp \
    DnarchOperator.cpp \
    ../ppmd/Model.cpp \
    ../ppmd/PPMd.cpp
