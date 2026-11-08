#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ldg.h>

#include "../../src/global.h"
#include "../../src/img-dcdr.h"

#include <setjmp.h>
#include <png.h>

#define PNG_BYTES_TO_CHECK 8

BOOL local_FastImages = FALSE;

void (*local_errprintf)(const char *s, ...);

static void pngldg_read(png_struct *png_ptr, png_bytep data, png_size_t count);
void *pngldg_malloc(png_struct *png_ptr, png_alloc_size_t size);
void pngldg_free(png_struct *png_ptr, void *ptr);

BOOL CDECL decPng_start (IMGINFO info, void * errfunc);
void CDECL blend_row (IMGINFO info, CHAR * row);
BOOL CDECL decPng_read (IMGINFO info, CHAR * buffer);
BOOL CDECL decPng_readi (IMGINFO info, CHAR * buffer);
void CDECL decPng_quit  (IMGINFO info);
const char* CDECL decPng_info(void);
void CDECL decPng_setFastImage(BOOL set_FastImages);

typedef struct png_mem_file {
  png_bytep data;
  png_uint_32 size;
  png_uint_32 offset;
} png_mem_file;

png_mem_file png_mf;

png_structp  png_read = NULL;
png_infop    img_info;

/*----------------------------------------------------------------------------*/
#define MIN(a,b) (((a)<(b))?(a):(b))

static void pngldg_read(png_struct *png_ptr, png_bytep data, png_size_t count)
{
  png_mem_file *mf = (png_mem_file *)png_get_io_ptr(png_ptr);
  
  count = MIN(count, mf->size - mf->offset);
  
  if (count < 1) { return; }
  
  if (mf->offset + count <= mf->size)
  {
    memcpy(data, mf->data + mf->offset, count);
    mf->offset += count;
  }
}

void *pngldg_malloc(png_struct *png_ptr, png_alloc_size_t size) { return ldg_Malloc(size); }
void pngldg_free(png_struct *png_ptr, void *ptr) { ldg_Free(ptr); }

/*----------------------------------------------------------------------------*/
BOOL CDECL decPng_start (IMGINFO info, void * errfunc)
{
	png_structp png_ptr;
	png_infop   info_ptr = NULL;
	png_color *palette;
	int num_colors;
	int color_type;
	BOOL alpha  = FALSE;
	WORD transp = -1;
	unsigned char * pal_alpha = NULL;
	WORD            num_alpha = 0;

  local_errprintf = errfunc;

  png_mf.data = info->_priv_load;
  png_mf.size = info->_priv_size;
  png_mf.offset = 0;

  if (png_sig_cmp(png_mf.data, 0, PNG_BYTES_TO_CHECK)) { return FALSE; }

  png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, pngldg_malloc, pngldg_free);
	if (!png_ptr || (info_ptr = png_create_info_struct (png_ptr)) == NULL)
  {
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
		local_errprintf ("decPng_start(): low memory.\n");
		return FALSE;
	}
	if (setjmp (png_jmpbuf (png_ptr))) {
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
		return FALSE;
	}
  png_set_read_fn(png_ptr, &png_mf, pngldg_read);
	png_read_info (png_ptr, info_ptr);
	
	if (png_get_bit_depth(png_ptr, info_ptr) == 16)
  {
		png_set_strip_16 (png_ptr);
	}
  else if (png_get_bit_depth(png_ptr, info_ptr) < 8)
  {
		png_set_packing (png_ptr);
		if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY)
    {
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		}
	}
	/* A palette whose entries are each fully clear or fully solid stays a
	 * palette, its first clear entry taking a GIF's kind of transparency and
	 * setup() handing any others the same pixel.  FAST_IMAGES counts every
	 * entry under half as clear and the rest as solid, so no palette leaves.
	 * Anything else that is not opaque becomes RGBA, for the rows to be
	 * blended over the background as they are read. */
	color_type = png_get_color_type (png_ptr, info_ptr);
	if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS)) {
		png_bytep trns   = NULL;
		int       n_trns = 0;
		png_get_tRNS (png_ptr, info_ptr, &trns, &n_trns, NULL);
		if (color_type == PNG_COLOR_TYPE_PALETTE) {
			int i, clear = 0;
			for (i = 0; i < n_trns && !alpha; i++) {
				if (trns[i] < 128 && (local_FastImages || !trns[i])) {
					if (!clear++) transp = i;
				} else if (trns[i] != 255 && !local_FastImages) {
					alpha = TRUE;
				}
			}
			if (alpha) {
				transp = -1;
				png_set_palette_to_rgb (png_ptr);
			} else if (clear > 1) {
				pal_alpha = trns;
				num_alpha = n_trns;
			}
		} else {
			alpha = TRUE;
		}
		if (alpha) {
			png_set_tRNS_to_alpha (png_ptr);
		}
	} else if (color_type & PNG_COLOR_MASK_ALPHA) {
		alpha = TRUE;
	}
	if (alpha && !(color_type & PNG_COLOR_MASK_COLOR)) {
		png_set_gray_to_rgb (png_ptr);
	}
	info->_priv_data = png_ptr;
	info->_priv_more = info_ptr;
	info->read       = decPng_read;
	info->quit       = decPng_quit;
	info->ImgWidth   = png_get_image_width(png_ptr, info_ptr);
	info->ImgHeight  = png_get_image_height(png_ptr, info_ptr);
	info->NumComps   = (png_get_channels(png_ptr, info_ptr) >= 3 ? 3 : 1);
	info->BitDepth   = png_get_bit_depth(png_ptr, info_ptr);
	if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_colors) & PNG_INFO_PLTE) {
		info->Palette = (unsigned char *)palette;
		info->NumColors = num_colors;
#if 1 /* bug-endian */
		info->PalRpos = (int)(offsetof(png_color, red) + sizeof(palette->red) - 1);
		info->PalGpos = (int)(offsetof(png_color, green) + sizeof(palette->green) - 1);
		info->PalBpos = (int)(offsetof(png_color, blue) + sizeof(palette->blue) - 1);
#else
		info->PalRpos = (int)offsetof(png_color, red);
		info->PalGpos = (int)offsetof(png_color, green);
		info->PalBpos = (int)offsetof(png_color, blue);
#endif
		info->PalStep = (int)sizeof(png_color);
	} else
	{
		info->Palette = NULL;
		info->NumColors = 0;
	}
	if (alpha) {
		info->NumComps  = 3;
		info->BitDepth  = 8;
		info->Palette   = NULL;
		info->NumColors = 0;
	}
	info->Alpha      = alpha;
	info->Transp     = transp;
	info->PalAlpha   = pal_alpha;
	info->NumAlpha   = num_alpha;
	info->Interlace  = 0;
	
	if (png_get_interlace_type(png_ptr, info_ptr) == PNG_INTERLACE_ADAM7) {
		png_set_interlace_handling (png_ptr);
		png_read_update_info (png_ptr, info_ptr);
		info->RowBytes = png_get_rowbytes (png_ptr, info_ptr);
		info->read     = decPng_readi;
	} else {
		png_read_update_info (png_ptr, info_ptr);
	}
	return TRUE;
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
BOOL CDECL decPng_read (IMGINFO info, CHAR * buffer)
{
	png_structp png_ptr = info->_priv_data;
	
  if (setjmp (png_jmpbuf (png_ptr))) { return FALSE; }

	png_read_row (png_ptr, (png_bytep)buffer, NULL);

	if (info->Alpha) { blend_row (info, buffer); }
	
  return TRUE;
}
	
