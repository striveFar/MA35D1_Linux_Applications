#include <iostream>
#include <fstream> 

#include "ObjectDetModel.h"
#include "YoloConfig.h"

#define LOG(x) std::cout

/* Initialise the model */
ObjectDetModel::ObjectDetModel() :
    m_bInited (false),
    m_tType(kTfLiteNoType),
    m_pInterpreter(nullptr),
    m_pModel(nullptr),
    m_i32NumberOfThreads(2),
    m_fThreshold(0.5)
{

}

ObjectDetModel::~ObjectDetModel()
{
    
    /**
     * No clean-up function available for allocator in TensorFlow Lite Micro yet.
     **/
}

bool ObjectDetModel::Init(string &model_file)
{
	// Load model
	m_pModel = FlatBufferModel::BuildFromFile(model_file.c_str());
	
	if(m_pModel == nullptr)
		return  false;

	// Build the interpreter with the InterpreterBuilder.
	// Note: all Interpreters should be built with the InterpreterBuilder,
	// which allocates memory for the Interpreter and does various set up
	// tasks so that the Interpreter can read the provided model.
	ops::builtin::BuiltinOpResolver resolver;
	InterpreterBuilder builder(*m_pModel, resolver);
	builder(&(m_pInterpreter));

	if(m_pInterpreter == nullptr)
		return  false;

	//Get the tensor index of first input layer 
	int input = m_pInterpreter->inputs()[0];
	m_tInputDatatype = m_pInterpreter->tensor(input)->type;

	//Using the tensor index to query tensor data type
	if (m_tInputDatatype == kTfLiteFloat32) {
		LOG(INFO) << "Floating point Tensorflow Lite Model\n";
	}
	else if (m_tInputDatatype == kTfLiteUInt8) {
		LOG(INFO) << "Unsigned int8 Tensorflow Lite Model\n";
	}
	else if (m_tInputDatatype == kTfLiteInt8) {
		LOG(INFO) << "Int8 Tensorflow Lite Model\n";
	}
	else {
		LOG(INFO) << "Other type Tensorflow Lite Model" << m_tInputDatatype << "\n";
	}

	//Get the tensor index of output layer
	int output = m_pInterpreter->outputs()[0];
	m_tOutDatatype = m_pInterpreter->tensor(output)->type;

	if (m_i32NumberOfThreads != -1)
	{
		m_pInterpreter->SetNumThreads(m_i32NumberOfThreads);
	}

	if (m_pInterpreter->AllocateTensors() != kTfLiteOk) {
		LOG(FATAL) << "Failed to allocate tensors!";
		return false;
	}


	if(GetNumberOfOutputs() == 4)
	{
		m_eModelType = eMODEL_TYPE_MOBILENET_SSD;
	}
	else
	{
		m_eModelType = eMODEL_TYPE_YOLO;
	}

	if(m_eModelType == eMODEL_TYPE_YOLO)
	{
		int output_channels;
		int num_class;
		//Get the tensor index of first output layer 
		int output = m_pInterpreter->outputs()[0];

		//Tensorflow defatult tensor format NHWC. 0:N--->batch ,1:H--->height ,2:W--->width, 3:C--->channel 
		TfLiteIntArray* output_dims = m_pInterpreter->tensor(output)->dims;
		output_channels = output_dims->data[3];
		num_class = (output_channels / ANCHOR_BOX) - 5;
		m_sYoloPP = YoloPostprocessing(m_fThreshold, 0.45, num_class, 0, m_tOutDatatype);
	}
	
	m_bInited = true;
	return true;
}

/*
// Fixed size list of integers. Used for dimensions and inputs/outputs tensor
// indices
typedef struct {
  int size;  <---axis
// gcc 6.1+ have a bug where flexible members aren't properly handled
// https://github.com/google/re2/commit/b94b7cd42e9f02673cd748c1ac1d16db4052514c
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ == 6 && \
    __GNUC_MINOR__ >= 1
  int data[0];
#else
  int data[]; <--- record each axis dimensions
#endif
} TfLiteIntArray;

*/

int ObjectDetModel::GetInputWidth()
{
	//Get the tensor index of first input layer 
	int input = m_pInterpreter->inputs()[0];

	//Tensorflow defatult tensor format NHWC. 0:N--->batch ,1:H--->height ,2:W--->width, 3:C--->channel 
	TfLiteIntArray* input_dims = m_pInterpreter->tensor(input)->dims;
	return input_dims->data[2];
}

