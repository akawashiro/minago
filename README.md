# How to install
## For ubuntu 22.04
```bash
sudo apt-get -y install \
         cmake \
         git \
         build-essential \
         freeglut3-dev \
         libopencv-dev \
         libboost-dev \
         libboost-program-options-dev \
         xorg-dev \
         libglu1-mesa-dev \
         doxygen \
         libusb-1.0-0-dev
git clone https://github.com/akawashiro/minago.git
cd minago
cmake -S . -B build
cmake --build build -- -j
```
## For Mac OS X
```bash
brew install cmake libusb opencv boost doxygen
git clone https://github.com/akawashiro/minago.git
cd minago
cmake -S . -B build
cmake -DCMAKE_CXX_FLAGS="-I/usr/local/Cellar/glog/0.4.0/include \-I/usr/local/Cellar/gflags/2.2.2/include -L/usr/local/Cellar/glog/0.4.0/lib -L/usr/local/Cellar/gflags/2.2.2/lib" --build build -- -j
```

# How to run
```bash
./build/launch-minago.sh
```
If you get any error, please retry with `GLOG_logtostderr=1 ./build/launch-minago.sh`.
