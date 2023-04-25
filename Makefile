PROGNAME = mandelbrot.out

SRCDIR = src/
OBJDIR = obj/
INCDIR = include/

SRC = $(wildcard $(SRCDIR)*.cpp)

CC = g++ -std=c++17

WARNFLAGS = -Wall -Wno-deprecated-declarations -Wno-writable-strings
CFLAGS = -g -O3 $(WARNFLAGS) -MD -Iinclude/ -Iimgui/ -Iimgui/backends/ -I/usr/local/include
LDFLAGS =-framework opencl -framework GLUT -framework OpenGL -framework Cocoa -L/usr/local/lib

# Do some substitution to get a list of .o files from the given .cpp files.
OBJFILES = $(patsubst $(SRCDIR)%.cpp, $(OBJDIR)%.o, $(SRC))
INCFILES = $(patsubst $(SRCDIR)%.cpp, $(INCDIR)%.hpp, $(SRC))


IMGUI_DIR = imgui/
IMGUI_SRC = $(IMGUI_DIR)imgui.cpp $(IMGUI_DIR)imgui_draw.cpp $(IMGUI_DIR)imgui_tables.cpp $(IMGUI_DIR)imgui_widgets.cpp
IMGUI_SRC += $(IMGUI_DIR)backends/imgui_impl_glut.cpp $(IMGUI_DIR)backends/imgui_impl_opengl2.cpp
IMGUI_OBJ = $(patsubst $(IMGUI_DIR)%.cpp, $(OBJDIR)%.o, $(IMGUI_SRC))

.PHONY: all clean

all: $(PROGNAME) double_$(PROGNAME)

$(PROGNAME): $(OBJFILES) $(IMGUI_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

double_$(PROGNAME): $(OBJFILES:%.o=%_double.o) $(IMGUI_OBJ)
	$(CC) $(CFLAGS) -DMANDEL_GPU_USE_DOUBLE -o $@ $^ $(LDFLAGS)

$(OBJDIR)%.o: $(SRCDIR)%.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)%_double.o: $(SRCDIR)%.cpp
	$(CC) -c $(CFLAGS) -DMANDEL_GPU_USE_DOUBLE -o $@ $<

$(OBJDIR)%.o: $(IMGUI_DIR)%.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -fv $(PROGNAME) double_$(PROGNAME) $(OBJFILES) $(OBJFILES:%.o=%.d) $(OBJFILES:%.o=%_double.o) $(OBJFILES:%.o=%_double.d)

-include $(OBJFILES:.o=.d)
-include $(OBJFILES:%.o=%_double.d)
