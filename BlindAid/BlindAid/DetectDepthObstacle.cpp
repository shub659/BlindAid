#include "DetectDepthObstacle.h"

using namespace std;
using namespace cv;

void DetectDepthObstacle::Init(Parameters *params, const cv::Mat *image, Results *results)
{
  _params = &params->GetDepthObstacleParams();
  _colorImage = image;
  _results = &results->GetDepthObstacleResults();
}

void DetectDepthObstacle::operator()()
{
  Process();
}

void DetectDepthObstacle::Process()
{
  cvtColor(*_colorImage, _grayImage, CV_BGR2GRAY);

  threshold(_grayImage, _maskImage, 0, 255, THRESH_BINARY);

  GaussianBlur(_grayImage, _grayImage, Size(-1, -1), 3);

  SeparateRegions();
  FindMaxInRegions();

  FindRowMax();
  FindColMax();

  SplitRowRegions();
  SplitColRegions();

  _rowRegion = Mat::zeros(HORZ_REGIONS, VERT_REGIONS, CV_8UC1);
}

void DetectDepthObstacle::PreProcess()
{
}

void DetectDepthObstacle::SeparateRegions()
{
  int width = _grayImage.cols / VERT_REGIONS;
  int height = _grayImage.rows / HORZ_REGIONS;

  for (int i = 0; i < HORZ_REGIONS; ++i)
  {
    for (int j = 0; j < VERT_REGIONS; ++j)
    {
      _regions[i * VERT_REGIONS + j].x = j * width;
      _regions[i * VERT_REGIONS + j].y = i * height;
      _regions[i * VERT_REGIONS + j].width = width;
      _regions[i * VERT_REGIONS + j].height = height;
    }
  }
}

void DetectDepthObstacle::FindMaxInRegions()
{
  double minVal = 0;
  double maxVal = 0;
  for (int i = 0; i < HORZ_REGIONS; ++i)
  {
    for (int j = 0; j < VERT_REGIONS; ++j)
    {
      minMaxLoc(_grayImage(_regions[i * VERT_REGIONS + j]), &minVal, &maxVal);
     
      _results->SetRegion(i, j, minVal);

      // Histogram Calculation
      Mat hist;
      int size[] = { 256 };
      float range[] = { 0, 256 };
      const float* ranges[] = { range };
      int channels[] = { 0 };
      calcHist(&_grayImage(_regions[i * VERT_REGIONS + j]), 1, channels, _maskImage(_regions[i * VERT_REGIONS + j]), hist, 1, size, ranges, true, false);
      normalize(hist, hist, 0, hist.rows, NORM_MINMAX, -1, Mat());

      Mat histDisplay = Mat::zeros(257, 256, CV_8UC1);
      for (int k = 0; k < 256; ++k)
        histDisplay.at<char>(256 - hist.at<float>(k), k) = 255;
    }
  }
}

void DetectDepthObstacle::FindRowMax()
{
  _rowMax = Mat::zeros(1, _grayImage.rows, CV_8UC1);
  int pixel = 0;
  int rowMax = 0;

  for (int i = 0; i < _grayImage.rows; ++i)
  {
    rowMax = 255;

    for (int j = 0; j < _grayImage.cols; ++j)
    {
      pixel = _grayImage.at<char>(i, j);

      if (pixel > 0 && pixel < rowMax)
        rowMax = pixel;
    }

    _rowMax.at<char>(0, i) = rowMax;
  }
}

void DetectDepthObstacle::FindColMax()
{
  _colMax = Mat::zeros(1, _grayImage.cols, CV_8UC1);
  int pixel = 0;
  int columnMax = 0;

  for (int j = 0; j < _grayImage.cols; ++j)
  {
    columnMax = 255;

    for (int i = 0; i < _grayImage.rows; ++i)
    {
      pixel = _grayImage.at<char>(i, j);

      if (pixel > 0 && pixel < columnMax)
        columnMax = pixel;
    }

    _colMax.at<char>(0, j) = columnMax;
  }
}

void DetectDepthObstacle::SplitRowRegions()
{
  _rowRegion = Mat::zeros(1, HORZ_REGIONS, CV_8UC1);
  int numPixels = _rowMax.cols / HORZ_REGIONS;
  int pixel = 0;
  int regionMax = 0;

  for (int i = 0; i < HORZ_REGIONS; ++i)
  {
    regionMax = 255;
    
    for (int j = 0; j < numPixels; ++j)
    {
      pixel = _rowMax.at<char>(0, i*numPixels + j);

      if (pixel > 0 && pixel < regionMax)
        regionMax = pixel;
    }

    _rowRegion.at<char>(0, i) = regionMax;
  }
}

void DetectDepthObstacle::SplitColRegions()
{
  _colRegion = Mat::zeros(1, VERT_REGIONS, CV_8UC1);
  int numPixels = _colMax.cols / VERT_REGIONS;
  int pixel = 0;
  int regionMax = 0;

  for (int i = 0; i < VERT_REGIONS; ++i)
  {
    regionMax = 255;

    for (int j = 0; j < numPixels; ++j)
    {
      pixel = _colMax.at<char>(0, i*numPixels + j);

      if (pixel > 0 && pixel < regionMax)
        regionMax = pixel;
    }

    _colRegion.at<char>(0, i) = regionMax;
  }
}

void DetectDepthObstacle::Draw()
{
}

void DetectDepthObstacle::Display()
{
}

void DetectDepthObstacle::Clear()
{
}

// TODO: implement function to break region into 3x5, iterate over them, and send (x, y, width, height) to...
//       another function that finds the local maximum in that region.
// Rect region_of_interest = Rect(x, y, w, h);
