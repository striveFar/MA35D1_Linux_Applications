#!/bin/bash
PROG_NAME="objectDetect"
OBJECT_DETECT_BUILD=${PROG_NAME}-aarch64_build
TF_SRC=${1}
TF_BUILD=${2}
OPENCV_INSTALL=${3}

source /usr/local/oecore-x86_64/environment-setup-aarch64-poky-linux

echo $TF_SRC
echo $TF_BUILD
echo $OPENCV_INSTALL

if [ ! -d "$TF_SRC" ]; then
	echo "Tensorflow source not found"
	exit 1
fi

if [ ! -d "$TF_BUILD" ]; then
	echo "Tensorflow build not found"
	exit 1
fi

if [ ! -d "$OPENCV_INSTALL" ]; then
	echo " OpenCV install not found"
	exit 1
fi

mkdir $OBJECT_DETECT_BUILD
cd $OBJECT_DETECT_BUILD

cmake -DTF_SRC=$TF_SRC \
	-DTF_FLATBUF_HEADER=$TF_BUILD/flatbuffers/include/ \
	-DTF_BUILD=$TF_BUILD/ \
	-DOPENCV_INSTALL=$OPENCV_INSTALL/ \
	-DAARCH64=ON\
	../

make VERBOSE=1
patchelf --set-rpath '/usr/lib' $PROG_NAME
aarch64-poky-linux-strip $PROG_NAME
