OS:=$(strip$(OS))

APP_NAME=rapi_core

ifeq ($(OS),win32)
	SHARED_LIB_EXT=dll
	STATIC_LIB_EXT=a
	EXE_EXT=exe
	BUILDDIR = build-win-32bit
	EXE_NAME = $(APP_NAME)-32bit
	TCHAIN=i686-w64-mingw32-
	LDFLAGS += -static
else ifeq ($(OS),win64)
	SHARED_LIB_EXT=dll
	STATIC_LIB_EXT=a
	EXE_EXT=exe
	BUILDDIR = build-win-64bit
	EXE_NAME = $(APP_NAME)-64bit
	TCHAIN=x86_64-w64-mingw32-
	LDFLAGS += -static
else
	SHARED_LIB_EXT=
	STATIC_LIB_EXT=
	EXE_EXT=
	BUILDDIR = build
	EXE_NAME = $(APP_NAME)
	PPDEFS += COLOR_TERM
endif

#
#Verbose mode
#
VERBOSE=no

#
#Colorize ouput
#
COLORIZE=no

#
#Enable binary creation
#
MAKE_BINARY=yes

#
#Enable binary creation
#
MAKE_EXECUTABLE=yes

#
#Enable shared library creation
#
MAKE_SHARED_LIB=no

#
#Enable static library creation
#
MAKE_STATIC_LIB=no

#
#Enable MAP-file creation
#
CREATE_MAP=no

#
#Tool-chain prefix
#
#TCHAIN = 

#
#CPU specific options
#
#MCPU += -m32

#
#C language dialect
#
CDIALECT = gnu99

#
#C++ language dialect
#
CPPDIALECT = c++1z

#
#Optimization
#
OPT_LVL = 2

#
#Additional C flags
#
#CFLAGS += 


#
#Additional CPP flags
#
#CPPFLAGS += -felide-constructors



#
#Additional static libraries
#
EXT_LIBS += pthread

#
#Additional non-project object files
#


#
#Preprocessor definitions
#
#PPDEFS += 



#
#Include directories
#
INCDIR += .

#
#C sources
#
C_SOURCES += $(wildcard *.c)

#
#Assembler sources
#
#S_SOURCES += 

#
#CPP sources
#
#CPP_SOURCES +=

#
#Linker scripts
#
#LDSCRIPT += 

include core.mk
