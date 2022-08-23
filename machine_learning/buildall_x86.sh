#!/bin/bash
OPENCV_LIB="opencv-lib"
OPENCV_BUILD="opencv-x86_build"
OPENCV_INSTALL="opencv-x86_install"

TF_LIB="tensorflow-lib"
TF_BUILD="tflite-x86_build"

FACE_RECOGN_BUILD="faceRecognition-x86_build"
FACE_ENROLL_BUILD="faceEnrollment-x86_build"
OBJECT_DETECT_BUILD="objectDetect-x86_build"

if [ ${1} == "clean" ]; then
	rm -rf  $OPENCV_LIB/$OPENCV_BUILD
	rm -rf  $OPENCV_LIB/$OPENCV_INSTALL

	rm -rf  $TF_LIB/$TF_BUILD

	rm -rf  faceRecognition/$FACE_RECOGN_BUILD
	rm -rf  faceEnrollment/$FACE_ENROLL_BUILD
	rm -rf  objectDetect/$OBJECT_DETECT_BUILD

	exit 0
fi

#build opencv
cd $OPENCV_LIB
./build_x86.sh $OPENCV_INSTALL
cd ../

#build tensorflow
cd $TF_LIB
./build_x86.sh $TF_BUILD
cd ../

#build application
cd faceRecognition
./build_x86.sh ../$TF_LIB/tensorflow-2.8.2 ../$TF_LIB/$TF_BUILD ../$OPENCV_LIB/$OPENCV_INSTALL
cd ../

cd faceEnrollment
./build_x86.sh ../$TF_LIB/tensorflow-2.8.2 ../$TF_LIB/$TF_BUILD ../$OPENCV_LIB/$OPENCV_INSTALL
cd ../

cd objectDetect
./build_x86.sh ../$TF_LIB/tensorflow-2.8.2 ../$TF_LIB/$TF_BUILD ../$OPENCV_LIB/$OPENCV_INSTALL
cd ../
