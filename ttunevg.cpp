/*
 * ttunevg
 *
 * Trilby HAT Tuning Utility , with openvg support
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
 * File:   ttune.c
 * Author: Keith Frewin, Olof Astrand
 *
 * Created on 13 October 2015, 14:22
 * 
 * Version History
 * dd/mm/yyyy vers comments
 * 15/04/2016 1.04 Initial release
 * 15/04/2016 OpenVG support, Checks keyboard
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <bcm2835.h>
#include "alsa/asoundlib.h"
#include <math.h>

// Keyboard hit check
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "shapes.h"

//#define NFRAMES 12000
#define NFRAMES 16384

static char *device = "default";                        /* playback device */
snd_output_t *output = NULL;
unsigned char buffer[2*NFRAMES + 3];                          /* some random data */

#define SAMPLING_RATE 48000

#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include "ttunevg.h"

/* Example thread */
#if 0
// Simple example
{
    fftw_complex *in, *out;
    fftw_plan p;
    ...
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    ...
    fftw_execute(p); /* repeat as needed */
    ...
    fftw_destroy_plan(p);
    fftw_free(in); fftw_free(out);
}
#endif

    /* Layout
      Text                     WTF_WIDTH  */

#define  WTF_WIDTH  512


pthread_t thread_id;

fft_holder thread_data[MAX_T];

int last_thread_ix=-1;

void* thread_function(void *idx)
{
    for(int qk=0;qk<100;qk++)
    {
        printf(".");
    }
    printf("fftw_execute %d\n",idx);
    int *i=(int *)idx;
    fftw_execute(thread_data[*i].plan);

    //char *a = malloc(10);
    //strcpy(a,"hello world");
    pthread_exit((void *)idx);
}

void init_fft() {

    for (int j=0;j<MAX_T;j++)
    {

        thread_data[j].index=j;
        thread_data[j].N_samples=NFRAMES;
        // Only need N/2 values for output?
        thread_data[j].output= (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * thread_data[j].N_samples);
        thread_data[j].windowed_input= (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * thread_data[j].N_samples);
        thread_data[j].plan=fftw_plan_dft_1d(thread_data[j].N_samples, thread_data[j].windowed_input, thread_data[j].output, FFTW_FORWARD, FFTW_ESTIMATE);
    }
}


int start_fft_thread(int ix)
{

    pthread_create (&thread_id, NULL,&thread_function, &thread_data[ix].index);

}

int join_fft_thread()
{
    int b;

    pthread_join(thread_id,(void **)&b);  //here we are reciving one pointer

    printf("b is %d\n",b);

}


/* To here */


void generate_freq(int *buffer, size_t count, float volume, float freq)
{
  size_t pos; // sample number we're on

  for (pos = 0; pos < count; pos++) {
    float a = 2 * 3.14159f * freq * pos / SAMPLING_RATE;
    float v = sin(a) * volume;
    // convert from [-1.0,1.0] to [-32767,32767]:
    //buffer[pos] = remap_level_to_signed_16_bit(v);
  }
}

// Returns true if keyboard was hit
int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}


void sweep(double f_start, double f_end, double interval, int n_steps) {

#if 0
    // Single freq
    size_t pos; // sample number we're on

    int freq=440;
    for (pos = 0; pos < n_steps; pos++) {
      float a = 2 * M_PI * freq * pos / 44100;
      float v = sin(a);
      //fwrite (&v,	sizeof(float),1, stdout);
      short tmp_shrt=10*v;
      fwrite (&tmp_shrt,	sizeof(short),1, stdout);
    }
#endif



    for (int i = 0; i < n_steps; ++i) {
        double delta = i / (float)n_steps;
        double t = interval * delta;
        double phase = 2 * M_PI * t * (f_start + (f_end - f_start) * delta / 2);
        //while (phase > 2 * M_PI)  phase -= 2 * M_PI; // optional
        {
            float tmp=sin(phase);
            // Float out
            fwrite (&tmp,	sizeof(float),1, stdout);
            //short tmp_shrt=32000*tmp;
            //fwrite (&tmp_shrt,	sizeof(short),1, stdout);
            //printf("%f\n", sin(phase));
        }
        //printf("%f %f %f", t, phase * 180 / PI, 3 * sin(phase));
    }
}


//

short triangle=0;
short tridelta=300;

 //Generate  NFRAMES of triangle wave sweep, increase frequency for each call
void short_sweep(short *data) {
    tridelta++;
    for (int i = 0; i < NFRAMES; ++i) {
        *data=triangle;
        triangle+=tridelta;
        *data=*data/2;
        data++;
    }
}


int width,height;

char AM, WB, HF;

unsigned long FreqInHz;


