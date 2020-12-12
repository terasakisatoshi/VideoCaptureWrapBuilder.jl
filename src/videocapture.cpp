#include <opencv2/opencv.hpp>
#include "jlcxx/jlcxx.hpp"

using namespace jlcxx;
using namespace cv;

// It is not used for our demo. but useful as an yet another example.
int capture_image(cv::VideoCapture &cap)
{
  if (!cap.isOpened())
    return -1;

  Mat frame, edges;
  namedWindow("edges", WINDOW_AUTOSIZE);
  for (int i = 1; i < 30; i++)
  {
    cap >> frame;
    cvtColor(frame, edges, COLOR_BGR2GRAY);
    GaussianBlur(edges, edges, Size(7, 7), 1.5, 1.5);
    Canny(edges, edges, 0, 30, 3);
    imshow("edges", edges);
    if (waitKey(1) >= 0)
      break;
  }
  return 0;
}

// Pass data from C++ to Julia
void set_jlvec(jlcxx::ArrayRef<uint8_t> jlvec, cv::Mat& frame)
{
  int W = frame.cols;
  int H = frame.rows;
  int C = frame.channels();
  if (C == 3){
    cvtColor(frame, frame, COLOR_BGR2RGB);
  }
  int idx = 0;
  for (int j = 0; j < W; j++)
  {
    for (int i = 0; i < H; i++)
    {
      for (int k = 0; k < C; k++)
      {
        jlvec[idx] = frame.ptr<cv::Vec3b>(i)[j][k];
        idx++;
      }
    }
  }
}

// Pass data from Julia to C++
cv::Mat to_cvimage(jlcxx::ArrayRef<uint8_t> jlimg, int C, int H, int W)
{
  cv::Mat frame(H, W, CV_8UC(C));
  int idx = 0;
  for (int j = 0; j < W; j++)
  {
    for (int i = 0; i < H; i++)
    {
      for (int k = 0; k < 3; k++)
      {
        frame.ptr<cv::Vec3b>(i)[j][k]= jlimg[idx];
        idx++;
      }
    }
  }
  cv::cvtColor(frame, frame, COLOR_BGR2RGB);
  return frame;
}

std::vector<uint8_t> cvmat2stdvec(cv::Mat &frame)
{
  frame = frame.t();
  int C = frame.channels();
  if (C == 3){
    cvtColor(frame, frame, COLOR_BGR2RGB);
  }
  std::vector<uchar> array;
  if (frame.isContinuous()) {
    // array.assign(frame.datastart, frame.dataend); // <- has problems for sub-matrix like frame = big_mat.row(i)
    array.assign(frame.data, frame.data + frame.total()*frame.channels());
  } else {
    for (int i = 0; i < frame.rows; ++i) {
      array.insert(array.end(), frame.ptr<uchar>(i), frame.ptr<uchar>(i)+frame.cols*frame.channels());
    }
  }
  return array;
}

double get_capture_width(cv::VideoCapture &cap)
{
  return cap.get(CAP_PROP_FRAME_WIDTH);
}

double get_capture_height(cv::VideoCapture &cap)
{
  return cap.get(CAP_PROP_FRAME_HEIGHT);
}

double get_image_width(cv::Mat &m)
{
  return m.cols;
}

double get_image_height(cv::Mat &m)
{
  return m.rows;
}

double get_image_channels(cv::Mat &m)
{
  return m.channels();
}


JLCXX_MODULE
define_videoio_module(Module &mod)
{
  mod.add_type<cv::Mat>("Mat")
      .constructor<int, int, int>();
  //mod.add_type<cv::String>("CVString"); // remove this if you are mac user

  mod.set_override_module(mod.julia_module());
  mod.add_type<cv::VideoCapture>("VideoCapture")
      .constructor<int>()
      .method(
          "isOpened",
          [](const cv::VideoCapture &instance) { return instance.isOpened(); })
      .method("release",
              [](cv::VideoCapture &instance) { return instance.release(); })
      .method("read",
              [](cv::VideoCapture &instance) {
                Mat frame;
                instance.read(frame);
                return frame;
              })
      .method("read!",
              [](cv::VideoCapture &instance, cv::Mat &frame) {
                instance.read(frame);
                return frame;
              });
  mod.unset_override_module();

  mod.method("namedWindow", [](const std::string &winname, int mode){
    cv::namedWindow(winname, mode);
  });
  mod.method("waitKey", cv::waitKey);
  mod.method("destroyWindow", [](const std::string &winname){
    cv::destroyWindow(winname);
  });
  mod.method("imshow", [](const std::string &winname, const cv::Mat &mat) {
    return cv::imshow(winname, cv::InputArray(mat));
  });
  mod.method("imread", [](const std::string &filename, int mode){return cv::imread(filename, mode);});
  
  mod.method("capture_image", capture_image);
  mod.method("get_capture_width", get_capture_width);
  mod.method("get_capture_height", get_capture_height);
  mod.method("get_image_width", get_image_width);
  mod.method("get_image_height", get_image_height);
  mod.method("get_image_channels", get_image_channels);
  mod.method("set_jlvec!", set_jlvec);
  mod.method("to_cvimage", to_cvimage);
  mod.method("cvmat2stdvec", cvmat2stdvec);
}
