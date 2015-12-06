LIB_PATHS=-L/usr/local/lib
LIBS=-lopencv_highgui -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lm
# CC := g++
CC := clang++ # and comment out the linker last line for sanity
SRCDIR := src
BUILDDIR := build
TARGET := visual
SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CFLAGS := -g -Wall -std=c++11
INC := -Iinclude -I/usr/local/include -I/usr/local/include/opencv

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB_PATHS) $(LIBS)"; $(CC) $^ -o $(TARGET) $(LIB_PATHS) $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

dist: doc src include makefile README.md
	@echo " Creating source distribution tar..."; tar czvf foxhall_visual.tar.gz *

.PHONY: clean
