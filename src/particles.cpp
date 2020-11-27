#include "particles.hpp"

//--------------------------------------------------------------
void Particles::initParticles(){
    mParticleForce = 5;
    
    std::vector<ci::vec2> points;
   
    // create random points on screen
    int howmany = 5000;
    for (int i=0; i<howmany; i++){
        auto loc = ci::vec2(ci::Rand::randFloat(ci::app::getWindowWidth()), ci::Rand::randFloat(ci::app::getWindowHeight()));
        points.push_back(loc);
    }

    mNumParticles=points.size();

    // Create initial particle layout.
    std::vector<Particle> particles;
    particles.assign( mNumParticles, Particle() );
 
    for( int i = 0; i < particles.size(); ++i )
    {	// assign starting values to particles.
        auto &p = particles.at( i );
        p.loc = ci::vec3(points[i].x,points[i].y,0);
        p.vel = ci::vec3(0.);
    }
    
    // Create particle buffers on GPU and copy data into the first buffer.
    // Mark as static since we only write from the CPU once.
    mParticleBuffer[mSourceIndex] = ci::gl::Vbo::create( GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW );
    mParticleBuffer[mDestinationIndex] = ci::gl::Vbo::create( GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), nullptr, GL_STATIC_DRAW );
    
    for( int i = 0; i < 2; ++i )
    {	// Describe the particle layout for OpenGL.
        mAttributes[i] = ci::gl::Vao::create();
        ci::gl::ScopedVao vao( mAttributes[i] );
        
        // Define attributes as offsets into the bound particle buffer
        ci::gl::ScopedBuffer buffer( mParticleBuffer[i] );
        ci::gl::enableVertexAttribArray( 0 );
        ci::gl::enableVertexAttribArray( 1 );
        
        ci::gl::vertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, loc) );
        ci::gl::vertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, vel) );
    }
    
}

//--------------------------------------------------------------
void Particles::setup()
{
    // GLSL
    
    // Load our update program.
    // Match up our attribute locations with the description we gave.
    mUpdateProg = ci::gl::GlslProg::create( ci::gl::GlslProg::Format().vertex( ci::app::loadAsset( "shaders/particleUpdate.vs" ) )
                                       .feedbackFormat( GL_INTERLEAVED_ATTRIBS )
                                       .feedbackVaryings( { "loc", "vel" } )
                                       .attribLocation( "iLoc", 0 )
                                       .attribLocation( "iVel", 1)
                                       );
    // Load the render program
    ci::gl::GlslProg::Format renderFormat;
    renderFormat
    .vertex(ci::app::loadAsset("shaders/render.vert"))
    .fragment(ci::app::loadAsset("shaders/render.frag"));
    mRenderProg=ci::gl::GlslProg::create(renderFormat);
    
    // render FBO
    cinder::gl::Fbo::Format format;
    format.setSamples( 16 );
    
    mRenderPass = ci::gl::Fbo::create( ci::app::getWindowWidth(), ci::app::getWindowHeight(),format ) ;
    
    {
        ci::gl::ScopedMatrices push;
        ci::gl::setMatricesWindow(ci::app::getWindowSize());
        ci::gl::viewport(0, 0, ci::app::getWindowWidth(), ci::app::getWindowHeight());
        {
            ci::gl::ScopedFramebuffer fbo(mRenderPass);
            ci::gl::clear(ci::ColorA(0,0,0,1));
        }
    }

    // load particle texture
    mParticleImage = ci::gl::Texture::create( loadImage( ci::app::loadAsset("images/BubbleSimple.png") ) ) ;
    
    //initialize the particles
    initParticles();
}

//--------------------------------------------------------------
void Particles::update(ci::vec3 trackedPos)
{
    // Update particles on the GPU
    {
        ci::gl::ScopedGlslProg prog( mUpdateProg );
        ci::gl::ScopedState rasterizer( GL_RASTERIZER_DISCARD, true );	// turn off fragment stage
        mUpdateProg->uniform("useTexture", false);

        if (mUseTexture){
            mOpticalFlowTex->bind(0);
            mUpdateProg->uniform("tex0", 0);
            mUpdateProg->uniform("useTexture", true);
        }
        mUpdateProg->uniform( "uMousePos", trackedPos );
        mUpdateProg->uniform( "uPreMousePos", mPreTrackedPos );
        mUpdateProg->uniform( "wh", ci::vec2(ci::app::getWindowWidth(),ci::app::getWindowHeight()));
        mUpdateProg->uniform( "time", (float)ci::app::getElapsedSeconds());
        mUpdateProg->uniform( "scale", mParticleForce);

        // Bind the source data (Attributes refer to specific buffers).
        ci::gl::ScopedVao source( mAttributes[mSourceIndex] );
        // Bind destination as buffer base.
        ci::gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mParticleBuffer[mDestinationIndex] );
        ci::gl::beginTransformFeedback( GL_POINTS);
        
        // Draw source into destination, performing our vertex transformations.
        ci::gl::drawArrays( GL_POINTS, 0, mNumParticles );
        
        ci::gl::endTransformFeedback();
        
        if (mUseTexture)
            mOpticalFlowTex->unbind();

        // Swap source and destination for next loop
        std::swap( mSourceIndex, mDestinationIndex );
    }
    mPreTrackedPos=trackedPos;
}

//--------------------------------------------------------------
void Particles::draw()
{
    ci::gl::clear( ci::Color( 0, 0, 0 ) );
    //render pass
    {
        ci::gl::ScopedFramebuffer fbo(mRenderPass);
        ci::gl::ScopedMatrices push;
        ci::gl::setMatricesWindow(ci::app::getWindowSize());
        ci::gl::ScopedViewport view(0, 0, ci::app::getWindowWidth(), ci::app::getWindowHeight());
        ci::gl::ScopedState stateScope( GL_PROGRAM_POINT_SIZE, true );
        ci::gl::ScopedBlendAlpha alpha;
        ci::gl::ScopedGlslProg render( mRenderProg );
        ci::gl::ScopedTextureBind texScope( mParticleImage );
        mRenderProg->uniform("ParticleTex", 0 );
        mRenderProg->uniform( "resolution", ci::vec2(ci::app::getWindowWidth(),ci::app::getWindowHeight()));
        mRenderProg->uniform( "time", (float)ci::app::getElapsedSeconds());
        mRenderProg->uniform( "mouse", ci::vec2(mPreTrackedPos.x, mPreTrackedPos.y) );
        mRenderProg->uniform( "useMouse", !mUseTexture );

        ci::gl::ScopedVao vao( mAttributes[mSourceIndex] );
        ci::gl::context()->setDefaultShaderVars();
        ci::gl::clear( ci::Color( 0,0,0) );
        ci::gl::drawArrays( GL_POINTS, 0, mNumParticles );
    }
    
    ci::gl::draw(mRenderPass->getColorTexture());

}

//--------------------------------------------------------------
void Particles::fromOpticalFlow(ci::gl::TextureRef _opticalFlowTex){
    mOpticalFlowTex = _opticalFlowTex;
}
