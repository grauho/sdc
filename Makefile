.POSIX:
CC		= cc
CFLAGS		= -Wall -pedantic -O2 -Wno-unused-function 
LDFLAGS		= 
OBJFILES	= main.o cJSON.o fileLoading.o converting.o
TARGET		= sdc

ifeq ($(OS),Windows_NT)
TARGET = sdc.exe
endif # Windows

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

debug: CFLAGS = -Wall -pg -Wextra -Wpedantic -ggdb -Og -DDEBUG_LOGGING
debug: CFLAGS += -fsanitize=address -fsanitize=leak 
debug: CFLAGS += -fsanitize=undefined
debug: CFLAGS += -Wdouble-promotion -Wformat -Wformat-overflow
debug: CFLAGS += -Wnull-dereference -Winfinite-recursion
debug: CFLAGS += -Wstrict-overflow -Wno-unused-function -Wconversion 
debug: all

rebuild: clean
rebuild: all

clean:
	rm -f $(OBJFILES) $(TARGET)

.PHONY: debug rebuild clean
