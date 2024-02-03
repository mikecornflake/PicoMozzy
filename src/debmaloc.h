
#include <stdio.h>
#include <stdlib.h>

#ifndef _debmaloc_h
#define _debmaloc_h

#ifdef DEBUG_MALLOC
#define DEBMALLOC(size)         deb_malloc((size),__FILE__,__LINE__)
#define DEBDEREF(what,where)    (what)[(where) + deb_deref((what),(where),__FILE__,__LINE__)]
#define DEBFREE(what)           deb_free((void *) (what),__FILE__,__LINE__)
#define DEBASSERT(cond)         if ( !(cond) ) { report_error(__FILE__,__LINE__); }
#endif

#ifndef DEBUG_MALLOC
#define DEBMALLOC(size)         malloc(size)
#define DEBDEREF(what,where)    (what)[where]
#define DEBFREE(what)           free(what)
#define DEBASSERT(cond)
#endif

void *deb_malloc(int size, const char *mess, int line);
int deb_deref(void *what, int where, const char *mess, int line);
void deb_free(void *what, const char *mess, int line);
void report_error(const char *mess, int line);

#endif
