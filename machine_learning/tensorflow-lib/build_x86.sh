#!/bin/bash
TF_SRC="tensorflow-2.8.2"
TF_BUILD=${1}
#TF_INSTALL=${1}

#Check source
if [ ! -d "$TF_SRC" ]; then
	echo "Downlod tensorflow"
	wget https://github.com/tensorflow/tensorflow/archive/refs/tags/v2.8.2.zip
	unzip v2.8.2.zip
	rm -f v2.8.2.zip

fi

if [ -z $TF_BUILD ]; then
	TF_BUILD="tflite-x86_build"
fi

mkdir $TF_BUILD
cd $TF_BUILD

cmake -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
	../$TF_SRC/tensorflow/lite

#cmake
#  -DCMAKE_BUILD_TYPE=Debug \
#  -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
#  ../$TF_SRC/tensorflow/lite/

make -j8

#make install
#-DCMAKE_INSTALL_PREFIX=../$TF_INSTALL \
