TEMPLATE = app
CONFIG += c++17
CONFIG -= app_bundle
CONFIG -= qt

CONFIG(debug, debug|release){
    DEFINES += DEBUG_LOG_ON
    message("Debug Build")
}

CONFIG(release, debug|release){
    message("Release Build")
}

QMAKE_CFLAGS *= -utf-8
QMAKE_CXXFLAGS *= -utf-8

QMAKE_LFLAGS *= /ENTRY:mainCRTStartup
QMAKE_LFLAGS_DEBUG *= /NODEFAULTLIB:msvcrt.lib

# LIBS += -L$$PWD/win_lib
LIBS += User32.lib

LIBS += -L$$PWD/zlib-ng/lib
# LIBS += zlibstatic-ng.lib

INCLUDEPATH += $$PWD/zlib-ng/include

HEADERS += \
    zlib-ng/include/zlib-ng.h


DEFINES += _WIN32_WINNT=0x0601
DEFINES += HAVE_ZLIB
DEFINES += _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE
DEFINES += MZ_ZIP_NO_CRYPTO
INCLUDEPATH += $$PWD/minizip-ng

HEADERS += \
    minizip-ng/mz.h \
    minizip-ng/mz_os.h \
    minizip-ng/mz_strm.h \
    minizip-ng/mz_strm_buf.h \
    minizip-ng/mz_strm_mem.h \
    minizip-ng/mz_strm_split.h \
    minizip-ng/mz_strm_zlib.h \
    minizip-ng/mz_zip.h \
    minizip-ng/mz_zip_rw.h

SOURCES += \
    minizip-ng/mz_crypt.c \
    minizip-ng/mz_os.c \
    minizip-ng/mz_strm.c \
    minizip-ng/mz_strm_buf.c \
    minizip-ng/mz_strm_mem.c \
    minizip-ng/mz_strm_split.c \
    minizip-ng/mz_strm_zlib.c \
    minizip-ng/mz_zip.c \
    minizip-ng/mz_zip_rw.c \
    minizip-ng/mz_os_win32.c \
    minizip-ng/mz_strm_os_win32.c

SOURCES += \
        main.cpp
