#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <vector>

bool verifySizes(cv::RotatedRect mr){
    float error=0.4;
    //NY plate width: 32 height: 17 aspect ratio (width/height): 32/17
    float aspect= 32 / 17.0;
    //Set a min and max area. All other patchs are discarded
    int min= 15*aspect*15; // minimum area
    int max= 125*aspect*125; // maximum area
    //Get only patchs that match to a respect ratio.
    float rmin= aspect-aspect*error;
    float rmax= aspect+aspect*error;
    int area= mr.size.height * mr.size.width;
    float r= (float)mr.size.width / (float)mr.size.height;
    if(r<1)
        r= (float)mr.size.height / (float)mr.size.width;
    std::cout << "min: " << min << ", max: " << max << ", rmin: " << rmin << ", rmax: " << rmax << ", area: " << area << ", r: " << r;
    if(( area < min || area > max ) || ( r < rmin || r > rmax )){
        std::cout << ", false" << std::endl;
        return false;
    }else{
        std::cout << ", true" << std::endl;
        return true;
    }
}

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
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::RotatedRect> rects;
    std::vector<std::vector<cv::Point> >::iterator itc= contours.begin();
    cv::Point2f vertices[4];
    double minVal = 100000, maxVal = -1;
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
        cv::Canny(modified_image, modified_image, canny_min_thresh, canny_min_thresh*canny_ratio);
        break;
/*
      case '4':
        cv::Sobel(modified_image, sobelx, CV_32F, 1, 0);
        cv::minMaxLoc(sobelx, &minVal, &maxVal); //find minimum and maximum intensities
        std::cout << "minVal : " << minVal << std::endl << "maxVal : " << maxVal << std::endl;
        sobelx.convertTo(modified_image, CV_8U, 255.0/(maxVal - minVal), -minVal * 255.0/(maxVal - minVal));
        break;
      case '5':
        cv::Sobel(modified_image, sobely, CV_32F, 0, 1);
        cv::minMaxLoc(sobely, &minVal, &maxVal); //find minimum and maximum intensities
        std::cout << "minVal : " << minVal << std::endl << "maxVal : " << maxVal << std::endl;
        sobely.convertTo(modified_image, CV_8U, 255.0/(maxVal - minVal), -minVal * 255.0/(maxVal - minVal));
        break;
*/
      case '4':
        /* perform Sobel operator for edge detection */
        cv::Sobel(modified_image, modified_image, CV_8U, 1, 0, 3, 1, 0);
        break;
      case '5':
        /* convert image to binary */
        threshold (modified_image, modified_image, 0, 255, CV_THRESH_OTSU+CV_THRESH_BINARY); 
        break;
      case '6':
        /* close morphology operation */
        /* structuring element size: width 20, height 3 */
        element = getStructuringElement(cv::MORPH_RECT, cv::Size(20, 3));
        morphologyEx(modified_image, modified_image, CV_MOP_CLOSE, element);
        break;
      case '7':
        /* find contours and eliminated false regions */
        findContours(modified_image,
                contours, // a vector of contours
                CV_RETR_EXTERNAL, // retrieve the external contours
                CV_CHAIN_APPROX_NONE); // all pixels of each contour

        /* draw contours */
        //Start to iterate to each contour found
        itc= contours.begin();
        //Remove patch that has no inside limits of aspect ratio and area.
        while (itc!=contours.end()) {
            //Create bounding rect of object
            cv::RotatedRect mr= minAreaRect(cv::Mat(*itc));
            if (!verifySizes(mr)){
                itc= contours.erase(itc);
            }else{
                ++itc;
                rects.push_back(mr);
            }
        }
        /* draw rectangles to the original image */
        original_image.copyTo(modified_image);
        for(int j = 0; j < rects.size(); ++j){
            rects[j].points(vertices);
            for (int i = 0; i < 4; i++)
                line(modified_image, vertices[i], vertices[(i+1)%4], cv::Scalar(0,255,0));
        }
      break;
      default:
        break;
      }
    }

    // Cleanup
    cv::destroyAllWindows();
}
