TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lboost_thread -lboost_system
LIBS += -lpthread -lz

QMAKE_CXXFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
QMAKE_CXXFLAGS += -DUSE_BOOST_THREAD


HEADERS += \
    ../orcom_bin/Utils.h \
    ../orcom_bin/Globals.h \
    ../orcom_bin/FileStream.h \
    ../orcom_bin/DnaParser.h \
    ../orcom_bin/DnaPacker.h \
    ../orcom_bin/DataStream.h \
    ../orcom_bin/Buffer.h \
    ../orcom_bin/BitMemory.h \
    ../orcom_bin/BinFile.h \
    ../orcom_bin/FastqStream.h \
    ../orcom_bin/DnaCategorizer.h \
    ../orcom_bin/BinModule.h \
    ../orcom_bin/DataQueue.h \
    ../orcom_bin/DataPool.h \
    ../orcom_bin/BinOperator.h \
    ../orcom_bin/DnaRecord.h \
    ../orcom_bin/Exception.h \
    ../orcom_bin/Collections.h \
    ../orcom_bin/BinBlockData.h \
    ../orcom_bin/DnaBlockData.h \
    ../orcom_bin/Params.h \
    ../orcom_bin/Thread.h \
    ../orcom_pack/BinFileExtractor.h \
    ../orcom_pack/DnaCompressor.h \
    ../orcom_pack/DnarchFile.h \
    ../orcom_pack/DnarchModule.h \
    ../orcom_pack/DnarchOperator.h \
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
    ../orcom_pack/CompressedBlockData.h \
    ../orcom_pack/Params.h \
    main.h

SOURCES += \
    ../orcom_bin/FileStream.cpp \
    ../orcom_bin/DnaParser.cpp \
    ../orcom_bin/DnaPacker.cpp \
    ../orcom_bin/BinFile.cpp \
    ../orcom_bin/FastqStream.cpp \
    ../orcom_bin/DnaCategorizer.cpp \
    ../orcom_bin/BinModule.cpp \
    ../orcom_bin/BinOperator.cpp \
    ../orcom_pack/BinFileExtractor.cpp \
    ../orcom_pack/DnaCompressor.cpp \
    ../orcom_pack/DnarchFile.cpp \
    ../orcom_pack/DnarchModule.cpp \
    ../orcom_pack/DnarchOperator.cpp \
    ../ppmd/Model.cpp \
    ../ppmd/PPMd.cpp \
    main.cpp
