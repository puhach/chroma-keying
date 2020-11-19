#include "mediafactory.h"
#include "imagefilereader.h"
#include "videofilereader.h"
#include "imagefilewriter.h"
#include "videofilewriter.h"
#include "dummywriter.h"

#include <memory>
#include <filesystem>


const std::set<std::string> MediaFactory::images{ ".jpg", ".jpeg", ".png", ".bmp" };
const std::set<std::string> MediaFactory::video{ ".mp4", ".avi"};



std::string MediaFactory::getFileExtension(const std::string &fileName)
{
	std::string ext = std::filesystem::path(fileName).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return std::tolower(c); });
	return ext;
}

std::unique_ptr<MediaSource> MediaFactory::createReader(const std::string &input, bool loop)
{
	std::string ext = getFileExtension(input);
	
	if (images.find(ext) != images.end())
		return std::make_unique<ImageFileReader>(input, loop);
	else if (video.find(ext) != video.end())
		return std::make_unique<VideoFileReader>(input, loop);
	else 
	{
		// TODO: may handle other input types here, e.g. webcam

		throw std::runtime_error(std::string("Input file type is not supported: ").append(ext));
	}
}	// createReader


std::unique_ptr<MediaSink> MediaFactory::createWriter(const std::string &output, cv::Size frameSize)
{
	if (output.empty())
		return std::make_unique<DummyWriter>();

	std::string ext = MediaFactory::getFileExtension(output);
	if (images.find(ext) != images.end())
		return std::make_unique<ImageFileWriter>(output, frameSize);
	else if (video.find(ext) != video.end())
		// Help the compiler to deduce the argument types which are passed to the constructor
		return std::make_unique<VideoFileWriter, const std::string &, cv::Size, const char(&)[4], double>(output, std::move(frameSize), { 'm','p','4','v' }, 30);
	else
	{
		// TODO: consider adding other sinks

		throw std::runtime_error(std::string("Output file type is not supported: ").append(ext));
	}
}	// createWriter

