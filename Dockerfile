FROM ubuntu:18.04 as build

RUN \
    apt-get update && \
    apt-get --no-install-recommends -y install \
      clang \
      libboost-dev \
      libboost-test-dev \
      libboost-system-dev \
      libc6 \
      libc6-dev \
      libstdc++6 \
      autoconf \
      automake \
      python3 \
      llvm \
      llvm-dev \
      libffi-dev \
      libz-dev \
      make

COPY . /nidhugg

RUN \
    /bin/bash -c \
    "cd /nidhugg && \
    autoreconf --install && \
    ./configure --prefix=/usr/local/ CXXFLAGS=-O3 && \
    make -j6 && \
    make install"

FROM ubuntu:18.04
RUN \
    apt-get update && \
    apt-get --no-install-recommends -y install \
      clang \
      libboost-system1.65.1 \
      libllvm6.0 \
      python3 \
      vim-tiny && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY --from=build /usr/local/bin/nidhugg* /usr/local/bin/
WORKDIR /root
