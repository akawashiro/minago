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

# TODO
- なんとかしてリアルタイム3Dモデル生成をする
- z方向の位置も計算する  
    - 顔の大きさを使うか  
- 床ほしい  

# DONE
- コマンドライン引数を実装  
    - 他のモデルも見たい  
    - webカメラの画像を切るオプションをつけたい  
    - カメラの画素数もいじれたほうがいいかな  
- ESCで終了するようにしとくか
    - Qで終了
- カクつくのは積分制御でも入れるか?  

# MEMO
- Use TurboPFor-Integer-Compression in compression
- When you update librealsense, do not forget changing flags in `CMake/lrs_options.cmake`.  
  Now, I changed `BUILD_GRAPHICAL_EXAMPLES` and `BUILD_EXAMPLES`.
