#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QDialog>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <vector>
#include <string>
#include "simplexsolver.h"
#include "dualsimplexsolver.h"
#include "ocrprocessor.h"
#include "cameracapture.h"

// Forward declaration only
class VisualizationDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onVariableCountChanged(int count);
    void onConstraintCountChanged(int count);
    void onSolveClicked();
    void onClearClicked();
    void onObjectiveTypeChanged(int index);
    void onSolveDualClicked();
    void onVisualizeClicked();
    void generateFallbackSolution();

    // Menu actions
    void onNewProblem();
    void onSaveProblem();
    void onLoadProblem();
    void onExit();
    void onAbout();

    // OCR slots
    void onOcrFromCamera();
    void onOcrFromFile();
    void onOcrCompleted(const QJsonObject &problemData);
    void onOcrError(const QString &errorMessage);

private:
    void setupUI();
    void setupMenuBar();
    void updateConstraintTable();
    void updateObjectiveTable();
    void updateVariableTypeTable();

    bool saveProblemToJson(const QString &filename);
    bool loadProblemFromJson(const QString &filename);
    bool loadProblemFromJsonObject(const QJsonObject &json);

    // Menu bar
    QMenuBar *menuBar;
    QMenu *fileMenu;
    QMenu *helpMenu;
    QMenu *ocrMenu;
    QAction *newAction;
    QAction *saveAction;
    QAction *loadAction;
    QAction *exitAction;
    QAction *aboutAction;
    QAction *ocrCameraAction;
    QAction *ocrFileAction;

    // Splitter for left/right panels
    QSplitter *mainSplitter;

    // Left panel widgets
    QWidget *leftPanel;
    QSpinBox *varCountSpinBox;
    QSpinBox *constraintCountSpinBox;
    QComboBox *objectiveTypeCombo;
    QTableWidget *objectiveTable;
    QTableWidget *constraintTable;
    QTableWidget *variableTypeTable;

    // Right panel widgets
    QWidget *rightPanel;
    QPushButton *solveButton;
    QPushButton *solveDualButton;
    QPushButton *visualizeButton;
    QPushButton *clearButton;
    QPushButton *ocrCameraButton;
    QPushButton *ocrFileButton;
    QTextEdit *outputText;

    // OCR processor
    OcrProcessor *m_ocrProcessor;

    // Données de solution pour visualisation
    std::vector<double> currentSolution;
    std::vector<double> currentObjectiveCoeffs;
    double currentObjectiveValue;
    QString currentProblemType;

    // Nouvelles variables pour le solveur polyèdre
    std::vector<std::vector<double>> currentConstraints;
    std::vector<double> currentBi;
    std::vector<int> currentConstraintTypes;

    // Current dimensions
    int currentVarCount;
    int currentConstraintCount;

    QString currentFilename;
};

#endif // MAINWINDOW_H
