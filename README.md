# CVDepthProjectorToolkit

This project pulls together many helpful tools for tracking people with a depth camera, projecting on the ground (or wall), and aligning the visuals with where people are. It was originally created and used for [MICHI](http://aaron-sherwood.com/works/michi/). 

![](https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/michi.jpg)

## Quick Functionality Overview
* Cinder
* Orbbec Astra depth cam
* Virtual placement of depth cam in room via pointcloud rotation
* Unique, per point, thresholds to allow for dynamic background subtraction
* Homography via OPENCV to match a section of the point cloud to the projected area
* GPU calculated optical flow
* Transform feedback particles with optical flow
* Projection mapping

### More Info

A pointcloud from an Orbbec Astra is rotated to get the viewing angle needed to track, allowing the camera to be placed anywhere in a room (inspired by Kyle McDonald's [ofxVirtualkinect](https://github.com/kylemcdonald/ofxVirtualKinect)). A novel approach of getting unique thresholds for every pointcloud point allows a dynamic background subtraction. [Homography] (https://docs.opencv.org/master/d9/dab/tutorial_homography.html) is used to map the area of the pointcloud you want to track onto a scaled rectangle with the same aspect ratio of the screen using OPENCV. Average depth is calculated as well as GPU calculated optical flow. The latter allows for optical flow use per pixel with transform feedback particle systems. In order to have flexibilty with where the projector goes as well, projection mapping is included via Paul Houx's [Cinder-Warping](https://github.com/paulhoux/Cinder-Warping) block. Orbbec Astra library is based on [ofxOrbbecAstra](https://github.com/mattfelsen/ofxOrbbecAstra). Opticalflow implementation is from somewher (I can't seem to find it now) on Andrew Benson's [website](https://pixlpa.com/). OSC is also included, but not used in the current example.

## Setup

Clone this addon into your `cinder/samples` folder.

```
cd PATH/TO/cinder_0.9.2_mac/samples/
git clone https://github.com/aaronsherwood/CVDepthProjectorToolkit.git
```

The Astra SDK is bundled in the repo and does not require a separate download or installation.

## Running the example project

Hopefully this should compile and run without any hiccups. Please open an issue if you run into trouble here.

## Using this in your project

If you're creating a new project, the easiest approach is to copy the example project, which is already set up with the correct header & linker paths. Otherwise you will need to set the following:

- add to **Header Search Paths**:
  - `../src/orbbecAstra/libs/astra/include`
- add to **Other Linker Flags**:
  - `../src/orbbecAstra/libs/astra/lib/osx/libastra.dylib`
  - `../src/orbbecAstra/libs/astra/lib/osx/libastra_core.dylib`
  - `../src/orbbecAstra/libs/astra/lib/osx/libastra_core_api.dylib`
- add to **Runpath Search Paths**:
  - `@executable_path/../../../../../../src/orbbecAstra/libs/astra/lib/osx/`

Note: this assumes your project folder is inside your samples folder, e.g. `cinder_0.9.2_mac/samples/CVDepthProjectorToolkit/`. If not, you may need to adjust the runpath setting.

## Dependencies
These are all included, but if making a new project you will need the following:
* [Cinder-OPENCV](https://github.com/cinder/Cinder-OpenCV)
* [Cinder-Warping](https://github.com/paulhoux/Cinder-Warping)
* OSC cinder block (not used currently)

## Support

This has been tested with the following setup:

- Xcode 11.3.1
- OS X 10.15.5
- cinder 0.9.2
- Astra SDK 0.5.0 (which is included, no separate download/installation is required)
- Orbbec Astra camera

## How To
The app will detect if you have the depth camera plugged in or not. If not, it will let you play with the visuals using the mouse. If it detects an Orbbec camera you will see something similar to the following. Counter clock wise from the bottom right we have the webcam image, the pointcloud image, the cleaned/treated image from the pointcloud, and the opticalflow texture from the GPU.

![](https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/overview.png)

1. First (if needing to projection map), edit the projection mapping points to match the area you want to track within.

![](https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/map.gif)
<br/>
<br/>

2. Next, using the params GUI menu, rotate and position the point cloud to get the desired viewing angle.

![](https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/rotate.gif)
<br/>
<br/>

3. Then set the homography rectangle to match your projection space in the pointcloud view. This saure is mapped, with the screen's aspect ratio, to the sqaure to the right, with blurring and thresholding, etc. done with OPENCV.

<img src="https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/setmap.gif" data-canonical-src="https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/setmap.gif" width="800" />
<br/>
<br/>

4. Click *Learn Background* in the GUI memory to analyze the depth and set per point depth thresholds.

<img src="https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/nobackground.gif" data-canonical-src="https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/nobackground.gif" width="800" />
<br/>
<br/>

5. The optical flow is shown in the bottom left.

![](https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/flow.gif)
<br/>
<br/>

6. Turn off debug mode and the optical flow will move the particles around.

![](https://github.com/aaronsherwood/CVDepthProjectorToolkit/blob/main/README_Images/bubbles.gif)
<br/>
<br/>
