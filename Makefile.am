
ACLOCAL_AMFLAGS = -I build-aux/m4

SUBDIRS = thirdparty src driver

# cppcheck stuff
.PHONY: cppcheck
cppcheck:
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_INCLUDE) \
		--project=$(CPPCHECK_PROJECT) \
		--suppressions-list=$(CPPCHECK_SUPPRESS) \
		-U__CYGWIN__ -U__APPLE__ -D__INTPTR_TYPE__=4 -v
# FIXME: MOVE __INTPTR_TYPE__ DEFINE INTO MACRO PER HOST

ramdisk:
	cd $(ROOT_DIR)/tool && nim compile --run ramdisk.nim $(ROOT_DIR)/build/driver $(ROOT_DIR)/build-aux/platform/rpi/initrd

# doxygen related
@DX_RULES@
MOSTLYCLEANFILES = $(DX_CLEANFILES)

