#include "depthCamera.hpp"

//--------------------------------------------------------------
bool DepthCamera::setup(){
    // get the aspect ratio of the screen
    float yAspectRatio = float(ci::app::getWindowHeight())/float(ci::app::getWindowWidth());
    // get a scaled height with the same aspect ratio of the screen with a 640px width (astra is 640x480)
    mScaledYsize = yAspectRatio*640;
    
    // load saved settings
    loadSettings();
    
    // try to open the astra
    bool connected = initiateAstra();
    
    //GLSL
    //Optical Flow Shader
    ci::gl::GlslProg::Format hsflowFormat;
    hsflowFormat
    .vertex(ci::app::loadAsset("shaders/base_vert.glsl"))
    .fragment(ci::app::loadAsset("shaders/hsflow_frag.glsl"));
    mFlowShader=ci::gl::GlslProg::create(hsflowFormat);
    // Background Shader
    ci::gl::GlslProg::Format GLSLformat;
    GLSLformat
    .vertex(ci::app::loadAsset("shaders/testDepthVert.glsl"))
    .fragment(ci::app::loadAsset("shaders/testDepthFrag.glsl"));
    mBackgroundShader=ci::gl::GlslProg::create(GLSLformat);
    
    //FBOs
    ci::gl::Fbo::Format format;
    format.colorTexture( ci::gl::Texture::Format().wrapS(GL_CLAMP_TO_EDGE).wrapT(GL_CLAMP_TO_EDGE).minFilter(GL_NEAREST).magFilter(GL_NEAREST));
    format.disableDepth();
    mFlowFBO   = ci::gl::Fbo::create( ci::app::getWindowWidth(), ci::app::getWindowHeight(), format ) ;
    mLastFrameFBO = ci::gl::Fbo::create( ci::app::getWindowWidth(), ci::app::getWindowHeight(), format ) ;
    mCVresizeFbo = ci::gl::Fbo::create( ci::app::getWindowWidth(), ci::app::getWindowHeight(), format ) ;
    
    ci::gl::Fbo::Format pointcloudFormat;
    pointcloudFormat.colorTexture( ci::gl::Texture::Format().internalFormat( GL_RGB ) );
    mPointCloudFbo = ci::gl::Fbo::create( 640, 480, pointcloudFormat) ;
    
    {
        ci::gl::ScopedFramebuffer fbo(mFlowFBO);
        ci::gl::clear(ci::ColorA(0,0,0,1));
    }
    {
        ci::gl::ScopedFramebuffer fbo(mLastFrameFBO);
        ci::gl::clear(ci::ColorA(0,0,0,1));
    }
    {
        ci::gl::ScopedFramebuffer fbo(mCVresizeFbo);
        ci::gl::clear(ci::ColorA(0,0,0,1));
    }
    {
        ci::gl::ScopedFramebuffer fbo(mPointCloudFbo);
        ci::gl::clear(ci::ColorA(0,0,0,1));
    }
    
    // SETUP POINTCLOUD
    mPointcloud.assign( 320*240, DepthParticle() );
    
    // create all points
    for( auto p : mPointcloud ){
        p.pos = ci::vec3( 0 );
        p.threshold = ci::vec3(0);
    }
    
    // creat the VBO
    mPointcloudVbo = ci::gl::Vbo::create( GL_ARRAY_BUFFER, mPointcloud, GL_DYNAMIC_DRAW );
    // each point has a position and an individual unique depth threshold
    ci::geom::BufferLayout particleLayout;
    particleLayout.append( ci::geom::Attrib::POSITION, 3, sizeof( DepthParticle ), offsetof( DepthParticle, pos ) );
    particleLayout.append( ci::geom::Attrib::CUSTOM_0, 3, sizeof( DepthParticle ), offsetof( DepthParticle, threshold ));
    // create a VBO mesh
    auto mesh = ci::gl::VboMesh::create( mPointcloud.size(), GL_POINTS, { { particleLayout, mPointcloudVbo } } );
    
    // create Batch
    mPointcloudBatch = ci::gl::Batch::create( mesh, mBackgroundShader, { { ci::geom::CUSTOM_0, "thresholds" } });
    ci::gl::pointSize( 1.0f );
    
    return connected;
}

