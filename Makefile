CC:=gcc
LDFLAGS:=-static
CFLAGS:=-fPIC -std=gnu99
LIBS:=-I include/
OBJS:= $(patsubst %.c, %.o, $(subst lib/,,$(wildcard lib/*.c)))
EXEC:= $(patsubst %.c, %.exe, $(subst src/,,$(wildcard src/*.c)))
TEST:= $(patsubst %.c, %.out, $(subst test/,,$(wildcard test/*.c)))
TOOLS:= $(patsubst %.c, %.app, $(subst tools/,,$(wildcard tools/*.c)))
RELEASE:=libhttp_requester.a
STATIC_BUILD:=kb

all: $(OBJS) $(EXEC)

test: $(OBJS) $(TEST) $(TOOLS)

# build the shared lib
$(RELEASE): $(OBJS)
	ar cr $(RELEASE) $(OBJS)
	ranlib $(RELEASE)

# build the release program
release: $(RELEASE)
	gcc -o $(STATIC_BUILD) src/kb.c $(LIBS) -L. -lhttp_requester $(CFLAGS) -lpthread

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
	rm -rf $(OBJS) $(EXEC) $(STATIC_BUILD) $(RELEASE) $(TEST) $(TOOLS) && rm -rf /tmp/http_request* 
