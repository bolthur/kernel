FROM archlinux/base

RUN pacman -Syu --noconfirm
RUN pacman -Sy --noconfirm \
  binutils \
  gcc \
  wget \
  bzip2 \
  autoconf \
  automake \
  libtool \
  m4 \
  grep \
  make

# Add folder
ADD . /code

# set work dir
WORKDIR /code/.travis-ci

RUN mkdir -p ~/.bolthur/cross
RUN wget -q -O $HOME/.bolthur/cross/arm-none-eabi-gcc.tar.bz2 https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2
RUN cd ~/.bolthur/cross && tar -jxf arm-none-eabi-gcc.tar.bz2 --strip=1
RUN export PATH=$HOME/.bolthur/cross/bin:$PATH
RUN cd /code && autoreconf -ivf

CMD [ "bash", "/code/.travis-ci/build.sh" ]
