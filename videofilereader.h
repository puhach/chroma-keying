#ifndef VIDEOFILEREADER_H
#define VIDEOFILEREADER_H


#include "mediasource.h"

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <string>


class VideoFileReader : public MediaSource
{
public:
	VideoFileReader(const std::string &videoFile, bool looped = false);
	VideoFileReader(const VideoFileReader&) = delete;
	VideoFileReader(VideoFileReader&& other) = default;

	VideoFileReader& operator = (const VideoFileReader&) = delete;
	VideoFileReader& operator = (VideoFileReader&& other) = default;

	virtual bool readNext(cv::Mat& frame) override;

	virtual void reset() override;

private:
	cv::String inputFile;
	cv::VideoCapture cap;
};	// VideoFileReader


#endif	// VIDEOFILEREADER_H