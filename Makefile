OBJS=robot.o parse.o
PROG=robot
CMD_FILES=cmd1.txt cmd2.txt cmd3.txt

CFLAGS=-O2 -g -Wextra -Wall -pedantic

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -MD -c -o $@ $<

check: all
	@for cmd in $(CMD_FILES) ; do \
		echo 'Running input' $$cmd ; \
		./$(PROG) ./input/$$cmd | diff ./expected/$$cmd - ; \
	done

clang-format:
	@for src in $(shell find . -name '*.c' -or -name '*.h') ; do \
		clang-format -style=file -i $$src -verbose ; \
	done

clean:
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f $(PROG)

.PHONY:= all clean check clang-format
