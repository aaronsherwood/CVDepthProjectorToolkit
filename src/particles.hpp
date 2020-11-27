#ifndef Particles_hpp
#define Particles_hpp

#include <stdio.h>
#include "cinder/Rand.h"
#include "mover.hpp"

#endif /* Particles_hpp */

struct Particle
{
    ci::vec3 loc;
    ci::vec3 vel;
};

class Particles{
public:
    void setup();
    void update(ci::vec3 trackedPos);
    void draw();
    void initParticles();
    void fromOpticalFlow(ci::gl::TextureRef _opticalFlowTex);
    bool mUseTexture = false;
    ci::gl::TextureRef mParticleImage;
    float mParticleForce;
    
private:
    ci::gl::GlslProgRef mRenderProg;
    ci::gl::GlslProgRef mArrivedProg;
    ci::gl::GlslProgRef mUpdateProg;
    ci::gl::GlslProgRef mBlurProg;
    ci::gl::GlslProgRef mDiscardProg;
    
    // Descriptions of particle data layout.
    ci::gl::VaoRef		mAttributes[2];
    // Buffers holding raw particle data on GPU.
    ci::gl::VboRef		mParticleBuffer[2];
    // TBO
    ci::gl::BufferTextureRef mBufTex[2];
    
    // Current source and destination buffers for transform feedback.
    // Source and destination are swapped each frame after update.
    std::uint32_t	mSourceIndex		= 0;
    std::uint32_t	mDestinationIndex	= 1;
    ci::vec3        mPreTrackedPos = ci::vec3( 0, 0, 0 );
    ci::vec3        mTrackedPos = ci::vec3( 0, 0, 0 );
    
    //particles
    int mNumParticles;
    
    //FBO
    ci::gl::FboRef mRenderPass;
    // optical flow texture
    ci::gl::TextureRef mOpticalFlowTex;
};


