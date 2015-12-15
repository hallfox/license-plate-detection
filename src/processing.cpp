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

bool hasTextRatio(const std::vector<cv::Point>& contour, cv::Size imgSize) {
  cv::Rect box = cv::boundingRect(contour);
  double ratio = (double)box.width / box.height;

  if (ratio < 0.1 || ratio > 10) {
    return false;
  }
  int boxArea = box.width * box.height;
  int imgArea = imgSize.width * imgSize.height;
  return boxArea >= 15 && boxArea <= imgArea / 5;
}

// bool isConnected(const std::vector<cv::Point>& contour) {
//   contour[0]
// }

bool keep(const std::vector<cv::Point>& contour, cv::Size imgSize) {
  return hasTextRatio(contour, imgSize);
}

int countChildren(const std::vector<std::vector<cv::Point> >& contours,
                  const std::vector<cv::Vec4i>& hier,
                  int index,
                  cv::Size imgSize) {
  if (hier[index][2] < 0) {
    // no children
    return 0;
  }

  // Do I have a child?
  int child = hier[index][2];
  int count = 0;
  if (keep(contours[child], imgSize)) {
    count = 1;
  }

  // Count my children's children
  count += countChildren(contours, hier, child, imgSize);

  // Count siblings
  // Make sure it's the children's siblings
  for (int next = hier[child][0]; next > 0; next = hier[next][0]) {
    if (keep(contours[next], imgSize)) {
      count++;
    }
    count += countChildren(contours, hier, next, imgSize);
  }
  for (int prev = hier[child][0]; prev > 0; prev = hier[prev][0]) {
    if (keep(contours[prev], imgSize)) {
      count++;
    }
    count += countChildren(contours, hier, prev, imgSize);
  }

  return count;
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

  std::vector<cv::Rect> keptRegions;
  for (int i = 0; i < contours.size(); i++) {
    cv::Rect bound = cv::boundingRect(contours[i]);

    // find the parent contour, if there exists one
    int parent;
    for (parent = hierarchy[i][3];
         parent > 0 && keep(contours[parent], img.size());
         parent = hierarchy[parent][3]);

    int numChildren = countChildren(contours, hierarchy, i, img.size());
    if (keep(contours[i], img.size()) &&
        !(parent > 0 && numChildren <= 2) &&
        numChildren <= 2) {
      keptRegions.push_back(bound);
      cv::rectangle(img, bound, cv::Scalar(255, 0, 0));
    }
  }

  dst = img;
}
