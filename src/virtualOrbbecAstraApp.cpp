#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "particles.hpp"
#include "depthCamera.hpp"
#include "cinder/CinderMath.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"
#include "Warp.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace ph::warping;

void prepareSettings( App::Settings *settings )
{
    settings->setHighDensityDisplayEnabled();
}

class virtualOrbbecAstraApp : public App {
public:
    virtualOrbbecAstraApp();
    void setup() override;
    void mouseMove( MouseEvent event ) override;
    void mouseDown( MouseEvent event ) override;
    void mouseDrag( MouseEvent event ) override;
    void mouseUp( MouseEvent event ) override;
    void update() override;
    void draw() override;
    void cleanup() override;
    void setupParams();
    void setupWarps();
    void editProjectionMapping();
    void learnBackground();
    
    vec3 mMousePos;
    Particles mParticles;
    params::InterfaceGlRef mParams;
    
    // camera
    DepthCamera mDepthCamera;
    bool mDrawDebug = false;
    bool mUseCamera = false;
    vec3 mTrackedLoc;
    bool mSetHomography = false;
    
    // projection mapping
    bool mUseBeginEnd;
    fs::path mSettings;
    WarpList mWarps;
    bool mDoWarp;
    bool mEditWarp;
};

virtualOrbbecAstraApp::virtualOrbbecAstraApp(){}

void virtualOrbbecAstraApp::setup()
{
    setFullScreen(true);
    
    mTrackedLoc=vec3(getWindowWidth(),getWindowHeight(),0.);
    mUseCamera = mDepthCamera.setup();
    
    setupParams();
    
    mParticles.setup();
    
    if (mUseCamera){
        mParticles.mUseTexture = true;
        mDrawDebug = true;
    }else {
        mParams->minimize();
    }
    
    setupWarps();
}

void virtualOrbbecAstraApp::mouseDown( MouseEvent event )
{
    if( !Warp::handleMouseDown( mWarps, event ) ) {
        mDepthCamera.mouseDown(event);
    }
}
void virtualOrbbecAstraApp::mouseUp( MouseEvent event )
{
    if( !Warp::handleMouseUp( mWarps, event ) ) {
        mDepthCamera.mouseUp(event);
    }
}

void virtualOrbbecAstraApp::mouseDrag( MouseEvent event )
{
    if( !Warp::handleMouseDrag( mWarps, event ) ) {
        mDepthCamera.mouseDrag(event);
    }
}

void virtualOrbbecAstraApp::mouseMove( MouseEvent event )
{
    if( !Warp::handleMouseMove( mWarps, event ) ) {
        mMousePos = vec3( event.getX(), event.getY(), 0.0f );
    }
}

void virtualOrbbecAstraApp::update()
{
    if (mUseCamera){
        mDepthCamera.update();
        // send the optical flow texture to the particle system
        mParticles.fromOpticalFlow(mDepthCamera.mFlowFBO->getColorTexture());
        mTrackedLoc.x = (
                        lmap(mDepthCamera.mAverageLoc.x, 0.f, 640.f, 0.f, float(getWindowWidth())));
        mTrackedLoc.y = (
                        lmap(mDepthCamera.mAverageLoc.y, 0.f, float(mDepthCamera.mScaledYsize), 0.f, float(getWindowHeight())));
    } else {
        mTrackedLoc.x = mMousePos.x;
        mTrackedLoc.y = mMousePos.y;
    }
    
    mParticles.update(mTrackedLoc);
}

void virtualOrbbecAstraApp::draw()
{
    if (mDrawDebug){
        gl::clear(Color::white());
        mDepthCamera.draw();
    } else {
        gl::clear();
        gl::color( Color::white() );
        // if projection mapping
        if( mDoWarp ) {
            // iterate over the warps and draw their content
            for( auto &warp : mWarps ) {
                warp->begin();
                gl::clear();
                mParticles.draw();
                warp->end();
            }
        } else // other wise draw without projection mapping
            mParticles.draw();
    }
    mParams->draw();
}

void virtualOrbbecAstraApp::editProjectionMapping()
{
    mDrawDebug = false;
    mDoWarp = true;
    Warp::enableEditMode( !Warp::isEditModeEnabled() );
}

