//
// libshapes: high-level OpenVG API
// Anthony Starks (ajstarks@gmail.com)
//

#ifdef __arm__
#define BCMHOST 1
#endif

// Additional outline / windowing functions
// Paeryn (github.com/paeryn)
//

#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <termios.h>
#include <jpeglib.h>
#include <string.h>
#endif
#include <assert.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#ifdef BCMHOST
#include "EGL/egl.h"
#include "bcm_host.h"
#else
#include "shContext.h"
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include "DejaVuSans.inc"				   // font data
#include "DejaVuSerif.inc"
#include "DejaVuSansMono.inc"
#include "shf.inc"

#include "eglstate.h"					   // data structures for graphics state
#include "fontinfo.h"					   // font data structure
#include "shapes.h"

// This is a temporary solution to be able to build openvg.go
#if !BCMHOST && !CMAKE_BUILD
#include "shivavg/shArrays.c"
#include "shivavg/shGeometry.c"
#include "shivavg/shParams.c"
#include "shivavg/shVectors.c"
#include "shivavg/shContext.c"
#include "shivavg/shImage.c"
#include "shivavg/shPath.c"
#include "shivavg/shVgu.c"
#include "shivavg/shExtensions.c"
#include "shivavg/shPaint.c"
#include "shivavg/shPipeline.c"
#ifdef WIN32
#include "shivavg/glw.c"
#else
#include "shivavg/glx.c"
#endif
#endif

#ifndef BCMHOST
#include "shgl.h"
#endif


//
// Libshape globals
//
/*
 void vgDestroyPaint(VGPaint paint)

 The resources associated with a paint object may be deallocated by calling
 vgDestroyPaint. Following the call, the paint handle is no longer valid in any
 of the contexts that shared it. If the paint object is currently active in a drawing
 context, the context continues to access it until it is replaced or the context
 is destroyed.

*/

VGPaint fillPaint;
VGPaint paint;
VGPaint strokePaint;


static STATE_T _state, *state = &_state;	// global graphics state
static const int MAXFONTPATH = 500;
static int init_x = 0;		// Initial window position and size
static int init_y = 0;
static unsigned int init_w = 0;
static unsigned int init_h = 0;
//
// Terminal settings
//
// terminal settings structures
#ifndef _WIN32
struct termios new_term_attr;
struct termios orig_term_attr;
#endif

// saveterm saves the current terminal settings
void saveterm() {
#ifndef _WIN32
	tcgetattr(fileno(stdin), &orig_term_attr);
#endif
}

// rawterm sets the terminal to raw mode
void rawterm() {
#ifndef _WIN32
	memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
	new_term_attr.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
	new_term_attr.c_cc[VTIME] = 0;
	new_term_attr.c_cc[VMIN] = 0;
	tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);
#endif
}

// restore resets the terminal to the previously saved setting
void restoreterm() {
#ifndef _WIN32
	tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);
#endif
}

//
// Font functions
//

// loadfont loads font path data
// derived from http://web.archive.org/web/20070808195131/http://developer.hybrid.fi/font2openvg/renderFont.cpp.txt
Fontinfo loadfont(const int *Points,
		  const int *PointIndices,
		  const unsigned char *Instructions,
		  const int *InstructionIndices, const int *InstructionCounts, const int *adv, const short *cmap, int ng) {

	Fontinfo f;
	int i;

	memset(f.Glyphs, 0, MAXFONTPATH * sizeof(VGPath));
	if (ng > MAXFONTPATH) {
		return f;
	}
	for (i = 0; i < ng; i++) {
		const int *p = &Points[PointIndices[i] * 2];
		const unsigned char *instructions = &Instructions[InstructionIndices[i]];
		int ic = InstructionCounts[i];
		VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_S_32,
					   1.0f / 65536.0f, 0.0f, 0, 0,
					   VG_PATH_CAPABILITY_ALL);
		f.Glyphs[i] = path;
		if (ic) {
			vgAppendPathData(path, ic, instructions, p);
		}
	}
	f.CharacterMap = cmap;
	f.GlyphAdvances = adv;
	f.Count = ng;
	f.descender_height = 0;
	f.font_height = 0;
	return f;
}

// unloadfont frees font path data
void unloadfont(VGPath * glyphs, int n) {
	int i;
	for (i = 0; i < n; i++) {
		vgDestroyPath(glyphs[i]);
	}
}

