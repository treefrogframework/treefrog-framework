FROM buildpack-deps:noble-curl
LABEL maintainer="AOYAMA Kazuharu <a.kazuharu@gmail.com>"

ENV TREEFROG_VERSION 2.11.0
ENV LANG C.UTF-8
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -q; apt-get install -yq --no-install-recommends \
        tzdata \
        coreutils \
        pkg-config \
        qmake6 \
        qt6-base-dev \
        qt6-base-dev-tools \
        qt6-tools-dev-tools \
        qt6-declarative-dev \
        libqt6sql6-mysql \
        libqt6sql6-psql \
        libqt6sql6-odbc \
        libqt6sql6-sqlite \
        libqt6core6 \
        libqt6qml6 \
        libqt6xml6 \
        libpq5 \
        libodbc2 \
        libmongoc-dev \
        libbson-dev \
        yarn \
        gcc \
        g++ \
        clang \
        make \
        cmake ; \
    rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/treefrog
RUN curl -sL https://github.com/treefrogframework/treefrog-framework/archive/v${TREEFROG_VERSION}.tar.gz | tar xz && \
    cd treefrog-framework-${TREEFROG_VERSION} && \
    ./configure --enable-shared-mongoc --spec=linux-clang && \
    make -j"$(nproc)" -C src && \
    make -C src install && \
    make -j"$(nproc)" -C tools && \
    make -C tools install && \
    rm -rf /usr/src/treefrog

VOLUME  /webapp
WORKDIR /webapp
EXPOSE  8800
CMD ["treefrog", "-p", "8800", "/webapp"]
