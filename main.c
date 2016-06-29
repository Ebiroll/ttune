/*
 * ttune
 *
 * Trilby HAT Tuning Utility 
 *
 * Copyright (c) 2015-2016 Kinetic Avionics Ltd
 * www.kinetic.co.uk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * File:   main.c
 * Author: Keith Frewin
 *
 * Created on 13 October 2015, 14:22
 * 
 * Version History
 * dd/mm/yyyy vers comments
 * 15/04/2016 1.04 Initial release
 *  
 */

#include <stdio.h>
#include <stdlib.h>
#include <bcm2835.h>
#include "alsa/asoundlib.h"

#define NFRAMES 12000

static char *device = "default";                        /* playback device */
snd_output_t *output = NULL;
unsigned char buffer[2*NFRAMES + 3];                          /* some random data */

/*
 * 
 */
int main(int argc, char** argv) {
    int i;
    unsigned long FreqInHz;
    char Buf[100];
    uint8_t Result;
    char RegNum[10];
    int ArgNum;
    int CharNum;
    char *ArgStr;
    char AM, WB, HF;
    int EmitSound = 0;
    int err;
    int Debug = 0;
    unsigned char ControlByte;
    unsigned int BytesAvail;
    int audio_val;
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;
    int DisplayRSSI = 0;
    
    printf("Trilby RF tuning utility v1.04\n\n");
    if (argc  > 1)
    {
        FreqInHz = 0;
        AM = 0;
        WB = 0;
        HF = 0;
        for (ArgNum = 1; ArgNum < argc; ++ArgNum)
        {
            ArgStr =argv[ArgNum];
            if (ArgStr[0] == '-') 
            {
                for (CharNum = 1; CharNum < strlen(ArgStr); CharNum++)
                {
                    if (ArgStr[CharNum] == 'a') {AM = 1; WB = 0;}
                    if (ArgStr[CharNum] == 'w') {WB = 1; AM = 0;}
                    if (ArgStr[CharNum] == 'n') {WB = 0; AM = 0;}
                    if (ArgStr[CharNum] == 'h') {HF = 1;}
                    if (ArgStr[CharNum] == 's') {EmitSound = 1;}
                    if (ArgStr[CharNum] == 'd') {Debug = 1;}
                    if (ArgStr[CharNum] == 'r') {DisplayRSSI = 1;}
                }
            }
            else
            {
                FreqInHz = atoi(argv[ArgNum]);
            }
        }
    }
    else
    {
        printf("Usage: ttune [OPTIONS] n\n", FreqInHz);
        printf("where: n = frequency in kHz\n\n", FreqInHz);
        printf("OPTIONS\n\n");
        printf("    -a   AM demodulation\n");
        printf("    -n   Narrow band FM demodulation (default)\n");
        printf("    -w   Wide band FM demodulation\n");
        printf("    -h   Use HF antenna and up-converter\n");
        printf("    -v   Use VHF/UHF antenna (default)\n");
        printf("    -s   Emit sound on playback channel (Ctrl-C to exit)\n");
        printf("\n");
        return (EXIT_SUCCESS);
     }
    if (WB) ControlByte = 0x02;
    else if (AM) ControlByte = 0x01;
    else ControlByte = 0x00;
    if (HF) ControlByte |= 0x04;
        
    if (!bcm2835_init())
        return 1;
    if (Debug) printf("bcm2835_init() returned OK\n");
    bcm2835_spi_begin();
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    
    Buf[0] = 0x01;  // read
    Buf[1] = 0x00;  // addr hi
    Buf[2] = 0x60;  // addr lo
    bcm2835_spi_transfern(Buf, 5);    
    printf("Firmware version is %d.%02d\n", Buf[3], Buf[4]);
    
    if (FreqInHz > 0)
    {
        printf("Tuning to %d kHz\n", FreqInHz);
        if (HF) printf("HF");
        else printf("VHF/UHF");
        printf(", ");
        if (WB) printf("Wide Band FM");
        else if (AM) printf("AM");
        else printf("Narrow Band FM");
        printf("\n\n");
        Buf[0] = 0x00;  // write
        Buf[1] = 0x0;  // addr hi for control
        Buf[2] = 0x40;  // addr lo for control
        Buf[3] = ControlByte;
        bcm2835_spi_transfern(Buf, 4);

        Buf[0] = 0x00;  // write
        Buf[1] = 0x0;  // addr hi for tuning
        Buf[2] = 0xf0;  // addr lo for tuning
        Buf[3] = FreqInHz & 0xff;  // RF frequency - low byte
        Buf[4] = (FreqInHz >> 8) & 0xff;
        Buf[5] = (FreqInHz >> 16) & 0xff;
        Buf[6] = (FreqInHz >> 24) & 0xff;  // high byte
        Buf[7] = 0xa0;      // command to retune
        bcm2835_spi_transfern(Buf, 8);
    }
    
    if (EmitSound)
    {
        printf("Playback enabled: Ctrl-C to exit\n");
        if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        if ((err = snd_pcm_set_params(handle,
                                      SND_PCM_FORMAT_S16_LE,
                                      SND_PCM_ACCESS_RW_INTERLEAVED,
                                      1,
                                      48000,
                                      1,
                                      250000)) < 0) {   /* 0.5sec */
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        Buf[0] = 0x00;  // write
        Buf[1] = 0x00;  // addr hi
        Buf[2] = 0x56;  // addr lo
        Buf[3] = 0x01;  // under sample factor
        Buf[4] = 0x80;  // audio mode = enable
        bcm2835_spi_transfern(Buf, 5);
        while(1) {
            if (DisplayRSSI)
            {
                Buf[0] = 0x01;  // read
                Buf[1] = 0x00;  // addr hi
                Buf[2] = 0xf8;  // addr lo
                bcm2835_spi_transfern(Buf, 7);
                printf("RSSI = %d, AGC = %d, %02x %02x\n", Buf[3], Buf[4], Buf[5], Buf[6]);
            }
            do
            {
                Buf[0] = 0x01;  // read
                Buf[1] = 0x00;  // addr hi
                Buf[2] = 0x54;  // addr lo
                bcm2835_spi_transfern(Buf, 6);
                BytesAvail = Buf[4]*265 + Buf[3];

                if (Debug) printf("Buffer count = %04x\n", BytesAvail);
            }
            while (BytesAvail < NFRAMES);
            
            buffer[0] = 0x01;  // read
            buffer[1] = 0x00;  // addr hi
            buffer[2] = 0x50;  // addr lo
            bcm2835_spi_transfern(buffer, sizeof(buffer));
                frames = snd_pcm_writei(handle, buffer+3, NFRAMES);
                if (frames < 0)
                        frames = snd_pcm_recover(handle, frames, 0);
                if (frames < 0) {
                        printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
                        break;
                }
                if (frames > 0 && frames < NFRAMES)
                        printf("Short write (expected %li, wrote %li)\n", NFRAMES, frames);
            
        }
        snd_pcm_close(handle);
        
    }
    
    if (Debug) printf("calling bcm2835_close()\n");
    bcm2835_close();
    if (Debug) printf("Done\n");
    
    return (EXIT_SUCCESS);
}

