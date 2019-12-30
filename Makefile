CC:=gcc
VER_MAJOR=1
VER_MINOR=0
VER_PATCH=0
DFLAGS= -DVER_MAJ=$(VER_MAJOR) -DVER_MIN=$(VER_MINOR) -DVER_PAT=$(VER_PATCH)
LDFLAGS:=-static 
CFLAGS:=-fPIC -O2 -std=gnu99
LIBS:=-I include/
SRCS:= $(wildcard lib/*.c)
OBJS:= $(patsubst %.c, %.o, $(subst lib/,,$(wildcard lib/*.c)))
EXEC:= $(patsubst %.c, %.exe, $(subst src/,,$(wildcard src/*.c)))
RELEASE:=libhttp_requester.a
BUILD:=kb

all: $(OBJS) $(EXEC)

test: $(OBJS) $(TEST) $(TOOLS)

# build the shared lib
$(RELEASE): $(OBJS)
	ar cr $(RELEASE) $(OBJS)
	ranlib $(RELEASE)

# build the release program (dynamic link)
release: $(RELEASE)
	gcc -o $(BUILD) src/kb.c $(LIBS) -L. -lhttp_requester $(CFLAGS) -lpthread

# build the release program (static link)
static: 
	gcc -o $(BUILD) $(SRCS) src/kb.c $(LIBS) $(LDFLAGS) $(CFLAGS) -lpthread

%.o: lib/%.c 
	$(CC) $(CFLAGS) $(DFLAGS) -c $^ $(LIBS) -lpthread 

%.exe: src/%.c $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $@ $(OBJS) $< $(LIBS) -lpthread

.PHONY=clean

clean:
	rm -rf $(OBJS) $(EXEC) $(STATIC_BUILD) $(RELEASE) $(TEST) $(TOOLS) && rm -rf /tmp/kb* 
