#ifndef DUMMYWRITER_H
#define DUMMYWRITER_H


#include <opencv2/core.hpp>


class DummyWriter: public MediaSink
{
public:
	DummyWriter() : MediaSink(MediaSink::Dummy, "") {}
	DummyWriter(const DummyWriter&) = default;
	DummyWriter(DummyWriter&&) = default;

	DummyWriter& operator = (const DummyWriter&) = default;
	DummyWriter& operator = (DummyWriter&&) = default;

	virtual void write(const cv::Mat &frame) override { }
};	// DummyWriter


#endif	// DUMMYWRITER_H
