#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <gem.h>
#include <ldg.h>

#include "global.h"
#include "Logging.h"
#include "mime.h"
#include "img-dcdr.h"

// from less used to most used
DECODER _decoder_xmp = { NULL, { MIME_IMG_X_XBM, MIME_IMG_X_XPM, 0 }, "xbm", NULL, NULL, NULL, NULL };
DECODER _decoder_ico = { &_decoder_xmp, { MIME_IMG_X_ICON, 0 }, "ico", NULL, NULL, NULL, NULL };
DECODER _decoder_jpg = { &_decoder_ico, { MIME_IMG_JPEG, 0 }, "jpg", NULL, NULL, NULL, NULL };
DECODER _decoder_png = { &_decoder_jpg, { MIME_IMG_PNG, 0 }, "png", NULL, NULL, NULL, NULL };
DECODER _decoder_gif = { &_decoder_png, { MIME_IMG_GIF, 0 }, "gif", NULL, NULL, NULL, NULL };
 
DECODER *decoder_chain = &_decoder_gif;

/*----------------------------------------------------------------------------*/
IMGINFO get_decoder (const char * file, UWORD want_w, UWORD want_h)
{
	char buf[64];
  	
	IMGINFO info = (decoder_chain ? calloc (1, sizeof (struct s_img_info)) : NULL);
	
  if (info)
  {
		memset (info, 0, offsetof (struct s_img_info, Pixel));

    if (!decoder_load_image(info, file))
    {
			free (info);
			info = NULL;
    }
  }
 
	if (info)
  {
		DECODER * decoder = NULL;
		DECODER * found   = NULL;
		MIMETYPE  mime    = mime_byExtension (file, NULL, NULL);
		info->WantW = want_w;
		info->WantH = want_h;
		if (MIME_Major(mime) && MIME_Minor(mime)) {
			found = decoder_chain;
			while (found) {
				MIMETYPE * mptr = found->Mime;
				while (*mptr && *mptr != mime) {
					mptr++;
				}
				if (*mptr)
        {
          if (found->libimg == NULL)
          {
            sprintf (buf, "modules\\imgd-%s.ldg", found->libname);

            found->libimg = ldg_open(buf, aes_global);
            
            if (logging_is_on && found->libimg) { logprintf(LOG_LMAGENTA, "%s loaded\n", buf); }
          }
          if (found->libimg)
          {
            if (found == &_decoder_png)
            {
              found->set = ldg_find("hw_set_fast_image", found->libimg);
              if (found->set) { (*found->set)(cfg_FastImages); }
            }

            found->open = ldg_find("hw_dec_open", found->libimg);
            if (found->open) { if ((*found->open)(info, &errprintf)) { decoder = found; } }
          }
					break;
				}
				found = found->Next;
			}
		}
		if (!decoder) { // not found using mime, so load all decoders to test file
			decoder = decoder_chain;
			while (decoder)
      {
        if (decoder != found)
        {
          if (decoder->libimg == NULL)
          {
            sprintf (buf, "modules\\imgd-%s.ldg", decoder->libname);

            decoder->libimg = ldg_open(buf, aes_global);
            
            if (logging_is_on && decoder->libimg) { logprintf(LOG_LMAGENTA, "%s loaded\n", buf); }
          }
          if (decoder->libimg)
          {
            if (decoder == &_decoder_png)
            {
              decoder->set = ldg_find("hw_set_fast_image", decoder->libimg);
              if (decoder->set) { (*decoder->set)(cfg_FastImages); }
            }

            decoder->open = ldg_find("hw_dec_open", decoder->libimg);
            if (*decoder->open) { if ((*decoder->open)(info, &errprintf)) { break; } }
          }
        }
				decoder = decoder->Next;
			}
		}
		if (!info->_priv_data)
    {
      if (info->_priv_load) { ldg_Free(info->_priv_load); }
			free (info);
			info = NULL;
		}
	}
	return info;
}

/*----------------------------------------------------------------------------*/
void * decoder_load_image(IMGINFO info, const char * image_file)
{
  void *ptr = NULL;

  info->_priv_load = NULL;
  info->_priv_size = 0;

  FILE *f = fopen(image_file, "rb");
  if (f)
  {
    size_t len;
            
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (len > 0)
    {
      ptr = (void *)ldg_Calloc(1, (int32_t)(len + (size_t)256));

      if (ptr)
      {
        if (fread(ptr, sizeof(int8_t), len, f) == len)
        {
          info->_priv_load = ptr;
          info->_priv_size = (unsigned int)len;
        }
        else
        {
          ldg_Free(ptr);
          ptr = NULL;
          if (logging_is_on) { logprintf(LOG_LMAGENTA, "%s not loaded as expected\n", image_file); }
        }
      }
      else { ptr = NULL; }
    }

    fclose (f);
  }
  return ptr;
}

/*----------------------------------------------------------------------------*/
void decoders_unload(void)
{
  DECODER * found = decoder_chain;
  while (found)
  {
    if (found->libimg)
    {
      ldg_close(found->libimg, aes_global);
      
      if (logging_is_on) { logprintf(LOG_LMAGENTA, "modules\\imgd-%s.ldg unloaded\n", found->libname); }

      found->libimg = NULL;
      found->open   = NULL;
      found->info   = NULL;
      found->set    = NULL;
    }
    found = found->Next;
  }
}
