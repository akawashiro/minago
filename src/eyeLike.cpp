#include "eyeLike.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <chrono>
#include <iostream>
#include <math.h>
#include <queue>
#include <stdio.h>

#include "constants.h"
#include "findEyeCenter.h"
#include "findEyeCorner.h"

/* Attempt at supporting openCV version 4.0.1 or higher */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#define CV_BGR2YCrCb cv::COLOR_BGR2YCrCb
#define CV_HAAR_SCALE_IMAGE cv::CASCADE_SCALE_IMAGE
#define CV_HAAR_FIND_BIGGEST_OBJECT cv::CASCADE_FIND_BIGGEST_OBJECT
#endif

/** Constants **/

/** Function Headers */
void detectAndDisplay(cv::Mat frame);

/** Global variables */
//-- Note, either copy these two files from opencv/data/haarscascades to your
// current folder, or change these locations
cv::String face_cascade_name = "../res/haarcascade_frontalface_alt.xml";
cv::CascadeClassifier face_cascade;
std::string main_window_name = "Capture - Face detection";
std::string face_window_name = "Capture - Face";
cv::RNG rng(12345);
cv::Mat debugImage;
cv::Mat skinCrCbHist = cv::Mat::zeros(cv::Size(256, 256), CV_8UC1);

int FRAME_WIDTH = 1280;
int FRAME_HEIGHT = 720;

/**
 * @function main
 */

namespace eye_like {
int run_main(std::pair<int, int> resolution, bool enable_image) {
    FRAME_WIDTH = resolution.first;
    FRAME_HEIGHT = resolution.second;

    cv::Mat frame;

    // Load the cascades
    if (!face_cascade.load(face_cascade_name)) {
        printf(
            "--(!)Error loading face cascade, please change face_cascade_name "
            "in source code.\n");
        return -1;
    };
    cv::namedWindow(main_window_name, CV_WINDOW_NORMAL);
    cv::moveWindow(main_window_name, 400, 100);
    cv::namedWindow(face_window_name, CV_WINDOW_NORMAL);
    cv::moveWindow(face_window_name, 10, 100);
    cv::namedWindow("Right Eye", CV_WINDOW_NORMAL);
    cv::moveWindow("Right Eye", 10, 600);
    cv::namedWindow("Left Eye", CV_WINDOW_NORMAL);
    cv::moveWindow("Left Eye", 10, 800);

    /* As the matrix dichotomy will not be applied, these windows are useless.
    cv::namedWindow("aa",CV_WINDOW_NORMAL);
    cv::moveWindow("aa", 10, 800);
    cv::namedWindow("aaa",CV_WINDOW_NORMAL);
    cv::moveWindow("aaa", 10, 800);*/

    createCornerKernels();
    ellipse(skinCrCbHist, cv::Point(113, 155), cv::Size(23, 15), 43.0, 0.0,
            360.0, cv::Scalar(255, 255, 255), -1);

    std::chrono::system_clock::time_point start, end;

    // I make an attempt at supporting both 2.x and 3.x OpenCV
    // #if CV_MAJOR_VERSION < 3
    //     CvCapture *capture = cvCaptureFromCAM(0);
    //     if (capture) {
    //         while (true) {
    //             frame = cvQueryFrame(capture);
    // #else
    cv::VideoCapture capture(0);
    if (capture.isOpened()) {
        capture.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
        while (true) {
            start = std::chrono::system_clock::now();
            capture.read(frame);
            // #endif

            // mirror it
            cv::flip(frame, frame, 1);
            frame.copyTo(debugImage);

            // Apply the classifier to the frame
            if (!frame.empty()) {
                detectAndDisplay(frame);
            } else {
                printf(" --(!) No captured frame -- Break!");
                break;
            }

            if (enable_image) {
                imshow(main_window_name, debugImage);

                int c = cv::waitKey(10);
                if ((char)c == 'q') {
                    break;
                }
                if ((char)c == 'f') {
                    imwrite("frame.png", frame);
                }
            }

            end = std::chrono::system_clock::now();
            double time = static_cast<double>(
                std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                      start)
                    .count() /
                1000.0);
            printf("time %lf[ms]\n", time);
        }
    }

