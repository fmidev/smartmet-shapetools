HTML = shapetools
PROG = shape2ps triangle2shape shape2triangle amalgamate shapedump shape2xml

MAINFLAGS = -Wall -W -Wno-unused-parameter

EXTRAFLAGS = -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wwrite-strings -Wconversion -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast

DIFFICULTFLAGS = -Weffc++ -Wredundant-decls -Wshadow -Woverloaded-virtual -Wunreachable-code

CC = g++
CFLAGS = -DUNIX -O0 -g $(MAINFLAGS) $(EXTRAFLAGS)
CFLAGS_RELEASE = -DUNIX -O2 -DNDEBUG $(MAINFLAGS)
LDFLAGS = -s
ARFLAGS = -r
INCLUDES = -I $(includedir)/newbase -I $(includedir)/imagine
LIBS = -L$(libdir) -limagine -lnewbase -lpng -ljpeg

# Common library compiling template

include ../../makefiles/makefile.prog

# Some extras

LIBNAME = shapetools
LIB = lib$(LIBNAME).a
lib:	$(LIB)
$(LIB): objdir $(OBJS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJFILES)

install-lib:
	@mkdir -p $(includedir)/$(LIBNAME)
	@list='$(HDRS)'; \
	for hdr in $$list; do \
	  if [[ include/$$hdr -nt $(includedir)/$(LIBNAME)/$$hdr ]]; \
	  then \
	    echo $(INSTALL_DATA) include/$$hdr $(includedir)/$(LIBNAME)/$$hdr; \
	  fi; \
	  $(INSTALL_DATA) include/$$hdr $(includedir)/$(LIBNAME)/$$hdr; \
	done
	@mkdir -p $(libdir)
	$(INSTALL_DATA) $(LIB) $(libdir)/$(LIB)

