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

all: $(OBJS) $(EXEC)

test: $(OBJS) $(TEST) $(TOOLS)

# build the shared lib
$(RELEASE): $(OBJS)
	ar cr $(RELEASE) $(OBJS)
	ranlib $(RELEASE)

# build the release program
release: $(RELEASE)
	gcc -o $(STATIC_BUILD) src/new_http_requester.c $(LIBS) -L. -lhttp_requester $(CFLAGS) -lpthread

%.o: lib/%.c 
	$(CC) $(CFLAGS) -c $^ $(LIBS) -lpthread

%.exe: src/%.c $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $(OBJS) $< $(LIBS) -lpthread

%.out: test/%.c $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $(OBJS) $< $(LIBS)

%.app: tools/%.c $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $(OBJS) $< $(LIBS)

.PHONY=clean

clean:
	rm *.exe *.o *.out *.app *.a
	rm /tmp/http_request* 
