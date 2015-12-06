#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>

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

    //Create the display window
    cv::namedWindow("Visual");

    //Display loop
    bool loop = true;
    while(loop) {
      imshow("Visual", modified_image);

      char c = cvWaitKey(15);
      switch(c) {
      case 27:  //Exit display loop if ESC is pressed
        loop = false;
        break;
      }
    }

    // Cleanup
    cv::destroyAllWindows();
}
