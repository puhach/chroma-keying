#ifndef MEDIASINK_H
#define MEDIASINK_H


#include <opencv2/core.hpp>

#include <string>


class MediaSink
{
public:

	enum MediaType
	{
		ImageFile,
		VideoFile,
		Dummy
	};

	MediaType getMediaType() const noexcept { return this->mediaType; }

	const std::string& getMediaPath() const noexcept { return this->mediaPath; }

	virtual void write(const cv::Mat &frame) = 0;

	virtual ~MediaSink() = default;

protected:
	MediaSink(MediaType mediaType, const std::string& mediaPath)
		: mediaType(mediaType)
		, mediaPath(mediaPath) {}	// std::string's copy constructor is not noexcept 

	MediaSink(const MediaSink&) = default;
	MediaSink(MediaSink&&) = default;

	MediaSink& operator = (const MediaSink&) = default;
	MediaSink& operator = (MediaSink&&) = default;

private:
	MediaType mediaType;
	std::string mediaPath;
};	// MediaSink



#endif	// MEDIASINK_H