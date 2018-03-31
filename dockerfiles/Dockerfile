FROM buildpack-deps:xenial
MAINTAINER AOYAMA Kazuharu <a.kazuharu@gmail.com>

ENV TREEFROG_VERSION development
ENV LANG C.UTF-8

RUN apt-get update && apt-get install -y \
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
        mariadb-client \
        libpq5 \
        libodbc1 \
        libmongoc-dev \
        libbson-dev \
        gcc \
        g++ \
        git \
        make && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/src/treefrog && \
    cd /usr/src/treefrog && \
    git clone https://github.com/treefrogframework/treefrog-framework.git && \
    cd treefrog-framework && \
    ./configure --enable-shared-mongoc && \
    make -j"$(nproc)" -C src && \
    make -C src install && \
    make -j"$(nproc)" -C tools && \
    make -C tools install && \
    rm -rf /usr/src/treefrog

CMD ["treefrog", "-v"]