    releaseCornerKernels();

    return 0;
}
} // namespace eye_like

void findEyes(cv::Mat frame_gray, cv::Rect face) {
    cv::Mat faceROI = frame_gray(face);
    cv::Mat debugFace = faceROI;

    if (kSmoothFaceImage) {
        double sigma = kSmoothFaceFactor * face.width;
        GaussianBlur(faceROI, faceROI, cv::Size(0, 0), sigma);
    }
    //-- Find eye regions and draw them
    int eye_region_width = face.width * (kEyePercentWidth / 100.0);
    int eye_region_height = face.width * (kEyePercentHeight / 100.0);
    int eye_region_top = face.height * (kEyePercentTop / 100.0);
    cv::Rect leftEyeRegion(face.width * (kEyePercentSide / 100.0),
                           eye_region_top, eye_region_width, eye_region_height);
    cv::Rect rightEyeRegion(
        face.width - eye_region_width - face.width * (kEyePercentSide / 100.0),
        eye_region_top, eye_region_width, eye_region_height);

    //-- Find Eye Centers
    cv::Point leftPupil = findEyeCenter(faceROI, leftEyeRegion, "Left Eye");
    cv::Point rightPupil = findEyeCenter(faceROI, rightEyeRegion, "Right Eye");

    // TODO hard-coding
    std::cout << "face.x = " << face.x << std::endl;
    std::cout << "face.y = " << face.y << std::endl;
    std::cout << "leftPupil.x + leftEyeRegion.x = "
              << leftPupil.x + leftEyeRegion.x << std::endl;
    std::cout << "rightPupil.x  + rightEyeRegion.x= "
              << rightPupil.x + rightEyeRegion.x << std::endl;

    eye_like::left_eye_center_x =
        (double)(leftPupil.x + leftEyeRegion.x + face.x) / (double)FRAME_WIDTH;
    eye_like::left_eye_center_y =
        (double)(leftPupil.y + leftEyeRegion.y + face.y) / (double)FRAME_HEIGHT;
    eye_like::right_eye_center_x =
        (double)(rightPupil.x + rightEyeRegion.x + face.x) /
        (double)FRAME_WIDTH;
    eye_like::right_eye_center_y =
        (double)(rightPupil.y + rightEyeRegion.y + face.y) /
        (double)FRAME_HEIGHT;
    std::cout << "left_eye_center_x  = " << eye_like::left_eye_center_x
              << std::endl;
    std::cout << "left_eye_center_y  = " << eye_like::left_eye_center_y
              << std::endl;
    std::cout << "right_eye_center_x  = " << eye_like::right_eye_center_x
              << std::endl;
    std::cout << "right_eye_center_y  = " << eye_like::right_eye_center_y
              << std::endl;

    // get corner regions
    cv::Rect leftRightCornerRegion(leftEyeRegion);
    leftRightCornerRegion.width -= leftPupil.x;
    leftRightCornerRegion.x += leftPupil.x;
    leftRightCornerRegion.height /= 2;
    leftRightCornerRegion.y += leftRightCornerRegion.height / 2;
    cv::Rect leftLeftCornerRegion(leftEyeRegion);
    leftLeftCornerRegion.width = leftPupil.x;
    leftLeftCornerRegion.height /= 2;
    leftLeftCornerRegion.y += leftLeftCornerRegion.height / 2;
    cv::Rect rightLeftCornerRegion(rightEyeRegion);
    rightLeftCornerRegion.width = rightPupil.x;
    rightLeftCornerRegion.height /= 2;
    rightLeftCornerRegion.y += rightLeftCornerRegion.height / 2;
    cv::Rect rightRightCornerRegion(rightEyeRegion);
    rightRightCornerRegion.width -= rightPupil.x;
    rightRightCornerRegion.x += rightPupil.x;
    rightRightCornerRegion.height /= 2;
    rightRightCornerRegion.y += rightRightCornerRegion.height / 2;
    rectangle(debugFace, leftRightCornerRegion, 200);
    rectangle(debugFace, leftLeftCornerRegion, 200);
    rectangle(debugFace, rightLeftCornerRegion, 200);
    rectangle(debugFace, rightRightCornerRegion, 200);
    // change eye centers to face coordinates
    rightPupil.x += rightEyeRegion.x;
    rightPupil.y += rightEyeRegion.y;
    leftPupil.x += leftEyeRegion.x;
    leftPupil.y += leftEyeRegion.y;
    // draw eye centers
    circle(debugFace, rightPupil, 3, 1234);
    circle(debugFace, leftPupil, 3, 1234);

    //-- Find Eye Corners
    if (kEnableEyeCorner) {
        cv::Point2f leftRightCorner =
            findEyeCorner(faceROI(leftRightCornerRegion), true, false);
        leftRightCorner.x += leftRightCornerRegion.x;
        leftRightCorner.y += leftRightCornerRegion.y;
        cv::Point2f leftLeftCorner =
            findEyeCorner(faceROI(leftLeftCornerRegion), true, true);
        leftLeftCorner.x += leftLeftCornerRegion.x;
        leftLeftCorner.y += leftLeftCornerRegion.y;
        cv::Point2f rightLeftCorner =
            findEyeCorner(faceROI(rightLeftCornerRegion), false, true);
        rightLeftCorner.x += rightLeftCornerRegion.x;
        rightLeftCorner.y += rightLeftCornerRegion.y;
        cv::Point2f rightRightCorner =
            findEyeCorner(faceROI(rightRightCornerRegion), false, false);
        rightRightCorner.x += rightRightCornerRegion.x;
        rightRightCorner.y += rightRightCornerRegion.y;
        circle(faceROI, leftRightCorner, 3, 200);
        circle(faceROI, leftLeftCorner, 3, 200);
        circle(faceROI, rightLeftCorner, 3, 200);
        circle(faceROI, rightRightCorner, 3, 200);
    }

    imshow(face_window_name, faceROI);
    //  cv::Rect roi( cv::Point( 0, 0 ), faceROI.size());
    //  cv::Mat destinationROI = debugImage( roi );
    //  faceROI.copyTo( destinationROI );
}

