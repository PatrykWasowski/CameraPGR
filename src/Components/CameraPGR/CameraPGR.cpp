/*!
 * \file
 * \brief Point Grey GigE camera component
 * \author Mikolaj Kojdecki
 */

#include <memory>
#include <string>
#include <sstream>
#include <iostream>

#include "CameraPGR.hpp"
#include "Common/Logger.hpp"

#include <boost/bind.hpp>
//FlyCapture2 imports
//#include <FlyCapture2.h>
//#include <GigECamera.h>
//#include <BusManager.h>
//#include <Image.h>

/***************************** IMPORTANT NOTICE ************************
 * For the time being this component IS NOT WORKING without submitting
 * the serial number of the camera in component configuration (parameter
 * camera_serial). The number can be chcecked on the sticker on the 
 * camera or in FlyCap application.
 * 
 * 
 */ 
namespace Sources {
namespace CameraPGR {

		CameraPGR_Source::CameraPGR_Source(const std::string & name) :
		Base::Component(name),
		camera_url("camera_url", string("null")),
		camera_serial("camera_serial", 0),
		pixel_format("pixel_format", string("RGB")),
		width("width", 1296), //need to rework
		height("height", 1032),
		offsetX("offsetX", 0),
		offsetY("offsetY", 0),
		brightness_mode("brightness_mode", string("previous")),
		brightness_value("brightness_value", -1),
        exposure_mode("exposure_mode", string("previous")),
		exposure_value("exposure_value", -1),
		sharpness_mode("sharpness_mode", string("previous")),
		sharpness_value("sharpness_value", -1),
		white_balance_mode("white_balance_mode", string("previous")),
		white_balance_value("white_balance_value", -1),
		hue_mode("hue_mode", string("previous")),
		hue_value("hue_value", -1),
		saturation_mode("saturation_mode", string("previous")),
		saturation_value("saturation_value", -1),
		gamma_mode("gamma_mode", string("previous")),
		gamma_value("gamma_value", -1),
		frame_rate_mode("frame_rate_mode", string("previous")),
		frame_rate_value("frame_rate_value", -1),
		shutter_mode("shutter_mode", string("previous")),
		shutter_value("shutter_value", -1),
		gain_mode("gain_mode", string("previous")),
		gain_value("gain_value", 0)
		/* Camera properties:
		 * BRIGHTNESS
		 * AUTO_EXPOSURE	AUTO	ONEPUSH
		 * SHARPNESS
		 * WHITE_BALANCE	AUTO	ONEPUSH
		 * HUE
		 * SATURATION
		 * GAMMA
		 * IRIS nieedytowalne
		 * FOCUS nieedytowalne
		 * ZOOM nieedytowalne
		 * PAN nieedytowalne
		 * TILT nieedytowalne
		 * SHUTTER	AUTO	ONEPUSH
		 * GAIN		AUTO	ONEPUSH
		 * TRIGGER_MODE
		 * TRIGGER_DELAY
		 * FRAME_RATE	AUTO
		 * TEMPERATURE
		 * 
		 * Po dwie własności na atrybut, tj. czy auto czy nie i jeśli nie to jaka wartość. Jeśli nie ma wartości to ta z flasha kamery.
		 * */{
			registerProperty(width);
			registerProperty(height);
			registerProperty(brightness_mode);
			registerProperty(brightness_value);
			registerProperty(exposure_mode);
			registerProperty(exposure_value);
			registerProperty(hue_mode);
			registerProperty(hue_value);
			registerProperty(saturation_mode);
			registerProperty(saturation_value);
			registerProperty(gamma_mode);
			registerProperty(gamma_value);
			registerProperty(frame_rate_mode);
			registerProperty(frame_rate_value);
			registerProperty(shutter_mode);
			registerProperty(shutter_value);
			registerProperty(gain_mode);
			registerProperty(gain_value);
			registerProperty(camera_url);
			registerProperty(camera_serial);
			registerProperty(pixel_format);
			registerProperty(offsetX);
			registerProperty(offsetY);
			
			changing = false;
		}

