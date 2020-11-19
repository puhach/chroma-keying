#ifndef IMAGEFILEREADER_H
#define IMAGEFILEREADER_H


#include "mediasource.h"

#include <opencv2/core.hpp>

#include <string>



class ImageFileReader : public MediaSource
{
public:
	ImageFileReader(const std::string& imageFile, bool looped = false);
	ImageFileReader(const ImageFileReader&) = delete;
	ImageFileReader(ImageFileReader&& other) = default;

	ImageFileReader& operator = (const ImageFileReader&) = delete;
	ImageFileReader& operator = (ImageFileReader&& other) = default;

	bool readNext(cv::Mat& frame) override;
	void reset() override;

private:
	cv::Mat cache;
};	// ImageFileReader


#endif	// IMAGEFILEREADER_H