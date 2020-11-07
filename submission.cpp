#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <filesystem>
#include <set>

using namespace std;
using namespace cv;

class ChromaKeyer
{
public:
	ChromaKeyer(const char* inputFile) : inputFile(inputFile) {}
	// TODO: define copy/move constructors and the assignment operators
	virtual ~ChromaKeyer() = default;

	String getInputFile() const { return inputFile; }

	void keyOut(const char* windowName, const char* backgroundFile, const char* outputFile);

private:

	//virtual void pickColor(const char* windowName) = 0;
	virtual void replaceBackground(const char* windowName, const char* backgroundFile, const char* outputFile) = 0;


	String inputFile;
};	// ChromaKeyer

void ChromaKeyer::keyOut(const char* windowName, const char* backgroundFile, const char* outputFile)
{
	//pickColor(windowName);
	replaceBackground(windowName, backgroundFile, outputFile);
}

class ImageKeyer : public ChromaKeyer
{
public:
	ImageKeyer(const char* inputFile) : ChromaKeyer(inputFile) {}

	//virtual void pickColor(const char* windowName) override;
	virtual void replaceBackground(const char* windowName, const char* backgroundFile, const char* outputFile) override;
};	// ImageKeyer


//void ImageKeyer::pickColor(const char* windowName)
//{
//}

void ImageKeyer::replaceBackground(const char* windowName, const char* backgroundFile, const char* outputFile)
{

}	// replaceBackground

class ChromaKeyingFactory
{
public:
	//static unique_ptr<ChromaKeyer> create(const char* inputFile);
	static unique_ptr<ChromaKeyer> create(const char* inputFile);
	//static auto create(const char* inputFile);
};	// ChromaKeyingFactory

unique_ptr<ChromaKeyer> ChromaKeyingFactory::create(const char* inputFile)
//auto ChromaKeyingFactory::create(const char* inputFile)
{
	string ext = filesystem::path(inputFile).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return std::tolower(c); });
	
	//static array<string, 4> images{ {".jpg", ".jpeg", ".png", ".bmp" } };
	//static array<string, 2> video{ {".mp4", ".avi"} };
	static set<string> images{".jpg", ".jpeg", ".png", ".bmp" };
	static set<string> video{".mp4", ".avi"};

	// TODO: perhaps, implement a webcam keyer?

	if (images.find(ext) != images.end())
		return make_unique<ImageKeyer>(inputFile);
	else if (video.find(ext) != video.end())
		return make_unique<VideoKeyer>(inputFile);
	else 
		throw std::runtime_error(string("Input file type is not supported: ").append(ext));
}	// create

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		cerr << "Usage: chromak <input video with green screen> <background file> <output file>" << endl;
		return -1;
	}

	try
	{
		unique_ptr keyer = ChromaKeyingFactory::create(argv[1]);
		keyer->keyOut("Chroma Keying", argv[2], argv[3]);
	}
	catch (const std::exception& e)		// cv::Exception inherits from std::exception
	{
		cerr << e.what() << endl;
		return -2;
	}

	return 0;
}