int ObjectDetModel::GetInputHeight()
{
	//Get the tensor index of first input layer 
	int input = m_pInterpreter->inputs()[0];

	//Tensorflow defatult tensor format NHWC. 0:N--->batch ,1:H--->height ,2:W--->width, 3:C--->channel 
	TfLiteIntArray* input_dims = m_pInterpreter->tensor(input)->dims;
	return input_dims->data[1];
}

int ObjectDetModel::GetInputChannels()
{
	//Get the tensor index of first input layer 
	int input = m_pInterpreter->inputs()[0];

	//Tensorflow defatult tensor format NHWC. 0:N--->batch ,1:H--->height ,2:W--->width, 3:C--->channel 
	TfLiteIntArray* input_dims = m_pInterpreter->tensor(input)->dims;
	return input_dims->data[3];
}

unsigned int ObjectDetModel::GetNumberOfInputs()
{
	//Get the number of input layer 
	const std::vector<int> inputs = m_pInterpreter->inputs();
	return inputs.size();
}

unsigned int ObjectDetModel::GetNumberOfOutputs()
{
	//Get the number of output layer 
	const std::vector<int> outputs = m_pInterpreter->outputs();
	return outputs.size();
}

const char* ObjectDetModel::GetOutputName(int index)
{
	return m_pInterpreter->GetOutputName(index);
}

//for tflite object post-process operator
//index 0: 	detect score ?	shape:[1, num_boxes]
//index 1:	detect box location shape:[1, num_boxes, 4]
//index 2:	num boxes detect shape a float32 tensor of size 1
//index 3:  detect class ? shape:[1, num_boxes]

#define DETECT_SCORE_TENSOR_INDEX		0
#define DETECT_BOX_LOC_TENSOR_INDEX		1
#define NUM_BOX_DETECT_TENSOR_INDEX		2
#define DETECT_CLASS_TENSOR_INDEX		3

unsigned int ObjectDetModel::GetOutputSize(int index)
{
	int output = m_pInterpreter->outputs()[index];
	int i;
	TfLiteIntArray* output_dims = m_pInterpreter->tensor(output)->dims;
	// assume output dims to be something like (1, 1, ... ,size)
#if 0
	LOG(INFO) << "============= start =========" << endl; 

	for( i = 0; i < output_dims->size; i ++)
	{
		LOG(INFO) << i << ":" << output_dims->data[i] << endl;
	}

	LOG(INFO) << "============= end =========" << endl; 
#endif

	return output_dims->data[output_dims->size - 1];
}

float_t ObjectDetModel::GetNumBoxesDetect()
{
	float *outputFloat = m_pInterpreter->typed_output_tensor<float>(NUM_BOX_DETECT_TENSOR_INDEX);
	
	return *outputFloat;
}

bool ObjectDetModel::LoadImageIntoTensor(uint8_t* img)
{
	int input_height = GetInputHeight();
	int input_width = GetInputWidth();
	int input_channels = GetInputChannels();
	auto sizeInBytes = input_height * input_width * input_channels;		//RGB888 img

	//Get the tensor index of first input layer 
	int input = m_pInterpreter->inputs()[0];

	//pre-process data according data type
	if (m_tInputDatatype == kTfLiteFloat32) {
		float *in = m_pInterpreter->typed_tensor<float>(input);

		if(m_eModelType == eMODEL_TYPE_MOBILENET_SSD)
		{
			//Normalize to [-1,1]
			for (int i = 0; i < sizeInBytes; i++)
				in[i] = (img[i] - 127.5) * 0.0078125;
		}
		else
		{
			//Normalize to [0,1]
			for (int i = 0; i < sizeInBytes; i++)
				in[i] = img[i] / 255.0 ;
		}
	} 
	else if( m_tInputDatatype == kTfLiteUInt8) {
		uint8_t *in = m_pInterpreter->typed_tensor<uint8_t>(input);
		for (int i = 0; i < sizeInBytes; i++)
			in[i] = img[i];
	}
	else if( m_tInputDatatype == kTfLiteInt8) {
		int8_t *in = m_pInterpreter->typed_tensor<int8_t>(input);
		for (int i = 0; i < sizeInBytes; i++)
			in[i] = (int8_t) ((int32_t) (img[i]) - 128);
	}

	return true;
}


