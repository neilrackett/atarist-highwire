/*******************************************************************************
 *
 * Private declarations for image.c, raster.c and img-dcdr.c
 *
 *******************************************************************************
*/

#ifndef __IMG_DCDR_H__
#define __IMG_DCDR_H__

#include <ldg.h>
#include "mime.h"

typedef struct s_img_info * IMGINFO;
struct s_img_info {
	void          * _priv_data; /* decoder private data */
	void          * _priv_more;
	unsigned char * _priv_load;
	unsigned int    _priv_size;
	BOOL            (*read)(IMGINFO i, CHAR * buffer);
	void            (*quit)(IMGINFO i);
	UWORD           ImgWidth, ImgHeight; /* original size of the image */
	unsigned NumComps :8;  /* 3 = TrueColor, 1 = grayscale or palette mode */
	unsigned BitDepth :8;
	WORD     NumColors;
	unsigned char   * Palette;
	unsigned PalRpos :8;
	unsigned PalGpos :8;
	unsigned PalBpos :8;
	unsigned PalStep :8;
	WORD     Transp;
	WORD     Interlace;
	BOOL     Alpha;    /* rows arrive with an alpha channel after the colour... */
	ULONG    AlphaBg;  /* ...to blend over this 0xRRGGBB, which setup() fills in */
	unsigned char * PalAlpha; /* palette entries under 128 here are clear */
	WORD     NumAlpha;        /* besides Transp: setup() gives them its pixel */
	UWORD    WantW, WantH; /* the size the page will show it at, if it said */
	UWORD    FullW, FullH; /* its real size, when decoded smaller than that */
	/* */
	void   (*raster)(IMGINFO, void * dst);
	void   * RowMem;
	CHAR   * RowBuf;   /* initially RowMem but might be changed for interlaced */
	ULONG    RowBytes; /* >0: RowBuf will cover the whole ig, else one line    */
	void   * DthBuf;
	UWORD    DthWidth; /* calculated width of the image */
	UWORD    PixMask;
	size_t   PgSize;
	size_t   LnSize;
	size_t   IncXfx;
	size_t   IncYfx;
	ULONG    Pixel[256];
};

typedef struct s_decoder {
	struct s_decoder * Next;
	MIMETYPE           Mime[4];
  char               libname[4];
  LDG __CDECL         * libimg;
	BOOL __CDECL        (* open) (IMGINFO i, void *errfunc);
  const char* __CDECL (* info) (void);
  void __CDECL        (* set) (BOOL cfg_val);
} DECODER;

extern DECODER *decoder_chain;

IMGINFO get_decoder (const char * file, UWORD want_w, UWORD want_h);
void *  decoder_load_image(IMGINFO info, const char * image_file);
void    decoders_unload(void);

typedef struct s_rasterizer {
	const char * DispInfo;
	BOOL         StndBitmap;
	void       (*cnvpal)(IMGINFO, ULONG);
	void       (*functn)(IMGINFO, void *);
} * RASTERIZER;

RASTERIZER rasterizer  (UWORD depth, UWORD comps);

#endif /*__IMG_DCDR_H__*/
