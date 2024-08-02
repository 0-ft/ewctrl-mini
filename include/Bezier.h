#ifndef NEWBEZ_H
#define NEWBEZ_H

#include <cstdint>
#include <stdexcept>

class Vec2 {
public:
    float x;
    uint8_t y;
    Vec2() : x(0), y(0) {}
    Vec2(float x, uint8_t y) : x(x), y(y) {}
    Vec2(float x, float y) : x(x), y(static_cast<uint8_t>(y * 255)) {}

    // Overloaded operators for Vec2
    Vec2 operator*(float scalar) const {
        return Vec2(x * scalar, static_cast<uint8_t>(y * scalar));
    }

    Vec2 operator+(const Vec2& v) const {
        return Vec2(x + v.x, static_cast<uint8_t>(y + v.y));
    }

    Vec2 operator-(const Vec2& v) const {
        return Vec2(x - v.x, static_cast<uint8_t>(y - v.y));
    }
};

// Class to hold either a Bezier curve or a pair of points for linear interpolation
class CurveSegment {
public:
    enum class Type {
        Bezier,
        Linear
    };

    // Constructors
    CurveSegment(const Vec2(&points)[4])
        : type(Type::Bezier), numPoints(4) {
        for (int i = 0; i < 4; ++i) {
            controlPoints[i] = points[i];
        }
    }

    CurveSegment(const Vec2& start, const Vec2& end)
        : type(Type::Linear), numPoints(2) {
        controlPoints[0] = start;
        controlPoints[1] = end;
    }

    // Function to evaluate the curve segment at a given t
    float valueAt(float t) const {
        if (type == Type::Bezier) {
            return evaluateBezier3(controlPoints, t);
        } else if (type == Type::Linear) {
            // Normalize y values to [0, 1] range by dividing by 255.0f
            float startY = controlPoints[0].y / 255.0f;
            float endY = controlPoints[1].y / 255.0f;
            return startY * (1 - t) + endY * t;
        } else {
            throw std::runtime_error("Unsupported curve type");
        }
    }

private:
    Type type;
    Vec2 controlPoints[4];
    int numPoints;

    // Function to evaluate a Bezier<3> curve at a given t
    float evaluateBezier3(const Vec2(&controlPoints)[4], float t) const {
        float u = 1 - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;

        // Normalize y values to [0, 1] range by dividing by 255.0f
        float p0y = controlPoints[0].y / 255.0f;
        float p1y = controlPoints[1].y / 255.0f;
        float p2y = controlPoints[2].y / 255.0f;
        float p3y = controlPoints[3].y / 255.0f;

        float y = p0y * uuu;            // u^3 * P0.y
        y += p1y * 3 * uu * t;          // 3 * u^2 * t * P1.y
        y += p2y * 3 * u * tt;          // 3 * u * t^2 * P2.y
        y += p3y * ttt;                 // t^3 * P3.y

        return y;
    }
};

#endif // NEWBEZ_H
