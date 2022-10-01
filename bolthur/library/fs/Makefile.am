
AM_CFLAGS = -DPROGRAM_NAME=\"bolthur/libfs\"

noinst_LTLIBRARIES = libfs.la
libfs_la_DEPENDENCIES = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
libfs_la_LIBADD = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
libfs_la_SOURCES = \
  ext/blockgroup.c \
  ext/cache.c \
  ext/directory.c \
  ext/fs.c \
  ext/inode.c \
  ext/superblock.c \
  fat/cache.c \
  mbr/partition.c \
  cache.c
