
ACLOCAL_AMFLAGS = -I build-aux/m4

SUBDIRS = bolthur

# cppcheck stuff
.PHONY: cppcheck
cppcheck:
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_KERNEL_INCLUDE) \
		--project=$(CPPCHECK_KERNEL_PROJECT) \
		-U__CYGWIN__ -U__APPLE__ -U__GNU_VISIBLE -UASSEMBLER_FILE -UVMS -v
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_LIBRARY_INCLUDE) \
		--project=$(CPPCHECK_LIBRARY_PROJECT) \
		-U__CYGWIN__ -U__APPLE__ -UVMS -v
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_SERVER_INCLUDE) \
		--project=$(CPPCHECK_SERVER_PROJECT) \
		-U__CYGWIN__ -U__APPLE__ -UVMS -v
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_APPLICATION_INCLUDE) \
		--project=$(CPPCHECK_APPLICATION_PROJECT) \
		-U__CYGWIN__ -U__APPLE__ -UVMS -v

ramdisk:
	cd $(ROOT_DIR)/tool && nim -d:release compile --run ramdisk.nim \
		$(ROOT_DIR)/build \
		$(ROOT_DIR)/build-aux/platform/${platform_subdir}/initrd \
		$(SYSROOT_DIR) \
		$(ROOT_DIR)/config/ini/${platform_subdir}/${platform_name}/

README: README.md
	pandoc -f markdown -t plain --wrap=none $< -o $@
CLEANFILES = README

# doxygen related
@DX_RULES@
MOSTLYCLEANFILES = $(DX_CLEANFILES)
