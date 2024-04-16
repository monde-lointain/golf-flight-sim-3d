#pragma once

#include "Types.hpp"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#define MAX_MATRICES 100

class MatrixStack
{
public:
    MatrixStack();

    glm::mat4 *top();
    
    void push();
    void pop();

    void translate(f32 tx, f32 ty, f32 tz);
    void translate(const glm::vec3 &v);
    void scale(f32 sx, f32 sy, f32 sz);
    void scale(f32 s);
    void scale(const glm::vec3 &v);
    void rotateX(f32 angle);
    void rotateY(f32 angle);
    void rotateZ(f32 angle);
    void orient(const glm::vec3 &direction, const glm::vec3 &up);

private:
    glm::mat4 matrices_[MAX_MATRICES];
    glm::mat4 *top_;
    u32 count_;

    bool stackIsEmpty() const;
};