# Copyright (C) 2025 Southern California Edison.

FROM ubuntu:24.04

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
        libprotobuf-c-dev \
        pkg-config \
        protobuf-compiler \
        protobuf-c-compiler \
    && rm -rf /var/lib/apt/lists/
