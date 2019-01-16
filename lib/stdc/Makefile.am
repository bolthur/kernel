
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include

noinst_LIBRARIES = libstdc.a
libstdc_a_SOURCES = \
  math/pow.c \
  math/powf.c \
  math/powl.c \
  stdio/fclose.c \
  stdio/fflush.c \
  stdio/fopen.c \
  stdio/fprintf.c \
  stdio/fread.c \
  stdio/fseek.c \
  stdio/ftell.c \
  stdio/fwrite.c \
  stdio/printf.c \
  stdio/putchar.c \
  stdio/puts.c \
  stdio/setbuf.c \
  stdio/snprintf.c \
  stdio/sprintf.c \
  stdio/vfprintf.c \
  stdio/vprintf.c \
  stdio/vsnprintf.c \
  stdio/vsprintf.c \
  stdlib/abort.c \
  stdlib/abs.c \
  stdlib/atexit.c \
  stdlib/atof.c \
  stdlib/atoi.c \
  stdlib/atol.c \
  stdlib/bsearch.c \
  stdlib/calloc.c \
  stdlib/div.c \
  stdlib/exit.c \
  stdlib/free.c \
  stdlib/ftoa.c \
  stdlib/getenv.c \
  stdlib/imaxdiv.c \
  stdlib/itoa.c \
  stdlib/labs.c \
  stdlib/ldiv.c \
  stdlib/malloc.c \
  stdlib/mblen.c \
  stdlib/mbstowcs.c \
  stdlib/mbtowc.c \
  stdlib/qsort.c \
  stdlib/rand.c \
  stdlib/realloc.c \
  stdlib/srand.c \
  stdlib/strtod.c \
  stdlib/strtol.c \
  stdlib/strtoul.c \
  stdlib/system.c \
  stdlib/utoa.c \
  stdlib/wcstombs.c \
  stdlib/wctomb.c \
  string/memchr.c \
  string/memcmp.c \
  string/memcpy.c \
  string/memmove.c \
  string/memset.c \
  string/strcat.c \
  string/strchr.c \
  string/strcmp.c \
  string/strcoll.c \
  string/strcpy.c \
  string/strcspn.c \
  string/strerror.c \
  string/strlen.c \
  string/strncat.c \
  string/strncmp.c \
  string/strncpy.c \
  string/strnlen.c \
  string/strpbrk.c \
  string/strrchr.c \
  string/strrev.c \
  string/strspn.c \
  string/strstr.c \
  string/strtok.c \
  string/strxfrm.c \
  unistd/execv.c \
  unistd/execve.c \
  unistd/execvp.c \
  unistd/fork.c
