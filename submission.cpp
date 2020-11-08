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

	//void keyOut(const char* windowName, const char* backgroundFile, const char* outputFile);

	void exec(const char* windowName, const char* backgroundFile, const char* outputFile);

private:

	//virtual void createGui(const char *windowName);
	//virtual void pickColor(const char* windowName) = 0;
	virtual bool setupKeyer(const char *windowName) = 0;
	virtual void keyOut(const char* windowName, const char* backgroundFile, const char* outputFile) = 0;

	//void cleanupHelper() { }

	String inputFile;
	//bool bSetUp = false;
};	// ChromaKeyer

void ChromaKeyer::exec(const char* windowName, const char* backgroundFile, const char* outputFile)
{
	
	/*struct Cleaner
	{
		constexpr Cleaner(ChromaKeyer *pKeyer, void(ChromaKeyer::*pCleanUp)()) noexcept : pKeyer(pKeyer), pCleanUp(pCleanUp) {}

		~Cleaner()	{	(pKeyer->*pCleanUp)(); }

	private:
		ChromaKeyer* pKeyer;
		void (ChromaKeyer::*pCleanUp)();
	} cleaner(this, &ChromaKeyer::cleanupHelper);*/

	try
	{
		if (setupKeyer(windowName))
			keyOut(windowName, backgroundFile, outputFile);

		//cleanup(windowName);
	}
	catch (const std::exception&)
	{
		destroyWindow(windowName);
		//this->bSetUp = false;
		//cleanup(windowName);
		throw;
	}
}	// exec

class ImageKeyer : public ChromaKeyer
{
public:
	ImageKeyer(const char* inputFile) : ChromaKeyer(inputFile) {}

	//virtual void pickColor(const char* windowName) override;
	virtual bool setupKeyer(const char* windowName) override;
	virtual void keyOut(const char* windowName, const char* backgroundFile, const char* outputFile) override;
};	// ImageKeyer


//void ImageKeyer::pickColor(const char* windowName)
//{
//}

bool ImageKeyer::setupKeyer(const char* windowName)
{
	return false;
}	// setupKeyer

void ImageKeyer::keyOut(const char* windowName, const char* backgroundFile, const char* outputFile)
{

}	// replaceBackground

class VideoKeyer : public ChromaKeyer
{
public:
	VideoKeyer(const char* inputFile) : ChromaKeyer(inputFile) {}

	//virtual void pickColor(const char* windowName) override;
	virtual bool setupKeyer(const char* windowName);
	virtual void keyOut(const char* windowName, const char* backgroundFile, const char* outputFile) override;

private:
	bool paramsSet = false;
};	// VideoKeyer

//void VideoKeyer::pickColor(const char* windowName)
//{
//}

bool VideoKeyer::setupKeyer(const char* windowName)
{
	//assert(!this->paramsSet);

	//namedWindow(windowName);
	//setMouseCallback(windowName, onMouse, this);

	//// TODO: add sliders

	/*struct WindowDestroyer
	{
		WindowDestroyer(const char* windowName) : windowName(windowName) {}
		~WindowDestroyer() { destroyWindow(windowName); }
		const char* windowName;
	};*/

	//ChromaKeyer::WindowDestroyer destroyer(windowName);
	//ChromaKeyer::setupKeyer(windowName, destroyer);
	
	//WindowDestroyer destroyer(windowName);

	/*struct GuiCleaner
	{
		GuiCleaner() { ChromaKeyer::setupKeyer(windowName); }
		~GuiCleaner() { destroyWindow(windowName); }
	} cleaner;*/

	//createGui(windowName);
	

	VideoCapture capIn(getInputFile());
	CV_Assert(capIn.isOpened());

	bool colorPicked = false;	// TODO: must be a member variable
	Mat frame;

	for (int frameIndex = 0, key = 0; !colorPicked && (key & 0xFF) != 27 ; ++frameIndex)
	{
		if (capIn.read(frame))
		{
			imshow(windowName, frame);
			key = waitKey(10);
		}	// frame read
		else
		{
			if (frameIndex == 0)
				throw runtime_error("Failed to read the input file. Is it empty?");

			// Rewind and try to read a frame from the beginning
			capIn.release();
			capIn.open(getInputFile());
			frameIndex = 0;
			continue;
		}	// empty frame
	}	// for

	//setMouseCallback(windowName, nullptr, 0);	// disable mouse handling

	return colorPicked;
}	// setupKeyer

void VideoKeyer::keyOut(const char* windowName, const char* backgroundFile, const char* outputFile)
{

}	// keyOut

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
		keyer->exec("Chroma Keying", argv[2], argv[3]);
	}
	catch (const std::exception& e)		// cv::Exception inherits from std::exception
	{
		cerr << e.what() << endl;
		return -2;
	}

	return 0;
}