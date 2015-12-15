#ifndef _DETECTION_H
#define _DETECTION_H

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <vector>
#include "stdlib.h"

void fillHoles(cv::Mat &image);
bool vertifySizes(cv::RotatedRect mr);
void detection(cv::Mat &src, std::vector<cv::Mat> &potentialRegions, cv::Mat &dst);

#endif
