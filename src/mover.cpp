#include "mover.hpp"

Mover::Mover(){}

Mover::Mover(float m, float x, float y){
    mMass = m;
    mPosition = ci::vec3(x,y,0);
    mVelocity = ci::vec3(0,0,0);
    mAcceleration = ci::vec3(0,0,0);
}

void Mover::applyForce(ci::vec3 &force){
    mAcceleration += force/mMass;
}

void Mover::update() {
    mVelocity+=mAcceleration;
    mPosition+=mVelocity;
    mVelocity*=.9;
    mAcceleration*=0;
}
