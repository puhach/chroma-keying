#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include <opencv2/core.hpp>

#include <string>

class MediaSource	// a universal media source
{
public:

	enum MediaType	// add new media types here
	{
		ImageFile,
		VideoFile,
		Webcam
	};

	MediaType getMediaType() const noexcept { return this->mediaType; }

	const std::string& getMediaPath() const noexcept { return this->mediaPath; }

	bool isLooped() const noexcept { return this->looped; }

	virtual bool readNext(cv::Mat &frame) = 0;
	
	virtual void reset() = 0;

	virtual ~MediaSource() = default;

protected:
	
	MediaSource(MediaType inputType, const std::string &mediaPath, bool looped)
		: mediaType(inputType)
		, mediaPath(mediaPath)
		, looped(looped) {}

	MediaSource(const MediaSource&) = default;
	MediaSource(MediaSource&&) = default;

	MediaSource& operator = (const MediaSource&) = default;
	MediaSource& operator = (MediaSource&&) = default;

private:
	MediaType mediaType;
	std::string mediaPath;
	bool looped = false;
};	// MediaSource



#endif	// MEDIASOURCE_H