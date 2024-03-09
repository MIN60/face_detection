#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <vector>

// 컬러 split/merge

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
    Mat frame;  // 프레임 저장 객체
    VideoCapture cap;   // 웹캠

    int deviceID = 0; // 0 = open 
    int apiID = cv::CAP_ANY; // 0 = autodetect 

    cap.open(deviceID + apiID); // 캠 열기

    if (!cap.isOpened()) {  // 카메라 안 열리면 에러
        cerr << "CAMERA ERROR\n";
        return -1;
    }

    cout << "Start grabbing" << endl
        << "Press any key to terminate" << endl;    // 에러 메세지 출력
    for (;;)
    {
        cap.read(frame);    // 웹캠 프레임 읽기
        if (frame.empty()) {    // 안 열리면 에러
            cerr << "FRAME ERROR\n";
            break;
        }

        // 이미지 처리 

        Mat ycbcr;  // YCbCr이미지 저장 객체
        cvtColor(frame, ycbcr, COLOR_RGB2YCrCb); // YCbCr로 변환
        // bgr[0] = Y 채널
        // bgr[1] = Cb 채널
        // bgr[2] = Cr 채널

        Mat result; // 처리 결과 저장
        Mat channel[3]; // Y, Cb, Cr 채널 분리를 위한 배열
        split(ycbcr, channel); // 채널별로 분리


        //채널 분리 후 임계값
        result = channel[0].clone();    // Y 채널 복사로 결과 이미지 초기화
        // skin color를 검출하기 위한 Cb,Cr 임계값 처리
        for (int i = 0; i < ycbcr.rows; i++) { // 행
            for (int j = 0; j < ycbcr.cols; j++) { // 열 
                // 80 127 140 170 
                if (channel[1].at<uchar>(i, j) > 75 && channel[1].at<uchar>(i, j) < 127
                    && channel[2].at<uchar>(i, j) > 135 && channel[2].at<uchar>(i, j) < 170) {
                    result.at<uchar>(i, j) = 255;   // 임계값 이내면 하얀색
                }
                else {
                    result.at<uchar>(i, j) = 0; // 임계값 밖이면 검은색
                }
            }
        }


        //침식, 팽창, 가우시안을 통한 노이즈 제거
        erode(result, result, Mat::ones(Size(3, 3), CV_8UC1), Point(-1, -1), 3);
        dilate(result, result, Mat::ones(Size(3, 3), CV_8UC1), Point(-1, -1), 3);
        GaussianBlur(result, result, Size(5, 5), 0);

        // 검출 영역 면적
        vector<vector<Point> > contour;
        vector<Vec4i> hierarchy;
        findContours(result, contour, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // 검출 가능 박스 크기 제한
        double minFrameSize = frame.cols * frame.rows * 0.01; // 1%
        double maxFramSize = frame.cols * frame.rows * 0.8; // 80%


        // 너무 작은 박스랑 큰 박스 거르기
        vector<Rect> FaceBoxes;
        for (size_t i = 0; i < contour.size(); i++) {
            if (contourArea(contour[i]) > minFrameSize && contourArea(contour[i]) < maxFramSize) {
                drawContours(frame, contour, i, Scalar(100, 0, 0), 2, 8, hierarchy, 0, Point());
                int rectArea = boundingRect(contour[i]).width * boundingRect(contour[i]).height;
                if (rectArea * 0.5 >= contourArea(contour[i])) { //사각형 넓이의 0.5퍼센트보다 컨투어 넓이가 작으면 제외.
                    continue;
                }
                int rectLength = boundingRect(contour[i]).width * 2 + boundingRect(contour[i]).height * 2;
                if (rectLength * 1.5 <= arcLength(contour[i], true)) { //사각형 둘레의 1.5퍼센트보다 컨투어의 둘레가 더 길면 제외.
                    continue;
                }
                //컨투어 길이가 사각형 길이보다 너무 길면 얼굴 아님
                FaceBoxes.push_back(boundingRect(contour[i]));
            }
        }



        // 박스 안에 박스 생기는 거 거르기
        // 박스끼리 겹치는 거 거르기
        vector<bool> Duplicate(FaceBoxes.size(), true);
        for (size_t i = 0; i < FaceBoxes.size(); i++) {
            for (size_t j = i + 1; j < FaceBoxes.size(); j++) {
                if ((FaceBoxes[j] & FaceBoxes[i]) == FaceBoxes[i]) { //사각형끼리 겹치는 영역
                    Duplicate[i] = false; //i 사각형이 작으니까 false
                }
                if ((FaceBoxes[j] & FaceBoxes[i]) == FaceBoxes[j]) { //사각형끼리 겹치는 영역
                    Duplicate[j] = false; //j 사각형이 작으니까 false
                }
            }
        }

        // 검출된 얼굴 수
        int Facenum = 0;

        // 얼굴/ 손 구별하고 얼굴 영역에 사각형 그리기
        for (size_t i = 0; i < FaceBoxes.size(); i++) {

            int maxWhite = 0; // 최대 하얀색
            int countWhite = 0; //하얀색인 부분 카운트

            
            int whiteThres = FaceBoxes[i].width * 0.5; // 하얀색 부분은 상자 너비의 50% 이상이어야 함
            // 이어진 하얀색 부분 비교 위치
            
            int check = FaceBoxes[i].y + 50;

            if (!Duplicate[i]) { // 겹치는 영역에 사각형을 그리지 않음
                continue;
            }


            for (int j = FaceBoxes[i].x; j < FaceBoxes[i].x + FaceBoxes[i].width; j++) {
                if (result.at<uchar>(check, j) == 255) { //만약 검사 위치가 흰색이면
                    countWhite++; // 하얀색부분 카운트
                    if (countWhite > maxWhite) { // 최대 하얀색 업데이트
                        maxWhite = countWhite;
                    }
                    // 검사위치에 하얀색인 부분을 빨간색으로 표시하는 부분
                    circle(frame, Point(j, check), 1, Scalar(0, 0, 255), -1);
                }
                else {
                    countWhite = 0; // 검은색이면 카운트하지 않음
                }
            }

            // 이어진 하얀색 부분이 설정값보다 크면 얼굴 영역으로 판단하고 사각형을 그림
            if (maxWhite > whiteThres) {
                rectangle(frame, FaceBoxes[i], Scalar(255, 0, 0), 2);
                Facenum++;
            }
            //cout << "maxWhite " << i << ": " << maxWhite << endl;
            cout << "Detected Faces: " << Facenum << endl;

        }

        // 검출된 얼굴 수 표시
        string text = "Faces: " + to_string(Facenum);
        //폰트크기2 두께 5
        putText(frame, text, Point(10, frame.rows -300), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 0, 0), 5, LINE_8);


        // 얼굴에 사각형 그리기
        imshow("Live1", frame); 

        //grayscale로 체크
        imshow("Live", result);

        if (waitKey(5) >= 0)
            break;
    }


    return 0;
}
