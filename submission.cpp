#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <filesystem>
#include <set>

using namespace std;
using namespace cv;

class MediaSource
{
public:

	enum MediaType
	{
		Image,
		Video,
		Webcam
	};

	constexpr MediaType getMediaType() const noexcept { return this->mediaType; }

	//string_view getMediaPath() const noexcept {	return this->mediaPath; }
	const string& getMediaPath() const noexcept { return this->mediaPath; }
	//string getMediaPath() const { return this->mediaPath; }

	constexpr bool isLooped() const noexcept { return this->looped; }

	virtual bool readNext(Mat &frame) = 0;
	
	virtual ~MediaSource() = default;

protected:
	
	//MediaSource(MediaType inputType, const char *mediaPath, bool looped) //noexcept 
	MediaSource(MediaType inputType, const string &mediaPath, bool looped)
		: mediaType(inputType)
		, mediaPath(mediaPath)
		, looped(looped) {}

	// TODO: define copy/move ctors and assignment operators

private:
	MediaType mediaType;
	string mediaPath;
	bool looped = false;
};	// MediaSource


class ImageReader : public MediaSource
{
public:
	//ImageReader(const char* imageFile, bool looped = false);
	//ImageReader(string imageFile, bool looped = false);
	ImageReader(const string& imageFile, bool looped = false);
		
	// TODO: implement copy/move constructors and assignment operators
	~ImageReader() = default;

	bool readNext(Mat& frame) override;

private:
	/*String imageFile;
	bool loop = false;
	bool imageRead = false;*/
	bool imageRead = false;
};	// ImageReader


//ImageReader::ImageReader(string imageFile, bool looped)
//	: MediaSource(MediaSource::Image, cv::haveImageReader(imageFile) ? std::move(imageFile) : throw runtime_error("No decoder for this image: " + imageFile), looped)
ImageReader::ImageReader(const string &imageFile, bool looped)
	: MediaSource(MediaSource::Image, cv::haveImageReader(imageFile) ? imageFile : throw runtime_error("No decoder for this image: " + imageFile), looped)
{
	//if (!cv::haveImageReader(imageFile))
	//	throw runtime_error("No decoder for this image: " + imageFile);
}	// ctor

bool ImageReader::readNext(Mat& frame)
{
	//if (this->imageRead && !this->loop)
	if (this->imageRead && !isLooped())
		return false;	// don't double read

	//frame = imread(this->imageFile, IMREAD_COLOR);
	frame = imread(String{ getMediaPath() }, IMREAD_COLOR);
	//frame = imread(getMediaPath(), IMREAD_COLOR);
	CV_Assert(!frame.empty());	
	return (this->imageRead = true);
}	// readNext


class VideoReader : public MediaSource
{
public:
	//VideoReader(const char* inputFile, bool loop = false);
	//VideoReader(string inputFile, bool looped = false);
	VideoReader(const string &videoFile, bool looped = false);

	virtual bool readNext(Mat& frame) override;

	// TODO

private:
	String inputFile;
	VideoCapture cap;
};	// VideoReader

//VideoReader::VideoReader(string inputFile, bool looped)
//	: MediaSource(MediaSource::Video, std::move(inputFile), looped)
VideoReader::VideoReader(const string &videoFile, bool looped)
	: MediaSource(MediaSource::Video, videoFile, looped)
	, cap(videoFile)
{
	CV_Assert(cap.isOpened());
}

bool VideoReader::readNext(Mat& frame)
{
	if (cap.read(frame))
		return true;

	if (isLooped())
	{
		// Try closing and read again
		cap.release();
		if (!cap.open(getMediaPath()) || !cap.read(frame))
			throw runtime_error("Failed to read the input file.");

		return true;
	}	// looped
	else return false;	// probably, the end of the stream
}	// readNext



class MediaFactory
{
public:
	static unique_ptr<MediaSource> createReader(const char* inputFile, bool loop = false);
	//unique_ptr<OutputWriter> createWriter(const char *outputFile)

private:
	static const set<string> images;
	static const set<string> video;
	//static const set<string> images{ ".jpg", ".jpeg", ".png", ".bmp" };
	//static const set<string> video{".mp4", ".avi"};
};	// MediaFactory

