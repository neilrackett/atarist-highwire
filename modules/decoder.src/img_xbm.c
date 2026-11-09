#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <ldg.h>

#include "../../src/version.h"
#include "../../src/global.h"
#include "../../src/img-dcdr.h"

#define TYPE_XBM   ((((((long)'X' <<8) | 'B') <<8) | 'M') <<8)
#define TYPE_XPM   ((((((long)'X' <<8) | 'P') <<8) | 'M') <<8)
typedef struct s_xmap {
	ULONG   Type;
	ULONG * PixChrs;
	WORD    NumChrs;
	WORD    Colors;
	WORD    W, H;
	char    Buffer[1024];
} * XMAP;

BOOL local_FastImages = FALSE;

void (*local_errprintf)(const char *s, ...);

BOOL CDECL decXbm_start (IMGINFO info, void * errfunc);
BOOL CDECL decXbm_r_bit (IMGINFO info, CHAR * buffer);
BOOL CDECL decXbm_r_pix (IMGINFO info, CHAR * buffer);
void  CDECL decXbm_quit (IMGINFO info);
const char* CDECL decXbm_info(void);
void CDECL decXbm_setFastImage(BOOL set_FastImages);

/*----------------------------------------------------------------------------*/

typedef struct xmp_mem_file {
  unsigned char *data;
  int size;
  int offset;
} xmp_mem_file;

xmp_mem_file xmp_mf;

static char * mgets(char *buf, int max)
{
  int count = 0;
  int exit  = 0;
  unsigned char c;
  
  memset(buf, 0, max);

  do
  {
    c = xmp_mf.data[xmp_mf.offset + count];

    if (c == '\0') { exit = 1; } else { if (c == '\n') { exit = 1; count++; } else { buf[count] = c; count++; } }
    
    if ((xmp_mf.offset + count) == (xmp_mf.size - 1)) { exit = 1; }
  }
  while (!exit);
  
  xmp_mf.offset += count;

  count = strlen(buf);

  if (count > 0) { if (buf[count - 1] == '\r') { buf[count - 1] = '\0'; } }

  if (xmp_mf.offset >= (xmp_mf.size - 1)) { return NULL; }

  return buf;
}

static int inraster  = FALSE;
static int incomment = FALSE;

static char * xmap_read (XMAP xmap, BOOL all)
{
	char * com;
  char * beg;
	char * end;

	while ((beg = mgets (xmap->Buffer,(int)sizeof(xmap->Buffer))) != NULL) {
		while (isspace (*beg)) beg++;
		if (*beg) {
			if (!all && (beg[0] == '/' && beg[1] == '*')
			         && (end = strstr (beg +2, "*/")) != NULL) {
				beg = end +2 +1;
				while (isspace (*beg)) beg++;
				if (!*beg) continue;
			}
			end = strchr (beg, '\0');
			while (isspace (end[-1])) end--;
			*end = '\0';

      if (!inraster)
      {
        if ((com = strstr(beg, "/*")) != NULL && (end = strstr(beg, "*/")) == NULL)
        {
          incomment = 1;
          *com = '\0';
        }
        else if (incomment && (end = strstr(beg, "*/")) != NULL && (com = strstr(beg, "/*")) == NULL)
        {
          incomment = 0;
          beg = end + 2;
        }
        if (incomment) { continue; }
        if (*beg == '\0') { continue; }
      }

      break;
		}
	}
 
#ifdef XBM_DBG
  //local_errprintf("xmap line:%s\n", beg);
#endif
	return beg;
}

