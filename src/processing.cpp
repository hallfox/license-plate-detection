#include "processing.hpp"

#include <opencv2/imgproc.hpp>

void plateBounds(const cv::Mat& src, cv::Mat& dst) {
  cv::Mat img = src.clone();

  // Threshold
  cv::threshold(img, img, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

  // Structuring
  cv::Mat rect = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 3));
  cv::erode(img, img, rect);

  // Find what's left
  std::vector<cv::Point> points;
  for (auto it = img.begin<uchar>(); it != img.end<uchar>(); it++) {
    if (*it) {
      points.push_back(it.pos());
    }
  }

  // Draw a rectangle around the remaining points
  cv::RotatedRect box = cv::minAreaRect(points);
  cv::Point2f vertices[4];
  box.points(vertices);
  for (int i = 0; i < 4; i++) {
    cv::line(img, vertices[i], vertices[(i+1)%4], cv::Scalar(255, 0, 0), CV_AA);
  }

  dst = img;
}

bool hasTextRatio(const cv::Rect& bound) {
  
}

bool keep(const std::vector<cv::Point>& contour) {
  return hasTextRatio(contour) && isConnected(contours);
}

void textBinary(const cv::Mat& src, cv::Mat& dst) {
  // Get edge image
  cv::Mat img;
  cv::Mat chans[3];
  cv::split(src, chans);
  for (int i = 0; i < 3; i++) {
    cv::Canny(chans[i], chans[i], 200, 250);
  }
  img = chans[0] | chans[1] | chans[2];

  // Find the contours and filter based on its size and connectedness
  std::vector<std::vector<cv::Point> > contours;
  std::vector<cv::Vec4i> hierarchy;
  cv::findContours(img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

  for (int i = 0; i < contours.size(); i++) {
    cv::Rect bound = cv::boundingRect(contours[i]);

  }

  dst = img;
}
