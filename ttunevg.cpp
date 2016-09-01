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
#include "fft.h"

// Keyboard hit check
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "shapes.h"
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include "ttunevg.h"

static const char *device = "default";                        /* playback device */
snd_output_t *output = NULL;
unsigned char buffer[2 * NFRAMES + 3];                          /* Buffer data for input */

#define SAMPLING_RATE 48000

#define WALPHA(x)  (x << 24)
#define WBLUE(x)   (x << 16)
#define WGREEN(x)  (x << 8)
#define WRED(x)    (x)


/* FFT thread */

double windowdata[NFRAMES];

#define  WTF_WIDTH  512

#define  WTF_HEIGHT  1024

// 32 bit RGBA waterfall
unsigned int imageData[WTF_WIDTH*(WTF_HEIGHT+1)];

#define FFT_WIDTH 512

#define FFT_HEIGHT 128

// 32 bit RGBA FFT
unsigned int fftImage[FFT_WIDTH*(FFT_HEIGHT+1)];


pthread_t thread_id;

fft_holder thread_data[MAX_T];

int last_thread_ix=0;
int first_thread=1;

void* thread_function(void *idx)
{
    int *i=(int *)idx;
    // Apply window

    for (int q=0;q<NFRAMES;q++) {
      thread_data[*i].windowed_input[q] = thread_data[*i].windowed_input[q] * windowdata[q];
    }

    //printf("fftw_execute %d\n",*i);
    fftw_execute(thread_data[*i].plan);

    pthread_exit((void *)idx);
}

void init_fft() {

    // Initialze window function
    compute_window(windowdata,HANNING_WINDOW,NFRAMES);

    // Set waterfall image to red
    for (int ii=0;ii<WTF_WIDTH*WTF_HEIGHT;ii++) {
      imageData[ii]=WRED(100);
    }

    // Initialze data for the fft threads
    for (int j=0;j<MAX_T;j++)
    {
        thread_data[j].index=j;
        thread_data[j].N_samples=NFRAMES;
        // Only need N/2 values for output?
        thread_data[j].output= (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * thread_data[j].N_samples);
        thread_data[j].windowed_input= (double*) fftw_malloc(sizeof(double) * thread_data[j].N_samples);
        // Maybe use, fftw_plan_dft_r2c_1d
        //thread_data[j].plan=fftw_plan_dft_1d(thread_data[j].N_samples, thread_data[j].windowed_input, thread_data[j].output, FFTW_FORWARD, FFTW_ESTIMATE);
        thread_data[j].plan = fftw_plan_dft_r2c_1d(thread_data[j].N_samples, thread_data[j].windowed_input, thread_data[j].output, FFTW_ESTIMATE);
    }
}

void start_fft_thread(int ix,unsigned char *buffer)
{
  // Copy data in main thread..
  short *tmp_ptr=(short *) buffer;
  double tmp;
  for (int q=0;q<NFRAMES;q++) {
    tmp=(double) *tmp_ptr;
    tmp_ptr++;
    // TODO add windowing function
    thread_data[ix].windowed_input[q]=tmp;
  }

  pthread_create (&thread_id, NULL,&thread_function, &thread_data[ix].index);

}


