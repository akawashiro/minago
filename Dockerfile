FROM ubuntu:20.04
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install cmake gcc g++ freeglut3 freeglut3-dev libopencv-dev libboost-dev xorg-dev libglu1-mesa-dev gnupg software-properties-common libboost-program-options-dev doxygen libgoogle-glog-dev
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install apt-utils
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install libusb-1.0-0-dev
COPY . /minago
RUN rm -rf /minago/build
RUN mkdir /minago/build
RUN cd /minago/build && cmake .. && make -j4
