// File generated by cmake from cmake/config.h.cmake.

#ifndef _EXV_CONF_H_
#define _EXV_CONF_H_

// Define to 1 if you want to use libcurl in httpIO.
/* #undef EXV_USE_CURL */

// Define if you require webready support.
/* #undef EXV_ENABLE_WEBREADY */

// Define if you want translation of program messages to the user's native language
/* #undef EXV_ENABLE_NLS */

// Define if you want to enable the decoding of video metadata
/* #undef EXV_ENABLE_VIDEO */

// Define if you want BMFF support.
/* #undef EXV_ENABLE_BMFF */

// Define if you want to use the inih library.
/* #undef EXV_ENABLE_INIH */

// Define if you have the strerror_r function.
/* #undef EXV_HAVE_STRERROR_R */

// Define if the strerror_r function returns char*.
/* #undef EXV_STRERROR_R_CHAR_P */

/* Define to `const' or to empty, depending on the second argument of `iconv'. */
/* #undef ICONV_ACCEPTS_CONST_INPUT */
#if defined(ICONV_ACCEPTS_CONST_INPUT) || defined(__NetBSD__)
#define EXV_ICONV_CONST const
#else
#define EXV_ICONV_CONST
#endif

// Define if you have the zlib library.
#define EXV_HAVE_LIBZ

// Define if you have the brotli library.
/* #undef EXV_HAVE_BROTLI */

/* Define if you have (Exiv2/xmpsdk) Adobe XMP Toolkit. */
#define EXV_HAVE_XMP_TOOLKIT

/* Define to the full name of this package. */
#define EXV_PACKAGE_NAME "exiv2"

/* Define to the full name and version of this package. */
#define EXV_PACKAGE_STRING "exiv2 1.0.0.9"

/* Define to the version of this package. */
#define EXV_PACKAGE_VERSION "1.0.0.9"

#define EXIV2_MAJOR_VERSION (1U)
#define EXIV2_MINOR_VERSION (0U)
#define EXIV2_PATCH_VERSION (0U)
#define EXIV2_TWEAK_VERSION (9U)

// Definition to enable translation of Nikon lens names.
#define EXV_HAVE_LENSDATA

// Define if you have the iconv function.
/* #undef EXV_HAVE_ICONV */

#endif /* !_EXV_CONF_H_ */