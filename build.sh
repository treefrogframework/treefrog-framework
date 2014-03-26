make -C src all -j10
sudo make -C src install
make -C tools all -j10
sudo make -C src install
cd test && ./testall.sh
