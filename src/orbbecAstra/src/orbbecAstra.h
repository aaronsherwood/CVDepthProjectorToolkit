//
//  orbbecAstra.h
//  orbbecAstra
//
//  Created by Matt Felsen on 10/24/15.
//
//

#pragma once


#include "astra.hpp"

class orbbecAstra : public astra::FrameListener {

public:

	orbbecAstra();
	~orbbecAstra();
    
	// For multiple cameras, use "device/sensor0",
	// "device/sensor1", etc. Otherwise, leave blank.
	void setup();
	void setup(const std::string& uri);
    
	void enableDepthImage(bool enable);
	void enableRegistration(bool useRegistration);
	void setDepthClipping(unsigned short near, unsigned short far);

	void initColorStream();
	void initDepthStream();
	void initPointStream();
	void initHandStream();
//	void initVideoGrabber(int deviceID = 0);

	void update();
	bool isFrameNew();

	void draw(float x = 0, float y = 0, float w = 0, float h = 0);
	void drawDepth(float x = 0, float y = 0, float w = 0, float h = 0);

	glm::vec3 getWorldCoordinateAt(int x, int y);

	unsigned short getNearClip();
	unsigned short getFarClip();

	ci::Channel16u getRawDepth();
	ci::Channel getDepthImage();
	ci::Surface getColorImage();

    std::unordered_map<int32_t,glm::vec2>& getHandsDepth();
    std::unordered_map<int32_t,glm::vec3>& getHandsWorld();

protected:

	virtual void on_frame_ready(astra::StreamReader& reader,
		astra::Frame& frame) override;

	void updateDepthLookupTable();

	astra::StreamSet streamset;
	astra::StreamReader reader;

	int width;
	int height;
	bool bSetup;
	bool bIsFrameNew;
	bool bDepthImageEnabled;
	unsigned short nearClip;
	unsigned short farClip;

	ci::Channel16u depthPixels;
	ci::Channel depthImage;
	ci::Surface colorImage;

	// Hack for Astra Pro cameras which only expose color via a webcam/UVC
	// stream, not through the SDK
//	bool bUseVideoGrabber;
//	shared_ptr<ofVideoGrabber> grabber;

    std::vector<char> depthLookupTable;
	std::vector<glm::vec3> cachedCoords;

	std::unordered_map<int32_t,glm::vec2> handMapDepth;
	std::unordered_map<int32_t,glm::vec3> handMapWorld;

};
