#include <iostream>
#include <ros/ros.h>
#include <algorithm>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <std_msgs/Empty.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/features2d/features2d.hpp>

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <sophus/se3.hpp>
#include <eigen_conversions/eigen_msg.h>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <opencv2/core/eigen.hpp>
#include "point_corr.hpp"
#include <std_msgs/String.h>

using namespace cv;
using namespace std;

Mat image2;
bool keyboardEventReceived=false ;
int frame_no = 1;

// float focal_length_x=0.771557;
// float focal_length_y=1.368560;
 //float c_x=0.552779;
 //float c_y=0.444056;
 //float scew=1.156010;

 float focal_length_x=579.845976;
 float focal_length_y=578.585421;
 float c_x=349.370674;
 float c_y=167.103928;
 float scew=0.0;
vector<float> distCoeff;
 Mat K1 = Mat(3,3, CV_32F, cvScalar(0.));

ros::Publisher pub_com ;

struct Xgreater {
	bool operator()(const DMatch& matchx, const DMatch& matchr) const {
		return matchx.distance < matchr.distance;

	}
};




int run_eight_point_algo_on_matches(vector<DMatch> matches, vector<KeyPoint> keyPoints1,vector<KeyPoint> keyPoints2, Mat image1, Mat image2 );

void drawEpilines(Mat fMatrix,std::vector<Point2f> objPoints,std::vector<Point2f> scenePoints, Mat image1, Mat image2){



}

void gotoCommand(float x, float y, float z , float yaw ){
   cout << "sending goto: " << x  << " " << y << " " << z << endl ;

   std_msgs::String msg;
    
    char buf[50] ;
    sprintf(buf,"c moveByRel %f %f %f %f",x,y,z,yaw);
    msg.data = buf;
    pub_com.publish(msg);

}
void setRefCommand( ){
    std_msgs::String msg;
    
    char buf[50]="c setReference $POSE$";
    //sprintf(buf,"c goto %f %f %f %f");
    msg.data = buf;
    pub_com.publish(msg);

}




double getYawAngleFromRotationMatrix(Mat rotationMatrix){

	    Eigen::Matrix3f c;
	     
	     cv2eigen(rotationMatrix,c);
	
  //	     cout << "\nEigen Rotation Matrix" << rotationMatrix << endl ;

             Eigen::Quaternionf orientation(c);
             
              double x=orientation.x();
              double y=orientation.y();
              double z=orientation.z();
              double w=orientation.w();

	      cout <<"X:" << x<< "Y" << y<< "Z::" << z<< "W::" << w <<std::endl ;
 
              double  current_yaw_angle   =  atan2(2*x*y + 2*w*z, w*w + x*x - y*y - z*z);

                       // double roll=(control_force[0]*sin(current_yaw_angle)- control_force[1]*cos(current_yaw_angle))/g;
                       // double pitch=(control_force[0]*cos(current_yaw_angle) + control_force[1]*sin(current_yaw_angle))/g 
	     return current_yaw_angle;
}

void getFramesFromVideo(Mat image) {
	cout << "Frame No" << frame_no << "\n" << endl;
	cout << "Frame Size:: \n Rows" << image.rows << "\nCols" << image.cols
			<< "\nSize" << image.size() << endl;

	cout << "Saving image" << endl;

	String result = "video_seq_images/frames_seq_"
			+ boost::lexical_cast<std::string>(frame_no) + ".png";
	try {
		imwrite(result, image);
	} catch (runtime_error& ex) {
		fprintf(stderr, "Exception converting image to PNG format: %s\n",
				ex.what());

	}
	frame_no = frame_no + 1;

}

