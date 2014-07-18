#ifndef SHM_CONFIG_H
#define SHM_CONFIG_H

#ifndef SHM_SIZE_IN_PAGES
#define SHM_SIZE_IN_PAGES 1
#endif /* SHM_SIZE_IN_PAGES */

#ifndef SHM_KEY
#define SHM_KEY 0xdeadbeef
#endif /* SHM_KEY */

#ifndef SHM_KEY_RETRY
#define SHM_KEY_RETRY 10
#endif /* SHM_KEY_RETRY */

#if defined(PAGESIZE)
#define SHM_PAGESIZE PAGESIZE
#elif defined(PAGE_SIZE)
#define SHM_PAGESIZE PAGE_SIZE
#elif defined(_SC_PAGESIZE)
#define SHM_PAGESIZE _SC_PAGESIZE
#endif

#if defined(SHM_PAGESIZE)
#define shm_pagesize __shm_pagesize()
static long __shm_pagesize() {
  static long s_page_size = 0;
  return s_page_size > 0 
      ? s_page_size 
      : (s_page_size = sysconf(SHM_PAGESIZE), s_page_size);
}
#endif

#ifndef bool
#define bool int
#define true 1
#define false 0
#endif

#ifdef UNITTEST
#define shm_internal
#else
#define shm_internal static
#endif

/*
 * see http://dbp-consulting.com/tutorials/SuppressingGCCWarnings.html 
 * for details about turning gcc warnings on/off
 *
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 402
#define GCC_DIAG_STR(s) #s
#define GCC_DIAG_JOINSTR(x,y) GCC_DIAG_STR(x ## y)
# define GCC_DIAG_DO_PRAGMA(x) _Pragma (#x)
# define GCC_DIAG_PRAGMA(x) GCC_DIAG_DO_PRAGMA(GCC diagnostic x)
# if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(push) \
      GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W,x))
#  define GCC_DIAG_ON(x) GCC_DIAG_PRAGMA(pop)
# else
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W,x))
#  define GCC_DIAG_ON(x)  GCC_DIAG_PRAGMA(warning GCC_DIAG_JOINSTR(-W,x))
# endif
#else
# define GCC_DIAG_OFF(x)
# define GCC_DIAG_ON(x)
#endif
*/

#endif /* SHM_CONFIG_H */

