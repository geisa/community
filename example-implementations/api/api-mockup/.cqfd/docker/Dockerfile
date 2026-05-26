# Copyright (C) 2025 Southern California Edison.

FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

#To be removed when the issue with the default mirror is resolved.
RUN sed -i \
    -e 's|http://archive.ubuntu.com/ubuntu|http://fr.archive.ubuntu.com/ubuntu|g' \
    -e 's|http://security.ubuntu.com/ubuntu|http://fr.archive.ubuntu.com/ubuntu|g' \
    /etc/apt/sources.list.d/ubuntu.sources

RUN set -x \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        clang-format \
        clang-tidy \
        libmosquitto-dev \
        libnanopb-dev \
        libprotobuf-c-dev \
        pkg-config \
        protobuf-compiler \
        python3-pip \
    && rm -rf /var/lib/apt/lists/

RUN set -x \
    && pip3 install --no-cache-dir --break-system-packages \
        nanopb
