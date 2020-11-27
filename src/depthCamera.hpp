#ifndef depthCamera_hpp
#define depthCamera_hpp

#include <stdio.h>
#include "CinderOpenCv.h"
#include "cinder/Xml.h"
#include "orbbecAstra.h"

using namespace std;

struct DepthParticle
{
    ci::vec3 pos;
    ci::vec3 threshold;
};

class DepthCamera {
public:
    bool setup();
    void update();
    void draw();
    void mouseDown( ci::app::MouseEvent event );
    void mouseDrag( ci::app::MouseEvent event );
    void mouseUp( ci::app::MouseEvent event );
    void setNewPoints();
    void saveSettings();
    
    orbbecAstra mAstra;
    
    //ANALYSIS
    ci::gl::FboRef mFlowFBO;
    int mThreshold;
    int mDepthWidth, mDepthHeight;
    ci::Surface mWarpedSurf;
    int mScaledYsize;
    ci::vec2 mAverageLoc;
    
    //CAMERA
    float mNearPlane, mFarPlane;
    float mBackgroundThreshold;
    float mRotateX, mRotateY, mRotateZ, mZoom, mTranslateX, mTranslateY, mTranslateZ, mBlurAmount;
    bool mLearnBrackground = false;
    bool mDrawDepth = false;
    
private:
    void drawPoints(vector<ci::vec2>& points);
    bool movePoint(vector<ci::vec2>& points, ci::vec2 point);
    void averageDepth(ci::Surface &s);
    void loadSettings();
    void learnBackground();
    bool initiateAstra();
    bool mSettingNewPoints;
    
    // for homography
    vector<ci::vec2> mDestPoints, mSrcPoints;
    cv::Mat mHomography;
    bool mMovingPoint;
    ci::vec2* mCurPoint;
    bool mHomographyReady=false;
    
    // settings for homography and camera
    ci::fs::path mXmlFilePath, mYmlFilePath;
    
    int mAmount = 0;
    bool mCalibrated = false;
    
    //OPENGL stuff
    // texture to store the warped image
    ci::gl::TextureRef mCvTexture;
    // SHADERS
    ci::gl::GlslProgRef mBackgroundShader, mFlowShader;
    // FBOs
    ci::gl::FboRef mLastFrameFBO, mCVresizeFbo, mPointCloudFbo;
    // Particle data on CPU
    vector<DepthParticle> mPointcloud;
    // Buffer holding raw particle data on GPU, written to every update()
    ci::gl::VboRef mPointcloudVbo;
    // Batch for rendering Pointcloud
    ci::gl::BatchRef mPointcloudBatch;
};

#endif /* depthCamera_hpp */
