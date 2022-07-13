# use_integ=$(ILLIXR_INTEGRATION)

SRCDIR=./src/
BINDIR=./bin/
# CC=clang-10
CC=riscv64-unknown-linux-gnu-gcc
# CXX=clang++-10
# LD=clang++-10
CXX=riscv64-unknown-linux-gnu-g++
LD=riscv64-unknown-linux-gnu-g++
# CFLAGS=-Wall -fPIC -I./include -DILLIXR_INTEGRATION=$(use_integ)
CFLAGS=-Wall -fPIC -I./include
# CXXFLAGS=-std=c++17 -Wall -fPIC -I./include -I./portaudio/include -Wno-overloaded-virtual -DILLIXR_INTEGRATION=$(use_integ)
CXXFLAGS=-std=c++17 -Wall -fPIC -I./include -I./portaudio/include -Wno-overloaded-virtual
LD_LIBS=-lpthread -pthread
DBG_FLAGS=-Og -g -I./libspatialaudio/build/Debug/include
OPT_FLAGS=-O3 -DNDEBUG -I./libspatialaudio/build/RelWithDebInfo/include
HPP_FILES := $(shell find -L . -name '*.hpp')
HPP_FILES := $(patsubst ./%,%,$(HPP_FILES))

SRCFILES=audio.cpp sound.cpp
DBGOBJFILES=$(patsubst %.c,%.dbg.o,$(patsubst %.cpp,%.dbg.o,$(SRCFILES)))
OPTOBJFILES=$(patsubst %.c,%.opt.o,$(patsubst %.cpp,%.opt.o,$(SRCFILES)))


.PHONY: clean deepclean

plugin.dbg.so: $(DBGOBJFILES) audio_component.dbg.o libspatialaudio/build/Debug/lib/libspatialaudio.a $(HPP_FILES)
	$(LD) $(CXXFLAGS) $(DBG_FLAGS) $(filter-out $(HPP_FILES),$^) -shared -o $@ $(LD_LIBS)

plugin.opt.so: $(OPTOBJFILES) audio_component.opt.o libspatialaudio/build/RelWithDebInfo/lib/libspatialaudio.a $(HPP_FILES)
	$(LD) $(CXXFLAGS) $(OPT_FLAGS) $(filter-out $(HPP_FILES),$^) -shared -o $@ $(LD_LIBS)

solo.dbg.exe: $(HPP_FILES) $(DBGOBJFILES) main.dbg.o libspatialaudio/build/Debug/lib/libspatialaudio.a
	$(LD) $(CXXFLAGS) $(DBG_FLAGS) $(filter-out $(HPP_FILES),$^) $(LD_LIBS) -o $@ 

solo.opt.exe: $(HPP_FILES) $(OPTOBJFILES) main.opt.o libspatialaudio/build/RelWithDebInfo/lib/libspatialaudio.a
	$(LD) $(CXXFLAGS) $(OPT_FLAGS) $(filter-out $(HPP_FILES),$^) $(LD_LIBS) -o $@ 

%.opt.o: src/%.cpp libspatialaudio/build/RelWithDebInfo/lib/libspatialaudio.a
	$(CXX) $(OPT_FLAGS) $(CXXFLAGS) $< -c -o $@

%.opt.o: src/%.c libspatialaudio/build/RelWithDebInfo/lib/libspatialaudio.a
	$(CC) $(OPT_FLAGS) $(CFLAGS) $< -c -o $@

%.dbg.o: src/%.cpp libspatialaudio/build/Debug/lib/libspatialaudio.a
	$(CXX) $(DBG_FLAGS) $(CXXFLAGS) $< -c -o $@

%.dbg.o: src/%.c libspatialaudio/build/Debug/lib/libspatialaudio.a
	$(CC) $(DBG_FLAGS) $(CFLAGS) $< -c -o $@

# libspatialaudio/build/Debug/lib/libspatialaudio.a:
# 	mkdir -p libspatialaudio/build/Debug
# 	cd libspatialaudio/build; \
# 	cmake -DCMAKE_INSTALL_PREFIX=Debug -DCMAKE_BUILD_TYPE=Debug -DILLIXR_INTEGRATION=$(use_integ) ..
# 	$(MAKE) -C libspatialaudio/build
# 	$(MAKE) -C libspatialaudio/build install

libspatialaudio/build/Debug/lib/libspatialaudio.a:
	mkdir -p libspatialaudio/build/Debug
	cd libspatialaudio/build; \
	cmake -DCMAKE_INSTALL_PREFIX=Debug -DCMAKE_BUILD_TYPE=Debug ..
	$(MAKE) -C libspatialaudio/build
	$(MAKE) -C libspatialaudio/build install

# libspatialaudio/build/RelWithDebInfo/lib/libspatialaudio.a:
# 	mkdir -p libspatialaudio/build/RelWithDebInfo
# 	cd libspatialaudio/build; \
# 	cmake -DCMAKE_INSTALL_PREFIX=RelWithDebInfo -DCMAKE_BUILD_TYPE=RelWithDebInfo -DILLIXR_INTEGRATION=$(use_integ) ..
# 	$(MAKE) -C libspatialaudio/build
# 	$(MAKE) -C libspatialaudio/build install

libspatialaudio/build/RelWithDebInfo/lib/libspatialaudio.a:
	mkdir -p libspatialaudio/build/RelWithDebInfo
	cd libspatialaudio/build; \
	cmake -DCMAKE_INSTALL_PREFIX=RelWithDebInfo -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF ..
	$(MAKE) -C libspatialaudio/build
	$(MAKE) -C libspatialaudio/build install

clean:
	rm -rf audio *.o *.so *.exe

deepclean: clean
	rm -rf libspatialaudio/build

.PHONY: tests/run
tests/run:
