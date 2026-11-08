#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ldg.h>

#include "../../src/global.h"
#include "../../src/logging.h"
#include "../../src/img-dcdr.h"

#include <gif_lib.h>
#undef TRUE
#undef FALSE

#define STRINGIFY(x) #x
#define VERSION_LIB(A,B,C) "LibGIF decoder " STRINGIFY(A) "." STRINGIFY(B) "." STRINGIFY(C) " (The GIFLIB project)"

BOOL local_FastImages = FALSE;

void (*local_errprintf)(const char *s, ...);

BOOL CDECL decGif_start (IMGINFO info, void * errfunc);
BOOL CDECL decGif_read (IMGINFO info, CHAR * buffer);
void CDECL decGif_quit (IMGINFO info);
const char* CDECL decGif_info(void);
void CDECL decGif_setFastImage(BOOL set_FastImages);


/*----------------------------------------------------------------------------*/
static void _PrintGifError(GifFileType *gif)
{
	const char *err = GifErrorString(gif->Error);
	if (err != NULL)
		local_errprintf ("\nGifLib error: %s.\n", err);
	else
		local_errprintf ("\nGifLib undefined error %d.\n", gif->Error);
}
#define PrintGifError() _PrintGifError(gif)

/*----------------------------------------------------------------------------*/

#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct gif_mem_file {
  unsigned char *data;
  int size;
  int offset;
} gif_mem_file;

gif_mem_file gif_mf;

GifFileType *gif_read = NULL;

static int gifldg_read(GifFileType* gif_ptr, GifByteType* data, int count)
{
  gif_mem_file *mf = (gif_mem_file *) gif_ptr->UserData;
    
  count = MIN(count, mf->size - mf->offset);
  
  if (count < 1) { return 0; }

  if (mf->offset + count <= mf->size)
  {
    memcpy(data, mf->data + mf->offset, count);
    mf->offset += count;
    return count;
  }
  
  return 0;
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decGif_start (IMGINFO info, void * errfunc)
{
	short interlace = 0;
	short transpar  = -1;
	short img_w     = 0;
	short img_h     = 0;
	
  local_errprintf = errfunc;
 
	ColorMapObject * map = NULL;
  
  gif_mf.data = info->_priv_load;
  gif_mf.size = info->_priv_size;
  gif_mf.offset = 0;

  int err = 0;
  
	GifFileType * gif = DGifOpen(&gif_mf, gifldg_read, &err);

	if (!gif)
  {
    local_errprintf("DGifOpen failed\n");
		return FALSE;
	}
  else
  {
		GifRecordType rec;
		do
    {
			if (DGifGetRecordType (gif, &rec) == GIF_ERROR)
      {
				local_errprintf ("DGifGetRecordType()");
				PrintGifError();
				break;
			}
      else if (rec == IMAGE_DESC_RECORD_TYPE)
      {
				if (DGifGetImageDesc(gif) == GIF_ERROR)
        {
					local_errprintf("DGifGetImageDesc()");
					PrintGifError();
					break;
				}
        else
        {
					GifImageDesc * dsc = &gif->Image;
					map = (dsc->ColorMap ? dsc->ColorMap : gif->SColorMap);
					img_w = dsc->Width;
					img_h = dsc->Height;
					if (dsc->Interlace) { interlace = 8; }
				}
				break;
			}
      else if (rec == EXTENSION_RECORD_TYPE)
      {
				int           code;
				GifByteType * block;
				if (DGifGetExtension (gif, &code, &block) == GIF_ERROR)
        {
					local_errprintf("DGifGetExtension()");
					PrintGifError();
					break;
        }
        else while (block != NULL)
        {
					if (code == 0xF9 && (block[1] & 1))
          {
						transpar = (short)block[4];
					}
					if (DGifGetExtensionNext (gif, &block) == GIF_ERROR)
          {
						local_errprintf("DGifGetExtensionNext()");
						PrintGifError();
						break;
					}
				}
			}
      else
      {
				local_errprintf("other: %i \n", rec);
				break;
			}
		} while (rec != TERMINATE_RECORD_TYPE);
	}
	
	if (!map || img_w <= 0 || img_w >= 4096 || img_h <= 0 || img_h >= 4096)
  {
		DGifCloseFile (gif, &err);
		return FALSE;
	}
	
	info->_priv_data = gif;
  info->_priv_more = NULL;
	info->read       = decGif_read;
	info->quit       = decGif_quit;
	info->ImgWidth   = img_w;
	info->ImgHeight  = img_h;
	info->NumComps   = 1;
	info->BitDepth   = map->BitsPerPixel;
	info->NumColors  = map->ColorCount;
	if ((info->Palette = &map->Colors->Red) != NULL) {
		GifColorType * rgb = NULL;
		info->PalRpos = (unsigned)&rgb->Red;
		info->PalGpos = (unsigned)&rgb->Green;
		info->PalBpos = (unsigned)&rgb->Blue;
		info->PalStep = (unsigned)&rgb[1];
	}
	info->Transp     = transpar;
	info->Interlace  = interlace;
	
	return TRUE;
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decGif_read (IMGINFO info, CHAR * buffer)
{
	return (DGifGetLine (info->_priv_data, (void *)buffer, info->ImgWidth) == GIF_OK);
}

/*----------------------------------------------------------------------------*/
void CDECL CDECL decGif_quit (IMGINFO info)
{
  int err = 0;

	if (info->_priv_data)
  {
		DGifCloseFile (info->_priv_data, &err);
    
    info->_priv_data = NULL;
    if (info->_priv_load)
    {
      ldg_Free(info->_priv_load);
      info->_priv_load = NULL;
      info->_priv_size = 0;
    }
 
    gif_mf.data = NULL;
    gif_mf.size = 0;
    gif_mf.offset = 0;
	}
}

/*----------------------------------------------------------------------------*/

const char* CDECL decGif_info()
{
  return VERSION_LIB(GIFLIB_MAJOR, GIFLIB_MINOR, GIFLIB_RELEASE);
}

/*----------------------------------------------------------------------------*/

void decGif_setFastImage(BOOL set_FastImages)
{
  local_FastImages = set_FastImages;
}

/*----------------------------------------------------------------------------*/

PROC LibFunc[] =
{
  {"hw_dec_open", "BOOL hw_dec_start(IMGINFO info);\n", decGif_start},
  {"hw_dec_info", "const char * hw_dec_info();\n", decGif_info},
  {"hw_set_fast_image", "void hw_set_fast_image(BOOL set_FastImages);\n", decGif_setFastImage},
};

LDGLIB LibLdg[] = { { 0x0001,  3, LibFunc,  "GIF decoder for HighWire", 1 } };

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