/*----------------------------------------------------------------------------*/
BOOL CDECL decPng_readi (IMGINFO info, CHAR * buffer)
{
	png_structp png_ptr  = info->_priv_data;
	
	(void)buffer;

	if (setjmp (png_jmpbuf (png_ptr))) { return FALSE; }
	
	if (png_get_current_row_number(png_ptr) == 0)
  {
		ULONG       rowbytes      = info->RowBytes;
		png_bytep * rowptrs       = info->RowMem;
		png_bytep   row           = info->RowBuf;
		int         number_passes = png_set_interlace_handling (png_ptr), pass, y;
		
		/* setup row pointer array */
		for (y = 0; y < info->ImgHeight; rowptrs[y++] = row, row += rowbytes);
		
		for (pass = 0; pass < number_passes -1; pass++)
    {
			png_read_rows (png_ptr, rowptrs, NULL, info->ImgHeight);
		}
	}
  else
  {
		info->RowBuf += info->RowBytes;
	}
	png_read_rows (png_ptr, (png_bytep*)&info->RowBuf, NULL, 1);
	
  if (info->Alpha) { blend_row (info, info->RowBuf); }

	return TRUE;
}

/*----------------------------------------------------------------------------*/
void CDECL decPng_quit  (IMGINFO info)
{
	png_structp png_ptr  = info->_priv_data;
	png_infop   info_ptr = info->_priv_more;
	if (png_ptr) {
		if (!setjmp (png_jmpbuf (png_ptr))) {
			png_read_end (png_ptr,  info_ptr);
		}
		png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    if (info->_priv_load)
    {
      ldg_Free(info->_priv_load);
      info->_priv_load = NULL;
      info->_priv_size = 0;
    }
		info->_priv_more = NULL;
		info->_priv_data = NULL;

    png_mf.data = NULL;
    png_mf.size = 0;
    png_mf.offset = 0;
	}
}

/*----------------------------------------------------------------------------*/

const char* CDECL decPng_info()
{
  return "LibPNG decoder " PNG_LIBPNG_VER_STRING " (The PNG Development Group)";
}

/*----------------------------------------------------------------------------*/

void CDECL decPng_setFastImage(BOOL set_FastImages)
{
  local_FastImages = set_FastImages;
}

/*----------------------------------------------------------------------------*/

PROC LibFunc[] =
{
  {"hw_dec_open", "BOOL hw_dec_start(IMGINFO info, void * errfunc);\n", decPng_start},
  {"hw_dec_info", "const char * hw_dec_info();\n", decPng_info},
  {"hw_set_fast_image", "void hw_set_fast_image(BOOL set_FastImages);\n", decPng_setFastImage},
};

LDGLIB LibLdg[] = { { 0x0001, 3, LibFunc,  "PNG decoder for HighWire", 1 } };

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
