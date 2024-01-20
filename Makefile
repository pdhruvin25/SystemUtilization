CC = gcc
CFLAGS = -Wall -Werror -std=c99 -I/usr/local/include
.PHONY: all
all: mySystemInfo

mySystemInfo: a3SystemUtil.o systemUtil.o
	$(CC) $(CFLAGS) -o mySystemInfo a3SystemUtil.o systemUtil.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

## clean: remove object files and executable
.PHONY: clean
clean:
	rm -f mySystemInfo
	rm -f *.o

CFLAGS += -D_POSIX_C_SOURCE=200809L