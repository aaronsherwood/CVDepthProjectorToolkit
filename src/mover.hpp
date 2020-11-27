#ifndef mover_hpp
#define mover_hpp

#include <stdio.h>

class Mover{
public:
    ci::vec3 mPosition;
    ci::vec3 mVelocity;
    ci::vec3 mAcceleration;
    float mMass;
    Mover();
    Mover(float m, float x, float y);
    void applyForce(ci::vec3 &force);
    void update();
};

#endif /* mover_hpp */
