#define MAX_BALLS 10000
#define MAX_COLLIDABLE_TRIANGLES 10000000
#define MAX_VERTICES 10000000

#define BALL_RADIUS 0.02135F              // 21.35 mm
#define BALL_MASS 0.0459F                 // 0.0459 kg
#define INV_BALL_MASS 21.78649237472767F  // 1 / 0.0459 kg
#define GRAVITY -9.81F

#define MIN_BOUNCE_HEIGHT 0.1F  // 50mm
#define SPEED_EPSILON 0.0001F

#define rpmToRadS(n) ((n) * 0.10471975511965977F)
#define radSToRPM(n) ((n) * 9.549296585513727F)

// 0.5, times the reference area of the golf ball (0.001425 m^2), times the air density (1.2 kg/m^3)
const f32 k = 0.0008551855026042919F;

const glm::vec3 gravityVec(0.0F, BALL_MASS * GRAVITY, 0.0F);

struct Triangle
{
    glm::vec3 normal;

    // Indices to the vertices in the vertex buffer.
    u16 a, b, c;
};

struct Vtx
{
    glm::vec3 position;
    glm::vec3 normal;
};

struct CollisionGeometry
{
    Vtx vertices[MAX_VERTICES];
    Triangle triangles[MAX_COLLIDABLE_TRIANGLES];

    size_t vertexCount;
    size_t triangleCount;
};

struct Coefficients
{
    f32 lift;
    f32 drag;
};

struct Wind
{
    f32 speed;
    f32 direction;
    bool logWind;
};

enum BallState
{
    BALL_STATE_IDLE,
    BALL_STATE_FLYING,
    BALL_STATE_ROLLING,

    BALL_STATE_COUNT,
};

struct Ball
{
public:
    glm::vec3 startPosition;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 windVector;
    glm::vec3 rotationAxis;

    glm::vec3 gravityForce;
    glm::vec3 liftForce;
    glm::vec3 dragForce;
    glm::vec3 netForce;

    BallState state;
    f32 spinRate;
    f32 launchSpinRate;
    f32 currFlightTime;
    f32 height;
    f32 maxHeight;
    bool alive;

    void simulate(Wind *wind, CollisionGeometry *collisionGeometry, f32 dt);

private:
    void computeSpinRate();
    void computeWindForce(Wind *wind);
    void computeLiftForce(const glm::vec3 &groundSpeed, f32 liftCoefficient);
    void computeDragForce(const glm::vec3 &groundSpeed, f32 dragCoefficient);
    f32 getCurrentHeight(const glm::vec3 &a, const glm::vec3 &normal) const;

    bool intersectsWithPlane(glm::vec3 &normal, f32 d, f32 *outCollisionTime, glm::vec3 &outIntersectionPoint) const;
    bool intersectsWithTriangle(glm::vec3 &p,
                                glm::vec3 &normal,
                                f32 *outCollisionTime,
                                glm::vec3 &outIntersectionPoint);

    bool checkCollision(CollisionGeometry *collisionGeometry,
                        f32 dt,
                        f32 *outCollisionTime,
                        glm::vec3 &outIntersectionPoint,
                        glm::vec3 &outNormal);
    void resolveCollision(const glm::vec3 &normal);
    void computeRebound(const glm::vec3 &surfaceNormal);

    void simulateFlying(Wind *wind, f32 dt);
    void simulateRolling(f32 dt);

    void integrate(f32 dt);
};

struct BallManager
{
    size_t activeBalls;

    void popBall();
    void pushBall(f32 launchSpeed, f32 launchAngle, f32 launchHeading, f32 launchSpinRate, f32 spinAngle);
    bool spawnBall(f32 launchSpeed, f32 launchAngle, f32 launchHeading, f32 launchSpinRate, f32 spinAngle);
    Ball *getBall(size_t ballIndex);

private:
    Ball balls[MAX_BALLS];
};

struct World
{
    BallManager ballManager;
    Wind wind;

    void update(CollisionGeometry *collisionGeometry, f32 dt);
};