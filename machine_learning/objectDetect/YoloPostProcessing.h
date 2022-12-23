/*
 * Copyright (c) 2022 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef YOLO_POST_PROCESSING_HPP
#define YOLO_POST_PROCESSING_HPP

#include <cstdio>
#include <vector>
#include <forward_list>
#include <cstdint>

#include "tensorflow/lite/interpreter.h"
//#include "tensorflow/lite/kernels/register.h"
//#include "tensorflow/lite/model.h"

#include "YoloDetectionResult.h"

struct Branch {
	int resolution;
	int numBox;
	const float* anchor;
	void* modelOutput;
	float scale;
	int zeroPoint;
	size_t size;
};

struct Network {
	int inputWidth;
	int inputHeight;
	int numClasses;
	std::vector<Branch> branches;
	int topN;
};

/**
 * Contains the x,y co-ordinates of a box centre along with the box width and height.
 */
struct Box {
	float x;
	float y;
	float w;
	float h;
};

struct Detection {
	Box bbox;
	std::vector<float> prob;
	float objectness;
};

/**
 * @brief   Helper class to manage tensor post-processing for "object_detection"
 *          output.
 */
class YoloPostprocessing {
public:
	/**
	 * @brief       Constructor.
	 * @param[in]   threshold     Post-processing threshold.
	 * @param[in]   nms           Non-maximum Suppression threshold.
	 * @param[in]   numClasses    Number of classes.
	 * @param[in]   topN          Top N for each class.
	 **/
	explicit YoloPostprocessing(float threshold = 0.5f,
									float nms = 0.45f,
									int numClasses = 1,
									int topN = 0,
									TfLiteType outputTensorType = kTfLiteFloat32);

	/**
	 * @brief       Post processing part of YOLO object detection CNN.
	 * @param[in]   imgNetRows      Number of rows in the network input image.
	 * @param[in]   imgNetCols      Number of columns in the network input image.
	 * @param[in]   imgSrcRows      Number of rows in the orignal input image.
	 * @param[in]   imgSrcCols      Number of columns in the oringal input image.
	 * @param[in]   modelOutput  Output tensors after CNN invoked.
	 * @param[out]  resultsOut   Vector of detected results.
	 **/
	void RunPostProcessing(uint32_t imgNetRows,
						   uint32_t imgNetCols,
						   uint32_t imgSrcRows,
						   uint32_t imgSrcCols,
						   TfLiteTensor* modelOutput0,
						   TfLiteTensor* modelOutput1,
						   std::vector<YoloDetectionResult>& resultsOut);

private:
	float m_threshold;  /* Post-processing threshold */
	float m_nms;        /* NMS threshold */
	int   m_numClasses; /* Number of classes */
	int   m_topN;       /* TopN */
	TfLiteType m_outputTensorType; /* output tensor data type */
	/**
	 * @brief       Insert the given Detection in the list.
	 * @param[in]   detections   List of detections.
	 * @param[in]   det          Detection to be inserted.
	 **/
	void InsertTopNDetections(std::forward_list<Detection>& detections, Detection& det);

	/**
	 * @brief        Given a Network calculate the detection boxes.
	 * @param[in]    net           Network.
	 * @param[in]    imageWidth    Original image width.
	 * @param[in]    imageHeight   Original image height.
	 * @param[in]    threshold     Detections threshold.
	 * @param[out]   detections    Detection boxes.
	 **/
	void GetNetworkBoxes(Network& net,
						 int imageWidth,
						 int imageHeight,
						 float threshold,
						 std::forward_list<Detection>& detections);

};

#endif
