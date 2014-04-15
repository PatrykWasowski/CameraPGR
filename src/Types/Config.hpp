#ifndef CONFIG_HPP_
#define CONFIG_HPP_

class Config {
public:
	string brightness_mode;
	float brightness_value;
	string auto_exposure_mode;
	float exposure_value;
	string sharpness_mode;
	float sharpness_value;
	string white_balance_mode;
	float white_balance_value;
	string hue_mode;
	float hue_value;
	string saturation_mode;
	float saturation_value;
	string gamma_mode;
	float gamma_value;	 
	string frame_rate_mode;
	float frame_rate_value;
	string shutter_mode;
	float shutter_value;
	string gain_mode;
	int gain_value;
	
	Config() {
		brightness_mode = "previous";
		brightness_value = -1;
		auto_exposure_mode = "previous";
		exposure_value = -1;
		sharpness_mode = "previous";
		sharpness_value = -1;
		white_balance_mode = "previous";
		white_balance_value = -1;
		hue_mode = "previous";
		hue_value = -1;
		saturation_mode = "previous";
		saturation_value = -1;
		gamma_mode = "previous";
		gamma_value = -1;	 
		frame_rate_mode = "previous";
		frame_rate_value = -1;
		shutter_mode = "previous";
		shutter_value = -1;
		gain_mode = "previous";
		gain_value = -1;
	}
	
};

#endif
