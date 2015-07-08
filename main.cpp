//
//  main.cpp
//  Contrast Division based Enhancement
//
//  Created by yearway (iyearway@gmail.com) on 2/5/15.
//

#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include "CDE.h"

using namespace std;
using namespace cv;

int main(int argc, char * argv[]) {
    string img_name;
    cout<<"Please provide the path to the input image: ";
    cin>>img_name;


    Mat in_img = imread(img_name);
    Mat out_img;
    CDE cde;
    cde.enhance(in_img, out_img);

    imshow("Input", in_img);
    imshow("Output", out_img);
    waitKey(0);
    destroyAllWindows();

    imwrite("./result.png", out_img);
    return 0;
}
