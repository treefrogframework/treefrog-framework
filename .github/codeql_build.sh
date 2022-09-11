#!/bin/bash
# Build script for CodeQL on Ubuntu 22.04

sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends qmake6 qt6-base-dev qt6-base-dev-tools qt6-tools-dev-tools qt6-declarative-dev libqt6sql6-mysql libqt6sql6-psql libqt6sql6-odbc libqt6sql6-sqlite libqt6core6 libqt6qml6 libqt6xml6 libpq5 libodbc1 libmongoc-dev libbson-dev gcc g++ clang make cmake

./configure --spec=linux-clang
make -j4 -C src && sudo make -C src install && make -j4 -C tools && sudo make -C tools install
