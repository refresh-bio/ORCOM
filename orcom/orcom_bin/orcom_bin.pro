TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lz -lboost_thread -lboost_system

QMAKE_CXXFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DUSE_BOOST_THREAD

SOURCES += main.cpp \
    FileStream.cpp \
    FastqStream.cpp \
    DnaCategorizer.cpp \
    DnaPacker.cpp \
    DnaParser.cpp \
    BinFile.cpp \
    BinModule.cpp \
    BinOperator.cpp

HEADERS += \
    FileStream.h \
    FastqStream.h \
    DataStream.h \
    Globals.h \
    Buffer.h \
    Utils.h \
    BitMemory.h \
    DnaCategorizer.h \
    DnaPacker.h \
    DnaParser.h \
    BinFile.h \
    BinModule.h \
    DataQueue.h \
    DataPool.h \
    BinOperator.h \
    DnaRecord.h \
    Exception.h \
    Collections.h \
    BinBlockData.h \
    DnaBlockData.h \
    Params.h \
    Thread.h \
    main.h

