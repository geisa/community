FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
ENV LC_ALL en_US.UTF-8

# Yocto mandatory packages for Ubuntu, list taken from:
# https://docs.yoctoproject.org/brief-yoctoprojectqs/index.html
# and added 'tmux' to allow 'devshell' tasks
RUN apt-get update && apt-get install -y --no-install-recommends \
	build-essential \
	chrpath \
	cpio \
	debianutils \
	diffstat \
	file \
	gawk \
	gcc \
	git \
	iputils-ping \
	libacl1 \
	libegl1-mesa \
	liblz4-tool \
	libsdl1.2-dev \
	locales \
	mesa-common-dev \
	python3 \
	python3-git \
	python3-jinja2 \
	python3-pexpect \
	python3-pip \
	python3-subunit \
	socat \
	texinfo \
	tmux \
	unzip \
	wget \
	xz-utils \
	zstd \
	&& rm -rf /var/lib/apt/lists/*

# Other useful packages
RUN apt-get update && apt-get install -y --no-install-recommends \
	asciidoc \
	bash \
	bc \
	bison \
	ca-certificates \
	curl \
	flex \
	gcc-multilib \
	g++-multilib \
	gir1.2-gtk-3.0 \
	git-core \
	libncurses5-dev \
	libssl-dev \
	poppler-utils \
	python-is-python3 \
	python3-gi \
	rsync \
	ruby \
	ruby-asciidoctor-pdf \
	ssh \
	sshpass \
	subversion \
	sudo \
	swig \
	xterm \
	xxd \
	zip \
	&& rm -rf /var/lib/apt/lists/*

# Packages for sfl_cd
COPY requirements.txt /tmp/requirements.txt
RUN pip3 install -r /tmp/requirements.txt

RUN curl https://storage.googleapis.com/git-repo-downloads/repo > /usr/local/bin/repo \
	&& chmod a+x /usr/local/bin/repo

RUN sed -i "s/# en_US\.UTF-8 UTF-8/en_US\.UTF-8 UTF-8/" /etc/locale.gen \
	&& locale-gen \
	&& dpkg-reconfigure locales
