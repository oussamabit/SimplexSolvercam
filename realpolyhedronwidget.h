#ifndef REALPOLYHEDRONWIDGET_H
#define REALPOLYHEDRONWIDGET_H

#include <QWidget>
#include <QPainter>
#include <vector>
#include <memory>
#include "dynamicpolyhedronsolver.h"

class RealPolyhedronWidget : public QWidget {
    Q_OBJECT

public:
    explicit RealPolyhedronWidget(QWidget *parent = nullptr);
    void setProblemData(const DynamicPolyhedronSolver& solver);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::unique_ptr<DynamicPolyhedronSolver> m_solver;
    bool m_hasData;

    void drawAxes(QPainter &painter, int width, int height, int margin,
                  double scale, int originX, int originY, double maxX, double maxY);
    void drawConstraints(QPainter &painter, int originX, int originY, double scale);
    void drawFeasibleRegion(QPainter &painter, int originX, int originY, double scale);
    void drawPoints(QPainter &painter, int originX, int originY, double scale);
    void drawObjectiveFunction(QPainter &painter, int originX, int originY, double scale,
                               int width, int height, int margin);
    void calculateGraphBounds(double &minX, double &maxX, double &minY, double &maxY);
};

#endif // REALPOLYHEDRONWIDGET_H
