#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <filesystem>
#include <set>
#include <chrono>

using namespace std;
using namespace cv;

class MediaSource
{
public:

	enum MediaType
	{
		ImageFile,
		VideoFile,
		Webcam
	};

	MediaType getMediaType() const noexcept { return this->mediaType; }

	//string_view getMediaPath() const noexcept {	return this->mediaPath; }
	const string& getMediaPath() const noexcept { return this->mediaPath; }
	//string getMediaPath() const { return this->mediaPath; }

	bool isLooped() const noexcept { return this->looped; }

	virtual bool readNext(Mat &frame) = 0;
	
	virtual void reset() = 0;

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


class ImageFileReader : public MediaSource
{
public:
	//ImageReader(const char* imageFile, bool looped = false);
	//ImageReader(string imageFile, bool looped = false);
	ImageFileReader(const string& imageFile, bool looped = false);
		
	// TODO: implement copy/move constructors and assignment operators
	~ImageFileReader() = default;

	bool readNext(Mat& frame) override;

	void reset() override;

private:
	//bool imageRead = false;
	Mat cache;
};	// ImageFileReader


ImageFileReader::ImageFileReader(const string &imageFile, bool looped)
	//: MediaSource(MediaSource::ImageFile, cv::haveImageReader(imageFile) ? imageFile : throw runtime_error("No decoder for this image file: " + imageFile), looped)
	: MediaSource(MediaSource::ImageFile, filesystem::exists(imageFile) ? imageFile : throw runtime_error("Input image doesn't exist: " + imageFile), looped)
{
	if (!cv::haveImageReader(imageFile))
		throw runtime_error("No decoder for this image file: " + imageFile);
}	// ctor

bool ImageFileReader::readNext(Mat& frame)
{
	//if (this->imageRead && !isLooped())
	//	return false;	// don't double read

	////frame = imread(String{ getMediaPath() }, IMREAD_COLOR);
	//frame = imread(getMediaPath(), IMREAD_COLOR);
	//CV_Assert(!frame.empty());	
	//return (this->imageRead = true);
	
	if (this->cache.empty())
	{
		this->cache = imread(getMediaPath(), IMREAD_COLOR);
		CV_Assert(!this->cache.empty());
		this->cache.copyTo(frame);
		return true;
	}	// cache empty
	else
	{
		if (isLooped())
		{
			this->cache.copyTo(frame);
			return true;
		}
		else return false;
	}	// image cached
}	// readNext

void ImageFileReader::reset()
{
	this->cache.release();	// this will force the image to be reread
}	// reset


class VideoFileReader : public MediaSource
{
public:
	//VideoFileReader(const char* inputFile, bool loop = false);
	//VideoFileReader(string inputFile, bool looped = false);
	VideoFileReader(const string &videoFile, bool looped = false);

	virtual bool readNext(Mat& frame) override;

	virtual void reset() override;

private:
	String inputFile;
	VideoCapture cap;
};	// VideoFileReader

//VideoReader::VideoReader(string inputFile, bool looped)
//	: MediaSource(MediaSource::Video, std::move(inputFile), looped)
VideoFileReader::VideoFileReader(const string &videoFile, bool looped)
	: MediaSource(MediaSource::VideoFile, filesystem::exists(videoFile) ? videoFile : throw runtime_error("Input video doesn't exist: "+videoFile), looped)
	, cap(videoFile)
{
	CV_Assert(cap.isOpened());
}

bool VideoFileReader::readNext(Mat& frame)
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

void VideoFileReader::reset()
{
	cap.release();
	CV_Assert(cap.open(getMediaPath()));
}	// reset


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

	const string& getMediaPath() const noexcept { return this->mediaPath; }

	virtual void write(const Mat &frame) = 0;

	virtual ~MediaSink() = default;

protected:
	MediaSink(MediaType mediaType, const string& mediaPath)
		: mediaType(mediaType)
		, mediaPath(mediaPath) {}	// std::string's copy constructor is not noexcept 

private:
	MediaType mediaType;
	string mediaPath;
};	// MediaSink


class DummyWriter: public MediaSink
{
public:
	DummyWriter() : MediaSink(MediaSink::Dummy, "") {}
	// TODO: define copy/move constructors and assignment operators
	~DummyWriter() = default;

	virtual void write(const Mat &frame) override { }
};	// DummyWriter


class ImageFileWriter: public MediaSink
{
public:
	ImageFileWriter(const string& imageFile, Size frameSize)
		: MediaSink(MediaSink::ImageFile, cv::haveImageWriter(imageFile) ? imageFile : throw runtime_error("No encoder for this image file: " + imageFile))
		, frameSize(move(frameSize)) { }
	// TODO: implement copy/move ctors and assignment operators

