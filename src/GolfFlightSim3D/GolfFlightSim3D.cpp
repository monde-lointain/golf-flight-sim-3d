// clang-format off
Coefficients COEFF_LUT[10][7] = 
{
     {{-0.11F, 0.52F}, {-0.06F, 0.39F}, {0.06F, 0.36F}, {0.35F, 0.42F}, {0.39F, 0.40F}, {0.41F, 0.48F}, {0.49F, 0.52F}},
     {{ 0.00F, 0.33F}, { 0.12F, 0.25F}, {0.18F, 0.28F}, {0.33F, 0.36F}, {0.36F, 0.38F}, {0.38F, 0.43F}, {0.45F, 0.45F}},
     {{ 0.06F, 0.22F}, { 0.17F, 0.24F}, {0.24F, 0.27F}, {0.29F, 0.31F}, {0.33F, 0.34F}, {0.34F, 0.37F}, {0.39F, 0.39F}},
     {{ 0.07F, 0.23F}, { 0.14F, 0.23F}, {0.19F, 0.25F}, {0.24F, 0.28F}, {0.28F, 0.30F}, {0.31F, 0.33F}, {0.35F, 0.36F}},
     {{ 0.07F, 0.24F}, { 0.13F, 0.24F}, {0.16F, 0.25F}, {0.20F, 0.27F}, {0.24F, 0.28F}, {0.27F, 0.30F}, {0.31F, 0.34F}},
     {{ 0.07F, 0.24F}, { 0.12F, 0.24F}, {0.15F, 0.25F}, {0.18F, 0.26F}, {0.21F, 0.26F}, {0.24F, 0.29F}, {0.28F, 0.32F}},
     {{ 0.08F, 0.25F}, { 0.12F, 0.25F}, {0.14F, 0.25F}, {0.17F, 0.26F}, {0.19F, 0.26F}, {0.22F, 0.28F}, {0.26F, 0.29F}},
     {{ 0.08F, 0.25F}, { 0.12F, 0.25F}, {0.14F, 0.25F}, {0.16F, 0.26F}, {0.18F, 0.26F}, {0.20F, 0.28F}, {0.23F, 0.29F}},
     {{ 0.07F, 0.25F}, { 0.11F, 0.25F}, {0.13F, 0.25F}, {0.15F, 0.26F}, {0.17F, 0.26F}, {0.18F, 0.27F}, {0.22F, 0.28F}},
     {{ 0.07F, 0.24F}, { 0.11F, 0.24F}, {0.13F, 0.25F}, {0.15F, 0.26F}, {0.16F, 0.26F}, {0.17F, 0.27F}, {0.20F, 0.27F}}
};
// clang-format on

static void computeLiftAndDragCoefficients(f32 groundSpeedSquared,
                                           f32 spinRate,
                                           f32 *liftCoefficient,
                                           f32 *dragCoefficient)
{
    s32 row, col;

    if (groundSpeedSquared > 7249.0F)
    {
        row = 9;
    }
    else if (groundSpeedSquared > 5939.0F)
    {
        row = 8;
    }
    else if (groundSpeedSquared > 4698.0F)
    {
        row = 7;
    }
    else if (groundSpeedSquared > 3588.0F)
    {
        row = 6;
    }
    else if (groundSpeedSquared > 2654.0F)
    {
        row = 5;
    }
    else if (groundSpeedSquared > 1874.0F)
    {
        row = 4;
    }
    else if (groundSpeedSquared > 1226.0F)
    {
        row = 3;
    }
    else if (groundSpeedSquared > 705.0F)
    {
        row = 2;
    }
    else if (groundSpeedSquared > 338.0F)
    {
        row = 1;
    }
    else
    {
        row = 0;
    }

    if (spinRate > 5478.0F)
    {
        col = 6;
    }
    else if (spinRate > 4223.0F)
    {
        col = 5;
    }
    else if (spinRate > 3283.0F)
    {
        col = 4;
    }
    else if (spinRate > 2340.0F)
    {
        col = 3;
    }
    else if (spinRate > 1433.0F)
    {
        col = 2;
    }
    else if (spinRate > 500.0F)
    {
        col = 1;
    }
    else
    {
        col = 0;
    }

    Coefficients *coeffs = &COEFF_LUT[row][col];
    *liftCoefficient = coeffs->lift;
    *dragCoefficient = coeffs->drag;
}

static void handleSlidingXY(f32 vx, f32 vy, f32 wz, f32 e, f32 mu, f32 r, f32 *vrx, f32 *vry, f32 *wrz)
{
    *vrx = vx - (mu * fabsf(vy) * (1.0F + e));
    *vry = -(e * vy);
    *wrz = ((5.0F * mu * fabsf(vy)) / (2.0F * r)) * (1.0F + e) - wz;
}

