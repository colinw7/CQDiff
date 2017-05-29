TEMPLATE = app

QT += widgets

TARGET = CQDiff

DEPENDPATH += .

INCLUDEPATH += . ../include

QMAKE_CXXFLAGS += -std=c++11

CONFIG += debug

MOC_DIR = .moc

# Input
SOURCES += \
main.cpp \
CQDiff.cpp \

HEADERS += \
CQDiff.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

INCLUDEPATH += \
. \
../../CQUtil/include \
../../CCommand/include \
../../CFile/include \
../../CStrUtil/include \
../../CUtil/include \

unix:LIBS += \
-L$$LIB_DIR \
-L../../CQUtil/lib \
-L../../CCommand/lib \
-L../../CConfig/lib \
-L../../CImageLib/lib \
-L../../CFont/lib \
-L../../CFile/lib \
-L../../CFileUtil/lib \
-L../../CMath/lib \
-L../../CStrUtil/lib \
-L../../CUtil/lib \
-L../../CRegExp/lib \
-L../../COS/lib \
-lCQUtil -lCCommand -lCConfig -lCImageLib -lCFont \
-lCFile -lCFileUtil -lCMath -lCStrUtil -lCUtil -lCOS \
-lCRegExp -lpng -ljpeg -lcurses -ltre
