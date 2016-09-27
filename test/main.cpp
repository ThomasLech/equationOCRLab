/*
http://ayoungprogrammer.blogspot.ca/2013/01/part-3-making-ocr-for-equations.html

Equation OCR

*/

#include <iostream>
#include "baseapi.h"

#include <Windows.h>

#ifdef _CH_
#pragma package <opencv>
#endif

#ifndef _EiC
#include <cv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <ml.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#endif

using namespace std;
using namespace cv;


std::string lang = "mat"; // file on disk is "my_trained_file.traineddata"
        tesseract::TessBaseAPI tess_api;



class comparator{
public:
        bool operator()(vector<Point> c1,vector<Point>c2){

                return boundingRect( Mat(c1)).x<boundingRect( Mat(c2)).x;

        }

};

void sendToWolfram(string eqn){


        stringstream url;
        url<<"http://www.wolframalpha.com/input/?i=";

        int i;
        for(i=0;i<eqn.length();i++){
                if(eqn[i]>='0'&&eqn[i]<='9')url<<eqn[i];
                if(eqn[i]=='-')url<<eqn[i];
                if(eqn[i]=='f')url<<eqn[i];
                if(eqn[i]=='x')url<<eqn[i];
                if(eqn[i]=='+')url<<"%2B";
                if(eqn[i]=='^')url<<"%5E";
                if(eqn[i]=='=')url<<"%3D";
                if(eqn[i]=='(')url<<"%28";
                if(eqn[i]==')')url<<"%29";


        }
        cout<<url.str()<<endl;


}




//Extracts the eqn from the mat
string extractContours(Mat& image,vector< vector<Point> > contours_poly){

        vector<Mat> extracted;

        stringstream outputText;
        sort(contours_poly.begin(),contours_poly.end(),comparator());


        //Loop through all contours to extract
                 for( int i = 0; i< contours_poly.size(); i++ ){

                        Rect r = boundingRect( Mat(contours_poly[i]) );


                        Mat mask = Mat::zeros(image.size(), CV_8UC1);
                        drawContours(mask, contours_poly, i, Scalar(255), CV_FILLED); // This is a OpenCV function

                        //Check for equal sign (2 dashes on top of each other) and merge
                        if(i+1<contours_poly.size()){
                                Rect r2 = boundingRect( Mat(contours_poly[i+1]) );
                                if(abs(r2.x-r.x)<20){
                                        drawContours(mask, contours_poly, i+1, Scalar(255), CV_FILLED); // This is a OpenCV function
                                        i++;
                                        int minX = min(r.x,r2.x);
                                        int minY = min(r.y,r2.y);
                                        int maxX =  max(r.x+r.width,r2.x+r2.width);
                                        int maxY = max(r.y+r.height,r2.y+r2.height);
                                        r = Rect(minX,minY,maxX - minX,maxY-minY);

                                        if((double)r2.width/r2.height>3){

                                                outputText<<"=";
                                                continue;

                                        }else {
                                                outputText<<"i";
                                                continue;
                                        }

                                }
                        }

                        if((double)r.width/r.height>3.0){

                                outputText<<"-";
                                continue;


                        }

                        if(r.y+r.height<image.size().height*2.0/3.0){
                                outputText<<"^";
                        }




                         Mat extractPic;
                         image.copyTo(extractPic,mask);
                         Mat resizedPic = extractPic(r);


                        //Use tesseract to use ocr
                        tess_api.TesseractRect( resizedPic .data, 1, resizedPic .step1(), 0, 0, resizedPic .cols, resizedPic .rows);
                        tess_api.SetImage(resizedPic .data,resizedPic.size().width,resizedPic .size().height,resizedPic .channels(),resizedPic .step1());
                        tess_api.Recognize(0);
                        const char* out=tess_api.GetUTF8Text();


                        //Output character to stream
                        outputText<<out[0];


                 }


                 cout<<outputText.str()<<endl;
                sendToWolfram(outputText.str());


        return outputText.str();



}