const set<string> MediaFactory::images{ ".jpg", ".jpeg", ".png", ".bmp" };
const set<string> MediaFactory::video{ ".mp4", ".avi"};

unique_ptr<MediaSource> MediaFactory::createReader(const char* inputFile, bool loop)
{
	string ext = filesystem::path(inputFile).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return std::tolower(c); });
	
	if (images.find(ext) != images.end())
		return make_unique<ImageReader>(inputFile, loop);
	else if (video.find(ext) != video.end())
		return make_unique<VideoReader>(inputFile, loop);
	else
	{
		// TODO: perhaps, implement a webcam keyer?

		throw std::runtime_error(string("Input file type is not supported: ").append(ext));
	}
}	// createReader


class ChromaKeyer
{
public:
	ChromaKeyer(const char* windowName) : windowName(windowName) {}
	// TODO: define copy/move constructors and the assignment operators
	~ChromaKeyer();

	bool setUp(const char* inputFile);

	void keyOut(const char *inputFile, const char *backgroundFile, const char *outputFile);

private:

	static void onMouse(int event, int x, int y, int flags, void* data);

	String windowName;
	bool paramsSet = false;
	Mat curFrame;
	Scalar color;
	int tolerance = 0, softness = 0, defringe = 0;
};	// ChromaKeyer

bool ChromaKeyer::setUp(const char* inputFile)
{
	this->paramsSet = false;

	unique_ptr<MediaSource> reader = MediaFactory::createReader(inputFile, true /*loop*/);

	namedWindow(this->windowName);
	setMouseCallback(this->windowName, ChromaKeyer::onMouse, this);
	createTrackbar("Tolerance", windowName, &this->tolerance, 100, nullptr /*ChromaKeyer::onToleranceChanged*/, this);
	createTrackbar("Softness", windowName, &this->softness, 100, nullptr, this);
	createTrackbar("Defringe", windowName, &this->defringe, 100, nullptr, this);

	for (int key = 0; (key & 0xFF) != 27; )
	{
		reader->readNext(this->curFrame);
		imshow(this->windowName, this->curFrame);
		key = waitKey(10);
	}

	destroyWindow(this->windowName);

	return this->paramsSet;
}	// setUp


void ChromaKeyer::onMouse(int event, int x, int y, int flags, void* data)
{
	assert(data != nullptr);
	ChromaKeyer* keyer = static_cast<ChromaKeyer*>(data);

	if (event == EVENT_LBUTTONUP)
	{
		keyer->color = keyer->curFrame.at<Vec3b>(y, x);
		keyer->paramsSet = true;
	}
}	// onMouse

void ChromaKeyer::keyOut(const char* inputFile, const char* backgroundFile, const char* outputFile)
{
	unique_ptr<MediaSource> srcIn = MediaFactory::createReader(inputFile, false)
		, srcBg = MediaFactory::createReader(backgroundFile, true);

	unique_ptr<MediaSink> sink = MediaFactory::createWriter(outputFile);

	if (srcIn->getMediaType() == MediaSource::Video)
	{
		if (sink->getMediaType() != MediaSink::Video && sink->getMediaType() != MediaSink::Dummy)
			throw runtime_error("Mismatching media types: the input file is a video, but the output is not.");
	}
	else if (srcIn->getMediaType() == MediaSource::Image)
	{
		if (srcBg->getMediaType() != MediaSource::Image)
			throw runtime_error("Background must be an image.");

		if (sink->getMediaType() != MediaSink::Image && sink->getMediaType() != MediaSink::Dummy)
			throw runtime_error("Mismatching media types: the input file is an image, but the output is not.");
	}
	else
	{
		// TODO: check other media types 
	}

	for (int key = 0; srcIn->readNext(this->curFrame) && (key & 0xFF) != 27; )
	{
		//imshow(this->windowName, this->curFrame);
		Mat bgFrame;
		srcBg->readNext(bgFrame);

		// resize the background frame to match the input frame
		cv::resize(bgFrame, bgFrame, this->curFrame.size(), 0, 0, 
			bgFrame.rows*bgFrame.cols > this->curFrame.rows*this->curFrame.cols ? INTER_AREA : INTER_CUBIC);

		// key out using the resized background frame
		Mat resFrame = keyOutFrame(bgFrame);

		// write the resulting frame to the sink
		sink->write(resFrame);

		imshow(this->windowName, resFrame);

		key = waitKey(10);
	}	// for
}	// keyOut

