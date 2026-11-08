#
# Makefile for highwire
#
BUILDDIR = ../../build/decoder

# compiler settings
CROSSPATH=/opt/cross-mint/bin/
CROSS = $(CROSSPATH)m68k-atari-mint-
CC = $(CROSS)gcc -g #-DDEBUG
AS = $(CC) -c
LD = $(CC) 
CP = cp
RM = rm -f
STRIP = $(CROSS)strip

CPU = 68000
#CPU = 68020-60
#CPU = 5475

DEFS =

OPTFLAGS = -funsigned-char \
       -fomit-frame-pointer -O2 -fstrength-reduce -DICO_DBG -DXBM_DBG

ifeq ($(CPU),5475)
	OPTS = $(CPU:%=-mcpu=%) $(OPTFLAGS)
else
	OPTS = $(CPU:%=-m%) $(OPTFLAGS)
endif

WARN = \
	-Wall \
	-Wmissing-prototypes \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-qual

CFLAGS = $(INCLUDE) $(WARN) $(OPTS) $(DEFS)
LDFLAGS = 
LIBS =

ifeq ($(CPU),5475)
  OBJDIR = $(BUILDDIR)/obj.$(CPU)
else
	OBJDIR = $(BUILDDIR)/obj$(CPU:68%=.%)
endif
DEPDIR = $(BUILDDIR)/.deps

all: $(TARGET)

#
# C source files
#
$(OBJDIR)/%.o: %.c
	@echo "$(CC) $(CFLAGS) -c $< -o $@"; \
	$(CC) -Wp,-MMD,$(DEPDIR)/$(<:.c=.P_) $(CFLAGS) -c $< -o $@
	@cat $(DEPDIR)/$(<:.c=.P_) \
	    | sed "s,^\(.*\)\.o:,$(OBJDIR)/\1.o:," > $(DEPDIR)/$(<:.c=.P)
	@rm -f $(DEPDIR)/$(<:.c=.P_)

#
# files
#
CFILES = img_gif.c img_ico.c img_jpg.c img_png.c img_xmp.c

HDR = 

SFILES = 

OBJS = $(SFILES:%.s=$(OBJDIR)/%.o) $(CFILES:%.c=$(OBJDIR)/%.o)
OBJS_MAGIC := $(shell mkdir -p $(OBJDIR) > /dev/null 2>&1 || :)

DEPENDENCIES = $(addprefix $(DEPDIR)/, $(patsubst %.c,%.P,$(CFILES)))

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(CFLAGS) -Wl,-stack,64k $(LIBS)
	$(STRIP) $(TARGET)
	rm $(OBJDIR)/*.o

000: ; $(MAKE) CPU=68000
020: ; $(MAKE) CPU=68020-60
v4e: ; $(MAKE) CPU=5475

CMINI=../../lib/libcmini/lib
CPUDIR=

ifeq ($(CPU),5475)
	CPUDIR = /m5475
else
	ifeq ($(CPU),68020-60)
		CPUDIR = /m68020-60
	endif
endif

img_gif: ; $(MAKE) TARGET=imgd-gif.ldg CFILES=img_gif.c \
									 INCLUDE="-I../../lib/giflib/include -I../../lib/ldg/include -I../../lib/libcmini/include" \
                   LDFLAGS="-s -nostdlib $(CMINI)/crt0.o" \
                   LIBS="-L$(CMINI)$(CPUDIR) -L../../lib/giflib$(CPUDIR) -L../../lib/ldg/lib.gcc$(CPUDIR) -lgif -lldg -lcmini -lgcc -lcmini"

img_ico: ; $(MAKE) TARGET=imgd-ico.ldg CFILES=img_ico.c \
									 INCLUDE="-I../../lib/ldg/include -I../../lib/libcmini/include" \
                   LDFLAGS="-s -nostdlib $(CMINI)/crt0.o" \
                   LIBS="-L$(CMINI)$(CPUDIR) -L../../lib/ldg/lib.gcc$(CPUDIR) -lm -lldg -lcmini -lgcc -lcmini"

img_jpg: ; $(MAKE) TARGET=imgd-jpg.ldg CFILES=img_jpg.c \
									 INCLUDE=-"I../../lib/jpeg/include -I../../lib/ldg/include -I../../lib/libcmini/include" \
                   LDFLAGS="-s -nostdlib $(CMINI)/crt0.o" \
                   LIBS="-L$(CMINI)$(CPUDIR) -L../../lib/jpeg$(CPUDIR) -L../../lib/ldg/lib.gcc$(CPUDIR) -ljpeg -lm -lldg -lcmini -lgcc -lcmini"

img_png: ; $(MAKE) TARGET=imgd-png.ldg CFILES=img_png.c \
									 INCLUDE="-I../../lib/libpng/include -I../../lib/ldg/include -I../../lib/libcmini/include" \
                   LDFLAGS="-s -nostdlib $(CMINI)/crt0.o" \
                   LIBS="-L$(CMINI)$(CPUDIR) -L../../lib/libpng$(CPUDIR) -L../../lib/ldg/lib.gcc$(CPUDIR) -lpng -lz -lm -lldg -lcmini -lgcc -lcmini"

img_xbm: ; $(MAKE) TARGET=imgd-xbm.ldg CFILES=img_xbm.c \
                   INCLUDE="-I../../lib/ldg/include -I../../lib/libcmini/include" \
                   LDFLAGS="-s -nostdlib $(CMINI)/crt0.o" \
                   LIBS="-L$(CMINI)$(CPUDIR) -L../../lib/ldg/lib.gcc$(CPUDIR) -lm -lldg -lcmini -lgcc -lcmini"

clean:
	$(RM) $(OBJS)

distclean:
	$(MAKE) clean;
	$(RM) *~


#
# adjust file names
#
$(CFILES) $(HDR) Makefile: ; mv `echo $@ | tr A-Z a-z` $@
case: Makefile $(HDR) $(CFILES)


#
# dependencies
#
DEPS_MAGIC := $(shell mkdir -p $(DEPDIR) > /dev/null 2>&1 || :)

-include $(DEPENDENCIES)
