/*!
 * \file
 * \brief
 * \author Mikolaj Kojdecki
 */

#include <memory>
#include <string>

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
		
		// TODO odzyskiwanie guid i wybór więcej niż jednej kamery
		FlyCapture2::Error error;
		FlyCapture2::BusManager busMgr;
      
		FlyCapture2::PGRGuid guid;
		error = busMgr.GetCameraFromSerialNumber(serial, &guid);
      
		// Connect to a camera
		error = cam.Connect(&guid);
		if (error != FlyCapture2::PGRERROR_OK)
		{
			PrintError( error );
			return -1;
		}
      
		// Get the camera information
		FlyCapture2::CameraInfo camInfo;
		error = cam.GetCameraInfo(&camInfo);
		if (error != FlyCapture2::PGRERROR_OK)
		{
			PrintError( error );
			return -1;
		}

		//PrintCameraInfo(&camInfo);

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
		imageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_RAW8;
								//  FlyCapture2::PIXEL_FORMAT_RGB8?

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
		image_thread = boost::thread(boost::bind(&Camera::feedImages, this));

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
	LOG(LINFO) << "CameraPGR_Source::stop()\n";
	/*char macAddress[64];
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

    printf(
        "\n*** CAMERA INFORMATION ***\n"
        "Serial number - %u\n"
        "Camera model - %s\n"
        "Camera vendor - %s\n"
        "Sensor - %s\n"
        "Resolution - %s\n"
        "Firmware version - %s\n"
        "Firmware build time - %s\n"
        "GigE version - %u.%u\n"
        "User defined name - %s\n"
        "XML URL 1 - %s\n"
        "XML URL 2 - %s\n"
        "MAC address - %s\n"
        "IP address - %s\n"
        "Subnet mask - %s\n"
        "Default gateway - %s\n\n",
        pCamInfo->serialNumber,
        pCamInfo->modelName,
        pCamInfo->vendorName,
        pCamInfo->sensorInfo,
        pCamInfo->sensorResolution,
        pCamInfo->firmwareVersion,
        pCamInfo->firmwareBuildTime,
        pCamInfo->gigEMajorVersion,
        pCamInfo->gigEMinorVersion,
        pCamInfo->userDefinedName,
        pCamInfo->xmlURL1,
        pCamInfo->xmlURL2,
        macAddress,
        ipAddress,
        subnetMask,
        defaultGateway );*/
	return true;
}

void CameraPGR_Source::onShutterTimeChanged() {
}



} //: namespace CameraPGR
} //: namespace Sources
