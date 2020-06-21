FROM ubuntu:20.04
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install cmake gcc g++ freeglut3 freeglut3-dev libopencv-dev libboost-dev xorg-dev libglu1-mesa-dev gnupg software-properties-common libboost-program-options-dev
RUN apt-key adv --keyserver keys.gnupg.net --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCDE || apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-key F6E65AC044F831AC80A06380C8B3A55A6F3EFCDE
RUN add-apt-repository "deb http://realsense-hw-public.s3.amazonaws.com/Debian/apt-repo bionic main" -u
RUN apt-get update && apt-get -y install librealsense2-dkms librealsense2-utils librealsense2-dev librealsense2-dbg
COPY . /minago
RUN rm -rf /minago/build
RUN mkdir /minago/build
RUN cd /minago/build && cmake .. && make -j4
