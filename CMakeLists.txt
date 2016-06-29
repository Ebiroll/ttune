cmake_minimum_required(VERSION 2.4)
PROJECT(ttune)
#SET (CMAKE_BUILD_TYPE Release)
SET (CMAKE_BUILD_TYPE Debug)


ADD_DEFINITIONS(-D WITH_TRILBY )


IF(WIN32)
ADD_DEFINITIONS(-D WIN32 )
set(CMAKE_CXX_FLAGS "-g -Wall -std=c++11 -static-libstdc++")
ELSE(WIN32)
set(CMAKE_CXX_FLAGS "-g -Wall -std=c++11")
set(CMAKE_C_FLAGS "-std=gnu99  -fpermissive")
#set(CMAKE_C_FLAGS "-std=gnu99 -mfloat-abi=hard -mcpu=arm1176jzf-s -mfpu=vfp -funsafe-math-optimizations -Wformat=0 ")
ENDIF(WIN32)


INCLUDE_DIRECTORIES(
acarsdec-3.2/acars.c
csdr
)

SET (LIBRARY_TYPE STATIC)

ADD_LIBRARY(csdr
csdr/csdr.c
csdr/fft_fftw.c
csdr/fft_rpi.c
csdr/ima_adpcm.c
csdr/libcsdr.c
csdr/libcsdr_gpl.c
csdr/libcsdr_wrapper.c
csdr/fft_fftw.h
csdr/fft_rpi.h
csdr/ima_adpcm.h
csdr/libcsdr_gpl.h
csdr/libcsdr.h
csdr/predefined.h
)

ADD_EXECUTABLE(acarsdec
acarsdec-3.2/acars.c
acarsdec-3.2/acarsdec.c
acarsdec-3.2/air.c
acarsdec-3.2/alsa.c
acarsdec-3.2/dbmgn.c
acarsdec-3.2/msk.c
acarsdec-3.2/output.c
acarsdec-3.2/rtl.c
acarsdec-3.2/acarsdec.h
acarsdec-3.2/acarsserv.h
acarsdec-3.2/syndrom.h
)

ADD_EXECUTABLE(ttune
main.c
)

ADD_EXECUTABLE(acarserv
acarsdec-3.2/acarsserv.c
acarsdec-3.2/dbmgn.c
)


TARGET_LINK_LIBRARIES(ttune csdr)

TARGET_LINK_LIBRARIES(acarsdec sqlite3 pthread m)


TARGET_LINK_LIBRARIES(acarserv sqlite3 pthread m)


