PROGNAME = mandelbrot.out

SRCDIR = src/
OBJDIR = obj/
INCDIR = include/

SRC = $(wildcard $(SRCDIR)*.cpp)

CC = g++ -std=c++17

WARNFLAGS = -Wall -Wno-deprecated-declarations -Wno-writable-strings
CFLAGS = -g -O3 $(WARNFLAGS) -MD -Iinclude/ -I./ -I/usr/local/include
LDFLAGS =-framework opencl -framework GLUT -framework OpenGL -framework Cocoa -L/usr/local/lib

# Do some substitution to get a list of .o files from the given .cpp files.
OBJFILES = $(patsubst $(SRCDIR)%.cpp, $(OBJDIR)%.o, $(SRC))
INCFILES = $(patsubst $(SRCDIR)%.cpp, $(INCDIR)%.hpp, $(SRC))

.PHONY: all clean

all: $(PROGNAME) double_$(PROGNAME)

$(PROGNAME): $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

double_$(PROGNAME): $(OBJFILES:%.o=%_double.o)
	$(CC) $(CFLAGS) -DUSE_DOUBLE -o $@ $^ $(LDFLAGS)

$(OBJDIR)%.o: $(SRCDIR)%.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)%_double.o: $(SRCDIR)%.cpp
	$(CC) -c $(CFLAGS) -DUSE_DOUBLE -o $@ $<

clean:
	rm -fv $(PROGNAME) $(PROGNAME)_double $(OBJFILES) $(OBJFILES:%.o=%.d) $(OBJFILES:%.o=%_double.o) $(OBJFILES:%.o=%_double.d)

-include $(OBJFILES:.o=.d)
-include $(OBJFILES:%.o=%_double.d)
