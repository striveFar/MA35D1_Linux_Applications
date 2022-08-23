#!/bin/bash
TF_SRC="tensorflow-2.8.2"
TF_BUILD=${1}
#TF_INSTALL=${1}

source /usr/local/oecore-x86_64/environment-setup-aarch64-poky-linux

#Check source
if [ ! -d "$TF_SRC" ]; then
	echo "Downlod tensorflow"
	wget https://github.com/tensorflow/tensorflow/archive/refs/tags/v2.8.2.zip
	unzip v2.8.2.zip
	rm -f v2.8.2.zip
fi

if [ -z $TF_BUILD ]; then
	TF_BUILD="tflite-aarch64_build"
fi

mkdir $TF_BUILD
cd $TF_BUILD

ARMCC_PREFIX=aarch64-poky-linux-
ARMCC_FLAGS="-funsafe-math-optimizations"
cmake -DCMAKE_C_COMPILER=${ARMCC_PREFIX}gcc \
  -DCMAKE_CXX_COMPILER=${ARMCC_PREFIX}g++ \
  -DCMAKE_C_FLAGS="${ARMCC_FLAGS}" \
  -DCMAKE_CXX_FLAGS="${ARMCC_FLAGS}" \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  ../$TF_SRC/tensorflow/lite/

make -j8

#make install
#-DCMAKE_INSTALL_PREFIX=../$TF_INSTALL \
