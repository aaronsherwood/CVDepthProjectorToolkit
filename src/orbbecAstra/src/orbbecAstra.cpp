//
//  orbbecAstra.h
//  orbbecAstra
//
//  Created by Matt Felsen on 10/24/15.
//
//

#include "orbbecAstra.h"


orbbecAstra::orbbecAstra() {
	width = 640;
	height = 480;
	nearClip = 300;
	farClip = 1800;

	bSetup = false;
	bIsFrameNew = false;
	bDepthImageEnabled = true;
}

orbbecAstra::~orbbecAstra() {
	astra::terminate();
}

void orbbecAstra::setup() {
	setup("device/default");
}

void orbbecAstra::setup(const std::string& uri) {
    ci::Surface c(width, height,false);
    colorImage = c;
    ci::Channel16u d(width, height);
    depthPixels = d;
    ci::Channel di(width, height);
    depthImage = di;
	cachedCoords.resize(width * height);
	updateDepthLookupTable();

	astra::initialize();

	streamset = astra::StreamSet(uri.c_str());
	reader = astra::StreamReader(streamset.create_reader());

	bSetup = true;
	reader.add_listener(*this);
}

void orbbecAstra::enableDepthImage(bool enable) {
	bDepthImageEnabled = enable;
}

void orbbecAstra::enableRegistration(bool useRegistration) {
	if (!bSetup) {
		std::cout << "Must call setup() before setRegistration()" << std::endl;
		return;
	}

	reader.stream<astra::DepthStream>().enable_registration(useRegistration);
}

void orbbecAstra::setDepthClipping(unsigned short near, unsigned short far) {
	nearClip = near;
	farClip = far;
	updateDepthLookupTable();
}

void orbbecAstra::initColorStream() {
	if (!bSetup) {
		std::cout << "Must call setup() before initColorStream()" << std::endl;
		return;
	}

	astra::ImageStreamMode colorMode;
	auto colorStream = reader.stream<astra::ColorStream>();

	colorMode.set_width(width);
	colorMode.set_height(height);
	colorMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_RGB888);
	colorMode.set_fps(30);

	colorStream.set_mode(colorMode);
	colorStream.start();
}

void orbbecAstra::initDepthStream() {
	if (!bSetup) {
		std::cout << "Must call setup() before initDepthStream()" << std::endl;
		return;
	}

	astra::ImageStreamMode depthMode;
	auto depthStream = reader.stream<astra::DepthStream>();

	depthMode.set_width(width);
	depthMode.set_height(height);
	depthMode.set_pixel_format(astra_pixel_formats::ASTRA_PIXEL_FORMAT_DEPTH_MM);
	depthMode.set_fps(30);

	depthStream.set_mode(depthMode);
	depthStream.start();
}

void orbbecAstra::initPointStream() {
	if (!bSetup) {
		std::cout << "Must call setup() before initPointStream()" << std::endl;
		return;
	}

	reader.stream<astra::PointStream>().start();
}

void orbbecAstra::initHandStream() {
	if (!bSetup) {
		std::cout << "Must call setup() before initHandStream()" << std::endl;
		return;
	}

	reader.stream<astra::HandStream>().start();
}


void orbbecAstra::update(){
	// See on_frame_ready() for more processing
	bIsFrameNew = false;
	astra_temp_update();
}

void orbbecAstra::draw(float x, float y, float w, float h){
	if (!w) w = width;
	if (!h) h = height;
    ci::gl::TextureRef texRGB=ci::gl::Texture2d::create(colorImage);
    ci::Rectf rect = ci::Rectf(x, y, x+w, y+h);
    ci::gl::draw(texRGB, rect);
}

void orbbecAstra::drawDepth(float x, float y, float w, float h){
	if (!w) w = width;
	if (!h) h = height;
    ci::gl::Texture::Format format;
    ci::gl::TextureRef texRGB = ci::gl::Texture2d::create(depthImage);//, ci::gl::Texture::Format().internalFormat( GL_R16 ));
    ci::Rectf rect = ci::Rectf(x, y, x+w, y+h);
    ci::gl::draw(texRGB, rect);
}

bool orbbecAstra::isFrameNew() {
	return bIsFrameNew;
}

void orbbecAstra::on_frame_ready(astra::StreamReader& reader,
									astra::Frame& frame)
{
	bIsFrameNew = true;

	auto colorFrame = frame.get<astra::ColorFrame>();
	auto depthFrame = frame.get<astra::DepthFrame>();
	auto pointFrame = frame.get<astra::PointFrame>();
	auto handFrame = frame.get<astra::HandFrame>();

	if (colorFrame.is_valid()) {
		colorFrame.copy_to((astra::RgbPixel*) colorImage.getData());
    }

	if (depthFrame.is_valid()) {
		depthFrame.copy_to((short*) depthPixels.getData());
		if (bDepthImageEnabled) {
			// TODO do this with a shader so it's fast?
            ci::Channel16u::Iter iter = depthPixels.getIter();
            ci::Channel::Iter outputIter = depthImage.getIter();
                while( iter.line() && outputIter.line()) {
                    while( iter.pixel() && outputIter.pixel()) {
                        short depth = iter.v();
                        float val = depthLookupTable[depth];
                        outputIter.v() = val;
                    }
                }
		}
	}

	if (pointFrame.is_valid()) {
		pointFrame.copy_to((astra::Vector3f*) cachedCoords.data());
	}

	if (handFrame.is_valid()) {
		handMapDepth.clear();
		handMapWorld.clear();
		auto& list = handFrame.handpoints();

		for (auto& handPoint : list) {
			const auto& id = handPoint.tracking_id();

			if (handPoint.status() == HAND_STATUS_TRACKING) {
				const auto& depthPos = handPoint.depth_position();
				const auto& worldPos = handPoint.world_position();

				handMapDepth[id] = glm::vec2(depthPos.x, depthPos.y);
				handMapWorld[id] = glm::vec3(worldPos.x, worldPos.y, worldPos.z);
			} else {
				handMapDepth.erase(id);
				handMapWorld.erase(id);
			}
		}
	}
}

void orbbecAstra::updateDepthLookupTable() {
	// From product specs, range is 8m
	int maxDepth = 8000;
	depthLookupTable.resize(maxDepth);

	// Depth values of 0 should be discarded, so set the LUT value to 0 as well
	depthLookupTable[0] = 0;

	// Set the rest
	for (int i = 1; i < maxDepth; i++) {
        int depthValue = ci::lmap(i, int(nearClip), int(farClip), 255, 0);
        depthLookupTable[i] = char(ci::clamp(depthValue, 0, 255));
	}
}

glm::vec3 orbbecAstra::getWorldCoordinateAt(int x, int y) {
	return cachedCoords[int(y) * width + int(x)];
}

unsigned short orbbecAstra::getNearClip() {
	return nearClip;
}

unsigned short orbbecAstra::getFarClip() {
	return farClip;
}

ci::Channel16u orbbecAstra::getRawDepth() {
	return depthPixels;
}

ci::Channel orbbecAstra::getDepthImage() {
	return depthImage;
}

ci::Surface orbbecAstra::getColorImage() {
	return colorImage;
}

std::unordered_map<int32_t,glm::vec2>& orbbecAstra::getHandsDepth() {
	return handMapDepth;
}

std::unordered_map<int32_t,glm::vec3>& orbbecAstra::getHandsWorld() {
	return handMapWorld;
}
