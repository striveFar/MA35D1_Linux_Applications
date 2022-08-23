MA35D1 machine learning applicaton project
---
## Kernel
1. Set MA35D1 CCAP status to "okay".  
		MA35D1_linux-5.4.y/arch/arm64/boot/dts/nuvoton/ma35d1.dtsi

		ccap0: ccap@40240000{
			compatible = "nuvoton,ma35d1-ccap";
			reg = <0x0 0x40240000 0x0 0x2000>;
			interrupts = <GIC_SPI 21 IRQ_TYPE_LEVEL_HIGH>;
			status = "disabled";  --->  "okay"
			port {
				/* Parallel bus endpoint */
				ccap0_1: endpoint {
					remote-endpoint = <&hm1055_0>;
					hsync-active = <0>;     /* Active low */
					vsync-active = <0>;     /* Active low */
					pclk-sample = <1>;      /* Rising */
				};
			};
		};
2. Rebuild kernel
## Application
1. Face enrollment
2. Face recognition
3. Object detect
## Require
1. MA35D1 SDK package which exported from MA35D1's Yocto
2. patchelf tool  
	#sudo apt install patchelf
3. QT5  
	#sudo apt install qt5-default
## Build
1. Keep internet connection, it will download OpenCV and tensorflow source from offical website
2. Execute build script  
	#buildall_aarch64.sh (for Arm aarch64 target)  
	#buildall_x86.sh (for x86 target)
3. Pack OpenCV install package  
	#cd opencv-lib  
	#./pack.sh
## Install
1. Execute faceEnrollment on PC to generate face labeled file  
	a. Create face label folder within faceEnrollment/sample  
	b. Put face picture into face label folder.

			     sample
				|
				+---John
				|    |				
				|    +---a.jpg
				|    |
				|    +---b.jpg
				|
				+---Leo
				|    |				
				|    +---c.jpg
				|    |
				|    +---d.jpg
	c. Execute faceEnrollment on PC.  
	&emsp;#cd faceEnrollment-x86_build  
	&emsp;#./faceEnrollment ./../faceRecognition/model/FaceMobileNet_Float32.tflite ../sample ../../opencv-lib/opencv-x86_install/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml  
	d. Labeled face embedding information will be output to "face_label.txt".
2. Install OpenCV libarary on MA35D1 target board  
	a. Copy opencv_target.tar.gz and target_opencv_install.sh to target board  
	b. Run install script  
	&emsp;#./target_opencv_install.sh
3. Copy application, model and labeled files to target board  
	a. Face recognition  
		&emsp;Application: faceRecognition/faceRecognition-aarch64_build/faceRecognition  
		&emsp;Model: faceRecognition/model/FaceMobileNet_Float32.tflite    
		&emsp;Label: faceEnrollment/faceEnrollment-x86_build/face_label.txt    
	b. Object detect  
		&emsp;Application: objectDetect/objectDetect-aarch64_build/objectDetect  
		&emsp;Model: objectDetect/model/mobilenetv3_ssd_float.tflite  
		&emsp;Label: objectDetect/lable/coco_2017_label.txt  
## Run on MA35D1
1. Execute faceRecognition  
	#./faceRecognition FaceMobileNet_Float32.tflite face_label.txt
2. Execute objectDetect  
	#./objectDetect mobilenetv3_ssd_float.tflite coco_2017_label.txt

	

