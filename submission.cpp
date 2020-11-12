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

	const string& getMediaPath() const noexcept { return this->mediaPath; }

	bool isLooped() const noexcept { return this->looped; }

	virtual bool readNext(Mat &frame) = 0;
	
	virtual void reset() = 0;

	virtual ~MediaSource() = default;

protected:
	
	MediaSource(MediaType inputType, const string &mediaPath, bool looped)
		: mediaType(inputType)
		, mediaPath(mediaPath)
		, looped(looped) {}

	MediaSource(const MediaSource&) = default;
	MediaSource(MediaSource&&) = default;

	MediaSource& operator = (const MediaSource&) = default;
	MediaSource& operator = (MediaSource&&) = default;

private:
	MediaType mediaType;
	string mediaPath;
	bool looped = false;
};	// MediaSource


class ImageFileReader : public MediaSource
{
public:
	ImageFileReader(const string& imageFile, bool looped = false);
	ImageFileReader(const ImageFileReader&) = delete;
	ImageFileReader(ImageFileReader&& other) = default;

	ImageFileReader& operator = (const ImageFileReader&) = delete;
	ImageFileReader& operator = (ImageFileReader&& other) = default;

	bool readNext(Mat& frame) override;
	void reset() override;

private:
	Mat cache;
};	// ImageFileReader


ImageFileReader::ImageFileReader(const string &imageFile, bool looped)
	: MediaSource(MediaSource::ImageFile, filesystem::exists(imageFile) ? imageFile : throw runtime_error("Input image doesn't exist: " + imageFile), looped)
{
	if (!cv::haveImageReader(imageFile))
		throw runtime_error("No decoder for this image file: " + imageFile);
}	// ctor

bool ImageFileReader::readNext(Mat& frame)
{
	if (this->cache.empty())
	{
		frame = imread(getMediaPath(), IMREAD_COLOR);
		CV_Assert(!frame.empty());
		frame.copyTo(this->cache);
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
	VideoFileReader(const string &videoFile, bool looped = false);
	VideoFileReader(const VideoFileReader&) = delete;
	VideoFileReader(VideoFileReader&& other) = default;

	VideoFileReader& operator = (const VideoFileReader&) = delete;
	VideoFileReader& operator = (VideoFileReader&& other) = default;

	virtual bool readNext(Mat& frame) override;

	virtual void reset() override;

private:
	String inputFile;
	VideoCapture cap;
};	// VideoFileReader

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
		// Close and try reading again
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

	MediaSink(const MediaSink&) = default;
	MediaSink(MediaSink&&) = default;

	MediaSink& operator = (const MediaSink&) = default;
	MediaSink& operator = (MediaSink&&) = default;

private:
	MediaType mediaType;
	string mediaPath;
};	// MediaSink


class DummyWriter: public MediaSink
{
public:
	DummyWriter() : MediaSink(MediaSink::Dummy, "") {}
	DummyWriter(const DummyWriter&) = default;
	DummyWriter(DummyWriter&&) = default;

	DummyWriter& operator = (const DummyWriter&) = default;
	DummyWriter& operator = (DummyWriter&&) = default;

	virtual void write(const Mat &frame) override { }
};	// DummyWriter


class ImageFileWriter: public MediaSink
{
public:
	ImageFileWriter(const string& imageFile, Size frameSize)
		: MediaSink(MediaSink::ImageFile, cv::haveImageWriter(imageFile) ? imageFile : throw runtime_error("No encoder for this image file: " + imageFile))
		, frameSize(move(frameSize)) { }
	
	ImageFileWriter(const ImageFileWriter&) = delete;
	ImageFileWriter(ImageFileWriter&&) = default;

	ImageFileWriter& operator = (const ImageFileWriter&) = delete;
	ImageFileWriter& operator = (ImageFileWriter&&) = default;

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
	VideoFileWriter(const string& videoFile, Size frameSize, const char (&fourcc)[4], double fps)
		: MediaSink(MediaSink::VideoFile, videoFile)
		, writer(videoFile, VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]), fps, move(frameSize), true) {	}

	VideoFileWriter(const VideoFileWriter&) = delete;
	VideoFileWriter(VideoFileWriter&&) = default;

	VideoFileWriter& operator = (const VideoFileWriter&) = delete;
	VideoFileWriter& operator = (VideoFileWriter&&) = default;

	virtual void write(const Mat& frame) override;

private:
	VideoWriter writer;
};	// VideoWriter


void VideoFileWriter::write(const Mat& frame)
{
	writer.write(frame);
}	// write



