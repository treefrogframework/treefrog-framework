#!/bin/bash
# Build script for CodeQL on Ubuntu

sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends qtbase5-dev qt5-default qt5-qmake qttools5-dev-tools qtdeclarative5-dev qtdeclarative5-dev-tools libqt5sql5 libqt5sql5-sqlite libsqlite3-dev libmongoc-dev libbson-dev gcc g++ clang make cmake

./configure --spec=linux-clang
make -j10 -C src && sudo make -C src install && make -j10 -C tools && sudo make -C tools install
