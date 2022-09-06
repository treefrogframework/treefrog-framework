#!/bin/bash
# Build script for CodeQL on Ubuntu

sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends qmake6 qt6-base-dev qt6-base-dev-tools qt6-tools-dev-tools qt6-declarative-dev libqt6sql6-mysql libqt6sql6-psql libqt6sql6-odbc libqt6sql6-sqlite libqt6core6 libqt6qml6 libqt6xml6 libpq5 libodbc1 libmongoc-dev libbson-dev gcc g++ clang make cmake

sudo rm -f /usr/bin/qmake
sudo ln -sf /usr/bin/qmake6 /usr/bin/qmake

./configure --spec=linux-clang
make -j10 -C src && sudo make -C src install && make -j10 -C tools && sudo make -C tools install
