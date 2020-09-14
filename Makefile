SRCDIR=./src/
BINDIR=./bin/
CC=clang-10
CXX=clang++-10
LD=clang++-10
CFLAGS=-Wall -fPIC -I./include
CXXFLAGS=-std=c++17 -Wall -fPIC -I./include -Wno-overloaded-virtual
LD_LIBS=-lpthread -pthread
LD_LIBS=-lpthread -pthread
DBG_FLAGS=-Og -I./libspatialaudio/build/Debug/include -g
OPT_FLAGS=-O3 -I./libspatialaudio/build/Release/include -DNDEBUG
HPP_FILES := $(shell find -L . -name '*.hpp')
HPP_FILES := $(patsubst ./%,%,$(HPP_FILES))

SRCFILES=audio.cpp sound.cpp
DBGOBJFILES=$(patsubst %.c,%.dbg.o,$(patsubst %.cpp,%.dbg.o,$(SRCFILES)))
OPTOBJFILES=$(patsubst %.c,%.opt.o,$(patsubst %.cpp,%.opt.o,$(SRCFILES)))

.PHONY: clean deepclean

plugin.dbg.so: $(DBGOBJFILES) audio_component.dbg.o libspatialaudio/build/Debug/lib/libspatialaudio.a $(HPP_FILES)
	$(LD) $(CXXFLAGS) $(DBG_FLAGS) $(filter-out $(HPP_FILES),$^) -shared -o $@ $(LD_LIBS)

plugin.opt.so: $(OPTOBJFILES) audio_component.opt.o libspatialaudio/build/Release/lib/libspatialaudio.a $(HPP_FILES)
	$(LD) $(CXXFLAGS) $(OPT_FLAGS) $(filter-out $(HPP_FILES),$^) -shared -o $@ $(LD_LIBS)

solo.dbg.exe: $(OBJFILES) main.o libspatialaudio/build/Debug/lib/libspatialaudio.a
	$(LD) $(DBG_FLAGS) $^ -o $@ $(LD_LIBS)

solo.opt.exe: $(OBJFILES) main.o libspatialaudio/build/Release/lib/libspatialaudio.a
	$(LD) $(OPT_FLAGS) $^ -o $@ $(LD_LIBS)

%.opt.o: src/%.cpp libspatialaudio/build/Release/lib/libspatialaudio.a
	$(CXX) $(OPT_FLAGS) $(CXXFLAGS) $< -c -o $@

%.opt.o: src/%.c libspatialaudio/build/Release/lib/libspatialaudio.a
	$(CC) $(OPT_FLAGS) $(CFLAGS) $< -c -o $@

%.dbg.o: src/%.cpp libspatialaudio/build/Debug/lib/libspatialaudio.a
	$(CXX) $(DBG_FLAGS) $(CXXFLAGS) $< -c -o $@

%.dbg.o: src/%.c libspatialaudio/build/Debug/lib/libspatialaudio.a
	$(CC) $(DBG_FLAGS) $(CFLAGS) $< -c -o $@

libspatialaudio/build/Debug/lib/libspatialaudio.a:
	mkdir -p libspatialaudio/build/Debug
	cd libspatialaudio/build; \
	cmake -DCMAKE_INSTALL_PREFIX=Debug -DCMAKE_BUILD_TYPE=Debug ..
	$(MAKE) -C libspatialaudio/build
	$(MAKE) -C libspatialaudio/build install

libspatialaudio/build/Release/lib/libspatialaudio.a:
	mkdir -p libspatialaudio/build/Release
	cd libspatialaudio/build; \
	cmake -DCMAKE_INSTALL_PREFIX=Release -DCMAKE_BUILD_TYPE=Release ..
	$(MAKE) -C libspatialaudio/build
	$(MAKE) -C libspatialaudio/build install

clean:
	rm -rf audio *.o *.so *.exe

deepclean: clean
	rm -rf libspatialaudio/build

.PHONY: tests/run
tests/run:
