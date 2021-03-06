# -------- Generated by configure -----------

CXX := g++
CXXFLAGS :=   
LD := g++
LIBS +=  -L/usr/lib -Wl,-rpath,/usr/lib -lSDL -lpthread -lz
RANLIB := ranlib
INSTALL := install
AR := ar cru
MKDIR := mkdir -p
ECHO := printf
CAT := cat
RM := rm -f
RM_REC := rm -f -r
ZIP := zip -q
CP := cp
WIN32PATH=
STRIP := strip
WINDRES := windres

MODULES +=  src/unix src/debugger src/debugger/gui src/yacc src/cheat
MODULE_DIRS += 
EXEEXT := 

PREFIX := /usr/local
BINDIR := /usr/local/bin
DOCDIR := /usr/local/share/doc/stella
DATADIR := /usr/local/share
PROFILE := 

HAVE_GCC3 = 1

INCLUDES += -Isrc/emucore -Isrc/emucore/m6502/src -Isrc/emucore/m6502/src/bspf/src -Isrc/common -Isrc/gui -I/usr/include/SDL -D_REENTRANT -Isrc/unix -Isrc/debugger -Isrc/debugger/gui -Isrc/yacc -Isrc/cheat
OBJS += 
DEFINES +=  -DUNIX -DBSPF_UNIX -DHAVE_GETTIMEOFDAY -DHAVE_INTTYPES -DSOUND_SUPPORT -DDEBUGGER_SUPPORT -DSNAPSHOT_SUPPORT -DJOYSTICK_SUPPORT -DCHEATCODE_SUPPORT
LDFLAGS += 
