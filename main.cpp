//#include <opencv2/core.hpp>
//#include <opencv2/imgcodecs.hpp>
//#include <opencv2/imgproc.hpp>
//#include <opencv2/highgui.hpp>
//#include <iostream>
//#include <filesystem>
//#include <set>
//#include <chrono>

#include "chromakeyer.h"

#include <string>
#include <iostream>

using namespace std;
//using namespace cv;





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
		while (keyer.setUp(inputFile))	
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
