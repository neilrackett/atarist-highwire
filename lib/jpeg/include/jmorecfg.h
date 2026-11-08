/*
 * jmorecfg.h
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * Modified 1997-2011 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains additional configuration options that customize the
 * JPEG software for special applications or support machine-dependent
 * optimizations.  Most users will not need to touch this file.
 */

#ifndef _JMORECFG_H
#define _JMORECFG_H

#ifdef __PUREC__
#define INCOMPLETE_TYPES_BROKEN
#undef INLINE
#endif


#if (defined(__MSHORT__) || defined(__PUREC__) || defined(__AHCC__)) && defined(JPEG_SLB)
typedef unsigned long juint_t;
typedef long jint_t;
typedef long boolean;
#define HAVE_BOOLEAN
#define JENUMINT(t) jint_t
#else
typedef unsigned int juint_t;
typedef int jint_t;
#define JENUMINT(t) t
#endif

#if !defined(__gcc_double_t_defined)
#if defined(JPEG_SLB) && !defined(__GNUC__)
struct __gcc_double {
	unsigned long hi, lo;
};
#define __gcc_double_t struct __gcc_double
#else
#define __gcc_double_t double
#endif
#define __gcc_double_t_defined
#endif
#if !defined(__gcc_float_t_defined)
#if defined(JPEG_SLB) && !defined(__GNUC__)
#define __gcc_float_t unsigned long
#else
#define __gcc_float_t float
#endif
#define __gcc_float_t_defined
#endif

/*
 * Define BITS_IN_JSAMPLE as either
 *   8   for 8-bit sample values (the usual setting)
 *   9   for 9-bit sample values
 *   10  for 10-bit sample values
 *   11  for 11-bit sample values
 *   12  for 12-bit sample values
 * Only 8, 9, 10, 11, and 12 bits sample data precision are supported for
 * full-feature DCT processing.  Further depths up to 16-bit may be added
 * later for the lossless modes of operation.
 * Run-time selection and conversion of data precision will be added later
 * and are currently not supported, sorry.
 * Exception:  The transcoding part (jpegtran) supports all settings in a
 * single instance, since it operates on the level of DCT coefficients and
 * not sample values.  The DCT coefficients are of the same type (16 bits)
 * in all cases (see below).
 */

#define BITS_IN_JSAMPLE  8	/* use 8, 9, 10, 11, or 12 */


/*
 * Maximum number of components (color channels) allowed in JPEG image.
 * To meet the letter of the JPEG spec, set this to 255.  However, darn
 * few applications need more than 4 channels (maybe 5 for CMYK + alpha
 * mask).  We recommend 10 as a reasonable compromise; use 4 if you are
 * really short on memory.  (Each allowed component costs a hundred or so
 * bytes of storage, whether actually used in an image or not.)
 */

#define MAX_COMPONENTS  10	/* maximum number of image components */


/*
 * Basic data types.
 * You may need to change these if you have a machine with unusual data
 * type sizes; for example, "char" not 8 bits, "short" not 16 bits,
 * or "long" not 32 bits.  We don't care whether "int" is 16 or 32 bits,
 * but it had better be at least 16.
 */

/* Representation of a single sample (pixel element value).
 * We frequently allocate large arrays of these, so it's important to keep
 * them small.  But if you have memory to burn and access to char or short
 * arrays is very slow on your hardware, you might want to change these.
 */

#if BITS_IN_JSAMPLE == 8
/* JSAMPLE should be the smallest type that will hold the values 0..255.
 * You can use a signed char by having GETJSAMPLE mask it with 0xFF.
 */

typedef unsigned char JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE	255
#define CENTERJSAMPLE	128

#endif /* BITS_IN_JSAMPLE == 8 */


#if BITS_IN_JSAMPLE == 9
/* JSAMPLE should be the smallest type that will hold the values 0..511.
 * On nearly all machines "short" will do nicely.
 */

typedef short JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE	511
#define CENTERJSAMPLE	256

#endif /* BITS_IN_JSAMPLE == 9 */