void getContours(const char* filename)
{
  cv::Mat img = cv::imread(filename, 0);


  //Apply blur to smooth edges and use adapative thresholding
   cv::Size size(3,3);
  cv::GaussianBlur(img,img,size,0);
   adaptiveThreshold(img, img,255,CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY,75,10);
  cv::bitwise_not(img, img);




  cv::Mat img2 = img.clone();


  std::vector<cv::Point> points;
  cv::Mat_<uchar>::iterator it = img.begin<uchar>();
  cv::Mat_<uchar>::iterator end = img.end<uchar>();
  for (; it != end; ++it)
    if (*it)
      points.push_back(it.pos());

  cv::RotatedRect box = cv::minAreaRect(cv::Mat(points));

   double angle = box.angle;
  if (angle < -45.)
    angle += 90.;

  cv::Point2f vertices[4];
  box.points(vertices);
  for(int i = 0; i < 4; ++i)
    cv::line(img, vertices[i], vertices[(i + 1) % 4], cv::Scalar(255, 0, 0), 1, CV_AA);



   cv::Mat rot_mat = cv::getRotationMatrix2D(box.center, angle, 1);

   cv::Mat rotated;
  cv::warpAffine(img2, rotated, rot_mat, img.size(), cv::INTER_CUBIC);



  cv::Size box_size = box.size;
  if (box.angle < -45.)
    std::swap(box_size.width, box_size.height);
  cv::Mat cropped;

  cv::getRectSubPix(rotated, box_size, box.center, cropped);

        Mat cropped2=cropped.clone();
cvtColor(cropped2,cropped2,CV_GRAY2RGB);

Mat cropped3 = cropped.clone();
cvtColor(cropped3,cropped3,CV_GRAY2RGB);

 vector<vector<Point> > contours;
  vector<Vec4i> hierarchy;

  /// Find contours
  cv:: findContours( cropped, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_TC89_KCOS, Point(0, 0) );



  /// Approximate contours to polygons + get bounding rects and circles
  vector<vector<Point> > contours_poly( contours.size() );
  vector<Rect> boundRect( contours.size() );
  vector<Point2f>center( contours.size() );
  vector<float>radius( contours.size() );


  //Get poly contours
        for( int i = 0; i < contours.size(); i++ )
     {
                 approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
     }


  //Get only important contours, merge contours that are within another
  vector<vector<Point> > validContours;
        for (int i=0;i<contours_poly.size();i++){

                Rect r = boundingRect(Mat(contours_poly[i]));
                if(r.area()<100)continue;
                bool inside = false;
                for(int j=0;j<contours_poly.size();j++){
                        if(j==i)continue;

                        Rect r2 = boundingRect(Mat(contours_poly[j]));
                        if(r2.area()<100||r2.area()<r.area())continue;
                        if(r.x>r2.x&&r.x+r.width<r2.x+r2.width&&
                                r.y>r2.y&&r.y+r.height<r2.y+r2.height){

                                inside = true;
                        }
                }
                if(inside)continue;
                validContours.push_back(contours_poly[i]);
        }


        //Get bounding rects
        for(int i=0;i<validContours.size();i++){
                boundRect[i] = boundingRect( Mat(validContours[i]) );
        }


        //Display
  Scalar color = Scalar(0,255,0);
  for( int i = 0; i< validContours.size(); i++ )
     {
        if(boundRect[i].area()<100)continue;
      drawContours( cropped2, validContours, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
       rectangle( cropped2, boundRect[i].tl(), boundRect[i].br(),color, 2, 8, 0 );
     }

  imshow("Contours",cropped2);

extractContours(cropped3,validContours);


cv::waitKey(0);

}

int main(void){



        //Init tesseract
        tess_api.Init("", lang.c_str(), tesseract::OEM_DEFAULT);
        tess_api.SetPageSegMode(static_cast<tesseract::PageSegMode>(10));

char fileName[256];

//cout<<numOfFiles("C:\\Users\\Michael\\Documents\\Visual Studio 2008\\Projects\\Project1\\OCRTest\\OCRTest\\output\\x\\*.txt");

cin>>fileName;
getContours(fileName);





}



/* Code to put in output folder
                        char cCurrentPath[256];
                        GetCurrentDirectory(sizeof(cCurrentPath),cCurrentPath );
                        fileOutput<<cCurrentPath;

                         fileOutput<<"\\output\\"<<ch;

                        cout<<dirExists(fileOutput.str())<<endl;

                        stringstream fileName;
                        fileName<<fileOutput.str();

                        fileOutput<<"\\*.jpg";

                         int n = numOfFiles((char*)fileOutput.str().c_str());
                         cout<<n<<endl;
                         fileName<<"\\"<<n<<".jpg";
                         imwrite(fileName.str(),image);
                        cout<<fileName.str()<<endl;*/
