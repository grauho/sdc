.POSIX:
CC		= cc
CFLAGS		= -Wall -pedantic -O2 -Wno-unused-function 
LDFLAGS		= 
OBJFILES	= main.o cJSON.o fileLoading.o
TARGET		= sdc

ifeq ($(OS),Windows_NT)
TARGET = sdc.exe
endif # Windows

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

rebuild: clean
rebuild: all

clean:
	rm -f $(OBJFILES) $(TARGET)

.PHONY: rebuild clean
