ifeq ($(CC),cc)
	CC=gcc
else
	echo Using compiler set in environment: $(CC)
endif

PREFIX=/usr/local

CFLAGS += -Icompat -Werror -DUNIX

SOURCES = a65816_Code.c a65816_Cond.c a65816_Data.c a65816_File.c \
	  a65816_Line.c a65816_Link.c a65816_Lup.c a65816_Macro.c a65816_OMF.c \
	  Dc_Library.c Main.c 

OBJECTS=$(SOURCES:.c=.o)

TARGET=Merlin32

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@

install:
	install merlin32.sh $(PREFIX)/bin/merlin32
	install merlin32.sh $(PREFIX)/bin/merlin
	install -d $(PREFIX)/libexec
	install -d $(PREFIX)/share/merlin32/asminc
	install $(TARGET) $(PREFIX)/libexec
	install ../Library/* $(PREFIX)/share/merlin32/asminc
	
clean:
	rm -f $(OBJECTS) $(TARGET)

