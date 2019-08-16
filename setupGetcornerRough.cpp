// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API

using namespace cv;
using namespace std;

Mat image1;
const char* window_trackbar = "Trackbar";


////////////////////////////////////////////////////////////////////
struct cropframe
{
    int left_width = 119;
    int right_width = 468;
    int up_height = 28;
    int down_height = 443;
};

struct filter
{
    int thresh = 5;
    int block = 3;
    int lowThrescanny = 20;
};

struct cropframe crop;
struct filter parameter;

int w;
int h;


////////////////////////////////////////////////////////////////////
vector<Point2f> getCorner(Mat src);

int main(int argc, char * argv[]) try
{
    // Declare depth colorizer for pretty visualization of depth data
    rs2::rates_printer printer;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();


    const auto window_name = "Display Image";
    namedWindow(window_name, WINDOW_AUTOSIZE);
    namedWindow(window_trackbar, CV_WINDOW_AUTOSIZE);
    
    createTrackbar("Left Width:", window_trackbar, &crop.left_width, 400, 0);
    createTrackbar("Right Width:", window_trackbar, &crop.right_width, 640, 0);
    createTrackbar("Up Height:", window_trackbar, &crop.up_height, 400, 0);
    createTrackbar("Down Height:", window_trackbar, &crop.down_height, 480, 0);

    createTrackbar("Threshold:", window_trackbar, &parameter.thresh, 30, 0);
    createTrackbar("2*Block+1:", window_trackbar, &parameter.block, 10, 0);
    createTrackbar("Thres Canny:", window_trackbar, &parameter.lowThrescanny, 50, 0);

    while (waitKey(1) < 0 && getWindowProperty(window_name, WND_PROP_AUTOSIZE) >= 0)
    {
        rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
        rs2::frame imageColor = data.get_color_frame().apply_filter(printer);

        // Query frame size (width and height)
        w = imageColor.as<rs2::video_frame>().get_width();
        h = imageColor.as<rs2::video_frame>().get_height();
        //cout<<"width: "<<w<<endl;
        //cout<<"heigth: "<<h<<endl;
        // Create OpenCV matrix of size (w,h) from the colorized depth data
        Mat image(Size(w, h), CV_8UC3, (void*)imageColor.get_data(), Mat::AUTO_STEP);
        
        vector<Point2f> hai = getCorner(image);
        image1 = image.clone();

        line(image1, Point(crop.left_width, 0), Point(crop.left_width, h), Scalar(0,255,0), 1, 8);
        line(image1, Point(crop.right_width, 0), Point(crop.right_width, h), Scalar(0,255,0), 1, 8);
        line(image1, Point(0, crop.up_height), Point(w, crop.up_height), Scalar(0,255,0), 1, 8);
        line(image1, Point(0, crop.down_height), Point(w, crop.down_height), Scalar(0,255,0), 1, 8);

        // Update the window with new data
        imshow(window_name, image1);
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}



//////////////////////////////////////////////////////////////
vector<Point2f> getCorner(Mat src)
{
    ////////////////////////////////////////////////////////// 
     
    #define radius 20
    
    const int ratio = 3;
    const int kernel_size = 3;

    Mat kernel_thr = getStructuringElement(MORPH_RECT, Size(3,3));
    Mat dstGray = Mat::zeros(Size(src.cols, src.rows), CV_8U);

    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////
    //---------------------------------------------------------
    //------------getNoiseFilter-----------------------------//
    Mat srcGray, adapThreshGray, CannyGray;
    Mat cropGray;

    Mat _src;

    //////////////////////////////////////////////////////////
    cvtColor(src, srcGray, CV_BGR2GRAY);
    ///------adaptiveThreshold------------------------------//
    adaptiveThreshold(srcGray, adapThreshGray, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 2*parameter.block+1, parameter.thresh);
    morphologyEx(adapThreshGray, adapThreshGray, MORPH_CLOSE, kernel_thr);
    imshow("adapThresh", adapThreshGray);
    ///-------Canny detector--------------------------------//
    Mat element = getStructuringElement(MORPH_ELLIPSE, Size(4,4), Point(1,1));
    blur(srcGray, CannyGray, Size(3,3));
    Canny(CannyGray, CannyGray, parameter.lowThrescanny, parameter.lowThrescanny*ratio, kernel_size );
    dilate( CannyGray, CannyGray, element );
    imshow("Canny", CannyGray);
    ///--------Result: dstGray----------------------------//
    bitwise_and(adapThreshGray, CannyGray, dstGray);
    //---------------------------------------------------//
    Mat ROI(dstGray, Rect(crop.left_width,crop.up_height,(crop.right_width-crop.left_width-1),
                            (crop.down_height-crop.up_height-1)));
    ROI.copyTo(cropGray);

    Mat ROI1(src, Rect(crop.left_width,crop.up_height,(crop.right_width-crop.left_width-1),
                            (crop.down_height-crop.up_height-1)));
    ROI1.copyTo(_src);
    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////
    //--------------getCannyObject------------------------//
    const uint16_t width = cropGray.cols;
    const uint16_t height = cropGray.rows;
    imshow("cropGray",cropGray);

    
    Mat frame = Mat::zeros(Size(width, height), CV_8U);
    Mat frame1 = frame.clone();
    Mat frame2 = frame.clone();
    ///////////////////////////////////////////////////////////
    for(uint16_t i=0; i<width; i++)
    {
        uint16_t index1;
        uint16_t index2 = 0;
        for(uint16_t j=0; j<height; j++)
        {
            if (cropGray.at<uchar>(j,i) == 255)
            {
                frame1.at<uchar>(j,i) = 255;
                index1 = j+1;
                break;
            }
        }
        for(uint16_t j=index1; j<height; j++)
        {
            if (cropGray.at<uchar>(j,i) == 255)
            {
                frame1.at<uchar>(j,i) = 255;
                frame1.at<uchar>(index2,i) = 0;
                index2 = j;
            }
        }
    }
    for(uint16_t i=0; i<height; i++)
    {
        uint16_t index1;
        uint16_t index2 = 0;
        for(uint16_t j=0; j<width; j++)
        {
            if (cropGray.at<uchar>(i,j) == 255)
            {
                frame2.at<uchar>(i,j) = 255;
                index1 = j+1;
                break;
            }
        }
        for(uint16_t j=index1; j<width; j++)
        {
            if (cropGray.at<uchar>(i,j) == 255)
            {
                frame2.at<uchar>(i,j) = 255;
                frame2.at<uchar>(i,index2) = 0;
                index2 = j;
            }
        }
    }
    //---------Result: frame--------------------------------------//
    bitwise_or(frame1,frame2,frame);
    imshow("frame", frame);
    ////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////
    //-----------------getCorners---------------------------------//
    vector<Point2f> PointCorner;

    ///////////////////////////////////////////////////////////////
    vector<Vec4i> lines;
    int egdeSet[40][4] = {0};
    int fourEdge[4][4] = {0};
    int fourPoint[4][2] = {0};
    int fourPointNew[4][2] = {0};
    float ablines[4][2] = {0.0f};

    HoughLinesP(frame, lines, 1, CV_PI/180, 14, 10, 700 );
    int numberLines = lines.size();
    //cout<<"numberLines: "<<numberLines<<endl;
    uint8_t indexNext;
    float lengthMax = 0;
    for (uint8_t i = 0; i < numberLines; i++)
    {
        Vec4i l = lines[i];
        float length = (float)((l[0]-l[2])*(l[0]-l[2])+(l[1]-l[3])*(l[1]-l[3]));
        if (length > lengthMax)
        {
            lengthMax = length;
            indexNext = i;
        }
        egdeSet[i][0] = l[0];
        egdeSet[i][1] = l[1];
        egdeSet[i][2] = l[2];
        egdeSet[i][3] = l[3];
    }
    for (uint8_t index = 0; index<4; index++)
    {
        bool flag_indexNext = false;
        float a = (float)(egdeSet[indexNext][1]-egdeSet[indexNext][3])/(float)(egdeSet[indexNext][0]-egdeSet[indexNext][2]);
        float b = (float)egdeSet[indexNext][1] - a*(float)egdeSet[indexNext][0];
        int vectorU1 = egdeSet[indexNext][0]-egdeSet[indexNext][2];
        int vectorU2 = egdeSet[indexNext][1]-egdeSet[indexNext][3];
        float length = 0.0f; 
        for (int8_t i = 0; i < numberLines; i++)
        {
            float distance1 = (a*(float)egdeSet[i][0]-(float)egdeSet[i][1]+b)*(a*(float)egdeSet[i][0]
                                -(float)egdeSet[i][1]+b)/(a*a+b*b);
            float distance2 = (a*(float)(egdeSet[i][2])-(float)(egdeSet[i][3])+b)*(a*(float)(egdeSet[i][2])
                                -(float)(egdeSet[i][3])+b)/(a*a+b*b);
            float distance = distance1+distance2;
            if (distance < 0.02f)
            {
                float lengthnow = (float)(egdeSet[i][0]-egdeSet[i][2])*(float)(egdeSet[i][0]-egdeSet[i][2])
                                    +(float)(egdeSet[i][1]-egdeSet[i][3])*(float)(egdeSet[i][1]-egdeSet[i][3]);
                if (lengthnow > length)
                {
                    length = lengthnow;
                    fourEdge[index][0] = egdeSet[i][0];
                    fourEdge[index][1] = egdeSet[i][1];
                    fourEdge[index][2] = egdeSet[i][2];
                    fourEdge[index][3] = egdeSet[i][3];
                }
                numberLines--;
                egdeSet[i][0] = egdeSet[numberLines][0];
                egdeSet[i][1] = egdeSet[numberLines][1];
                egdeSet[i][2] = egdeSet[numberLines][2];
                egdeSet[i][3] = egdeSet[numberLines][3];
                i--;
            }
            else if (flag_indexNext == false)
            {
                int vectorV1 = egdeSet[i][0]-egdeSet[i][2];
                int vectorV2 = egdeSet[i][1]-egdeSet[i][3];
                float angle = (float)((vectorU1*vectorV1+vectorU2*vectorV2)
                        *(vectorU1*vectorV1+vectorU2*vectorV2))
                        /(float)((vectorU1*vectorU1+vectorU2*vectorU2)
                        *(vectorV1*vectorV1+vectorV2*vectorV2));
                if(angle <= 0.0013f)
                {
                    flag_indexNext = true;
                    indexNext = i;
                }
            }
            //cout<<"khoang cach: "<<distance<<endl;
        }
        if ((flag_indexNext==false) && (index<3))
        {
            PointCorner.push_back(Point(0.0f,0.0f));
            //cout<<"goc lon"<<endl;
            return PointCorner;
        }
        //cout<<"ket thuc vong lap 1"<<endl;
    }
    //cout<<"ket thuc vong lap 2"<<endl;
    //cout<<"NumberLines = "<<numberLines<<endl;
    if(numberLines>0)
    {
        PointCorner.push_back(Point(0.0f,0.0f));
        return PointCorner;   
    }
    //cout<<"hello dung oi"<<endl;
    ///////////////////////////////////////////////////////////////
    for (uint8_t i=0; i<4; i++)
    {
        ablines[i][0] = (float)(fourEdge[i][1]-fourEdge[i][3])/(float)(fourEdge[i][0]-fourEdge[i][2]);
        ablines[i][1] = (float)fourEdge[i][1] - ablines[i][0]*(float)fourEdge[i][0];
    }
    float temp1[1][2] = {{ablines[1][0], ablines[1][1]}};
    ablines[1][0] = ablines[2][0];
    ablines[1][1] = ablines[2][1];
    ablines[2][0] = temp1[0][0];
    ablines[2][1] = temp1[0][1];
    for (uint8_t i=0; i<2; i++)
    {
        float coordinatesX1 = ((ablines[2][1]-ablines[i][1])/(ablines[i][0]-ablines[2][0]));
        fourPoint[2*i][0] = (int16_t)coordinatesX1; 
        fourPoint[2*i][1] = (int16_t)(ablines[2][0]*coordinatesX1+ablines[2][1]);
        float coordinatesX2 = ((ablines[3][1]-ablines[i][1])/(ablines[i][0]-ablines[3][0]));
        fourPoint[2*i+1][0] = (int16_t)coordinatesX2;
        fourPoint[2*i+1][1] = (int16_t)(ablines[3][0]*coordinatesX2+ablines[3][1]);
    }
    int temp2[1][2] = {{fourPoint[2][0], fourPoint[2][1]}};
    fourPoint[2][0] = fourPoint[3][0];
    fourPoint[2][1] = fourPoint[3][1];
    fourPoint[3][0] = temp2[0][0];
    fourPoint[3][1] = temp2[0][1];
    for (uint8_t i=0; i<4; i++)
    {
        circle(_src, Point(fourPoint[i][0],fourPoint[i][1]), 1, Scalar(0,255,0),-1);
    }
    for (uint8_t index=0; index<4; index++)
    {
        if (frame.at<uchar>(fourPoint[index][1],fourPoint[index][0])==0)
        {
            uint8_t r = 1;
            int16_t pre_x = fourPoint[index][0];
            int16_t pre_y = fourPoint[index][1];
            do
            {
                int8_t a = 0,b = 0;
                int16_t x = pre_x-r;
                int16_t y = pre_y-r;
                for (uint8_t i = 0; i < 8*r; i++)
                {
                    if (i<=2*r)
                    {
                        a = i;
                    }
                    else if (i<=4*r)
                    {
                        b = i-2*r;
                    }
                    else if (i<=6*r)
                    {
                        a = 6*r-i;
                    }
                    else
                    {
                        b = 8*r-i;
                    }
                    if(frame.at<uchar>(y+b,x+a) == 255)
                    {
                        fourPoint[index][0] = x+a;
                        fourPoint[index][1] = y+b;
                        r = radius-1;
                        break;
                    }
                }
                r++; 
            } while (r < radius);
        }
    }
    for(int i=0; i<4; i++)
    {
        Vec4i l;
        l[0] = fourEdge[i][0];
        l[1] = fourEdge[i][1];
        l[2] = fourEdge[i][2];
        l[3] = fourEdge[i][3];
        line(_src, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(255,0,0), 1, 8);
    }
    for (uint8_t i=0; i<4; i++)
    {
        
        //cout<<"diem = ( "<<fourPoint[i][0]<<", "<<fourPoint[i][1]<<")"<<endl;
        circle(_src, Point(fourPoint[i][0],fourPoint[i][1]), 1, Scalar(0,0,255),-1);
        fourPoint[i][0] += crop.left_width;
        fourPoint[i][1] += crop.up_height;
        PointCorner.push_back(Point2i(fourPoint[i][0],fourPoint[i][1]));
    }
    imshow("anh", _src);
    
    //------------The Final Results Find PointCorner--------------//
    //PointCorner.push_back(pointD); 
    //////////////////////////////////////////////////////////
    
    return PointCorner;
}