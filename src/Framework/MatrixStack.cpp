#include <Framework/MatrixStack.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <spdlog/spdlog.h>


static glm::vec3 arbitraryTangent(const glm::vec3 &v)
{
    f32 vx = fabsf(v.x);
    f32 vy = fabsf(v.y);
    f32 vz = fabsf(v.z);
    glm::vec3 tmp;
    if (vx <= vy && vx <= vz)
    {
        tmp = glm::vec3(1.0F, 0.0F, 0.0F);
    }
    else if (vy <= vx && vy <= vz)
    {
        tmp = glm::vec3(0.0F, 1.0F, 0.0F);
    }
    else
    {
        tmp = glm::vec3(0.0F, 0.0F, 1.0F);
    }
    tmp = glm::cross(v, tmp);
    return tmp;
}

MatrixStack::MatrixStack() : top_(NULL), count_(0)
{
    matrices_[0] = glm::mat4(1.0F);
    top_ = &matrices_[0];
}

glm::mat4 *MatrixStack::top()
{
    return top_;
}

void MatrixStack::push()
{
    if (count_ >= MAX_MATRICES)
    {
        spdlog::warn("Tried to push matrix when matrix stack was already full\n");
        return;
    }
    // Make a copy of the current matrix and set it as the new head
    matrices_[count_ + 1] = matrices_[count_];
    top_ = &(matrices_[count_ + 1]);
    count_++;
}

void MatrixStack::pop()
{
    if (count_ <= 0)
    {
        spdlog::warn("Tried to pop matrix when stack was already empty\n");
        return;
    }
    count_--;
    top_ = &(matrices_[count_]);
}

void MatrixStack::translate(f32 tx, f32 ty, f32 tz)
{
    if (stackIsEmpty())
    {
        return;
    }

    glm::mat4 trans = glm::translate(glm::mat4(1.0F), glm::vec3(tx, ty, tz));
    *top_ *= trans;
}

void MatrixStack::translate(const glm::vec3 &v)
{
    if (stackIsEmpty())
    {
        return;
    }

    glm::mat4 trans = glm::translate(glm::mat4(1.0F), v);
    *top_ *= trans;
}

void MatrixStack::scale(f32 sx, f32 sy, f32 sz)
{
    if (stackIsEmpty())
    {
        return;
    }

    glm::mat4 trans = glm::scale(glm::mat4(1.0F), glm::vec3(sx, sy, sz));
    *top_ *= trans;
}

void MatrixStack::scale(f32 s)
{
    if (stackIsEmpty())
    {
        return;
    }

    glm::mat4 trans = glm::scale(glm::mat4(1.0F), glm::vec3(s));
    *top_ *= trans;
}

void MatrixStack::scale(const glm::vec3 &v)
{
    if (stackIsEmpty())
    {
        return;
    }

    glm::mat4 trans = glm::scale(glm::mat4(1.0F), v);
    *top_ *= trans;
}

void MatrixStack::rotateX(f32 angle)
{
    if (stackIsEmpty())
    {
        return;
    }

    glm::mat4 rot_x = glm::rotate(glm::mat4(1.0F), angle, glm::vec3(1.0F, 0.0F, 0.0F));
    *top_ *= rot_x;
}

void MatrixStack::rotateY(f32 angle)
{
    if (stackIsEmpty())
    {
        return;
    }

    glm::mat4 rot_y = glm::rotate(glm::mat4(1.0F), angle, glm::vec3(0.0F, 1.0F, 0.0F));
    *top_ *= rot_y;
}

void MatrixStack::rotateZ(f32 angle)
{
    if (stackIsEmpty())
    {
        return;
    }

    glm::mat4 rot_z = glm::rotate(glm::mat4(1.0F), angle, glm::vec3(0.0F, 0.0F, 1.0F));
    *top_ *= rot_z;
}

void MatrixStack::orient(const glm::vec3 &direction, const glm::vec3 &up)
{
    if (stackIsEmpty())
    {
        return;
    }

    glm::vec3 normal(glm::normalize(direction));
    glm::vec3 rotationAxis(glm::cross(normal, up));

    if (fabsf(glm::length2(rotationAxis)) <= FLT_EPSILON)
    {
        // Vectors are opposed to each other, so pick an arbitrary axis
        rotationAxis = arbitraryTangent(normal);
    }

    f32 angle = acosf(glm::dot(normal, up));
    glm::mat4 orientation = glm::rotate(angle, rotationAxis);
    *top_ *= orientation;
}

bool MatrixStack::stackIsEmpty() const
{
    if (count_ == 0)
    {
        spdlog::warn("Tried to operate on empty matrix stack\n");
        return true;
    }
    return false;
}