cv::Mat findSkin(cv::Mat &frame) {
    cv::Mat input;
    cv::Mat output = cv::Mat(frame.rows, frame.cols, CV_8U);

    cvtColor(frame, input, CV_BGR2YCrCb);

    for (int y = 0; y < input.rows; ++y) {
        const cv::Vec3b *Mr = input.ptr<cv::Vec3b>(y);
        //    uchar *Or = output.ptr<uchar>(y);
        cv::Vec3b *Or = frame.ptr<cv::Vec3b>(y);
        for (int x = 0; x < input.cols; ++x) {
            cv::Vec3b ycrcb = Mr[x];
            //      Or[x] = (skinCrCbHist.at<uchar>(ycrcb[1], ycrcb[2]) > 0) ?
            //      255 : 0;
            if (skinCrCbHist.at<uchar>(ycrcb[1], ycrcb[2]) == 0) {
                Or[x] = cv::Vec3b(0, 0, 0);
            }
        }
    }
    return output;
}

/**
 * @function detectAndDisplay
 */
void detectAndDisplay(cv::Mat frame) {
    std::vector<cv::Rect> faces;
    // cv::Mat frame_gray;

    std::vector<cv::Mat> rgbChannels(3);
    cv::split(frame, rgbChannels);
    cv::Mat frame_gray = rgbChannels[2];

    // cvtColor( frame, frame_gray, CV_BGR2GRAY );
    // equalizeHist( frame_gray, frame_gray );
    // cv::pow(frame_gray, CV_64F, frame_gray);
    //-- Detect faces
    face_cascade.detectMultiScale(frame_gray, faces, 1.1, 2,
                                  0 | CV_HAAR_SCALE_IMAGE |
                                      CV_HAAR_FIND_BIGGEST_OBJECT,
                                  cv::Size(150, 150));
    //  findSkin(debugImage);

    for (int i = 0; i < faces.size(); i++) {
        rectangle(debugImage, faces[i], 1234);
    }
    //-- Show what you got
    if (faces.size() > 0) {
        findEyes(frame_gray, faces[0]);
    }
}