	virtual void write(const Mat& frame) override;

private:
	Size frameSize;
};	// ImageFileWriter

void ImageFileWriter::write(const Mat& frame)
{
	CV_Assert(frame.size() == this->frameSize);
	if (!imwrite(getMediaPath(), frame))
		throw runtime_error("Failed to write the output image.");
}	// write


class VideoFileWriter : public MediaSink
{
public:
	VideoFileWriter(const string& videoFile, Size frameSize, const char (&fourcc)[4], double fps);
	// TODO: implement copy/move ctors and assignment operators

	virtual void write(const Mat& frame) override;

private:
	VideoWriter writer;
	//Size frameSize;
};	// VideoWriter

// TODO: can be defined in-class
VideoFileWriter::VideoFileWriter(const string& videoFile, Size frameSize, const char (&fourcc)[4], double fps)
	: MediaSink(MediaSink::VideoFile, videoFile)
	, writer(videoFile, VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]), fps, move(frameSize), true)
	//, frameSize(move(frameSize))
{
}

void VideoFileWriter::write(const Mat& frame)
{
	writer.write(frame);
}	// write



class MediaFactory
{
public:
	static unique_ptr<MediaSource> createReader(const string &inputFile, bool loop = false);
	static unique_ptr<MediaSink> createWriter(const string &outputFile, Size frameSize);

private:
	static const set<string> images;
	static const set<string> video;
	//static const set<string> images{ ".jpg", ".jpeg", ".png", ".bmp" };
	//static const set<string> video{".mp4", ".avi"};

	static string getFileExtension(const string &inputFile);
};	// MediaFactory

const set<string> MediaFactory::images{ ".jpg", ".jpeg", ".png", ".bmp" };
const set<string> MediaFactory::video{ ".mp4", ".avi"};

string MediaFactory::getFileExtension(const string &fileName)
{
	string ext = filesystem::path(fileName).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return std::tolower(c); });
	return ext;
}

unique_ptr<MediaSource> MediaFactory::createReader(const string &inputFile, bool loop)
{
	string ext = getFileExtension(inputFile);
	
	if (images.find(ext) != images.end())
		return make_unique<ImageFileReader>(inputFile, loop);
	else if (video.find(ext) != video.end())
		return make_unique<VideoFileReader>(inputFile, loop);
	else 
	{
		// TODO: perhaps, implement a webcam keyer?

		throw std::runtime_error(string("Input file type is not supported: ").append(ext));
	}
}	// createReader


unique_ptr<MediaSink> MediaFactory::createWriter(const string &outputFile, Size frameSize)
{
	if (outputFile.empty())
		return make_unique<DummyWriter>();

	string ext = MediaFactory::getFileExtension(outputFile);
	if (images.find(ext) != images.end())
		return make_unique<ImageFileWriter>(outputFile, frameSize);
	else if (video.find(ext) != video.end())
	{
		//char* pfcc = new char[4];
		//char fcc[] = { 'x','v','i' };

		// Help the compiler to deduce the argument types which are passed to the constructor
		return make_unique<VideoFileWriter, const string &, Size, const char(&)[4], double>(outputFile, move(frameSize), { 'm','p','4','v' }, 30);
		//return make_unique<VideoFileWriter>(outputFile, frameSize, "XVID", 30);
	}
	else
	{
		// TODO: consider implementing other sinks
		throw runtime_error(string("Output file type is not supported: ").append(ext));
	}
}	// createWriter



class ChromaKeyer
{
public:
	ChromaKeyer(const char* windowName) : windowName(windowName) {}
	// TODO: define copy/move constructors and the assignment operators
	~ChromaKeyer();

	//bool setUp(const char* inputFile);
	bool setUp(const string &inputFile);

	//void keyOut(const char *inputFile, const char *backgroundFile, const char *outputFile);
	void keyOut(const string &inputFile, const string &backgroundFile, const string &outputFile);

private:

	Mat keyOutFrame(const Mat &background);

	static void onMouse(int event, int x, int y, int flags, void* data);

	String windowName;
	bool paramsSet = false;
	Mat curFrame;
	Scalar color;
	//int tolerance = 10, softness = 3, defringe = 40;
	int tolerance = 12, softness = 2, defringe = 40;	// default parameters
};	// ChromaKeyer

