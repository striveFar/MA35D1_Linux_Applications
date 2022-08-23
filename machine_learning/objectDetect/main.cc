#include <cstdio>
#include <iostream>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"

#include "ObjectDetModel.h"

using namespace std;
using namespace cv;

//#define _OPEN_CAM
#define _READ_IMAGE_FILE

static string format(const string& format, ...)
{
    va_list args;
    va_start(args, format);
    size_t len = vsnprintf(NULL, 0, format.c_str(), args);
    va_end(args);
    vector<char> vec(len + 1);
    va_start(args, format);
    vsnprintf(&vec[0], len + 1, format.c_str(), args);
    va_end(args);
    return &vec[0];
}

static int object_detect(
	ObjectDetModel &objectDetModel,
	vector<S_DETECT_BOX> &detectBoxInfo,
	Mat &modelImage,
	Mat &srcImage
)
{
	detectBoxInfo.clear();

    int64 t0 = cv::getTickCount();

	// Feed image data to model
	if(objectDetModel.LoadImageIntoTensor((uint8_t *)modelImage.ptr(0)) == false)
	{
		cerr << "load image to tensor failed " << endl;
		return -1;
	}

	//Run inference
	if(objectDetModel.RunInference() == false)
	{
		cerr << "inference failed " << endl;
		return -2;
	}
	
	//Get output tensor
    int64 t1 = cv::getTickCount();

	objectDetModel.GetDetBoxes(detectBoxInfo, srcImage.cols, srcImage.rows);

	return detectBoxInfo.size();
}

int main(int argc, char* argv[]) {

	if (argc != 3) {
		cerr << "objectDetect <tflite model> <label file> \n";
		return -1;
	}

	ObjectDetModel objectDetModel;
	string modelFile = argv[1];
	string labelFile = argv[2];

#if defined(_OPEN_CAM)
    VideoCapture capture;

	// Read the video stream
	capture.open( 0, cv::CAP_V4L2 );
	if ( ! capture.isOpened() )
	{
		cerr << "Error opening video capture\n";
		return -2;
	}
#endif

	// Load model
	if(objectDetModel.Init(modelFile) == false)
	{
		cerr << "Error unable load face recognition model \n";
		return -3;
	}
	
	int i;
	
	for( i = 0; i < objectDetModel.GetNumberOfOutputs(); i ++)
	{
		cout << format("Model output layer size [%d] = %d ", i, objectDetModel.GetOutputSize(i)) << endl;
	}
	 
#if defined(_OPEN_CAM)
	// Setup capture property
    capture.set(cv::CAP_PROP_FRAME_WIDTH, objectDetModel.GetInputWidth());
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, objectDetModel.GetInputHeight());
	// Try to reduce video-in latency
//    capture.set(cv::CAP_PROP_FRAME_COUNT , 1);
    capture.set(cv::CAP_PROP_BUFFERSIZE , 1);
    capture.set(cv::CAP_PROP_FPS  , 15);
#endif

	//Read label file
	std::vector<ObjectDetModel::S_LABEL_INFO> labelInfo;
	std::vector<S_DETECT_BOX> detectBox;

		
	if(objectDetModel.ReadLabelsFile(labelFile, labelInfo, nullptr) == false)
	{
		cerr << "Error unable load face recognition model \n";
		return -4;
	}
	
	// Read image from camera
    Mat frame;
    Mat resizeFrame;
    
#if defined(_OPEN_CAM)
    while ( capture.read(frame) )
    {
#endif
#if defined(_READ_IMAGE_FILE)
		// Read picture
		frame = imread("../sample/dinner.jpg", IMREAD_COLOR);
#endif

		if(frame.empty())
		{
			cerr << "Could not read the image " << endl;
			return -1;
		}	

		// Convert image to rgb and resize to model input resolution
		resize(frame, resizeFrame, Size(objectDetModel.GetInputWidth(), objectDetModel.GetInputHeight()));
		cvtColor(resizeFrame, resizeFrame, cv::COLOR_BGR2RGB);

        if( resizeFrame.empty() )
        {
            cout << "Could not resize the image \n";
#if defined(_OPEN_CAM)
            break;
#endif
        }

		if(object_detect(objectDetModel, detectBox, resizeFrame, frame) > 0)
		{
			Rect detectROI;
			int font = cv::FONT_HERSHEY_COMPLEX;
			double font_scale = 0.5;
			int thickness = 1;
			int baseline;

			//Draw object position and label
			for(i = 0; i < detectBox.size(); i ++)
			{
				detectROI.width = detectBox[i].sBoxPos.u32Width;
				detectROI.height = detectBox[i].sBoxPos.u32Height;
				detectROI.x = detectBox[i].sBoxPos.u32X;
				detectROI.y = detectBox[i].sBoxPos.u32Y;

				// draw face rectangle
				rectangle( frame, detectROI, Scalar(200,0,0), 0, 4, 0);

				// decide text position
				cv::Point origin;
				origin.x = detectROI.x ;
				origin.y = detectROI.y ;
				cv::putText(frame, labelInfo[detectBox[i].i32ClassIndex].szLabel, origin, font, font_scale, cv::Scalar(0, 0, 255), thickness, 8, 0);

			}

		}

		imshow("detect frame", frame);

#if defined(_READ_IMAGE_FILE)
		while(1)
        {
			if(cv::waitKey(100) == 27)	//escape key
				break;
        }
#endif

#if defined(_OPEN_CAM)

		if( waitKey(1) == 27 )
        {
            break; // escape
        }

	}
#endif

#if defined(_OPEN_CAM)
	capture.release();
#endif

	return 0;
}