static BOOL xmap_get_color(XMAP xmap, char *s, char *i, char *val)
{
  char *p;

  *i = (ULONG)0;
  memset(val, 0, 20);
  
  if (*s != '\"') { return FALSE; }
  s++;
  
  p = s + xmap->NumChrs;
  *p = '\0';
  
  strncpy(i, s, xmap->NumChrs);
 
  s = p;
  do { s++; } while((*s != 'c') && (*s != 'C'));
  s++;
  while(isspace(*s)) s++;
  p = s + 1;
  while(*p != '\"') p++;
  *p = '\0';
    
  if (strlen(s)) { strncpy(val, s, 19); return TRUE; }
  
  return FALSE;
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decXbm_start (IMGINFO info, void * errfunc)
{
	WORD   depth  = 1;
	WORD   transp = -1;
	XMAP   xmap;
	char * p;
  
  local_errprintf = errfunc;

  xmp_mf.data = info->_priv_load;
  xmp_mf.size = info->_priv_size + 1; // with terminal nullbyte
  xmp_mf.offset = 0;

  inraster = FALSE;

  if ((xmap = ldg_Calloc (1, sizeof(struct s_xmap))) == NULL)
  {
		local_errprintf ("decXbm_start(): low memory.");
		return FALSE;
	}
	
	xmap->Type    = 0ul;
	xmap->PixChrs = NULL;
	xmap->Colors  = 2;
	xmap->W       = 0;
	xmap->H       = 0;
	
	while ((p = xmap_read (xmap, !xmap->Type)) != NULL) {
		if (strncmp (p, "#define ", 8) == 0
		    && (p = strrchr (p +8, '_')) != NULL) {
			p++;
			if (sscanf (p, "width  %hi", &xmap->W) == 1 && xmap->W > 0) continue;
			if (sscanf (p, "height %hi", &xmap->H) == 1 && xmap->H > 0) continue;
		
		} else if (p[0] == '/' && p[1] == '*') {
			if (strcmp (p +2, " XPM */") == 0) {
				xmap->Type = TYPE_XPM;
			}
			continue;
		
		} else if (strncmp (p, "static char", 11) == 0) {
			if (!xmap->Type) {
				char * q = strrchr (p +11, '_');
				if (q && strcmp (q +1, "bits[] = {") == 0) {
					xmap->Type = TYPE_XBM;
				}
			} else { /* XPM */
				char * q = strrchr (p +11, '*');
				if (q == NULL) { xmap->Type = 0ul; }
        else
        {
          q++;
          if ((q = strchr (q, '[')) == NULL) { xmap->Type = 0ul; }
          else
          {
            q++;
            if (strcmp (q, "] = {") != 0) { xmap->Type = 0ul; }
          }
				}
			}
		}
		break;
	}
	if (xmap->Type == TYPE_XPM) {
		ULONG * pix = NULL, * map = NULL;
		if ((p = xmap_read (xmap, FALSE)) != NULL) {
      int i = sscanf (p, "\" %hd %hd %hd %hd \",",
			                &xmap->W, &xmap->H, &xmap->Colors, &xmap->NumChrs);
			if (i == 4 && xmap->W > 0 && xmap->H > 0
			    && xmap->Colors >= 2 && xmap->Colors <= 255
			    && xmap->NumChrs >= 1 && xmap->NumChrs <= 3) {
				pix = ldg_Calloc (1, sizeof(ULONG) *256 *2);
				map = pix +256;
			}
			if (pix) {
				char val[20];
				memset (pix, 0, (char*)map - (char*)pix);
				for (i = 0; i < xmap->Colors; i++) {
					BOOL ok = FALSE;
          p = xmap_read (xmap, FALSE);
          if (p != NULL)
          {
            if (xmap_get_color(xmap, p, (char*)&pix[i], val))
            {
              if (strlen(val))
              {
                if (val[0] == '#' && isxdigit (val[1]))
                {
                  map[i] = strtoul (val +1, NULL, 16);
                  ok = TRUE;
                }
                else if (strnicmp ("None", val, 4) == 0)
                {
                  map[i] = 0ul; /* transparency value */
                  transp = i;
                  ok = TRUE;
                }
              }
            }
          }
					if (!ok)
          {
						ldg_Free (pix);
						pix = NULL;
						break;
					}
				}
			}
		}
		
		if (pix) {
			depth         = 8;
			xmap->PixChrs = pix;
			info->Palette = (unsigned char*)map;
			info->PalRpos = 1;
			info->PalGpos = 2;
			info->PalBpos = 3;
			info->PalStep = 4;
		} else {
			xmap->Type = 0ul;
		}
	}

#ifdef XBM_DBG
  local_errprintf("xmap type  : TYPE_%s\n", xmap->Type == TYPE_XBM ? "XBM" : xmap->Type == TYPE_XPM ? "XPM" : "?");
  local_errprintf("xmap size  : %d x %d px\n", xmap->W, xmap->H);
  local_errprintf("xmap colors: %d\n", xmap->Colors);
#endif

	if (!xmap->Type) {
    if (xmap->PixChrs) { ldg_Free (xmap->PixChrs); }
    ldg_Free (xmap);
		return FALSE;
	}
	
	info->_priv_data = xmap;
	info->read       = (xmap->Type == TYPE_XBM ? decXbm_r_bit : decXbm_r_pix);
	info->quit       = decXbm_quit;
	
	info->ImgWidth   = xmap->W;
	info->ImgHeight  = xmap->H;
	info->NumComps   = 1;
	info->BitDepth   = depth;
	info->NumColors  = xmap->Colors;
	info->Transp     = transp;
	info->Interlace  = 0;
	
	xmap->Buffer[0]  = '\0';

  inraster = TRUE;

	return TRUE;
}

/*----------------------------------------------------------------------------*/
BOOL decXbm_r_bit (IMGINFO info, CHAR * buffer)
{
	XMAP xmap = info->_priv_data;
	if (xmap->H-- >= 0) {
		UWORD  mask = 0;
		UWORD  val  = -1;
		WORD   x    = xmap->W;
		char * p    = xmap->Buffer;
		while (x--) {
			int n;
			if (!mask) {
				int v1, v2;
				if ((!*p && (p = xmap_read (xmap, FALSE)) == NULL)
            || sscanf (p, "0x%x,%n", &v1, &n) != 2) {
				xmap->H = -1;
					break;
				}
				p += n;
				while (isspace (*p)) p++;
				if (x >= 8) {
					if ((!*p && (p = xmap_read (xmap, FALSE)) == NULL)
					    || sscanf (p, "0x%x,%n", &v2, &n) != 2) {
						xmap->H = -1;
						break;
					}
					p += n;
					while (isspace (*p)) p++;
				} else {
					v2 = 0;
				}
				val = (v2 <<8) | v1;
				mask = 0x0001;
			}
			*(buffer++) = (val & mask ? 1 : 0);
			mask      <<= 1;
		}
		val = 0;
		while ((xmap->Buffer[val++] = *(p++)) != '\0');
	}
	return (xmap->H >= 0);
}

/*----------------------------------------------------------------------------*/
BOOL CDECL decXbm_r_pix (IMGINFO info, CHAR * buffer)
{
	XMAP xmap = info->_priv_data;
	if (xmap->H-- >= 0) {
		WORD   x = xmap->W;
		char * p = xmap_read (xmap, FALSE);
    if (!p) { return FALSE; }
		if (!*p || *p != '"') {
			xmap->H = -1;
			x       = 0;
		}
		while (x--) {
			WORD   num = xmap->NumChrs;
			ULONG  pix = 0uL;
			char * chr = (char*)&pix;
			while (num-- && (*(chr++) = *(++p)) != '\0');
			num = xmap->Colors;
			while (num-- && pix != xmap->PixChrs[num]);
			if (num < 0) {
				xmap->H = -1;
				break;
			}
			*(buffer++) = num;
		}
		if (!*p || *(++p) != '"') {
			xmap->H = -1;
		}
	}
	return (xmap->H >= 0);
}

/*----------------------------------------------------------------------------*/
void CDECL decXbm_quit (IMGINFO info)
{
	XMAP xmap = info->_priv_data;
	if (xmap) {
		if (xmap->PixChrs) ldg_Free (xmap->PixChrs);
		ldg_Free (xmap);
		info->_priv_data = NULL;

    if (info->_priv_load)
    {
      ldg_Free(info->_priv_load);
      info->_priv_load = NULL;
      info->_priv_size = 0;
    }
 
    xmp_mf.data = NULL;
    xmp_mf.size = 0;
    xmp_mf.offset = 0;
	}
}

/*----------------------------------------------------------------------------*/

const char* CDECL decXbm_info()
{
	return "HighWire's BMP decoder " _HIGHWIRE_VERSION_;
}

/*----------------------------------------------------------------------------*/

void CDECL decXbm_setFastImage(BOOL set_FastImages)
{
  local_FastImages = set_FastImages;
}

/*----------------------------------------------------------------------------*/

PROC LibFunc[] =
{
  {"hw_dec_open", "BOOL hw_dec_open(IMGINFO info);\n", decXbm_start},
  {"hw_dec_info", "const char * hw_dec_info();\n", decXbm_info},
  {"hw_set_fast_image", "void hw_set_fast_image(BOOL set_FastImages);\n", decXbm_setFastImage},
};

LDGLIB LibLdg[] = { { 0x0001, 3, LibFunc,  "BMP decoder for HighWire", 1 } };

int main(void)
{
  ldg_init(LibLdg);
  return 0;
}