class MediaFactory
{
public:
	static unique_ptr<MediaSource> createReader(const string &input, bool loop = false);
	static unique_ptr<MediaSink> createWriter(const string &output, Size frameSize);

private:
	static const set<string> images;
	static const set<string> video;

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

unique_ptr<MediaSource> MediaFactory::createReader(const string &input, bool loop)
{
	string ext = getFileExtension(input);
	
	if (images.find(ext) != images.end())
		return make_unique<ImageFileReader>(input, loop);
	else if (video.find(ext) != video.end())
		return make_unique<VideoFileReader>(input, loop);
	else 
	{
		// TODO: may handle other input types here, e.g. webcam

		throw std::runtime_error(string("Input file type is not supported: ").append(ext));
	}
}	// createReader


unique_ptr<MediaSink> MediaFactory::createWriter(const string &output, Size frameSize)
{
	if (output.empty())
		return make_unique<DummyWriter>();

	string ext = MediaFactory::getFileExtension(output);
	if (images.find(ext) != images.end())
		return make_unique<ImageFileWriter>(output, frameSize);
	else if (video.find(ext) != video.end())
		// Help the compiler to deduce the argument types which are passed to the constructor
		return make_unique<VideoFileWriter, const string &, Size, const char(&)[4], double>(output, move(frameSize), { 'm','p','4','v' }, 30);
	else
	{
		// TODO: consider adding other sinks

		throw runtime_error(string("Output file type is not supported: ").append(ext));
	}
}	// createWriter



class ChromaKeyer
{
public:
	ChromaKeyer(string windowName) : windowName(move(windowName)) {}
	ChromaKeyer(const ChromaKeyer&) = delete;
	ChromaKeyer(ChromaKeyer&&) = default;
	~ChromaKeyer();

	ChromaKeyer& operator = (const ChromaKeyer&) = delete;
	ChromaKeyer& operator = (ChromaKeyer&&) = delete;

	bool setUp(const string &inputFile);

	void keyOut(const string &inputFile, const string &backgroundFile, const string &outputFile);

private:

	Mat & keyOutFrame(const Scalar& colorHSV);

	static void onMouse(int event, int x, int y, int flags, void* data);

	String windowName;
	bool paramsSet = false;
	int tolerance = 12, softness = 2, defringe = 40;	// default parameters
	Scalar color;