void virtualOrbbecAstraApp::setupParams()
{
    // Setup the gui controls
    mParams = params::InterfaceGl::create( getWindow(), "App parameters", toPixels( ivec2( 310, 600 ) ) );
    mParams->addText( "text0", "label=`Projection mapping`" );
    mParams->addParam("Toggle Projection Mapping (p)", &mDoWarp).keyIncr("p");
    mParams->addParam( "Edit Projection Mapping (e)", &mEditWarp ).updateFn( [this] {editProjectionMapping();} ).keyIncr("e");
    mParams->addSeparator();
    mParams->addText( "text1", "label=`Show the camera views or visuals`" );
    mParams->addParam( "Draw Debug (d)", &mDrawDebug ).keyIncr("d");
    mParams->addSeparator();
    mParams->addText( "tex2", "label=`Rotate and position the point cloud to be`" );
    mParams->addText( "text3", "label=`at the desired viewing angle and size`" );
    mParams->addParam( "Near Plane", &mDepthCamera.mNearPlane ).step( 100.f ).min( 0.1f );
    mParams->addParam( "Far Plane", &mDepthCamera.mFarPlane ).step( 100.f ).min( 0.1f );
    mParams->addParam( "Rotate X", &mDepthCamera.mRotateX ).step( 1.f);
    mParams->addParam( "Rotate Y", &mDepthCamera.mRotateY ).step( 1.f);
    mParams->addParam( "Rotate Z", &mDepthCamera.mRotateZ ).step( 1.f );
    mParams->addParam( "Translate X", &mDepthCamera.mTranslateX ).step( 1.f);
    mParams->addParam( "Translate Y", &mDepthCamera.mTranslateY ).step( 1.f);
    mParams->addParam( "Translate Z", &mDepthCamera.mTranslateZ ).step( 1.f);
    mParams->addParam( "Zoom", &mDepthCamera.mZoom ).step( 1.f );
    mParams->addSeparator();
    mParams->addText( "text4", "label=`Select the area of the point cloud that`" );
    mParams->addText( "text5", "label=`aligns with the projection`" );
    mParams->addParam( "Edit Homography (h)", &mSetHomography ).updateFn( [this] {mDepthCamera.setNewPoints(); mDrawDebug = true; Warp::enableEditMode( false );} ).keyIncr("h");
    mParams->addSeparator();
    mParams->addText( "text6", "label=`Blur and threshold the image`" );
    mParams->addParam( "Blur Amount", &mDepthCamera.mBlurAmount).step( 1.f );
    mParams->addParam( "Threshold", &mDepthCamera.mThreshold );
    mParams->addSeparator();
    mParams->addText( "text7", "label=`Learn and adjust the background subtraction`" );
    mParams->addButton( "Learn Background (b)", bind( &virtualOrbbecAstraApp::learnBackground, this ), "keyIncr=`b`" );
    mParams->addParam( "Background Threshold", &mDepthCamera.mBackgroundThreshold ).step( 0.0001f );
    mParams->addSeparator();
    mParams->addText( "tex8", "label=`Draw the org. depth image`" );
    mParams->addParam( "Show Depth Image", &mDepthCamera.mDrawDepth );
    mParams->addSeparator();
    mParams->addText( "tex9", "label=`Visuals`" );
    mParams->addParam( "Particle Force", &mParticles.mParticleForce ).step( 1.f);
    mParams->addSeparator();
}

void virtualOrbbecAstraApp::setupWarps()
{
    // projection mapping setup
    mUseBeginEnd = false;
    
    // initialize warps
    mSettings = getAssetPath( "" ) / "warps.xml";
    if( fs::exists( mSettings ) ) {
        // load warp settings from file if one exists
        mWarps = Warp::readSettings( loadFile( mSettings ) );
    }
    else {
        // otherwise create a warp from scratch
        mWarps.push_back( WarpPerspective::create() );
    }
    
    // adjust the content size of the warps
    Warp::setSize( mWarps, ci::app::getWindowSize() );
    mDoWarp = false;
    mEditWarp = false;
}

void virtualOrbbecAstraApp::learnBackground()
{
    mDepthCamera.mLearnBrackground = true;
}

void virtualOrbbecAstraApp::cleanup()
{
    // save warp & param settings when the app closes
    Warp::writeSettings( mWarps, writeFile( mSettings ) );
    mDepthCamera.saveSettings();
}



CINDER_APP( virtualOrbbecAstraApp, RendererGl( RendererGl::Options().msaa( 16 ) ), prepareSettings )
