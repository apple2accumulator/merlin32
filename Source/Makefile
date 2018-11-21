# Makefile 
#   - Builds with nmake.exe and Microsoft Visual C++ on Windows.
#     See GNUmakefile for building on Linux, BSD, & Mac.
#   - Installs to C:\Users\<user>\Applications\Merlin32 because installing
#     to C:\Program Files requires privilege elevation.
#   - Usage:
#   	1. Start a "Native Tools Command Prompt for Visual Studio" from the
#   	   Start Menu.
#   	2. cd to the "Source" directory in the Merlin32 distribution.
#   	3. Type: 
#   		nmake
#   		nmake install

HOME=$(USERPROFILE:\=\\)
PREFIX=$(HOME)\\Applications\\Merlin32
MERLIN_BIN=$(PREFIX)\\bin
MACRO_DIR=$(PREFIX)\\asminc
EXE_EXT=.exe
INSTALL=copy
INSTALL_DIR=mkdir

TARGET=merlin32$(EXE_EXT)
CFLAGS=/DMACRO_DIR="\"$(MACRO_DIR)\""

SOURCES = a65816_Code.c a65816_Cond.c a65816_Data.c a65816_File.c \
	  a65816_Line.c a65816_Link.c a65816_Lup.c a65816_Macro.c a65816_OMF.c \
	  Dc_Library.c Main.c 

OBJECTS=$(SOURCES:.c=.obj)

$(TARGET): $(OBJECTS)
	link $** /out:$@

install: $(TARGET)
	..\Scripts\install.bat "$(PREFIX:\\=\)"
	
clean:
	del $(OBJECTS) $(TARGET)

