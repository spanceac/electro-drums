CROSS_COMPILE = arm-none-eabi-
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld -v
CP      = $(CROSS_COMPILE)objcopy
OD      = $(CROSS_COMPILE)objdump
  
CFLAGS  =  -I./ -c -fno-common -O0 -mcpu=cortex-m3 -mthumb -g
LFLAGS  = -Tstm32f100.ld
CPFLAGS = -Obinary
ODFLAGS = -S

all: test

clean:
	-rm -f *.lst *.o *.elf *.bin

test: drums.elf
	@ echo "...copying"
	$(CP) $(CPFLAGS) drums.elf drums.bin
	$(OD) $(ODFLAGS) drums.elf > drums.lst

drums.elf: drums.o stm32f100.ld
	@ echo "..linking"
	$(LD) $(LFLAGS) -o drums.elf drums.o startup_stm32f10x.o

drums.o: drums.c
	@ echo ".compiling"
	$(CC) $(CFLAGS) drums.c
	$(CC) $(CFLAGS) startup_stm32f10x.c



