
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ldg.h>

#include "../../src/global.h"
#include "../../src/img-dcdr.h"

#include <libnsbmp.h>

#define BYTES_PER_PIXEL sizeof(ULONG)
#define TRANSPARENT_COLOR 0xffffffff


BOOL local_FastImages = FALSE;

void (*local_errprintf)(const char *s, ...);

static void *bitmap_create(int width, int height, unsigned int state);
static unsigned char *bitmap_get_buffer(void *bitmap);
static void bitmap_destroy(void *bitmap);

BOOL CDECL decBmp_start (IMGINFO info, void * errfunc);
void CDECL blend_row (IMGINFO info, CHAR * row);
BOOL CDECL decBmp_read (IMGINFO info, CHAR * buffer);
void CDECL decBmp_quit (IMGINFO info);
const char* CDECL decBmp_info(void);
void CDECL decBmp_setFastImage(BOOL set_FastImages);


/*----------------------------------------------------------------------------*/

static void *bitmap_create(int width, int height, unsigned int state)
{
  (void) state;
	/* Ensure a stupidly large bitmap is not created */
	if (width > 4096 || height > 4096) { return NULL; }
  
  return (void *)ldg_Calloc(width * height, BYTES_PER_PIXEL);
}

static unsigned char *bitmap_get_buffer(void *bitmap) { if (bitmap) { return (unsigned char *)bitmap; } return NULL; }

static void bitmap_destroy(void *bitmap) { if (bitmap) { ldg_Free(bitmap); } }

static void warning(const char *context, bmp_result code)
{
  switch (code)
  {
    case BMP_INSUFFICIENT_MEMORY: local_errprintf("%s failed: BMP_INSUFFICIENT_MEMORY\n", context); break;
    case BMP_INSUFFICIENT_DATA:   local_errprintf("%s failed: BMP_INSUFFICIENT_DATA\n", context); break;
    case BMP_DATA_ERROR:          local_errprintf("%s failed: BMP_DATA_ERROR\n", context); break;
    default:                      local_errprintf("%s failed: BMP unknown code %i\n", context, code);
  }
}

static bmp_bitmap_callback_vt bitmap_callbacks = { bitmap_create, bitmap_destroy, bitmap_get_buffer };

static UWORD bmp_line_read;

/*----------------------------------------------------------------------------*/
BOOL CDECL decBmp_start (IMGINFO info, void * errfunc)
{
	BOOL       res  = TRUE;
  bmp_result code = BMP_OK;
  
  bmp_line_read = 0;
  
  local_errprintf = errfunc;
 
  struct bmp_image *bmp = (struct bmp_image *)ldg_Malloc (sizeof(struct bmp_image));
 
  if (bmp == NULL) { warning("bmp_image_alloc", BMP_INSUFFICIENT_MEMORY); res = FALSE; goto cleanup; }

  bmp_create(bmp, &bitmap_callbacks);

  code = bmp_analyse(bmp, (size_t)info->_priv_size, info->_priv_load);
  if (code != BMP_OK) { warning("bmp_analyse", code); res = FALSE; goto cleanup; }

  code = bmp_decode(bmp);
  //code = bmp_decode_trans(bmp, TRANSPARENT_COLOR);

  if (code != BMP_OK)
  {
    warning("bmp_decode", code);
    /* allow partially decoded images */
    if ((code != BMP_INSUFFICIENT_DATA) && (code != BMP_DATA_ERROR)) { res = FALSE; goto cleanup; }
  }

#ifdef BMP_DBG
  local_errprintf("bmp size: %d x %d px\n", bmp->width, bmp->height);
#endif

	info->_priv_data = bmp;
  //info->_priv_more = NULL;
	info->read       = decBmp_read;
	info->quit       = decBmp_quit;
	info->ImgWidth   = bmp->width;
	info->ImgHeight  = bmp->height;
	info->BitDepth   = 8;
  info->NumComps   = 3;
  info->NumColors  = 0;
  //info->Palette    = NULL;
  //info->PalRpos    = 0;
  //info->PalGpos    = 0;
  //info->PalBpos    = 0;
  //info->PalStep    = 0;
	info->Alpha      = TRUE;
  info->Transp     = -1;
  //info->Interlace  = 0;
	
cleanup:
  if (!res)
  {
    if (bmp) { bmp_finalise(bmp); ldg_Free (bmp); }
  }
 
  return res;
}

