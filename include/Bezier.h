#ifndef NEWBEZ_H
#define NEWBEZ_H

class Vec2 {
public:
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}

    // Overloaded operators for Vec2
    Vec2 operator*(float scalar) const {
        return Vec2(x * scalar, y * scalar);
    }

    Vec2 operator+(const Vec2& v) const {
        return Vec2(x + v.x, y + v.y);
    }

    Vec2 operator-(const Vec2& v) const {
        return Vec2(x - v.x, y - v.y);
    }
};

// Struct to hold the control points
struct BezierControlPoints {
    Vec2 p0, p1, p2, p3;
};

// Class to hold either a Bezier curve or a pair of points for linear interpolation
class CurveSegment {
public:
    enum class Type {
        Bezier,
        Linear
    };

    // Constructors
    CurveSegment(const BezierControlPoints& bezierPoints)
        : type(Type::Bezier), bezierPoints(bezierPoints) {}

    CurveSegment(const Vec2& start, const Vec2& end)
        : type(Type::Linear) {
        linearPoints.start = start;
        linearPoints.end = end;
    }

    // Function to evaluate the curve segment at a given t
    Vec2 valueAt(float t) const {
        if (type == Type::Bezier) {
            return evaluateBezier3(bezierPoints, t);
        } else {
            return linearPoints.start * (1 - t) + linearPoints.end * t;
        }
    }

private:
    Type type;
    union {
        BezierControlPoints bezierPoints;
        struct {
            Vec2 start;
            Vec2 end;
        } linearPoints;
    };

    // Function to evaluate a Bezier<3> curve at a given t
    Vec2 evaluateBezier3(const BezierControlPoints& controlPoints, float t) const {
        float u = 1 - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;

        Vec2 p = controlPoints.p0 * uuu;           // u^3 * P0
        p = p + controlPoints.p1 * 3 * uu * t;     // 3 * u^2 * t * P1
        p = p + controlPoints.p2 * 3 * u * tt;     // 3 * u * t^2 * P2
        p = p + controlPoints.p3 * ttt;            // t^3 * P3

        return p;
    }
};

#endif // NEWBEZ_H
