#include "dynamicpolyhedronsolver.h"
#include <cmath>
#include <algorithm>
#include <limits>
#include <set>

DynamicPolyhedronSolver::DynamicPolyhedronSolver(const std::vector<std::vector<double>>& constraints,
                                                 const std::vector<double>& objective,
                                                 const std::vector<double>& rhs,
                                                 const std::vector<int>& constraintTypes) {

    // Convertir les contraintes au format standard
    for (size_t i = 0; i < constraints.size(); ++i) {
        if (constraints[i].size() >= 2) {
            double a = constraints[i][0];
            double b = constraints[i][1];
            double c = rhs[i];

            // Ajuster selon le type de contrainte
            if (constraintTypes[i] == 0) { // GEQ -> convertir en LEQ
                a = -a; b = -b; c = -c;
            }
            // EQ reste le même
            // LEQ reste le même

            m_constraints.push_back(Constraint2D(a, b, c, "C" + std::to_string(i + 1)));
        }
    }

    // Ajouter les contraintes de non-négativité
    m_constraints.push_back(Constraint2D(1, 0, 0, "x₁≥0"));  // x >= 0
    m_constraints.push_back(Constraint2D(0, 1, 0, "x₂≥0"));  // y >= 0

    m_objective = objective;
    calculateAllIntersections();
    calculateFeasibleRegion();
    findOptimalPoint();
}

Point2D DynamicPolyhedronSolver::findIntersection(const Constraint2D& c1, const Constraint2D& c2) const {
    double det = c1.a * c2.b - c2.a * c1.b;

    if (std::abs(det) < 1e-10) {
        return Point2D(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
    }

    double x = (c1.c * c2.b - c2.c * c1.b) / det;
    double y = (c1.a * c2.c - c2.a * c1.c) / det;

    return Point2D(x, y);
}

bool DynamicPolyhedronSolver::isPointFeasible(const Point2D& p) const {
    for (const auto& constraint : m_constraints) {
        double value = constraint.a * p.x + constraint.b * p.y;
        if (value > constraint.c + 1e-10) {
            return false;
        }
    }
    return true;
}

double DynamicPolyhedronSolver::evaluateObjective(const Point2D& p) const { // Changé en const
    if (m_objective.size() >= 2) {
        return m_objective[0] * p.x + m_objective[1] * p.y;
    }
    return 0;
}

void DynamicPolyhedronSolver::calculateAllIntersections() {
    m_intersectionPoints.clear();
    std::set<std::pair<double, double>> uniquePoints;

    // Trouver toutes les intersections entre contraintes
    for (size_t i = 0; i < m_constraints.size(); ++i) {
        for (size_t j = i + 1; j < m_constraints.size(); ++j) {
            Point2D p = findIntersection(m_constraints[i], m_constraints[j]);
            if (!std::isnan(p.x) && !std::isnan(p.y)) {
                // Vérifier l'unicité et la faisabilité
                auto key = std::make_pair(std::round(p.x * 1000), std::round(p.y * 1000));
                if (uniquePoints.find(key) == uniquePoints.end() && isPointFeasible(p)) {
                    uniquePoints.insert(key);
                    m_intersectionPoints.push_back(p);
                }
            }
        }
    }

    // Ajouter les intersections avec les axes
    for (const auto& constraint : m_constraints) {
        // Intersection avec axe X (y=0)
        if (std::abs(constraint.a) > 1e-10) {
            Point2D p(constraint.c / constraint.a, 0);
            if (p.x >= 0 && isPointFeasible(p)) {
                auto key = std::make_pair(std::round(p.x * 1000), std::round(p.y * 1000));
                if (uniquePoints.find(key) == uniquePoints.end()) {
                    uniquePoints.insert(key);
                    m_intersectionPoints.push_back(p);
                }
            }
        }

        // Intersection avec axe Y (x=0)
        if (std::abs(constraint.b) > 1e-10) {
            Point2D p(0, constraint.c / constraint.b);
            if (p.y >= 0 && isPointFeasible(p)) {
                auto key = std::make_pair(std::round(p.x * 1000), std::round(p.y * 1000));
                if (uniquePoints.find(key) == uniquePoints.end()) {
                    uniquePoints.insert(key);
                    m_intersectionPoints.push_back(p);
                }
            }
        }
    }
}

void DynamicPolyhedronSolver::calculateFeasibleRegion() {
    m_feasibleRegion.clear();

    if (m_intersectionPoints.empty()) return;

    // Trier les points par angle pour créer un polygone convexe
    // Pour simplifier, on utilise tous les points faisables
    m_feasibleRegion = m_intersectionPoints;
}

void DynamicPolyhedronSolver::findOptimalPoint() {
    m_optimalValue = -std::numeric_limits<double>::infinity();

    for (const auto& point : m_intersectionPoints) {
        double objectiveValue = evaluateObjective(point);
        if (objectiveValue > m_optimalValue) {
            m_optimalValue = objectiveValue;
            m_optimalPoint = point;
        }
    }
}

void DynamicPolyhedronSolver::solve() {
    // Déjà calculé dans le constructeur
}

std::vector<Point2D> DynamicPolyhedronSolver::getFeasibleRegion() const {
    return m_feasibleRegion;
}

std::vector<Point2D> DynamicPolyhedronSolver::getIntersectionPoints() const {
    return m_intersectionPoints;
}

std::vector<Constraint2D> DynamicPolyhedronSolver::getConstraints() const {
    return m_constraints;
}

Point2D DynamicPolyhedronSolver::getOptimalPoint() const {
    return m_optimalPoint;
}

double DynamicPolyhedronSolver::getOptimalValue() const {
    return m_optimalValue;
}

std::vector<std::pair<Point2D, double>> DynamicPolyhedronSolver::getPointsWithObjective() const {
    std::vector<std::pair<Point2D, double>> result;
    for (const auto& point : m_intersectionPoints) {
        result.emplace_back(point, evaluateObjective(point));
    }
    return result;
}
