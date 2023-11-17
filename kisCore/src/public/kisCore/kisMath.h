#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
typedef glm::vec2 kisVec2;
typedef glm::vec3 kisVec3;
typedef glm::mat2x2 kisMatrix2;
typedef glm::mat2x2 kisMatrix3;
typedef glm::mat4x4 kisMat4;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
static const double k_kisPI = M_PI;

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
template<typename T>
T kisDegToRad(T deg)
{
    return (T)(deg * (k_kisPI / 180.0));
}

template<typename T>
T kisRadToDeg(T rad)
{
    return (T)(rad * (180.0 / k_kisPI));
}

