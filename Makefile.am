
ACLOCAL_AMFLAGS = -I build-aux/m4

SUBDIRS = thirdparty src

# cppcheck stuff
.PHONY: cppcheck
cppcheck:
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_INCLUDE) \
		--project=$(CPPCHECK_PROJECT)

# doxygen related
@DX_RULES@
MOSTLYCLEANFILES = $(DX_CLEANFILES)