void join_fft_thread()
{
    int b;

    int line=last_thread_ix%WTF_HEIGHT;

    pthread_join(thread_id,(void **)&b);  //here we are reciving one pointer

    /* normalize  */
    //w_pwr = 0.0;
    //for (i = 0; i < N; i++) {
    //  w_pwr += window[i] * window[i];
    //}
    //for (i = 0; i < N; i++) {
    // window[i] /= sqrt(w_pwr);
    //}


    // Set fft image
    for(int x=0;x<FFT_WIDTH;x++) {
        int y=0;
        int freq = x*2;
        double *result = (double *) thread_data[last_thread_ix%4].output;
        double power = sqrt(result[freq*2] * result[freq*2 ] + result[1+ freq*2 ] * result[1 + freq*2]);
        int ipower = power/10000;
	//int phase = result[1 + freq*2]/100;
	//printf("%d ",phase);
        //printf("%d ",ipower);
        //if (phase<0) phase=phase*-1;
        //if (phase>128) phase=128;
        //if (phase<0) phase=0;

        if (ipower>FFT_HEIGHT) ipower=FFT_HEIGHT;
        if (ipower <0) ipower=0;
	

        for (y=0;y<ipower;y++)
        {
          fftImage[x+y*FFT_WIDTH]=WGREEN(255); // | WBLUE(phase);
        }
        while (y<FFT_HEIGHT) {
            fftImage[x+y*FFT_WIDTH]=0;
            y++;
        }
    }

    for (int j=0;j<WTF_WIDTH;j++)
    {
        //
        //int freq = j*thread_data[last_thread_ix].N_samples / 2*WTF_WIDTH;
        int freq = j*4;
        double *result = (double *) thread_data[last_thread_ix%4].output;
        //if (freq >= NFRAMES/2) freq = NFRAMES/2 - 1;
        double power = sqrt(result[freq*2] * result[freq*2 ] + result[1+ freq*2 ] * result[1 + freq*2]);
        int ipower = power/2000;
        //printf("%d ",ipower);

        // Clip power
        if (ipower>255) ipower=255;
        if (ipower <0) ipower=0;

        imageData[line*WTF_WIDTH+j]=WGREEN(ipower);
        //imageData[WTF_WIDTH*WTF_HEIGHT-(line*WTF_WIDTH+j)]=0;
    }


    //printf("b is %d\n",b);

}


/* To here */


