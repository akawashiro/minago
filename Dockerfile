FROM ubuntu:22.04
RUN apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install \
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
COPY . /minago
WORKDIR /minago
RUN cmake -S . -B build
RUN cmake --build build -- -j
