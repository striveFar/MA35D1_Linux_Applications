#!/bin/bash
OPENCV_LIB="opencv-lib"
OPENCV_BUILD="opencv-aarch64_build"
OPENCV_INSTALL="opencv-aarch64_install"

TF_LIB="tensorflow-lib"
TF_BUILD="tflite-aarch64_build"

FACE_RECOGN_BUILD="faceRecognition-aarch64_build"
OBJECT_DETECT_BUILD="objectDetect-aarch64_build"


if [ ${1} == "clean" ]; then
	rm -rf  $OPENCV_LIB/$OPENCV_BUILD
	rm -rf  $OPENCV_LIB/$OPENCV_INSTALL

	rm -rf  $TF_LIB/$TF_BUILD

	rm -rf  faceRecognition/$FACE_RECOGN_BUILD
	rm -rf  objectDetect/$OBJECT_DETECT_BUILD
	exit 0
fi

#build opencv
cd $OPENCV_LIB
./build_aarch64.sh $OPENCV_INSTALL
cd ../

#build tensorflow
cd $TF_LIB
./build_aarch64.sh $TF_BUILD

cd ../

#build application
cd faceRecognition
./build_aarch64.sh ../$TF_LIB/tensorflow-2.8.2 ../$TF_LIB/$TF_BUILD ../$OPENCV_LIB/$OPENCV_INSTALL
cd ../

cd objectDetect
./build_aarch64.sh ../$TF_LIB/tensorflow-2.8.2 ../$TF_LIB/$TF_BUILD ../$OPENCV_LIB/$OPENCV_INSTALL
cd ../