void getGoodMatches(vector<DMatch> &good_matches, vector<DMatch> matches,
		Mat image1, vector<KeyPoint> keyPoints1, Mat image2,
		vector<KeyPoint> keyPoints2) {

	double max_dist = 0;
	double min_dist = 100;


	//This is always less than descriptor 1
	int matches_size = matches.size();

	//-- Quick calculation of max and min distances between keypoints
	for (int i = 0; i < matches_size; i++) {
		double dist = matches[i].distance;
		if (dist < min_dist)
			min_dist = dist;
		if (dist > max_dist)
			max_dist = dist;
	}

	cout << "Initial good_matches size :" << good_matches.size() << endl;
	//printf("-- Max dist : %f \n", max_dist);
	//printf("-- Min dist : %f \n", min_dist);

	//-- Draw only "good" matches (i.e. whose distance is less than 2*min_dist,
	//-- or a small arbitary value ( 0.02 ) in the event that min_dist is very
	//-- small)
	//-- PS.- radiusMatch can also be used here.

	for (int i = 0; i < matches_size; i++) {
		//if (matches[i].distance <= max(2 * min_dist, 0.02)) {
		if (matches[i].distance <= max(2 * min_dist, 0.02)) {
			good_matches.push_back(matches[i]);
		}
	}


	Mat img_matches;
	drawMatches(image1, keyPoints1, image2, keyPoints2, good_matches, img_matches,
			Scalar::all(-1), Scalar::all(-1), vector<char>(),
			DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

	//-- Show good matches
//	imshow("good_matches", img_matches);

	//		for (int i = 0; i < (int) good_matches.size(); i++) {
	//			printf("-- Good Match [%d] Keypoint 1: %d  -- Keypoint 2: %d  \n",
	//					i, good_matches[i].queryIdx, good_matches[i].trainIdx);
	//		}

	cout << "good_matches size :" << good_matches.size() << endl;

	return ;

	//		// Sort them in the order of their distance.
	//		 sort(matches.begin(), matches.end(),Xgreater());
	//		 matches.resize(2);
	////		 cout << "Ist match distance" << matches.at(0).distance  << endl ;
	////		 cout << "2st match distance" << matches.at(1).distance  << endl ;
	////		 cout << "3rd match distance" << matches.at(2).distance  << endl ;
	////		 cout << "4th match distance" << matches.at(3).distance  << endl ;
	////		 cout << "5th match distance" << matches.at(4).distance  << endl ;
	//		 cout << "No of matches found " << matches.size() << endl ;
	//		 Mat img_matches;
	//				 if(matches.size() > 0){
	//				 	 drawMatches(image1, keyPoints1, image2, keyPoints2,
	//						matches, img_matches,Scalar::all(-1), Scalar::all(-1),
	//			               vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
	//
	//				//-- Show detected matches
	//				//	 cv::imshow("video_stream", image1);
	//				 	 cout << "drawing matches" << endl ;
	//				}
	//				imshow("matches", img_matches);

	//

}

void findAllMatchesImage1To2(vector<DMatch> &matches,Mat image1,
		vector<KeyPoint> &keyPoints1, Mat &descriptors1,Mat image2,
		vector<KeyPoint> &keyPoints2, Mat &descriptors2) {

	//Fast is  not scale invariant
	//		cv::FeatureDetector * detector = new cv::OrbFeatureDetector();
	//		//ORB descriptor is extension of BRIEF //not scale invariant
	//		cv::DescriptorExtractor * extractor = new cv::OrbDescriptorExtractor();

	Ptr<FeatureDetector> detector = FeatureDetector::create("ORB");
	Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create("ORB");

	if (detector == NULL) {
		cout << "detector not found " << endl;
		return ;
	}
	if (extractor == NULL) {
		cout << "descriptor not found " << endl;
		return ;
	}
	// FAST(gray,keypoints,threshold,true);

	//	cout  << "Image1 Rows: " << image1.rows << "\nImage1 Cols" << image1.cols << endl ;
	//	cout  << "Image2 Rows: " << image2.rows << "\nImage2 Cols" << image2.cols << endl ;

	detector->detect(image1, keyPoints1);
	detector->detect(image2, keyPoints2);

	//		cout << "Before No of keypoints in image1" << keyPoints1.size() << endl ;
	//		cout << "Before No of keypoints in image2" << keyPoints2.size() << endl ;

	extractor->compute(image1, keyPoints1, descriptors1);

	extractor->compute(image2, keyPoints2, descriptors2);

	if (descriptors1.type() != CV_32F) {
		descriptors1.convertTo(descriptors1, CV_32F);
	}

	if (descriptors2.type() != CV_32F) {
		descriptors2.convertTo(descriptors2, CV_32F);
	}

	cout << "\nNo of keypoints in image1 :: " << keyPoints1.size() << endl;
	cout << "\nNo of keypoints in image2 :: " << keyPoints2.size() << endl;
	cout << "\nDescriptor1 size" << descriptors1.size() << endl;
	cout << "\nDescriptor2 size" << descriptors2.size() << endl;

	BFMatcher matcher(NORM_L2, true);	//crosCheck=true

	matcher.match(descriptors1, descriptors2, matches);

	//-- Step 3: Matching descriptor vectors using FLANN matcher
	//		FlannBasedMatcher matcher;
	//		std::vector<DMatch> matches;
	//
	//		matcher.match(descriptors1, descriptors2, matches);

	cout << "Desc rows" << descriptors1.rows << endl;
	//cout << "Matches Size Inside" << matches.size() << endl;

	//-- Draw ALL matches
//	Mat img_matches;
//	drawMatches(image1, keyPoints1, image2, keyPoints2, matches, img_matches,
//			Scalar::all(-1), Scalar::all(-1), vector<char>(),
//			DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

	//-- Show detected matches
	//imshow("matches", img_matches);

}

int calculate(Mat image1){

                //getFramesFromVideo(image2);
                   	Mat gray1;

                        vector<KeyPoint> keyPoints1;
                        vector<KeyPoint> keyPoints2;
                        Mat descriptors1;
                        Mat descriptors2;

                        std::vector<DMatch> matches;

                        findAllMatchesImage1To2(matches,image1,  keyPoints1,
                                        descriptors1,image2, keyPoints2, descriptors2);

                        cout << "-- Matches Size: " << matches.size() << endl;

                        std::vector<DMatch> good_matches;

                        getGoodMatches(good_matches, matches,image1,keyPoints1,image2,keyPoints2);

                        int matches_size = matches.size();

                    //8 point algorithm

			int status=0 ;
                        if (matches_size >= 8) {
                                cout << "Going to run 8 point algo" << endl;
                            status=run_eight_point_algo_on_matches(matches,keyPoints1,keyPoints2,image1,image2);

                        }else{
                                cout << "not enough points for homography/8-point algo " << endl;
                        }
			return status ;
}

void keyboardCallback(const std_msgs::Empty& toggle_msg){
	std::cout << "keyboard event received" << std::endl ;
	keyboardEventReceived=true ;	
}

void imageCallback(const sensor_msgs::ImageConstPtr& msg) {

//cout << "image callback received" << endl ;
	try {
	if(keyboardEventReceived){
		  cout << "going to process current frame " << endl ;
		  Mat  image1 = cv_bridge::toCvShare(msg, "bgr8")->image;
		  Mat undistort_image1;
	          undistort(image1,undistort_image1,K1,distCoeff);	
	       	int status=0;
	
		image1=undistort_image1;
		//gotoCommand(1,0,0.7,0);
		//gotoCommand(0,0,0.5,0);
		
			status=calculate(image1);

		if(status == 1){
			cout << "Success" << endl ;
			keyboardEventReceived=false ;
		}else {	
			cout << "failed.trying for next frame" << endl ;
			keyboardEventReceived=false ;		
}

	}		// cv::imshow("video_stream", image2);

     	} catch (...) {

	}

}

int run_eight_point_algo_on_matches(vector<DMatch> matches, vector<KeyPoint> keyPoints1,vector<KeyPoint> keyPoints2, Mat image1, Mat image2 )
{
    std::cout<<"\ngoing to run 8 point algorithm to determine rotation and translation" << std::endl;

	//-- Localize the object
	std::vector<Point2f> objPoints;
	std::vector<Point2f> scenePoints;
        std::vector<Point2f> img1InlierPoints;
        std::vector<Point2f> img2InlierPoints;

//	CvMat* points1Matrix;
//	CvMat* points2Matrix;

	int matches_size=matches.size();

//	int matches_size=8 ;
//	points1Matrix = cvCreateMat(2, matches_size, CV_32F);
//	points2Matrix = cvCreateMat(2, matches_size, CV_32F);

	for (int i = 0; i < matches_size; i++) {

		int image1Idx = matches[i].queryIdx;
		int image2Idx = matches[i].trainIdx;

		objPoints.push_back(keyPoints1[image1Idx].pt);
		scenePoints.push_back(keyPoints2[image2Idx].pt);

//		cvmSet(points1Matrix, 0, i, keyPoints1[image1Idx].pt.x);
//		cvmSet(points1Matrix, 1, i, keyPoints1[image1Idx].pt.y);

//		cvmSet(points2Matrix, 0, i, keyPoints2[image1Idx].pt.x);
//		cvmSet(points2Matrix, 1, i, keyPoints2[image1Idx].pt.y);

	}

//	cout << "\nPoints1 Matrix Size Rows::" << points1Matrix->rows << "\nCols::"
//			<< points1Matrix->cols << endl;

//	CvMat* fundMatr;
//	CvMat* status ;

//	fundMatr = cvCreateMat(3, 3, CV_32F);
//	status = cvCreateMat(1,matches_size,CV_8UC1);

	//see opencv manual for other options in computing the fundamental matrix
//	int num = cvFindFundamentalMat(points1Matrix, points2Matrix, fundMatr,
//	CV_FM_RANSAC, 1.0, 0.9999,status);
	
//		cv::Mat fMatrix=fundMatr ;

		//printf("\nFundamental matrix was found\n ");
		//cout << fMatrix << "\n" << endl ;
////////////////////////////////////////////////
	std::vector<cv::Point2f> undistort_img1_points;
        std::vector<cv::Point2f> undistort_img2_points;
	std::vector<float> zerodist;
	cv::undistortPoints(objPoints,undistort_img1_points,K1,distCoeff);
	cv::undistortPoints(scenePoints,undistort_img2_points,K1,distCoeff);


	vector<uchar> states,dist_states,undist_states;	
	Mat fMatrix1=findFundamentalMat(objPoints, scenePoints, FM_RANSAC, 1.0, 0.9999,dist_states);		
	Mat fMatrixUndistort=findFundamentalMat(undistort_img1_points, undistort_img2_points, FM_RANSAC, 1.0, 0.9999,undist_states);		
//	Mat hMatrix1=findHomography(objPoints, scenePoints, CV_RANSAC, 1.0);		
	Mat fMatrix2;
	Mat fMatrixUndistort2;
	fMatrix1.convertTo(fMatrix2,CV_32F) ;
	fMatrixUndistort.convertTo(fMatrixUndistort2,CV_32F);
	cout << "\nF computed ::" << fMatrix2.type() << "\n" << fMatrix2 << endl ;
	
	cout << "\nF Undistorted computed ::"  << fMatrixUndistort2 << endl ;
//	cout<<" \nH computed :: " << hMatrix1<<endl;
//	Mat poseFromH;
//	cameraPoseFromHomography(hMatrix1, poseFromH);
//	cout<<"Pose from H ::"<<poseFromH<<endl; 

	
	states = dist_states;
	for(int i=0;i<states.size();i++){
		if(states[i]==1){
			img1InlierPoints.push_back(objPoints[i]);
			img2InlierPoints.push_back(scenePoints[i]);
		}	
	}

		cout<<" Num Inlier Points : "<<img2InlierPoints.size()<<endl;	
		cout << "\nWill find rotation and traslation from fundamental matrix" << endl ;

               
		std::cout << "\nCamera Intrinsic Matrix:: " << K1 << "\n" << std::endl ;
	
                //std::cout << "\nType of fundamental Matrix" << fMatrix.type() << "\n Type of camera matrix" << cameraIntrinsicMatrix.type() << std::endl ;


//		drawEpilines(fMatrix2,objPoints, scenePoints,image1,image2 );		   				

		std::string title = "Epilines";

		std::cout << "Will draw epilines" << std::endl ;
		drawEpipolarLines<float,float>(title, fMatrix2, image1, image2, img1InlierPoints, img2InlierPoints);


		Mat hard_F(3, 3, CV_32F, Scalar(0));
		hard_F.at<float>(0,2)=-0.0072;
		hard_F.at<float>(1,2)=-0.0250;
		hard_F.at<float>(2,0)=0.0041;
		hard_F.at<float>(2,1)=0.0172;
		hard_F.at<float>(2,2)=0.9995;
 
		Mat essentialMatrix = K1.t() * fMatrix2 * K1;
	//	essentialMatrix = fMatrixUndistort2;
		std::cout << "UnNormalised Essential matrix" << essentialMatrix << std::endl ;
		SVD decomp = SVD(essentialMatrix);
//
		Mat U = decomp.u;
		Mat VT = decomp.vt; //(transpose of V)
                Mat W = decomp.w ;
		float det_U=determinant(U);
		float det_VT=determinant(VT);

   	        cout << "\nDeterminant of U:: " << det_U << ", Determinant of VT ::" << det_VT  << endl ;
		if (det_U < 0 || det_VT < 0) {
   			 cout << "will do svd for negated essential matrix" << endl ;
			 decomp = SVD(-essentialMatrix);
			 U=decomp.u ;
			 VT=decomp.vt ;
		         W=decomp.w ;

			 det_U=determinant(U);
			 det_VT=determinant(VT);
   			 cout << "\nDeterminant of U:: " << det_U << ", Determinant of VT ::" << det_VT << endl ;
		
		}

                std::cout << "\nMatrix U after decomposition:: " << U  << std::endl ;
                std::cout << "\nMatrix VT after decomposition:: " << VT  << std::endl ;
		std::cout << "\nDiagonal matrix W::" << W << "\n" << std::endl ;

//		//Diogonal matrix
		Mat D(3, 3, CV_32F, Scalar(0));
	       //D.at<float>(0, 0) = W.at<float>(0, 0);
		float e = (W.at<float>(0.0) +  W.at<float>(1.1))/2.0;
		D.at<float>(0,0)=e;	
	        //D.at<float>(1, 1) = W.at<float>(0, 1);
	        D.at<float>(1,1)=e;
		//D.at<float>(2, 2) = W.at<float>(0, 2);
                D.at<float>(2,2)=0;

		std::cout << "\nMatrix D::" << D << std::endl ;

		Mat finalE ;
		Mat hard_finalE(3, 3, CV_32F, Scalar(0));
		hard_finalE.at<float>(0,0) = 0.3939;
		hard_finalE.at<float>(0,1) = 13.7709;
		hard_finalE.at<float>(0,2) = -0.2684;
		hard_finalE.at<float>(1,0) = -13.1206;
		hard_finalE.at<float>(1,1) = 0.2425;
		hard_finalE.at<float>(1,2) = -5.4987;
		hard_finalE.at<float>(2,0) = 0.0068;
		hard_finalE.at<float>(2,1) = 3.5477;
		hard_finalE.at<float>(2,2) = -0.1088;

		finalE= U * D * VT; //not used anywhere
		
		SVD decomp_finalE = SVD(finalE);
		std::cout << "\nNormalised essential Matrix" << finalE << std::endl ;
		// Rz and its transpose is used

		U = decomp_finalE.u;
                VT = decomp_finalE.vt; //(transpose of V)
		

		Mat Rz(3, 3, CV_32F, Scalar(0));
		Rz.at<float>(0, 1) = -1;
		Rz.at<float>(1, 0) = 1;
		Rz.at<float>(2, 2) = 1;
		
		cout << "\nRz " << Rz << "\nRz transpose " << Rz.t() << std::endl ;

                Mat Rotation_1= U * Rz * VT ;
		cout << "\nR1.t()*R1 = " << Rotation_1.t()*Rotation_1 ;
		if(determinant(Rotation_1)<0){
			cout << "\nMinus of R1.t()*R1 = " << Rotation_1.t()*Rotation_1 ;
			Rotation_1 = - Rotation_1;
		}
		double yawAngle1 = getYawAngleFromRotationMatrix(Rotation_1);
		
		Mat Z(3, 3, CV_32F, Scalar(0));
		Z.at<float>(0,1)=1;
		Z.at<float>(1,0)=-1;
		cout << "\n Computed Rotation1:: " << Rotation_1<< "\nYaw Angle:: " << yawAngle1 << endl;
               	Mat T_hat1 = U * Z * U.t();
		cout << "\nComputed Transation matrix1:: " << T_hat1 << std::endl ;
		

		Mat T1 = Mat(3,1,CV_32F);
		T1.at<float>(0,0)=-T_hat1.at<float>(1,2) ;
		T1.at<float>(0,1)= T_hat1.at<float>(0,2)  ;	
		T1.at<float>(0,2)=-T_hat1.at<float>(0,1);
		
		cout << "\nTranslation vector1:: " << T1 << endl ;
	
		Mat Rotation_2= U * Rz.t() * VT;
		cout << "\nR2.t()*R2 = " << Rotation_2.t()*Rotation_2 ;
		if(determinant(Rotation_2)<0){
		cout << "\nR2.t()*R2 = " << Rotation_2.t()*Rotation_2 ;
			Rotation_2 = - Rotation_2;
		}
		double yawAngle2 = getYawAngleFromRotationMatrix(Rotation_2);
	        
		cout << "Computed rotation2:: " <<  Rotation_2 << "\nYaw Angle2:: " << yawAngle2 << endl;;
		Mat T_hat2 = U * Z.t() * U.t();
		cout << "\nComputed Transation matrix2:: " << T_hat2 << std::endl ;


		Mat T2 = Mat(3,1,CV_32F);
		
		T2.at<float>(0,0)=-T_hat2.at<float>(1,2) ;
		T2.at<float>(0,1)= T_hat2.at<float>(0,2)  ;
		T2.at<float>(0,2)=-T_hat2.at<float>(0,1);
		
		cout << "\nTranslation vector2:: " << T2 << endl ;
		
		Mat validR = Mat(3,3,CV_32F);
		Mat validT = Mat(3,1,CV_32F);
	//	std::cout<<"R1 = "<<Rotation_1<<endl;
	//	std::cout<<"R2 = "<<Rotation_2<<endl;
	//	std::cout<<"T1 = "<<T1<<endl;
	//	std::cout<<"T2 = "<<T2<<endl;
		validateRandT(K1,K1, distCoeff,Rotation_1,Rotation_2,T1,T2,image1,image2,img1InlierPoints,img2InlierPoints,validR,validT);

		Mat new_camera_center=  -validR.t()*validT ; 
		float x=new_camera_center.at<float>(0,0);
		float y=new_camera_center.at<float>(1,0);
		float z=new_camera_center.at<float>(2,0);

		cout << "size of M" << new_camera_center.size() << " Sending x" << x << " y" << y << " z" << z << endl ;
  
		gotoCommand(x,y,z,0);

		double yaw = getYawAngleFromRotationMatrix(validR);
	

		std::cout<<"R1 = "<<Rotation_1<<endl;
		std::cout<<"R2 = "<<Rotation_2<<endl;
		std::cout<<"T1 = "<<T1<<endl;
		std::cout<<"T2 = "<<T2<<endl;
		
		std::cout<<"Yaw:"<<yaw<<std::endl;
		std::cout<<"T ="<<validT<<std::endl;
		std::cout<<"R ="<<validR<<std::endl;
		return 1 ;

//	} else {
//		printf("Fundamental matrix was not found\n");

//	}




	//			  cout << "Will find homography NOW" << endl ;
	//			  Mat H = findHomography( obj, scene, CV_RANSAC );
	//
	//			  std::vector<Point2f> obj_corners(4);
	//			  obj_corners[0] = cvPoint(0,0);
	//			  obj_corners[1] = cvPoint( gray1.cols, 0 );
	//			  obj_corners[2] = cvPoint( gray1.cols, gray1.rows );
	//			  obj_corners[3] = cvPoint( 0, gray1.rows );
	//			  std::vector<Point2f> scene_corners(4);
	//
	//			  perspectiveTransform( obj_corners, scene_corners, H);
	//			  //-- Draw lines between the corners (the mapped object in the scene - image_2 )
	//			  line( img_matches, scene_corners[0] + Point2f( gray1.cols, 0), scene_corners[1] + Point2f( gray1.cols, 0), Scalar(0, 255, 0), 4 );
	//			  line( img_matches, scene_corners[1] + Point2f( gray1.cols, 0), scene_corners[2] + Point2f( gray1.cols, 0), Scalar( 0, 255, 0), 4 );
	//			  line( img_matches, scene_corners[2] + Point2f( gray1.cols, 0), scene_corners[3] + Point2f( gray1.cols, 0), Scalar( 0, 255, 0), 4 );
	//			  line( img_matches, scene_corners[3] + Point2f( gray1.cols, 0), scene_corners[0] + Point2f( gray1.cols, 0), Scalar( 0, 255, 0), 4 );
	//
	////			    //-- Show detected matches
	////
	//		imshow( "matches_and_object_detection", img_matches );

}


//void droneposeCb(const tum_ardrone::filter_stateConstPtr statePtr)
//{
//
//
//        // as long as no KI present:
//        // pop next KI (if next KI present).
////        while(currentKI == NULL && commandQueue.size() > 0)
////                popNextCommand(statePtr);
////
////        // if there is no current KI now, we obviously have no current goal -> send drone hover
////        if(currentKI != NULL)
////        {
////                // let current KI control.
////                this->updateControl(statePtr);
////        }
////        else if(isControlling)
////        {
////                sendControlToDrone(hoverCommand);
////                ROS_DEBUG("Autopilot is Controlling, but there is no KI -> sending HOVER");
////        }
//
//
//
//}


int main(int argc, char **argv) {
	ros::init(argc, argv, "image_listener");
	ros::NodeHandle nh;
//	cv::namedWindow("video_stream",cv::WINDOW_NORMAL);
//	cv::startWindowThread();

	//cv::namedWindow("matches", cv::WINDOW_NORMAL);
		//cv::startWindowThread();
	cv::namedWindow("good_matches", cv::WINDOW_NORMAL);
        cv::startWindowThread();



	//

     //Mat K1 = Mat(3,3, CV_32F, cvScalar(0.));
     K1.at<float>(0,0)=focal_length_x;
     K1.at<float>(0,1)=scew;
     K1.at<float>(0,2)=c_x;

     K1.at<float>(1,1)=focal_length_y;
     K1.at<float>(1,2)=c_y;

     K1.at<float>(2,2)=1;


    cv::namedWindow("handy_image",cv::WINDOW_NORMAL);
    cv::startWindowThread();

	//to check opencv version //compatibe with 2 not 3
	if (CV_MAJOR_VERSION < 3) {
		cout << "less than 3" << endl;
	} else {
		cout << "MORE than 3" << endl;
	}


//		cv::namedWindow("image1",cv::WINDOW_NORMAL);
//		cv::startWindowThread();
//		imshow("image1",image1);

//267
			image2 = imread("/usr/prakt/w041/video_seq_images/frames_seq_50.png",CV_LOAD_IMAGE_COLOR); // Read the file

			if (!image2.data)                             // Check for invalid input
			{
					cout << "Could not open or find the image2" << std::endl;
					return 0;
			}

		//	Mat image1=imread("/usr/prakt/w041/video_seq_images/frames_seq_2.png", CV_LOAD_IMAGE_COLOR) ;

      		//	if (!image1.data)                             // Check for invalid input
        	//	{
               	//			 cout << "Could not open or find the image1" << std::endl;
//                			return 0;
        	//	}

		//	Mat undistort_image1;
			Mat undistort_image2;
			//vector<float> distCoeff;// = [-0.525878 0.321315 0.000212 -0.000429 0.000000];
			distCoeff.push_back(-0.525878);
			distCoeff.push_back(0.321315);
			distCoeff.push_back(0.000212);
			distCoeff.push_back(-0.000429);
			distCoeff.push_back(0.000000);
        	//	undistort(image1,undistort_image1,K1,distCoeff);
        		undistort(image2,undistort_image2,K1,distCoeff);
		//	image1 = undistort_image1 ;
			image2 = undistort_image2;
//			calculate(image1);
//

      //Size size(640, 360);                        //the dst image size,e.g.100x100
     // resize(input, image1, size);                             //resize image


        cv::imshow("handy_image", image2);

	cout << "going to subscibe" << endl ;
//	calculate(image1);
        image_transport::ImageTransport it(nh);
//
        image_transport::Subscriber sub = it.subscribe("/ardrone/front/image_raw",
                        1, imageCallback);

	//ros::Subscriber dronepose_sub = nh.subscribe("ardrone/predictedPose", 10, droneposeCb);
        
	ros::Subscriber sub1=nh.subscribe("/my_topic",1,keyboardCallback);


        pub_com = nh.advertise<std_msgs::String>("/tum_ardrone/com",1000);
        ros::spin();

}

//Mat output;
//drawKeypoints(gray, keypoints, output);
// namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.

// imshow( "Display window", output );
// imshow( "Display window", output );                   // Show our image inside it.