// createImageFromJpeg decompresses a JPEG image to the standard image format
// source: https://github.com/ileben/ShivaVG/blob/master/examples/test_image.c
VGImage createImageFromJpeg(const char *filename) {
#ifndef _WIN32
	FILE *infile;
	struct jpeg_decompress_struct jdc;
	struct jpeg_error_mgr jerr;
	JSAMPARRAY buffer;
	unsigned int bstride;
	unsigned int bbpp;

	VGImage img;
	VGubyte *data;
	unsigned int width;
	unsigned int height;
	unsigned int dstride;
	unsigned int dbpp;

	VGubyte *brow;
	VGubyte *drow;
	unsigned int x;
	unsigned int lilEndianTest = 1;
	VGImageFormat rgbaFormat;

	// Check for endianness
	if (((unsigned char *)&lilEndianTest)[0] == 1)
		rgbaFormat = VG_sABGR_8888;
	else
		rgbaFormat = VG_sRGBA_8888;

	// Try to open image file
	infile = fopen(filename, "rb");
	if (infile == NULL) {
		printf("Failed opening '%s' for reading!\n", filename);
		return VG_INVALID_HANDLE;
	}
	// Setup default error handling
	jdc.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&jdc);

	// Set input file
	jpeg_stdio_src(&jdc, infile);

	// Read header and start
	jpeg_read_header(&jdc, TRUE);
	jpeg_start_decompress(&jdc);
	width = jdc.output_width;
	height = jdc.output_height;

	// Allocate buffer using jpeg allocator
	bbpp = jdc.output_components;
	bstride = width * bbpp;
	buffer = (*jdc.mem->alloc_sarray)
	    ((j_common_ptr) & jdc, JPOOL_IMAGE, bstride, 1);

	// Allocate image data buffer
	dbpp = 4;
	dstride = width * dbpp;
	data = (VGubyte *) malloc(dstride * height);

	// Iterate until all scanlines processed
	while (jdc.output_scanline < height) {

		// Read scanline into buffer
		jpeg_read_scanlines(&jdc, buffer, 1);
		drow = data + (height - jdc.output_scanline) * dstride;
		brow = buffer[0];
		// Expand to RGBA
		for (x = 0; x < width; ++x, drow += dbpp, brow += bbpp) {
			switch (bbpp) {
			case 4:
#ifdef BCMHOST
				drow[0] = brow[0];
				drow[1] = brow[1];
				drow[2] = brow[2];
				drow[3] = brow[3];
#else
                // jpeg has no alpha channel? This should not happen?
                drow[3] = brow[0];
                drow[2] = brow[1];
                drow[1] = brow[2];
                drow[0] = brow[3];
#endif
				break;
			case 3:
#ifdef BCMHOST
				drow[0] = brow[0];
				drow[1] = brow[1];
				drow[2] = brow[2];
				drow[3] = 255;
#else
                // I dont know if shiva or libshapes is broken but this fixes the problem
                drow[3] = brow[0];
				drow[2] = brow[1];
				drow[1] = brow[2];
				drow[0] = 255;
#endif
				break;
			}
		}
	}

	// Create VG image
	img = vgCreateImage(rgbaFormat, width, height, VG_IMAGE_QUALITY_BETTER);
	vgImageSubData(img, data, dstride, rgbaFormat, 0, 0, width, height);

	// Cleanup
	jpeg_destroy_decompress(&jdc);
	fclose(infile);
	free(data);

	return img;
#else 
	return 0;
#endif
}

// makeimage makes an image from a raw raster of red, green, blue, alpha values
void makeimage(VGfloat x, VGfloat y, int w, int h, VGubyte * data) {
	unsigned int dstride = w * 4;
#ifdef BCMHOST
    // This seems to work on the raspberry, just leave it
	VGImageFormat rgbaFormat = VG_sABGR_8888;
#else
    VGImageFormat rgbaFormat = VG_sRGBA_8888;
#endif
	VGImage img = vgCreateImage(rgbaFormat, w, h, VG_IMAGE_QUALITY_BETTER);
	vgImageSubData(img, (void *)data, dstride, rgbaFormat, 0, 0, w, h);
	vgSetPixels(x, y, img, 0, 0, w, h);
	vgDestroyImage(img);
}

// Image places an image at the specifed location
void Image(VGfloat x, VGfloat y, int w, int h, const char *filename) {
	VGImage img = createImageFromJpeg(filename);
	vgSetPixels(x, y, img, 0, 0, w, h);
	vgDestroyImage(img);
}

void ScaledImage(VGfloat x, VGfloat y, int w, int h, char *filename) {
    VGImage img = createImageFromJpeg(filename);
    #if 0 //vgSetPixels paints directly without scale and transform
    vgSetPixels(x, y, img, 0, 0, w, h);
    #else //vgDrawImage is better
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
    vgTranslate(x, y);
    vgDrawImage(img);
    vgTranslate(-x, -y);
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    #endif
    vgDestroyImage(img);
}

