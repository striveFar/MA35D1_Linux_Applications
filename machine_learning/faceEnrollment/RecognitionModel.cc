
#include <iostream>
#include <fstream> 

#include "RecognitionModel.h"

#define LOG(x) std::cout

/* Initialise the model */
RecognitionModel::RecognitionModel() :
    m_bInited (false),
    m_tType(kTfLiteNoType),
    m_pInterpreter(nullptr),
    m_pModel(nullptr),
    m_i32NumberOfThreads(-1)
{

}

RecognitionModel::~RecognitionModel()
{
    
    /**
     * No clean-up function available for allocator in TensorFlow Lite Micro yet.
     **/
}

bool RecognitionModel::Init(string &model_file)
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

int RecognitionModel::GetInputWidth()
{
	//Get the tensor index of first input layer 
	int input = m_pInterpreter->inputs()[0];

	//Tensorflow defatult tensor format NHWC. 0:N--->batch ,1:H--->height ,2:W--->width, 3:C--->channel 
	TfLiteIntArray* input_dims = m_pInterpreter->tensor(input)->dims;
	return input_dims->data[2];
}

int RecognitionModel::GetInputHeight()
{
	//Get the tensor index of first input layer 
	int input = m_pInterpreter->inputs()[0];

	//Tensorflow defatult tensor format NHWC. 0:N--->batch ,1:H--->height ,2:W--->width, 3:C--->channel 
	TfLiteIntArray* input_dims = m_pInterpreter->tensor(input)->dims;
	return input_dims->data[1];
}

int RecognitionModel::GetInputChannels()
{
	//Get the tensor index of first input layer 
	int input = m_pInterpreter->inputs()[0];

	//Tensorflow defatult tensor format NHWC. 0:N--->batch ,1:H--->height ,2:W--->width, 3:C--->channel 
	TfLiteIntArray* input_dims = m_pInterpreter->tensor(input)->dims;
	return input_dims->data[3];
}

unsigned int RecognitionModel::GetNumberOfInputs()
{
	//Get the number of input layer 
	const std::vector<int> inputs = m_pInterpreter->inputs();
	return inputs.size();
}

unsigned int RecognitionModel::GetNumberOfOutputs()
{
	//Get the number of output layer 
	const std::vector<int> outputs = m_pInterpreter->outputs();
	return outputs.size();
}

unsigned int RecognitionModel::GetOutputSize(int index)
{
	int output = m_pInterpreter->outputs()[index];
	TfLiteIntArray* output_dims = m_pInterpreter->tensor(output)->dims;
	// assume output dims to be something like (1, 1, ... ,size)
	return output_dims->data[output_dims->size - 1];
}

bool RecognitionModel::LoadImageIntoTensor(uint8_t* img)
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

		//Normalize to [-1,1]
		for (int i = 0; i < sizeInBytes; i++)
			in[i] = (img[i] - 127.5) * 0.0078125;
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

/* label file format
 * label1:param1:
 * label2:param2:
 * label3:param3:
*/

bool RecognitionModel::ReadLabelsFile(const string &label_file,
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
		
		pos = 0;
		sNewLabel.szLable.clear();
		sNewLabel.szParam.clear();

		while ((pos = line.find(delimiter)) != string::npos) {
			token = line.substr(0, pos);

			if(sNewLabel.szLable.size() == 0)		//Get label string
				sNewLabel.szLable = token;
			else if(sNewLabel.szParam.size() == 0)	//Get param string
				sNewLabel.szParam = token;
			
			line.erase(0, pos + delimiter.length());
		}

		LabelInfo.push_back(sNewLabel);
	}

	if(found_label_count)
		* found_label_count = LabelInfo.size();
	return true;
}

// Run inference
bool RecognitionModel::RunInference()
{
	if(m_pInterpreter->Invoke() !=  kTfLiteOk)
		return false;
	return true;
}

// Copy inference result to output array
bool RecognitionModel::LoadOutputFromTensor(std::vector<float> &OutputArray)
{
	int i;
	TfLiteTensor *outputTensor = m_pInterpreter->output_tensor(0);

	switch(m_tOutDatatype)
	{
		case kTfLiteFloat32:
		{
			float *outputFloat = m_pInterpreter->typed_output_tensor<float>(0);
			for(i = 0; i < OutputArray.size(); i++)
			{
				OutputArray[i] = outputFloat[i];
			}
		}
		break;
		case kTfLiteUInt8:
		{
			uint8_t *outputUInt8 = m_pInterpreter->typed_output_tensor<uint8_t>(0);
			for(i = 0; i < OutputArray.size(); i++)
			{
				OutputArray[i] = outputUInt8[i];
			}
		}
		break;
		case kTfLiteInt8:
		{
			int8_t *outputInt8 = m_pInterpreter->typed_output_tensor<int8_t>(0);
			for(i = 0; i < OutputArray.size(); i++)
			{
				OutputArray[i] = outputTensor->params.scale * (static_cast<float>(outputInt8[i]) - outputTensor->params.zero_point);
			}
		}
		break;
		default:
		{
			LOG(FATAL) << "Unknown output data type \n";
			return false;
		}
	}

	return true;
}