		CameraPGR_Source::~CameraPGR_Source() {
			ok = false;
			changing = true;
			image_thread.join();

			cam.StopCapture();
			cam.Disconnect();
		}

		void CameraPGR_Source::prepareInterface() {
			// Register data streams, events and event handlers HERE!
			registerStream("configChange", &configChange);
			registerStream("out_img", &out_img);
			registerStream("out_info", &out_info);
			// Register handlers
			h_onConfigChanged.setup(boost::bind(&CameraPGR_Source::onConfigChanged, this));
			registerHandler("onConfigChanged", &h_onConfigChanged);
			addDependency("onConfigChanged", &configChange);

		}

		bool CameraPGR_Source::onInit() {
			// TODO odzyskiwanie guid i wybór więcej niż jednej kamery
			FlyCapture2::Error error;
			FlyCapture2::BusManager busMgr;

			FlyCapture2::PGRGuid guid;
			if(camera_serial != (unsigned int) 0)
				error = busMgr.GetCameraFromSerialNumber(camera_serial, &guid);

			// Connect to a camera
			// According to documentation when guid is null it should connect to first found camera.
			// Now, the  question is: is guid in this case null? Need to check.
			
			//God damn it, of course it's not working.
			error = cam.Connect(&guid);
			if (error != FlyCapture2::PGRERROR_OK)
			{
				LOG(LERROR) << "connect niet ";// << (int) error;
				//return -1;
			}

			// Get the camera information
			// This is held as a member of this class, since it's gonna be static during the execution
			error = cam.GetCameraInfo(&camInfo);
			if (error != FlyCapture2::PGRERROR_OK)
			{
				LOG(LERROR) << "cameraInfo niet ";//<< (int) error;
				//return -1;
			}

			FlyCapture2::GigEImageSettingsInfo imageSettingsInfo;
			error = cam.GetGigEImageSettingsInfo( &imageSettingsInfo );
			if (error != FlyCapture2::PGRERROR_OK)
			{
				LOG(LERROR) << "getGigEImageSettingsInfo niet ";// << (int) error;
				//return -1;
			}

			FlyCapture2::GigEImageSettings imageSettings;
			imageSettings.offsetX = offsetX;
			imageSettings.offsetY = offsetY;
			imageSettings.height = height;
			imageSettings.width = width;
			if(pixel_format == "RAW")
				imageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_RAW8;
			else if(pixel_format == "RGB" || pixel_format == "RGB8")
				imageSettings.pixelFormat = FlyCapture2::PIXEL_FORMAT_RGB;

			LOG(LINFO) << "Setting GigE image settings...\n";

			error = cam.SetGigEImageSettings( &imageSettings );
			if (error != FlyCapture2::PGRERROR_OK)
			{
				LOG(LERROR) << "setGigEImageSettings niet ";// << (int) error;
				//return -1;
			}

			/* and turn on the streamer */
			error = cam.StartCapture();
			if (error != FlyCapture2::PGRERROR_OK)
			{
				LOG(LERROR) << "startCapture niet ";//<< (int) error;
				//return -1;
			}
			ok = true;
			image_thread = boost::thread(boost::bind(&CameraPGR_Source::captureAndSendImages, this));
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
			sendCameraInfo();
			return true;
		}

