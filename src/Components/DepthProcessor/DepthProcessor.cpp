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
        numberOfDisparities("numberOfDisparities", int(0)),
        SADWindowSize("SADWindowSize (MUST BE ODD >= 1)", int(5)),
        preFilterCap("preFilterCap", int(63)),
        minDisparity("minDisparity", int(0)),
        disp12MaxDiff("disp12MaxDiff", int(1)),
        speckleRange("speckleRange", int(32)),
        speckleWindowSize("speckleWindowSize", int(100)),
        uniquenessRatio("uniquenessRatio", int(10)),
        textureThreshold("textureThreshold", int(10)),
        algorythm_type("algorythm_type", STEREO_SGBM)
{
    bm = NULL;
    sgbm = NULL;


    registerProperty(algorythm_type);
    numberOfDisparities.addConstraint("1");
    numberOfDisparities.addConstraint("9999");
    numberOfDisparities.setToolTip("dupa");
    registerProperty(numberOfDisparities);
    SADWindowSize.addConstraint("1");
    SADWindowSize.addConstraint("9999");
    registerProperty(SADWindowSize);
    preFilterCap.addConstraint("1");
    preFilterCap.addConstraint("9999");
    registerProperty(preFilterCap);
    minDisparity.addConstraint("0");
    minDisparity.addConstraint("9999");
    registerProperty(minDisparity);
    disp12MaxDiff.addConstraint("0");
    disp12MaxDiff.addConstraint("9999");
    registerProperty(disp12MaxDiff);
    speckleRange.addConstraint("0");
    speckleRange.addConstraint("9999");
    registerProperty(speckleRange);
    speckleWindowSize.addConstraint("0");
    speckleWindowSize.addConstraint("9999");
    registerProperty(speckleWindowSize);
    uniquenessRatio.addConstraint("0");
    uniquenessRatio.addConstraint("100");
    registerProperty(uniquenessRatio);
    textureThreshold.addConstraint("0");
    textureThreshold.addConstraint("9999");
    registerProperty(textureThreshold);
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
    registerStream("out_cloud_xyzrgb", &out_cloud_xyzrgb);

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
    cv::Rect roi1, roi2;
    //Reprojection matix - Q
    cv::Mat Q;
    //Rectification and projection matrices - R* and P*
    cv::Mat R1, P1, R2, P2;

    //Size of input image
    cv::Size img_size = oLeftImage.size();

    // Camera matrices
    cv::Mat M1 = oLeftCamInfo.cameraMatrix();
    cv::Mat M2 = oRightCamInfo.cameraMatrix();

    //Distortion coefficients matrices
    cv::Mat D1 = oLeftCamInfo.distCoeffs();
    cv::Mat D2 = oRightCamInfo.distCoeffs();

    //Rotation and translation of second camera to the first
    cv::Mat R = oRightCamInfo.rotationMatrix();
    cv::Mat T = oRightCamInfo.translationMatrix();


    LOG(LINFO) << "M1 "<< M1;
    LOG(LINFO) << "M2 "<< M2;
    LOG(LINFO) << "D1 "<< D1;
    LOG(LINFO) << "D2 "<< D2;
    LOG(LINFO) << "Size " << img_size.height << "x"<<img_size.width;

    cv::stereoRectify( M1, D1, M2, D2, img_size, R, T, R1, R2, P1, P2, Q, cv::CALIB_ZERO_DISPARITY, -1, img_size, &roi1, &roi2 );



    LOG(LINFO) << "R1 "<< R1;
    LOG(LINFO) << "P1 "<< P1;
    LOG(LINFO) << "R2 "<< R2;
    LOG(LINFO) << "P2 "<< P2;

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

    bm->state->roi1 = roi1;
    bm->state->roi2 = roi2;
    bm->state->preFilterCap = preFilterCap;
    bm->state->SADWindowSize = SADWindowSize;
    bm->state->minDisparity = minDisparity;
    bm->state->numberOfDisparities = numberOfDisparities;
    bm->state->textureThreshold = textureThreshold;
    bm->state->uniquenessRatio = uniquenessRatio;
    bm->state->speckleWindowSize = speckleWindowSize;
    bm->state->speckleRange = speckleRange;
    bm->state->disp12MaxDiff = disp12MaxDiff;


    int cn = oLeftImage.channels();

    sgbm->preFilterCap = preFilterCap;
    sgbm->SADWindowSize = SADWindowSize;
    sgbm->P1 = 8*cn*sgbm->SADWindowSize*sgbm->SADWindowSize;
    sgbm->P2 = 32*cn*sgbm->SADWindowSize*sgbm->SADWindowSize;
    sgbm->minDisparity = minDisparity;
    sgbm->numberOfDisparities = numberOfDisparities;
    sgbm->uniquenessRatio = uniquenessRatio;
    sgbm->speckleWindowSize = speckleWindowSize;
    sgbm->speckleRange = speckleRange;
    sgbm->disp12MaxDiff = disp12MaxDiff;
    sgbm->fullDP = ( algorythm_type == STEREO_HH );

    cv::Mat disp, disp8;

    LOG(LINFO) << "Calculating disparity";
    int64 t = cv::getTickCount();
    if( algorythm_type == STEREO_BM )
        bm->operator ()(oLeftRectified, oRightRectified, disp);
//    else if( algorythm_type == STEREO_VAR ) {
//        var(oLeftRectified, oRightRectified, disp);
//    }
    else if( algorythm_type == STEREO_SGBM || algorythm_type == STEREO_HH )
        sgbm->operator ()(oLeftRectified, oRightRectified, disp);
    t = cv::getTickCount() - t;
    float time = t * 1000/cv::getTickFrequency();
    LOG(LINFO) << boost::format("Time elapsed: %1%ms\n") % time;

    //disp = dispp.colRange(numberOfDisparities, img1p.cols);
    LOG(LINFO) << "Converting disparity to depth image";
    disp.convertTo(disp8, CV_8U, 255/(numberOfDisparities*16.));
//    if( algorythm_type != STEREO_VAR )
//        disp.convertTo(disp8, CV_8U, 255/(numberOfDisparities*16.));
//    else
//        disp.convertTo(disp8, CV_8U);

    //LOG(LINFO) << "Calculate Q transform matrix";
    //generateQ(P1,P2, Q);

    LOG(LINFO) << "Generating depth point cloud";
    cv::Mat xyz;
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>());
    uint32_t pr, pg, pb;
    reprojectImageTo3D(disp8, xyz, Q, true);
    const double max_z = 1.0e4;
    for(int y = 0; y < xyz.rows; y++)
    {
        uchar* rgb_ptr = oLeftRectified.ptr<uchar>(y);
        for(int x = 0; x < xyz.cols; x++)
        {
            cv::Vec3f point = xyz.at<cv::Vec3f>(y, x);
            //cv::Vec3f rgbVal = oLeftRectified.at<cv::Vec3f>(y, x);
            if(fabs(point[2] - max_z) < FLT_EPSILON || fabs(point[2]) > max_z) continue;


            //Get RGB info
            pb = rgb_ptr[3*x];
            pg = rgb_ptr[3*x+1];
            pr = rgb_ptr[3*x+2];

            //Insert info into point cloud structure
            pcl::PointXYZRGB point1;
            point1.x = point[0];
            point1.y = point[1];
            point1.z = -point[2];
            uint32_t rgb = (static_cast<uint32_t>(pr) << 16 | static_cast<uint32_t>(pg) << 8 | static_cast<uint32_t>(pb));
            //uint8_t r = 255, g = 0, b = 0; // Example: Red color
            //uint32_t rgb = ((uint32_t)pr << 16 | (uint32_t)pg << 8 | (uint32_t)pb);
            point1.rgb = *reinterpret_cast<float*>(&rgb);
            cloud->push_back(point1);
        }
    }

    LOG(LINFO) << "Writing to data stream";
    out_depth_map.write(disp8);
    out_left_dispared.write(oLeftRectified);
    out_right_dispared.write(oRightRectified);
    out_cloud_xyzrgb.write(cloud);
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