	// Holding matrices in member variables saves memory allocations
	Mat inputFrameBGR, backgroundBGR, outputFrameBGR;
	Mat inputFrameBGRF, backgroundBGRF;
	Mat3f inputFrameHSVF;
	Mat1b maskB;
	Mat1f maskF;
	Mat3f mask3F;
};	// ChromaKeyer

bool ChromaKeyer::setUp(const string &inputFile)
{
	this->paramsSet = false;

	unique_ptr<MediaSource> reader = MediaFactory::createReader(inputFile, true /*loop*/);

	namedWindow(this->windowName);
	setMouseCallback(this->windowName, ChromaKeyer::onMouse, this);
	createTrackbar("Tolerance", windowName, &this->tolerance, 100, nullptr, this);
	createTrackbar("Softness", windowName, &this->softness, 10, nullptr, this);
	createTrackbar("Defringe", windowName, &this->defringe, 100, nullptr, this);

	for (int key = 0; !this->paramsSet && (key & 0xFF) != 27; )
	{
		reader->readNext(this->inputFrameBGR);
		imshow(this->windowName, this->inputFrameBGR);
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
		keyer->color = keyer->inputFrameBGR.at<Vec3b>(y, x);
		keyer->paramsSet = true;
	}
}	// onMouse

void ChromaKeyer::keyOut(const string &inputFile, const string &backgroundFile, const string &outputFile)
{
	assert(this->paramsSet);

	unique_ptr<MediaSource> srcIn = MediaFactory::createReader(inputFile, false)	// input is not looped
		, srcBg = MediaFactory::createReader(backgroundFile, true);		// background is looped

	unique_ptr<MediaSink> sink = MediaFactory::createWriter(outputFile, this->inputFrameBGR.size());	// frame size obtained during the parameters setting phase

	int delay = 10;		// keystroke waiting period 

	if (srcIn->getMediaType() == MediaSource::VideoFile)
	{
		if (sink->getMediaType() != MediaSink::VideoFile && sink->getMediaType() != MediaSink::Dummy)
			throw runtime_error("Mismatching media types: the input file is a video, but the output is not.");
	}
	else if (srcIn->getMediaType() == MediaSource::ImageFile)
	{
		delay = 0;		// it's probably better to show the output image for longer time before closing up

		if (srcBg->getMediaType() != MediaSource::ImageFile)
			throw runtime_error("Background must be an image.");

		if (sink->getMediaType() != MediaSink::ImageFile && sink->getMediaType() != MediaSink::Dummy)
			throw runtime_error("Mismatching media types: the input file is an image, but the output is not.");
	}
	else
	{
		// TODO: check other media types 
	}


	// Convert the chosen color from BGR to HSV
	Mat4f colorMatF((Vec4f)(this->color / 255));
	Mat3f colorMatHSV;
	cvtColor(colorMatF, colorMatHSV, COLOR_BGR2HSV, 3);
	Scalar colorHSV = colorMatHSV.at<Vec3f>();

	// Process input frames from the source until we are done or the user hits Escape
	for (int key = 0; srcIn->readNext(this->inputFrameBGR) && (key & 0xFF) != 27; )
	{
		// Read the next background frame. Since this media source is looped, the same image will be returned in case the background file is an image.
		// In case the background is a video, we will read as many frames as it's needed to match the input stream, or start from the beginning, 
		// if it's shorter.
		srcBg->readNext(this->backgroundBGR);

		// Resize the background frame to match the input frame
		cv::resize(this->backgroundBGR, this->backgroundBGR, this->inputFrameBGR.size(), 0, 0,
			this->backgroundBGR.rows * this->backgroundBGR.cols > this->inputFrameBGR.rows * this->inputFrameBGR.cols ? INTER_AREA : INTER_CUBIC);

		// Key out using the resized background frame
		const Mat &resFrame = keyOutFrame(colorHSV);

		// Write the resulting frame to the sink
		sink->write(resFrame);

		// Display the output frame
		imshow(this->windowName, resFrame);

		key = waitKey(delay);
	}	// for

	destroyWindow(this->windowName);
}	// keyOut

Mat & ChromaKeyer::keyOutFrame(const Scalar& colorHSV)
{
	CV_Assert(this->inputFrameBGR.size() == this->backgroundBGR.size());

	// Convert pixel values from 0..255 to 0..1 range so as to perform arithmetic operations with semi-transparent masks.
	// It is also recommended to use 32-bit images for HSV conversion.
	this->inputFrameBGR.convertTo(this->inputFrameBGRF, CV_32F, 1.0/255);
	this->backgroundBGR.convertTo(this->backgroundBGRF, CV_32F, 1.0/255);
	
	// Convert the input frame to HSV, which is easier to analyze
	cvtColor(this->inputFrameBGRF, this->inputFrameHSVF, COLOR_BGR2HSV, 3);	


	// Prepare the color range for green screen masking.
	// We are mainly interested in hues (which must be close to green). However, the value and saturation ranges control how we
	// treat pixels near the edge of the green background and foreground objects. Green is somewhat different in these areas,
	// probably, because it is shaded or cast on the foreground objects. 

	Scalar hsvRange{ 360, 1, 1 };
	assert(this->tolerance >= 0 && this->tolerance <= 100);
	assert(this->defringe >= 0 && this->defringe <= 100);
	Scalar lowerHSV = { colorHSV[0] - this->tolerance / 100.0 * hsvRange[0], this->defringe/100.0, this->defringe/100.0 }
		, upperHSV = { colorHSV[0] + this->tolerance / 100.0 * hsvRange[0], hsvRange[1], hsvRange[2] };

	inRange(this->inputFrameHSVF, lowerHSV, upperHSV, this->maskB);

	// Account for hue values wrapping around the 360-degree cycle
	
	Mat1b orMaskB;
	if (lowerHSV[0] < 0 && upperHSV[0] < hsvRange[0])
	{
		lowerHSV[0] = lowerHSV[0] + hsvRange[0];
		upperHSV[0] = hsvRange[0];
		inRange(this->inputFrameHSVF, lowerHSV, upperHSV, orMaskB);
		bitwise_or(this->maskB, orMaskB, this->maskB);
	}
	else if (lowerHSV[0] > 0 && upperHSV[0] > hsvRange[0])
	{
		lowerHSV[0] = 0;
		upperHSV[0] = upperHSV[0] - hsvRange[0];
		inRange(this->inputFrameHSVF, lowerHSV, upperHSV, orMaskB);
		bitwise_or(this->maskB, orMaskB, this->maskB);
	}

	// Convert the mask values from 0..255 to 0..1 range
	this->maskB.convertTo(this->maskF, CV_32F, 1.0 / 255);

	// Make foreground borders less rigid
	if (this->softness > 0)
	{
		int ksize = 2*this->softness + 1;
		dilate(this->maskF, this->maskF, getStructuringElement(MORPH_RECT, Size(ksize, ksize)));		
		GaussianBlur(this->maskF, this->maskF, Size(ksize, ksize), 0, 0);
	}

	// To perform arithmetic operations with a mask, we need it to have the same number of channels as a source matrix
	merge(vector<Mat1f>{maskF, maskF, maskF}, this->mask3F);

	// Our mask contains values of around 1 where frame pixels are close to green.
	// Thus we can extract pixel values from the background frame which correspond to the green screen.
	multiply(this->backgroundBGRF, this->mask3F, this->backgroundBGRF);
	
	// Invert the mask to extract pixel values of the foreground
	this->mask3F.convertTo(this->mask3F, -1, -1, 1.0);	// a slightly faster way to compute Scalar::all(1.0) - mask3F
	multiply(this->inputFrameBGRF, this->mask3F, this->inputFrameBGRF);
	
	// Combine the foreground with a new background
	this->inputFrameBGRF += this->backgroundBGRF;

	// Scale pixel values back from 0..1 to 0..255 range
	this->inputFrameBGRF.convertTo(this->outputFrameBGR, CV_8U, 255);

	return this->outputFrameBGR;
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
