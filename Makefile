SRCDIR=./src/
BINDIR=./bin/
CC=clang
CXX=clang++
LD=clang++
CFLAGS=-Wall -fPIC -I./include
CXXFLAGS=-std=c++17 -Wall -fPIC -I./include
LD_LIBS=-lpthread -pthread portaudio/lib/.libs/libportaudio.so
LD_LIBS=-lpthread -pthread portaudio/lib/.libs/libportaudio.so
DBG_FLAGS=-g -I./libspatialaudio/build/Debug/include -I./portaudio/include
OPT_FLAGS=-O0 -g -I./libspatialaudio/build/RelWithDebInfo/include -I./portaudio/include

SRCFILES=audio.cpp sound.cpp realtime.cpp
OBJFILES=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRCFILES)))

.PHONY: clean deepclean

solo.dbg: CFLAGS += $(DBG_FLAGS)
solo.dbg: CXXFLAGS += $(DBG_FLAGS)
solo.dbg: LIBSPATIALAUDIO_BUILD_TYPE=Debug
solo.dbg: $(OBJFILES) main.o libspatialaudio/build/Debug/lib/libspatialaudio.a portaudio/lib/.libs/libportaudio.so
	$(LD) $(DBG_FLAGS) $^ -o $@ $(LD_LIBS)

solo.opt: CFLAGS += $(OPT_FLAGS)
solo.opt: CXXFLAGS += $(OPT_FLAGS)
solo.opt: LIBSPATIALAUDIO_BUILD_TYPE=RelWithDebInfo
solo.opt: $(OBJFILES) main.o libspatialaudio/build/RelWithDebInfo/lib/libspatialaudio.a portaudio/lib/.libs/libportaudio.so
	$(LD) $(OPT_FLAGS) $^ -o $@ $(LD_LIBS)

%.o: src/%.cpp libspatialaudio/build
	$(CXX) $(CXXFLAGS) $< -c -o $@

%.o: src/%.c libspatialaudio/build
	$(CC) $(CFLAGS) $< -c -o $@

libspatialaudio/build/Debug/lib/libspatialaudio.a: libspatialaudio/build
libspatialaudio/build/RelWithDebInfo/lib/libspatialaudio.a: libspatialaudio/build

portaudio/lib/.libs/libportaudio.so: libportaudio/build

libspatialaudio/build:
	mkdir -p libspatialaudio/build/$(LIBSPATIALAUDIO_BUILD_TYPE)
	cd libspatialaudio/build; \
	cmake -DCMAKE_INSTALL_PREFIX=$(LIBSPATIALAUDIO_BUILD_TYPE) \
	      -DCMAKE_BUILD_TYPE=$(LIBSPATIALAUDIO_BUILD_TYPE) ..
	$(MAKE) -C libspatialaudio/build
	$(MAKE) -C libspatialaudio/build install

libportaudio/build:
	cd portaudio &&	./configure && make

clean:
	rm -rf audio *.o *.so solo.dbg solo.opt

deepclean: clean
	rm -rf libspatialaudio/build
