#include <stddef.h>
#include <stdlib.h>

#include <ldg.h>

#include "../../src/global.h"
#include "../../src/img-dcdr.h"

#define XMD_H /* avoid redefining INT16 and INT32, already done in gemlib */
#include <setjmp.h>
#undef INLINE
#undef LOCAL
#undef GLOBAL
#include <jpeglib.h>
#undef XMD_H

#define STRINGIFY(x) #x
#define VERSION_LIB(A,B) "LibJPEG decoder " STRINGIFY(A) "." STRINGIFY(B) ".0  (Independent JPEG Group)"

typedef struct jpeg_decompress_struct * JPEG_DEC;
typedef struct jpeg_error_mgr         * JPEG_ERR;

BOOL local_FastImages = FALSE;

void (*local_errprintf)(const char *s, ...);

static void _jpeg_shutup (j_common_ptr cinfo, int msg_level);
static void _jpeg_errjmp (j_common_ptr cinfo);

BOOL CDECL decJpg_start (IMGINFO info, void * errfunc);
BOOL CDECL decJpg_read (IMGINFO info, CHAR * buffer);
void CDECL decJpg_quit (IMGINFO info);
const char* CDECL decJpg_info(void);
void CDECL decJpg_setFastImage(BOOL set_FastImages);


/*----------------------------------------------------------------------------*/
static void _jpeg_shutup (j_common_ptr cinfo, int msg_level)
{
	(void)cinfo;
	(void)msg_level;
/*	puts("blah...");*/
}

/*----------------------------------------------------------------------------*/
static void _jpeg_errjmp (j_common_ptr cinfo)
{
	jmp_buf * escape = cinfo->client_data;
	if (!escape) {
		//hwUi_fatal ("image::jpeg_error", "got lost!");
	} else {
		longjmp (*escape, TRUE);
	}
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decJpg_start (IMGINFO info, void * errfunc)
{
	jmp_buf escape;
	
	volatile JPEG_DEC jpeg = NULL;
	volatile JPEG_ERR jerr = NULL;
	int header = 0;
	
  local_errprintf = errfunc;
 
  if ((jpeg = ldg_Calloc (1, sizeof(struct jpeg_decompress_struct))) == NULL ||
      (jerr = ldg_Calloc (1, sizeof(struct jpeg_error_mgr)))         == NULL)
  {
		local_errprintf ("decJpg_start(): low memory.\n");
		if (jerr) ldg_Free (jerr);
		if (jpeg) ldg_Free (jpeg);
		return FALSE;
	}
	
	if (setjmp (escape)) {
		if (header >= 2) {
			jpeg_abort_decompress (jpeg);
		}
		jpeg_destroy_decompress (jpeg);
		ldg_Free (jerr);
		ldg_Free (jpeg);
		return (header != 0);
	}

	jpeg->err               = jpeg_std_error (jerr);
	jpeg->err->emit_message = _jpeg_shutup;
	jpeg->err->error_exit   = _jpeg_errjmp;
	jpeg->client_data       = &escape;
	
	jpeg_create_decompress (jpeg);
	jpeg_mem_src           (jpeg, info->_priv_load, (unsigned long)info->_priv_size);
	jpeg_read_header       (jpeg, TRUE);

	/* Shown at half the size or less, the IDCT can produce that directly:
	 * the scaling below would only throw the extra pixels away, at the cost
	 * of decoding and converting them first.  Take the smallest of 1/8, 1/4
	 * and 1/2 that still covers the size asked for; a side not asked for is
	 * 0, which any size covers. */
	if (info->WantW || info->WantH) {
		unsigned d = 8;
		while (d > 1 && ((jpeg->image_width  + d -1) / d < info->WantW ||
		                 (jpeg->image_height + d -1) / d < info->WantH)) {
			d /= 2;
		}
		if (d > 1) {
			jpeg->scale_num   = 1;
			jpeg->scale_denom = d;
			info->FullW = jpeg->image_width;
			info->FullH = jpeg->image_height;
		}
	}

	jpeg->dct_method          = JDCT_IFAST;
#ifdef __M68881__
	jpeg->do_fancy_upsampling = TRUE;
#else
	jpeg->do_fancy_upsampling = FALSE;
#endif
	
	switch (jpeg->out_color_space) {
		case JCS_RGB:
			info->NumComps = 3;
			break;
		case JCS_GRAYSCALE:
			info->NumComps = 1;
			break;
		default: ;
			jpeg->out_color_space = JCS_RGB;
			info->NumComps = jpeg->out_color_components = 3;
			break;
	}
	header = 1;
	jpeg_start_decompress  (jpeg);
	header = 2;
	
	info->_priv_data = jpeg;
/*	info->_priv_more = NULL; */
	info->read       = decJpg_read;
	info->quit       = decJpg_quit;
	info->ImgWidth   = jpeg->output_width;
	info->ImgHeight  = jpeg->output_height;
	info->BitDepth   = 8;
/*	info->NumColors  = 0;    0-values needn't to be set */
/*	info->Palette    = NULL; */
/*	info->PalRpos    = 0; */
/*	info->PalGpos    = 0; */
/*	info->PalBpos    = 0; */
/*	info->PalStep    = 0; */
	info->Transp     = -1;
/*	info->Interlace  = 0; */
	
	jpeg->client_data = NULL; /* unset jump buffer */
	
	return TRUE;
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decJpg_read (IMGINFO info, CHAR * buffer)
{
	JPEG_DEC jpeg = info->_priv_data;

	return (jpeg_read_scanlines (info->_priv_data, (JSAMPROW*)&buffer, 1) < jpeg->output_height);
}

/*----------------------------------------------------------------------------*/
void CDECL decJpg_quit (IMGINFO info)
{
	JPEG_DEC jpeg = info->_priv_data;
	if (jpeg) {
		jmp_buf escape;
		jpeg->client_data = &escape;
		if (!setjmp (escape)) {
			jpeg_finish_decompress (jpeg);
		}
		jpeg->client_data = NULL;
		jpeg_destroy_decompress (jpeg);
		ldg_Free (jpeg->err);
		ldg_Free (jpeg);

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

const char* CDECL decJpg_info()
{
  return VERSION_LIB(JPEG_LIB_VERSION_MAJOR, JPEG_LIB_VERSION_MINOR);
}

/*----------------------------------------------------------------------------*/

void CDECL decJpg_setFastImage(BOOL set_FastImages)
{
  local_FastImages = set_FastImages;
}

/*----------------------------------------------------------------------------*/

PROC LibFunc[] =
{
  {"hw_dec_open", "BOOL hw_dec_start(IMGINFO info, void * errfunc);\n", decJpg_start},
  {"hw_dec_info", "const char * hw_dec_info();\n", decJpg_info},
  {"hw_set_fast_image", "void hw_set_fast_image(BOOL set_FastImages);\n", decJpg_setFastImage},
};

LDGLIB LibLdg[] = { { 0x0001, 3, LibFunc,  "JPG decoder for HighWire", 1 } };

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
