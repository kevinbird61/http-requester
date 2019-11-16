CC:=gcc
CFLAGS:=-std=gnu99 -O2 -g
LIBS:=-Ilib/
OBJS:= $(patsubst %.c, %.o, $(subst lib/,,$(wildcard lib/*.c)))
EXEC:= $(patsubst %.c, %.exe, $(subst src/,,$(wildcard src/*.c)))
TEST:= $(patsubst %.c, %.out, $(subst test/,,$(wildcard test/*.c)))
TOOLS:= $(patsubst %.c, %.app, $(subst tools/,,$(wildcard tools/*.c)))

all: $(OBJS) $(EXEC) $(TEST) $(TOOLS)

%.o: lib/%.c 
	$(CC) $(CFLAGS) -c $^ $(LIBS)

%.exe: src/%.c $(OBJS)
	$(CC) $(LIBS) -o $@ $(OBJS) $< $(LIBS)

%.out: test/%.c $(OBJS)
	$(CC) $(LIBS) -o $@ $(OBJS) $< $(LIBS)

%.app: tools/%.c $(OBJS)
	$(CC) $(LIBS) -o $@ $(OBJS) $< $(LIBS)

.PHONY=clean

clean:
	rm *.exe *.o *.out*.app
