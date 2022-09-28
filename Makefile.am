
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

image:
	cd $(ROOT_DIR)/tool && nim -d:release -d:ssl compile --run image.nim \
		$(SYSROOT_DIR) \
		$(ROOT_DIR)/build \
		$(ROOT_DIR)/build-aux/platform/${platform_subdir}/initrd \
		$(ROOT_DIR)/config/ini/${platform_subdir}/${platform_name}/ \
		$(ROOT_DIR)/build/bolthur/kernel/target/${platform_subdir}/${output_img} \
		$(ROOT_DIR)/build-aux/platform/${platform_subdir}/config.txt \
		$(ROOT_DIR)/build-aux/platform/${platform_subdir}/cmdline.txt \
		${platform_subdir} \
		$(ROOT_DIR)/build-aux/platform/${platform_subdir}/sdcard.img

README: README.md
	pandoc -f markdown -t plain --wrap=none $< -o $@
CLEANFILES = README

# doxygen related
@DX_RULES@
MOSTLYCLEANFILES = $(DX_CLEANFILES)
