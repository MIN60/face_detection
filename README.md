# face_detection
Face detection using opencv

Computer Vision Class for the Second Semester of 2023

Face (Hand) Detection

## Color space transformation
RGB color space is sensitive to variations in external lighting, which can lead to changes in the distribution of colors. In contrast, the YCbCr color space is less affected by changes in lighting compared to RGB color space, making it more suitable for accurate facial area extraction. YCbCr separates luminance information from chrominance information, where Y represents luminance (brightness), and Cb and Cr represent the blue color difference and red color difference, respectively.

By isolating the components Cb and Cr, which are less affected by lighting, from the facial color area and ensuring that the image area falls within a predefined range of Cb and Cr values, you have set up a method to estimate the facial area. Subsequently, noise is removed, and various processing techniques are applied to detect the facial area.

This approach takes advantage of the characteristics of YCbCr color space to enhance facial area extraction and reduce the impact of lighting variations on the results.

## Outline
1.	Convert RGB to YCbCr to create Y, Cb, and Cr channels.
2.	Combine Cb and Cr channels as the skin color information exists in these two channels, estimating the common facial area.
3.	Perform binary segmentation to differentiate between the background and estimated facial area.
4.	Apply erosion, dilation, and Gaussian filters for noise reduction.
5.	Remove false detection information through detection area size and overlap checks.
6.	Distinguish between hand and facial areas by comparing white areas predicted as skin color.
7.	Create blue rectangles on the areas identified as faces.
8.	Output the number of detected faces based on the drawn rectangles.

## Algorithm Explanation

```C++
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
```

First, convert the RGB color model to YCbCr and separate each channel. Then, if the region falls within the predefined Cb and Cr range values, set it as white; otherwise, set it as black, and perform binarization.

```C++
        //침식, 팽창, 가우시안을 통한 노이즈 제거
        erode(result, result, Mat::ones(Size(3, 3), CV_8UC1), Point(-1, -1), 3);
        dilate(result, result, Mat::ones(Size(3, 3), CV_8UC1), Point(-1, -1), 3);
        GaussianBlur(result, result, Size(5, 5), 0);
```
노이즈 제거를 위해 영상 처리 및 응용 시간에 배운 침식, 팽창, 가우시안 필터를 이용하였습니다.

![image](https://github.com/MIN60/face_detection/assets/49427080/e1d99df1-4df5-4f00-99f0-0dd11e927bf6)

이진화를 진행하였을 때의 실험 영상입니다.

```C++
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
```

findContours 함수를 통해 검출된 윤곽을 토대로 1차로 얼굴과 손을 구분해줍니다. 먼저 윤곽의 넓이가 임의로 지정한 최소 넓이보다 크고, 최대 크기보다 작으면 검출을 시도합니다. 
그 다음 윤곽 영역의 넓이와 윤곽을 감싸는 최소 크기의 사각형 박스의 넓이를 비교하여 얼굴 여부를 판별합니다. 얼굴의 윤곽은 동그란 모습이기 때문에 얼굴 윤곽 영역을 감싸는 사각형 박스의 경우 얼굴 영역과 많은 부분이 겹쳐 있습니다. 하지만 펼친 손의 경우 얼굴이 비해 상대적으로 박스 내부에서 손 영역이 차지하는 비율이 적습니다. 이를 이용해 윤곽의 영역의 넓이가 윤곽을 감싸는 박스의 넓이의 0.5퍼센트보다 작은 경우 얼굴이 아니라고 판별했습니다. 
이후 윤곽의 둘레와 윤곽을 감싸는 박스의 둘레를 비교하여 얼굴 여부를 판별합니다. 얼굴의 윤곽은 보통 동그란 모양으로 일정합니다. 하지만 손의 경우 손의 윤곽을 따라 그려지기 때문에 얼굴을 검출한 영역의 둘레보다 긴 둘레를 가집니다. 이를 이용하여 윤곽을 감싸는 박스의 둘레와 비교하여 박스의 둘레의 1.5배보다 작은 경우에만 얼굴이라고 판별했습니다. 


![image](https://github.com/MIN60/face_detection/assets/49427080/dbbd823e-0492-4cb3-b08a-8c05c4ac31cd)

```C++
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
```

실험 시 얼굴 영역 박스 안에 또 박스가 생기는 문제점이 있었습니다. 이를 방지하기 위해서 겹치는 영역의 경우에는 표시하지 않도록 알고리즘을 구성하였습니다.

```C++
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
```

2차로 손과 얼굴을 구분하기 위한 알고리즘입니다. 먼저 손과 얼굴의 특징을 생각했습니다. 이진화 한 영상에서 손의 경우에는 손가락으로 인해 연결된 흰색 부분이 끊기거나 면적이 작습니다. 반대로 얼굴의 이마는 끊긴 부분 없이 연결된 모습입니다. 이를 이용하여 얼굴 영역이라고 인식된 박스의 상단에서 50픽셀정도 떨어진 부분의 행을 검사하여 흰색 부분이 박스 너비의 50% 이상이 되지 않으면 얼굴 영역이 아닌 것으로 판단하고, 50% 이상일 경우 얼굴영역으로 판단하여 파란색 박스를 그리도록 알고리즘을 구상하였습니다.

![image](https://github.com/MIN60/face_detection/assets/49427080/1cb6e37d-be46-49b7-9c9b-a2cce52e4c87)

이마와 손가락의 빨간 줄이 검사 위치입니다.

```C++
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
```

마지막으로 얼굴이 검출된 영상을 띄우고 표시되는 영상 하단에 검출된 얼굴의 개수를 표시합니다. 

## 결과 이미지

![image](https://github.com/MIN60/face_detection/assets/49427080/46fd8200-9400-4f75-ad0d-995df5a21039)
![image](https://github.com/MIN60/face_detection/assets/49427080/81495442-cca0-46c1-a541-114836a52ebc)









