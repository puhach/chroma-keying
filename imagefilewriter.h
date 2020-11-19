#ifndef IMAGEFILEWRITER_H
#define IMAGEFILEWRITER_H

#include "mediasink.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <memory>
#include <string>


class ImageFileWriter: public MediaSink
{
public:
	ImageFileWriter(const std::string& imageFile, cv::Size frameSize)
		: MediaSink(MediaSink::ImageFile, cv::haveImageWriter(imageFile) ? imageFile : throw std::runtime_error("No encoder for this image file: " + imageFile))
		, frameSize(std::move(frameSize)) { }
	
	ImageFileWriter(const ImageFileWriter&) = delete;
	ImageFileWriter(ImageFileWriter&&) = default;

	ImageFileWriter& operator = (const ImageFileWriter&) = delete;
	ImageFileWriter& operator = (ImageFileWriter&&) = default;

	virtual void write(const cv::Mat& frame) override;

private:
	cv::Size frameSize;
};	// ImageFileWriter


#endif	// IMAGEFILEWRITER_H
