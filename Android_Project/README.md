UnderwaterGPS APP
============
UnderwaterGPS APP is an Android application that plays and records sounds with the C++ OpenSLES API using JNI. This app is the implementation of our distributed underwater localization protocol. Currently, this App can perform our distributed localization protocol with the preamble exchanging among multiple devices and save the essential raw data to the disk. The localization algorithm part is deployed in the folder "Offline_Process".

# Pre-requisites

- Android Studio 2.2+ with [NDK](https://developer.android.com/ndk/) bundle.


# Code Structures
 ### fftw3/
 The library and source code for fftw library for Fourier Transform.

### gradle/
The configuration files for Android Studio

### app/src/main/
The source codes for main function of Android App 
#### app/src/main/assets: 
It contain the background music file

#### app/src/main/assets/java: 
Contain the Java codes for App. Under the folder "com\example\nativeaudio":
(1)  Constants.java: contains the input parameters setup, default parameters and coding loading preamble from disk to App
(2)  MyTask.java: contains the main function `def work()`, which is the start of the entire Java program and it calls the function `NativeAudio.calibrate()` from c++ files (`Java_com_example_nativeaudio_NativeAudio_calibrate()` in c++ files).
(3) NativeAudio.java: Enable the functionalities of text box, 

#### app/src/main/assets/res: 
Folder "layout" contains the UI configuration of the App; Folder "raw" contains the sending preamble files in .bin format.

#### app/src/main/assets/cpp: 
contain the c++ files for low-level audio programming and signal processing algorithm:



# How to use 
(1) Git clone this repo <br>
(2) Open Android Studio 2.2+ <br>
(3) Connect your smartphone to computer through USB <br>
(4) click "run" button <br>

The leader side UI setup up example:
<p align="center">
<img src="Leader_UI.png" width="700">
</p>

The user side UI setup up example:
<p align="center">
<img src="User_UI.png" width="700">
</p>
