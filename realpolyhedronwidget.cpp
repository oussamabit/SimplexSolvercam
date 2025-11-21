#include "realpolyhedronwidget.h"
#include <cmath>
#include <algorithm>
#include <QFontMetrics>
#include <memory>

RealPolyhedronWidget::RealPolyhedronWidget(QWidget *parent)
    : QWidget(parent), m_solver(nullptr), m_hasData(false) { // Initialisation corrigée
    setMinimumSize(800, 600);
    setStyleSheet("background-color: white; border: 2px solid #3498db; border-radius: 8px;");
}

void RealPolyhedronWidget::setProblemData(const DynamicPolyhedronSolver& solver) {
    m_solver = std::make_unique<DynamicPolyhedronSolver>(solver); // Création par copie
    m_hasData = true;
    update();
}

void RealPolyhedronWidget::calculateGraphBounds(double &minX, double &maxX, double &minY, double &maxY) {
    minX = 0; maxX = 10; minY = 0; maxY = 10; // Valeurs par défaut

    if (!m_hasData || !m_solver) return;

    auto points = m_solver->getIntersectionPoints();
    auto constraints = m_solver->getConstraints();

    if (points.empty()) return;

    // Trouver les bornes réelles basées sur les points d'intersection
    minX = minY = std::numeric_limits<double>::max();
    maxX = maxY = std::numeric_limits<double>::lowest();

    for (const auto& p : points) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }

    // Ajouter les intersections avec les axes
    for (const auto& constraint : constraints) {
        if (std::abs(constraint.a) > 1e-10) {
            double x = constraint.c / constraint.a;
            if (x >= 0) {
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
            }
        }
        if (std::abs(constraint.b) > 1e-10) {
            double y = constraint.c / constraint.b;
            if (y >= 0) {
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
        }
    }

    // Ajouter des marges
    double marginX = std::max((maxX - minX) * 0.1, 1.0);
    double marginY = std::max((maxY - minY) * 0.1, 1.0);

    minX = std::max(0.0, minX - marginX);
    maxX = maxX + marginX;
    minY = std::max(0.0, minY - marginY);
    maxY = maxY + marginY;

    // Assurer un minimum
    maxX = std::max(maxX, 5.0);
    maxY = std::max(maxY, 5.0);
}

// Les autres méthodes restent similaires mais utilisent m_solver-> au lieu de m_solver.
void RealPolyhedronWidget::drawAxes(QPainter &painter, int width, int height, int margin,
                                    double scale, int originX, int originY, double maxX, double maxY) {
    // Dessiner les axes
    painter.setPen(QPen(QColor(100, 100, 100), 2));
    painter.drawLine(margin, originY, width - margin, originY); // Axe X
    painter.drawLine(originX, margin, originX, height - margin); // Axe Y

    // Étiquettes des axes
    painter.setPen(QColor(50, 50, 50));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(width - margin + 10, originY - 5, "x₁");
    painter.drawText(originX + 5, margin - 10, "x₂");

    // Graduations sur l'axe X
    for (int i = 0; i <= maxX; i += std::max(1, static_cast<int>(maxX/5))) {
        int x = originX + i * scale;
        painter.setPen(QPen(QColor(200, 200, 200), 1));
        painter.drawLine(x, originY - 5, x, originY + 5);

        painter.setPen(QColor(100, 100, 100));
        painter.drawText(x - 5, originY + 20, QString::number(i));
    }

    // Graduations sur l'axe Y
    for (int i = 0; i <= maxY; i += std::max(1, static_cast<int>(maxY/5))) {
        int y = originY - i * scale;
        painter.setPen(QPen(QColor(200, 200, 200), 1));
        painter.drawLine(originX - 5, y, originX + 5, y);

        painter.setPen(QColor(100, 100, 100));
        painter.drawText(originX - 30, y + 5, QString::number(i));
    }
}

void RealPolyhedronWidget::drawConstraints(QPainter &painter, int originX, int originY, double scale) {
    if (!m_hasData || !m_solver) return;

    auto constraints = m_solver->getConstraints();
    QVector<QColor> constraintColors = {
        QColor(231, 76, 60),   // Rouge
        QColor(52, 152, 219),  // Bleu
        QColor(46, 204, 113),  // Vert
        QColor(155, 89, 182),  // Violet
        QColor(241, 196, 15),  // Jaune
        QColor(230, 126, 34),  // Orange
        QColor(149, 165, 166)  // Gris
    };

    for (size_t i = 0; i < constraints.size(); ++i) {
        const auto& c = constraints[i];
        QColor color = constraintColors[i % constraintColors.size()];

        // Calculer deux points pour la ligne
        QPointF p1, p2;
        double maxVal = 20.0; // Valeur maximale pour le tracé

        if (std::abs(c.b) > 1e-10) {
            // x = 0
            p1 = QPointF(0, c.c / c.b);
            // x = maxVal
            p2 = QPointF(maxVal, (c.c - c.a * maxVal) / c.b);
        } else if (std::abs(c.a) > 1e-10) {
            // Ligne verticale
            p1 = QPointF(c.c / c.a, 0);
            p2 = QPointF(c.c / c.a, maxVal);
        } else {
            continue;
        }

        // Convertir en coordonnées écran
        int x1 = originX + p1.x() * scale;
        int y1 = originY - p1.y() * scale;
        int x2 = originX + p2.x() * scale;
        int y2 = originY - p2.y() * scale;

        // Dessiner la ligne
        painter.setPen(QPen(color, 3));
        painter.drawLine(x1, y1, x2, y2);

        // Étiquette
        painter.drawText(x2 + 5, y2 + 5, QString::fromStdString(c.name));
    }
}