void drawOpenVG() {
    char Buffer[128];
    Start(width, height);				   // Start the picture
    Background(0, 0, 0);				   // Black background
    Fill(44, 77, 232, 1);				   // Big blue marble
    Roundrect(0,0,width-WTF_WIDTH, height,20,20);		   // The "world"
    Fill(255, 255, 255, 1);				   // White text
    Text(20, height -20 , "ttunevg", SerifTypeface, 20);	// Info
    if (AM) {
         sprintf(Buffer,"%d kHz AM %s", FreqInHz, WB ? " wide" : "");
    } else {
        sprintf(Buffer,"%d kHz FM %s", FreqInHz, WB ? " wide" : "");
    }
    Text(20, height -60 ,Buffer, SerifTypeface, 20);	// Info

    End();
}

/*
 * 
 */
int main(int argc, char** argv) {
    int i;
    char Buf[100];
    uint8_t Result;
    char RegNum[10];
    int ArgNum;
    int CharNum;
    char *ArgStr;
    int EmitSound = 0;
    int StdOutSound = 0;
    int TestSound = 0;

    int err;
    int Debug = 0;
    unsigned char ControlByte;
    unsigned int BytesAvail;
    int audio_val;
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;
    int DisplayRSSI = 0;
    
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
                    if (ArgStr[CharNum] == 'o') {StdOutSound = 1;}
                    if (ArgStr[CharNum] == 'd') {Debug = 1;}
                    if (ArgStr[CharNum] == 'r') {DisplayRSSI = 1;}
                    if (ArgStr[CharNum] == 't') {TestSound = 1;}
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
        printf("    -o   Output to stdout float value of sound (Ctrl-C to exit)\n");
        printf("    -t   Output Sweep from 10 to 10kHz in 60 seconds\n");
        printf("\n");
        return (EXIT_SUCCESS);
     }
    if (WB) ControlByte = 0x02;
    else if (AM) ControlByte = 0x01;
    else ControlByte = 0x00;
    if (HF) ControlByte |= 0x04;
        
    /// Initialise openvg and create fft thread
    ///
    init_fft();

    init(&width, &height);				   // Graphics initialization

    ///



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
    if (Debug) printf("Firmware version is %d.%02d\n", Buf[3], Buf[4]);
    
    if (TestSound && !EmitSound) {
        sweep(10,10000,60,48000*60);
        sweep(10000,10,60,48000*60);
        exit(0);
    }


    if (FreqInHz > 0)
    {
        if (!StdOutSound)
        {
            printf("Trilby RF tuning utility v1.04\n\n");
            printf("Tuning to %d kHz\n", FreqInHz);
             if (HF) printf("HF");
            else printf("VHF/UHF");
            printf(", ");
            if (WB) printf("Wide Band FM");
            else if (AM) printf("AM");
            else printf("Narrow Band FM");
            printf("\n\n");
        }
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
    

    if (EmitSound || StdOutSound)
    {
        fprintf(stderr,"Playback enabled: q to quit, Ctrl-C to exit\n");
	if (EmitSound) 
        {
	  if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr,"Playback open error: %s\n", snd_strerror(err));
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
	}
        Buf[0] = 0x00;  // write
        Buf[1] = 0x00;  // addr hi
        Buf[2] = 0x56;  // addr lo
        Buf[3] = 0x01;  // under sample factor
        Buf[4] = 0x80;  // audio mode = enable
        bcm2835_spi_transfern(Buf, 5);
        while(EmitSound || StdOutSound) {
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
            bcm2835_spi_transfern((char *)buffer, sizeof(buffer));
            // Generate testsound for output
            if (TestSound) {
               unsigned char *silly= buffer +3;
               short_sweep((short *)(silly));
            }

            if (StdOutSound) {
                unsigned char *silly= buffer+3;
                short *tmp_ptr=(short *) silly;
                //fwrite (tmp_ptr, sizeof(short),NFRAMES,stdout);
   
                for(int q=0;q< NFRAMES;q++) {
                  float tmp=(float) *silly;
                  tmp=*tmp_ptr;
                  fwrite (&tmp,	sizeof(float),1, stdout);
                  tmp_ptr++;
                }
		  
            }
            else
            {
                unsigned char *charp= buffer+3;
                frames = snd_pcm_writei(handle, charp, NFRAMES);
                if (frames < 0)
                        frames = snd_pcm_recover(handle, frames, 0);
                if (frames < 0) {
                        printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
                        break;
                }
                if (frames > 0 && frames < NFRAMES)
                        printf("Short write (expected %li, wrote %li)\n", NFRAMES, frames);
            }

	    // Check keyboard

	    if (_kbhit())
        {
            char c=getchar();

            switch(c)
              {
            case 'o':
              tridelta-=30;
              //printf("p-pressed\n");
              // Increase freqeuncy
              break;

              case 'p':
                tridelta+=30;
                //printf("p-pressed\n");
                // Increase freqeuncy
                break;
               case 'q':
               case 'Q':
                EmitSound=0;
                StdOutSound=0;
                break;
              default:
                printf("pressed %d\n",c);

                break;
              }
          }
        else
        {
	  //printf(".");
        }
        drawOpenVG();


        }

        snd_pcm_close(handle);
        
    }
    
    if (Debug) printf("calling bcm2835_close()\n");
    bcm2835_close();
    if (Debug) printf("Done\n");
    finish();
    
    return (EXIT_SUCCESS);
}

