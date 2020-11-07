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

class VideoKeyer : public ChromaKeyer
{
public:
	VideoKeyer(const char* inputFile) : ChromaKeyer(inputFile) {}

	//virtual void pickColor(const char* windowName) override;
	virtual void replaceBackground(const char* windowName, const char* backgroundFile, const char* outputFile) override;
};	// VideoKeyer

//void VideoKeyer::pickColor(const char* windowName)
//{
//}

void VideoKeyer::replaceBackground(const char* windowName, const char* backgroundFile, const char* outputFile)
{
	namedWindow(windowName);

	VideoCapture capIn(getInputFile()), capBg(backgroundFile);
	CV_Assert(capIn.isOpened());
	CV_Assert(capBg.isOpened());

	int frameWidth = capIn.get(CAP_PROP_FRAME_WIDTH)
	  , frameHeight = capIn.get(CAP_PROP_FRAME_HEIGHT)
	  , frameCount = capIn.get(CAP_PROP_FRAME_COUNT);

	bool colorPicked = true;
	Mat frameIn, frameBg;

	for (int frameIndex = 0; ; ++frameIndex)
	{
		capIn >> frameIn;

		if (colorPicked)
		{
			if (frameIn.empty())
				break;	// looks like we've reached the end of the stream

			if (!capBg.read(frameBg))
			{
				//CV_Assert(capBg.set(CAP_PROP_POS_FRAMES, 0));	
				//capBg.read(frameBg);

				capBg.release();	// perhaps, we've reached the end of the stream?
				if (!capBg.open(backgroundFile) || !capBg.read(frameBg))
				//if (!capBg.set(CAP_PROP_POS_FRAMES, 0) || !capBg.read(frameBg))	// try to rewind and read again
					throw runtime_error("Failed to read the background frame.");
			}	// empty

			// TODO: 			
			// resize the background frame
			// replace the background
			// show the frameIn
			// wait for the key
		}	// colorPicked
		else
		{
			if (frameIn.empty())
			{				
				if (frameIndex == 0)
					throw runtime_error("Failed to read the input file. Is it empty?");

				//CV_Assert(capIn.set(CAP_PROP_POS_FRAMES, 0));
				capIn.release();
				capIn.open(getInputFile());
				frameIndex = 0;
				continue;
			}	// frameIn is empty
			else
			{
				// TODO:
				// show the frameIn
				// wait for the key
			}	// frameIn is not empty
		}	// !colorPicked

	}	// for

	//capIn.release();

	// TODO: destroy window
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