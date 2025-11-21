#ifndef VISUALIZATIONDIALOG_H
#define VISUALIZATIONDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <vector>
#include <QString>
#include <memory>

// Remove the forward declarations and DynamicPolyhedronWidget definition from here
// They will be implemented in the .cpp file

class VisualizationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VisualizationDialog(QWidget *parent = nullptr);

    void setSolutionData(const std::vector<double>& solution,
                         const std::vector<double>& objectiveCoeffs,
                         double objectiveValue,
                         const QString& problemType);

private:
    QVBoxLayout *mainLayout;
};

#endif // VISUALIZATIONDIALOG_H
