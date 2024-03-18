# How to install
## For ubuntu 22.04
```bash
sudo apt-get -y install cmake gcc g++ freeglut3 freeglut3-dev libopencv-dev libboost-dev xorg-dev libglu1-mesa-dev software-properties-common libboost-program-options-dev apt-utils libusb-1.0-0-dev doxygen
git clone https://github.com/akawashiro/minago.git
cd minago
cmake -S . -B build
cmake --build build -- -j
```
## For Mac OS X
```bash
brew install cmake libusb opencv boost doxygen glog gflags
git clone https://github.com/akawashiro/minago.git
cd minago
cmake -S . -B build
cmake -DCMAKE_CXX_FLAGS="-I/usr/local/Cellar/glog/0.4.0/include \-I/usr/local/Cellar/gflags/2.2.2/include -L/usr/local/Cellar/glog/0.4.0/lib -L/usr/local/Cellar/gflags/2.2.2/lib" --build build -- -j
```

# How to run
```bash
cd minago/build
./launch-minago.sh  
```
If you get any error, please retry with `GLOG_logtostderr=1 ./launch-minago.sh`.

# For contributors
Please check your patches are built successfully by the following command.
```
sudo docker build --no-cache -t minago .
```

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
