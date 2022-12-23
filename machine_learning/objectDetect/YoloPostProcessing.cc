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

#include "YoloPostProcessing.h"
#include "PlatformMath.h"

#include "YoloConfig.h"

const float anchor1[] = ANCHOR1;
const float anchor2[] = ANCHOR2;
const int numClasses = DEF_CLASSES;

static float Calculate1DOverlap(float x1Center, float width1, float x2Center, float width2)
{
	float left_1 = x1Center - width1/2;
	float left_2 = x2Center - width2/2;
	float leftest = left_1 > left_2 ? left_1 : left_2;

	float right_1 = x1Center + width1/2;
	float right_2 = x2Center + width2/2;
	float rightest = right_1 < right_2 ? right_1 : right_2;

	return rightest - leftest;
}

static float CalculateBoxIntersect(Box& box1, Box& box2)
{
	float width = Calculate1DOverlap(box1.x, box1.w, box2.x, box2.w);
	if (width < 0) {
		return 0;
	}
	float height = Calculate1DOverlap(box1.y, box1.h, box2.y, box2.h);
	if (height < 0) {
		return 0;
	}

	float total_area = width*height;
	return total_area;
}

static float CalculateBoxUnion(Box& box1, Box& box2)
{
	float boxes_intersection = CalculateBoxIntersect(box1, box2);
	float boxes_union = box1.w * box1.h + box2.w * box2.h - boxes_intersection;
	return boxes_union;
}

static float CalculateBoxIOU(Box& box1, Box& box2)
{
	float boxes_intersection = CalculateBoxIntersect(box1, box2);
	if (boxes_intersection == 0) {
		return 0;
	}

	float boxes_union = CalculateBoxUnion(box1, box2);
	if (boxes_union == 0) {
		return 0;
	}

	return boxes_intersection / boxes_union;
}

static void CalculateNMS(std::forward_list<Detection>& detections, int classes, float iouThreshold)
{
	int idxClass{0};
	auto CompareProbs = [idxClass](Detection& prob1, Detection& prob2) {
		return prob1.prob[idxClass] > prob2.prob[idxClass];
	};

	for (idxClass = 0; idxClass < classes; ++idxClass) {
		detections.sort(CompareProbs);

		for (auto it=detections.begin(); it != detections.end(); ++it) {
			if (it->prob[idxClass] == 0) continue;
			for (auto itc=std::next(it, 1); itc != detections.end(); ++itc) {
				if (itc->prob[idxClass] == 0) {
					continue;
				}
				if (CalculateBoxIOU(it->bbox, itc->bbox) > iouThreshold) {
					itc->prob[idxClass] = 0;
				}
			}
		}
	}
}

YoloPostprocessing::YoloPostprocessing(
    const float threshold,
    const float nms,
    int numClasses,
    int topN,
    TfLiteType outputTensorType)
    :   m_threshold(threshold),
        m_nms(nms),
        m_numClasses(numClasses),
        m_topN(topN),
        m_outputTensorType(outputTensorType)
{}