// Found in video.c
void video_decode_test(char *filename,int x,int y,int w,int h);

void Video(VGfloat x, VGfloat y,  VGfloat w,  VGfloat  h, char *filename) {
#ifndef __arm__    
    Fill(22, 35, 116, 1);				   // Blue testblob for location of video
    Rect(x,y,w,h);
    swap_buffers_gls();
#else
    video_decode_test(filename,(int) x,h-(int) y,(int) w,(int) h);
#endif
}


// dumpscreen writes the raster
void dumpscreen(int w, int h, FILE * fp) {
	void *ScreenBuffer = malloc(w * h * 4);
	vgReadPixels(ScreenBuffer, (w * 4), VG_sABGR_8888, 0, 0, w, h);
	fwrite(ScreenBuffer, 1, w * h * 4, fp);
	free(ScreenBuffer);
}

Fontinfo SansTypeface, SerifTypeface, MonoTypeface,scaryTypeface;

// initWindowSize requests a specific window size & position, if not called
// then init() will open a full screen window.
// Done this way to preserve the original init() behaviour.
void initWindowSize(int x, int y, unsigned int w, unsigned int h) {
	init_x = x;
	init_y = y;
	init_w = w;
	init_h = h;
}

// init sets the system to its initial state
void init(int *w, int *h) {
#ifdef BCMHOST
	bcm_host_init();
	memset(state, 0, sizeof(*state));
	state->window_x = init_x;
	state->window_y = init_y;
	state->window_width = init_w;
	state->window_height = init_h;
	oglinit(state);
#else
	memset(state, 0, sizeof(*state));
    state->screen_width=960;
    state->screen_height=720;

    // ONLY fullscreen, needs further investigation
    state->window_width=state->screen_width;
    state->window_height=state->screen_height;

    init_gls(&state->screen_width,&state->screen_height);

    // OLAS
#endif
	SansTypeface = loadfont(DejaVuSans_glyphPoints,
				DejaVuSans_glyphPointIndices,
				DejaVuSans_glyphInstructions,
				DejaVuSans_glyphInstructionIndices,
				DejaVuSans_glyphInstructionCounts,
				DejaVuSans_glyphAdvances, DejaVuSans_characterMap, DejaVuSans_glyphCount);
	SansTypeface.descender_height = DejaVuSans_descender_height;
	SansTypeface.font_height = DejaVuSans_font_height;

	SerifTypeface = loadfont(DejaVuSerif_glyphPoints,
				 DejaVuSerif_glyphPointIndices,
				 DejaVuSerif_glyphInstructions,
				 DejaVuSerif_glyphInstructionIndices,
				 DejaVuSerif_glyphInstructionCounts,
				 DejaVuSerif_glyphAdvances, DejaVuSerif_characterMap, DejaVuSerif_glyphCount);
	SerifTypeface.descender_height = DejaVuSerif_descender_height;
	SerifTypeface.font_height = DejaVuSerif_font_height;

	MonoTypeface = loadfont(DejaVuSansMono_glyphPoints,
				DejaVuSansMono_glyphPointIndices,
				DejaVuSansMono_glyphInstructions,
				DejaVuSansMono_glyphInstructionIndices,
				DejaVuSansMono_glyphInstructionCounts,
				DejaVuSansMono_glyphAdvances, DejaVuSansMono_characterMap, DejaVuSansMono_glyphCount);
	MonoTypeface.descender_height = DejaVuSansMono_descender_height;
	MonoTypeface.font_height = DejaVuSansMono_font_height;


	scaryTypeface = loadfont(shf_glyphPoints,
				shf_glyphPointIndices,
				shf_glyphInstructions,
				shf_glyphInstructionIndices,
				shf_glyphInstructionCounts,
				shf_glyphAdvances, shf_characterMap, shf_glyphCount);


    //*w = state->screen_width;
    //*h = state->screen_height;

    fillPaint = vgCreatePaint();

    paint = vgCreatePaint();

    strokePaint = vgCreatePaint();

    *w = state->window_width;
    *h = state->window_height;
}

// finish cleans up
void finish() {
	unloadfont(SansTypeface.Glyphs, SansTypeface.Count);
	unloadfont(SerifTypeface.Glyphs, SerifTypeface.Count);
	unloadfont(MonoTypeface.Glyphs, MonoTypeface.Count);

    vgDestroyPaint(fillPaint);
    vgDestroyPaint(paint);
    vgDestroyPaint(strokePaint);
#ifdef BCMHOST
       //glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(state->display, state->surface);
	eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(state->display, state->surface);
	eglDestroyContext(state->display, state->context);
	eglTerminate(state->display);
#endif
}