static void handleRollingXY(f32 vx, f32 vy, f32 wz, f32 e, f32 r, f32 *vrx, f32 *vry, f32 *wrz)
{
    *vrx = ((5.0F * vx) - (2.0F * r * wz)) / 7.0F;
    *vry = -(e * vy);
    *wrz = *vrx / r;
}

static void handleSlidingZY(f32 vy, f32 wx, f32 e, f32 mu, f32 r, f32 *vrz, f32 *wrx)
{
    *vrz = -(mu * fabsf(vy) * (1.0F + e));
    *wrx = ((5.0F * mu * fabsf(vy)) / (2.0F * r)) * (1.0F + e) - wx;
}

static void handleRollingZY(f32 vz, f32 wx, f32 r, f32 *vrz, f32 *wrx)
{
    *vrz = ((5.0F * vz) - (2.0F * r * wx)) / 7.0F;
    *wrx = *vrz / r;
}

void Ball::computeSpinRate()
{
    // Spin decreases roughly 4% per second
    const f32 spinDecayRate = 24.5F;

    spinRate = launchSpinRate * expf(-currFlightTime / spinDecayRate);
}

void Ball::computeWindForce(Wind *wind)
{
    windVector = glm::vec3(wind->speed * sinf(wind->direction), 0.0F, wind->speed * cosf(wind->direction));

    if (!wind->logWind)
    {
        return;
    }

    f32 ballHeight = position.y;

    // Reference height for the log wind profile
    const f32 referenceHeight = 10.0F;

    // Height at which the wind speed becomes bzero
    const f32 roughnessLengthScale = 0.4F;

    if (ballHeight < roughnessLengthScale)
    {
        ballHeight = roughnessLengthScale;
    }

    windVector *= logf(ballHeight / roughnessLengthScale) / logf(referenceHeight / roughnessLengthScale);
}

void Ball::computeLiftForce(const glm::vec3 &groundSpeed, f32 liftCoefficient)
{
    liftForce = glm::vec3(0.0F);

    f32 speedSq = glm::length2(groundSpeed);

    if (speedSq > 0.0F)
    {
        // Lift acts perpendicular to the relative motion of the golf ball
        glm::vec3 direction = glm::normalize(glm::cross(groundSpeed, rotationAxis));

        f32 magnitude = k * liftCoefficient * speedSq;

        liftForce = direction * magnitude;
    }
}

void Ball::computeDragForce(const glm::vec3 &groundSpeed, f32 dragCoefficient)
{
    dragForce = glm::vec3(0.0F);
    f32 speedSq = glm::length2(groundSpeed);

    if (speedSq > 0.0F)
    {
        // Drag acts opposite to the relative motion of the golf ball
        glm::vec3 direction = glm::normalize(groundSpeed) * -1.0F;

        f32 magnitude = k * dragCoefficient * speedSq;

        dragForce = direction * magnitude;
    }
}

void Ball::simulateFlying(Wind *wind, f32 dt)
{
    computeSpinRate();

    computeWindForce(wind);

    glm::vec3 groundSpeed = velocity - windVector;

    f32 liftCoefficient, dragCoefficient;
    computeLiftAndDragCoefficients(glm::length2(groundSpeed), spinRate, &liftCoefficient, &dragCoefficient);

    computeLiftForce(groundSpeed, liftCoefficient);

    computeDragForce(groundSpeed, dragCoefficient);

    netForce = gravityForce + liftForce + dragForce;

    integrate(dt);

    currFlightTime += dt;
}

bool Ball::intersectsWithPlane(glm::vec3 &normal, f32 d, f32 *outCollisionTime, glm::vec3 &outIntersectionPoint) const
{
    f32 distance = glm::dot(normal, position) - d;

    if (fabsf(distance) <= BALL_RADIUS)
    {
        // Sphere is already overlapping
        *outCollisionTime = 0.0F;
        outIntersectionPoint = position;

        return true;
    }

    f32 denom = glm::dot(normal, velocity);
    if (denom * distance >= 0.0F)
    {
        // No intersection if the ball is moving parallel to or away from the plane
        return false;
    }

    f32 r = distance > 0.0F ? BALL_RADIUS : -BALL_RADIUS;

    *outCollisionTime = (r - distance) / denom;
    outIntersectionPoint = position + *outCollisionTime * velocity - r * normal;

    return true;
}

