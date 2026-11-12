#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ldg.h>

#include "../../src/version.h"
#include "../../src/global.h"
#include "../../src/img-dcdr.h"

/* The maximum number of icons we'll try to handle in any one file */
#define MAXICONS 32

/* Header of a single icon image in an icon file */
typedef struct s_icon_dir_entry {
	CHAR  bWidth;
	CHAR  bHeight;
	CHAR  bColorCount;
	CHAR  bReserved;
#define  bHasMask bReserved /* used for our own purpose */
	UWORD wPlanes;
	UWORD wBitCount;
	ULONG dwBytesInRes;
	ULONG dwImageOffset;
	/*  */
	UWORD depth;
} ICONDIRENTRY;

/* Header of an icon file */
typedef struct s_icon_header {
	UWORD idReserved;
	UWORD idType;
	UWORD idCount;
/*	ICONDIRENTRY  idEntries[MAXICONS];*/
} ICONHEADER;

/* Bitmap header - this is on the images themselves */
typedef struct s_bitmap_info_header {
	ULONG biSize;
	LONG  biWidth;
	LONG  biHeight;
	UWORD biPlanes;
	UWORD biBitCount;
	ULONG biCompression;
	ULONG biSizeImage;
	LONG  biXPelsPerMeter;
	LONG  biYPelsPerMeter;
	ULONG biClrUsed;
	ULONG biClrImportant;
} BITMAPINFOHEADER;

typedef struct s_icon {
	WORD   Inc; /* width, 4byte aligned */
	WORD   Cnt; /* height */
	char * Ptr;
	/* image data follow here */
} * ICON;

BOOL local_FastImages = FALSE;

void (*local_errprintf)(const char *s, ...);

BOOL CDECL decIco_start (IMGINFO info, void * errfunc);
BOOL CDECL decIco_rd_4 (IMGINFO info, CHAR * buffer);
BOOL CDECL decIco_rd_8 (IMGINFO info, CHAR * buffer);
void CDECL decIco_quit (IMGINFO info);
const char* CDECL decIco_info(void);
void CDECL decIco_setFastImage(BOOL set_FastImages);

typedef struct ico_mem_file {
  unsigned char *data;
  int size;
  int offset;
} ico_mem_file;

ico_mem_file ico_mf;


/*---------------------------------------------------------------------------*/
static char mread_char (void)
{
	unsigned char b[1] = {0};
  if (ico_mf.offset + 1 <= ico_mf.size)
  {
    b[0] = ico_mf.data[ico_mf.offset]; ico_mf.offset++;
  }
  return (b[0]);
}

/*---------------------------------------------------------------------------*/
static short mread_word (void)
{
	unsigned char b[2] = {0, 0};
  if (ico_mf.offset + 2 <= ico_mf.size)
  {
    b[0] = ico_mf.data[ico_mf.offset]; ico_mf.offset++;
    b[1] = ico_mf.data[ico_mf.offset]; ico_mf.offset++;
  }
	return (((unsigned short)b[1] <<8) | b[0]);
}

