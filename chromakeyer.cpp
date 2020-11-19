#include "chromakeyer.h"
#include "mediafactory.h"
#include "mediasource.h"
#include "mediasink.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>


bool ChromaKeyer::setUp(const std::string &inputFile)
{
	this->paramsSet = false;

	std::unique_ptr<MediaSource> reader = MediaFactory::createReader(inputFile, true /*loop*/);

	cv::namedWindow(this->windowName);
	cv::setMouseCallback(this->windowName, ChromaKeyer::onMouse, this);
	cv::createTrackbar("Tolerance", windowName, &this->tolerance, 100, nullptr, this);
	cv::createTrackbar("Softness", windowName, &this->softness, 10, nullptr, this);
	cv::createTrackbar("Defringe", windowName, &this->defringe, 100, nullptr, this);

	for (int key = 0; !this->paramsSet && (key & 0xFF) != 27; )
	{
		reader->readNext(this->inputFrameBGR);
		imshow(this->windowName, this->inputFrameBGR);
		key = cv::waitKey(10);
	}

	
    cv::destroyWindow(this->windowName);
    
	return this->paramsSet;
}	// setUp


void ChromaKeyer::onMouse(int event, int x, int y, int flags, void* data)
{
	assert(data != nullptr);
	ChromaKeyer* keyer = static_cast<ChromaKeyer*>(data);

	if (event == cv::EVENT_LBUTTONUP)
	{
		keyer->color = keyer->inputFrameBGR.at<cv::Vec3b>(y, x);
		keyer->paramsSet = true;
	}
}	// onMouse

void ChromaKeyer::keyOut(const std::string &inputFile, const std::string &backgroundFile, const std::string &outputFile)
{
	assert(this->paramsSet);

	std::unique_ptr<MediaSource> srcIn = MediaFactory::createReader(inputFile, false)	// input is not looped
		, srcBg = MediaFactory::createReader(backgroundFile, true);		// background is looped

	std::unique_ptr<MediaSink> sink = MediaFactory::createWriter(outputFile, this->inputFrameBGR.size());	// frame size obtained during the parameters setting phase

	int delay = 10;		// keystroke waiting period 

	if (srcIn->getMediaType() == MediaSource::VideoFile)
	{
		if (sink->getMediaType() != MediaSink::VideoFile && sink->getMediaType() != MediaSink::Dummy)
			throw std::runtime_error("Mismatching media types: the input file is a video, but the output is not.");
	}
	else if (srcIn->getMediaType() == MediaSource::ImageFile)
	{
		delay = 0;		// it's probably better to show the output image for longer time before closing up

		if (srcBg->getMediaType() != MediaSource::ImageFile)
			throw std::runtime_error("Background must be an image.");

		if (sink->getMediaType() != MediaSink::ImageFile && sink->getMediaType() != MediaSink::Dummy)
			throw std::runtime_error("Mismatching media types: the input file is an image, but the output is not.");
	}
	else
	{
		// TODO: check other media types 
	}


	// Convert the chosen color from BGR to HSV
	cv::Mat4f colorMatF((cv::Vec4f)(this->color / 255));
	cv::Mat3f colorMatHSV;
	cv::cvtColor(colorMatF, colorMatHSV, cv::COLOR_BGR2HSV, 3);
	cv::Scalar colorHSV = colorMatHSV.at<cv::Vec3f>();

	// Process input frames from the source until we are done or the user hits Escape
	for (int key = 0; srcIn->readNext(this->inputFrameBGR) && (key & 0xFF) != 27; )
	{
		// Read the next background frame. Since this media source is looped, the same image will be returned in case the background file is an image.
		// In case the background is a video, we will read as many frames as it's needed to match the input stream, or start from the beginning, 
		// if it's shorter.
		srcBg->readNext(this->backgroundBGR);

		// Resize the background frame to match the input frame
		cv::resize(this->backgroundBGR, this->backgroundBGR, this->inputFrameBGR.size(), 0, 0,
			this->backgroundBGR.rows * this->backgroundBGR.cols > this->inputFrameBGR.rows * this->inputFrameBGR.cols ? cv::INTER_AREA : cv::INTER_CUBIC);

		// Key out using the resized background frame
		const cv::Mat &resFrame = keyOutFrame(colorHSV);

		// Write the resulting frame to the sink
		sink->write(resFrame);

		// Display the output frame
		cv::imshow(this->windowName, resFrame);

		key = cv::waitKey(delay);
	}	// for

	cv::destroyWindow(this->windowName);
}	// keyOut

