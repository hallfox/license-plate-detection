#include "processing.hpp"

#include <opencv2/imgproc.hpp>
#include <iostream>
#include <algorithm>

uchar intensity(const cv::Mat& img, cv::Point loc) {
  if (loc.x < 0 || loc.x >= img.cols || loc.y < 0 || loc.y >= img.rows) {
    return 0;
  }
  cv::Vec3b pix = img.at<cv::Vec3b>(loc);
  return static_cast<uchar>( 0.3 * pix[2] + 0.59 * pix[1] + 0.11 * pix[0] );
}

bool hasTextRatio(const std::vector<cv::Point>& contour, cv::Size imgSize) {
  cv::Rect box = cv::boundingRect(contour);
  double ratio = static_cast<double>(box.width) / box.height;

  if (ratio > 1 || ratio < 0.1) {
    return false;
  }
  double boxArea = box.width * box.height;
  double imgArea = imgSize.width * imgSize.height;
  return boxArea >= 100 && boxArea <= imgArea / 5;
}

bool isConnected(const std::vector<cv::Point>& contour) {
  cv::Point first = contour.front(), last = contour.back();
  return std::abs(first.x - last.x) <= 1 && std::abs(first.y - last.y) <= 1;
}

bool keep(const std::vector<cv::Point>& contour, cv::Size imgSize) {
  return hasTextRatio(contour, imgSize) && isConnected(contour);
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

  // Count siblings
  // Make sure it's the children's siblings
  for (int next = hier[child][0]; next > 0; next = hier[next][0]) {
    if (keep(contours[next], imgSize)) {
      count++;
    }
    // count += countChildren(contours, hier, next, imgSize);
  }
  for (int prev = hier[child][1]; prev > 0; prev = hier[prev][1]) {
    if (keep(contours[prev], imgSize)) {
      count++;
    }
    // count += countChildren(contours, hier, prev, imgSize);
  }

  // Count my children's children
  // count += countChildren(contours, hier, child, imgSize);


  return count;
}

void textBinary(const cv::Mat& src, cv::Mat& dst, int *numKept) {
  // Get edge image
  cv::Mat img, edges;

  img = src.clone();

  cv::Mat chans[3];
  cv::split(img, chans);
  for (int i = 0; i < 3; i++) {
    cv::Canny(chans[i], chans[i], 200, 250);
  }
  edges = chans[0] | chans[1] | chans[2];

  // Find the contours and filter based on its size and connectedness
  std::vector<std::vector<cv::Point> > contours;
  std::vector<cv::Vec4i> hierarchy;
  cv::findContours(edges, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

  std::vector<std::pair<std::vector<cv::Point>, cv::Rect> > keptRegions;
  for (int i = 0; i < contours.size(); i++) {
    cv::Rect bound = cv::boundingRect(contours[i]);
    // cv::rectangle(edges, bound, cv::Scalar(255, 0, 0));
    std::cout << i << ": ";

    // find the parent contour, if there exists one
    int parent;
    for (parent = hierarchy[i][3];
         parent > 0 && !keep(contours[parent], edges.size());
         parent = hierarchy[parent][3]);

    int numChildren = countChildren(contours, hierarchy, i, edges.size());
    if (keep(contours[i], edges.size()) &&
        !(parent > 0 && numChildren <= 2) &&
        numChildren <= 2) {
      cv::rectangle(edges, bound, cv::Scalar(255, 0, 0));
      cv::putText(edges, std::to_string(i), bound.tl(), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255));
      keptRegions.push_back(std::make_pair(contours[i], bound));
      std::cout << "Region : parent = " << parent << "; numChildren = " << numChildren << "\n";
    }
    else {
      // cv::rectangle(edges, bound, cv::Scalar(255, 0, 0));
      // cv::putText(edges, std::to_string(i), bound.tl(), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255));
      if (!keep(contours[i], edges.size())) {
        std::cout << "Size not right/not connected\n";
      }
      if((parent > 0 && numChildren <= 2)) {
        std::cout << "Internal contour\n";
      }
      if (numChildren > 2) {

        std::cout << "too many children: " << numChildren << "\n";
      }
    }
  }

  if (numKept != nullptr) {
    *numKept = keptRegions.size();
  }

  cv::Mat filter(edges.size(), CV_8U, cv::Scalar(255));

  for (auto&& p: keptRegions) {
    auto contour = p.first;
    auto box = p.second;

    // Foreground thresholding
    double fgThresh = 0;
    for (auto&& point: contour) {
      fgThresh += intensity(img, point);
    }
    fgThresh /= contour.size();

    // Background thresholding
    double bgThresh;
    std::vector<uchar> bgs = {
      // Top left
      intensity(src, cv::Point(box.x - 1, box.y - 1)),
      intensity(src, cv::Point(box.x - 1, box.y)),
      intensity(src, cv::Point(box.x, box.y - 1)),
      // Top right
      intensity(src, cv::Point(box.x + box.width + 1, box.y - 1)),
      intensity(src, cv::Point(box.x + box.width + 1, box.y)),
      intensity(src, cv::Point(box.x + box.width, box.y - 1)),
      // Bottom left
      intensity(src, cv::Point(box.x - 1, box.y + box.height + 1)),
      intensity(src, cv::Point(box.x - 1, box.y + box.height)),
      intensity(src, cv::Point(box.x, box.y + box.height + 1)),
      // Bottom right
      intensity(src, cv::Point(box.x + box.width + 1, box.y + box.height + 1)),
      intensity(src, cv::Point(box.x + box.width, box.y + box.height + 1)),
      intensity(src, cv::Point(box.x + box.width + 1, box.y + box.height)),
    };
    // Find median for bg thresh
    int len = bgs.size();
    std::nth_element(bgs.begin(), bgs.begin() + len/2, bgs.end());
    std::nth_element(bgs.begin(), bgs.begin() + len/2 + 1, bgs.end());
    bgThresh = ( bgs[len/2] + bgs[len/2 + 1] ) / 2.0;

    // Adjust colors for thresholds
    uchar fg, bg;
    // fg = 0;
    // bg = 255;
    if (fgThresh >= bgThresh) {
      fg = 255;
      bg = 0;
    }
    else {
      fg = 0;
      bg = 255;
    }

    // fill in the box
    for (int x = box.x; x < box.x + box.width; x++) {
      for (int y = box.y; y < box.y + box.height; y++) {
        if (x >= 0 && x < img.cols && y >= 0 && y < img.rows) {
          cv::Point p(x, y);
          if (intensity(img, p) > fgThresh) {
            filter.at<uchar>(p) = bg;
          }
          else {
            filter.at<uchar>(p) = fg;
          }
        }
      }
    }
  }

  cv::blur(filter, filter, cv::Size(2, 2));

  dst = filter;
}
