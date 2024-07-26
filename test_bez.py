def bezier_point(ps, t):
    p0, p1, p2, p3 = ps
    """
    Calculate a point on a cubic Bezier curve at parameter t.

    Parameters:
    p0, p1, p2, p3 -- tuples representing the control points (x, y).
    t -- the parameter (float) between 0 and 1.

    Returns:
    A tuple representing the point (x, y) on the Bezier curve.
    """
    x = (1-t)**3 * p0[0] + 3 * (1-t)**2 * t * p1[0] + 3 * (1-t) * t**2 * p2[0] + t**3 * p3[0]
    y = (1-t)**3 * p0[1] + 3 * (1-t)**2 * t * p1[1] + 3 * (1-t) * t**2 * p2[1] + t**3 * p3[1]
    return (x, y)


c = [(0, 0), (0.1, 0.9), (0.9, 0.1), (1.0, 1.0)]
pp = 50
for i in range(pp+1):
    v = bezier_point(c, i/pp)
    print(" " * int(v[1] * 50) + "#")
# bezier_point(, 0.5)