cv::Mat & ChromaKeyer::keyOutFrame(const cv::Scalar& colorHSV)
{
	CV_Assert(this->inputFrameBGR.size() == this->backgroundBGR.size());

	// Convert pixel values from 0..255 to 0..1 range so as to perform arithmetic operations with semi-transparent masks.
	// It is also recommended to use 32-bit images for HSV conversion.
	this->inputFrameBGR.convertTo(this->inputFrameBGRF, CV_32F, 1.0/255);
	this->backgroundBGR.convertTo(this->backgroundBGRF, CV_32F, 1.0/255);
	
	// Convert the input frame to HSV, which is easier to analyze
	cvtColor(this->inputFrameBGRF, this->inputFrameHSVF, cv::COLOR_BGR2HSV, 3);	


	// Prepare the color range for green screen masking.
	// We are mainly interested in hues (which must be close to green). However, the value and saturation ranges control how we
	// treat pixels near the edge of the green background and foreground objects. Green is somewhat different in these areas,
	// probably, because it is shaded or cast on the foreground objects. 

	cv::Scalar hsvRange{ 360, 1, 1 };
	assert(this->tolerance >= 0 && this->tolerance <= 100);
	assert(this->defringe >= 0 && this->defringe <= 100);
	cv::Scalar lowerHSV = { colorHSV[0] - this->tolerance / 100.0 * hsvRange[0], this->defringe/100.0, this->defringe/100.0 }
			, upperHSV = { colorHSV[0] + this->tolerance / 100.0 * hsvRange[0], hsvRange[1], hsvRange[2] };

	inRange(this->inputFrameHSVF, lowerHSV, upperHSV, this->maskB);

	// Account for hue values wrapping around the 360-degree cycle
	
	cv::Mat1b orMaskB;
	if (lowerHSV[0] < 0 && upperHSV[0] < hsvRange[0])
	{
		lowerHSV[0] = lowerHSV[0] + hsvRange[0];
		upperHSV[0] = hsvRange[0];
		cv::inRange(this->inputFrameHSVF, lowerHSV, upperHSV, orMaskB);
		cv::bitwise_or(this->maskB, orMaskB, this->maskB);
	}
	else if (lowerHSV[0] > 0 && upperHSV[0] > hsvRange[0])
	{
		lowerHSV[0] = 0;
		upperHSV[0] = upperHSV[0] - hsvRange[0];
		cv::inRange(this->inputFrameHSVF, lowerHSV, upperHSV, orMaskB);
		cv::bitwise_or(this->maskB, orMaskB, this->maskB);
	}

	// Convert the mask values from 0..255 to 0..1 range
	this->maskB.convertTo(this->maskF, CV_32F, 1.0 / 255);

	// Make foreground borders less rigid
	if (this->softness > 0)
	{
		int ksize = 2*this->softness + 1;
		cv::dilate(this->maskF, this->maskF, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(ksize, ksize)));		
		cv::GaussianBlur(this->maskF, this->maskF, cv::Size(ksize, ksize), 0, 0);
	}

	// To perform arithmetic operations with a mask, we need it to have the same number of channels as a source matrix
	merge(std::vector<cv::Mat1f>{maskF, maskF, maskF}, this->mask3F);

	// Our mask contains values of around 1 where frame pixels are close to green.
	// Thus we can extract pixel values from the background frame which correspond to the green screen.
	multiply(this->backgroundBGRF, this->mask3F, this->backgroundBGRF);
	
	// Invert the mask to extract pixel values of the foreground
	this->mask3F.convertTo(this->mask3F, -1, -1, 1.0);	// a slightly faster way to compute Scalar::all(1.0) - mask3F
	multiply(this->inputFrameBGRF, this->mask3F, this->inputFrameBGRF);
	
	// Combine the foreground with a new background
	this->inputFrameBGRF += this->backgroundBGRF;

	// Scale pixel values back from 0..1 to 0..255 range
	this->inputFrameBGRF.convertTo(this->outputFrameBGR, CV_8U, 255);

	return this->outputFrameBGR;
}	// keyOutFrame

ChromaKeyer::~ChromaKeyer()
{
    try
    {        
        cv::destroyWindow(this->windowName);    // may throw a cv::Exception about non-registered windows
    }
    catch (const cv::Exception &e)  // don't let the exception be thrown from the destructor
    {
#ifdef DEBUG_MODE        
        std::cerr << "Warning! Failed to destroy the window: " << e.what() << std::endl;        
#endif
    }
}

