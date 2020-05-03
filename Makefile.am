
ACLOCAL_AMFLAGS = -I build-aux/m4

.PHONY: cppcheck
cppcheck:
	cppcheck -I $(top_srcdir)/include -I $(top_srcdir)/include/lib --quiet --inline-suppr -UCMC --force --enable=all --suppress-xml=$(top_srcdir)/build-aux/platform/rpi/cppcheck.suppression.xml --platform=$(top_srcdir)/build-aux/platform/rpi/cppcheck.32.xml --include=$(top_srcdir)/build/include/config.h --project=$(top_srcdir)/build-aux/platform/rpi/project.cppcheck $(top_srcdir)

SUBDIRS = thirdparty src
