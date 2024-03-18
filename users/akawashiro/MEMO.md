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
