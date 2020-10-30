
ACLOCAL_AMFLAGS = -I build-aux/m4

.PHONY: cppcheck
cppcheck:
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force --platform=$(shell $(BOLTHUR_READLINK) -f $(top_srcdir)/build-aux/platform/$(platform_subdir)/cppcheck.32.xml) --include=$(shell $(BOLTHUR_READLINK) -f $(top_srcdir)/build/include/config.h) --project=$(shell $(BOLTHUR_READLINK) -f $(top_srcdir)/build-aux/platform/$(platform_subdir)/project.$(executable_format).cppcheck)

SUBDIRS = thirdparty src