void RealPolyhedronWidget::drawFeasibleRegion(QPainter &painter, int originX, int originY, double scale) {
    if (!m_hasData || !m_solver) return;

    auto points = m_solver->getIntersectionPoints();

    if (points.size() < 3) return;

    // Créer un polygone avec les points faisables
    QPolygon feasiblePoly;
    for (const auto& p : points) {
        if (p.x >= 0 && p.y >= 0) {
            int x = originX + p.x * scale;
            int y = originY - p.y * scale;
            feasiblePoly << QPoint(x, y);
        }
    }

    // Trier les points par angle pour un polygone convexe
    if (feasiblePoly.size() >= 3) {
        painter.setBrush(QColor(52, 152, 219, 80)); // Bleu semi-transparent
        painter.setPen(QPen(QColor(52, 152, 219, 150), 2));
        painter.drawPolygon(feasiblePoly);
    }
}

void RealPolyhedronWidget::drawPoints(QPainter &painter, int originX, int originY, double scale) {
    if (!m_hasData || !m_solver) return;

    auto pointsWithObj = m_solver->getPointsWithObjective();
    auto optimalPoint = m_solver->getOptimalPoint();
    double optimalValue = m_solver->getOptimalValue();

    if (pointsWithObj.empty()) return;

    char pointLabel = 'A';

    for (const auto& [point, objValue] : pointsWithObj) {
        if (point.x >= 0 && point.y >= 0) {
            int x = originX + point.x * scale;
            int y = originY - point.y * scale;

            bool isOptimal = (std::abs(point.x - optimalPoint.x) < 1e-6 &&
                              std::abs(point.y - optimalPoint.y) < 1e-6);

            if (isOptimal) {
                // Point optimal - plus grand et vert
                painter.setBrush(QColor(46, 204, 113));
                painter.setPen(QPen(QColor(39, 174, 96), 3));
                painter.drawEllipse(QPoint(x, y), 10, 10);

                // Valeur Z optimale
                painter.setPen(QColor(39, 174, 96));
                painter.setFont(QFont("Arial", 10, QFont::Bold));
                painter.drawText(x + 15, y - 10,
                                 QString("Z=%1*").arg(optimalValue, 0, 'f', 1));
            } else {
                // Point réalisable normal - bleu
                painter.setBrush(QColor(52, 152, 219));
                painter.setPen(QPen(QColor(41, 128, 185), 2));
                painter.drawEllipse(QPoint(x, y), 8, 8);

                // Valeur Z
                painter.setPen(QColor(52, 152, 219));
                painter.setFont(QFont("Arial", 8));
                painter.drawText(x + 12, y - 8,
                                 QString("Z=%1").arg(objValue, 0, 'f', 1));
            }

            // Label du point (A, B, C, ...)
            painter.setPen(QColor(30, 30, 30));
            painter.setFont(QFont("Arial", 10, QFont::Bold));
            painter.drawText(x - 5, y + 20, QString("%1").arg(pointLabel));

            pointLabel++;
            if (pointLabel > 'Z') pointLabel = 'A'; // Recycler si trop de points
        }
    }
}

void RealPolyhedronWidget::drawObjectiveFunction(QPainter &painter, int originX, int originY, double scale,
                                                 int width, int height, int margin) {
    if (!m_hasData || !m_solver) return;

    auto objective = m_solver->getOptimalPoint();
    auto constraints = m_solver->getConstraints();

    if (constraints.empty()) return;

    // Dessiner la direction de la fonction objectif (ligne pointillée)
    QPen objPen(QColor(230, 126, 34), 2);
    objPen.setStyle(Qt::DashLine);
    painter.setPen(objPen);

    // Tracer une ligne dans la direction d'optimisation
    int x1 = originX;
    int y1 = originY;
    int x2 = originX + objective.x * scale * 1.5;
    int y2 = originY - objective.y * scale * 1.5;

    painter.drawLine(x1, y1, x2, y2);

    // Étiquette de la fonction objectif
    painter.setPen(QColor(230, 126, 34));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(x2 + 5, y2 - 5, "Z");
}

void RealPolyhedronWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int width = this->width();
    int height = this->height();
    int margin = 80;
    int graphWidth = width - 2 * margin;
    int graphHeight = height - 2 * margin;

    // Fond
    painter.fillRect(rect(), QColor(255, 255, 255));

    if (!m_hasData) {
        painter.setPen(QColor(100, 100, 100));
        painter.drawText(rect().center(), "Aucune donnée de problème");
        return;
    }

    // Calculer les bornes du graphique
    double minX, maxX, minY, maxY;
    calculateGraphBounds(minX, maxX, minY, maxY);

    double scaleX = graphWidth / (maxX - minX);
    double scaleY = graphHeight / (maxY - minY);
    double scale = std::min(scaleX, scaleY);

    // Origine (coin inférieur gauche)
    int originX = margin - minX * scale;
    int originY = height - margin + minY * scale;

    // Dessiner les éléments dans l'ordre
    drawAxes(painter, width, height, margin, scale, originX, originY, maxX, maxY);
    drawConstraints(painter, originX, originY, scale);
    drawFeasibleRegion(painter, originX, originY, scale);
    drawPoints(painter, originX, originY, scale);
    drawObjectiveFunction(painter, originX, originY, scale, width, height, margin);

    // Titre
    painter.setPen(QColor(30, 30, 30));
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    painter.drawText(rect().center().x() - 150, margin - 20,
                     "Polyèdre des Contraintes - Solution Réelle");

    // Légende
    painter.setFont(QFont("Arial", 9));
    painter.drawText(margin, height - margin + 30,
                     "● Vert: Solution optimale   ● Bleu: Points réalisables   ─── Contraintes");
}
