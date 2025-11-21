#ifndef DYNAMICPOLYHEDRONSOLVER_H
#define DYNAMICPOLYHEDRONSOLVER_H

#include <vector>
#include <utility>
#include <string>

struct Point2D {
    double x, y;
    Point2D(double x = 0, double y = 0) : x(x), y(y) {}
    bool operator==(const Point2D& other) const {
        return std::abs(x - other.x) < 1e-10 && std::abs(y - other.y) < 1e-10;
    }
};

struct Constraint2D {
    double a, b, c; // a*x + b*y <= c
    std::string name;
    Constraint2D(double a = 0, double b = 0, double c = 0, const std::string& name = "")
        : a(a), b(b), c(c), name(name) {}
};

class DynamicPolyhedronSolver {
public:
    DynamicPolyhedronSolver(const std::vector<std::vector<double>>& constraints,
                            const std::vector<double>& objective,
                            const std::vector<double>& rhs,
                            const std::vector<int>& constraintTypes);

    void solve();
    std::vector<Point2D> getFeasibleRegion() const;
    std::vector<Point2D> getIntersectionPoints() const;
    std::vector<Constraint2D> getConstraints() const;
    Point2D getOptimalPoint() const;
    double getOptimalValue() const;
    std::vector<std::pair<Point2D, double>> getPointsWithObjective() const;

private:
    std::vector<Constraint2D> m_constraints;
    std::vector<double> m_objective;
    std::vector<Point2D> m_intersectionPoints;
    std::vector<Point2D> m_feasibleRegion;
    Point2D m_optimalPoint;
    double m_optimalValue;

    Point2D findIntersection(const Constraint2D& c1, const Constraint2D& c2) const;
    bool isPointFeasible(const Point2D& p) const;
    double evaluateObjective(const Point2D& p) const; // Ajout de const
    void calculateAllIntersections();
    void calculateFeasibleRegion();
    void findOptimalPoint();
};

#endif // DYNAMICPOLYHEDRONSOLVER_H
