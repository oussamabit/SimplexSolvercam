#ifndef POLYHEDRONSOLVER_H
#define POLYHEDRONSOLVER_H

#include <vector>
#include <utility>
#include "simplexsolver.h"
struct Point {
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

struct Constraint {
    double a, b, c; // a*x + b*y <= c
    Constraint(double a = 0, double b = 0, double c = 0) : a(a), b(b), c(c) {}
};

class PolyhedronSolver {
public:
    PolyhedronSolver(const std::vector<std::vector<double>>& constraints,
                     const std::vector<double>& objective,
                     const std::vector<TypeContrainte>& constraintTypes);

    void solve();
    std::vector<Point> getFeasibleRegion() const;
    std::vector<Point> getIntersectionPoints() const;
    Point getOptimalPoint() const;
    double getOptimalValue() const;
    std::vector<Constraint> getConstraints() const;
    double evaluateObjective(const Point& p);

private:
    std::vector<Constraint> m_constraints;
    std::vector<double> m_objective;
    std::vector<Point> m_intersectionPoints;
    std::vector<Point> m_feasibleRegion;
    Point m_optimalPoint;
    double m_optimalValue;

    Point findIntersection(const Constraint& c1, const Constraint& c2);
    bool isPointFeasible(const Point& p);

    void calculateFeasibleRegion();
};

#endif // POLYHEDRONSOLVER_H
