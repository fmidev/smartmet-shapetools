HTML = shapetools

MAINFLAGS = -Wall -W -Wno-unused-parameter

EXTRAFLAGS = -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wwrite-strings -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast

DIFFICULTFLAGS = -Weffc++ -Wredundant-decls -Wshadow -Woverloaded-virtual -Wunreachable-code -Wconversion

CC = g++

# Default compile options

CFLAGS = -DUNIX -O2 -DNDEBUG $(MAINFLAGS)
LDFLAGS = 

# Special modes

CFLAGS_DEBUG = -DUNIX -O0 -g $(MAINFLAGS) $(EXTRAFLAGS) -Werror
CFLAGS_PROFILE = -DUNIX -O2 -g -pg -DNDEBUG $(MAINFLAGS)

LDFLAGS_DEBUG = 
LDFLAGS_PROFILE = 

INCLUDES = -I$(includedir) \
	-I$(includedir)/smartmet \
	-I$(includedir)/smartmet/newbase \
	-I$(includedir)/smartmet/imagine

LIBS = -L$(libdir) \
	-lsmartmet_imagine \
	-lsmartmet_newbase \
	-lsmartmet_macgyver \
	-lboost_iostreams\
	-lboost_program_options \
	-lboost_filesystem\
	-lboost_regex\
	-lboost_thread \
	-lboost_system\
	-lpng -ljpeg -lz -lbz2 -lpthread -lrt

#	-lsmartmet_macgyver \

# Common library compiling template

# Installation directories
processor := $(shell uname -p)

ifeq ($(origin PREFIX), undefined)
  PREFIX = /usr
else
  PREFIX = $(PREFIX)
endif

ifeq ($(processor), x86_64)
  libdir = $(PREFIX)/lib64
else
  libdir = $(PREFIX)/lib
endif

objdir = obj
includedir = $(PREFIX)/include

ifeq ($(origin BINDIR), undefined)
  bindir = $(PREFIX)/bin
else
  bindir = $(BINDIR)
endif

# rpm variables

CWP = $(shell pwd)
BIN = $(shell basename $(CWP))

rpmsourcedir=/tmp/$(shell whoami)/rpmbuild

rpmerr = "There's no spec file ($(specfile)). RPM wasn't created. Please make a spec file or copy and rename it into $(specfile)"

rpmversion := $(shell grep "^Version:" $(HTML).spec  | cut -d\  -f 2 | tr . _)
rpmrelease := $(shell grep "^Release:" $(HTML).spec  | cut -d\  -f 2 | tr . _)

rpmexcludevcs := $(shell tar --help | grep -m 1 -o -- '--exclude-vcs')

# Special modes

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_DEBUG)
  LDFLAGS = $(LDFLAGS_DEBUG)
endif

ifneq (,$(findstring profile,$(MAKECMDGOALS)))
  CFLAGS = $(CFLAGS_PROFILE)
  LDFLAGS = $(LDFLAGS_PROFILE)
endif

# Compilation directories

vpath %.cpp source main
vpath %.h include
vpath %.o $(objdir)

# How to install

INSTALL_PROG = install -m 775
INSTALL_DATA = install -m 664

# The files to be compiled

SRCS = $(patsubst source/%,%,$(wildcard *.cpp source/*.cpp))
HDRS = $(patsubst include/%,%,$(wildcard *.h include/*.h))
OBJS = $(SRCS:%.cpp=%.o)

OBJFILES = $(OBJS:%.o=obj/%.o)

MAINSRCS     = $(patsubst main/%,%,$(wildcard main/*.cpp))
MAINPROGS    = $(MAINSRCS:%.cpp=%)
MAINOBJS     = $(MAINSRCS:%.cpp=%.o)
MAINOBJFILES = $(MAINOBJS:%.o=obj/%.o)

SRCS     = $(patsubst source/%,%,$(wildcard source/*.cpp))
OBJS     = $(SRCS:%.cpp=%.o)
OBJFILES = $(OBJS:%.o=obj/%.o)

INCLUDES := -Iinclude $(INCLUDES)

# For make depend:

ALLSRCS = $(wildcard *.cpp source/*.cpp)

.PHONY: test rpm

# The rules

all: objdir $(MAINPROGS)
debug: objdir $(MAINPROGS)
release: objdir $(MAINPROGS)
profile: objdir $(MAINPROGS)

$(MAINPROGS): % : $(OBJS) %.o
	$(CC) $(LDFLAGS) -o $@ obj/$@.o $(OBJFILES) $(LIBS)

clean:
	rm -f $(MAINPROGS) $(OBJFILES) $(MAINOBJFILES)*~ source/*~ include/*~

install:
	mkdir -p $(bindir)
	@list='$(MAINPROGS)'; \
	for prog in $$list; do \
	  echo $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	  $(INSTALL_PROG) $$prog $(bindir)/$$prog; \
	done

depend:
	gccmakedep -fDependencies -- $(CFLAGS) $(INCLUDES) -- $(ALLSRCS)

test:
	cd test && make test

html::
	mkdir -p ../../../../html/bin/$(HTML)
	doxygen $(HTML).dox

objdir:
	@mkdir -p $(objdir)

tag:
	cvs -f tag 'smartmet_$(HTML)_$(rpmversion)-$(rpmrelease)' .

rpm: clean depend
	if [ -a $(BIN).spec ]; \
	then \
	  smartspecupdate $(BIN).spec ; \
	  mkdir -p $(rpmsourcedir) ; \
	  tar $(rpmexcludevcs) -C ../ -cf $(rpmsourcedir)/smartmet-$(BIN).tar $(BIN) ; \
	  gzip -f $(rpmsourcedir)/smartmet-$(BIN).tar ; \
	  rpmbuild -ta $(rpmsourcedir)/smartmet-$(BIN).tar.gz ; \
	  rm -f $(rpmsourcedir)/smartmet-$(BIN).tar.gz ; \
	else \
	  echo $(rpmerr); \
	fi;

.SUFFIXES: $(SUFFIXES) .cpp

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $(objdir)/$@ $<

-include Dependencies
