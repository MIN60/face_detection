#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <vector>

// �÷� split/merge

using namespace cv;
using namespace std;

int main(int argc, char** argv)
{
    Mat frame;  // ������ ���� ��ü
    VideoCapture cap;   // ��ķ

    int deviceID = 0; // 0 = open 
    int apiID = cv::CAP_ANY; // 0 = autodetect 

    cap.open(deviceID + apiID); // ķ ����

    if (!cap.isOpened()) {  // ī�޶� �� ������ ����
        cerr << "CAMERA ERROR\n";
        return -1;
    }

    cout << "Start grabbing" << endl
        << "Press any key to terminate" << endl;    // ���� �޼��� ���
    for (;;)
    {
        cap.read(frame);    // ��ķ ������ �б�
        if (frame.empty()) {    // �� ������ ����
            cerr << "FRAME ERROR\n";
            break;
        }

        // �̹��� ó�� 

        Mat ycbcr;  // YCbCr�̹��� ���� ��ü
        cvtColor(frame, ycbcr, COLOR_RGB2YCrCb); // YCbCr�� ��ȯ
        // bgr[0] = Y ä��
        // bgr[1] = Cb ä��
        // bgr[2] = Cr ä��

        Mat result; // ó�� ��� ����
        Mat channel[3]; // Y, Cb, Cr ä�� �и��� ���� �迭
        split(ycbcr, channel); // ä�κ��� �и�


        //ä�� �и� �� �Ӱ谪
        result = channel[0].clone();    // Y ä�� ����� ��� �̹��� �ʱ�ȭ
        // skin color�� �����ϱ� ���� Cb,Cr �Ӱ谪 ó��
        for (int i = 0; i < ycbcr.rows; i++) { // ��
            for (int j = 0; j < ycbcr.cols; j++) { // �� 
                // 80 127 140 170 
                if (channel[1].at<uchar>(i, j) > 75 && channel[1].at<uchar>(i, j) < 127
                    && channel[2].at<uchar>(i, j) > 135 && channel[2].at<uchar>(i, j) < 170) {
                    result.at<uchar>(i, j) = 255;   // �Ӱ谪 �̳��� �Ͼ��
                }
                else {
                    result.at<uchar>(i, j) = 0; // �Ӱ谪 ���̸� ������
                }
            }
        }


        //ħ��, ��â, ����þ��� ���� ������ ����
        erode(result, result, Mat::ones(Size(3, 3), CV_8UC1), Point(-1, -1), 3);
        dilate(result, result, Mat::ones(Size(3, 3), CV_8UC1), Point(-1, -1), 3);
        GaussianBlur(result, result, Size(5, 5), 0);

        // ���� ���� ����
        vector<vector<Point> > contour;
        vector<Vec4i> hierarchy;
        findContours(result, contour, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        // ���� ���� �ڽ� ũ�� ����
        double minFrameSize = frame.cols * frame.rows * 0.01; // 1%
        double maxFramSize = frame.cols * frame.rows * 0.8; // 80%


        // �ʹ� ���� �ڽ��� ū �ڽ� �Ÿ���
        vector<Rect> FaceBoxes;
        for (size_t i = 0; i < contour.size(); i++) {
            if (contourArea(contour[i]) > minFrameSize && contourArea(contour[i]) < maxFramSize) {
                drawContours(frame, contour, i, Scalar(100, 0, 0), 2, 8, hierarchy, 0, Point());
                int rectArea = boundingRect(contour[i]).width * boundingRect(contour[i]).height;
                if (rectArea * 0.5 >= contourArea(contour[i])) { //�簢�� ������ 0.5�ۼ�Ʈ���� ������ ���̰� ������ ����.
                    continue;
                }
                int rectLength = boundingRect(contour[i]).width * 2 + boundingRect(contour[i]).height * 2;
                if (rectLength * 1.5 <= arcLength(contour[i], true)) { //�簢�� �ѷ��� 1.5�ۼ�Ʈ���� �������� �ѷ��� �� ��� ����.
                    continue;
                }
                //������ ���̰� �簢�� ���̺��� �ʹ� ��� �� �ƴ�
                FaceBoxes.push_back(boundingRect(contour[i]));
            }
        }



        // �ڽ� �ȿ� �ڽ� ����� �� �Ÿ���
        // �ڽ����� ��ġ�� �� �Ÿ���
        vector<bool> Duplicate(FaceBoxes.size(), true);
        for (size_t i = 0; i < FaceBoxes.size(); i++) {
            for (size_t j = i + 1; j < FaceBoxes.size(); j++) {
                if ((FaceBoxes[j] & FaceBoxes[i]) == FaceBoxes[i]) { //�簢������ ��ġ�� ����
                    Duplicate[i] = false; //i �簢���� �����ϱ� false
                }
                if ((FaceBoxes[j] & FaceBoxes[i]) == FaceBoxes[j]) { //�簢������ ��ġ�� ����
                    Duplicate[j] = false; //j �簢���� �����ϱ� false
                }
            }
        }

        // ����� �� ��
        int Facenum = 0;

        // ��/ �� �����ϰ� �� ������ �簢�� �׸���
        for (size_t i = 0; i < FaceBoxes.size(); i++) {

            int maxWhite = 0; // �ִ� �Ͼ��
            int countWhite = 0; //�Ͼ���� �κ� ī��Ʈ

            
            int whiteThres = FaceBoxes[i].width * 0.5; // �Ͼ�� �κ��� ���� �ʺ��� 50% �̻��̾�� ��
            // �̾��� �Ͼ�� �κ� �� ��ġ
            
            int check = FaceBoxes[i].y + 50;

            if (!Duplicate[i]) { // ��ġ�� ������ �簢���� �׸��� ����
                continue;
            }


            for (int j = FaceBoxes[i].x; j < FaceBoxes[i].x + FaceBoxes[i].width; j++) {
                if (result.at<uchar>(check, j) == 255) { //���� �˻� ��ġ�� ����̸�
                    countWhite++; // �Ͼ���κ� ī��Ʈ
                    if (countWhite > maxWhite) { // �ִ� �Ͼ�� ������Ʈ
                        maxWhite = countWhite;
                    }
                    // �˻���ġ�� �Ͼ���� �κ��� ���������� ǥ���ϴ� �κ�
                    circle(frame, Point(j, check), 1, Scalar(0, 0, 255), -1);
                }
                else {
                    countWhite = 0; // �������̸� ī��Ʈ���� ����
                }
            }

            // �̾��� �Ͼ�� �κ��� ���������� ũ�� �� �������� �Ǵ��ϰ� �簢���� �׸�
            if (maxWhite > whiteThres) {
                rectangle(frame, FaceBoxes[i], Scalar(255, 0, 0), 2);
                Facenum++;
            }
            //cout << "maxWhite " << i << ": " << maxWhite << endl;
            cout << "Detected Faces: " << Facenum << endl;

        }

        // ����� �� �� ǥ��
        string text = "Faces: " + to_string(Facenum);
        //��Ʈũ��2 �β� 5
        putText(frame, text, Point(10, frame.rows -300), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 0, 0), 5, LINE_8);


        // �󱼿� �簢�� �׸���
        imshow("Live1", frame); 

        //grayscale�� üũ
        imshow("Live", result);

        if (waitKey(5) >= 0)
            break;
    }


    return 0;
}
