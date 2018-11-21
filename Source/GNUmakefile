# GNUmakefile 
#   - Supports building with GNU make & gcc
#   - Supports building on Linux, Mac OS, BSD, and Windows
#     assuming that you have GNU make an gcc on those systems.
#   - Automatically separates GNU make builds from other make systems
#     like BSD make and Microsoft's nmake.exe, because
#     GNU make looks for GNUmakefile first and other makes don't
#     look for it at all.
#   - Compiler can be customized on the command line like this: 
#       make CC=gcc-apple-4.2
#   - Install directory can be customized on the command line like this:
#       make PREFIX=/opt/merlin32
#     The default PREFIX is /usr/local.

ifeq ($(OS),Windows_NT)
    ifndef PREFIX
	PREFIX=$(USERPROFILE)/Applications/Merlin32
    endif
    MERLIN_BIN=$(PREFIX)/bin
    MACRO_DIR=$(PREFIX)/asminc
    EXE_EXT=.exe
else
    ifndef PREFIX
	PREFIX=/usr/local
    endif
    MERLIN_BIN=$(PREFIX)/bin
    MACRO_DIR=$(PREFIX)/share/merlin32/asminc
    EXE_EXT=
    INSTALL=install
    INSTALL_DIR=install -d
endif

TARGET=merlin32$(EXE_EXT)
CFLAGS+= -Wall -DMACRO_DIR=\"$(MACRO_DIR)\"

SOURCES = a65816_Code.c a65816_Cond.c a65816_Data.c a65816_File.c \
	  a65816_Line.c a65816_Link.c a65816_Lup.c a65816_Macro.c a65816_OMF.c \
	  Dc_Library.c Main.c 

OBJECTS=$(SOURCES:.c=.o)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@

install:
	$(INSTALL_DIR) $(MERLIN_BIN)
	$(INSTALL) $(TARGET) $(MERLIN_BIN)
	$(INSTALL_DIR) $(MACRO_DIR)
	$(INSTALL) ../Library/* $(MACRO_DIR)
	
clean:
	$(RM) $(OBJECTS) $(TARGET)

