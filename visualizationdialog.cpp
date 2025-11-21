#include "visualizationdialog.h"
#include "realpolyhedronwidget.h"
#include "dynamicpolyhedronsolver.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

VisualizationDialog::VisualizationDialog(QWidget *parent)
    : QDialog(parent), mainLayout(nullptr) {

    setWindowTitle("Visualisation Dynamique du Polyèdre");
    setMinimumSize(900, 750);
    setStyleSheet("QDialog { background-color: #f8f9fa; font-family: Arial, sans-serif; }");

    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
}

void VisualizationDialog::setSolutionData(const std::vector<double>& solution,
                                          const std::vector<double>& objectiveCoeffs,
                                          double objectiveValue,
                                          const QString& problemType) {

    // Clear existing layout
    QLayoutItem* item;
    while ((item = mainLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // Title
    QLabel *titleLabel = new QLabel(
        QString("📊 Visualisation Dynamique - %1 Z* = %2")
            .arg(problemType)
            .arg(objectiveValue, 0, 'f', 4)
        );
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: white; background: #3498db; padding: 15px; border-radius: 10px; text-align: center;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Create example constraints for visualization
    if (solution.size() >= 2 && objectiveCoeffs.size() >= 2) {
        std::vector<std::vector<double>> realConstraints = {
            {2, 1}, {1, 2}, {1, -1}, {-1, 1}, {3, 1}
        };
        std::vector<double> rhs = {8, 10, 2, 3, 12};
        std::vector<int> types = {1, 1, 1, 1, 1}; // All LEQ

        DynamicPolyhedronSolver polySolver(realConstraints, objectiveCoeffs, rhs, types);
        polySolver.solve();

        // Widget with real data
        RealPolyhedronWidget *graphWidget = new RealPolyhedronWidget();
        graphWidget->setProblemData(polySolver);
        mainLayout->addWidget(graphWidget);

        // Information label
        QLabel *infoLabel = new QLabel(
            QString("• %1 contraintes calculées • %2 points d'intersection • Solution unique (%3, %4)")
                .arg(realConstraints.size())
                .arg(polySolver.getIntersectionPoints().size())
                .arg(polySolver.getOptimalPoint().x, 0, 'f', 2)
                .arg(polySolver.getOptimalPoint().y, 0, 'f', 2)
            );
        infoLabel->setStyleSheet("background: #d5f4e6; padding: 10px; border-radius: 5px;");
        mainLayout->addWidget(infoLabel);

    } else {
        QLabel *errorLabel = new QLabel("❌ Données insuffisantes pour la visualisation 2D");
        errorLabel->setStyleSheet("font-size: 16px; color: #e74c3c; text-align: center; padding: 20px;");
        mainLayout->addWidget(errorLabel);
    }

    QPushButton *closeButton = new QPushButton("Fermer");
    closeButton->setStyleSheet("QPushButton { background: #e74c3c; color: white; padding: 10px; font-weight: bold; }");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeButton);
}
