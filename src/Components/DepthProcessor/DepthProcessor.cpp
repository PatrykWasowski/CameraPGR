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
		algorythm_type("algorythm_type", STEREO_BM) {
	numberOfDisparities = 512;
	bm = NULL;
	sgbm = NULL;
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

	// Register handlers
	h_CalculateDepthMap.setup(boost::bind(&DepthProcessor::CalculateDepthMap, this));
	registerHandler("CalculateDepthMap", &h_CalculateDepthMap);
	addDependency("CalculateDepthMap", &l_in_img);
	addDependency("CalculateDepthMap", &r_in_img);
}

bool DepthProcessor::onInit() {
	//TODO usunac 1280 z kodu bo hardcode!!!!
	numberOfDisparities = numberOfDisparities > 0 ? numberOfDisparities : ((1280/8) + 15) & -16;
	sgbm = new cv::StereoSGBM(minDisparity, numberOfDisparities,SADWindowSize,sgbmP1,sgbmP2, disp12MaxDiff, preFilterCap, uniquenessRatio, speckleWindowSize, speckleRange, fullDP);
	bm = new cv::StereoBM(cv::StereoBM::BASIC_PRESET, numberOfDisparities);
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

/*
	LOG(LINFO) << "Get images from Data Streams";
	cv::Mat oLeftRectifyMat =  Types::MatrixTranslator::fromStr("0.969748028765496 0.0307665090006707 -0.24216148048223; -0.0327201255585135 0.99945635200596 -0.0040489280524987; 0.241905258497762 0.011849994044338 0.970227511232433", CV_64F);
	cv::Mat oRightRectifyMat = Types::MatrixTranslator::fromStr("0.972643867434977 0.0343306345904133 -0.229750548790966; -0.0324764362110336 0.999402273156889 0.0118481011083424; 0.230019973550475 -0.00406250384151901 0.973177428750984", CV_64F);
*/

	LOG(LINFO) << "Create bunch of fresh cv::Mat objects";
	cv::StereoVar var;
    cv::Mat Q;
    cv::Mat R1, P1, R2, P2;
    cv::Size img_size = oLeftImage.size();

    // Rotation and Translation matrices between cameras
    cv::Mat R = Types::MatrixTranslator::fromStr("1529.207571 0.000000 726.349884; 0.000000 1529.207571 535.944351; 0 0 1", CV_64F);
    cv::Mat T = Types::MatrixTranslator::fromStr("-141.355314; 0; 0", CV_64F);

    // Camera matrices
    cv::Mat M1 = oLeftCamInfo.cameraMatrix();
    cv::Mat M2 = oRightCamInfo.cameraMatrix();

    //Distortion coefficients matrices
    cv::Mat D1 = oLeftCamInfo.distCoeffs();
    cv::Mat D2 = oRightCamInfo.distCoeffs();

    LOG(LINFO) << "Calculate stereo rectification maps";
    cv::stereoRectify( M1, D1, M2, D2, img_size, R, T, R1, R2, P1, P2, Q, cv::CALIB_ZERO_DISPARITY, -1, img_size);

    LOG(LINFO) << M1;
    LOG(LINFO) << M2;
    LOG(LINFO) << D1;
    LOG(LINFO) << D2;
    LOG(LINFO) << R;
    LOG(LINFO) << T;
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

//    var.levels = 3; // ignored with USE_AUTO_PARAMS
//    var.pyrScale = 0.5; // ignored with USE_AUTO_PARAMS
//    var.nIt = 25;
//    var.minDisp = -numberOfDisparities;
//    var.maxDisp = 0;
//    var.poly_n = 3;
//    var.poly_sigma = 0.0;
//    var.fi = 15.0f;
//    var.lambda = 0.03f;
//    var.penalization = var.PENALIZATION_TICHONOV; // ignored with USE_AUTO_PARAMS
//    var.cycle = var.CYCLE_V; // ignored with USE_AUTO_PARAMS
//    var.flags = var.USE_SMART_ID | var.USE_AUTO_PARAMS | var.USE_INITIAL_DISPARITY | var.USE_MEDIAN_FILTERING ;

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

	LOG(LINFO) << "Writing to data stream";

	out_depth_map.write(disp8);
	out_left_dispared.write(oLeftRectified);
	out_right_dispared.write(oRightRectified);
	} catch (...)
	{
		LOG(LERROR) << "Error occured in processing input";
	}

}



} //: namespace DepthProcessor
} //: namespace Processors
