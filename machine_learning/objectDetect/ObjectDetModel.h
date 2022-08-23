#ifndef OBJECT_DET_MODEL_H
#define OBJECT_DET_MODEL_H


#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"

#include <cstdint>

using namespace std;
using namespace tflite;

typedef struct{
	uint32_t u32X;
	uint32_t u32Y;
	uint32_t u32Width;
	uint32_t u32Height;
}S_BOX_POS;

typedef struct{
	int i32ClassIndex;
	float fClassScores;
	S_BOX_POS sBoxPos;
}S_DETECT_BOX;

class ObjectDetModel {
public:
	/** @brief Constructor. */
	ObjectDetModel();
	/** @brief Destructor. */
	~ObjectDetModel();

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

	typedef struct {
		string szLabel;
	}S_LABEL_INFO;


	bool ReadLabelsFile(const string &label_file,
					std::vector<S_LABEL_INFO> &LabelInfo,
					size_t* found_label_count);


	bool LoadImageIntoTensor(uint8_t* img);
	bool RunInference();
	float_t GetNumBoxesDetect();
	int GetDetBoxes(std::vector<S_DETECT_BOX> &detBoxes, uint32_t srcImgWidth, uint32_t srcImgHeight);

	TfLiteType m_tInputDatatype;	//input data type kTfLiteFloat32, kTfLiteUInt8, kTfLiteInt8, ....
	TfLiteType m_tOutDatatype;		//output data type kTfLiteFloat32, kTfLiteUInt8, kTfLiteInt8, ....

private:

	unique_ptr<Interpreter> m_pInterpreter;
	unique_ptr<FlatBufferModel> m_pModel;
	bool m_bInited;        /* Indicates whether this object has been initialised. */
	TfLiteType	m_tType;
	int m_i32NumberOfThreads;
	float m_fThreshold;
};

#endif