void YoloPostprocessing::RunPostProcessing(
    uint32_t imgNetRows,
    uint32_t imgNetCols,
    uint32_t imgSrcRows,
    uint32_t imgSrcCols,
    TfLiteTensor* modelOutput0,
    TfLiteTensor* modelOutput1,
    std::vector<YoloDetectionResult>& resultsOut)
{
    /* init postprocessing */
    Network net {
        .inputWidth = static_cast<int>(imgNetCols),
        .inputHeight = static_cast<int>(imgNetRows),
        .numClasses = m_numClasses,
        .branches = {
            Branch {
                .resolution = modelOutput0->dims->data[1],
                .numBox = ANCHOR_BOX,
                .anchor = anchor1,
                .modelOutput = modelOutput0->data.int8,
                .scale = ((TfLiteAffineQuantization*)(modelOutput0->quantization.params))->scale->data[0],
                .zeroPoint = ((TfLiteAffineQuantization*)(modelOutput0->quantization.params))->zero_point->data[0],
                .size = modelOutput0->bytes
            },
            Branch {
                .resolution = modelOutput1->dims->data[1],
                .numBox = ANCHOR_BOX,
                .anchor = anchor2,
                .modelOutput = modelOutput1->data.int8,
                .scale = ((TfLiteAffineQuantization*)(modelOutput1->quantization.params))->scale->data[0],
                .zeroPoint = ((TfLiteAffineQuantization*)(modelOutput1->quantization.params))->zero_point->data[0],
                .size = modelOutput1->bytes
            }
        },
        .topN = m_topN
    };

    /* End init */

	if(m_outputTensorType == kTfLiteFloat32)
	{
		net.branches[0].zeroPoint = 0.0;
		net.branches[0].scale = 1.0;
		net.branches[1].zeroPoint = 0.0;
		net.branches[1].scale = 1.0;
	}

    /* Start postprocessing */
    int originalImageWidth = imgSrcCols;
    int originalImageHeight = imgSrcRows;

    std::forward_list<Detection> detections;
    GetNetworkBoxes(net, originalImageWidth, originalImageHeight, m_threshold, detections);

    /* Do nms */
    CalculateNMS(detections, net.numClasses, m_nms);

    for (auto& it: detections) {
        float xMin = it.bbox.x - it.bbox.w / 2.0f;
        float xMax = it.bbox.x + it.bbox.w / 2.0f;
        float yMin = it.bbox.y - it.bbox.h / 2.0f;
        float yMax = it.bbox.y + it.bbox.h / 2.0f;

        if (xMin < 0) {
            xMin = 0;
        }
        if (yMin < 0) {
            yMin = 0;
        }
        if (xMax > originalImageWidth) {
            xMax = originalImageWidth;
        }
        if (yMax > originalImageHeight) {
            yMax = originalImageHeight;
        }

        float boxX = xMin;
        float boxY = yMin;
        float boxWidth = xMax - xMin;
        float boxHeight = yMax - yMin;

        for (int j = 0; j < net.numClasses; ++j) {

            if (it.prob[j] > 0) {

                YoloDetectionResult tmpResult = {};
                tmpResult.m_normalisedVal = it.prob[j];
                tmpResult.m_x0 = boxX;
                tmpResult.m_y0 = boxY;
                tmpResult.m_w = boxWidth;
                tmpResult.m_h = boxHeight;
				tmpResult.m_cls = j;

                resultsOut.push_back(tmpResult);
            }
        }
	}
}

void YoloPostprocessing::InsertTopNDetections(std::forward_list<Detection>& detections, Detection& det)
{
    std::forward_list<Detection>::iterator it;
    std::forward_list<Detection>::iterator last_it;
    for ( it = detections.begin(); it != detections.end(); ++it ) {
        if(it->objectness > det.objectness)
            break;
        last_it = it;
    }
    if(it != detections.begin()) {
        detections.emplace_after(last_it, det);
        detections.pop_front();
    }
}

