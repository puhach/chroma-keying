# Chroma Keying

This GUI application allows a user to replace a background in green-screen images and videos with a new background image or video.

## Set Up

It is assumed that OpenCV 4.x, C++11 compiler, and cmake 2.18.12 or newer are installed on the system.

### Specify OpenCV_DIR in CMakeLists

Open CMakeLists.txt and set the correct OpenCV directory in the following line:

```
SET(OpenCV_DIR /home/hp/workfolder/OpenCV-Installation/installation/OpenCV-master/lib/cmake/opencv4)
```

Depending on the platform and the way OpenCV was installed, it may be needed to provide the path to cmake files explicitly. On my KUbuntu 20.04 after building OpenCV 4.4.0 from sources the working `OpenCV_DIR` looks like <OpenCV installation path>/lib/cmake/opencv4. On Windows 8.1 after installing a binary distribution of OpenCV 4.2.0 it is C:\OpenCV\build.


### Build the Project

In the root of the project folder create the `build` directory unless it already exists. Then from the terminal run the following:

```
cd build
cmake ..
```

This should generate the build files. When it's done, compile the code:

```
cmake --build . --config Release
```

## Usage

The program has to be run from the command line. It can take two or three arguments: 

```
chromak <input> <background> [<output>]
```

Where *input* is an image or a video with a green screen background, *background* is an image or a video to be used as a new background, *output* is an optional output file where the resulting image or video will be stored. In case the output file name is not provided, the result will only be displayed in the application window.

The project folder contains images and videos which may be used for testing.

Sample usage (linux):
```
./chromak ../video/greenscreen-demo.mp4 ../video/hillside.mp4 ./out.mp4
```

To perform chroma keying the user should set three parameters using corresponding sliders: 

**tolerance** controls how much green hues can diverge from the chosen color

**softness** makes foreground edges softer or sharper

**defringe** aids in reducing feathering and green cast effects

Once these parameters have been set, pick the color by left-clicking on the preview. Then keying out starts. Resulting frames will be written to the output file and displayed as they are processed. 

To cancel the process, press **Escape**. If you do this, the app will return to the setup window where new parameters can be set. Move the sliders, then click on the preview to choose a different color and restart keying out. Or, if you just want to quit, press **Escape** again. 