		void CameraPGR_Source::sendCameraInfo() {
			std::stringstream ss;
			char macAddress[64];
			sprintf(
					macAddress,
					"%02X:%02X:%02X:%02X:%02X:%02X",
					camInfo.macAddress.octets[0],
					camInfo.macAddress.octets[1],
					camInfo.macAddress.octets[2],
					camInfo.macAddress.octets[3],
					camInfo.macAddress.octets[4],
					camInfo.macAddress.octets[5]);

			char ipAddress[32];
			sprintf(
					ipAddress,
					"%u.%u.%u.%u",
					camInfo.ipAddress.octets[0],
					camInfo.ipAddress.octets[1],
					camInfo.ipAddress.octets[2],
					camInfo.ipAddress.octets[3]);

			char subnetMask[32];
			sprintf(
					subnetMask,
					"%u.%u.%u.%u",
					camInfo.subnetMask.octets[0],
					camInfo.subnetMask.octets[1],
					camInfo.subnetMask.octets[2],
					camInfo.subnetMask.octets[3]);

			char defaultGateway[32];
			sprintf(
					defaultGateway,
					"%u.%u.%u.%u",
					camInfo.defaultGateway.octets[0],
					camInfo.defaultGateway.octets[1],
					camInfo.defaultGateway.octets[2],
					camInfo.defaultGateway.octets[3]);

			ss << "\n*** CAMERA INFORMATION ***\n"
			<< "Serial number - " << camInfo.serialNumber << "\n"
			<< "Camera model - " << camInfo.modelName << "\n"
			<< "Sensor - " << camInfo.sensorInfo << "\n"
			<< "Resolution - " << camInfo.sensorResolution << "\n"
			<< "Firmware version - " << camInfo.firmwareVersion << "\n"
			<< "Firmware build time - " << camInfo.firmwareBuildTime << "\n"
			<< "GigE version - " << camInfo.gigEMajorVersion << "." << camInfo.gigEMinorVersion << "\n"
			<< "User defined name - " << camInfo.userDefinedName << "\n"
			<< "XML URL 1 - " << camInfo.xmlURL1 << "\n"
			<< "XML URL 2 - " << camInfo.xmlURL2 << "\n"
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
			//setting camera properties
			configure();
			while (ok) {
				while(!changing)
				{
					//unsigned char *img_frame = NULL;
					//uint32_t bytes_used;

					// Retrieve an image
					error = cam.RetrieveBuffer( &image );
					if (error != FlyCapture2::PGRERROR_OK)
					{
	//            PrintError( error );
						continue;
					}
					
					if (pixel_format == "RAW")
						//TODO need to convert from RAW to RGB using FlyCapture2 SDK
						std::cout << "Do this, ye lazy bastard!" << std::endl;
						
					unsigned int rowBytes = (double) image.GetReceivedDataSize() / (double) image.GetRows();
					img = cv::Mat(image.GetRows(), image.GetCols(), CV_8UC3, image.GetData(), rowBytes);
					cvtColor(img, img, CV_RGB2BGR);

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
		}

		void CameraPGR_Source::onConfigChanged() {
			Config config = configChange.read();
			brightness_mode = config.brightness_mode;
			brightness_value = config.brightness_value;
			exposure_mode = config.exposure_mode;
			exposure_value = config.exposure_value;
			sharpness_mode = config.sharpness_mode;
			sharpness_value = config.sharpness_value;
			white_balance_mode = config.white_balance_mode;
			white_balance_value = config.white_balance_value;
			hue_mode = config.hue_mode;
			hue_value = config.hue_value;
			saturation_mode = config.saturation_mode;
			saturation_value = config.saturation_value;
			gamma_mode = config.gamma_mode;
			gamma_value = config.gamma_value;	 
			frame_rate_mode = config.frame_rate_mode;
			frame_rate_value = config.frame_rate_value;
			shutter_mode = config.shutter_mode;
			shutter_value = config.shutter_value;
			gain_mode = config.gain_mode;
			gain_value = config.gain_value;
			
			configure();
		}
		
		void CameraPGR_Source::configure() {
			changing = true;
				FlyCapture2::Property prop;
				if(frame_rate_mode != "previous")
				{
					
					prop.type = FlyCapture2::FRAME_RATE;
					prop.onOff = true;
					if(frame_rate_mode == "auto")
						prop.autoManualMode = true;
					/*else if(frame_rate_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}*/
					else if(frame_rate_mode == "manual" && (int) frame_rate_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = frame_rate_value;
					}
					cam.SetProperty(&prop);
				}
				
				if(exposure_mode != "previous")
				{
					
					prop.type = FlyCapture2::AUTO_EXPOSURE;
					prop.onOff = true;
					if(exposure_mode == "auto")
						prop.autoManualMode = true;
					else if(exposure_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}
					else if(exposure_mode == "manual" && (int) exposure_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = exposure_value;
					}
					cam.SetProperty(&prop);
				}
				
				if(shutter_mode != "previous")
				{
					
					prop.type = FlyCapture2::SHUTTER;
					prop.onOff = true;
					if(shutter_mode == "auto")
						prop.autoManualMode = true;
					else if(shutter_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}
					else if(shutter_mode == "manual" && (int) shutter_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = shutter_value;
					}
					cam.SetProperty(&prop);
				}
				
				if(gain_mode != "previous")
				{
					
					prop.type = FlyCapture2::GAIN;
					prop.onOff = true;
					if(gain_mode == "auto")
						prop.autoManualMode = true;
					else if(gain_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}
					else if(gain_mode == "manual" && (int) gain_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = gain_value;
					}
					cam.SetProperty(&prop);
				}
				
				if(white_balance_mode != "previous")
				{
					
					prop.type = FlyCapture2::WHITE_BALANCE;
					prop.onOff = true;
					if(white_balance_mode == "auto")
						prop.autoManualMode = true;
					else if(white_balance_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}
					else if(white_balance_mode == "manual" && (int) white_balance_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = white_balance_value;
					}
					cam.SetProperty(&prop);
				}
				
				if(brightness_mode != "previous")
				{
					
					prop.type = FlyCapture2::BRIGHTNESS;
					prop.onOff = true;
					/*if(auto_exposure_mode == "auto")
						prop.autoManualMode = true;
					else if(auto_exposure_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}
					else*/ if(brightness_mode == "manual" && (int) brightness_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = brightness_value;
					}
					cam.SetProperty(&prop);
				}
				
				if(sharpness_mode != "previous")
				{
					
					prop.type = FlyCapture2::SHARPNESS;
					prop.onOff = true;
					/*if(auto_exposure_mode == "auto")
						prop.autoManualMode = true;
					else if(auto_exposure_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}
					else*/ if(sharpness_mode == "manual" && (int) sharpness_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = sharpness_value;
					}
					cam.SetProperty(&prop);
				}
				
				if(hue_mode != "previous")
				{
					
					prop.type = FlyCapture2::HUE;
					prop.onOff = true;
					/*if(auto_exposure_mode == "auto")
						prop.autoManualMode = true;
					else if(auto_exposure_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}
					else*/ if(hue_mode == "manual" && (int) hue_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = hue_value;
					}
					cam.SetProperty(&prop);
				}
				
				if(saturation_mode != "previous")
				{
					
					prop.type = FlyCapture2::SATURATION;
					prop.onOff = true;
					/*if(auto_exposure_mode == "auto")
						prop.autoManualMode = true;
					else if(auto_exposure_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}
					else*/ if(saturation_mode == "manual" && (int) saturation_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = saturation_value;
					}
					cam.SetProperty(&prop);
				}
				
				if(gamma_mode != "previous")
				{
					
					prop.type = FlyCapture2::GAMMA;
					prop.onOff = true;
					/*if(auto_exposure_mode == "auto")
						prop.autoManualMode = true;
					else if(auto_exposure_mode == "onepush")
					{
						prop.autoManualMode = false;
						prop.onePush = true;
					}
					else*/ if(gamma_mode == "manual" && (int) gamma_value != -1)
					{
						prop.autoManualMode = false;
						prop.absControl = true;
						prop.absValue = gamma_value;
					}
					cam.SetProperty(&prop);
				}
			
			changing = false;
		}

} //: namespace CameraPGR
} //: namespace Sources
