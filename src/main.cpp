#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <vector>
#include "processing.hpp"
#include "detection.hpp"


int main(int argc, char **argv) {
    if(argc != 2) {
        std::cout << "USAGE: skeleton <input file path>" << std::endl;
        return -1;
    }

    //Load two copies of the image. One to leave as the original, and one to be modified.
    //Done for display purposes only
    cv::Mat original_image = cv::imread(argv[1]);//, IMREAD_GRAYSCALE);
    cv::Mat modified_image = original_image.clone();

    //Check that the images loaded
    if(!original_image.data || !modified_image.data) {
        std::cout << "ERROR: Could not load image data." << std::endl;
        return -1;
    }

    // Window variables
    std::string window_name = "License Plate Detector";
    std::string canny_trackbar_name = "Canny Min Threshold";
    std::string gauss_trackbar_name = "Gaussian Kernel Size";

    const int trackbar_thresh_max = 100;
    int canny_min_thresh = 0;
    int canny_ratio = 3;
    int gauss_kernel_size = 5;

    //Create the display window
    cv::namedWindow(window_name);

    // Trackbars for variable customizations
    cv::createTrackbar(canny_trackbar_name, window_name, &canny_min_thresh,
                       trackbar_thresh_max);
    cv::createTrackbar(gauss_trackbar_name, window_name, &gauss_kernel_size,
                       trackbar_thresh_max);

    //Display loop
    bool loop = true;
    cv::Mat sobelx, sobely, sobel, element;
    std::vector<cv::Mat> potentialRegions;
    int maxContours = -1;
    int trueLocation = -1;
    while(loop) {
      imshow(window_name, modified_image);

      char c = cvWaitKey(15);
      switch(c) {
      case 27:  //Exit display loop if ESC is pressed
        loop = false;
        break;
      case ' ':
        original_image.copyTo(modified_image);
        break;
      case '1':
        cv::cvtColor(modified_image, modified_image, cv::COLOR_BGR2GRAY);
        break;
      case '2':
        cv::GaussianBlur(modified_image, modified_image, cv::Size(gauss_kernel_size, gauss_kernel_size), 0);
        break;
      case '3':
        std::cout << "canny_min_thresh: " << canny_min_thresh << std::endl;
        cv::Canny(modified_image, modified_image, canny_min_thresh, canny_min_thresh*canny_ratio);
        break;

      case 'b':
        maxContours = textBinary(modified_image, modified_image);
        break;
      case 'a':
        detection(modified_image, potentialRegions, modified_image);
        break;
      case 'c':
        //Detect potential plate regions
        detection(modified_image, potentialRegions, modified_image);
        for(int i = 0; i < potentialRegions.size(); ++i){
            //number of contours of each potential region
            int tmp = dummy(potentialRegions[i], modified_image);
            std::cout << "number of contours: " << tmp;
            if(tmp > maxContours){
                maxContours = tmp;
                trueLocation = i;
            }

            imshow(window_name, potentialRegions[i]);
            std::cout << " at region " << i+1 << std::endl;
            sleep(5);
        }
        /* draw rectangles to the original image */
        original_image.copyTo(modified_image);
        for(int j = 0; j < rects.size(); ++j){
            rects[j].points(vertices);
            for (int i = 0; i < 4; i++)
                line(modified_image, vertices[i], vertices[(i+1)%4], cv::Scalar(0,255,0));
        }
      break;
      case 'b':
        textBinary(modified_image, modified_image);
        cv::imwrite("output.png", modified_image);
        break;
      default:
        break;
      }
    }

    // Cleanup
    cv::destroyAllWindows();

    return 0;
}
