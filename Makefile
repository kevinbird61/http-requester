CC:=gcc
CFLAGS:=-std=gnu99 -O2
LIBS:=-Ilib/
OBJS:= $(patsubst %.c, %.o, $(subst lib/,,$(wildcard lib/*.c)))
EXEC:= $(patsubst %.c, %.exe, $(subst src/,,$(wildcard src/*.c)))
TEST:= $(patsubst %.c, %.out, $(subst test/,,$(wildcard test/*.c)))

all: $(OBJS) $(EXEC) $(TEST)

%.o: lib/%.c 
	$(CC) $(CFLAGS) -c $^ $(LIBS)

%.exe: src/%.c $(OBJS)
	$(CC) $(LIBS) -o $@ $(OBJS) $< $(LIBS)

%.out: test/%.c $(OBJS)
	$(CC) $(LIBS) -o $@ $(OBJS) $< $(LIBS)

.PHONY=clean

clean:
	rm *.exe *.o *.out