void YoloPostprocessing::GetNetworkBoxes(Network& net, int imageWidth, int imageHeight, float threshold, std::forward_list<Detection>& detections)
{
    int numClasses = net.numClasses;
    int num = 0;
    auto det_objectness_comparator = [](Detection& pa, Detection& pb) {
        return pa.objectness < pb.objectness;
    };
    
    float bbox_obj;
    float bbox_x;
    float bbox_y;
    float bbox_w;
    float bbox_h;
    float bbox_scores;
    
    float *pfModelOutput;
    int8_t *pi8ModelOutput;
    
    for (size_t i = 0; i < net.branches.size(); ++i) {
        int height   = net.branches[i].resolution;
        int width    = net.branches[i].resolution;
        int channel  = net.branches[i].numBox*(5+numClasses);

		pfModelOutput = NULL;
		pi8ModelOutput = NULL;

		if(m_outputTensorType == kTfLiteFloat32)
			pfModelOutput = (float *)net.branches[i].modelOutput;
		else if(m_outputTensorType == kTfLiteInt8)
			pi8ModelOutput = (int8_t *)net.branches[i].modelOutput;

        for (int h = 0; h < net.branches[i].resolution; h++) {
            for (int w = 0; w < net.branches[i].resolution; w++) {
                for (int anc = 0; anc < net.branches[i].numBox; anc++) {

                    /* Objectness score */
                    int bbox_obj_offset = h * width * channel + w * channel + anc * (numClasses + 5) + 4;

					if(pfModelOutput)
					{
						bbox_obj = pfModelOutput[bbox_obj_offset];
					}
					else if(pi8ModelOutput)
					{
						int8_t *pi8ModelOutput = (int8_t *)net.branches[i].modelOutput;
						bbox_obj = (float)pi8ModelOutput[bbox_obj_offset];
					}

					float objectness = MathUtils::SigmoidF32(
                            (static_cast<float>(bbox_obj)
                            - net.branches[i].zeroPoint
                            ) * net.branches[i].scale);

                    if(objectness > threshold) {
                        Detection det;
                        det.objectness = objectness;
                        /* Get bbox prediction data for each anchor, each feature point */
                        int bbox_x_offset = bbox_obj_offset -4;
                        int bbox_y_offset = bbox_x_offset + 1;
                        int bbox_w_offset = bbox_x_offset + 2;
                        int bbox_h_offset = bbox_x_offset + 3;
                        int bbox_scores_offset = bbox_x_offset + 5;

						if(pfModelOutput)
						{
							bbox_x = pfModelOutput[bbox_x_offset];
							bbox_y = pfModelOutput[bbox_y_offset];
							bbox_w = pfModelOutput[bbox_w_offset];
							bbox_h = pfModelOutput[bbox_h_offset];
						}
						else if(pi8ModelOutput)
						{
							bbox_x = (float)pi8ModelOutput[bbox_x_offset];
							bbox_y = (float)pi8ModelOutput[bbox_y_offset];
							bbox_w = (float)pi8ModelOutput[bbox_w_offset];
							bbox_h = (float)pi8ModelOutput[bbox_h_offset];
						}

                        det.bbox.x = (static_cast<float>(bbox_x) - net.branches[i].zeroPoint) * net.branches[i].scale;
                        det.bbox.y = (static_cast<float>(bbox_y) - net.branches[i].zeroPoint) * net.branches[i].scale;
                        det.bbox.w = (static_cast<float>(bbox_w) - net.branches[i].zeroPoint) * net.branches[i].scale;
                        det.bbox.h = (static_cast<float>(bbox_h) - net.branches[i].zeroPoint) * net.branches[i].scale;

                        float bbox_x, bbox_y;

                        /* Eliminate grid sensitivity trick involved in YOLOv4 */
                        bbox_x = MathUtils::SigmoidF32(det.bbox.x);
                        bbox_y = MathUtils::SigmoidF32(det.bbox.y);
                        det.bbox.x = (bbox_x + w) / width;
                        det.bbox.y = (bbox_y + h) / height;

                        det.bbox.w = std::exp(det.bbox.w) * net.branches[i].anchor[anc*2] / net.inputWidth;
                        det.bbox.h = std::exp(det.bbox.h) * net.branches[i].anchor[anc*2+1] / net.inputHeight;

                        for (int s = 0; s < numClasses; s++) {
							if(pfModelOutput)
							{
								bbox_scores = pfModelOutput[bbox_scores_offset + s];
							}
							else if(pi8ModelOutput)
							{
								bbox_scores = (float)pi8ModelOutput[bbox_scores_offset + s];
							}

                            float sig = MathUtils::SigmoidF32(
                                    (static_cast<float>(bbox_scores) -
                                    net.branches[i].zeroPoint) * net.branches[i].scale
                                    ) * objectness;

                            det.prob.emplace_back((sig > threshold) ? sig : 0);
                        }

                        /* Correct_YOLO_boxes */
                        det.bbox.x *= imageWidth;
                        det.bbox.w *= imageWidth;
                        det.bbox.y *= imageHeight;
                        det.bbox.h *= imageHeight;

                        if (num < net.topN || net.topN <=0) {
                            detections.emplace_front(det);
                            num += 1;
                        } else if (num == net.topN) {
                            detections.sort(det_objectness_comparator);
                            InsertTopNDetections(detections,det);
                            num += 1;
                        } else {
                            InsertTopNDetections(detections,det);
                        }
                    }
                }
            }
        }
    }
    if(num > net.topN)
        num -=1;
}