#if BITS_IN_JSAMPLE == 10
/* JSAMPLE should be the smallest type that will hold the values 0..1023.
 * On nearly all machines "short" will do nicely.
 */

typedef short JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE	1023
#define CENTERJSAMPLE	512

#endif /* BITS_IN_JSAMPLE == 10 */


#if BITS_IN_JSAMPLE == 11
/* JSAMPLE should be the smallest type that will hold the values 0..2047.
 * On nearly all machines "short" will do nicely.
 */

typedef short JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE	2047
#define CENTERJSAMPLE	1024

#endif /* BITS_IN_JSAMPLE == 11 */


#if BITS_IN_JSAMPLE == 12
/* JSAMPLE should be the smallest type that will hold the values 0..4095.
 * On nearly all machines "short" will do nicely.
 */

typedef short JSAMPLE;
#define GETJSAMPLE(value)  ((int) (value))

#define MAXJSAMPLE	4095
#define CENTERJSAMPLE	2048

#endif /* BITS_IN_JSAMPLE == 12 */


/* Representation of a DCT frequency coefficient.
 * This should be a signed value of at least 16 bits; "short" is usually OK.
 * Again, we allocate large arrays of these, but you can change to int
 * if you have memory to burn and "short" is really slow.
 */

typedef short JCOEF;


/* Compressed datastreams are represented as arrays of JOCTET.
 * These must be EXACTLY 8 bits wide, at least once they are written to
 * external storage.  Note that when using the stdio data source/destination
 * managers, this is also the data type passed to fread/fwrite.
 */

typedef unsigned char JOCTET;
#define GETJOCTET(value)  (value)

/* These typedefs are used for various table entries and so forth.
 * They must be at least as wide as specified; but making them too big
 * won't cost a huge amount of memory, so we don't provide special
 * extraction code like we did for JSAMPLE.  (In other words, these
 * typedefs live at a different point on the speed/space tradeoff curve.)
 */

/* UINT8 must hold at least the values 0..255. */
typedef unsigned char UINT8;

/* UINT16 must hold at least the values 0..65535. */
typedef unsigned short UINT16;

/* INT16 must hold at least the values -32768..32767. */

#ifndef XMD_H			/* X11/xmd.h correctly defines INT16 */
#undef INT16
typedef short INT16;
#endif

/* INT32 must hold at least signed 32-bit values. */

#ifndef XMD_H			/* X11/xmd.h correctly defines INT32 */
#ifndef _BASETSD_H_		/* Microsoft defines it in basetsd.h */
#ifndef _BASETSD_H		/* MinGW is slightly different */
#ifndef QGLOBAL_H		/* Qt defines it in qglobal.h */
#if defined(__MSHORT__) || defined(__PUREC__) || defined(__AHCC__)
typedef long INT32;
#else
typedef int INT32;
#endif
#endif
#endif
#endif
#endif

/* Datatype used for image dimensions.  The JPEG standard only supports
 * images up to 64K*64K due to 16-bit fields in SOF markers.  Therefore
 * "unsigned int" is sufficient on all machines.  However, if you need to
 * handle larger images and you don't mind deviating from the spec, you
 * can change this datatype.
 */

typedef juint_t JDIMENSION;

#define JPEG_MAX_DIMENSION  65500L  /* a tad under 64K to prevent overflows */


/* These macros are used in all function definitions and extern declarations.
 * You could modify them if you need to change function linkage conventions;
 * in particular, you'll need to do that to make the library a Windows DLL.
 * Another application is to make all functions global for use with debuggers
 * or code profilers that require it.
 */

#if defined(JPEG_SLB)
/* a function called through method pointers: */
#define METHODDEF(type)		static type __CDECL
/* a function used only in its module: */
#define LOCAL(type)		static type
/* a function referenced thru EXTERNs: */
#define GLOBAL(type)		type __CDECL
/* a reference to a GLOBAL function: */
#define EXTERN(type)		extern type __CDECL

/* This macro is used to declare a "method", that is, a function pointer.
 * We want to supply prototype parameters if the compiler can cope.
 * Note that the arglist parameter must be parenthesized!
 * Again, you can customize this if you need special linkage keywords.
 */

