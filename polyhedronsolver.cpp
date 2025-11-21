#include "polyhedronsolver.h"
#include <cmath>
#include <algorithm>
#include <limits>

PolyhedronSolver::PolyhedronSolver(const std::vector<std::vector<double>>& constraints,
                                   const std::vector<double>& objective,
                                   const std::vector<TypeContrainte>& constraintTypes) {

    // Convertir les contraintes au format standard
    for (size_t i = 0; i < constraints.size(); ++i) {
        if (constraints[i].size() >= 2) {
            double a = constraints[i][0];
            double b = constraints[i][1];
            double c = (constraints[i].size() >= 3) ? constraints[i][2] : 0;

            // Ajuster selon le type de contrainte
            if (constraintTypes[i] == GEQ) {
                a = -a; b = -b; c = -c;
            }
            m_constraints.push_back(Constraint(a, b, c));
        }
    }

    // Ajouter les contraintes de non-négativité
    m_constraints.push_back(Constraint(1, 0, 0));  // x >= 0
    m_constraints.push_back(Constraint(0, 1, 0));  // y >= 0

    m_objective = objective;
    calculateFeasibleRegion();
}

Point PolyhedronSolver::findIntersection(const Constraint& c1, const Constraint& c2) {
    double det = c1.a * c2.b - c2.a * c1.b;

    if (std::abs(det) < 1e-10) {
        return Point(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
    }

    double x = (c1.c * c2.b - c2.c * c1.b) / det;
    double y = (c1.a * c2.c - c2.a * c1.c) / det;

    return Point(x, y);
}

bool PolyhedronSolver::isPointFeasible(const Point& p) {
    for (const auto& constraint : m_constraints) {
        double value = constraint.a * p.x + constraint.b * p.y;
        if (value > constraint.c + 1e-10) {
            return false;
        }
    }
    return true;
}

double PolyhedronSolver::evaluateObjective(const Point& p) {
    if (m_objective.size() >= 2) {
        return m_objective[0] * p.x + m_objective[1] * p.y;
    }
    return 0;
}

void PolyhedronSolver::calculateFeasibleRegion() {
    m_intersectionPoints.clear();
    m_feasibleRegion.clear();
    m_optimalValue = -std::numeric_limits<double>::infinity();

    // Trouver toutes les intersections
    for (size_t i = 0; i < m_constraints.size(); ++i) {
        for (size_t j = i + 1; j < m_constraints.size(); ++j) {
            Point p = findIntersection(m_constraints[i], m_constraints[j]);
            if (!std::isnan(p.x) && !std::isnan(p.y) && isPointFeasible(p)) {
                m_intersectionPoints.push_back(p);
            }
        }
    }

    // Ajouter les points sur les axes
    for (const auto& constraint : m_constraints) {
        // Intersection avec axe X (y=0)
        if (std::abs(constraint.a) > 1e-10) {
            Point p(constraint.c / constraint.a, 0);
            if (p.x >= 0 && isPointFeasible(p)) {
                m_intersectionPoints.push_back(p);
            }
        }

        // Intersection avec axe Y (x=0)
        if (std::abs(constraint.b) > 1e-10) {
            Point p(0, constraint.c / constraint.b);
            if (p.y >= 0 && isPointFeasible(p)) {
                m_intersectionPoints.push_back(p);
            }
        }
    }

    // Trouver le point optimal
    for (const auto& point : m_intersectionPoints) {
        double objectiveValue = evaluateObjective(point);
        if (objectiveValue > m_optimalValue) {
            m_optimalValue = objectiveValue;
            m_optimalPoint = point;
        }
    }

    // Calculer la région réalisable (convex hull simplifié)
    if (!m_intersectionPoints.empty()) {
        m_feasibleRegion = m_intersectionPoints;
    }
}

void PolyhedronSolver::solve() {
    // Déjà calculé dans le constructeur
}

std::vector<Point> PolyhedronSolver::getFeasibleRegion() const {
    return m_feasibleRegion;
}

std::vector<Point> PolyhedronSolver::getIntersectionPoints() const {
    return m_intersectionPoints;
}

Point PolyhedronSolver::getOptimalPoint() const {
    return m_optimalPoint;
}

double PolyhedronSolver::getOptimalValue() const {
    return m_optimalValue;
}

std::vector<Constraint> PolyhedronSolver::getConstraints() const {
    return m_constraints;
}
