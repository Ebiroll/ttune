# This software is part of libcsdr, a set of simple DSP routines for 
# Software Defined Radio.
#
# Copyright (c) 2014, Andras Retzler <randras@sdr.hu>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of the copyright holder nor the
#      names of its contributors may be used to endorse or promote products
#      derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL ANDRAS RETZLER BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



LIBSOURCES =  csdr/fft_fftw.c csdr/libcsdr_wrapper.c 
#SOURCES = csdr.c $(LIBSOURCES)

cpufeature = $(if $(findstring $(1),$(shell cat /proc/cpuinfo)),$(2))
PARAMS_SSE = $(call cpufeature,sse,-msse) $(call cpufeature,sse2,-msse2) $(call cpufeature,sse3,-msse3) $(call cpufeature,sse4,-msse4) $(call cpufeature,sse4_1,-msse4.1) $(call cpufeature,sse4_2,-msse4.2) -mfpmath=sse 
PARAMS_NEON = -mfloat-abi=hard -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mvectorize-with-neon-quad -funsafe-math-optimizations -Wformat=0 -DNEON_OPTS
#tnx Jan Szumiec for the Raspberry Pi support
PARAMS_RASPI = -mfloat-abi=hard -mcpu=arm1176jzf-s -mfpu=vfp -funsafe-math-optimizations -Wformat=0
PARAMS_ARM = $(if $(call cpufeature,BCM2708,dummy-text),$(PARAMS_RASPI),$(PARAMS_NEON))
PARAMS_SIMD = $(if $(call cpufeature,sse,dummy-text),$(PARAMS_SSE),$(PARAMS_ARM))
PARAMS_LOOPVECT = -O3 -ffast-math -fdump-tree-vect-details -dumpbase dumpvect
PARAMS_LIBS = -g -lm -lrt -lfftw3f -DUSE_FFTW -DLIBCSDR_GPL -DUSE_IMA_ADPCM
PARAMS_SO = -fpic  
PARAMS_MISC = -Wno-unused-result
FFTW_PACKAGE = fftw-3.3.3


### These are for the openvg part shapes.h library

INCLUDEFLAGS=-I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -fPIC
LIBFLAGS=-L/opt/vc/lib  -lEGL -lGLESv2 -lbcm_host -ljpeg
# 
FONTLIB=/usr/share/fonts/truetype/ttf-dejavu
FONTFILES=DejaVuSans.inc  DejaVuSansMono.inc DejaVuSerif.inc

# clean-vect library

all:	ttunevg 
	@echo NOTE: you may have to manually edit Makefile to optimize for your CPU \(especially if you compile on ARM, please edit PARAMS_NEON\).
	@echo Auto-detected optimization parameters: $(PARAMS_SIMD)
	@echo
	#gcc -std=gnu99 $(PARAMS_LOOPVECT) $(PARAMS_SIMD) $(LIBSOURCES) $(PARAMS_LIBS) $(PARAMS_MISC) -fpic -shared -o libcsdr.so
	#-./csdr/parsevect dumpvect*.vect
	#gcc -std=gnu99 $(PARAMS_LOOPVECT) $(PARAMS_SIMD) csdr/csdr.c $(PARAMS_LIBS) -L. -lcsdr $(PARAMS_MISC) -o csdr.bin
libshapes.o:	libshapes.c shapes.h fontinfo.h
	#gcc -O2 -Wall $(INCLUDEFLAGS) -c libshapes.c


oglinit.o:	oglinit.c
	gcc -O2 -Wall $(INCLUDEFLAGS) -c oglinit.c


fonts:	$(FONTFILES)

library: oglinit.o libshapes.o
	gcc $(LIBFLAGS) -shared -o libshapes.so oglinit.o libshapes.o

ttunevg.o:  ttunevg.cpp
	g++ -g -Wall $(INCLUDEFLAGS) -fpermissive  -c ttunevg.cpp -o ttunevg.o

ttunevg:  ttunevg.o 
	g++ -g -Wall $(INCLUDEFLAGS) $(LIBFLAGS) ttunevg.o oglinit.o libshapes.o $(PARAMS_LIBS) $(PARAMS_MISC)  -fpic -lfftw3  -lbcm2835  -lasound  -lpthread -o ttunevg

#########

arm-cross: clean-vect
	#note: this doesn't work since having added FFTW
	arm-linux-gnueabihf-gcc -std=gnu99 -O3 -fshort-double -ffast-math -dumpbase dumpvect-arm -fdump-tree-vect-details -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mvectorize-with-neon-quad -Wno-unused-result -Wformat=0 $(SOURCES) -lm -o ./csdr
clean-vect:
	rm -f dumpvect*.vect
clean: clean-vect
	rm -f libcsdr.so csdr 
install: 
	install -m 0755 libcsdr.so /usr/lib
	install -m 0755 csdr /usr/bin
	install -m 0755 csdr-fm /usr/bin
	ldconfig
uninstall:
	rm /usr/lib/libcsdr.so /usr/bin/csdr /usr/bin/csdr-fm
	ldconfig