bool Ball::intersectsWithTriangle(glm::vec3 &p,
                                  glm::vec3 &normal,
                                  f32 *outCollisionTime,
                                  glm::vec3 &outIntersectionPoint)
{
    // Test against the plane representing the triangle
    const f32 d = glm::dot(normal, p);
    bool colliding = intersectsWithPlane(normal, d, outCollisionTime, outIntersectionPoint);

    return colliding;
}

f32 Ball::getCurrentHeight(const glm::vec3 &a, const glm::vec3 &normal) const
{
    f32 currentHeight = glm::dot(position - a, normal);
    return currentHeight;
}

bool Ball::checkCollision(CollisionGeometry *collisionGeometry,
                          f32 dt,
                          f32 *outCollisionTime,
                          glm::vec3 &outIntersectionPoint,
                          glm::vec3 &outNormal)
{
    for (size_t triangleIndex = 0; triangleIndex < collisionGeometry->triangleCount; triangleIndex++)
    {
        Triangle *triangle = &collisionGeometry->triangles[triangleIndex];

        glm::vec3 &p = collisionGeometry->vertices[triangle->a].position;
        glm::vec3 &normal = triangle->normal;

        f32 collisionTime;
        glm::vec3 intersectionPoint;

        height = getCurrentHeight(p, normal);
        if (height > maxHeight)
        {
            maxHeight = height;
        }

        bool intersects = intersectsWithTriangle(p, normal, &collisionTime, intersectionPoint);

        if (intersects && collisionTime <= dt && collisionTime >= 0.0F)
        {
            *outCollisionTime = collisionTime;
            outIntersectionPoint = intersectionPoint;
            outNormal = normal;
            return true;
        }
    }

    return false;
}

void Ball::integrate(f32 dt)
{
    acceleration = netForce * INV_BALL_MASS;

    velocity += acceleration * dt;

    position += velocity * dt;
}

void Ball::computeRebound(const glm::vec3 &surfaceNormal)
{
    const f32 e = 0.5F;
    const f32 mu = 0.4F;
    const f32 r = BALL_RADIUS;

    // Compute angular velocity from rotation axis and spin rate
    glm::vec3 angularVelocity = rpmToRadS(spinRate) * rotationAxis;

    // Compute basis vectors of the new coordinate system
    glm::vec3 xBasis = glm::normalize(glm::vec3(velocity.x, 0.0F, velocity.z));
    glm::vec3 yBasis = surfaceNormal;
    glm::vec3 zBasis = glm::normalize(glm::cross(xBasis, yBasis));

    // clang-format off
    glm::mat3 T(xBasis.x, yBasis.x, zBasis.x,
                xBasis.y, yBasis.y, zBasis.y,
                xBasis.z, yBasis.z, zBasis.z);
    // clang-format on

    // Transform velocity and angular velocity to the new coordinate system
    glm::vec3 transformedVelocity = T * velocity;
    glm::vec3 transformedAngularVelocity = T * angularVelocity;

    // Extract components
    f32 vx = transformedVelocity.x;
    f32 vy = transformedVelocity.y;
    f32 vz = transformedVelocity.z;
    f32 wx = transformedAngularVelocity.x;
    f32 wy = transformedAngularVelocity.y;
    f32 wz = transformedAngularVelocity.z;

    // Determine sliding or rolling in XY plane
    f32 muCz = (2.0F * (vx + r * wz)) / (7.0F * (vy * (1.0F + e)));

    f32 vrx, vry, wrz;
    if (mu < muCz)
    {
        handleSlidingXY(vx, vy, wz, e, mu, r, &vrx, &vry, &wrz);
    }
    else
    {
        handleRollingXY(vx, vy, wz, e, r, &vrx, &vry, &wrz);
    }

    // Determine sliding or rolling in ZY plane
    f32 muCx = -(2.0F * (r * wz)) / (7.0F * (vy * (1.0F + e)));

    f32 vrz, wrx;
    if (mu < muCx)
    {
        handleSlidingZY(vy, wx, e, mu, r, &vrz, &wrx);
    }
    else
    {
        handleRollingZY(vz, wx, r, &vrz, &wrx);
    }

    // Transform back to the original coordinate system
    glm::mat3 invT = glm::inverse(T);
    glm::vec3 finalVelocity = invT * glm::vec3(vrx, vry, vz);
    glm::vec3 finalAngularVelocity = invT * glm::vec3(wx, wy, wrz);

    // Save the resulting conditions after collision
    velocity = finalVelocity;
    rotationAxis = glm::normalize(finalAngularVelocity);
    spinRate = radSToRPM(glm::length(finalAngularVelocity));
}

