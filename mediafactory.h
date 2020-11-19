#ifndef MEDIAFACTORY_H
#define MEDIAFACTORY_H

#include <opencv2/core.hpp>

#include <memory>
#include <string>
#include <set>

class MediaSource;
class MediaSink;

class MediaFactory
{
public:
	static std::unique_ptr<MediaSource> createReader(const std::string &input, bool loop = false);
	static std::unique_ptr<MediaSink> createWriter(const std::string &output, cv::Size frameSize);

private:
	static const std::set<std::string> images;
	static const std::set<std::string> video;

	static std::string getFileExtension(const std::string &inputFile);
};	// MediaFactory


#endif	// MEDIAFACTORY_H