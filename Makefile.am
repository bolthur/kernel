
ACLOCAL_AMFLAGS = -I build-aux/m4

SUBDIRS = bolthur

# cppcheck stuff
.PHONY: cppcheck
cppcheck:
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_KERNEL_INCLUDE) \
		--project=$(CPPCHECK_KERNEL_PROJECT) \
		-U__CYGWIN__ -U__APPLE__ -U__GNU_VISIBLE -UASSEMBLER_FILE -UVMS \
		-U__restrict__ -U_BSD_RUNE_T_ -v -j4 --force --max-configs=12
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_LIBRARY_INCLUDE) \
		--project=$(CPPCHECK_LIBRARY_PROJECT) \
		-U__CYGWIN__ -U__APPLE__ -UVMS -v -j4 --force --max-configs=12
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
		--include=$(CPPCHECK_SERVER_INCLUDE) \
		--project=$(CPPCHECK_SERVER_PROJECT) \
		-U__CYGWIN__ -U__APPLE__ -UVMS -v -j4 --force --max-configs=12
	cppcheck --quiet -UCMC --inline-suppr --enable=all --force \
		--platform=$(CPPCHECK_PLATFORM) \
	--include=$(CPPCHECK_APPLICATION_INCLUDE) \
		--project=$(CPPCHECK_APPLICATION_PROJECT) \
		-U__CYGWIN__ -U__APPLE__ -UVMS -v -j4 --force --max-configs=12

image:
	cd $(ROOT_DIR)/tool && nim -d:release -d:ssl compile --run image.nim \
		$(ROOT_DIR) \
		$(BUILD_DIR) \
		$(SYSROOT_DIR) \
		$(ROOT_DIR)/config/ini/${platform_subdir}/${platform_name}/ \
		$(BUILD_DIR)/bolthur/kernel/target/${platform_subdir}/${output_img} \
		${platform_subdir}

README: README.md
	pandoc -f markdown -t plain --wrap=none $< -o $@
CLEANFILES = README

# doxygen related
@DX_RULES@
MOSTLYCLEANFILES = $(DX_CLEANFILES)
