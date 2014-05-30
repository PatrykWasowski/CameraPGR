/*!
 * \file
 * \brief 
 * \author Mikolaj Kojdecki
 */

#ifndef CAMERAPGR_HPP_
#define CAMERAPGR_HPP_

#include "Component_Aux.hpp"
#include "Component.hpp"
#include "DataStream.hpp"
#include "Property.hpp"
#include "EventHandler2.hpp"

#include "Config.hpp"

#include <opencv2/opencv.hpp>

#include <boost/thread.hpp>
//FlyCapture2 imports
#include <FlyCapture2.h>
#include <GigECamera.h>
#include <BusManager.h>
#include <Image.h>



namespace Sources {
namespace CameraPGR {

/*!
 * \class CameraPGR
 * \brief CameraPGR processor class.
 *
 * CameraPGR processor.
 */
class CameraPGR_Source: public Base::Component {
public:
	/*!
	 * Constructor.
	 */
	CameraPGR_Source(const std::string & name = "CameraPGR");

	/*!
	 * Destructor
	 */
	virtual ~CameraPGR_Source();

	/*!
	 * Prepare components interface (register streams and handlers).
	 * At this point, all properties are already initialized and loaded to 
	 * values set in config file.
	 */
	void prepareInterface();

protected:

	/*!
	 * Connects source to given device.
	 */
	bool onInit();

	/*!
	 * Disconnect source from device, closes streams, etc.
	 */
	bool onFinish();

	/*!
	 * Start component
	 */
	bool onStart();

	/*!
	 * Stop component
	 */
	bool onStop();

	void captureAndSendImages();
	void configure();

	// Input data streams
	Base::DataStreamIn<Config> configChange;

	// Output data streams
	Base::DataStreamOut<cv::Mat> out_img;
	Base::DataStreamOut<string> out_info;

	// Handlers
	Base::EventHandler2 h_onConfigChanged;
	// Properties
	Base::Property<string> camera_url;
	Base::Property<unsigned int> camera_serial;
	Base::Property<string> pixel_format;
	Base::Property<int> width;
	Base::Property<int> height;
	
	/* Camera properties:
		 * BRIGHTNESS
		 * AUTO_EXPOSURE
		 * SHARPNESS
		 * WHITE_BALANCE
		 * HUE
		 * SATURATION
		 * GAMMA
		 * IRIS nieedytowalne
		 * FOCUS nieedytowalne
		 * ZOOM nieedytowalne
		 * PAN nieedytowalne
		 * TILT nieedytowalne
		 * SHUTTER
		 * GAIN
		 * TRIGGER_MODE
		 * TRIGGER_DELAY
		 * FRAME_RATE
		 * TEMPERATURE
		 * 
		 * Po dwie własności na atrybut, tj. czy auto czy nie i jeśli nie to jaka wartość. Jeśli nie ma wartości to ta z flasha kamery.
		 * */
	Base::Property<string> brightness_mode;
	Base::Property<float> brightness_value;
	Base::Property<string> auto_exposure_mode;
	Base::Property<float> exposure_value;
	Base::Property<string> sharpness_mode;
	Base::Property<float> sharpness_value;
	Base::Property<string> white_balance_mode;
	Base::Property<float> white_balance_value;
	Base::Property<string> hue_mode;
	Base::Property<float> hue_value;
	Base::Property<string> saturation_mode;
	Base::Property<float> saturation_value;
	Base::Property<string> gamma_mode;
	Base::Property<float> gamma_value;	 
	Base::Property<string> frame_rate_mode;
	Base::Property<float> frame_rate_value;
	Base::Property<string> shutter_mode;
	Base::Property<float> shutter_value;
	Base::Property<string> gain_mode;
	Base::Property<int> gain_value;

	void sendCameraInfo();
	// Handlers
	void onShutterTimeChanged();

private:
	bool ok;
	bool changing;
	FlyCapture2::GigECamera cam;
	FlyCapture2::CameraInfo camInfo;
	boost::thread image_thread;
};

} //: namespace CameraPGR
} //: namespace Sources

/*
 * Register processor component.
 */
REGISTER_COMPONENT("CameraPGR", Sources::CameraPGR::CameraPGR_Source)

#endif /* CAMERAPGR_HPP_ */
