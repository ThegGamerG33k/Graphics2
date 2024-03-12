#include "Camera.h"
#include "Uniforms.h"
#include "math2801.h"
#include <cmath>
Camera::Camera(vec3 eye_, vec3 coi_, vec3 up_, float fov, float aspectRatio, float hither_, float yon_)
{
    this->fov_h = fov;
    this->fov_v = aspectRatio*this->fov_h;
    this->hither=hither_;
    this->yon=yon_;
    this->viewMatrix = mat4::identity();
    this->projMatrix = mat4::identity();
    this->updateProjMatrix();
    this->lookAt(eye_,coi_,up_);
}

void Camera::updateProjMatrix()
{

    float d_h = float(1.0/std::tan(this->fov_h));
    float d_v = float(1.0/std::tan(this->fov_v));

    float f=this->yon;
    float n=this->hither;

    this->projMatrix = mat4(
        d_h  , 0    , 0 , 0  ,
        0    , -d_v , 0 , 0  ,
        0    , 0    , f/(n-f) , -1 ,
        0    , 0    , (n*f)/(n-f) , 0  );
    this->viewProjMatrix=this->viewMatrix*this->projMatrix;
}

void Camera::lookAt(vec3 eye_, vec3 coi_, vec3 up_)
{
    this->eye = eye_;
    this->look = normalize(coi_-this->eye);
    this->right = normalize(cross(this->look,up_));
    this->up = normalize(cross(this->right,this->look));
    this->updateViewMatrix();
}

void Camera::updateViewMatrix()
{
    this->viewMatrix = mat4(
        this->right.x, this->up.x, -this->look.x, 0,
        this->right.y, this->up.y, -this->look.y, 0,
        this->right.z, this->up.z, -this->look.z, 0,
        -dot(this->eye,this->right), -dot(this->eye,this->up), dot(this->eye,this->look), 1.0f
    );

    this->viewProjMatrix=this->viewMatrix*this->projMatrix;
}

void Camera::setUniforms(Uniforms* uniforms)
{
    uniforms->set( "projMatrix",this->projMatrix);
    uniforms->set( "viewMatrix", this->viewMatrix );
    uniforms->set( "viewProjMatrix",this->viewProjMatrix);
    uniforms->set( "eyePosition", this->eye);
}

void Camera::strafe( float deltaRight, float deltaUp, float deltaLook)
{
    this->eye = this->eye + deltaRight*this->right+deltaUp*this->up+deltaLook*this->look;
    this->updateViewMatrix();
}

void Camera::turn( float angle)
{
    mat3 M = axisRotation3x3( vec3(0,1,0), angle );
    this->right = this->right*M;
    this->up = this->up*M;
    this->look = this->look*M;
    this->updateViewMatrix();
}

void Camera::roll(float angle)
{
    mat3 M = axisRotation3x3( this->look, angle );
    this->right = this->right*M;
    this->up = this->up*M;
    this->updateViewMatrix();
}

void Camera::tilt(float angle)
{
    mat3 M = axisRotation3x3( this->right, angle );
    this->up = this->up*M;
    this->look = this->look*M;
    this->updateViewMatrix();
}

void Camera::strafeNoUpDown(float dr,float du,float dl)
{

    //FIXME: If up is pointing straight up, this gives
    //singularity.
    vec3 r = normalize(vec3(this->right.x,0,this->right.z)  );
    if( std::isinf(r.x) ){
        //division by zero
        r = this->right;
    }

    vec3 u = normalize(vec3(this->up.x,0,this->up.z)        );
    if( std::isinf(u.x) ){
        //division by zero
        u = this->up;
    }

    vec3 l = normalize(vec3(this->look.x,0,this->look.z)    );
    if( std::isinf(l.x) )
        l = this->look;

    vec3 delta = (
        dr * r +
        du * u +
        dl * l
    );
    delta.y = 0.0f;
    this->eye = this->eye + delta;
    this->updateViewMatrix();
}

void Camera::strafeUp(float d)
{
    this->eye.y += d;
    this->updateViewMatrix();
}
  