/*---------------------------------------------------------------------------*/
static long mread_long (void)
{
	return (mread_word () | ((long)mread_word () <<16));
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decIco_start (IMGINFO info, void * errfunc)
{
	ICONDIRENTRY * found   = NULL;
	ICONDIRENTRY * entries = NULL;
	UWORD          count   = 0;
  
  ico_mf.data = info->_priv_load;
  ico_mf.size = info->_priv_size;
  ico_mf.offset = 0;

  local_errprintf = errfunc;

	/* Magic number for an icon file */
	/* This is the the idReserved and idType fields of the header */
	const char magic_number[4] = { 0, 0, 1, 0 };
	ICONHEADER iconheader;
  
  if (ico_mf.offset + 4 <= ico_mf.size)
  {
    memcpy(&iconheader, ico_mf.data + ico_mf.offset, 4);
    ico_mf.offset += 4;
  }
 
 	iconheader.idCount    = mread_word ();

	if (memcmp (magic_number, &iconheader, sizeof(magic_number)) != 0)
  {
		local_errprintf ("decIco_start(): not an icon file.\n");
		return FALSE;
  }
  else if ((count = iconheader.idCount) == 0 || count >= MAXICONS)
  {
		local_errprintf ("decIco_start(): wrong icon count %i.\n", count);
    return FALSE;
	}
  else if ((entries = ldg_Calloc (count, sizeof(struct s_icon_dir_entry))) == NULL)
  {
    local_errprintf("decXmp_start(): low memory.\n");
    return FALSE;
  }
#ifdef ICO_DBG
	local_errprintf ("______________________________________________________________\nICO: %i icon%s\n", count, (count == 1 ? "" : "s"));
#endif
	
	if (!entries)
  {
		return FALSE;
	}
  else
  {
		ICONDIRENTRY * entry;
		int            i;

		for (i = 0, entry = entries; i < count; i++, entry++)
    {
			entry->bWidth        = mread_char ();
			entry->bHeight       = mread_char ();
			entry->bColorCount   = mread_char ();
			entry->bReserved     = mread_char ();
			entry->wPlanes       = mread_word ();
			entry->wBitCount     = mread_word ();
			entry->dwBytesInRes  = mread_long ();
			entry->dwImageOffset = mread_long ();
		}
		for (i = 0, entry = entries; i < count; i++, entry++)
    {
			BITMAPINFOHEADER header;
			short            depth;
			long             colors;
#ifdef ICO_DBG
			local_errprintf ("ICO: %i/%i = %i*%i *%i / %i / %i / %li\n", i+1, count, entry->bWidth, entry->bHeight, entry->bColorCount, entry->wPlanes, entry->wBitCount, entry->dwBytesInRes);
#endif
			ico_mf.offset          = entry->dwImageOffset;
			
      header.biSize          = mread_long ();
			header.biWidth         = mread_long ();
			header.biHeight        = mread_long ();
			header.biPlanes        = mread_word ();
			header.biBitCount      = mread_word ();
			header.biCompression   = mread_long ();
			header.biSizeImage     = mread_long ();
			header.biXPelsPerMeter = mread_long ();
			header.biYPelsPerMeter = mread_long ();
			header.biClrUsed       = mread_long ();
			header.biClrImportant  = mread_long ();
			
			depth  = header.biPlanes * header.biBitCount;
			colors = (header.biClrImportant != 0 ? header.biClrImportant :
			          header.biClrUsed      != 0 ? header.biClrUsed      :
			                                       1 << depth);
#ifdef ICO_DBG
			local_errprintf ("           %li*%li  %ix%i %li/%li  -> %i, %li\n", header.biWidth, header.biHeight, header.biPlanes, header.biBitCount, header.biClrImportant, header.biClrUsed, depth, colors);
#endif
			if (depth != 4 && depth != 8) {
#ifdef ICO_DBG
				BOOL verbose = FALSE;
#else
				BOOL verbose = (depth != 1 && depth != 24);
#endif
				if (verbose)
        {
					local_errprintf ("decIco_start(): unsupported color depth %i, skipped.\n", depth);
				}
			}
      else if (!found || (found->bWidth  < entry->bWidth && found->bHeight < entry->bHeight))
      {
				entry->dwImageOffset += header.biSize;
				entry->bColorCount   =  colors -1; /* take care for 256 */
				entry->wBitCount     =  depth;
				entry->bHasMask      = (header.biHeight == entry->bHeight *2);
				found                =  entry;
			}
		}
	}
	
	if (found){
		WORD   inc  = ((found->bWidth +3) & ~3) / (8 / found->wBitCount);
		size_t b_sz = inc * found->bHeight;
		ICON   icon = ldg_Calloc (1, sizeof (struct s_icon) + b_sz);
		size_t c_sz = sizeof(long) * (found->bColorCount +1);
		long * cmap = (icon ? ldg_Calloc (1, c_sz) : NULL);
		if (!cmap)
    {
			if (icon) ldg_Free (icon);
			found = NULL;
			local_errprintf ("decXmp_start(): low memory.\n");
		}
    else
    {
      ico_mf.offset = found->dwImageOffset;
			/* Read the colormap */
      if (ico_mf.offset + (int)c_sz <= ico_mf.size)
      {
        memcpy(cmap, ico_mf.data + ico_mf.offset, c_sz);
        ico_mf.offset += c_sz;
      }
			/* Read the image */
      if (ico_mf.offset + (int)b_sz <= ico_mf.size)
      {
        memcpy(icon + 1, ico_mf.data + ico_mf.offset, b_sz);
        ico_mf.offset += b_sz;
      }
#ifdef ICO_DBG
			local_errprintf ("ICO: found %i*%i *%i / %i   %i %li\n", found->bWidth, found->bHeight, found->bColorCount +1, found->wBitCount, inc, b_sz);
#endif
			icon->Inc = inc;
			icon->Cnt = found->bHeight;
			icon->Ptr = (char*)(icon + 1) + inc * found->bHeight;
			
			info->_priv_data = icon;
			info->read       = (found->wBitCount == 8 ? decIco_rd_8 : decIco_rd_4);
			info->quit       = decIco_quit;
			
			info->ImgWidth   = found->bWidth;
			info->ImgHeight  = found->bHeight;
			info->NumComps   = 1; /* not supported */
			info->BitDepth   = found->wBitCount;
			info->NumColors  = found->bColorCount +1;
			info->Palette = (unsigned char*)cmap;
			info->PalRpos = 2;
			info->PalGpos = 1;
			info->PalBpos = 0;
			info->PalStep = 4;
			info->Transp     = -1;
			info->Interlace  = 0;
		}
	}
	
	ldg_Free (entries);
	
	if (!found)
  {
		return FALSE;
	}
  else if (found->bHasMask)
  {
		/* Check for at least one transparent pixel */
		WORD    transp = -1;
		long    fsave  = ico_mf.offset;
		ICON    icon   = info->_priv_data;
		int     nlongs = (found->bWidth +31) /32;
		int     nbytes = nlongs * 4;
		ULONG   rmask  = ~((1uL << (32 - (found->bWidth & 0x1F))) -1);
		CHAR    cbuf[32]; /* == 256 bits */
		ULONG * lbuf   = (ULONG*)cbuf;
		int     y, x;

		for (y = 0; y < found->bHeight && transp < 0; y++)
    {
      if (ico_mf.offset + nbytes <= ico_mf.size)
      {
        memcpy(cbuf, ico_mf.data + ico_mf.offset, nbytes);
        ico_mf.offset += nbytes;
      }
			lbuf[nlongs -1] &= rmask;
			for (x = 0; x < nlongs; x++) {
				if (lbuf[x])
        {
					transp = 1 << found->wBitCount;
#ifdef ICO_DBG
					local_errprintf ("ICO: transparency detected.\n");
#endif
					break;
				}
			}
		}
		if (transp >= 0) {
			/* search for an available color index */
			CHAR   bit = 0;
			CHAR * ptr = (CHAR*)icon->Ptr;
			memset (cbuf, 0, sizeof(cbuf));
			for (y = 0; y < found->bHeight; y++) {
				CHAR * p = ptr -= icon->Inc;
				for (x = 0; x < found->bWidth; x++) {
					CHAR pix = *(p++);
					cbuf[pix >>3] |= 1 << (7 - (pix & 7));
				}
			}
			ptr = cbuf + sizeof(cbuf);
			while (--transp >= 0) {
				if ((bit <<= 1) == 0) {
					bit = 0x01u;
					ptr--;
				}
				if (!(*ptr & bit)) {
#ifdef ICO_DBG
					local_errprintf ("ICO: transparency index %i %08lX.\n", transp, ((ULONG*)info->Palette)[transp]);
#endif
					break;
				}
			}
		}
		if (transp >= 0) {
			CHAR * ptr = (CHAR*)(icon +1);
			ico_mf.offset = fsave;
			for (y = 0; y < found->bHeight; y++, ptr += icon->Inc) {
				CHAR * p   = ptr;
				CHAR   bit = 0;
				CHAR * msk = cbuf -1;
        if (ico_mf.offset + nbytes <= ico_mf.size)
        {
          memcpy(cbuf, ico_mf.data + ico_mf.offset, nbytes);
          ico_mf.offset += nbytes;
        }
				for (x = 0; x < found->bWidth; x++, p++) {
					if ((bit >>= 1) == 0) {
						if (!(*(++msk))) {
							x += 7;
							p += 7;
							continue;
						}
						bit = 0x80u;
					}
					if (*msk & bit) {
						*p = transp;
					}
				}
			}
			info->Transp = transp;
		}
	}
	
	return TRUE;
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decIco_rd_4 (IMGINFO info, CHAR * buffer)
{
	ICON icon = info->_priv_data;
	if (icon->Cnt > 0) {
		WORD   x = info->ImgWidth / 2;
		WORD   o = info->ImgWidth & 1;
		char * p = icon->Ptr -= icon->Inc;
		while (x--) {
			CHAR pix = *(p++);
			*(buffer++) = pix >> 4;
			*(buffer++) = pix & 0x0F;
		}
		if (o) {
			*(buffer)   = *(p) >> 4;
		}
	}
	return (icon->Cnt-- > 0);
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decIco_rd_8 (IMGINFO info, CHAR * buffer)
{
	ICON icon = info->_priv_data;
	if (icon->Cnt > 0) {
		WORD   x = info->ImgWidth;
		char * p = icon->Ptr -= icon->Inc;
		while (x--) {
			*(buffer++) = *(p++);
		}
	}
	return (icon->Cnt-- > 0);
}

/*----------------------------------------------------------------------------*/
void CDECL decIco_quit (IMGINFO info)
{
	ICON icon = info->_priv_data;
	if (icon) {
		ldg_Free (info->_priv_data);
    ldg_Free (info->Palette);
    info->_priv_data = NULL;

    if (info->_priv_load)
    {
      ldg_Free(info->_priv_load);
      info->_priv_load = NULL;
      info->_priv_size = 0;
    }
 
    ico_mf.data = NULL;
    ico_mf.size = 0;
    ico_mf.offset = 0;
	}
}

/*----------------------------------------------------------------------------*/

const char* CDECL decIco_info()
{
	return "HighWire's (old) ICO decoder " _HIGHWIRE_VERSION_;
}

/*----------------------------------------------------------------------------*/

void CDECL decIco_setFastImage(BOOL set_FastImages)
{
  local_FastImages = set_FastImages;
}

/*----------------------------------------------------------------------------*/

PROC LibFunc[] =
{
  {"hw_dec_open", "BOOL hw_dec_start(IMGINFO info, void * errfunc);\n", decIco_start},
  {"hw_dec_info", "const char * hw_dec_info();\n", decIco_info},
  {"hw_set_fast", "void hw_set_fast(BOOL set_FastImages);\n", decIco_setFastImage},
};

LDGLIB LibLdg[] = { { 0x0001, 3, LibFunc,  "ICO decoder for HighWire", 1 } };

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
