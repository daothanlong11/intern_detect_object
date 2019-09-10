// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API

using namespace cv;
using namespace std;

vector<Point2f> getCorner(rs2::depth_frame depth, int width, int height);
////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[]) try
{
    int w = 640;
    int h = 360;
    rs2::align align_to(RS2_STREAM_DEPTH);
    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    rs2::hole_filling_filter hole;
    hole.set_option(RS2_OPTION_HOLES_FILL,1);
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_COLOR, w, h, RS2_FORMAT_BGR8, 0);
    cfg.enable_stream(RS2_STREAM_DEPTH, w, h, RS2_FORMAT_Z16, 0);
    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start(cfg);


    const auto window_name = "Display Image";
    namedWindow(window_name, WINDOW_AUTOSIZE);
    
    while (waitKey(1) < 0 && getWindowProperty(window_name, WND_PROP_AUTOSIZE) >= 0)
    {
        rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
        rs2::depth_frame depth = data.get_depth_frame();
        data = data.apply_filter(hole);
        data = data.apply_filter(align_to);

        //rs2::frame imageColor = data.get_color_frame();
        //rs2::frame ir_frame = data.first(RS2_STREAM_INFRARED);
        
        rs2::frame dep_frame = data.get_color_frame();
        
        
        // Create OpenCV matrix of size (w,h) from the colorized depth data
        Mat image(Size(w, h), CV_8UC3, (void*)dep_frame.get_data(), Mat::AUTO_STEP);
        
        vector<Point2f> hai = getCorner(depth, w, h);
        cout<<"size corner: "<<hai.size()<<endl;
        if( hai.size() > 1)
        {
            for(int i = 0; i < hai.size()-1; i++ )
            line(image, hai[i], hai[(i+1)%4], Scalar(0, 255, 0), 1, LINE_AA);
        }
        
        // Update the window with new data
        imshow(window_name, image);
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

vector<Point2f> getCorner(rs2::depth_frame depth, int width, int height)
{
    int64 t0 = cv::getTickCount();
    //#define radius 6
    //float vicinityDistance = 0.025f;
    //float vicinityDistance = 0.012f;
    //float setVicinityDistance[4] = {0.003f, 0.0275f, 0.011f, 0.018f};
    
    #define radius 6
    float vicinityDistance;
    float setVicinityDistance[4] = {0.003f, 0.0275f, 0.011f, 0.021f};
    
    Mat frameDepth = Mat::zeros(Size(width, height), CV_8U);
    vector<Point2f> fourCorner;
    float objectHeight;
    
    float distanceCenter = depth.get_distance(width/2, height/2);
    float setDistance[10][2] = {0.0f};
    float maxDistance = 0.0f;
    float objectDistance = 0.0f;
    float frequencyMaxDistancePoint = 0.0f;
    float frequencyDistancePoint = 0.0f;
    float frequencyDistancePointSecondary = 0.0f;
    float sumMaxDistance = 0.0f;
    float sumObjectDistance = 0.0f;
    uint sumObjectPoint = 0;
    uint sumMaxPoint = 0;
    uint numberNext;
    uint8_t numberloop = 0;
    
    do
    {
        numberNext = 0;
        vicinityDistance = setVicinityDistance[numberloop];
        for (uint16_t x = 10; x < width-5; x+=25)
        {
            for (uint16_t y = 3; y < height-3; y+=5)
            {
                float distance = depth.get_distance(x,y);
                if ((distance > vicinityDistance)&&(distance < 2.0f))
                {
                    bool flagNext = true;
                    for (uint8_t i = 0; i < numberNext; i++)
                    {
                        float subDistance = abs(setDistance[i][0] - distance);
                        if (subDistance <= vicinityDistance)
                        {
                            setDistance[i][1]++;
                            flagNext = false;
                            break;
                        }
                    }
                    if (flagNext && (numberNext < 10))
                    {
                        setDistance[numberNext][0] = distance;
                        numberNext++;
                    }
                }
            }
        }
        numberloop++;
        cout<<"so luong: "<<numberNext<<endl;
        if (numberloop == 4)
        {
            fourCorner.push_back(Point2f(0.0f,0.0f));
            return fourCorner;
        }
    } while (numberNext > 3);
    
    
    for (uint16_t i = 0; i < numberNext; i++)
    {
        if (setDistance[i][0] > maxDistance)
        {
            maxDistance = setDistance[i][0];
            frequencyMaxDistancePoint = setDistance[i][1];
        }
        if (setDistance[i][1] > frequencyDistancePoint)
        {
            frequencyDistancePoint = setDistance[i][1];
            objectDistance = setDistance[i][0];
        }
    }
    
    if (frequencyDistancePoint == frequencyMaxDistancePoint)
    {
        for (uint16_t i = 0; i < numberNext; i++)
        {
            if ((setDistance[i][1] > frequencyDistancePointSecondary)&&(setDistance[i][1] != frequencyDistancePoint))
            {
                frequencyDistancePointSecondary = setDistance[i][1];
                objectDistance = setDistance[i][0];
            }
        }
    }

    for (uint16_t x = 5; x < width-5; x++)
    {
        for (uint16_t y = 5; y < height-5; y++)
        {
            float distance = depth.get_distance(x,y);
            if (abs(distance - objectDistance) <= vicinityDistance)
            {
                frameDepth.at<uchar>(y,x) = 255;
                sumObjectDistance += distance;
                sumObjectPoint++;
            }
            else if (abs(distance - maxDistance) <= vicinityDistance)
            {
                sumMaxDistance += distance;
                sumMaxPoint++;
            }
        }
    }

    //-------------Result Object Height--------------------//
    objectHeight = (sumMaxDistance/(float)sumMaxPoint - sumObjectDistance/(float)sumObjectPoint)*100;
    //cout<<"chieu cao vat the: "<<objectHeight<<endl;
    objectDistance = sumObjectDistance/(float)sumObjectPoint;
    /////////////////////////////////////////////////////////
    
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    Mat imageContours = frameDepth.clone();
    /// Find contours
    findContours( imageContours, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
    
    int indexmaxContours[2] = {0, 0};
    for (uint8_t i=0; i<contours.size(); i++)
    {
        if (contours[i].size() > indexmaxContours[1])
        {
            indexmaxContours[0] = i;
            indexmaxContours[1] = contours[i].size();
        }
    }

    Point2f fourPoint[4];
    RotatedRect box = minAreaRect(contours[indexmaxContours[0]]);
    box.points(fourPoint);
    
    imshow("frameDepth", frameDepth);
    for (uint8_t i = 0; i < 4; i++)
    {
        fourCorner.push_back(fourPoint[i]);
    }
    fourCorner.push_back(Point2f(objectHeight, objectDistance));
    //////////////////////////////////////////////////////////////////
    int64 t1 = cv::getTickCount();
    double secs = (t1-t0)/cv::getTickFrequency();
    cout<<"thoi gian: "<<secs<<endl;
    return fourCorner;
}
