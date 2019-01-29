
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include

noinst_LIBRARIES = libstdc.a
libstdc_a_SOURCES = \
  ctype/isalpha.c \
  ctype/isdigit.c \
  ctype/islower.c \
  ctype/isspace.c \
  ctype/isupper.c \
  stdio/printf.c \
  stdio/putchar.c \
  stdio/puts.c \
  stdlib/itoa.c \
  stdlib/rand.c \
  stdlib/srand.c \
  stdlib/utoa.c \
  string/memchr.c \
  string/memcmp.c \
  string/memcpy.c \
  string/memmove.c \
  string/memset.c \
  string/strcmp.c \
  string/strlen.c \
  string/strncpy.c \
  string/strnlen.c \
  string/strrev.c
