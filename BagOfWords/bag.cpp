////////////////////////////////////////////////////////////////
/////////////////////code by Arpita S Tugave
////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/calib3d/calib3d.hpp>
#include <string>
#include <sstream>
#include "ml.h"
#include <limits> 

using namespace cv; 
using namespace std;

using std::cout;
using std::cerr;
using std::endl;
using std::vector;


/*initialize global variables*/

//number of sets in database
int data_number = 4;
//number of images in each set
int image_number = 120;
int addset = (image_number/3)-1;

//matcher, extractor, and detector
Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("FlannBased");

//SURF
cv::DescriptorExtractor * extractor = new cv::SURF();
cv::FeatureDetector * detector = new cv::SURF(500.0);

//SIFT
//cv::DescriptorExtractor * extractor = new cv::SIFT();
//cv::FeatureDetector * detector = new cv::SIFT();

//FAST
//cv::DescriptorExtractor * extractor = new cv::SIFT();
//cv::FeatureDetector * detector = new cv::FastFeatureDetector();

//BRISK
//cv::DescriptorExtractor * extractor = new cv::SIFT();
//cv::FeatureDetector * detector = new cv::BRISK();

//STAR
//cv::DescriptorExtractor * extractor = new cv::SIFT();
//cv::FeatureDetector * detector = new cv::StarFeatureDetector();

//MSER
//cv::DescriptorExtractor * extractor = new cv::SIFT();
//cv::FeatureDetector * detector = new cv::MSER();

//GFFT
//cv::DescriptorExtractor * extractor = new cv::SIFT();
//cv::FeatureDetector * detector = new cv::GFTTDetector();

//DENSE don't use this one as it is too slow
//cv::DescriptorExtractor * extractor = new cv::SIFT();
//cv::FeatureDetector * detector = new cv::DenseFeatureDetector();

//bowtrainer
int dictionarySize = 1500;
TermCriteria tc(CV_TERMCRIT_ITER, 10, 0.001);
int retries = 1;
int flags = KMEANS_PP_CENTERS;
BOWKMeansTrainer bowTrainer(dictionarySize, tc, retries, flags);
BOWImgDescriptorExtractor bowDE(extractor, matcher);

//class to extract features
void ClassExtractFeatures(int first_set, int second_set ) {

	int first_set1 = first_set + addset;
	int second_set1 = second_set + addset;

	IplImage *img;
	int i1,i2,j;
	for(j=1;j<=data_number;j++)
	{
	for(i1=first_set;i1<=first_set1 ;i1++){

	std::ostringstream convert;
	convert << "/home/arpita/Documents/bagOfWords/BagOfWords/dataset/"<< j << "/"  << i1 << ".jpg";
                    img = cvLoadImage(convert.str().c_str(),0);
					vector<KeyPoint> keypoint;
					detector->detect(img, keypoint);
					Mat features;
					extractor->compute(img, keypoint, features);
					bowTrainer.add(features);
	}
	for(i2=second_set;i2<=second_set1;i2++){

	std::ostringstream convert;
	convert << "/home/arpita/Documents/bagOfWords/BagOfWords/dataset/"<< j << "/"  << i2 << ".jpg";
                    img = cvLoadImage(convert.str().c_str(),0);
					vector<KeyPoint> keypoint;
					detector->detect(img, keypoint);
					Mat features;
					extractor->compute(img, keypoint, features);
					bowTrainer.add(features);
	}
	}
return;
}