//
// Transformations
//

// Translate the coordinate system to x,y
void Translate(VGfloat x, VGfloat y) {
	vgTranslate(x, y);
}

// Rotate around angle r
void Rotate(VGfloat r) {
	vgRotate(r);
}

// Shear shears the x coordinate by x degrees, the y coordinate by y degrees
void Shear(VGfloat x, VGfloat y) {
	vgShear(x, y);
}

// Scale scales by  x, y
void Scale(VGfloat x, VGfloat y) {
	vgScale(x, y);
}

//
// Style functions
//

// setfill sets the fill color
void setfill(VGfloat color[4]) {
	vgSetParameteri(fillPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(fillPaint, VG_PAINT_COLOR, 4, color);
	vgSetPaint(fillPaint, VG_FILL_PATH);
	//vgDestroyPaint(fillPaint);
}

// setstroke sets the stroke color
void setstroke(VGfloat color[4]) {
        //VGPaint strokePaint = vgCreatePaint();
	vgSetParameteri(strokePaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameterfv(strokePaint, VG_PAINT_COLOR, 4, color);
	vgSetPaint(strokePaint, VG_STROKE_PATH);
    //vgDestroyPaint(strokePaint);
}

// StrokeWidth sets the stroke width
void StrokeWidth(VGfloat width) {
	vgSetf(VG_STROKE_LINE_WIDTH, width);
	vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
	vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_MITER);
}

//
// Color functions
//
//

// RGBA fills a color vectors from a RGBA quad.
void RGBA(unsigned int r, unsigned int g, unsigned int b, VGfloat a, VGfloat color[4]) {
	if (r > 255) {
		r = 0;
	}
	if (g > 255) {
		g = 0;
	}
	if (b > 255) {
		b = 0;
	}
	if (a < 0.0 || a > 1.0) {
		a = 1.0;
	}
	color[0] = (VGfloat) r / 255.0f;
	color[1] = (VGfloat) g / 255.0f;
	color[2] = (VGfloat) b / 255.0f;
	color[3] = a;
}

// RGB returns a solid color from a RGB triple
void RGB(unsigned int r, unsigned int g, unsigned int b, VGfloat color[4]) {
	RGBA(r, g, b, 1.0f, color);
}

// Stroke sets the stroke color, defined as a RGB triple.
void Stroke(unsigned int r, unsigned int g, unsigned int b, VGfloat a) {
	VGfloat color[4];
	RGBA(r, g, b, a, color);
	setstroke(color);
}

// Fill sets the fillcolor, defined as a RGBA quad.
void Fill(unsigned int r, unsigned int g, unsigned int b, VGfloat a) {
	VGfloat color[4];
	RGBA(r, g, b, a, color);
	setfill(color);
}

// setstops sets color stops for gradients
void setstop(VGPaint paint, VGfloat * stops, int n) {
	VGboolean multmode = VG_FALSE;
	VGColorRampSpreadMode spreadmode = VG_COLOR_RAMP_SPREAD_REPEAT;
	vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, spreadmode);
	vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_PREMULTIPLIED, multmode);
	vgSetParameterfv(paint, VG_PAINT_COLOR_RAMP_STOPS, 5 * n, stops);
	vgSetPaint(paint, VG_FILL_PATH);
}

// LinearGradient fills with a linear gradient
void FillLinearGradient(VGfloat x1, VGfloat y1, VGfloat x2, VGfloat y2, VGfloat * stops, int ns) {
	VGfloat lgcoord[4] = { x1, y1, x2, y2 };
        //VGPaint paint = vgCreatePaint();
	vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT);
	vgSetParameterfv(paint, VG_PAINT_LINEAR_GRADIENT, 4, lgcoord);
	setstop(paint, stops, ns);
        //vgDestroyPaint(paint);
}

// RadialGradient fills with a linear gradient
void FillRadialGradient(VGfloat cx, VGfloat cy, VGfloat fx, VGfloat fy, VGfloat radius, VGfloat * stops, int ns) {
	VGfloat radialcoord[5] = { cx, cy, fx, fy, radius };
	//VGPaint paint = vgCreatePaint();
	vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_RADIAL_GRADIENT);
	vgSetParameterfv(paint, VG_PAINT_RADIAL_GRADIENT, 5, radialcoord);
	setstop(paint, stops, ns);
	//vgDestroyPaint(paint);
}