#define JMETHOD(type,methodname,arglist)  type __CDECL (*methodname) arglist

#else
/* a function called through method pointers: */
#define METHODDEF(type)		static type
/* a function used only in its module: */
#define LOCAL(type)		static type
/* a function referenced thru EXTERNs: */
#define GLOBAL(type)		type
/* a reference to a GLOBAL function: */
#define EXTERN(type)		extern type

/* This macro is used to declare a "method", that is, a function pointer.
 * We want to supply prototype parameters if the compiler can cope.
 * Note that the arglist parameter must be parenthesized!
 * Again, you can customize this if you need special linkage keywords.
 */

#define JMETHOD(type,methodname,arglist)  type (*methodname) arglist

#endif



/* Here is the pseudo-keyword for declaring pointers that must be "far"
 * on 80x86 machines.  Most of the specialized coding for 80x86 is handled
 * by just saying "FAR *" where such a pointer is needed.  In a few places
 * explicit coding is needed; see uses of the NEED_FAR_POINTERS symbol.
 */

#ifndef FAR
#ifdef NEED_FAR_POINTERS
#define FAR  far
#else
#define FAR
#endif
#endif


/*
 * On a few systems, type boolean and/or its values FALSE, TRUE may appear
 * in standard header files.  Or you may have conflicts with application-
 * specific header files that you want to include together with these files.
 * Defining HAVE_BOOLEAN before including jpeglib.h should make it work.
 */

#ifndef HAVE_BOOLEAN
typedef int boolean;
#endif
#ifndef FALSE			/* in case these macros already exist */
#define FALSE	0		/* values of boolean */
#endif
#ifndef TRUE
#define TRUE	1
#endif


#ifndef JDOUBLE_T
#define JDOUBLE_T __gcc_double_t
#endif

/*
 * The remaining options affect code selection within the JPEG library,
 * but they don't need to be visible to most applications using the library.
 * To minimize application namespace pollution, the symbols won't be
 * defined unless JPEG_INTERNALS or JPEG_INTERNAL_OPTIONS has been defined.
 */

#ifdef JPEG_INTERNALS
#define JPEG_INTERNAL_OPTIONS
#endif

#ifdef JPEG_INTERNAL_OPTIONS


/*
 * These defines indicate whether to include various optional functions.
 * Undefining some of these symbols will produce a smaller but less capable
 * library.  Note that you can leave certain source files out of the
 * compilation/linking process if you've #undef'd the corresponding symbols.
 * (You may HAVE to do that if your compiler doesn't like null source files.)
 */

/* Capability options common to encoder and decoder: */

#define DCT_ISLOW_SUPPORTED	/* slow but accurate integer algorithm */
#define DCT_IFAST_SUPPORTED	/* faster, less accurate integer method */
#define DCT_FLOAT_SUPPORTED	/* floating-point: accurate, fast on fast HW */

/* Encoder capability options: */

#if JPEG_LIB_VERSION >= 70
#define C_ARITH_CODING_SUPPORTED    /* Arithmetic coding back end? */
#else
#undef  C_ARITH_CODING_SUPPORTED    /* Arithmetic coding back end? */
#endif
#define C_MULTISCAN_FILES_SUPPORTED /* Multiple-scan JPEG files? */
#define C_PROGRESSIVE_SUPPORTED	    /* Progressive JPEG? (Requires MULTISCAN)*/
#if JPEG_LIB_VERSION >= 70
#define DCT_SCALING_SUPPORTED	    /* Input rescaling via DCT? (Requires DCT_ISLOW)*/
#endif
#define ENTROPY_OPT_SUPPORTED	    /* Optimization of entropy coding parms? */
/* Note: if you selected more than 8-bit data precision, it is dangerous to
 * turn off ENTROPY_OPT_SUPPORTED.  The standard Huffman tables are only
 * good for 8-bit precision, so arithmetic coding is recommended for higher
 * precision.  The Huffman encoder normally uses entropy optimization to
 * compute usable tables for higher precision.  Otherwise, you'll have to
 * supply different default Huffman tables.
 * The exact same statements apply for progressive JPEG: the default tables
 * don't work for progressive mode.  (This may get fixed, however.)
 */
