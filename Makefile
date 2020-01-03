LIBS:= $(wildcard lib/*.c)
OBJS:= $(patsubst %.c, %.o, $(subst lib/,,$(wildcard lib/*.c)))
EXEC:= $(patsubst %.c, %.exe, $(subst src/,,$(wildcard src/*.c)))
VER_MAJOR=1
VER_MINOR=1
VER_PATCH=0

RELEASE:=libkb.a
DYNAMIC:=kb_d_$(VER_MAJOR)_$(VER_MINOR)_$(VER_PATCH).exe
STATIC:=kb_s_$(VER_MAJOR)_$(VER_MINOR)_$(VER_PATCH).exe

CC:=gcc
DFLAGS= -DVER_MAJ=$(VER_MAJOR) -DVER_MIN=$(VER_MINOR) -DVER_PAT=$(VER_PATCH)
LDFLAGS:=-static 
CFLAGS:=-fPIC -O2 -std=gnu99
INC:=-I include/

all: $(OBJS) $(EXEC)
test: $(OBJS) $(TEST) $(TOOLS)
# build the release program 
release: dynamic static

# build the release program (dynamic link)
dynamic: $(RELEASE)
	gcc -o $(DYNAMIC) src/kb.c $(INC) -L. -lkb $(CFLAGS) -lpthread
# build the release program (static link)
static: 
	gcc -o $(STATIC) $(LIBS) src/kb.c $(INC) $(LDFLAGS) $(CFLAGS) -DRELEASE -lpthread

# build the shared lib
$(RELEASE): $(OBJS)
	ar cr $(RELEASE) $(OBJS)
	ranlib $(RELEASE)

%.o: lib/%.c 
	$(CC) $(CFLAGS) $(DFLAGS) -c $^ $(INC) -lpthread 

%.exe: src/%.c $(OBJS)
	$(CC) $(CFLAGS) $(INC) -o $@ $(OBJS) $< $(INC) -lpthread

.PHONY=clean

clean:
	rm -rf $(OBJS) $(EXEC) $(STATIC_BUILD) $(RELEASE) $(TEST) $(TOOLS) && rm -rf /tmp/kb* 
