#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <vector>
#include "stdlib.h"

/* fill enclosed regions */
void fillHoles(cv::Mat &image)
{
    cv::Mat holes = image.clone();
    cv::floodFill(holes,cv::Point2i(0,0), CV_RGB(255, 255, 255));
    bitwise_not(holes, holes);
    image = (image | holes);
}

/* Check whether the contour is in approprieta size range*/
bool verifySizes(cv::RotatedRect mr){
    float error=0.4;
    //NY plate width: 32 height: 17 aspect ratio (width/height): 32/17
    float aspect= 32 / 17.0;
    //Set a min and max area. All other patchs are discarded
    int min= 30*aspect*30; // minimum area
    int max= 125*aspect*125; // maximum area
    //Get only patchs that match to a respect ratio.
    float rmin= aspect-aspect*error;
    float rmax= aspect+aspect*error;
    int area= mr.size.height * mr.size.width;
    float r= (float)mr.size.width / (float)mr.size.height;
    if(r<1)
        r= (float)mr.size.height / (float)mr.size.width;
//    std::cout << "min: " << min << ", max: " << max << ", rmin: " << rmin << ", rmax: " << rmax << ", area: " << area << ", r: " << r;
    if(( area < min || area > max ) || ( r < rmin || r > rmax )){
        return false;
    }else{
        return true;
    }
}

/**
 * Take in a source image and produce a dst image with rectangles surrounding the interest regions
 * The program also generates a vector of the cropped interest regions
 */
void detection(cv::Mat &src, std::vector<cv::Mat> &potentialRegions, cv::Mat &dst){
   cv::Mat result = src.clone();
   
   cv::Mat sobel, element;
   std::vector<std::vector<cv::Point>> contours;
   std::vector<cv::RotatedRect> rects;
   std::vector<std::vector<cv::Point> >::iterator itc;
   cv::Point2f vertices[4];
   
   //convert to grayscale
   cv::cvtColor(result, result, cv::COLOR_BGR2GRAY);
   //blur the image for better edge detection
   cv::GaussianBlur(result, result, cv::Size(5, 5), 0);  
   //Sobel operator
   cv::Sobel(result, result, CV_8U, 1, 0, 3, 1, 0);
   //convert to binary
   threshold(result, result, 0, 255, CV_THRESH_OTSU+CV_THRESH_BINARY);
   // close morphology operation
   // structuring element size: width 17, height 3 
   element = getStructuringElement(cv::MORPH_RECT, cv::Size(17, 3));
   morphologyEx(result, result, CV_MOP_CLOSE, element);
   // find contours and eliminated false regions
   findContours(result,
                contours, // a vector of contours
                CV_RETR_EXTERNAL, // retrieve the external contours
                CV_CHAIN_APPROX_NONE); // all pixels of each contour
   // draw contours
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
   src.copyTo(result);
   //produce canny edge image
   //cv::cvtColor(result, result, cv::COLOR_BGR2GRAY);
   //cv::GaussianBlur(result, result, cv::Size(5, 5), 0);
   //cv::Canny(result, result, 80, 240);


   //Create an vector of cropped interest regions
   std::cout << "rows: " << result.rows << ", cols: " << result.cols << std::endl;

   //draw rectangles to the original image
   for(int j = 0; j < rects.size(); ++j){
     rects[j].points(vertices);
     std::cout << "points: " << vertices[0] << ", " << vertices[1] << ", " << vertices[2] << ", " << vertices[3] << std::endl;
     //Rect object to store crop region
     for (int i = 0; i < 4; i++){
        line(result, vertices[i], vertices[(i+1)%4], cv::Scalar(255, 255, 255));
     }
   }
   
   std::vector<cv::Mat> ROI;
   for(int i = 0; i < rects.size(); ++i){
        bool valid = true;
        rects[i].points(vertices);
        for(int j = 0; j < 4; ++j){
            if(vertices[j].x < 0 || vertices[j].x >= result.cols || vertices[j].y < 0 || vertices[j].y >= result.rows){
                valid = false;
                break;
            }
        }
        if(valid){
            cv::Rect cropRegion = rects[i].boundingRect();
            ROI.push_back(*(new cv::Mat(result, cropRegion)));
        }
   }
    
    potentialRegions = ROI;
   result.copyTo(dst);
}

// The method is suppose to determine whether a region contains text to remove false plate regions
/*
void detection(cv::Mat &src, cv::Mat &des){
    cv::Mat large = src.clone();
    cv::Mat rgb;
    std::string wn = "tmp";
    cv::namedWindow(wn); 
    //reduce resolution on image
    pyrDown(large, rgb);
    cv::Mat small;
//    pyrUp(result, result, cv::Size(result.cols*2, result.rows*2));
    //convert to grayscale
    cvtColor(rgb, small, CV_BGR2GRAY);
    cv::Mat grad;
    cv::Mat morphKernel = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    morphologyEx(small, grad, cv::MORPH_GRADIENT, morphKernel);
    // binarize
    cv::Mat bw;
    threshold(grad, bw, 0.0, 255.0, cv::THRESH_BINARY | cv::THRESH_OTSU);
    // connect horizontally oriented regions
    cv::Mat connected;
    morphKernel = getStructuringElement(cv::MORPH_RECT, cv::Size(9, 1));
    morphologyEx(bw, connected, cv::MORPH_CLOSE, morphKernel);
    // find contours
    cv::Mat mask = cv::Mat::zeros(bw.size(), CV_8UC1);
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    findContours(connected, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
    // filter contours
    for(int idx = 0; idx >= 0; idx = hierarchy[idx][0])
    {
        cv::Rect rect = boundingRect(contours[idx]);
        cv::Mat maskROI(mask, rect);
        maskROI = cv::Scalar(0, 0, 0);
        // fill the contour
        drawContours(mask, contours, idx, cv::Scalar(255, 255, 255), CV_FILLED);
        // ratio of non-zero pixels in the filled region
        double r = (double)countNonZero(maskROI)/(rect.width*rect.height);
        
        if (r > .45 // assume at least 45% of the area is filled if it contains text
            &&
            (rect.height > 8 && rect.width > 8) // constraints on region size 
            // these two conditions alone are not very robust. better to use something
            // like the number of significant peaks in a horizontal projection as a third condition 
            )
        {
            rectangle(rgb, rect, cv::Scalar(0, 255, 0), 2);
        }
    }
    pyrUp(rgb, rgb, cv::Size(rgb.cols*2, rgb.rows*2));
    rgb.copyTo(des);
}
*/

