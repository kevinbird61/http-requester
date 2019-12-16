CC:=gcc
LDFLAGS:=-static
CFLAGS:=-std=gnu99 -fPIC -g3
LIBS:=-I include/
OBJS:= $(patsubst %.c, %.o, $(subst lib/,,$(wildcard lib/*.c)))
EXEC:= $(patsubst %.c, %.exe, $(subst src/,,$(wildcard src/*.c)))
TEST:= $(patsubst %.c, %.out, $(subst test/,,$(wildcard test/*.c)))
TOOLS:= $(patsubst %.c, %.app, $(subst tools/,,$(wildcard tools/*.c)))
RELEASE:=libhttp_requester.a
STATIC_BUILD:=sc_http_requester.exe

all: $(OBJS) $(EXEC) $(TEST) $(TOOLS)

# build the shared lib
lib: $(OBJS)
	ar cr $(RELEASE) $(OBJS)
	ranlib $(RELEASE)

# build the release program
release: $(RELEASE)
	gcc -o $(STATIC_BUILD) src/new_http_requester.c $(LIBS) -L. -lhttp_requester $(CFLAGS)

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
	rm *.exe *.o *.out *.app *.a
	rm /tmp/http_request* 