//bool ChromaKeyer::setUp(const char* inputFile)
bool ChromaKeyer::setUp(const string &inputFile)
{
	this->paramsSet = false;
	//this->tolerance = 10;
	//this->softness = 3;
	//this->defringe = 20;

	unique_ptr<MediaSource> reader = MediaFactory::createReader(inputFile, true /*loop*/);

	namedWindow(this->windowName);
	setMouseCallback(this->windowName, ChromaKeyer::onMouse, this);
	createTrackbar("Tolerance", windowName, &this->tolerance, 100, nullptr /*ChromaKeyer::onToleranceChanged*/, this);
	createTrackbar("Softness", windowName, &this->softness, 10, nullptr, this);
	createTrackbar("Defringe", windowName, &this->defringe, 100, nullptr, this);

	for (int key = 0; !this->paramsSet && (key & 0xFF) != 27; )
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

//void ChromaKeyer::keyOut(const char* inputFile, const char* backgroundFile, const char* outputFile)
void ChromaKeyer::keyOut(const string &inputFile, const string &backgroundFile, const string &outputFile)
{
	assert(this->paramsSet);

	unique_ptr<MediaSource> srcIn = MediaFactory::createReader(inputFile, false)
		, srcBg = MediaFactory::createReader(backgroundFile, true);

	unique_ptr<MediaSink> sink = MediaFactory::createWriter(outputFile, this->curFrame.size());	// frame size obtained during the parameters setting phase

	int delay = 10;	// TODO: add delay as a function parameter

	if (srcIn->getMediaType() == MediaSource::VideoFile)
	{
		if (sink->getMediaType() != MediaSink::VideoFile && sink->getMediaType() != MediaSink::Dummy)
			throw runtime_error("Mismatching media types: the input file is a video, but the output is not.");
	}
	else if (srcIn->getMediaType() == MediaSource::ImageFile)
	{
		delay = 0;

		if (srcBg->getMediaType() != MediaSource::ImageFile)
			throw runtime_error("Background must be an image.");

		if (sink->getMediaType() != MediaSink::ImageFile && sink->getMediaType() != MediaSink::Dummy)
			throw runtime_error("Mismatching media types: the input file is an image, but the output is not.");
	}
	else
	{
		// TODO: check other media types 
	}

	for (int key = 0; srcIn->readNext(this->curFrame) && (key & 0xFF) != 27; )
	{
		std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
		//imshow(this->windowName, this->curFrame);
		Mat bgFrame;
		srcBg->readNext(bgFrame);

		cout << "bg read: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t).count() << endl;


		// resize the background frame to match the input frame
		cv::resize(bgFrame, bgFrame, this->curFrame.size(), 0, 0, 
			bgFrame.rows*bgFrame.cols > this->curFrame.rows*this->curFrame.cols ? INTER_AREA : INTER_CUBIC);

		// key out using the resized background frame
		Mat resFrame = keyOutFrame(bgFrame);

		t = std::chrono::steady_clock::now();

		// write the resulting frame to the sink
		sink->write(resFrame);

		cout << "res.write: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t).count() << endl;

		imshow(this->windowName, resFrame);

		key = waitKey(delay);
	}	// for

	destroyWindow(this->windowName);
}	// keyOut

Mat ChromaKeyer::keyOutFrame(const Mat& background)
{
	auto t = std::chrono::steady_clock::now();
	Mat frameF, bgF;
	this->curFrame.convertTo(frameF, CV_32F, 1.0/255);
	background.convertTo(bgF, CV_32F, 1.0/255);
	cout << "conv. to float: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t).count() << endl;

	t = std::chrono::steady_clock::now();
	Mat3f frameHSV, bgHSV;	
	cvtColor(frameF, frameHSV, COLOR_BGR2HSV, 3);
	cvtColor(bgF, bgHSV, COLOR_BGR2HSV, 3);
	cout << "cvtcolor: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t).count() << endl;

	// TODO: perhaps, perform this conversion once before keying out
	Mat4f colorMatF((Vec4f)(this->color/255));
	Mat3f colorMatHSV;
	cvtColor(colorMatF, colorMatHSV, COLOR_BGR2HSV, 3);
	Scalar colorHSV = colorMatHSV.at<Vec3f>();


	t = std::chrono::steady_clock::now();
	Scalar hsvRange{ 360, 1, 1 };
	//double tolUp = 1 + this->tolerance / 100.0, tolLo = 1 - this->tolerance / 100.0;
	//assert(tolUp <= 2 && tolLo >= 0);
	//Scalar lowerBound = colorHSV * tolUp;
	assert(this->tolerance >= 0 && this->tolerance <= 100);
	assert(this->defringe >= 0 && this->defringe <= 100);
	Scalar lowerHSV = { colorHSV[0] - this->tolerance / 100.0 * hsvRange[0], this->defringe/100.0, this->defringe/100.0 }
		, upperHSV = { colorHSV[0] + this->tolerance / 100.0 * hsvRange[0], hsvRange[1], hsvRange[2] };

	///scaleAdd(colorHSV, -this->tolerance/100.0, colorHSV, lowerHSV);
	///scaleAdd(colorHSV, +this->tolerance/100.0, colorHSV, upperHSV);
	//scaleAdd(hsvRange, -this->tolerance/100.0, colorHSV, lowerHSV);
	//scaleAdd(hsvRange, +this->tolerance/100.0, colorHSV, upperHSV);

	Mat1b maskB, orMaskB;
	inRange(frameHSV, lowerHSV, upperHSV, maskB);

	//imshow(this->windowName, maskB);
	//waitKey();

	// Hue values wrap around the 360-degree cycle
	if (lowerHSV[0] < 0 && upperHSV[0] < hsvRange[0])
	{
		lowerHSV[0] = lowerHSV[0] + hsvRange[0];
		upperHSV[0] = hsvRange[0];
		inRange(frameHSV, lowerHSV, upperHSV, orMaskB);

		//imshow(this->windowName, orMaskB);
		//waitKey();

		bitwise_or(maskB, orMaskB, maskB);

		//imshow(this->windowName, maskB);
		//waitKey();

		/*Point minLoc;
		minMaxLoc(maskB, nullptr, nullptr, &minLoc);
		cout << frameHSV.at<Vec3f>(minLoc) << endl;*/
	}
	else if (lowerHSV[0] > 0 && upperHSV[0] > hsvRange[0])
	{
		lowerHSV[0] = 0;
		upperHSV[0] = upperHSV[0] - hsvRange[0];
		inRange(frameHSV, lowerHSV, upperHSV, orMaskB);
		bitwise_or(maskB, orMaskB, maskB);
	}

	Mat1f maskF;
	maskB.convertTo(maskF, CV_32F, 1.0 / 255);

	cout << "mask: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t).count() << endl;

	//imshow(this->windowName, maskB);
	//waitKey();

	//t = std::chrono::steady_clock::now();
	//erode(maskF, maskF, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)), Point(-1, -1), 1);	// TEST!
	//dilate(maskF, maskF, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)), Point(-1, -1), this->defringe);
	///dilate(maskF, maskF, getStructuringElement(MORPH_ELLIPSE, Size(11, 11)));
	///morphologyEx(maskF, maskF, MORPH_OPEN, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)), Point(-1,-1), 3);
	//cout << "dilate: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t).count() << endl;

	//imshow(this->windowName, maskF);
	//waitKey();

	///GaussianBlur(mask, mask, Size(0, 0), this->softness / 100.0, this->softness / 100.0);
	//GaussianBlur(maskF, maskF, Size(2*this->softness+1, 2*this->softness+1), 0, 0);
	//GaussianBlur(maskF, maskF, Size(2*this->softness+1,2*this->softness+1), 0, 0);
	if (this->softness > 0)
	{
		int ksize = 2*this->softness + 1;
		dilate(maskF, maskF, getStructuringElement(MORPH_RECT, Size(ksize, ksize)));		
		GaussianBlur(maskF, maskF, Size(ksize, ksize), 0, 0);
	}

	//imshow(this->windowName, maskF);
	//waitKey();

	//addWeighted(frameHSV, );
	//Mat fgHSV;
	//Mat3f invMask = Vec3f::all(1) - maskF;
	Mat3f mask3f;
	merge(vector<Mat1f>{maskF, maskF, maskF}, mask3f);
	//multiply(frameHSV, (Scalar::all(1) - maskF), fgHSV);
	Mat3f maskInv = Scalar::all(1.0) - mask3f;
	//imshow(this->windowName, maskInv);
	//waitKey();

	//multiply(frameHSV, Scalar::all(1.0)-mask3f, fgHSV);
	Mat3f fgBGRF, bgBGRF;
	multiply(frameF, maskInv, fgBGRF);
	multiply(bgF, mask3f, bgBGRF);
	

	//imshow(this->windowName, fgBGRF);
	//waitKey();

	Mat3f resBGRF;
	add(fgBGRF, bgBGRF, resBGRF);

	//Mat3f resBGRF;
	//cvtColor(resHSV, resBGRF, COLOR_HSV2BGR);
	resBGRF.convertTo(this->curFrame, CV_8U, 255);

	//imshow(this->windowName, resBGRF);
	//waitKey();

	return this->curFrame;
}	// keyOutFrame

ChromaKeyer::~ChromaKeyer()
{
	destroyWindow(this->windowName);
}



int main(int argc, char* argv[])
{
	if (argc != 3 && argc != 4)
	{
		cerr << "Usage: chromak <input video with green screen> <background file> [<output file>]" << endl;
		return -1;
	}

	try
	{
		string inputFile(argv[1]), backgroundFile(argv[2]), outputFile(argc > 3 ? argv[3] : "");

		ChromaKeyer keyer("Chroma Keying");
		while (keyer.setUp(inputFile))	// TODO
		{
			keyer.keyOut(inputFile, backgroundFile, outputFile);
		}
	}
	catch (const std::exception& e)
	{
		cerr << e.what() << endl;
		return -2;
	}

	return 0;
}
