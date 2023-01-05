.SUFFIXES : .x .o .c .s

CC := aarch64-linux-gcc
STRIP := aarch64-linux-strip

TARGET = cpu-freq
SRCS := cpu-freq.c
LIBS = -lpthread -lc -lgcc
BUILD_DIR = build

all:
	$(CC) -static $(SRCS) -o $(TARGET) $(LIBS)
	$(STRIP) $(TARGET)
	mkdir -p ./$(BUILD_DIR)
	mv $(TARGET) ./build/$(TARGET).elf
	sync

clean:
	rm -f *.o
	rm -f *.x
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET)
	rm -rf $(BUILD_DIR)
	rm -f *.gdb
