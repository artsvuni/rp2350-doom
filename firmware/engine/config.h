/*
 * Hand-written equivalent of the values CMake's configure_file() would
 * generate from upstream's cmake/config.h.cin for the "doom" target -
 * values taken directly from their top-level CMakeLists.txt (project name
 * "Chocolate Doom", version 3.0.0). Written by hand instead of wiring up
 * configure_file() since these values never change per-build for us.
 */
#define PACKAGE_NAME "Chocolate Doom"
#define PACKAGE_TARNAME "chocolate-doom"
#define PACKAGE_VERSION "3.0.0"
#define PACKAGE_STRING "Chocolate Doom 3.0.0"
#define PROGRAM_PREFIX "chocolate-"

/* No libsamplerate/libpng/dirent/mmap on this target. */
#define HAVE_DECL_STRCASECMP 0
#define HAVE_DECL_STRNCASECMP 0

/*
 * whd_gen (the host-side WAD->WHD preprocessing tool, engine/whd_gen/) also
 * includes this same config.h transitively through shared headers like
 * doomtype.h - both share one file rather than fighting over which
 * same-named config.h a given #include "config.h" resolves to. Values
 * below are whd_gen's own defaults, from upstream's src/whd_gen/config.h.
 */
#define VERIFY_ENCODING 1
#define USE_MUSX 1
#define MUS_GROUP_SIZE_CODE 1
#define TEXTURE_PIXEL_STATS 1
#ifndef NDEBUG
#define LOOKAHEAD 3
#else
#define LOOKAHEAD 5
#endif
#define CONSIDER_SEQUENCES 0
#define SEQUENCE 10

#ifdef IS_WHD_GEN
void __attribute__((noreturn)) fail(const char *msg, ...);
#endif