// ClipRect limits the drawing area to specified rectangle
void ClipRect(VGint x, VGint y, VGint w, VGint h) {
	vgSeti(VG_SCISSORING, VG_TRUE);
	VGint coords[4] = { x, y, w, h };
	vgSetiv(VG_SCISSOR_RECTS, 4, coords);
}

// ClipEnd stops limiting drawing area to specified rectangle
void ClipEnd() {
	vgSeti(VG_SCISSORING, VG_FALSE);
}

// Text Functions

// next_utf_char handles UTF encoding
unsigned char *next_utf8_char(unsigned char *utf8, int *codepoint) {
	int seqlen;
	int datalen = (int)strlen((const char *)utf8);
	unsigned char *p = utf8;

	if (datalen < 1 || *utf8 == 0) {		   // End of string
		return NULL;
	}
	if (!(utf8[0] & 0x80)) {			   // 0xxxxxxx
		*codepoint = (wchar_t) utf8[0];
		seqlen = 1;
	} else if ((utf8[0] & 0xE0) == 0xC0) {		   // 110xxxxx 
		*codepoint = (int)(((utf8[0] & 0x1F) << 6) | (utf8[1] & 0x3F));
		seqlen = 2;
	} else if ((utf8[0] & 0xF0) == 0xE0) {		   // 1110xxxx
		*codepoint = (int)(((utf8[0] & 0x0F) << 12) | ((utf8[1] & 0x3F) << 6) | (utf8[2] & 0x3F));
		seqlen = 3;
	} else {
		return NULL;				   // No code points this high here
	}
	p += seqlen;
	return p;
}


// Text renders a string of text at a specified location, size, using the specified font glyphs
// derived from http://web.archive.org/web/20070808195131/http://developer.hybrid.fi/font2openvg/renderFont.cpp.txt
void Text(VGfloat x, VGfloat y, const char *s, Fontinfo f, int pointsize) {
	VGfloat size = (VGfloat) pointsize, xx = x, mm[9];
	vgGetMatrix(mm);
	int character;
	unsigned char *ss = (unsigned char *)s;
	while ((ss = next_utf8_char(ss, &character)) != NULL) {
		int glyph = f.CharacterMap[character];
		if (glyph == -1) {
			continue;			   //glyph is undefined
		}
		VGfloat mat[9] = {
			size, 0.0f, 0.0f,
			0.0f, size, 0.0f,
			xx, y, 1.0f
		};
		vgLoadMatrix(mm);
		vgMultMatrix(mat);
		vgDrawPath(f.Glyphs[glyph], VG_FILL_PATH);
		xx += size * f.GlyphAdvances[glyph] / 65536.0f;
	}
	vgLoadMatrix(mm);
}

// TextWidth returns the width of a text string at the specified font and size.
VGfloat TextWidth(const char *s, Fontinfo f, int pointsize) {
	VGfloat tw = 0.0;
	VGfloat size = (VGfloat) pointsize;
	int character;
	unsigned char *ss = (unsigned char *)s;
	while ((ss = next_utf8_char(ss, &character)) != NULL) {
		int glyph = f.CharacterMap[character];
		if (glyph == -1) {
			continue;			   //glyph is undefined
		}
		tw += size * f.GlyphAdvances[glyph] / 65536.0f;
	}
	return tw;
}

// TextMid draws text, centered on (x,y)
void TextMid(VGfloat x, VGfloat y, const char *s, Fontinfo f, int pointsize) {
	VGfloat tw = TextWidth(s, f, pointsize);
	Text(x - (tw / 2.0), y, s, f, pointsize);
}

// TextEnd draws text, with its end aligned to (x,y)
void TextEnd(VGfloat x, VGfloat y, const char *s, Fontinfo f, int pointsize) {
	VGfloat tw = TextWidth(s, f, pointsize);
	Text(x - tw, y, s, f, pointsize);
}

// TextHeight reports a font's height
VGfloat TextHeight(Fontinfo f, int pointsize) {
	return (f.font_height * pointsize) / 65536;
}

// TextDepth reports a font's depth (how far under the baseline it goes)
VGfloat TextDepth(Fontinfo f, int pointsize) {
	return (-f.descender_height * pointsize) / 65536;
}

//
// Shape functions
//

// newpath creates path data
// Changed capabilities as others not needed at the moment - allows possible
// driver optimisations.
VGPath newpath() {
	return vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_APPEND_TO);	// Other capabilities not needed
}

// makecurve makes path data using specified segments and coordinates
void makecurve(VGubyte * segments, VGfloat * coords, VGbitfield flags) {
	VGPath path = newpath();
	vgAppendPathData(path, 2, segments, coords);
	vgDrawPath(path, flags);
	vgDestroyPath(path);
}

