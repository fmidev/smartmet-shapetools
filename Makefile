HTML = shapetools
PROG = shape2ps triangle2shape shape2triangle amalgamate

MAINFLAGS = -Wall -W -Wno-unused-parameter

EXTRAFLAGS = -Werror -pedantic -Wpointer-arith -Wcast-qual \
	-Wcast-align -Wwrite-strings -Wconversion -Winline \
	-Wctor-dtor-privacy -Wnon-virtual-dtor -Wno-pmf-conversions \
	-Wsign-promo -Wchar-subscripts -Wold-style-cast -Wunreachable-code

DIFFICULTFLAGS = -Weffc++ -Wredundant-decls -Wshadow -Woverloaded-virtual

CC = g++
CFLAGS = -DUNIX -O0 -g $(MAINFLAGS) $(EXTRAFLAGS) -Werror
CFLAGS_RELEASE = -DUNIX -O2 -DNDEBUG $(MAINFLAGS)
LDFLAGS = -s
ARFLAGS = -r
INCLUDES = -I $(includedir)/newbase -I $(includedir)/imagine
LIBS = -L$(libdir) -limagine -lnewbase -lpng -ljpeg

# Common library compiling template

include ../../makefiles/makefile.prog
