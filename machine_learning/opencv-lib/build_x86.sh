#!/bin/bash
OPENCV_SRC="opencv-4.5.3"
OPENCV_BUILD="opencv-x86_build"
OPENCV_INSTALL=${1}

#Check source
if [ ! -d "$OPENCV_SRC" ]; then
	echo "Downlod opencv 4.5.3"
	wget https://github.com/opencv/opencv/archive/4.5.3.zip
	unzip 4.5.3.zip
	rm -f 4.5.3.zip
fi

if [ -z $OPENCV_INSTALL ]; then
	OPENCV_INSTALL="opencv-x86_install"
fi

mkdir $OPENCV_BUILD
cd $OPENCV_BUILD

cmake -DCMAKE_BUILD_TYPE=RELEASE \
	-DENABLE_TBB=ON \
	-DENABLE_IPP=OFF \
	-DWITH_OPENMP=ON \
	-DWITH_CSTRIPES=ON \
	-DWITH_OPENCL=OFF \
	-DWITH_QT=ON \
	-DOE_QMAKE_PATH_EXTERNAL_HOST_BINS=/usr/bin \
	-DCMAKE_INSTALL_PREFIX=../$OPENCV_INSTALL \
	../$OPENCV_SRC

make VERBOSE=1 -j8 
make install
