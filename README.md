Manus - your personal robotics laboratory
=========================================

Manus is a manipulation robotics framework for educational purposes. It was written as a support software for a course on robotics and computer vision.

The project is still work-in-progress and is evolving. Please report any bugs or feature requests.

Building
--------

The project is built using CMake. Before that a few dependencies have to be installed:

 * [OpenCV](http://opencv.org/) - Computer vision library, packages available in Ubuntu repositories
 * [EchoLib](https://github.com/vicoslab/echolib) - a light-weight IPC library
 * [EchoCV](https://github.com/vicoslab/echocv) - support for OpenCV structures with EchoLib
 * [EchoMsg](https://github.com/vicoslab/echomsg) - support for custom messages with EchoLib
 * [Orocos KDL](http://www.orocos.org/) - Kinematics library, packages available in Ubuntu repositories

If you want to get it running on Ubuntu systems we maintain a [PPA repository](https://launchpad.net/~vicoslab/+archive/ubuntu/manus) with all the required packages already compiled.




