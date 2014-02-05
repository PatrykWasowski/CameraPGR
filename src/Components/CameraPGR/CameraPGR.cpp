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

namespace Processors {
namespace CameraPGR {

CameraPGR::CameraPGR(const std::string & name) :
		Base::Component(name) , 
		width("width", 640), 
		height("height", 480), 
		fps("fps", 10), 
		shutter("shutter", 60), 
		gain("gain", 0) {
		registerProperty(width);
		registerProperty(height);
		registerProperty(fps);
		registerProperty(shutter);
		registerProperty(gain);

}

CameraPGR::~CameraPGR() {
}

void CameraPGR::prepareInterface() {
	// Register data streams, events and event handlers HERE!
registerStream("shutterTimeChange", &shutterTimeChange);
registerStream("out_img", &out_img);
registerStream("out_info", &out_info);
	// Register handlers
	h_onShutterTimeChanged.setup(boost::bind(&CameraPGR::onShutterTimeChanged, this));
	registerHandler("onShutterTimeChanged", &h_onShutterTimeChanged);
	addDependency("onShutterTimeChanged", &shutterTimeChange);

}

bool CameraPGR::onInit() {

	return true;
}

bool CameraPGR::onFinish() {
	return true;
}

bool CameraPGR::onStop() {
	return true;
}

bool CameraPGR::onStart() {
	return true;
}

void CameraPGR::onShutterTimeChanged() {
}



} //: namespace CameraPGR
} //: namespace Processors
