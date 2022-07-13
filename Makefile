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

CROSS_COMPILE ?= riscv64-unknown-linux-gnu-

ESP_BUILD_DRIVERS = ${PWD}/esp-build/drivers

ESP_DRIVERS ?= $(ESP_ROOT)/soft/common/drivers
ESP_DRV_LINUX = $(ESP_DRIVERS)/linux

ESP_INCDIR += -I${ESP_DRIVERS}/common/include
ESP_INCDIR += -I${ESP_DRIVERS}/linux/include

ESP_LD_LIBS += -L$(ESP_BUILD_DRIVERS)/contig_alloc
ESP_LD_LIBS += -L$(ESP_BUILD_DRIVERS)/test
ESP_LD_LIBS += -L$(ESP_BUILD_DRIVERS)/libesp

ESP_LD_FLAGS += -lrt
ESP_LD_FLAGS += -lesp
ESP_LD_FLAGS += -ltest
ESP_LD_FLAGS += -lcontig

OPT_FLAGS += $(ESP_INCDIR) $(ESP_LD_LIBS)
LD_LIBS += $(ESP_LD_FLAGS)

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

libspatialaudio/build/RelWithDebInfo/lib/libspatialaudio.a: esp-libs
	mkdir -p libspatialaudio/build/RelWithDebInfo
	cd libspatialaudio/build; \
	cmake -DCMAKE_INSTALL_PREFIX=RelWithDebInfo -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	      -DCMAKE_AR=/home/espuser/riscv/bin/riscv64-unknown-linux-gnu-ar \
	      -DCMAKE_RANLIB=/home/espuser/riscv/bin/riscv64-unknown-linux-gnu-ranlib \
		  -DBUILD_SHARED_LIBS=OFF ..
	$(MAKE) -C libspatialaudio/build
	$(MAKE) -C libspatialaudio/build install

clean:
	rm -rf audio *.o *.so *.exe

deepclean: clean
	rm -rf libspatialaudio/build

.PHONY: tests/run
tests/run:

esp-build:
	@mkdir -p $(ESP_BUILD_DRIVERS)/contig_alloc
	@mkdir -p $(ESP_BUILD_DRIVERS)/esp
	@mkdir -p $(ESP_BUILD_DRIVERS)/esp_cache
	@mkdir -p $(ESP_BUILD_DRIVERS)/libesp
	@mkdir -p $(ESP_BUILD_DRIVERS)/probe
	@mkdir -p $(ESP_BUILD_DRIVERS)/test
	@mkdir -p $(ESP_BUILD_DRIVERS)/utils/baremetal
	@mkdir -p $(ESP_BUILD_DRIVERS)/utils/linux
	@ln -sf $(ESP_DRV_LINUX)/contig_alloc/* $(ESP_BUILD_DRIVERS)/contig_alloc
	@ln -sf $(ESP_DRV_LINUX)/esp/* $(ESP_BUILD_DRIVERS)/esp
	@ln -sf $(ESP_DRV_LINUX)/esp_cache/* $(ESP_BUILD_DRIVERS)/esp_cache
	@ln -sf $(ESP_DRV_LINUX)/driver.mk $(ESP_BUILD_DRIVERS)
	@ln -sf $(ESP_DRV_LINUX)/include $(ESP_BUILD_DRIVERS)
	@ln -sf $(ESP_DRV_LINUX)/../common $(ESP_BUILD_DRIVERS)/../common

esp-build-distclean:
	$(QUIET_CLEAN)$(RM) -rf esp-build

esp-libs: esp-build
	CROSS_COMPILE=$(CROSS_COMPILE) DRIVERS=$(ESP_DRV_LINUX) $(MAKE) -C $(ESP_BUILD_DRIVERS)/contig_alloc/ libcontig.a
	cd $(ESP_BUILD_DRIVERS)/test; CROSS_COMPILE=$(CROSS_COMPILE) BUILD_PATH=$$PWD $(MAKE) -C $(ESP_DRV_LINUX)/test
	cd $(ESP_BUILD_DRIVERS)/libesp; CROSS_COMPILE=$(CROSS_COMPILE) BUILD_PATH=$$PWD $(MAKE) -C $(ESP_DRV_LINUX)/libesp
	cd $(ESP_BUILD_DRIVERS)/utils; CROSS_COMPILE=$(CROSS_COMPILE) BUILD_PATH=$$PWD $(MAKE) -C $(ESP_DRV_LINUX)/utils