void generate_freq(short *buffer, size_t count, float volume, float freq)
{
  static size_t pos=0; // sample number we're on
  size_t endpos=pos+count;

  int j=0;
  for (pos = pos; pos < endpos; pos++) {
    float a = 2 * 3.14159f * freq * pos / SAMPLING_RATE;
    float v = sin(a) * volume;
    // convert from [-1.0,1.0] to [-32767,32767]:
    buffer[j++] = 32767.0*(v);
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
short tridelta=440;
float frequency=440.0f;
int TestSound = 0;
int drawHelpText=0;

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


int width=1280,height=1024;

char AM, WB, HF;

unsigned long FreqInHz;

int rssi=0;
int agc=0;

int squelch=0;


void drawHelp(int pos) {
    char Buffer[128];
	// s/S - Squelch +/-
	// t/T - Tune  +50/-50

    pos +=40;
    sprintf(Buffer,"t/T - Tune  +50/-50");
    Text(20, height - pos , Buffer, SerifTypeface, 20);	// Info

    pos +=40;
    sprintf(Buffer,"g/G - Tune  +10/-10");
    Text(20, height - pos , Buffer, SerifTypeface, 20);	// Info

    pos +=40;
    sprintf(Buffer,"s/S - Squelch +/-");
    Text(20, height - pos , Buffer, SerifTypeface, 20);	// Info

    pos +=40;
    sprintf(Buffer,"1-3 Preset frequencies");
    Text(20, height - pos , Buffer, SerifTypeface, 20);	// Info

     if (TestSound==1) {
         pos +=40;

         sprintf(Buffer,"o,p,l/L change test frequncy");
         Text(20, height - pos , Buffer, SerifTypeface, 20);	// Info
   }

   pos +=40;
   sprintf(Buffer,"q,Q - quit, H -remove help");
   Text(20, height - pos , Buffer, SerifTypeface, 20);	// Info

}


void drawOpenVG() {
    char Buffer[128];

    int line=(last_thread_ix)%WTF_HEIGHT;
    if (line<0) line=1;
    if (line>=WTF_HEIGHT-1) line=WTF_HEIGHT-1;

    Start(width, height);				   // Start the picture
    Background(0, 0, 0);				   // Black background
    Fill(44, 77, 232, 1);				   // Blue background
    //Roundrect(0,0,width-WTF_WIDTH, height,30,30);	   // Blue rounded area
    Fill(255, 255, 255, 1);				   // White text
    Text(20, height -20 , "ttunevg", SerifTypeface, 20);	// Info
    if (AM) {
         sprintf(Buffer,"%d kHz AM %s", FreqInHz, WB ? " wide" : "");
    } else {
        sprintf(Buffer,"%d kHz FM %s", FreqInHz, WB ? " wide" : "");
    }
    Text(20, height -60 ,Buffer, SerifTypeface, 20);	// Info

    sprintf(Buffer, "RSSI:%d Squelch:%d agc:%d ", rssi,squelch,agc);

    Text(20, height - 100, Buffer, SerifTypeface, 20);	// Info

    Text(20, height - 140, "q - to quit, h - help", SerifTypeface, 20);	// Info

    if (TestSound==1) {
        sprintf(Buffer, "Test freq: %.2f Hz", frequency);
        Text(20, height - 180, Buffer, SerifTypeface, 20);	// Info
    }

    if (drawHelpText) {
        drawHelp(180);
    }


#define WATERFALL 1

#if WATERFALL
    // Draw top
    makeimage(width-WTF_WIDTH,WTF_HEIGHT-(line+1),WTF_WIDTH,line+1,(VGubyte *)&imageData[0]);

    // Draw the rest
    makeimage(width-WTF_WIDTH,0,WTF_WIDTH,WTF_HEIGHT-line,(VGubyte *)&imageData[line*WTF_WIDTH]); 
#else
    makeimage(width-WTF_WIDTH,height-WTF_HEIGHT,WTF_WIDTH,WTF_HEIGHT,(VGubyte *)imageData);
#endif

    // WTF_HEIGHT-(line+2)

    makeimage(0,0,FFT_WIDTH,FFT_HEIGHT,(VGubyte *)fftImage);


    End();
}

// Sends the tuned frequency to radio
void retune() {
        char Buf[64];
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

// Sends the squelch value to radio
void setSquelch() 
{
	char Buf[64];
	Buf[0] = 0x00;     // write
	Buf[1] = 0x0;      // addr hi for squelch
	Buf[2] = 0xf5;     // addr lo for squlech
	Buf[3] = squelch;  // RF frequency - low byte
	bcm2835_spi_transfern(Buf, 4);
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


    int err;
    int Debug = 0;
    unsigned char ControlByte;
    unsigned int BytesAvail;
    int audio_val;
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;
    int DisplayRSSI = 1;
    
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
    
    // Set to zero
    setSquelch();

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
	        rssi = Buf[3];
	        agc = Buf[4];
	       //printf("RSSI = %d, AGC = %d, %02x %02x\n", Buf[3], Buf[4], Buf[5], Buf[6]);
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
               //short_sweep((short *)(silly));
               generate_freq((short *)silly,NFRAMES,0.8f,frequency);
               //frequency=frequency+10.0f;
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

	    // Join thread
            unsigned char *fcharp= buffer+3;
	    if (first_thread!=1) {
	      join_fft_thread();
	    }
	    first_thread=0;
	    last_thread_ix++;
	    start_fft_thread(last_thread_ix%4,fcharp);

	    // Check keyboard

        if (_kbhit())
        {
            char c=getchar();

            switch(c)
            {
            case '1':
	      // ESSB Atis 
                FreqInHz=122450;
                retune();
                break;
            case '2':
	      // ESSB TWR
                FreqInHz=118100;
                retune();
                break;
            case '3':
	      // ESSB GND
                FreqInHz=121600;
                retune();
                break;

            case 'a':
	      // Acars
                FreqInHz=131825;
                retune();
                break;

            case 'A':
	      // Acars
                FreqInHz=131725;
                retune();
                break;

            case 'z':
	      // Acars European freq
                FreqInHz=136900;
                retune();
                break;

            case 'Z':
	      // Acars European freq
                FreqInHz=136750;
                retune();
                break;

            case 'h':
                drawHelpText=1;
                break;

            case 'H':
                drawHelpText=0;
                break;

            case 't':
                FreqInHz+=50;
                retune();
                break;
            case 'T':
                FreqInHz-=50;
                retune();
                break;

            case 'g':
                FreqInHz+=10;
                retune();
                break;
            case 'G':
                FreqInHz-=10;
                retune();
                break;

            case 's':
                squelch += 10;
                if (squelch > 255) squelch = 255;
                setSquelch();
                break;

            case 'S':
                squelch -= 1;
                if (squelch < 0) squelch = 0;
                setSquelch();
                break;
            case 'o':
                tridelta-=30;
                frequency=frequency/2;
                printf("freq %.2f Hz\n",frequency);
                // Increase freqeuncy
                break;
            case 'p':
                tridelta+=30;
                frequency=frequency*2;
                printf("freq %.2f Hz\n",frequency);
                // Increase freqeuncy
                break;
            case 'l':
                frequency=frequency+100;
                printf("freq %.2f Hz\n",frequency);
                // Increase freqeuncy
                break;
            case 'L':
                frequency=frequency-100;
                printf("freq %.2f Hz\n",frequency);
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

