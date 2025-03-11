#include <opencv2/opencv.hpp>
#include <iostream>
#include "json.hpp"
#include <map>
#include <fstream>
using namespace cv;
using namespace std;
using json = nlohmann::json;

struct Bit {
    cv::Point2i location;
    float radius;
};


std::map<int,Bit> GetBitDef(const std::string& fn, float scale)
{
      std::map<int,Bit> bits;
      std::ifstream ifs(fn);
      json j;
      ifs >> j;
      for (auto& value : j) {
          Bit b;
          int key = value["bit"];
          b.location = cv::Point2i(value["location"][0],value["location"][1]);
          b.location.x *= scale;
          b.location.y *= scale;
          b.radius = value["radius"];

          bits[key] = b;
      }
      return bits;
}


void iterateOnCircle (const cv::Mat& img, cv::Point2i center, float radius, std::function<void(cv::Point2i)> f)
{
    for (int y = center.y - radius; y <= center.y + radius; y++) {
      for (int x = center.x - radius; x <= center.x + radius; x++) {
        // Ensure we are within image bounds
        if (x >= 0 && x < img.cols && y >= 0 && y < img.rows) {
          // Check if the pixel is inside the circle
          if ((x - center.x) * (x - center.x) + (y - center.y) * (y - center.y) <= radius * radius) {
            f(cv::Point2i(x, y));
          }
        }
      }
    }
}
int main(int argc, char** argv) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <image_path> <mask_path>" << endl;
        return -1;
    }

    const int processingSize = 640;
    // Load image and mask
    Mat image = imread(argv[1], IMREAD_COLOR);

    const float scale = float(processingSize) / image.cols;
    const int newHeight = int(image.rows * scale);
    cv::resize(image, image, cv::Size(processingSize, newHeight));
    Mat mask = imread(argv[2], IMREAD_GRAYSCALE);
    cv::resize(mask, mask, cv::Size(processingSize, newHeight));
    threshold(mask, mask, 125, 255, THRESH_BINARY);
    const auto Bits = GetBitDef("/home/michal/code/MandeyeLedTimestamp/ImageDecode/bits.json",scale);

    //mask = 255-mask;
    if (image.empty() || mask.empty()) {
        cerr << "Error loading image or mask!" << endl;
        return -1;
    }

//    // Compute threshold
//    int thresholdValue = computeThreshold(image, mask);
//    cout << "Computed Threshold: " << thresholdValue << endl;

    // apply mask

    // Apply thresholding
    Mat gray, thresholded, gray_masked;
    vector<Mat> channels(3);
    split(image, channels);
    //cvtColor(image, gray, COLOR_BGR2GRAY);

    gray = channels[2];
    gray.copyTo(gray_masked, mask);
    cv::GaussianBlur(gray_masked, gray_masked, cv::Size(5, 5), 0);

    threshold(gray_masked, thresholded,250, 255, THRESH_BINARY);

    cv::Mat openedImage;
    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    thresholded.copyTo(openedImage);
    for (int i=0; i < 10; i++) {
      cv::morphologyEx(openedImage, openedImage, cv::MORPH_CLOSE, element);
    }
    //
    cv::Mat rgb_image = image;

    for (const auto& [key, value] : Bits) {
      double redCumulative = 0;
      double blueCumulative = 0;
      double greenCumulative = 0;
      auto countPixels =  [&](cv::Point2i loc) {
          //get pixel
          cv::Vec3b pixel = rgb_image.at<cv::Vec3b>(loc);
          redCumulative += pixel[2];
          greenCumulative += pixel[1];
          blueCumulative +=pixel[0];
      };
      iterateOnCircle(rgb_image, value.location, value.radius, countPixels);

      double mean = (redCumulative + greenCumulative + blueCumulative) / 3;
      double blue = blueCumulative / mean;
      double red = redCumulative / mean;
      char buffer[100];
      bool isRed = red > 1.1*blue;
      sprintf(buffer, "%d [%s] R: %.2f, B: %.2f", key, isRed?"high":"low", red, blue);
      cv::putText(rgb_image, buffer, {0, 20+key*20}, cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(0, 255, 0), 1);

      cv::putText(rgb_image, std::to_string(key), value.location+cv::Point2i {10,0}, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);


      auto color = cv::Scalar(0, 255, 0);
      if (red > 1.1*blue) {
        color = cv::Scalar(0, 0, 255);
      }
      if (blue > 1.1*red) {
        color = cv::Scalar(255, 0, 0);
      }

      cv::circle(rgb_image, value.location, value.radius, color, 2);
    }

    cv::Mat result;
    std::vector<cv::Mat> images{ rgb_image};
    hconcat(images, image);
    imwrite("Data.png", image);
    cv::imshow("Data", image);

    waitKey(0);
    return 0;
}
