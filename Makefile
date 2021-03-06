
# Makefile based on Terasic Makefile for my_first_hps-fpga template project
# Created by Satyen Akolkar
# Date: Mar 12 2017

TARGET = generateJSON

CFLAGS = -static -g -Wall -D DEBUG
LDFLAGS = -g -Wall -l json -lm
CC = gcc
ARCH = arm

build: $(TARGET)

$(TARGET): $(TARGET).o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) *.a *.o *~
