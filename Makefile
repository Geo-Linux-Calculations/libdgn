PROJECT=libdgn
SRC=src/
SOVER=1
CXX = g++
CXXFLAGS = -Wall -I$(SRC)cpl/ -I$(SRC)dng/
ifeq ($(shell uname --machine), x86_64)
    CXXFLAGS += -fPIC
endif
LDFLAGS = -s

DGNLIB_OBJ =	$(SRC)dng/dgnfloat.o \
                $(SRC)dng/dgnhelp.o \
                $(SRC)dng/dgnread.o \
                $(SRC)dng/dgnwrite.o \
                $(SRC)dng/dgnopen.o \
                $(SRC)dng/dgnstroke.o
CPLLIB_OBJ =	$(SRC)cpl/cpl_conv.o \
                $(SRC)cpl/cpl_dir.o \
                $(SRC)cpl/cpl_error.o \
                $(SRC)cpl/cpl_multiproc.o \
                $(SRC)cpl/cpl_path.o \
                $(SRC)cpl/cpl_string.o \
                $(SRC)cpl/cpl_vsil_simple.o \
                $(SRC)cpl/cpl_vsisimple.o

OBJ = $(CPLLIB_OBJ) $(DGNLIB_OBJ)

default: dgndump dgnwritetest

$(PROJECT).so.$(SOVER): $(OBJ)
	$(CXX) $(CXXFLAGS) -shared $^ -o $@ $(LDFLAGS)

dgndump: $(SRC)dgndump.c $(PROJECT).so.$(SOVER)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

dgnwritetest: $(SRC)dgnwritetest.c $(PROJECT).so.$(SOVER)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJ) dgndump dgnwritetest $(PROJECT).so.$(SOVER)
