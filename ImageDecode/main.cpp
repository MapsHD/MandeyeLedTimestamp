#include <opencv2/opencv.hpp>
#include <iostream>
#include "json.hpp"
#include <map>
#include <fstream>
#include "gray.h"
#include <filesystem>
using namespace cv;
using namespace std;
using json = nlohmann::json;


struct Bit {
    cv::Point2i location;
    float radius;
};

struct Settings {
    std::string coding;
};

std::map<int,Bit> GetBitDef(const std::string& fn, float scale)
{
      std::map<int,Bit> bits;
      std::ifstream ifs(fn);
      json j;
      ifs >> j;
      for (auto& value : j["bits"]) {
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

Settings GetSettings(const std::string& fn)
{
    std::ifstream ifs(fn);
    json j;
    ifs >> j;
    Settings s;
    s.coding = j["coding"];
    return s;
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

void help(const std::string argv0)
{
  cout << "Usage: " << argv0 << " <image_path> <mask_path> <options>" << endl;
  cout << "options:\n";
  cout << " -h : Show this help message" << endl;
  cout << " -ne : Do not export image with name as decoded timestamp" << endl;
  cout << " -nw : Do not show input" << endl;

}
int main(int argc, char** argv) {
    if (argc < 3) {
        help(argv[0]);
        return -1;
    }

    bool exportImage = true;
    bool gui = true;
    for (int i =0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            help(argv[0]);
            return -1;
        }
        else if (strcmp(argv[i], "-ne") == 0) {
            exportImage = false;
        }
        else if (strcmp(argv[i], "-nw") == 0) {
          gui = false;
        }
    }
    const std::string jsonPath = argv[2];
    const auto settings = GetSettings(jsonPath);
    const int processingSize = 640;
    // Load image and mask
    const Mat inputImage = imread(argv[1], IMREAD_COLOR);

    cv::Mat image = inputImage.clone();
    const float scale = float(processingSize) / image.cols;
    const int newHeight = int(image.rows * scale);

    const auto Bits = GetBitDef(jsonPath,scale);

    cv::resize(image, image, cv::Size(processingSize, newHeight));

    // Apply thresholding
    Mat gray, thresholded, gray_masked;
    vector<Mat> channels(3);
    split(image, channels);
    //cvtColor(image, gray, COLOR_BGR2GRAY);

    cv::Mat rgb_image = image;

    //set bits
    std::uint32_t bitValue = 0x0;

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
      cv::putText(rgb_image, buffer, {0, 40+key*20}, cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(0, 255, 0), 1);

      cv::putText(rgb_image, std::to_string(key), value.location+cv::Point2i {10,0}, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);

      auto color = cv::Scalar(0, 255, 0);
      if (isRed) {

        color = cv::Scalar(0, 0, 255);
      }
      else {
        color = cv::Scalar(255, 0, 0);
      }

      cv::circle(rgb_image, value.location, value.radius, color, 2);

      // setup bit
      if (isRed) {
        bitValue |= 1 << key;
      }
    }

    // supported codings below:
    const std::set<std::string> supportedCodings = {"gray", "binary", ""};
    const std::string coding = settings.coding;
    if (supportedCodings.find(coding) == supportedCodings.end()) {
      std::cerr << "Unsupported coding: " << coding << std::endl;
      return -1;
    }
    if (settings.coding == "gray") {
      bitValue = fromGrayCode(bitValue);
    }

    char buffer[100];
    sprintf(buffer, "Bit value: %u", bitValue);
    cv::putText(rgb_image, buffer, {0, 20}, cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(255, 255, 0), 1);


    if (exportImage)
    {
       const std::filesystem::path imagePath (argv[1]);
       const std::filesystem::path parentPath = imagePath.parent_path();

       char resultFn[256];
       sprintf(buffer, "%u.jpg", bitValue);

       const std::filesystem::path resultPath = parentPath / std::string (buffer);
       std::cout << "Will save to " << resultPath << std::endl;
       cv::imwrite(resultPath, image);

    }
    if (gui) {
      cv::Mat result;
      std::vector<cv::Mat> images{rgb_image};
      hconcat(images, image);
      cv::imshow("Data", image);
      waitKey(0);
    }
    return 0;
}
