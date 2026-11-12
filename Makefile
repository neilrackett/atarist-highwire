#
# Makefile for highwire
#
TARGET = highwire.prg

VERSION = $(shell sed -n 's/.*_HIGHWIRE_VERSION_[[:space:]]*"\([^"]*\)".*/\1/p' src/version.h)

DISTDIR_68K = highwire.68k
DISTDIR_020 = highwire.020
DISTDIR_V4E = highwire.v4e

BUILDDIR = build

RELEASEDIR = releases

# compiler settings

CROSSPATH=/opt/cross-mint/bin/
CROSS = $(CROSSPATH)m68k-atari-mint-
CC = $(CROSS)gcc -g #-DDEBUG
AS = $(CC) -c
LD = $(CC) 

CPU = 68000
#CPU = 68020-60
#CPU = 5475

DEFS = -DUSE_INET
OPTFLAGS = -funsigned-char \
       -fomit-frame-pointer -O2 -fstrength-reduce

ifeq ($(CPU),5475)
	OPTS = $(CPU:%=-mcpu=%) $(OPTFLAGS)
else
	OPTS = $(CPU:%=-m%) $(OPTFLAGS)
endif

DISABLED_WARNINGS = -Wno-deprecated-declarations
WARN = \
	-Wall \
	-Wmissing-prototypes \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-qual \
	$(DISABLED_WARNINGS) \
	-Werror \

CFLAGS = $(WARN) $(OPTS) $(DEFS)
ASFLAGS = $(OPTS)
LDFLAGS = -s
LIBS = -lgem -lcflib -liio -lldg

ifeq ($(CPU),5475)
  OBJDIR = $(BUILDDIR)/obj.$(CPU)
else
	OBJDIR = $(BUILDDIR)/obj$(CPU:68%=.%)
endif

# Dependency files record which OBJDIR they belong to, so they have to live
# per CPU target too.  Shared, they would silently stop applying as soon as
# you built for a second CPU, and a header change would then be missed.
DEPDIR = $(OBJDIR)/.deps

# The sources live in src/, so make looks for them there.  The file lists below
# stay as bare names, which is also what the Pure C project files in src/ want.
VPATH = src

#
# C source files
#
# Makefile is a prerequisite so that changing the flags, the CPU mapping or the
# dependency layout rebuilds rather than quietly reusing stale objects.
$(OBJDIR)/%.o: %.c Makefile
	@echo "$(CC) $(CFLAGS) -c $< -o $@"; \
	$(CC) -Wp,-MMD,$(DEPDIR)/$*.P_ $(CFLAGS) -c $< -o $@
	@sed "1s,^[^:]*:,$@:," \
	     $(DEPDIR)/$*.P_ > $(DEPDIR)/$*.P
	@rm -f $(DEPDIR)/$*.P_

#
# files
#
CFILES = \
	Logging.c \
	schedule.c \
	mime.c \
	mbedTLS.c \
	inet.c \
	http.c \
	cache.c \
	Location.c \
	cookie.c \
	DomBox.c \
	O_Struct.c \
	fontbase.c \
	W_Struct.c \
	raster.c \
	image.c \
	img-dcdr.c \
	Paragrph.c \
	list.c \
	Form.c \
	Table.c \
	Frame.c \
	color.c \
	encoding.c \
	scanner.c \
	parser.c \
	p_about.c \
	p_dir.c \
	render.c \
	Containr.c \
	Loader.c \
	Redraws.c \
	clipbrd.c \
	Window.c \
	formwind.c \
	fntsetup.c \
	dl_mngr.c \
	Widget.c \
	hwWind.c \
	av_prot.c \
	dragdrop.c \
	olga.c \
	config.c \
	bookmark.c \
	Variable.c \
	Nice_VDI.c \
	romvdi.c \
	keyinput.c \
	Mouse_R.c \
	AEI.c \
	HighWire.c \
	strtools.c \
#	mem-diag.c

HDR = hwWind.h Loader.h Containr.h Table.h Location.h Logging.h Form.h

SFILES = 

OBJS = $(SFILES:%.s=$(OBJDIR)/%.o) $(CFILES:%.c=$(OBJDIR)/%.o)
OBJS_MAGIC := $(shell mkdir -p ./$(OBJDIR) > /dev/null 2>&1 || :)

DEPENDENCIES = $(addprefix ./$(DEPDIR)/, $(patsubst %.c,%.P,$(CFILES)))


$(BUILDDIR)/$(TARGET) $(DISTDIR_68K)/$(TARGET): $(OBJS)
	mkdir -p $(@D)
	$(LD) -o $@ -Wl,-stack,128k -Wl,--mprg-flags=0x17 $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

000: ; $(MAKE) CPU=68000
020: ; $(MAKE) CPU=68020-60
v4e: ; $(MAKE) CPU=5475

