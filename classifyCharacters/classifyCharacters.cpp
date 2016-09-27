/*
ayoungprogrammer.blogspot.com

Part 2: Training: Classifying characters
*/

#include <iostream>
 #include <Windows.h>

#ifdef _CH_
#pragma package <opencv>
#endif

#ifndef _EiC
#include <cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <ml.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#endif

using namespace std;
using namespace cv;


bool createDir(const std::string& dirName_in)
{
  if (CreateDirectory(dirName_in.c_str(), NULL)){
	return true;
  }else return false;
}

int numOfFiles(char* searchPath)
{
	WIN32_FIND_DATA	FindData;
	HANDLE		hFiles;
	LPTSTR		lptszFiles[100];
	UINT		nFileCount = 0;

	hFiles = FindFirstFile(searchPath, &FindData);

	if (hFiles == INVALID_HANDLE_VALUE)
		return 0;

	bool bFinished = false;
	while(!bFinished){
		if(!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
			nFileCount++;
		}

		if(!FindNextFile(hFiles, &FindData)){
			bFinished = true;
		}
	}
	FindClose(hFiles);
	return nFileCount;
}



class comparator{
public:
	bool operator()(vector<Point> c1,vector<Point>c2){

		return boundingRect( Mat(c1)).x<boundingRect( Mat(c2)).x;

	}

};


void extractContours(Mat& image,vector< vector<Point> > contours_poly){

	vector<Mat> extracted;

	sort(contours_poly.begin(),contours_poly.end(),comparator());


	char curDir[256];
	GetCurrentDirectory(256,curDir);

	stringstream fileOutput;
	fileOutput<<curDir<<"\\output\\";

	createDir("output");





	//Loop through all contours to extract
		 for( int i = 0; i< contours_poly.size(); i++ ){

			Rect r = boundingRect( Mat(contours_poly[i]) );


			Mat mask = Mat::zeros(image.size(), CV_8UC1);
			//Draw mask
			drawContours(mask, contours_poly, i, Scalar(255), CV_FILLED);

			//Check for equal sign (2 dashes on top of each other) and merge
			if(i+1<contours_poly.size()){
				Rect r2 = boundingRect( Mat(contours_poly[i+1]) );
				//Shape on top of another shape
				if(abs(r2.x-r.x)<20){
					drawContours(mask, contours_poly, i+1, Scalar(255), CV_FILLED);
					i++;
					int minX = min(r.x,r2.x);
					int minY = min(r.y,r2.y);
					int maxX =  max(r.x+r.width,r2.x+r2.width);
					int maxY = max(r.y+r.height,r2.y+r2.height);
					r = Rect(minX,minY,maxX - minX,maxY-minY);

					//Ignore equal signs
					if((double)r.width/r.height>3.0){
						continue;
					}

				}
			}

			//Ignore dashes
			if((double)r.width/r.height>3.0){


				continue;

			}


			//Extract shape from mask
			 Mat extractPic;
			 image.copyTo(extractPic,mask);
			 Mat resizedPic = extractPic(r);//extractPic(cv::Range(r.x, r.y), cv::Range(r.x+r.width, r.y+r.height));

			cv::Mat image=resizedPic.clone();
			imshow("image",image);
			char ch  = waitKey(0);

			stringstream outputFile;
			outputFile<<fileOutput.str()<<ch;

			createDir(outputFile.str());

			//Search for jpgs
			stringstream searchMask;
			searchMask<<outputFile.str()<<"\\*.jpg";

			//Write to file
			int n = numOfFiles((char*)searchMask.str().c_str());
			outputFile<<"\\"<<n<<".jpg";
			imwrite(outputFile.str(),resizedPic);


		 }



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

  //Rotate the points from calculated angle
  std::vector<cv::Point> points;
  cv::Mat_<uchar>::iterator it = img.begin<uchar>();
  cv::Mat_<uchar>::iterator end = img.end<uchar>();
  for (; it != end; ++it)
    if (*it)
      points.push_back(it.pos());


  cv::Point2f vertices[4];

  cv::RotatedRect box = cv::minAreaRect(cv::Mat(points));
   double angle = box.angle;
  if (angle < -45.)
    angle += 90.;

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
  cv::imshow("Cropped", cropped);

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
	for( int i = 0; i < contours.size(); i++ ) {
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
  extractContours(cropped3,validContours);

cv::waitKey(0);

}


dorosz je buraki w lesie a kancik je kartofli i wtedy był sobie pies i zdechł i wtedy przyszła krowa smierci i rozzpaczy

int main(void){

char fileName[256];
cin>>fileName;
getContours(fileName);

}
