#CC = arm-none-linux-gnueabi-gcc
CC = gcc
CFLAGS = -static
OBJS = main.o input_process.o output_process.o merge_process.o main_process.o device.o
TARGET = 20211584.out

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "Target file created: $(TARGET)"

main.o : main.c main.h
	$(CC) $(CFLAGS) -c main.c

inp_proc.o : input_process.c input_process.h main.h
	$(CC) $(CFLAGS) -c input_process.c

output_process.o : output_process.c output_process.h main.h
	$(CC) $(CFLAGS) -c output_process.c

merge_process.o : merge_process.c merge_process.h main.h
	$(CC) $(CFLAGS) -c merge_process.c

main_process.o : main_process.c main_process.h main.h
	$(CC) $(CFLAGS) -c main_process.c

device.o : device.c device.h main.h
	$(CC) $(CFLAGS) -c device.c

clean:
	@echo "Cleaning files: $(TARGET) $(OBJS)"
	rm -rf $(TARGET) $(OBJS)

push:
	make
	@echo "Pushing file: $(TARGET)"
	adb push $(TARGET) /data/local/tmp

.PHONY: clean push

