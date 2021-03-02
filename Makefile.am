
ACLOCAL_AMFLAGS = -I build-aux/m4

SUBDIRS = thirdparty src driver

# cppcheck stuff
.PHONY: cppcheck
cppcheck:
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_INCLUDE) \
		--project=$(CPPCHECK_PROJECT) \
		-U__CYGWIN__ -U__APPLE__ -D__INTPTR_TYPE__=4 -v -j$(CPU_COUNT)
# FIXME: MOVE __INTPTR_TYPE__ DEFINE INTO MACRO PER HOST

# doxygen related
@DX_RULES@
MOSTLYCLEANFILES = $(DX_CLEANFILES)
