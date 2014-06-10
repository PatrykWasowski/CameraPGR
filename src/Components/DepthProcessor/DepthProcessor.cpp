/*!
 * \file
 * \brief
 * \author Dawid Kaczmarek
 */

#include <memory>
#include <string>

#include "DepthProcessor.hpp"
#include "Common/Logger.hpp"
#include <Types/MatrixTranslator.hpp>

#include <boost/bind.hpp>
#include <boost/format.hpp>

namespace Processors {
namespace DepthProcessor {

DepthProcessor::DepthProcessor(const std::string & name) :
		Base::Component(name),
        algorythm_type("algorythm_type", STEREO_SGBM) {
    bm = NULL;
    sgbm = NULL;
    numberOfDisparities = 0;
    //TODO hardcoded window size
    SADWindowSize = 5;
}

DepthProcessor::~DepthProcessor() {
	if (sgbm != 0 ) delete sgbm;
	if (bm != 0 ) delete bm;
}

void DepthProcessor::prepareInterface() {
	// Register data streams, events and event handlers HERE!
	registerStream("l_in_img", &l_in_img);
	registerStream("r_in_img", &r_in_img);
	registerStream("l_cam_info", &l_in_cam_info);
	registerStream("r_cam_info", &r_in_cam_info);
	registerStream("out_depth_map", &out_depth_map);
	registerStream("out_left", &out_left_dispared);
    registerStream("out_right", &out_right_dispared);
    registerStream("out_cloud_xyz", &out_cloud_xyz);

	// Register handlers
	h_CalculateDepthMap.setup(boost::bind(&DepthProcessor::CalculateDepthMap, this));
	registerHandler("CalculateDepthMap", &h_CalculateDepthMap);
	addDependency("CalculateDepthMap", &l_in_img);
	addDependency("CalculateDepthMap", &r_in_img);
}

bool DepthProcessor::onInit() {
    sgbm = new cv::StereoSGBM();
    bm = new cv::StereoBM();
	return true;
}

bool DepthProcessor::onFinish() {
	return true;
}

bool DepthProcessor::onStop() {
	return true;
}

bool DepthProcessor::onStart() {
	return true;
}

void DepthProcessor::CalculateDepthMap() {
	LOG(LINFO) << "Init CalculateDepthMap";
	cv::Mat oLeftImage(l_in_img.read());
	cv::Mat oRightImage(r_in_img.read());
    if( algorythm_type == STEREO_BM ){
    	cv::Mat LgreyMat, RgreyMat;
    	cv::cvtColor(oLeftImage, LgreyMat, CV_BGR2GRAY);
    	cv::cvtColor(oRightImage, RgreyMat, CV_BGR2GRAY);
    	oLeftImage=LgreyMat;
    	oRightImage=RgreyMat;
    }
	try {
	LOG(LINFO) << "Get images from Data Streams";
	Types::CameraInfo oLeftCamInfo(l_in_cam_info.read());
	Types::CameraInfo oRightCamInfo(r_in_cam_info.read());

	LOG(LINFO) << "Create bunch of fresh cv::Mat objects";
	cv::StereoVar var;
    cv::Mat Q;
    cv::Mat R1, P1, R2, P2;
    cv::Size img_size = oLeftImage.size();

    cv::FileStorage fs("/home/dkaczmar/stereo/extrisinc.yml", cv::FileStorage::READ);

    fs["R1"] >> R1;
    fs["R2"] >> R2;
    fs["P1"] >> P1;
    fs["P2"] >> P2;

    fs.release();

    // Camera matrices
    cv::Mat M1 = oLeftCamInfo.cameraMatrix();
    cv::Mat M2 = oRightCamInfo.cameraMatrix();

    //Distortion coefficients matrices
    cv::Mat D1 = oLeftCamInfo.distCoeffs();
    cv::Mat D2 = oRightCamInfo.distCoeffs();


    LOG(LINFO) << "M1 "<< M1;
    LOG(LINFO) << "M2 "<< M2;
    LOG(LINFO) << "D1 "<< D1;
    LOG(LINFO) << "D2 "<< D2;
    LOG(LINFO) << "Size " << img_size.height << "x"<<img_size.width;

    LOG(LINFO) << R1;
    LOG(LINFO) << P1;
    LOG(LINFO) << R2;
    LOG(LINFO) << P2;

    cv::Mat map11, map12, map21, map22;

    LOG(LINFO) << "Calculate left transformation maps";
    cv::initUndistortRectifyMap(M1, D1, R1, P1, img_size, CV_16SC2, map11, map12);
    LOG(LINFO) << "Calculate right transformation maps";
    cv::initUndistortRectifyMap(M2, D2, R2, P2, img_size, CV_16SC2, map21, map22);

    LOG(LINFO) << "Rectification of images";
	cv::Mat oLeftRectified;
	cv::Mat oRightRectified;
    cv::remap(oLeftImage, oLeftRectified, map11, map12, cv::INTER_LINEAR);
    cv::remap(oRightImage, oRightRectified, map21, map22, cv::INTER_LINEAR);

    LOG(LINFO) << "#ofDisparities: " << numberOfDisparities;

    numberOfDisparities = numberOfDisparities > 0 ? numberOfDisparities : ((img_size.width/8) + 15) & -16;

    bm->state->preFilterCap = 31;
    bm->state->SADWindowSize = SADWindowSize > 0 ? SADWindowSize : 9;
    bm->state->minDisparity = 0;
    bm->state->numberOfDisparities = numberOfDisparities;
    bm->state->textureThreshold = 10;
    bm->state->uniquenessRatio = 15;
    bm->state->speckleWindowSize = 100;
    bm->state->speckleRange = 32;
    bm->state->disp12MaxDiff = 1;

    sgbm->preFilterCap = 63;
    sgbm->SADWindowSize = SADWindowSize > 0 ? SADWindowSize : 3;

    int cn = oLeftImage.channels();

    sgbm->P1 = 8*cn*sgbm->SADWindowSize*sgbm->SADWindowSize;
    sgbm->P2 = 32*cn*sgbm->SADWindowSize*sgbm->SADWindowSize;
    sgbm->minDisparity = 0;
    sgbm->numberOfDisparities = numberOfDisparities;
    sgbm->uniquenessRatio = 10;
    sgbm->speckleWindowSize = bm->state->speckleWindowSize;
    sgbm->speckleRange = bm->state->speckleRange;
    sgbm->disp12MaxDiff = 1;
    sgbm->fullDP = ( algorythm_type == STEREO_BM );

    cv::Mat disp, disp8;

    LOG(LINFO) << "Calculating disparity";
    int64 t = cv::getTickCount();
    if( algorythm_type == STEREO_BM )
        bm->operator ()(oLeftRectified, oRightRectified, disp);
    else if( algorythm_type == STEREO_VAR ) {
        var(oLeftRectified, oRightRectified, disp);
    }
    else if( algorythm_type == STEREO_SGBM || algorythm_type == STEREO_HH )
        sgbm->operator ()(oLeftRectified, oRightRectified, disp);
    t = cv::getTickCount() - t;
    float time = t * 1000/cv::getTickFrequency();
    LOG(LINFO) << boost::format("Time elapsed: %1%ms\n") % time;

    //disp = dispp.colRange(numberOfDisparities, img1p.cols);
    LOG(LINFO) << "Converting disparity to depth image";
    if( algorythm_type != STEREO_VAR )
        disp.convertTo(disp8, CV_8U, 255/(numberOfDisparities*16.));
    else
        disp.convertTo(disp8, CV_8U);

    LOG(LINFO) << "Calculate Q transform matrix";
    generateQ(P1,P2, Q);

    LOG(LINFO) << "Generating depth point cloud";
    cv::Mat xyz;
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>());
    reprojectImageTo3D(disp, xyz, Q, true);
    const double max_z = 1.0e4;
    for(int y = 0; y < xyz.rows; y++)
    {
        for(int x = 0; x < xyz.cols; x++)
        {
            cv::Vec3f point = xyz.at<cv::Vec3f>(y, x);
            if(fabs(point[2] - max_z) < FLT_EPSILON || fabs(point[2]) > max_z) continue;
            //fprintf(fp, "%f %f %f\n", point[0], point[1], point[2]);
            pcl::PointXYZ point1(point[0], point[1], point[2]);
            cloud->push_back(point1);
        }
    }

    LOG(LINFO) << "Writing to data stream";
	out_depth_map.write(disp8);
	out_left_dispared.write(oLeftRectified);
	out_right_dispared.write(oRightRectified);
    out_cloud_xyz.write(cloud);
	} catch (...)
	{
		LOG(LERROR) << "Error occured in processing input";
	}
}

void DepthProcessor::generateQ(const cv::Mat& leftPMatrix, const cv::Mat& rightPMatrix, cv::Mat& Q) {
    double rFx = rightPMatrix.at<double>(0,0);
    double Tx = rightPMatrix.at<double>(0,3) / -rFx;
    double rCx = rightPMatrix.at<double>(0,2);
    double rCy = rightPMatrix.at<double>(1,2);
    double lCx = leftPMatrix.at<double>(0,2);
    Q = cv::Mat(4,4, CV_64F);
    Q.data[0,0] = 1.0;
    Q.data[1,1] = 1.0;
    Q.data[3,2] = -1.0 / Tx;
    Q.data[0,3] = -rCx;
    Q.data[1,3] = -rCy;
    Q.data[2,3] = rFx;
    Q.data[3,3] = (rCx - lCx) / Tx;
}

} //: namespace DepthProcessor
} //: namespace Processors
