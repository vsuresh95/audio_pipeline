SRCDIR=./src/
BINDIR=./bin/
CC=clang
CXX=clang++
LD=clang++
CFLAGS=-Wall -fPIC -I./include
CXXFLAGS=-std=c++17 -Wall -fPIC -I./include -Wno-overloaded-virtual
LD_LIBS=-lpthread -pthread
LD_LIBS=-lpthread -pthread
DBG_FLAGS=-g -I./libspatialaudio/build/Debug/include
OPT_FLAGS=-O3 -I./libspatialaudio/build/Release/include
HPP_FILES ?= $(shell find -L . -name '*.hpp')

SRCFILES=audio.cpp sound.cpp
DBGOBJFILES=$(patsubst %.c,%.dbg.o,$(patsubst %.cpp,%.dbg.o,$(SRCFILES)))
OPTOBJFILES=$(patsubst %.c,%.opt.o,$(patsubst %.cpp,%.opt.o,$(SRCFILES)))

DBG_SO_NAME=plugin.dbg.so
OPT_SO_NAME=plugin.opt.so

.PHONY: clean deepclean

$(DBG_SO_NAME): CFLAGS += $(DBG_FLAGS)
$(DBG_SO_NAME): CXXFLAGS += $(DBG_FLAGS)
$(DBG_SO_NAME): LIBSPATIALAUDIO_BUILD_TYPE=Debug
$(DBG_SO_NAME): $(DBGOBJFILES) audio_component.dbg.o libspatialaudio/build/Debug/lib/libspatialaudio.a $(HPP_FILES)
	$(LD) $(CXXFLAGS) $(DBG_FLAGS) $(DBGOBJFILES) audio_component.dbg.o libspatialaudio/build/Debug/lib/libspatialaudio.a -shared -o $@ $(LD_LIBS)

$(OPT_SO_NAME): CFLAGS += $(OPT_FLAGS)
$(OPT_SO_NAME): CXXFLAGS += $(OPT_FLAGS)
$(OPT_SO_NAME): LIBSPATIALAUDIO_BUILD_TYPE=Release
$(OPT_SO_NAME): $(OPTOBJFILES) audio_component.opt.o libspatialaudio/build/Release/lib/libspatialaudio.a $(HPP_FILES)
	$(LD) $(CXXFLAGS) $(OPT_FLAGS) $(OPTOBJFILES) audio_component.opt.o libspatialaudio/build/Release/lib/libspatialaudio.a -shared -o $@ $(LD_LIBS)

solo.dbg: CFLAGS += $(DBG_FLAGS)
solo.dbg: CXXFLAGS += $(DBG_FLAGS)
solo.dbg: LIBSPATIALAUDIO_BUILD_TYPE=Debug
solo.dbg: $(OBJFILES) main.o libspatialaudio/build/Debug/lib/libspatialaudio.a
	$(LD) $(DBG_FLAGS) $^ -o $@ $(LD_LIBS)

solo.opt: CFLAGS += $(OPT_FLAGS)
solo.opt: CXXFLAGS += $(OPT_FLAGS)
solo.opt: LIBSPATIALAUDIO_BUILD_TYPE=Release
solo.opt: $(OBJFILES) main.o libspatialaudio/build/Release/lib/libspatialaudio.a
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
	rm -rf audio *.o *.so solo.dbg solo.opt build/

deepclean: clean
	rm -rf libspatialaudio/build
