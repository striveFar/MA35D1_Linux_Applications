#ifndef RECOGNITION_MODEL_H
#define RECOGNITION_MODEL_H


#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#include <cstdint>

using namespace std;
using namespace tflite;

class RecognitionModel {
public:
	/** @brief Constructor. */
	RecognitionModel();
	/** @brief Destructor. */
	~RecognitionModel();

	/** @brief      Initialise the model class object.
	 *  @param[in]  model_file specify the loation of model file
	 *  @return     true if initialisation succeeds, false otherwise.
	**/
	bool Init(string &model_file);

	int GetInputWidth();
	
	int GetInputHeight();
	
	int GetInputChannels();

	unsigned int GetNumberOfInputs();
	
	unsigned int GetNumberOfOutputs();
	
	unsigned int GetOutputSize(int index);

	bool LoadImageIntoTensor(uint8_t* img);

	typedef struct {
		string szLable;
		string szParam;		
	}S_LABEL_INFO;


	bool ReadLabelsFile(const string &label_file,
					std::vector<S_LABEL_INFO> &LabelInfo,
					size_t* found_label_count);

	bool RunInference();

	bool LoadOutputFromTensor(std::vector<float> &OutputArray);

	TfLiteType m_tInputDatatype;	//input data type kTfLiteFloat32, kTfLiteUInt8, kTfLiteInt8, ....
	TfLiteType m_tOutDatatype;		//output data type kTfLiteFloat32, kTfLiteUInt8, kTfLiteInt8, ....

private:
	unique_ptr<Interpreter> m_pInterpreter;
	unique_ptr<FlatBufferModel> m_pModel;
	bool m_bInited;        /* Indicates whether this object has been initialised. */
	TfLiteType	m_tType;
	int m_i32NumberOfThreads; 
};


#endif