bool ObjectDetModel::ReadLabelsFile(const string &label_file,
				std::vector<S_LABEL_INFO> &LabelInfo,
				size_t* found_label_count)
{
	ifstream file(label_file);
	string delimiter = ":";

	if (!file) {
		LOG(FATAL) << "Labels file " << label_file << " not found\n";
		return false;
	}
	
	LabelInfo.clear();

	string line;
	size_t pos = 0;
	std::string token;		

	while (getline(file, line)) {
		//parse one line data

		S_LABEL_INFO sNewLabel;
		
		sNewLabel.szLabel.clear();
		sNewLabel.szLabel = line;

		LabelInfo.push_back(sNewLabel);
	}

	if(found_label_count)
		* found_label_count = LabelInfo.size();
	return true;
}

// Run inference
bool ObjectDetModel::RunInference()
{
	if(m_pInterpreter->Invoke() !=  kTfLiteOk)
		return false;
	return true;
}

/* tensorflow box output tensor encoding format: BoxCornerEncoding
struct BoxCornerEncoding {
  float ymin;
  float xmin;
  float ymax;
  float xmax;
};

struct CenterSizeEncoding {
  float y;
  float x;
  float h;
  float w;
};

*/

typedef struct {
  float ymin;
  float xmin;
  float ymax;
  float xmax;
}S_TF_BoxCornerEncoding ;

int ObjectDetModel::GetDetBoxes(std::vector<S_DETECT_BOX> &detBoxes, uint32_t srcImgWidth, uint32_t srcImgHeight)
{
	int i;

	detBoxes.clear();

	//mobilenet-ssd
	if(m_eModelType == eMODEL_TYPE_MOBILENET_SSD)
	{
		int numBoxes = GetNumBoxesDetect();
		float *scoreFloat = m_pInterpreter->typed_output_tensor<float>(DETECT_SCORE_TENSOR_INDEX);
		float *classFloat = m_pInterpreter->typed_output_tensor<float>(DETECT_CLASS_TENSOR_INDEX);
		S_TF_BoxCornerEncoding *psBoxEncoding = (S_TF_BoxCornerEncoding *) m_pInterpreter->typed_output_tensor<float>(DETECT_BOX_LOC_TENSOR_INDEX);

		for(i = 0; i < numBoxes; i ++)
		{
			if(*scoreFloat > m_fThreshold)
			{

				S_DETECT_BOX sNewBox;
				
				sNewBox.i32ClassIndex = (int32_t)*classFloat;
				sNewBox.fClassScores = *scoreFloat;

				sNewBox.sBoxPos.u32X = (uint32_t)(psBoxEncoding->xmin * srcImgWidth);
				sNewBox.sBoxPos.u32Y = (uint32_t)(psBoxEncoding->ymin * srcImgHeight);
				sNewBox.sBoxPos.u32Width = (uint32_t)((psBoxEncoding->xmax - psBoxEncoding->xmin) * srcImgWidth); 
				sNewBox.sBoxPos.u32Height = (uint32_t)((psBoxEncoding->ymax - psBoxEncoding->ymin) * srcImgHeight);

				detBoxes.push_back(sNewBox);

			}
			scoreFloat ++;
			classFloat ++;
			psBoxEncoding ++;
		}
		
		return numBoxes;
	}

	//Yolo
	std::vector<YoloDetectionResult> YoloResults;

	TfLiteTensor* modelOutput0 = m_pInterpreter->output_tensor(0);
	TfLiteTensor* modelOutput1 = m_pInterpreter->output_tensor(1);

	m_sYoloPP.RunPostProcessing(
		GetInputHeight(),
		GetInputWidth(),
		srcImgHeight,
		srcImgWidth,
		modelOutput0,
		modelOutput1,
		YoloResults);		
	
	for(i = 0; i < YoloResults.size(); i ++){

		S_DETECT_BOX sNewBox;
		
		sNewBox.i32ClassIndex = YoloResults[i].m_cls;
		sNewBox.fClassScores = YoloResults[i].m_normalisedVal;

		sNewBox.sBoxPos.u32X = YoloResults[i].m_x0;
		sNewBox.sBoxPos.u32Y = YoloResults[i].m_y0;
		sNewBox.sBoxPos.u32Width = YoloResults[i].m_w; 
		sNewBox.sBoxPos.u32Height = YoloResults[i].m_h;

		detBoxes.push_back(sNewBox);
	}

	YoloResults.clear();

	return detBoxes.size();
}




