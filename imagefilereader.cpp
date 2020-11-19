#include "imagefilereader.h"

#include <opencv2/imgcodecs.hpp>

#include <filesystem>


ImageFileReader::ImageFileReader(const std::string &imageFile, bool looped)
	: MediaSource(MediaSource::ImageFile, std::filesystem::exists(imageFile) ? imageFile : throw std::runtime_error("Input image doesn't exist: " + imageFile), looped)
{
	if (!cv::haveImageReader(imageFile))
		throw std::runtime_error("No decoder for this image file: " + imageFile);
}	// ctor

bool ImageFileReader::readNext(cv::Mat& frame)
{
	if (this->cache.empty())
	{
		frame = cv::imread(getMediaPath(), cv::IMREAD_COLOR);
		CV_Assert(!frame.empty());
		frame.copyTo(this->cache);
		return true;
	}	// cache empty
	else
	{
		if (isLooped())
		{
			this->cache.copyTo(frame);
			return true;
		}
		else return false;
	}	// image cached
}	// readNext

void ImageFileReader::reset()
{
	this->cache.release();	// this will force the image to be reread
}	// reset

