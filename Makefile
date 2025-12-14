#
# Remember to set CC65_HOME in your .bashrc file.
#   example:  export CC65_HOME="~/sym1/cc65"
#
AS = $(CC65_HOME)/bin/ca65
CC = $(CC65_HOME)/bin/cc65
CL = $(CC65_HOME)/bin/cl65
LD = $(CC65_HOME)/bin/ld65
DA = $(CC65_HOME)/bin/da65

NULLDEV = /dev/null
DEL = $(RM)
RMDIR = $(RM) -r

C_SOURCES = main.c
C_OBJECTS = $(C_SOURCES:.c=.o)

ASM_SOURCES = i2c.s
ASM_OBJECTS = $(ASM_SOURCES:.s=.o)

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

LIBS = --lib sym1.lib

TARGET  = tmp1075.bin

.PHONY:	all clean flatten

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -t sym1 $(OBJECTS) -o $@ $(LIBS)	

%.o: %.c
	$(CC) -t sym1 -O $< 
	$(AS) --cpu 6502 -l $(basename $<).lst $(basename $<).s -o $@ 
	@$(DEL) $(basename $<).s

%.o: %.s	
	$(AS) --cpu 6502 -l $(basename $<).lst $< -o $@ 

clean:
	$(DEL) -rf *.o *.bin *.lst *.out


flatten:
	hexdump -v -e '1/1 "%02x\n"' $(TARGET) > $(basename $(TARGET)).out

