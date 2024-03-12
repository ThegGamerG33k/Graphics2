#pragma once

#include "math2801.h"
using namespace math2801;

class Light{
  public:

    std::string name;
    vec3 position;
    bool positional;
    vec3 direction;
    float cosInnerSpotAngle;
    float cosOuterSpotAngle;
    vec3 color;
    float intensity;

    Light(std::string name_, vec3 position_, bool positional_,
            vec3 direction_, float cosInnerSpotAngle_, float cosOuterSpotAngle_,
            vec3 color_, float intensity_){
        this->name=name_;
        this->position = position_;
        this->positional = positional_;
        this->direction = direction_;
        this->cosInnerSpotAngle = cosInnerSpotAngle_;
        this->cosOuterSpotAngle = cosOuterSpotAngle_;
        this->color = color_;
        this->intensity = intensity_;
    }
};

