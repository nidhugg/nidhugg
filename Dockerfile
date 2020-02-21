FROM ubuntu:18.04

RUN \
    apt-get update && \
    apt-get -y install \
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
      make && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY . /nidhugg

RUN \
    /bin/bash -c \
    "cd /nidhugg && \
    autoreconf --install && \
    ./configure && \
    make && \
    make install"