// CBezier makes a quadratic bezier curve
void Cbezier(VGfloat sx, VGfloat sy, VGfloat cx, VGfloat cy, VGfloat px, VGfloat py, VGfloat ex, VGfloat ey) {
	VGubyte segments[] = { VG_MOVE_TO_ABS, VG_CUBIC_TO };
	VGfloat coords[] = { sx, sy, cx, cy, px, py, ex, ey };
	makecurve(segments, coords, VG_FILL_PATH | VG_STROKE_PATH);
}

// QBezier makes a quadratic bezier curve
void Qbezier(VGfloat sx, VGfloat sy, VGfloat cx, VGfloat cy, VGfloat ex, VGfloat ey) {
	VGubyte segments[] = { VG_MOVE_TO_ABS, VG_QUAD_TO };
	VGfloat coords[] = { sx, sy, cx, cy, ex, ey };
	makecurve(segments, coords, VG_FILL_PATH | VG_STROKE_PATH);
}

// interleave interleaves arrays of x, y into a single array
void interleave(VGfloat * x, VGfloat * y, int n, VGfloat * points) {
	while (n--) {
		*points++ = *x++;
		*points++ = *y++;
	}
}

// poly makes either a polygon or polyline
void poly(VGfloat * x, VGfloat * y, VGint n, VGbitfield flag) {
	VGfloat *points=(VGfloat *)malloc(sizeof(VGfloat) * n * 2);
	VGPath path = newpath();
	interleave(x, y, n, points);
	vguPolygon(path, points, n, VG_FALSE);
	vgDrawPath(path, flag);
	vgDestroyPath(path);
	free (points);
}

// Polygon makes a filled polygon with vertices in x, y arrays
void Polygon(VGfloat * x, VGfloat * y, VGint n) {
	poly(x, y, n, VG_FILL_PATH);
}

// Polyline makes a polyline with vertices at x, y arrays
void Polyline(VGfloat * x, VGfloat * y, VGint n) {
	poly(x, y, n, VG_STROKE_PATH);
}

