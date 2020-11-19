#include "videofilereader.h"

#include <filesystem>


VideoFileReader::VideoFileReader(const std::string &videoFile, bool looped)
	: MediaSource(MediaSource::VideoFile, std::filesystem::exists(videoFile) ? videoFile : throw std::runtime_error("Input video doesn't exist: "+videoFile), looped)
	, cap(videoFile)
{
	CV_Assert(cap.isOpened());
}

bool VideoFileReader::readNext(cv::Mat& frame)
{
	if (cap.read(frame))
		return true;

	if (isLooped())
	{
		// Close and try reading again
		cap.release();
		if (!cap.open(getMediaPath()) || !cap.read(frame))
			throw std::runtime_error("Failed to read the input file.");

		return true;
	}	// looped
	else return false;	// probably, the end of the stream
}	// readNext

void VideoFileReader::reset()
{
	cap.release();
	CV_Assert(cap.open(getMediaPath()));
}	// reset