clean:
	rm -Rf *.bak */*.bak */*/*.bak *[%~] */*[%~] */*/*[%~]
	rm -Rf $(BUILDDIR) *.o */*/*.o
	rm -Rf *.app *.[gt]tp *.prg
	rm -Rf $(DISTDIR_68K)
	rm -Rf $(DISTDIR_020)
	rm -Rf $(DISTDIR_V4E)
	rm -Rf modules/decoder.src/*,ldg
	rm -Rf releases

#
# distribution/snapshot archive
#
all::
	$(MAKE) clean
	
	mkdir -p $(DISTDIR_68K)
	mkdir -p $(DISTDIR_020)
	mkdir -p $(DISTDIR_V4E)
	
	$(MAKE) CPU=68000 $(BUILDDIR)/$(TARGET)
	mv $(BUILDDIR)/$(TARGET) $(DISTDIR_68K)/highwire.prg
	$(MAKE) CPU=68020-60 $(BUILDDIR)/$(TARGET)
	mv $(BUILDDIR)/$(TARGET) $(DISTDIR_020)/highwire.prg
	$(MAKE) CPU=5475 $(BUILDDIR)/$(TARGET)
	mv $(BUILDDIR)/$(TARGET) $(DISTDIR_V4E)/highwire.prg

	cp -a rsc/deskicon.rsc rsc/highwire.rsc $(DISTDIR_68K)
	cp -a rsc/deskicon.rsc rsc/highwire.rsc $(DISTDIR_020)
	cp -a rsc/deskicon.rsc rsc/highwire.rsc $(DISTDIR_V4E)

	mkdir -p $(DISTDIR_68K)/doc
	cp -a change.log $(DISTDIR_68K)/doc
	cp -a docs/highwire.txt $(DISTDIR_68K)/doc
	cp -a docs/hotkeys.txt $(DISTDIR_68K)/doc
	mkdir -p $(DISTDIR_68K)/doc/archives
	cp -a docs/compile.old $(DISTDIR_68K)/doc/archives/compile.txt
	cp -a docs/history1.txt $(DISTDIR_68K)/doc/archives
	cp -a docs/history2.txt $(DISTDIR_68K)/doc/archives
	mkdir -p $(DISTDIR_68K)/doc/licenses
	cp -a lib/giflib/COPYING $(DISTDIR_68K)/doc/licenses/giflib.txt
	cp -a lib/libpng/LICENSE $(DISTDIR_68K)/doc/licenses/libpng.txt
	cp -a lib/jpeg/README $(DISTDIR_68K)/doc/licenses/libjpeg.txt
	cp -a lib/libnsbmp/COPYING $(DISTDIR_68K)/doc/licenses/libnsbmp.txt
	cp -a LICENSE $(DISTDIR_68K)/doc/licenses/highwire.txt
	
	mkdir -p $(DISTDIR_68K)/html
	cp -pvr html/. $(DISTDIR_68K)/html
	
	mkdir -p $(DISTDIR_68K)/example.cfg
	cp -a example.cfg/highwire.cfg $(DISTDIR_68K)/example.cfg

	cp -a $(DISTDIR_68K)/doc $(DISTDIR_68K)/html $(DISTDIR_68K)/example.cfg $(DISTDIR_020)
	cp -a $(DISTDIR_68K)/doc $(DISTDIR_68K)/html $(DISTDIR_68K)/example.cfg $(DISTDIR_V4E)

	$(MAKE) -C modules/network.src clean

	mkdir -p $(DISTDIR_68K)/modules
	cp -a modules/readme.txt $(DISTDIR_68K)/modules/readme.txt
	
	$(MAKE) -C modules/network.src CPU=68000 LDG_TYPE=USE_MINTNET
	mv modules/network.src/network.ldg $(DISTDIR_68K)/modules/mintnet.ldg
	$(MAKE) -C modules/network.src CPU=68000 sting
	mv modules/network.src/network.ldg $(DISTDIR_68K)/modules/sting.ldg
	$(MAKE) -C modules/network.src CPU=68000 LDG_TYPE=USE_MAGICNET
	mv modules/network.src/network.ldg $(DISTDIR_68K)/modules/magxnet.ldg
	# ICONNECT module must be made with PureC
	cp modules/network.src/iconnect.ldg $(DISTDIR_68K)/modules/iconnect.ldg

	mkdir -p $(DISTDIR_020)/modules
	cp -a modules/readme.txt $(DISTDIR_020)/modules/readme.txt

	$(MAKE) -C modules/network.src CPU=68020-60 LDG_TYPE=USE_MINTNET
	mv modules/network.src/network.ldg $(DISTDIR_020)/modules/mintnet.ldg
	$(MAKE) -C modules/network.src CPU=68020-60 sting
	mv modules/network.src/network.ldg $(DISTDIR_020)/modules/sting.ldg
	$(MAKE) -C modules/network.src CPU=68020-60 LDG_TYPE=USE_MAGICNET
	mv modules/network.src/network.ldg $(DISTDIR_020)/modules/magxnet.ldg
	# ICONNECT module must be made with PureC
	cp modules/network.src/iconnect.ldg $(DISTDIR_020)/modules/iconnect.ldg

	mkdir -p $(DISTDIR_V4E)/modules
	cp -a modules/readme.txt $(DISTDIR_V4E)/modules/readme.txt

	$(MAKE) -C modules/network.src CPU=5475 LDG_TYPE=USE_MINTNET
	mv modules/network.src/network.ldg $(DISTDIR_V4E)/modules/mintnet.ldg
	$(MAKE) -C modules/network.src CPU=5475 sting
	mv modules/network.src/network.ldg $(DISTDIR_V4E)/modules/sting.ldg

	$(MAKE) -C modules/decoder.src CPU=68000 img_gif
	mv modules/decoder.src/imgd-gif.ldg $(DISTDIR_68K)/modules/imgd-gif.ldg
	$(MAKE) -C modules/decoder.src CPU=68000 img_ico
	mv modules/decoder.src/imgd-ico.ldg $(DISTDIR_68K)/modules/imgd-ico.ldg
	$(MAKE) -C modules/decoder.src CPU=68000 img_jpg
	mv modules/decoder.src/imgd-jpg.ldg $(DISTDIR_68K)/modules/imgd-jpg.ldg
	$(MAKE) -C modules/decoder.src CPU=68000 img_png
	mv modules/decoder.src/imgd-png.ldg $(DISTDIR_68K)/modules/imgd-png.ldg
	$(MAKE) -C modules/decoder.src CPU=68000 img_xbm
	mv modules/decoder.src/imgd-xbm.ldg $(DISTDIR_68K)/modules/imgd-xbm.ldg
	$(MAKE) -C modules/decoder.src CPU=68000 img_bmp
	mv modules/decoder.src/imgd-bmp.ldg $(DISTDIR_68K)/modules/imgd-bmp.ldg

	$(MAKE) -C modules/decoder.src CPU=68020-60 img_gif
	mv modules/decoder.src/imgd-gif.ldg $(DISTDIR_020)/modules/imgd-gif.ldg
	$(MAKE) -C modules/decoder.src CPU=68020-60 img_ico
	mv modules/decoder.src/imgd-ico.ldg $(DISTDIR_020)/modules/imgd-ico.ldg
	$(MAKE) -C modules/decoder.src CPU=68020-60 img_jpg
	mv modules/decoder.src/imgd-jpg.ldg $(DISTDIR_020)/modules/imgd-jpg.ldg
	$(MAKE) -C modules/decoder.src CPU=68020-60 img_png
	mv modules/decoder.src/imgd-png.ldg $(DISTDIR_020)/modules/imgd-png.ldg
	$(MAKE) -C modules/decoder.src CPU=68020-60 img_xbm
	mv modules/decoder.src/imgd-xbm.ldg $(DISTDIR_020)/modules/imgd-xbm.ldg
	$(MAKE) -C modules/decoder.src CPU=68020-60 img_bmp
	mv modules/decoder.src/imgd-bmp.ldg $(DISTDIR_020)/modules/imgd-bmp.ldg

	$(MAKE) -C modules/decoder.src CPU=5475 img_gif
	mv modules/decoder.src/imgd-gif.ldg $(DISTDIR_V4E)/modules/imgd-gif.ldg
	$(MAKE) -C modules/decoder.src CPU=5475 img_ico
	mv modules/decoder.src/imgd-ico.ldg $(DISTDIR_V4E)/modules/imgd-ico.ldg
	$(MAKE) -C modules/decoder.src CPU=5475 img_jpg
	mv modules/decoder.src/imgd-jpg.ldg $(DISTDIR_V4E)/modules/imgd-jpg.ldg
	$(MAKE) -C modules/decoder.src CPU=5475 img_png
	mv modules/decoder.src/imgd-png.ldg $(DISTDIR_V4E)/modules/imgd-png.ldg
	$(MAKE) -C modules/decoder.src CPU=5475 img_xbm
	mv modules/decoder.src/imgd-xbm.ldg $(DISTDIR_V4E)/modules/imgd-xbm.ldg
	$(MAKE) -C modules/decoder.src CPU=5475 img_bmp
	mv modules/decoder.src/imgd-bmp.ldg $(DISTDIR_V4E)/modules/imgd-bmp.ldg

	mkdir -p $(RELEASEDIR)
	(cwd=`pwd`; cd $(DISTDIR_68K); zip -r "$$cwd"/$(RELEASEDIR)/highwire-`date +%Y%m%d`-68k.zip . -x "*/.DS_Store" -x "__MACOSX")
	(cwd=`pwd`; cd $(DISTDIR_020); zip -r "$$cwd"/$(RELEASEDIR)/highwire-`date +%Y%m%d`-020.zip . -x "*/.DS_Store" -x "__MACOSX")
	(cwd=`pwd`; cd $(DISTDIR_V4E); zip -r "$$cwd"/$(RELEASEDIR)/highwire-`date +%Y%m%d`-v4e.zip . -x "*/.DS_Store" -x "__MACOSX")

#
# dependencies
#
DEPS_MAGIC := $(shell mkdir -p ./$(DEPDIR) > /dev/null 2>&1 || :)

-include $(DEPENDENCIES)