/*----------------------------------------------------------------------------*/
/* Composite a row of RGBA over info->AlphaBg, in place, leaving the RGB the
 * rasterizers expect.  Most pixels are fully opaque or fully clear, so only
 * the edges pay for the multiplies.
*/
void blend_row (IMGINFO info, CHAR * row)
{
	const UWORD     r_bg = (UWORD)(info->AlphaBg >>16) & 0xFF;
	const UWORD     g_bg = (UWORD)(info->AlphaBg >> 8) & 0xFF;
	const UWORD     b_bg = (UWORD)(info->AlphaBg     ) & 0xFF;
	/* FAST_IMAGES cuts at half way, so nothing is left to blend */
	const UWORD     solid = (local_FastImages ? 128 : 255);
	const UWORD     clear = (local_FastImages ? 127 :   0);
	unsigned char * src  = (unsigned char *)row;
	unsigned char * dst  = src;
	UWORD           n    = info->ImgWidth;

	while (n--) {
		UWORD a = src[3];
		if (a >= solid) {
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
		} else if (a <= clear) {
			dst[0] = r_bg;
			dst[1] = g_bg;
			dst[2] = b_bg;
		} else {
			/* (x + (x >>8)) >>8 divides by 255, rounded, once x carries +128 */
			UWORD b = 255 - a, x;
			x = (UWORD)(src[0] * a + r_bg * b + 128); dst[0] = (x + (x >>8)) >>8;
			x = (UWORD)(src[1] * a + g_bg * b + 128); dst[1] = (x + (x >>8)) >>8;
			x = (UWORD)(src[2] * a + b_bg * b + 128); dst[2] = (x + (x >>8)) >>8;
		}
		src += 4;
		dst += 3;
	}
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decBmp_read (IMGINFO info, CHAR * buffer)
{
  bmp_image *bmp = (bmp_image *)info->_priv_data;
  
  if (!bmp) { return FALSE; }
  if (bmp_line_read >= bmp->height) { return FALSE; }
  if (!bmp->decoded) { return FALSE; }

  CHAR *ptr = (CHAR *)bmp->bitmap;

  ptr += (bmp_line_read * bmp->width) * BYTES_PER_PIXEL;

  memcpy(buffer, (const void *)ptr, (size_t)(bmp->width * BYTES_PER_PIXEL));
  
  bmp_line_read++;

	if (info->Alpha) { blend_row (info, buffer); }

  return (bmp_line_read < bmp->height);
}

/*----------------------------------------------------------------------------*/
void CDECL decBmp_quit (IMGINFO info)
{
  bmp_image *bmp = (bmp_image *)info->_priv_data;
	if (bmp)
  {
    bmp_finalise(bmp);
		ldg_Free (bmp);

    info->_priv_data = NULL;
    if (info->_priv_load)
    {
      ldg_Free(info->_priv_load);
      info->_priv_load = NULL;
      info->_priv_size = 0;
    }
	}
}

/*----------------------------------------------------------------------------*/

const char* CDECL decBmp_info()
{
  return "NetSurf's BMP decoder (based on libnsbmp 0.1.7)";
}

/*----------------------------------------------------------------------------*/

void CDECL decBmp_setFastImage(BOOL set_FastImages)
{
  local_FastImages = set_FastImages;
}

/*----------------------------------------------------------------------------*/

PROC LibFunc[] =
{
  {"hw_dec_open", "BOOL hw_dec_start(IMGINFO info, void * errfunc);\n", decBmp_start},
  {"hw_dec_info", "const char * hw_dec_info();\n", decBmp_info},
  {"hw_set_fast", "void hw_set_fast(BOOL set_FastImages);\n", decBmp_setFastImage},
};

LDGLIB LibLdg[] = { { 0x0001, 3, LibFunc,  "BMP decoder for HighWire", 1 } };

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