#define INPUT_SMOOTHING_SUPPORTED   /* Input image smoothing option? */

/* Decoder capability options: */

#if JPEG_LIB_VERSION >= 70
#define D_ARITH_CODING_SUPPORTED    /* Arithmetic coding back end? */
#else
#undef  D_ARITH_CODING_SUPPORTED    /* Arithmetic coding back end? */
#endif
#define D_MULTISCAN_FILES_SUPPORTED /* Multiple-scan JPEG files? */
#define D_PROGRESSIVE_SUPPORTED	    /* Progressive JPEG? (Requires MULTISCAN)*/
#define IDCT_SCALING_SUPPORTED	    /* Output rescaling via IDCT? (Requires DCT_ISLOW)*/
#define SAVE_MARKERS_SUPPORTED	    /* jpeg_save_markers() needed? */
#define BLOCK_SMOOTHING_SUPPORTED   /* Block smoothing? (Progressive only) */
#undef  UPSAMPLE_SCALING_SUPPORTED  /* Output rescaling at upsample stage? */
#define UPSAMPLE_MERGING_SUPPORTED  /* Fast path for sloppy upsampling? */
#define QUANT_1PASS_SUPPORTED	    /* 1-pass color quantization? */
#define QUANT_2PASS_SUPPORTED	    /* 2-pass color quantization? */

/* more capability options later, no doubt */


/*
 * Ordering of RGB data in scanlines passed to or from the application.
 * If your application wants to deal with data in the order B,G,R, just
 * change these macros.  You can also deal with formats such as R,G,B,X
 * (one extra byte per pixel) by changing RGB_PIXELSIZE.  Note that changing
 * the offsets will also change the order in which colormap data is organized.
 * RESTRICTIONS:
 * 1. The sample applications cjpeg,djpeg do NOT support modified RGB formats.
 * 2. The color quantizer modules will not behave desirably if RGB_PIXELSIZE
 *    is not 3 (they don't understand about dummy color components!).  So you
 *    can't use color quantization if you change that value.
 */

#define RGB_RED		0	/* Offset of Red in an RGB scanline element */
#define RGB_GREEN	1	/* Offset of Green */
#define RGB_BLUE	2	/* Offset of Blue */
#define RGB_PIXELSIZE	3	/* JSAMPLEs per RGB scanline element */


/* Definitions for speed-related optimizations. */


/* If your compiler supports inline functions, define INLINE
 * as the inline keyword; otherwise define it as empty.
 */

#ifndef INLINE
#ifdef __GNUC__			/* for instance, GNU C knows about inline */
#define INLINE __inline__
#endif
#ifndef INLINE
#define INLINE			/* default is to define it as empty */
#endif
#endif


/* On some machines (notably 68000 series) "int" is 32 bits, but multiplying
 * two 16-bit shorts is faster than multiplying two ints.  Define MULTIPLIER
 * as short on such a machine.  MULTIPLIER must be at least 16 bits wide.
 */

#ifndef MULTIPLIER
#define MULTIPLIER  int		/* type for fastest integer multiply */
#endif


/* FAST_FLOAT should be either float or double, whichever is done faster
 * by your compiler.  (Note that this type is only used in the floating point
 * DCT routines, so it only matters if you've defined DCT_FLOAT_SUPPORTED.)
 * Typically, float is faster in ANSI C compilers, while double is faster in
 * pre-ANSI compilers (because they insist on converting to double anyway).
 * The code below therefore chooses float if we have ANSI-style prototypes.
 */

#ifndef FAST_FLOAT
#ifdef HAVE_PROTOTYPES
#define FAST_FLOAT  __gcc_float_t
#else
#define FAST_FLOAT  __gcc_double_t
#endif
#endif

#endif /* JPEG_INTERNAL_OPTIONS */

#endif /* _JMORECFG_H */
