/*!
 * \file
 * \brief
 * \author Mikolaj Kojdecki
 */

#include <memory>
#include <string>
#include <sstream>

#include "CameraPGR.hpp"
#include "Common/Logger.hpp"

#include <boost/bind.hpp>

namespace Sources {
namespace CameraPGR {

CameraPGR_Source::CameraPGR_Source(const std::string & name) :
		Base::Component(name) , 
		width("width", 640), 
		height("height", 480), 
		fps("fps", 10), 
		shutter("shutter", 60), 
		gain("gain", 0),
		camera_url("camera_url", "")
		camera_serial("camera_serial", "") 
		/* Camera properties:
		 * BRIGHTNESS
		 * AUTO_EXPOSURE
		 * SHARPNESS
		 * WHITE_BALANCE
		 * HUE
		 * SATURATION
		 * GAMMA
		 * IRIS
		 * FOCUS
		 * ZOOM
		 * PAN
		 * TILT
		 * SHUTTER
		 * GAIN
		 * TRIGGER_MODE
		 * TRIGGER_DELAY
		 * FRAME_RATE
		 * TEMPERATURE
		 * */{
		registerProperty(width);
		registerProperty(height);
		registerProperty(fps);
		registerProperty(shutter);
		registerProperty(gain);
		registerProperty(camera_url);
		registerProperty(camera_serial);
		
		// TODO odzyskiwanie guid i wybór więcej niż jednej kamery
		FlyCapture2::Error error;
		FlyCapture2::BusManager busMgr;
      
		FlyCapture2::PGRGuid guid;
		if(camera_serial != "")
			error = busMgr.GetCameraFromSerialNumber(serial, &guid);
      
		// Connect to a camera
		// According to documentation when guid is null it should connect to first found camera.
		// Now, the  question is: is guid in this case null? Need to check.
		error = cam.Connect(&guid);
		if (error != FlyCapture2::PGRERROR_OK)
		{
			PrintError( error );
			return -1;
		}
      
		// Get the camera information
		// This is held as a member of the class
		error = cam.GetCameraInfo(&camInfo);
		if (error != FlyCapture2::PGRERROR_OK)
		{
			PrintError( error );
			return -1;
		}

		FlyCapture2::GigEImageSettingsInfo imageSettingsInfo;
		error = cam.GetGigEImageSettingsInfo( &imageSettingsInfo );
		if (error != FlyCapture2::PGRERROR_OK)
		{
			PrintError( error );
			return -1;
		}

		FlyCapture2::GigEImageSettings imageSettings;
		imageSettings.offsetX = 0;
		imageSettings.offsetY = 0;
		imageSettings.height = height;
		imageSettings.width = width;
		imageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_RGB8;
								//  FlyCapture2::PIXEL_FORMAT_RAW8?

		printf( "Setting GigE image settings...\n" );

		error = cam.SetGigEImageSettings( &imageSettings );
		if (error != FlyCapture2::PGRERROR_OK)
		{
			PrintError( error );
			return -1;
		}

		/* and turn on the streamer */
		error = cam.StartCapture();
		if (error != FlyCapture2::PGRERROR_OK)
		{
			PrintError( error );
			return -1;
		}
		ok = true;
		image_thread = boost::thread(boost::bind(&CameraPGR_Source::captureAndSendImages, this));
}

CameraPGR_Source::~CameraPGR_Source() {
	ok = false;
    image_thread.join();
      
    cam.StopCapture();
    cam.Disconnect();
}

void CameraPGR_Source::prepareInterface() {
	// Register data streams, events and event handlers HERE!
	registerStream("shutterTimeChange", &shutterTimeChange);
	registerStream("out_img", &out_img);
	registerStream("out_info", &out_info);
	// Register handlers
	h_onShutterTimeChanged.setup(boost::bind(&CameraPGR_Source::onShutterTimeChanged, this));
	registerHandler("onShutterTimeChanged", &h_onShutterTimeChanged);
	addDependency("onShutterTimeChanged", &shutterTimeChange);

}

bool CameraPGR_Source::onInit() {

	return true;
}

bool CameraPGR_Source::onFinish() {
	return true;
}

bool CameraPGR_Source::onStop() {
	return true;
}

bool CameraPGR_Source::onStart() {
	LOG(LINFO) << "CameraPGR_Source::start()\n";
	sendInfo();
	return true;
}

void CameraPGR_Source::sendCameraInfo() {
	std::stringstream ss;
	char macAddress[64];
    sprintf( 
        macAddress, 
        "%02X:%02X:%02X:%02X:%02X:%02X", 
        pCamInfo->macAddress.octets[0],
        pCamInfo->macAddress.octets[1],
        pCamInfo->macAddress.octets[2],
        pCamInfo->macAddress.octets[3],
        pCamInfo->macAddress.octets[4],
        pCamInfo->macAddress.octets[5]);

    char ipAddress[32];
    sprintf( 
        ipAddress, 
        "%u.%u.%u.%u", 
        pCamInfo->ipAddress.octets[0],
        pCamInfo->ipAddress.octets[1],
        pCamInfo->ipAddress.octets[2],
        pCamInfo->ipAddress.octets[3]);

    char subnetMask[32];
    sprintf( 
        subnetMask, 
        "%u.%u.%u.%u", 
        pCamInfo->subnetMask.octets[0],
        pCamInfo->subnetMask.octets[1],
        pCamInfo->subnetMask.octets[2],
        pCamInfo->subnetMask.octets[3]);

    char defaultGateway[32];
    sprintf( 
        defaultGateway, 
        "%u.%u.%u.%u", 
        pCamInfo->defaultGateway.octets[0],
        pCamInfo->defaultGateway.octets[1],
        pCamInfo->defaultGateway.octets[2],
        pCamInfo->defaultGateway.octets[3]);
        
	ss << "\n*** CAMERA INFORMATION ***\n"
	   << "Serial number - " << pCamInfo->serialNumber << "\n"
	   << "Camera model - " << pCamInfo->modelName << "\n"
	   << "Sensor - " << pCamInfo->sensorInfo << "\n"
	   << "Resolution - " << pCamInfo->sensorResolution << "\n"
       << "Firmware version - " << pCamInfo->firmwareVersion << "\n"
       << "Firmware build time - " << pCamInfo->firmwareBuildTime << "\n"
       << "GigE version - " << pCamInfo->gigEMajorVersion << "." << pCamInfo->gigEMinorVersion << "\n"
       << "User defined name - " << pCamInfo->userDefinedName << "\n"
       << "XML URL 1 - " << pCamInfo->xmlURL1 << "\n"
       << "XML URL 2 - " << pCamInfo->xmlURL2 << "\n"
       << "MAC address - " << macAddress << "\n"
       << "IP address - " << ipAddress << "\n"
       << "Subnet mask - " << subnetMask << "\n"
       << "Default gateway - " << defaultGateway << "\n\n";
    //TODO dopisać wypisanie aktualnego configa
    out_info.write(ss.str());
}

void CameraPGR_Source::captureAndSendImages() {
	cv::Mat img;
	FlyCapture2::Image image;
      FlyCapture2::Error error;
      unsigned int pair_id = 0;
      while (ok) {
        //unsigned char *img_frame = NULL;
        //uint32_t bytes_used;

        // Retrieve an image
        error = cam.RetrieveBuffer( &image );
        if (error != FlyCapture2::PGRERROR_OK)
        {
//            PrintError( error );
            continue;
        }
        
        unsigned int rowBytes = (double) image.GetReceivedDataSize() / (double) image.GetRows();
        img = cv::Mat(image.GetRows(), image.GetCols(), CV_8UC3, image.GetData(), rowBytes);
        
        out_img.write(img);
        //timestamp?
        /*ImagePtr image(new Image);
        
        image->height = convertedImage.GetRows();
        image->width = convertedImage.GetCols();
        image->step = convertedImage.GetStride();
        image->encoding = image_encodings::RGB8;

        image->header.stamp = capture_time;
        image->header.seq = pair_id;

        image->header.frame_id = frame;
  
        int data_size = convertedImage.GetDataSize();

        image->data.resize(data_size);

        memcpy(&image->data[0], convertedImage.GetData(), data_size);

        pub.publish(image);

        sendInfo(image, capture_time);*/
      }
}

void CameraPGR_Source::onShutterTimeChanged() {
	//todo zmiana configa (i nazwy metody...)
}

} //: namespace CameraPGR
} //: namespace Sources