ChromaKeyer::~ChromaKeyer()
{
	destroyWindow(this->windowName);
}



int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		cerr << "Usage: chromak <input video with green screen> <background file> <output file>" << endl;
		return -1;
	}

	try
	{
		ChromaKeyer keyer("Chroma Keying");
		if (keyer.setUp(argv[1]))
			keyer.keyOut(argv[1], argv[2], argv[3]);
	}
	catch (const std::exception& e)
	{
		cerr << e.what() << endl;
		return -2;
	}

	return 0;
}

//
//class ChromaKeyer
//{
//public:
//	ChromaKeyer(const char* inputFile) : inputFile(inputFile) {}
//	// TODO: define copy/move constructors and the assignment operators
//	virtual ~ChromaKeyer() = default;
//
//	String getInputFile() const { return inputFile; }
//
//	//void keyOut(const char* windowName, const char* backgroundFile, const char* outputFile);
//
//	void exec(const char* windowName, const char* backgroundFile, const char* outputFile);
//
//	constexpr int getTolerance() const noexcept { return this->tolerance; }
//
//	constexpr int getSoftness() const noexcept { return this->softness; }
//
//	constexpr int getDefringe() const noexcept { return this->defringe; }
//
//protected:
//
//	void createSettingsWindow(const char *windowName, int tolerance, int softness, int defringe);
//
//private:
//
//	virtual bool setupKeyer(const char* windowName) = 0;
//
//	virtual void keyOut(const char* windowName, const char* backgroundFile, const char* outputFile) = 0;
//
//
//	// Default implementation does nothing and is provided not to force the descendants to define their handlers
//	// unless they really need it. Override these virtual methods in order to handle events in derived classes.
//
//	static void onMouse(int event, int x, int y, int flags, void* data);
//
//	virtual void onMouse(int event, int x, int y, int flags) {}		
//	
//	static void onToleranceChanged(int pos, void* data);
//
//	virtual void onToleranceChanged(int pos) { }
//	
//	static void onSoftnessChanged(int pos, void *data);
//
//	virtual void onSoftnessChanged(int pos) { }
//
//	static void onDefringeChanged(int pos, void* data);
//
//	virtual void onDefringeChanged(int pos) {}
//
//	//void cleanupHelper() { }
//
//	String inputFile;
//	//bool bSetUp = false;
//	int tolerance = 0, softness = 0, defringe = 0;
//};	// ChromaKeyer
//
//void ChromaKeyer::exec(const char* windowName, const char* backgroundFile, const char* outputFile)
//{
//	
//	/*struct Cleaner
//	{
//		constexpr Cleaner(ChromaKeyer *pKeyer, void(ChromaKeyer::*pCleanUp)()) noexcept : pKeyer(pKeyer), pCleanUp(pCleanUp) {}
//
//		~Cleaner()	{	(pKeyer->*pCleanUp)(); }
//
//	private:
//		ChromaKeyer* pKeyer;
//		void (ChromaKeyer::*pCleanUp)();
//	} cleaner(this, &ChromaKeyer::cleanupHelper);*/
//
//	try
//	{
//		if (setupKeyer(windowName))
//			keyOut(windowName, backgroundFile, outputFile);
//
//		//cleanup(windowName);
//	}
//	catch (const std::exception&)
//	{
//		// TODO: perhaps, it makes sense to use RAII for cleanup
//		destroyWindow(windowName);
//		//this->bSetUp = false;
//		//cleanup(windowName);
//		throw;
//	}
//}	// exec
//
//
//void ChromaKeyer::createSettingsWindow(const char* windowName, int tolerance, int softness, int defringe) 
//{
//	this->tolerance = tolerance;
//	this->softness = softness;
//	this->defringe = defringe;
//
//	//destroyWindow(windowName);	
//	namedWindow(windowName);
//	setMouseCallback(windowName, ChromaKeyer::onMouse, this);
//	createTrackbar("Tolerance", windowName, &this->tolerance, 100, ChromaKeyer::onToleranceChanged, this);
//	createTrackbar("Softness", windowName, &this->softness, 100, ChromaKeyer::onSoftnessChanged, this);
//	createTrackbar("Defringe", windowName, &this->defringe, 100, ChromaKeyer::onDefringeChanged, this);
//}	// createSettingsWindow
//
//
//void ChromaKeyer::onMouse(int event, int x, int y, int flags, void* data)
//{
//	assert(data != nullptr);
//	ChromaKeyer* keyer = static_cast<ChromaKeyer*>(data);
//	keyer->onMouse(event, x, y, flags);
//}	// onMouseStatic
//
//void ChromaKeyer::onToleranceChanged(int pos, void* data)
//{
//	assert(data != nullptr);
//	static_cast<ChromaKeyer*>(data)->onToleranceChanged(pos);
//}	// onToleranceChanged
//
//void ChromaKeyer::onSoftnessChanged(int pos, void* data)
//{
//	assert(data != nullptr);
//	static_cast<ChromaKeyer*>(data)->onSoftnessChanged(pos);
//}	// onSoftnessChanged
//
//void ChromaKeyer::onDefringeChanged(int pos, void* data)
//{
//	assert(data != nullptr);
//	static_cast<ChromaKeyer*>(data)->onDefringeChanged(pos);
//}	// onDefringeChanged
//
//
//
//class ImageKeyer : public ChromaKeyer
//{
//public:
//	ImageKeyer(const char* inputFile) : ChromaKeyer(inputFile) {}
//
//protected:
//	
//private:
//	//virtual void pickColor(const char* windowName) override;
//	virtual bool setupKeyer(const char* windowName) override;
//	virtual void keyOut(const char* windowName, const char* backgroundFile, const char* outputFile) override;
//
//	// TODO: mouse and trackbar handlers
//	virtual void onMouse(int event, int x, int y, int flags) override;
//
//	bool paramsSet = false;
//	Scalar color;
//	Mat imSrc;
//};	// ImageKeyer
//
//
////void ImageKeyer::pickColor(const char* windowName)
////{
////}
//
//bool ImageKeyer::setupKeyer(const char* windowName)
//{
//	this->paramsSet = false;
//
//	createSettingsWindow(windowName, 10, 30, 20);
//
//	this->imSrc = imread(getInputFile(), IMREAD_COLOR);
//	CV_Assert(!this->imSrc.empty());
//
//	for (int key = 0; (key & 0xFF) != 27; )
//	{
//		imshow(windowName, this->imSrc);
//		key = waitKey();
//	}
//
//	destroyWindow(windowName);
//	return this->paramsSet;
//}	// setupKeyer
//
//void ImageKeyer::onMouse(int event, int x, int y, int flags)
//{
//	//assert(!this->paramsSet);
//
//	if (event == EVENT_LBUTTONDOWN)
//	{
//		this->color = this->imSrc.at<Vec3b>(y, x);
//		this->paramsSet = false;
//	}
//}
//
//void ImageKeyer::keyOut(const char* windowName, const char* backgroundFile, const char* outputFile)
//{
//
//}	// replaceBackground
//
//
//
//
//class VideoKeyer : public ChromaKeyer
//{
//public:
//	VideoKeyer(const char* inputFile) : ChromaKeyer(inputFile) {}
//
//protected:
//
//private:
//
//	//virtual void pickColor(const char* windowName) override;
//	virtual bool setupKeyer(const char* windowName) override;
//	virtual void keyOut(const char* windowName, const char* backgroundFile, const char* outputFile) override;
//
//	virtual void onMouse(int event, int x, int y, int flags) override;
//
//	//virtual void onToleranceChanged(int pos) override;
//
//	//virtual void onSoftnessChanged(int pos) override;
//
//	bool paramsSet = false;
//	Scalar color;
//	//int tolerance = 0, softness = 0;
//	Mat curFrame;
//};	// VideoKeyer
//
////void VideoKeyer::pickColor(const char* windowName)
////{
////}
//
//bool VideoKeyer::setupKeyer(const char* windowName)
//{
//	//assert(!this->paramsSet);
//
//	//namedWindow(windowName);
//	//setMouseCallback(windowName, onMouse, this);
//
//	//// TODO: add sliders
//
//	/*struct WindowDestroyer
//	{
//		WindowDestroyer(const char* windowName) : windowName(windowName) {}
//		~WindowDestroyer() { destroyWindow(windowName); }
//		const char* windowName;
//	};*/
//
//	//ChromaKeyer::WindowDestroyer destroyer(windowName);
//	//ChromaKeyer::setupKeyer(windowName, destroyer);
//	
//	//WindowDestroyer destroyer(windowName);
//
//	/*struct GuiCleaner
//	{
//		GuiCleaner() { ChromaKeyer::setupKeyer(windowName); }
//		~GuiCleaner() { destroyWindow(windowName); }
//	} cleaner;*/
//
//	//createGui(windowName);
//
//	this->paramsSet = false;
//	
//	createSettingsWindow(windowName, 10, 50, 20);	// set the default parameters
//
//	VideoCapture capIn(getInputFile());
//	CV_Assert(capIn.isOpened());
//
//	//Mat frame;
//
//	for (int frameIndex = 0, key = 0; !this->paramsSet && (key & 0xFF) != 27 ; ++frameIndex)
//	{
//		if (capIn.read(this->curFrame))
//		{
//			imshow(windowName, this->curFrame);
//			key = waitKey(10);
//		}	// frame read
//		else
//		{
//			if (frameIndex == 0)
//				throw runtime_error("Failed to read the input file. Is it empty?");
//
//			// Rewind and try to read a frame from the beginning
//			capIn.release();
//			capIn.open(getInputFile());
//			frameIndex = 0;
//			continue;
//		}	// empty frame
//	}	// for
//
//	//setMouseCallback(windowName, nullptr, 0);	// disable mouse handling
//	destroyWindow(windowName);
//
//	return this->paramsSet;
//}	// setupKeyer
//
//void VideoKeyer::onMouse(int event, int x, int y, int flags)
//{
//	assert(!this->paramsSet);
//
//	if (event == EVENT_LBUTTONDOWN)
//	{
//		this->color = this->curFrame.at<Vec3b>(x, y);
//		this->paramsSet = true;
//	}
//}	// onMouse
//
////void VideoKeyer::onToleranceChanged(int pos)
////{
////	assert(!this->paramsSet);
////	this->tolerance = pos;
////}	// onToleranceChanged
////
////void VideoKeyer::onSoftnessChanged(int pos)
////{
////	assert(!this->paramsSet);
////	this->softness = pos;
////}
//
//void VideoKeyer::keyOut(const char* windowName, const char* backgroundFile, const char* outputFile)
//{
//
//}	// keyOut
//
//
//
//
//class ChromaKeyingFactory
//{
//public:
//	//static unique_ptr<ChromaKeyer> create(const char* inputFile);
//	static unique_ptr<ChromaKeyer> create(const char* inputFile);
//	//static auto create(const char* inputFile);
//};	// ChromaKeyingFactory
//
//unique_ptr<ChromaKeyer> ChromaKeyingFactory::create(const char* inputFile)
////auto ChromaKeyingFactory::create(const char* inputFile)
//{
//	string ext = filesystem::path(inputFile).extension().string();
//	std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return std::tolower(c); });
//	
//	//static array<string, 4> images{ {".jpg", ".jpeg", ".png", ".bmp" } };
//	//static array<string, 2> video{ {".mp4", ".avi"} };
//	static set<string> images{".jpg", ".jpeg", ".png", ".bmp" };
//	static set<string> video{".mp4", ".avi"};
//
//	// TODO: perhaps, implement a webcam keyer?
//
//	if (images.find(ext) != images.end())
//		return make_unique<ImageKeyer>(inputFile);
//	else if (video.find(ext) != video.end())
//		return make_unique<VideoKeyer>(inputFile);
//	else 
//		throw std::runtime_error(string("Input file type is not supported: ").append(ext));
//}	// create
//
//int main(int argc, char* argv[])
//{
//	if (argc != 4)
//	{
//		cerr << "Usage: chromak <input video with green screen> <background file> <output file>" << endl;
//		return -1;
//	}
//
//	try
//	{
//		unique_ptr keyer = ChromaKeyingFactory::create(argv[1]);
//		keyer->exec("Chroma Keying", argv[2], argv[3]);
//	}
//	catch (const std::exception& e)		// cv::Exception inherits from std::exception
//	{
//		cerr << e.what() << endl;
//		return -2;
//	}
//
//	return 0;
//}