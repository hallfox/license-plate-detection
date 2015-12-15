#ifndef _PROCESSING_H
#define _PROCESSING_H

#include <opencv2/core.hpp>

void plateBounds(const cv::Mat& src, cv::Mat& dst);
int textBinary(const cv::Mat& src, cv::Mat& dst);

#endif