// Rect makes a rectangle at the specified location and dimensions
void Rect(VGfloat x, VGfloat y, VGfloat w, VGfloat h) {
	VGPath path = newpath();
	vguRect(path, x, y, w, h);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Line makes a line from (x1,y1) to (x2,y2)
void Line(VGfloat x1, VGfloat y1, VGfloat x2, VGfloat y2) {
	VGPath path = newpath();
	vguLine(path, x1, y1, x2, y2);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Roundrect makes an rounded rectangle at the specified location and dimensions
void Roundrect(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGfloat rw, VGfloat rh) {
	VGPath path = newpath();
	vguRoundRect(path, x, y, w, h, rw, rh);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Ellipse makes an ellipse at the specified location and dimensions
void Ellipse(VGfloat x, VGfloat y, VGfloat w, VGfloat h) {
	VGPath path = newpath();
	vguEllipse(path, x, y, w, h);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Circle makes a circle at the specified location and dimensions
void Circle(VGfloat x, VGfloat y, VGfloat r) {
	Ellipse(x, y, r, r);
}

// Arc makes an elliptical arc at the specified location and dimensions
void Arc(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGfloat sa, VGfloat aext) {
	VGPath path = newpath();
	vguArc(path, x, y, w, h, sa, aext, VGU_ARC_OPEN);
	vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);
	vgDestroyPath(path);
}

// Start begins the picture, clearing a rectangular region with a specified color
void Start(int width, int height) {
	VGfloat color[4] = { 1, 1, 1, 1 };
	vgSetfv(VG_CLEAR_COLOR, 4, color);
	vgClear(0, 0, width, height);
	color[0] = 0, color[1] = 0, color[2] = 0;
	setfill(color);
	setstroke(color);
	StrokeWidth(0);
	vgLoadIdentity();
}

// End checks for errors, and renders to the display
void End() {
#ifdef BCMHOST
	assert(vgGetError() == VG_NO_ERROR);
	eglSwapBuffers(state->display, state->surface);
	assert(eglGetError() == EGL_SUCCESS);
#else
    swap_buffers_gls();
#endif
}

// SaveEnd dumps the raster before rendering to the display 
void SaveEnd(const char *filename) {
	FILE *fp;
	assert(vgGetError() == VG_NO_ERROR);
	if (strlen(filename) == 0) {
		dumpscreen(state->screen_width, state->screen_height, stdout);
	} else {
		fp = fopen(filename, "wb");
		if (fp != NULL) {
			dumpscreen(state->screen_width, state->screen_height, fp);
			fclose(fp);
		}
	}
#ifdef BCMHOST
	eglSwapBuffers(state->display, state->surface);
	assert(eglGetError() == EGL_SUCCESS);
#else
    swap_buffers_gls();
#endif
}

// Backgroud clears the screen to a solid background color
void Background(unsigned int r, unsigned int g, unsigned int b) {
#ifdef BCMHOST
	VGfloat colour[4];
	RGB(r, g, b, colour);
	vgSetfv(VG_CLEAR_COLOR, 4, colour);
	vgClear(0, 0, state->window_width, state->window_height);
#else
    Fill(r, g, b, 1);
    Rect(0, 0, state->screen_width, state->screen_height);
#endif
}

// BackgroundRGB clears the screen to a background color with alpha
void BackgroundRGB(unsigned int r, unsigned int g, unsigned int b, VGfloat a) {
	VGfloat colour[4];
	RGBA(r, g, b, a, colour);
	vgSetfv(VG_CLEAR_COLOR, 4, colour);
	vgClear(0, 0, state->window_width, state->window_height);
}

// WindowClear clears the window to previously set background colour
void WindowClear() {
	vgClear(0, 0, state->window_width, state->window_height);
}

// AreaClear clears a given rectangle in window coordinates (not affected by
// transformations)
void AreaClear(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
	vgClear(x, y, w, h);
}

// WindowOpacity sets the  window opacity
void WindowOpacity(unsigned int a) {
#ifdef __arm__
	dispmanChangeWindowOpacity(state, a);
#endif
}

// WindowPosition moves the window to given position
void WindowPosition(int x, int y) {
#ifdef __arm__
	dispmanMoveWindow(state, x, y);
#endif
}

// Outlined shapes
// Hollow shapes -because filling still happens even with a fill of 0,0,0,0
// unlike where using a strokewidth of 0 disables the stroke.
// Either this or change the original functions to require the VG_x_PATH flags

// CBezier makes a quadratic bezier curve, stroked
void CbezierOutline(VGfloat sx, VGfloat sy, VGfloat cx, VGfloat cy, VGfloat px, VGfloat py, VGfloat ex, VGfloat ey) {
	VGubyte segments[] = { VG_MOVE_TO_ABS, VG_CUBIC_TO };
	VGfloat coords[] = { sx, sy, cx, cy, px, py, ex, ey };
	makecurve(segments, coords, VG_STROKE_PATH);
}

// QBezierOutline makes a quadratic bezier curve, outlined 
void QbezierOutline(VGfloat sx, VGfloat sy, VGfloat cx, VGfloat cy, VGfloat ex, VGfloat ey) {
	VGubyte segments[] = { VG_MOVE_TO_ABS, VG_QUAD_TO };
	VGfloat coords[] = { sx, sy, cx, cy, ex, ey };
	makecurve(segments, coords, VG_STROKE_PATH);
}

// RectOutline makes a rectangle at the specified location and dimensions, outlined 
void RectOutline(VGfloat x, VGfloat y, VGfloat w, VGfloat h) {
	VGPath path = newpath();
	vguRect(path, x, y, w, h);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

// RoundrectOutline  makes an rounded rectangle at the specified location and dimensions, outlined 
void RoundrectOutline(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGfloat rw, VGfloat rh) {
	VGPath path = newpath();
	vguRoundRect(path, x, y, w, h, rw, rh);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

// EllipseOutline makes an ellipse at the specified location and dimensions, outlined
void EllipseOutline(VGfloat x, VGfloat y, VGfloat w, VGfloat h) {
	VGPath path = newpath();
	vguEllipse(path, x, y, w, h);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

// CircleOutline makes a circle at the specified location and dimensions, outlined
void CircleOutline(VGfloat x, VGfloat y, VGfloat r) {
	EllipseOutline(x, y, r, r);
}

// ArcOutline makes an elliptical arc at the specified location and dimensions, outlined
void ArcOutline(VGfloat x, VGfloat y, VGfloat w, VGfloat h, VGfloat sa, VGfloat aext) {
	VGPath path = newpath();
	vguArc(path, x, y, w, h, sa, aext, VGU_ARC_OPEN);
	vgDrawPath(path, VG_STROKE_PATH);
	vgDestroyPath(path);
}

#if 0

#include FT_FREETYPE_H
#include FT_OUTLINE_H

#define FONTLIB	"/usr/share/fonts/TTF/HanaMinA.ttf"






void Text(VGfloat x, VGfloat y, char *s, int pointsize) {
	float size = (VGfloat) pointsize, xx = x, mm[9];

	vgGetMatrix(mm);
	int character;
	char *ss = s;
	int error;
	while ((ss = readNextChar(ss, &character)) != NULL) {
		int glyphIndex = FT_Get_Char_Index(face, character);

		error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING | FT_LOAD_IGNORE_TRANSFORM);
		assert(error == 0);

		int *points = NULL;
		int point_len = 0;
		unsigned char *instructions = NULL;
		int instruction_len = 0;
		FT_Outline outline = face->glyph->outline;
		float minx = 0.0f, miny = 0.0f, maxx = 0.0f, maxy = 0.0f;
		int s = 0, e;
		for (int con = 0; con < outline.n_contours; ++con) {
			int pnts = 1;
			e = outline.contours[con] + 1;

			//read the contour start point
			instruction_len += 1;
			instructions = realloc(instructions, instruction_len * sizeof(unsigned char));
			instructions[instruction_len - 1] = 2;
			point_len += 2;
			points = realloc(points, point_len * sizeof(int));
			points[point_len - 2] = outline.points[s].x * 16.0f;
			points[point_len - 1] = outline.points[s].y * 16.0f;

			int i = s + 1;
			while (i <= e) {
				int c = (i == e) ? s : i;
				int n = (i == e - 1) ? s : (i + 1);
				if (outline.tags[c] & 1) { //line
					++i;
					instruction_len += 1;
					instructions = realloc(instructions, instruction_len * sizeof(unsigned char));
					instructions[instruction_len - 1] = 4;
					point_len += 2;
					points = realloc(points, point_len * sizeof(int));
					points[point_len - 2] = outline.points[c].x * 16.0f;
					points[point_len - 1] = outline.points[c].y * 16.0f;
					pnts += 1;
				} else {		   //spline
					instruction_len += 1;
					instructions = realloc(instructions, instruction_len * sizeof(unsigned char));
					instructions[instruction_len - 1] = 10;
					point_len += 2;
					points = realloc(points, point_len * sizeof(int));
					points[point_len - 2] = outline.points[c].x * 16.0f;
					points[point_len - 1] = outline.points[c].y * 16.0f;
					if (outline.tags[n] & 1) {	//next on
						point_len += 2;
						points = realloc(points, point_len * sizeof(int));
						points[point_len - 2] = outline.points[n].x * 16.0f;
						points[point_len - 1] = outline.points[n].y * 16.0f;
						i += 2;
					} else {	   //next off, use middle point
						point_len += 2;
						points = realloc(points, point_len * sizeof(int));
						points[point_len - 2] = (outline.points[c].x + outline.points[c].x) * 16.0f * 0.5f;
						points[point_len - 1] = (outline.points[c].y + outline.points[c].y) * 16.0f * 0.5f;
						++i;
					}
					pnts += 2;
				}
			}
			instruction_len += 1;
			instructions = realloc(instructions, instruction_len * sizeof(unsigned char));
			instructions[instruction_len - 1] = 0;
			s = e;
		}

		for (unsigned int i = 0; i < point_len / 2; ++i) {
			if (points[i * 2] < minx)
				minx = points[i * 2];
			if (points[i * 2] > maxx)
				maxx = points[i * 2];
			if (points[i * 2 + 1] < miny)
				miny = points[i * 2 + 1];
			if (points[i * 2 + 1] > maxy)
				maxy = points[i * 2 + 1];
		}

		VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_S_32,
					   1.0f / 65536.0f, 0.0f, 0, 0,
					   VG_PATH_CAPABILITY_ALL);
		if (instruction_len > 0) {
			vgAppendPathData(path, instruction_len, instructions, points);
		}

		VGfloat mat[9] = {
			size, 0.0f, 0.0f,
			0.0f, size, 0.0f,
			xx, y, 1.0f
		};
		free(points);
		free(instructions);
		vgLoadMatrix(mm);
		vgMultMatrix(mat);
		vgDrawPath(path, VG_FILL_PATH);
		xx += size * (float)face->glyph->advance.x / 4096.0f;
	}
	vgLoadMatrix(mm);
}

// TextWidth returns the width of a text string at the specified font and size.
VGfloat TextWidth(char *s, int pointsize) {
	VGfloat tw = 0.0;
	VGfloat size = (VGfloat) pointsize;
	int character;
	char *ss = s;
	int error;
	while ((ss = readNextChar(ss, &character)) != NULL) {
		int glyphIndex = FT_Get_Char_Index(face, character);

		error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING | FT_LOAD_IGNORE_TRANSFORM);
		assert(error == 0);
		tw += size * (float)face->glyph->advance.x / 4096.0f;
	}
	return tw;
}
#endif