void Ball::resolveCollision(const glm::vec3 &surfaceNormal)
{
    computeRebound(surfaceNormal);
}

void Ball::simulateRolling(f32 dt)
{
    const f32 frictionMagnitude = 0.04F;

    acceleration = glm::vec3(0.0F);
    liftForce = glm::vec3(0.0F);
    dragForce = glm::vec3(0.0F);
    velocity.y = 0.0F;

    // TODO: This is basically just a hack right now and will not work with sloped surfaces!
    if (glm::length2(velocity) > SPEED_EPSILON)
    {
        // Calculate frictional force direction and magnitude
        glm::vec3 frictionDirection = glm::normalize(velocity) * -1.0F;

        // Compute frictional force vector
        glm::vec3 frictionForce = frictionMagnitude * frictionDirection;
        netForce = frictionForce;

        integrate(dt);
    }
    else
    {
        velocity = glm::vec3(0.0F);
        spinRate = 0.0F;
        state = BALL_STATE_IDLE;
    }
}

void Ball::simulate(Wind *wind, CollisionGeometry *collisionGeometry, f32 dt)
{
    if (!alive)
    {
        return;
    }

    switch (state)
    {
        case BALL_STATE_IDLE:
        {
            break;
        }
        case BALL_STATE_FLYING:
        {
            simulateFlying(wind, dt);

            f32 collisionTime;
            glm::vec3 intersectionPoint;
            glm::vec3 normal;

            bool colliding = checkCollision(collisionGeometry, dt, &collisionTime, intersectionPoint, normal);

            if (colliding)
            {
                spdlog::debug(
                    "Collision detected:\n"
                    "Velocity: ({:.2f}, {:.2f}, {:.2f})\n"
                    "Normal: ({:.2f}, {:.2f}, {:.2f})\n"
                    "Rotation axis: ({:.2f}, {:.2f}, {:.2f})\n"
                    "Spin rate: {:.2f} RPM",
                    velocity.x, velocity.y, velocity.z, normal.x, normal.y, normal.z, rotationAxis.x, rotationAxis.y,
                    rotationAxis.z, spinRate);

                position = intersectionPoint;
                position.y += BALL_RADIUS;

                currFlightTime = 0.0F;

                if (maxHeight <= MIN_BOUNCE_HEIGHT)
                {
                    state = BALL_STATE_ROLLING;
                    break;
                }

                resolveCollision(normal);

                maxHeight = 0.0F;
            }

            break;
        }
        case BALL_STATE_ROLLING:
        {
            simulateRolling(dt);
            break;
        }
        default:
        {
            invalidDefaultCase;
        }
    }
}

void BallManager::popBall()
{
    bzero(&balls[activeBalls--], sizeof(Ball));
}

void BallManager::pushBall(f32 launchSpeed, f32 launchAngle, f32 launchHeading, f32 launchSpinRate, f32 spinAngle)
{
    Ball *ball = &balls[activeBalls++];

    const f32 teeHeight = 0.0381F;  // 1.5 in

    ball->startPosition = glm::vec3(0.0F, teeHeight + BALL_RADIUS, 0.0F);
    ball->position = ball->startPosition;

    ball->state = BALL_STATE_FLYING;

    ball->gravityForce = gravityVec;

    ball->velocity = glm::rotateX(glm::vec3(0.0F, 0.0F, launchSpeed), -launchAngle);
    ball->velocity = glm::rotateY(ball->velocity, launchHeading);

    ball->rotationAxis = glm::rotateY(glm::vec3(1.0F, 0.0F, 0.0F), launchHeading);
    ball->rotationAxis = glm::rotateZ(ball->rotationAxis, spinAngle);

    ball->launchSpinRate = launchSpinRate;

    ball->alive = true;
}

bool BallManager::spawnBall(f32 launchSpeed, f32 launchAngle, f32 launchHeading, f32 launchSpinRate, f32 spinAngle)
{
    if (activeBalls >= MAX_BALLS)
    {
        spdlog::warn("Max balls reached");

        return false;
    }

    pushBall(launchSpeed, launchAngle, launchHeading, launchSpinRate, spinAngle);

    return true;
}

Ball *BallManager::getBall(size_t ballIndex)
{
    Ball *ball = &balls[ballIndex];
    return ball;
}

void World::update(CollisionGeometry *collisionGeometry, f32 dt)
{
    for (size_t ballIndex = 0; ballIndex < ballManager.activeBalls; ballIndex++)
    {
        Ball *ball = ballManager.getBall(ballIndex);
        ball->simulate(&wind, collisionGeometry, dt);
    }
}