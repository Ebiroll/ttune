S = fft/hex/shader_256.hex \
    fft/hex/shader_512.hex \
    fft/hex/shader_1k.hex \
    fft/hex/shader_2k.hex \
    fft/hex/shader_4k.hex \
    fft/hex/shader_8k.hex \
    fft/hex/shader_16k.hex \
    fft/hex/shader_32k.hex \
    fft/hex/shader_64k.hex \
    fft/hex/shader_128k.hex \
    fft/hex/shader_256k.hex \
    fft/hex/shader_512k.hex \
    fft/hex/shader_1024k.hex \
    fft/hex/shader_2048k.hex \
    fft/hex/shader_4096k.hex

C = fft/mailbox.c fft/gpu_fft.c fft/gpu_fft_base.c fft/gpu_fft_twiddles.c fft/gpu_fft_shaders.c

C1D = $(C) fft/hello_fft.c
C2D = $(C) fft/hello_fft_2d.c fft/gpu_fft_trans.c

H1D = fft/gpu_fft.h fft/mailbox.h 
H2D = fft/gpu_fft.h fft/mailbox.h fft/gpu_fft_trans.h fft/hello_fft_2d_bitmap.h

F = -I fft -lrt -lm -ldl

all:	hello_fft.bin hello_fft_2d.bin
	g++  -fpermissive -I /usr/local/include/ main.c -o ttune    -lbcm2835  -lasound

hello_fft.bin:	$(S) $(C1D) $(H1D)
	gcc -o hello_fft.bin $(F) $(C1D)

hello_fft_2d.bin:	$(S) fft/hex/shader_trans.hex $(C2D) $(H2D)
	gcc -o hello_fft_2d.bin $(F) $(C2D)

clean:
	rm -f *.bin

