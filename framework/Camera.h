#pragma once

#include "math2801.h"
using namespace math2801;

class Uniforms;


class Camera{
  public:
    float fov_h;
    float fov_v;
    float hither;
    float yon;
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 viewProjMatrix;
    vec3 eye;
    vec3 right;
    vec3 up;
    vec3 look;

    Camera(vec3 eye, vec3 coi, vec3 up, float fov=35.0f, float aspectRatio=1.0f, float hither=0.01f, float yon=100.0f);
    void updateProjMatrix();
    void lookAt(vec3 eye, vec3 coi, vec3 up);
    void updateViewMatrix();
    void setUniforms(Uniforms* uniforms);
    void strafe(float deltaRight, float deltaUp, float deltaLook);
    void turn(float angle);
    void roll(float angle);
    void tilt(float angle);
    void strafeNoUpDown(float dr, float du, float dl);
    void strafeUp(float d);
};
