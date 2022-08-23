#!/bin/bash
OPENCV_SRC="opencv-4.5.3"
OPENCV_BUILD="opencv-aarch64_build"
OPENCV_INSTALL=${1}

source /usr/local/oecore-x86_64/environment-setup-aarch64-poky-linux

#Check source
if [ ! -d "$OPENCV_SRC" ]; then
	echo "Downlod opencv 4.5.3"
	wget https://github.com/opencv/opencv/archive/4.5.3.zip
	unzip 4.5.3.zip
	rm -f 4.5.3.zip
fi

if [ -z $OPENCV_INSTALL ]; then
	OPENCV_INSTALL="opencv-aarch64_install"
fi

mkdir $OPENCV_BUILD
cd $OPENCV_BUILD

cmake -DCMAKE_BUILD_TYPE=RELEASE \
	-DENABLE_NEON=ON \
	-DENABLE_TBB=ON \
	-DENABLE_IPP=OFF \
	-DENABLE_VFVP3=ON \
	-DWITH_OPENMP=ON \
	-DWITH_CSTRIPES=ON \
	-DWITH_OPENCL=OFF \
	-DWITH_QT=ON \
	-DOE_QMAKE_PATH_EXTERNAL_HOST_BINS=/usr/bin \
	-DCMAKE_INSTALL_PREFIX=../$OPENCV_INSTALL \
	../$OPENCV_SRC

make VERBOSE=1 -j8 
make install
