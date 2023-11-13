UnderwaterGPS APP
============
UnderwaterGPS APP is an Android application that plays and records sounds with the C++ OpenSLES API using JNI. This app is the implementation of our distributed underwater localization protocol. Currently, this App can perform our distributed localization protocol with the preamble exchanging among multiple devices and save the essential raw data to the disk. The localization algorithm part is deployed in the folder "Offline_Process".

# Pre-requisites

- Android Studio 2.2+ with [NDK](https://developer.android.com/ndk/) bundle.

# How to use 
(1) Git clone this repo <br>
(2) Open Android Studio 2.2+
(3) Connect your smartphone to computer through USB
(4) click "run" button

The leader side UI setup up example:
<p align="center">
<img src="Leader_UI.png" width="700">
</p>

The user side UI setup up example:
<p align="center">
<img src="User_UI.png" width="700">
</p>
