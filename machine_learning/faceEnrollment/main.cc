
#include <cstdio>
#include <iostream>
#include <fstream>
#include <experimental/filesystem>

#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/optional_debug_tools.h"

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"

#include "RecognitionModel.h"

#define OUTPUT_LABEL_FILE "face_label.txt"

using namespace std;
using namespace cv;

CascadeClassifier faceCascade;

// https://en.wikipedia.org/wiki/Cosine_similarity
float cosine_similarity(std::vector<float> embedArray1, std::vector<float> embedArray2)
{
	int i, numElem;
	float ret = 0, mod1 = 0, mod2 = 0;
	
	//embedArray1 and embedArray2 must the same size.  In mobileFaceNet, it is 128.
	numElem = embedArray1.size();

	for (int i = 0; i < numElem; i++) {
		ret += embedArray1[i] * embedArray2[i];
		mod1 += embedArray1[i] * embedArray1[i];
		mod2 += embedArray2[i] * embedArray2[i];
	}

	return ret / sqrt(mod1) / sqrt(mod2);
} 

int run_face_infer(
	RecognitionModel &model,
	Mat &image,
	std::vector<float> &embeding	
)
{
	if(model.LoadImageIntoTensor((uint8_t *)image.ptr(0)) == false)
	{
		cerr << "Could not load the image to tensor" << endl;
		return -1;
	}	

	//Run inference
	if(model.RunInference() == false)
	{
		cerr << "inference failed " << endl;
		return -2;
	}

	model.LoadOutputFromTensor(embeding);

	return 0;
}

bool detect_face(
	Mat frame,
	Rect &faceROI 
)
{
    Mat frameGray;
	int maxRegion;
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
	
	return true;
}

int main(int argc, char* argv[]) {

	if (argc != 4) {
		cerr << "faceEnrollment <tflite model> <face dir> <face_cascade_file>\n";
		return -1;
	}
	
	RecognitionModel faceModel;
	string modelFile = argv[1];
	string faceDir = argv[2];
	string faceCascadeFile = argv[3];
	Mat image_bgr;
	Mat image_rgb;
	int embedDimension;
	int i;
	fstream labelFile;

	const std::experimental::filesystem::path faceDirPth(faceDir);

	labelFile.open(OUTPUT_LABEL_FILE, ios::out | ios::trunc);

	// Load model
	if(faceModel.Init(modelFile) == false)
	{
		cerr << "Error unable load face recognition model \n";
		return -2;
	}

	// Load opencv face cascade
	if( ! faceCascade.load(faceCascadeFile))
	{
		cerr << "unable load cascade file \n";
		return -3;
	}

	// Set embedDimension to output layer dimension 
	embedDimension = faceModel.GetOutputSize(0);
	std::vector<float> embedArray(embedDimension);

	string label;
	string imageFile;
	std::experimental::filesystem::file_status fileStaus;
    Rect faceROI;
    Mat faceImage;

    for(auto const& dir_entry: std::experimental::filesystem::directory_iterator(faceDirPth))
    {
		fileStaus = dir_entry.status();
		if(fileStaus.type() != std::experimental::filesystem::file_type::directory)
			continue;

		label = dir_entry.path().filename();

		for(auto const& file_entry: std::experimental::filesystem::directory_iterator(dir_entry.path()))
		{
			fileStaus = file_entry.status();
			if(fileStaus.type() != std::experimental::filesystem::file_type::regular)
				continue;

			imageFile = file_entry.path();

			// Read picture
			image_bgr = imread(imageFile, IMREAD_COLOR);
			
			if(image_bgr.empty())
			{
				cerr << "Could not read the image: " << imageFile << endl;
				continue;
			}	

			if(detect_face( image_bgr, faceROI) == false)
			{
				cerr << "No face in picture: " << imageFile << endl;
				continue;
			}
			// Crop face from picture
			faceImage = image_bgr(faceROI);

			// Convert image to rgb and resize to model input resolution
			cvtColor(faceImage, faceImage, cv::COLOR_BGR2RGB);
			resize(faceImage, faceImage, Size(faceModel.GetInputWidth(), faceModel.GetInputHeight()));

			if(run_face_infer(faceModel, faceImage, embedArray)!=0)
			{
				cerr << "Could not inference the image: " << imageFile << endl;
				continue;
			}
			
			labelFile << label << ':';
			
			//print embed to file
			for(i = 0; i < embedArray.size(); i ++)
			{
				labelFile << embedArray[i] << ':';
			}
			
			labelFile << endl;
			
			cout << imageFile << " done \n";
		}
	}
	
	labelFile.close();
	
	return 0;
}
