#ifndef VIDEOFILEWRITER_H
#define VIDEOFILEWRITER_H

#include "mediasink.h"

#include <opencv2/videoio.hpp>

#include <memory>
#include <string>


class VideoFileWriter : public MediaSink
{
public:
	VideoFileWriter(const std::string& videoFile, cv::Size frameSize, const char (&fourcc)[4], double fps)
		: MediaSink(MediaSink::VideoFile, videoFile)
		, writer(videoFile, cv::VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]), fps, std::move(frameSize), true) {	}

	VideoFileWriter(const VideoFileWriter&) = delete;
	VideoFileWriter(VideoFileWriter&&) = default;

	VideoFileWriter& operator = (const VideoFileWriter&) = delete;
	VideoFileWriter& operator = (VideoFileWriter&&) = default;

	virtual void write(const cv::Mat& frame) override;

private:
	cv::VideoWriter writer;
};	// VideoWriter


#endif // VIDEOFILEWRITER_H