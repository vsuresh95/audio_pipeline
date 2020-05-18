SRCDIR=./src/
BINDIR=./bin/
CC=clang
CXX=clang++
CFLAGS= -Wall -Wno-overloaded-virtual -Wno-unused-variable -fPIC
CXXFLAGS=-std=c++2a -Wall -fPIC
HEADERDIR=./include
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
wd := $(patsubst %/,%,$(dir $(mkfile_path)))
LIBS.dbg=-I./libspatialaudio/build/dbg/include -L$(wd)/libspatialaudio/build/dbg/lib -Wl,-rpath=$(wd)/libspatialaudio/build/dbg/lib -lspatialaudio
LIBS.opt=-I./libspatialaudio/build/opt/include -L$(wd)/libspatialaudio/build/opt/lib -Wl,-rpath=$(wd)/libspatialaudio/build/opt/lib -lspatialaudio

DBG_FLAGS=-g
OPT_FLAGS=-O3

CFiles=$(wildcard $(SRCDIR)*.cpp)
Objects=$(patsubst %.c,%.o,$(wildcard $(CFiles)))

DBG_SO_NAME=plugin.dbg.so
OPT_SO_NAME=plugin.opt.so
.PHONY: all clean

all: $(DBG_SO_NAME)

$(DBG_SO_NAME): audio_component.cpp $(CFiles) libspatialaudio/build/dbg
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(DBG_FLAGS) audio_component.cpp $(CFiles) -shared -o $(DBG_SO_NAME) -I$(HEADERDIR) $(LIBS.dbg) -lpthread -pthread

$(OPT_SO_NAME): audio_component.cpp $(CFiles) libspatialaudio/build/opt
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(OPT_FLAGS) audio_component.cpp $(CFiles) -shared -o $(OPT_SO_NAME) -I$(HEADERDIR) $(LIBS.opt) -lpthread -pthread


solo_opt: $(Objects) libspatialaudio/build/opt 
	$(CC) $(CFLAGS) $(OPT_FLAGS) $(Objects) -o audio -lspatialaudio -lstdc++ -lm -I$(HEADERDIR)

solo_dbg: $(Objects) libspatialaudio/build/dbg
	$(CC) $(CFLAGS) $(DBG_FLAGS) $(Objects) -o audio -lspatialaudio -lstdc++ -lm -I$(HEADERDIR)

libspatialaudio/build/dbg:
	cd libspatialaudio && \
	cmake CMakeLists.txt && \
	cmake -DCMAKE_INSTALL_PREFIX=build/dbg && \
	make && make install 

libspatialaudio/build/opt:
	cd libspatialaudio && \
	cmake CMakeLists.txt && \
	cmake -DCMAKE_INSTALL_PREFIX=build/opt -DCMAKE_BUILD_TYPE=Release && \
	make && make install 

.PHONY: clean
clean:
	rm -rf audio *.o *.so

.PHONY: deepclean
deepclean: clean
	rm -rf libspatialaudio/build
