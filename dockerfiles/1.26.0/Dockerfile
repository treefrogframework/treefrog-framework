FROM buildpack-deps:bionic-curl
MAINTAINER AOYAMA Kazuharu <a.kazuharu@gmail.com>

ENV TREEFROG_VERSION 1.26.0
ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -q; apt-get install -yq --no-install-recommends \
        tzdata \
        coreutils \
        qt5-default \
        qt5-qmake \
        qtbase5-dev \
        qtdeclarative5-dev \
        qtbase5-dev-tools \
        libqt5sql5-mysql \
        libqt5sql5-psql \
        libqt5sql5-odbc \
        libqt5sql5-sqlite \
        libqt5core5a \
        libqt5qml5 \
        libqt5xml5 \
        libpq5 \
        libodbc1 \
        libmongoc-dev \
        libbson-dev \
        gcc \
        g++ \
        make \
        cmake ; \
    rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/treefrog
RUN curl -sL https://github.com/treefrogframework/treefrog-framework/archive/v${TREEFROG_VERSION}.tar.gz | tar xz && \
    cd treefrog-framework-${TREEFROG_VERSION} && \
    ./configure --enable-shared-mongoc && \
    make -j"$(nproc)" -C src && \
    make -C src install && \
    make -j"$(nproc)" -C tools && \
    make -C tools install && \
    rm -rf /usr/src/treefrog

VOLUME  /webapp
WORKDIR /webapp
EXPOSE  8800
CMD ["treefrog", "-p", "8800", "/webapp"]
