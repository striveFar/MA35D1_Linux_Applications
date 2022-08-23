
#include <cstdio>
#include <iostream>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"

#include "RecognitionModel.h"

//#define OPENCV_FACE_CASCADE_FILE "../../opencv-lib/opencv-x86_install/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml"
#define OPENCV_FACE_CASCADE_FILE "/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml"
#define FACE_PREDICT_THRESHOLD 0.7


using namespace std;
using namespace cv;

CascadeClassifier faceCascade;

// https://en.wikipedia.org/wiki/Cosine_similarity
static float cosine_similarity(std::vector<float> embedArray1, std::vector<float> embedArray2)
{
	int i, numElem;
	float ret = 0, mod1 = 0, mod2 = 0;
	
	if(embedArray1.size() != embedArray2.size())
		return -1;
	
	//embedArray1 and embedArray2 must the same size.  In mobileFaceNet, it is 128.
	numElem = embedArray1.size();

	for (int i = 0; i < numElem; i++) {
		ret += embedArray1[i] * embedArray2[i];
		mod1 += embedArray1[i] * embedArray1[i];
		mod2 += embedArray2[i] * embedArray2[i];
	}

	return ret / sqrt(mod1) / sqrt(mod2);
} 

std::tuple<int, float> predict_face(
	RecognitionModel &faceModel,
	std::vector<RecognitionModel::S_LABEL_INFO> &LabelInfo,
	Mat &face
)
{
	int embedDimension;
	int i;

    int64 t0 = cv::getTickCount();

	// Set embedDimension to output layer dimension 
	embedDimension = faceModel.GetOutputSize(0);
	
	// Feed image data to model
	if(faceModel.LoadImageIntoTensor((uint8_t *)face.ptr(0)) == false)
	{
		cerr << "load image to tensor failed " << endl;
		return std::make_tuple(-1, 0.0);
	}

	//Run inference
	if(faceModel.RunInference() == false)
	{
		cerr << "inference failed " << endl;
		return std::make_tuple(-2, 0.0);
	}
	
	//Get output tensor
    int64 t1 = cv::getTickCount();
	
	std::vector<float> embedArray(embedDimension);
	
	faceModel.LoadOutputFromTensor(embedArray);
	
	float distance;
	
	int predictLabelIndex = -1;
	float closedDisatance = -1;
	
	for(i = 0 ; i < LabelInfo.size(); i ++)
	{
		distance = cosine_similarity(embedArray, LabelInfo[i].fParam);
		
		if(distance > closedDisatance)
		{
			predictLabelIndex = i;
			closedDisatance = distance;
		}
	}
	
	cout << "cal dist" << closedDisatance << "time " << ((t1-t0)/cv::getTickFrequency()) << endl;
	return std::make_tuple(predictLabelIndex, closedDisatance);
}

bool detect_face(
	Mat frame,
	Rect &faceROI 
)
{
    Mat frameGray;
	int maxRegion = 0;
	int faceRegion;
	int maxRegionFace = -1;

    cvtColor( frame, frameGray, COLOR_BGR2GRAY );
//    equalizeHist( frame_gray, frame_gray );
    //-- Detect faces
    std::vector<Rect> faces;
    faceCascade.detectMultiScale( frameGray, faces, 1.3, 3, 0, Size(), Size());

    for ( size_t i = 0; i < faces.size(); i++ )
    {
		faceRegion = faces[i].width * faces[i].height;
		
		if(faceRegion > maxRegion){
			maxRegion = faceRegion;
			maxRegionFace = i;
		}		
    }

	if(maxRegionFace == -1)
		return false;
	
	faceROI.width = faces[maxRegionFace].width;
	faceROI.height = faces[maxRegionFace].height;
	faceROI.x = faces[maxRegionFace].x;
	faceROI.y = faces[maxRegionFace].y;
	
	cout << "face detected" << endl;
	return true;
}


int main(int argc, char* argv[]) {

	if (argc != 3) {
		cerr << "faceRecognition <tflite model> <label file> \n";
		return -1;
	}

	RecognitionModel faceModel;
	string modelFile = argv[1];
	string labelFile = argv[2];
	Mat image_bgr;
	Mat image_rgb;
	int i;

	String cascadeFile = OPENCV_FACE_CASCADE_FILE;

	// Load opencv face cascade
	if( ! faceCascade.load(cascadeFile))
	{
		cerr << "unable load cascade file \n";
		return -2;
	}

    VideoCapture capture;

	// Read the video stream
	capture.open( 0, cv::CAP_V4L2 );
	if ( ! capture.isOpened() )
	{
		cerr << "Error opening video capture\n";
		return -3;
	}

	// Setup capture property
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 320);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
	// Try to reduce video-in latency
//    capture.set(cv::CAP_PROP_FRAME_COUNT , 1);
    capture.set(cv::CAP_PROP_BUFFERSIZE , 1);
    capture.set(cv::CAP_PROP_FPS  , 15);

	// Load model
	if(faceModel.Init(modelFile) == false)
	{
		cerr << "Error unable load face recognition model \n";
		return -4;
	}
	
	// Read label file
	std::vector<RecognitionModel::S_LABEL_INFO> LabelInfo;
		
	if(faceModel.ReadLabelsFile(labelFile, LabelInfo, nullptr) == false)
	{
		cerr << "Error unable load face recognition model \n";
		return -5;
	}
	
	// Read image from camera
    Mat frame;
    Rect faceROI;
    Mat faceImage;
    int predictIndex;
    float predictValue;    
    Mat displayImage;
    string predictLabelInfo;

    while ( capture.read(frame) )
    {
        if( frame.empty() )
        {
            cout << "(!) No captured frame -- Break!\n";
            break;
        }

        //Apply face image to predict
        if(detect_face( frame, faceROI) == true)
        {
			faceImage = frame(faceROI);
			cvtColor(faceImage, faceImage, cv::COLOR_BGR2RGB);
			resize(faceImage, faceImage, Size(faceModel.GetInputWidth(), faceModel.GetInputHeight()));

			tie(predictIndex, predictValue) = predict_face(faceModel, LabelInfo, faceImage);

			predictLabelInfo.clear();

			// draw face label 
			if((predictIndex >= 0) && (predictValue > FACE_PREDICT_THRESHOLD))
			{
				predictLabelInfo = LabelInfo[predictIndex].szLable + string(":") + std::to_string(predictValue);
				putText (frame, predictLabelInfo, Point(20,50), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(200,0,0), 2);
			}
			else
			{
				predictLabelInfo = string("???") + string(":") + std::to_string(predictValue);
				putText (frame, predictLabelInfo, Point(20,50), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(200,0,0), 2);
			}
			
			// draw face rectangle
			rectangle( frame, faceROI, Scalar(200,0,0), 0, 4, 0);

		}

		 //-- Show what you got
		resize(frame, displayImage, Size(640, 480));
		imshow( "Face detection", displayImage );

		if( waitKey(1) == 27 )
        {
            break; // escape
        }
    }
	
	return 0;
}
