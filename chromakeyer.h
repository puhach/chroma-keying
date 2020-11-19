#ifndef CHROMAKEYER_H
#define CHROMAKEYER_H

#include <opencv2/core.hpp>

#include <string>

class ChromaKeyer
{
public:
	ChromaKeyer(std::string windowName) : windowName(move(windowName)) {}
	ChromaKeyer(const ChromaKeyer&) = delete;
	ChromaKeyer(ChromaKeyer&&) = default;
	~ChromaKeyer();

	ChromaKeyer& operator = (const ChromaKeyer&) = delete;
	ChromaKeyer& operator = (ChromaKeyer&&) = delete;

	bool setUp(const std::string &inputFile);

	void keyOut(const std::string& inputFile, const std::string& backgroundFile, const std::string& outputFile);

private:

	cv::Mat & keyOutFrame(const cv::Scalar& colorHSV);

	static void onMouse(int event, int x, int y, int flags, void* data);

	cv::String windowName;
	bool paramsSet = false;
	int tolerance = 12, softness = 2, defringe = 40;	// default parameters
	cv::Scalar color;

	// Holding matrices in member variables saves memory allocations
	cv::Mat inputFrameBGR, backgroundBGR, outputFrameBGR;
	cv::Mat inputFrameBGRF, backgroundBGRF;
	cv::Mat3f inputFrameHSVF;
	cv::Mat1b maskB;
	cv::Mat1f maskF;
	cv::Mat3f mask3F;
};	// ChromaKeyer


#endif // CHROMAKEYER_H