int main()
{

	//set the timer
        clock_t t1,t2;
        t1=clock();

	//store response
	FileStorage fs("response.yml", FileStorage::WRITE);

	//initialize cross validation numbers
	int first_set = 1;
	int second_set =41;
	int third_set = 81;

	int first_set1 = first_set + addset;
	int second_set1 = second_set + addset;
	int third_set1 = third_set + addset;

	int i1,i2,i,j;
	IplImage *img2;
	
	//collect extract features
	ClassExtractFeatures(first_set,second_set);

	//get all descriptors
	vector<Mat> descriptors = bowTrainer.getDescriptors();

	int count=0;
	for(vector<Mat>::iterator iter=descriptors.begin();iter!=descriptors.end();iter++)
	{
		count+=iter->rows;
	}
	cout<<"Clustering "<<count<<" features"<<endl;


	//choosing cluster's centroids as dictionary's words
	Mat dictionary = bowTrainer.cluster();
	bowDE.setVocabulary(dictionary);

	//initialize labels and training data
	Mat labels(0, 1, CV_32FC1);
	Mat trainingData(0, dictionarySize, CV_32FC1);

	//initialize keypoint and bowdescriptor
	vector<KeyPoint> keypoint1;
	Mat bowDescriptor1;

	//extracting histogram in the form of bow for each image 
	for(j=1;j<=data_number;j++)
	{
	for(i1=first_set;i1<=first_set1 ;i1++){

	std::ostringstream convert;
	convert << "/home/arpita/Documents/bagOfWords/BagOfWords/dataset/"<< j << "/"  << i1 << ".jpg";
			//load image, detect features
			img2 = cvLoadImage(convert.str().c_str(),0);
			detector->detect(img2, keypoint1);
			//stack in features in bow dictionary and training data
			bowDE.compute(img2, keypoint1, bowDescriptor1);
			trainingData.push_back(bowDescriptor1);
			labels.push_back((float) j);
	}
	for(i2=second_set;i2<=second_set1;i2++){

	std::ostringstream convert;
	convert << "/home/arpita/Documents/bagOfWords/BagOfWords/dataset/"<< j << "/"  << i2 << ".jpg";
			//load image, detect features
			img2 = cvLoadImage(convert.str().c_str(),0);
			detector->detect(img2, keypoint1);
			//stack in features in bow dictionary and training data
			bowDE.compute(img2, keypoint1, bowDescriptor1);
			trainingData.push_back(bowDescriptor1);
			labels.push_back((float) j);
	}
	}
		


	//Setting up SVM parameters
	CvSVMParams params;
	params.kernel_type=CvSVM::RBF;
	params.svm_type=CvSVM::C_SVC;
	params.gamma=0.50625000000000009;
	params.C=312.50000000000000;
	params.term_crit=cvTermCriteria(CV_TERMCRIT_ITER,100,0.000001);
	CvSVM svm;

	bool res=svm.train(trainingData,labels,cv::Mat(),cv::Mat(),params);
	//knn.train(trainingData,labels);

	//store actual label
	Mat groundTruth(0, 1, CV_32FC1);
	//store detected label
	Mat results(0, 1, CV_32FC1);
	//iniatize evalData, keypoints and descriptor
	Mat evalData(0, dictionarySize, CV_32FC1);
	vector<KeyPoint> keypoint2;
	Mat bowDescriptor2;
	//inialize svm and knn results
	float response;
	float ret;
	float resul;

	for(j=1;j<=data_number;j++)
	for(i=third_set;i<=third_set1;i++){

	std::ostringstream convert;
	convert << "/home/arpita/Documents/bagOfWords/BagOfWords/dataset/"<< j << "/"  << i << ".jpg";

			//load image and match in dictionary
			img2 = cvLoadImage(convert.str().c_str(),0);
			detector->detect(img2, keypoint2);
			bowDE.compute(img2, keypoint2, bowDescriptor2);
			evalData.push_back(bowDescriptor2);
			//store actual label
			groundTruth.push_back((float) j);
			//predict in svm
			response = svm.predict( bowDescriptor2 );
			//ret,resul = knn.find_nearest( bowDescriptor2, 2);
			results.push_back(response);
	
	std::ostringstream rrStore;
	rrStore << "/home/arpita/Documents/bagOfWords/BagOfWords/dataset/"<< j << "/"  << i << ".jpg =>" 
	<< response ;
	fs << "response" << rrStore.str().c_str();

	}


	//calculate the number of unmatched classes 
	double errorRate = (double) countNonZero(groundTruth- results) / evalData.rows;
	cout<<"Error rate is:" <<errorRate<<endl;
	fs << "Error rate is " << errorRate;

	fs.release();

    t2=clock();
    float diff ((float)t2-(float)t1);
    float seconds = diff / CLOCKS_PER_SEC;
    cout<<"time:" <<seconds<<endl;
    system ("pause");
    return 0;

}