//--------------------------------------------------------------
void DepthCamera::update(){
    mAstra.setDepthClipping(0, mFarPlane);
    mAstra.update();
    
    // update the point cloud positions
    for(int y = 0; y < 480; y += 2) {
        for(int x = 0; x < 640; x += 2) {
            int i = y/2 * 320 + x/2;
            mPointcloud.at( i ).pos = mAstra.getWorldCoordinateAt(x, y);
        }
    }
    
    // Copy particle data onto the GPU.
    // Map the GPU memory and write over it.
    void *gpuMem = mPointcloudVbo->mapReplace();
    memcpy( gpuMem, mPointcloud.data(), mPointcloud.size() * sizeof(DepthParticle) );
    mPointcloudVbo->unmap();
    
    if (mLearnBrackground){
        learnBackground();
    }
    
    {
        //draw the point cloud into the FBO
        ci::gl::ScopedFramebuffer fbo(mPointCloudFbo);
        ci::gl::clear( ci::Color( 0, 0, 0 ) );
        ci::gl::ScopedMatrices scoped;
        ci::gl::ScopedViewport view(0,0,640,480);
        ci::gl::ScopedDepth depth(true);
        // do transformations on the point cloud with an orthographic view
        ci::mat4 projection = ci::gl::getProjectionMatrix();
        float orthoScale = -mZoom;
        float left = -320 * orthoScale;
        float top = 240 * orthoScale;
        float right = 320 * orthoScale;
        float bottom = -240 * orthoScale;
        projection = glm::ortho(left, right, bottom, top, mNearPlane, mFarPlane);
        ci::gl::setProjectionMatrix(projection);
        ci::gl::translate(mTranslateX, mTranslateY, mTranslateZ);
        ci::gl::rotate(rotate( ci::toRadians( mRotateX ), ci::vec3(1,0,0)));
        ci::gl::rotate(rotate( ci::toRadians( mRotateY ), ci::vec3(0,1,0)));
        ci::gl::rotate(rotate( ci::toRadians( mRotateZ ), ci::vec3(0,0,1)));
        
        // render the point cloud and subtract background with the shader
        {
            ci::gl::ScopedGlslProg shader(mBackgroundShader);
            mBackgroundShader->uniform("threshold", mBackgroundThreshold);
            mBackgroundShader->uniform("calibrated", mCalibrated);
            mPointcloudBatch->draw();
        }
    }
    
    // bring pointcloud image from GPU to CPU first
    ci::Surface mySurface =mPointCloudFbo->getColorTexture()->createSource();
    
    // then copy into OPENCV and convert to grayscale
    cv::Mat grayImage, input;
    input = cv::Mat( toOcv( mySurface));
    cv::cvtColor(input, grayImage, CV_BGR2GRAY);
    
    
    // for setting the homography
    if(mSettingNewPoints) {
        vector<cv::Point2f> sPoints, dPoints;
        for(int i = 0; i < mDestPoints.size(); i++) {
            sPoints.push_back(cv::Point2f(mSrcPoints[i].x , mSrcPoints[i].y));
            dPoints.push_back(cv::Point2f(mDestPoints[i].x, mDestPoints[i].y));
        }
        // generate a homography from the two sets of points
        mHomography = findHomography(cv::Mat(sPoints), cv::Mat(dPoints));
        mHomographyReady = true;
        cv::FileStorage fs;
    }
    
    // if the homography is ready do further CV
    if (mHomographyReady){
        cv::Mat warped, flipped, dilate, blurred, erode, thresholded;
        // warp the selected pointcloud image using the homography
        // set the new image to be the same aspect ration as the screen
        cv::warpPerspective(grayImage, warped, mHomography, ci::toOcv(ci::ivec2(640,mScaledYsize)));
        // image cleaning
        cv::blur(warped, blurred, cv::Size(mBlurAmount, mBlurAmount));
        cv::erode(blurred, erode, cv::Mat());
        cv::dilate(erode, dilate, cv::Mat());
        cv::threshold(dilate, thresholded, mThreshold, 255, CV_8U);
        // bring the finished image back from OPENCV
        mWarpedSurf = ci::Surface(ci::fromOcv(thresholded));
        
        //vector to store the found contours
        std::vector<std::vector<cv::Point> > contours;
        // find em
        cv::findContours(thresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        
        //reset average depth reading
        mAmount = 0;
        
        // find the average depth
        averageDepth(mWarpedSurf);
        
        // bring the finished CV surface to the GPU
        // create the texture if not already created
        if( ! mCvTexture) {
            auto format = ci::gl::Texture::Format().wrapS(GL_CLAMP_TO_EDGE).wrapT(GL_CLAMP_TO_EDGE).minFilter(GL_NEAREST).magFilter(GL_NEAREST);
            mCvTexture = ci::gl::Texture::create(mWarpedSurf);
        }
        else {
            mCvTexture->update( mWarpedSurf );
            // resize the texture to full screen size & save in FBO
            {
                ci::gl::ScopedFramebuffer fbo(mCVresizeFbo);
                ci::gl::ScopedViewport view(ci::app::getWindowSize());
                ci::gl::clear( ci::Color( 0, 0, 0 ) );
                ci::gl::draw(mCvTexture, ci::Rectf(0,0,ci::app::getWindowWidth(), ci::app::getWindowHeight()) );
            }
            // OPTICAL FLOW
            {
                ci::gl::ScopedBlendAlpha alpha;
                ci::gl::disableAlphaBlending();
                {
                    //do optical flow
                    ci::gl::ScopedFramebuffer fbo(mFlowFBO);
                    ci::gl::ScopedMatrices push;
                    ci::gl::ScopedViewport view(ci::app::getWindowSize());
                    ci::gl::clear();
                    ci::gl::ScopedTextureBind tex0(mCVresizeFbo->getColorTexture(), 0);
                    ci::gl::ScopedTextureBind tex1(mLastFrameFBO->getColorTexture(), 1);
                    ci::gl::ScopedGlslProg glsl(mFlowShader);
                    mFlowShader->uniform("tex0", 0);
                    mFlowShader->uniform("tex1", 1);
                    mFlowShader->uniform("scale", ci::vec2(100.f,100.f));
                    mFlowShader->uniform("offset", ci::vec2(5.f/ci::app::getWindowWidth(),5.f/ci::app::getWindowHeight()));
                    ci::gl::drawSolidRect(ci::Rectf(0, 0, ci::app::getWindowWidth(), ci::app::getWindowHeight()));
                }
                
                //save last frame
                {
                    ci::gl::ScopedFramebuffer fbo(mLastFrameFBO);
                    ci::gl::ScopedMatrices push;
                    ci::gl::ScopedViewport view(ci::app::getWindowSize());
                    ci::gl::clear();
                    ci::gl::draw(mFlowFBO->getColorTexture(),ci::Rectf(0, 0, ci::app::getWindowWidth(), ci::app::getWindowHeight()));
                }
                ci::gl::enableAlphaBlending();
            }
        }
    }
}

//--------------------------------------------------------------
void DepthCamera::draw(){
    // draw the webcam
    mAstra.draw(640, 480);
    
    // draw the point cloud
    ci::gl::draw(mPointCloudFbo->getColorTexture(), ci::Rectf(640, 0, 640+640, 0+ 480));
    
    if (mHomographyReady){
        {
            ci::gl::ScopedMatrices push;
            float yTrans = (480-mScaledYsize)*.5;
            ci::gl::translate(-1,yTrans, 0);
            
            // draw the treated pointcloud used for tracking
            ci::gl::draw(mCvTexture);
            
            // draw text
            ci::TextLayout text;
            text.setColor( ci::Color( 1.,0,0 ) );
            text.addLine( "The mapped image is scaled to the aspect ratio of the screen." );
            auto texture = ci::gl::Texture2d::create( text.render( true, false ) );
            float x = texture->getWidth();
            x = 640-x-2;
            ci::gl::draw(texture, ci::vec2(x, -20));
            
            //draw rectangel outline
            ci::gl::ScopedColor col;
            ci::gl::color(255, 0, 0);
            ci::gl::drawStrokedRect(ci::Rectf(0,0,640,mScaledYsize),2);
            
            // draw average depth center
            if (mAmount>0){
                ci::gl::drawSolidCircle(mAverageLoc, 20);
            }
        }
        // draw the optical flow
        ci::gl::ScopedBlendAlpha alpha;
        ci::gl::disableAlphaBlending();
        ci::gl::draw(mFlowFBO->getColorTexture(),ci::Rectf(0, mDepthHeight, mDepthWidth, mDepthHeight*2));
        ci::gl::enableAlphaBlending();
    }
    
    if (mDrawDepth)
        mAstra.drawDepth(0, 480);
    
    if (mSettingNewPoints){
        drawPoints(mSrcPoints);
    }
}

//--------------------------------------------------------------
void DepthCamera::learnBackground()
{
    // update the point cloud thresholds
    std::cout<<"learning......"<<std::endl;
    for(int y = 0; y < 480; y += 2) {
        for(int x = 0; x < 640; x += 2) {
            int i = y/2 * 320 + x/2;
            mPointcloud.at( i ).threshold = mAstra.getWorldCoordinateAt(x, y);
        }
    }
    
    // Copy particle data onto the GPU.
    // Map the GPU memory and write over it.
    void *gpuMem = mPointcloudVbo->mapReplace();
    memcpy( gpuMem, mPointcloud.data(), mPointcloud.size() * sizeof(DepthParticle) );
    mPointcloudVbo->unmap();
    
    mLearnBrackground = false;
    mCalibrated = true;
    
    std::cout<<"done!"<<std::endl;
}

//--------------------------------------------------------------
void DepthCamera::averageDepth(ci::Surface &s){
    // get average depth
    ci::vec2 pos = ci::vec2(0);
    ci::Surface::Iter iter = s.getIter( s.getBounds() );
    while( iter.line() ) {
        while( iter.pixel() ) {
            if(iter.r() > 0 && iter.r()<=255) {
                pos+=(ci::vec2(iter.x(), iter.y()));
                mAmount++;
                iter.r() = 255;
                iter.g() = 255;
                iter.b() = 255;
            }
            else {
                iter.r() = 0;
                iter.g() = 0;
                iter.b() = 0;
            }
        }
    }
    pos/=mAmount;
    mAverageLoc=pos;
}

//--------------------------------------------------------------
void DepthCamera::loadSettings(){
    // load the homography yaml file
    mSettingNewPoints = false;
    mYmlFilePath = ci::app::getAssetPath( "" ) / "homography.yml";
    if (ci::fs::exists(mYmlFilePath)){
        
        cv::FileStorage fs(mYmlFilePath.generic_string(), cv::FileStorage::READ);
        fs["homography"] >> mHomography;
        mHomographyReady = true;
        cout<<"yml file loaded"<<endl;
    }
    else {
        cout<<"no yml file"<<endl;
    }
    
    // load the other settings
    ci::XmlTree  doc;
    mXmlFilePath = ci::app::getAssetPath( "" ) / "settings.xml";
    
    // try to load the specified xml file
    try {
        doc = ci::XmlTree( ci::loadFile( mXmlFilePath  ));
        
        ci::XmlTree xml = doc.getChild( "config/warp" );
        
        mDepthWidth = xml.getAttributeValue<int>( "width", 640 );
        mDepthHeight = xml.getAttributeValue<int>( "height", 480 );
        mNearPlane = xml.getAttributeValue<float>( "mNearPlane", 0);
        mFarPlane = xml.getAttributeValue<float>( "mFarPlane", 12000);
        mBackgroundThreshold = xml.getAttributeValue<float>( "mBackgroundThreshold", .0056);
        mRotateX = xml.getAttributeValue<float>( "mRotateX", 180);
        mRotateY = xml.getAttributeValue<float>( "mRotateY", 0);
        mRotateZ = xml.getAttributeValue<float>( "mRotateZ", 0);
        mZoom = xml.getAttributeValue<float>( "mZoom", 1);
        mTranslateX = xml.getAttributeValue<float>( "mTranslateX", 0);
        mTranslateY = xml.getAttributeValue<float>( "mTranslateY", 0);
         mTranslateZ = xml.getAttributeValue<float>( "mTranslateZ", 0);
        mBlurAmount = xml.getAttributeValue<float>( "blurAmount", 50);
        mThreshold = xml.getAttributeValue<float>( "threshold", 15);
        
        //     load control points
        mSrcPoints.clear();
        for( auto child = xml.begin( "control-point" ); child != xml.end(); ++child ) {
            const auto x = child->getAttributeValue<float>( "x", 0.0f );
            const auto y = child->getAttributeValue<float>( "y", 0.0f );
            mSrcPoints.emplace_back( x, y );
        }
        cout<<"xml file loaded"<<endl;
    }
    catch( ... ) {
        // set some default source points if none loaded
        cout<<"no xml file"<<endl;
        mSrcPoints.push_back(ci::vec2(20,20));
        mSrcPoints.push_back(ci::vec2(mDepthWidth-20,20));
        mSrcPoints.push_back(ci::vec2(mDepthWidth-20,mDepthHeight-20));
        mSrcPoints.push_back(ci::vec2(20,mDepthHeight-20));
        mDepthWidth = 640;
        mDepthHeight = 480;
    }
    
    // destination points are where it should map to
    // scale to the screen's aspect ratio
    mDestPoints.push_back(ci::vec2(0,0));
    mDestPoints.push_back(ci::vec2(640,0));
    mDestPoints.push_back(ci::vec2(640,mScaledYsize));
    mDestPoints.push_back(ci::vec2(0,mScaledYsize));
}

//--------------------------------------------------------------
bool DepthCamera::initiateAstra(){
    // see if mAstra is connected
    bool connected = false;
    const char* cmd = "ioreg -p IOUSB -w0 | sed \'s/[^o]*o //; s/@.*$//\' | grep -v \'^Root.*\'";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    if(result.find("ORBBEC")!=std::string::npos){
        std::cout<<"mAstra is connected"<<std::endl;
        connected = true;
        mAstra.setup();
        mAstra.enableRegistration(false);
        mAstra.initColorStream();
        mAstra.initDepthStream();
        mAstra.initPointStream();
        
    } else {
        std::cout<<"mAstra is not connected"<<std::endl;
        connected = false;
    }
    return connected;
}

//--------------------------------------------------------------
void DepthCamera::drawPoints(vector<ci::vec2>& points) {
    // draw homography points and lines
    ci::gl::ScopedMatrices mat;
    ci::gl::translate(mDepthWidth, 0, 0);
    
    ci::gl::ScopedColor col;
    ci::gl::color(0,0,255);
    for(int i = 0; i < points.size(); i++) {
        ci::gl::drawStrokedCircle(points[i], 10);
        ci::gl::drawStrokedCircle(points[i], 1);
    }
    ci::gl::color(128,128,128);
    ci::gl::drawLine(mSrcPoints[mSrcPoints.size()-1], mSrcPoints[0]);
    for(int i = 1; i < mSrcPoints.size(); i++) {
        ci::gl::drawLine(mSrcPoints[i-1], mSrcPoints[i]);
    }
}

//--------------------------------------------------------------
void DepthCamera::saveSettings(){
    
    // create default <profile> (profiles are not yet supported)
    ci::XmlTree warp;
    
    warp.setTag( "warp" );
    warp.setAttribute( "width", mDepthWidth );
    warp.setAttribute( "height", mDepthHeight );
    warp.setAttribute( "mNearPlane", mNearPlane);
    warp.setAttribute( "mFarPlane", mFarPlane);
    warp.setAttribute( "mBackgroundThreshold", mBackgroundThreshold);
    warp.setAttribute( "mRotateX", mRotateX);
    warp.setAttribute( "mRotateY", mRotateY);
    warp.setAttribute( "mRotateZ", mRotateZ);
    warp.setAttribute( "mZoom", mZoom);
    warp.setAttribute( "mTranslateX", mTranslateX);
    warp.setAttribute( "mTranslateY", mTranslateY);
    warp.setAttribute( "mTranslateZ", mTranslateZ);
    warp.setAttribute( "blurAmount", mBlurAmount);
    warp.setAttribute( "threshold", mThreshold);
    std::vector<ci::vec2>::const_iterator itr;
    for( itr = mSrcPoints.begin(); itr != mSrcPoints.end(); ++itr ) {
        ci::XmlTree cp;
        cp.setTag( "control-point" );
        cp.setAttribute( "x", ( *itr ).x );
        cp.setAttribute( "y", ( *itr ).y );
        
        warp.push_back( cp );
    }
    
    // create config document and root <config>
    ci::XmlTree doc;
    doc.setTag( "config" );
    doc.setAttribute( "version", "1.1" );
    doc.setAttribute( "profile", "default" );
    
    // add profile to root
    doc.push_back( warp );
    
    // write file
    doc.write( ci::writeFile( mXmlFilePath ));
}

//--------------------------------------------------------------
bool DepthCamera::movePoint(vector<ci::vec2>& points, ci::vec2 point) {
    // if mouse is close to point move point
    for(int i = 0; i < points.size(); i++) {
        ci::vec2 offset(mDepthWidth,0);
        if(glm::distance( points[i], point-offset) < 20) {
            mMovingPoint = true;
            mCurPoint = &points[i];
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------
void DepthCamera::mouseDown( ci::app::MouseEvent event )
{
    ci::vec2 cur(event.getX(), event.getY());
    movePoint(mSrcPoints, cur);
}

//--------------------------------------------------------------
void DepthCamera::mouseDrag( ci::app::MouseEvent event )
{
    if(mMovingPoint) {
        mCurPoint->x=event.getX()-mDepthWidth;
        mCurPoint->y=event.getY();
    }
}

//--------------------------------------------------------------
void DepthCamera::mouseUp( ci::app::MouseEvent event )
{
    mMovingPoint = false;
}

//--------------------------------------------------------------
void DepthCamera::setNewPoints(){
    if (mSettingNewPoints){
        cv::FileStorage fs(mYmlFilePath.generic_string(), cv::FileStorage::WRITE);
        fs << "homography" << mHomography;
        saveSettings();
    }
    mSettingNewPoints = !mSettingNewPoints;
}
