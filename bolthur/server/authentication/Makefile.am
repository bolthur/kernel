
AM_CFLAGS = -DPROGRAM_NAME=\"authentication\"

bin_PROGRAMS = authentication
authentication_LDADD = -lcrypt
authentication_SOURCES = \
  pid/node.c \
  rpc/custom/fetch.c \
  rpc/custom/reload.c \
  rpc/custom/request.c \
  rpc/fork.c \
  rpc/init.c \
  main.c
authentication_LDFLAGS = -all-static --static
