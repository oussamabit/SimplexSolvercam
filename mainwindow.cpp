#include "MainWindow.h"
#include "dualsimplexsolver.h"
#include "DynamicPolyhedronSolver.h"
#include "visualizationdialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QHeaderView>
#include <QScrollArea>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QPlainTextEdit>
#include <sstream>
#include <iostream>
#include <memory>



MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), currentVarCount(2), currentConstraintCount(2) {
    setWindowTitle("Résolveur Simplexe - USTOMB Oran");
    setWindowIcon(QIcon(":/Ressources/structure.ico"));

    // Initialize OCR processor
    m_ocrProcessor = new OcrProcessor(this);
    connect(m_ocrProcessor, &OcrProcessor::ocrCompleted, this, &MainWindow::onOcrCompleted);
    connect(m_ocrProcessor, &OcrProcessor::ocrError, this, &MainWindow::onOcrError);
    connect(m_ocrProcessor, &OcrProcessor::processingStarted, this, [this]() {
        outputText->setPlainText("Traitement OCR en cours...");
    });

    setupUI();
    setupMenuBar();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupMenuBar() {
    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // ======================
    // File Menu
    // ======================
    fileMenu = new QMenu("&Fichier", this);

    newAction = new QAction("&Nouveau Problème", this);
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::onNewProblem);

    saveAction = new QAction("&Enregistrer...", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveProblem);

    loadAction = new QAction("&Charger...", this);
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, this, &MainWindow::onLoadProblem);

    exitAction = new QAction("&Quitter", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExit);

    fileMenu->addAction(newAction);
    fileMenu->addSeparator();
    fileMenu->addAction(saveAction);
    fileMenu->addAction(loadAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    menuBar->addMenu(fileMenu);

    // ======================
    // OCR Menu (ADDED)
    // ======================
    QMenu *ocrMenu = new QMenu("&OCR", this);

    QAction *ocrCameraAction = new QAction("Depuis la &Caméra", this);
    ocrCameraAction->setShortcut(QKeySequence("Ctrl+C"));
    connect(ocrCameraAction, &QAction::triggered, this, &MainWindow::onOcrFromCamera);

    QAction *ocrFileAction = new QAction("Depuis un &Fichier", this);
    ocrFileAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(ocrFileAction, &QAction::triggered, this, &MainWindow::onOcrFromFile);

    ocrMenu->addAction(ocrCameraAction);
    ocrMenu->addAction(ocrFileAction);

    menuBar->addMenu(ocrMenu);

    // ======================
    // Help Menu
    // ======================
    helpMenu = new QMenu("&Aide", this);

    aboutAction = new QAction("À &propos", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);

    helpMenu->addAction(aboutAction);
    menuBar->addMenu(helpMenu);
}


void MainWindow::setupUI() {
    // Main widget
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Main splitter (Left | Right)
    mainSplitter = new QSplitter(Qt::Horizontal);

    // ========== LEFT PANEL ==========
    leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(10);

    // Title for left panel
    QLabel *titleLabel = new QLabel("<h2>Configuration du Problème</h2>");
    titleLabel->setStyleSheet("color: #667eea; padding: 10px;");
    leftLayout->addWidget(titleLabel);

    // 1. Problem dimensions group
    QGroupBox *dimensionsGroup = new QGroupBox("1. Dimensions du Problème");
    dimensionsGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #667eea; border-radius: 5px; margin-top: 10px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QVBoxLayout *dimLayout = new QVBoxLayout();

    QHBoxLayout *varLayout = new QHBoxLayout();
    varLayout->addWidget(new QLabel("Nombre de variables:"));
    varCountSpinBox = new QSpinBox();
    varCountSpinBox->setMinimum(1);
    varCountSpinBox->setMaximum(10);
    varCountSpinBox->setValue(2);
    varLayout->addWidget(varCountSpinBox);
    varLayout->addStretch();
    dimLayout->addLayout(varLayout);

    QHBoxLayout *constLayout = new QHBoxLayout();
    constLayout->addWidget(new QLabel("Nombre de contraintes:"));
    constraintCountSpinBox = new QSpinBox();
    constraintCountSpinBox->setMinimum(1);
    constraintCountSpinBox->setMaximum(10);
    constraintCountSpinBox->setValue(2);
    constLayout->addWidget(constraintCountSpinBox);
    constLayout->addStretch();
    dimLayout->addLayout(constLayout);

    QHBoxLayout *objTypeLayout = new QHBoxLayout();
    objTypeLayout->addWidget(new QLabel("Type d'objectif:"));
    objectiveTypeCombo = new QComboBox();
    objectiveTypeCombo->addItem("Maximiser");
    objectiveTypeCombo->addItem("Minimiser");
    objectiveTypeCombo->setCurrentIndex(1);
    objTypeLayout->addWidget(objectiveTypeCombo);
    objTypeLayout->addStretch();
    dimLayout->addLayout(objTypeLayout);

    dimensionsGroup->setLayout(dimLayout);
    leftLayout->addWidget(dimensionsGroup);

    // 2. Objective function group
    QGroupBox *objectiveGroup = new QGroupBox("2. Fonction Objectif");
    objectiveGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #667eea; border-radius: 5px; margin-top: 10px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QVBoxLayout *objLayout = new QVBoxLayout();

    objectiveTable = new QTableWidget();
    objectiveTable->setMaximumHeight(80);
    objLayout->addWidget(objectiveTable);
    objectiveGroup->setLayout(objLayout);
    leftLayout->addWidget(objectiveGroup);

    // 3. Variable types group
    QGroupBox *varTypeGroup = new QGroupBox("3. Types des Variables");
    varTypeGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #667eea; border-radius: 5px; margin-top: 10px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QVBoxLayout *varTypeLayout = new QVBoxLayout();

    variableTypeTable = new QTableWidget();
    variableTypeTable->setMaximumHeight(80);
    varTypeLayout->addWidget(variableTypeTable);
    varTypeGroup->setLayout(varTypeLayout);
    leftLayout->addWidget(varTypeGroup);

    // 4. Constraints group
    QGroupBox *constraintsGroup = new QGroupBox("4. Contraintes");
    constraintsGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #667eea; border-radius: 5px; margin-top: 10px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QVBoxLayout *constGroupLayout = new QVBoxLayout();

    constraintTable = new QTableWidget();
    constraintTable->setMinimumHeight(200);
    constGroupLayout->addWidget(constraintTable);
    constraintsGroup->setLayout(constGroupLayout);
    leftLayout->addWidget(constraintsGroup);

    leftLayout->addStretch();

    // ========== RIGHT PANEL ==========
    rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(10);

    // Title for right panel
    QLabel *rightTitleLabel = new QLabel("<h2>Résolution</h2>");
    rightTitleLabel->setStyleSheet("color: #667eea; padding: 10px;");
    rightLayout->addWidget(rightTitleLabel);

    // Buttons group
    QGroupBox *buttonsGroup = new QGroupBox("Actions");
    buttonsGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #4CAF50; border-radius: 5px; margin-top: 10px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QHBoxLayout *btnLayout = new QHBoxLayout();

    solveButton = new QPushButton("▶ Résoudre");
    solveButton->setMinimumHeight(50);
    solveButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; font-size: 16px; border-radius: 5px; } QPushButton:hover { background-color: #45a049; }");
    btnLayout->addWidget(solveButton);

    solveDualButton = new QPushButton("🔄 Résoudre (Simplexe Dual)");
    solveDualButton->setMinimumHeight(50);
    solveDualButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 16px; border-radius: 6px; } QPushButton:hover { background-color: #1976D2; }");
    btnLayout->addWidget(solveDualButton);

    visualizeButton = new QPushButton("📊 Visualiser la Solution");
    visualizeButton->setMinimumHeight(50);
    visualizeButton->setStyleSheet("QPushButton { background-color: #9C27B0; color: white; font-weight: bold; font-size: 12px; border-radius: 5px; } QPushButton:hover { background-color: #7B1FA2; }");
    visualizeButton->setEnabled(false); // Désactivé au début
    btnLayout->addWidget(visualizeButton);

    clearButton = new QPushButton("✖ Effacer");
    clearButton->setMinimumHeight(50);
    clearButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; font-size: 16px; border-radius: 5px; } QPushButton:hover { background-color: #da190b; }");
    btnLayout->addWidget(clearButton);

    // Add OCR buttons to the buttons group
    QPushButton *ocrCameraButton = new QPushButton("📷 OCR Caméra", this);
    ocrCameraButton->setMinimumHeight(40);
    ocrCameraButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; border-radius: 5px; } QPushButton:hover { background-color: #F57C00; }");
    btnLayout->addWidget(ocrCameraButton);

    QPushButton *ocrFileButton = new QPushButton("📁 OCR Fichier", this);
    ocrFileButton->setMinimumHeight(40);
    ocrFileButton->setStyleSheet("QPushButton { background-color: #795548; color: white; font-weight: bold; border-radius: 5px; } QPushButton:hover { background-color: #5D4037; }");
    btnLayout->addWidget(ocrFileButton);

    // Connect OCR buttons
    connect(ocrCameraButton, &QPushButton::clicked, this, &MainWindow::onOcrFromCamera);
    connect(ocrFileButton, &QPushButton::clicked, this, &MainWindow::onOcrFromFile);

    buttonsGroup->setLayout(btnLayout);
    rightLayout->addWidget(buttonsGroup);

    // Output group
    QGroupBox *outputGroup = new QGroupBox("Résultats");
    outputGroup->setStyleSheet("QGroupBox { font-weight: bold; border: 2px solid #2196F3; border-radius: 5px; margin-top: 10px; padding-top: 10px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QVBoxLayout *outputLayout = new QVBoxLayout();

    outputText = new QTextEdit();
    outputText->setReadOnly(true);
    outputText->setFont(QFont("Courier New", 14));
    outputText->setStyleSheet(
        "QTextEdit { "
        "background-color: #444444; "
        "border: 1px solid #ddd; "
        "color: #ffffff; "
        "font-weight: bold; "
        "}");

    outputText->setLineWrapMode(QTextEdit::NoWrap);                      // <-- désactive le wrapping
    outputText->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);    // ou AlwaysOn si tu veux toujours la voir
    outputText->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    outputLayout->addWidget(outputText);
    outputGroup->setLayout(outputLayout);
    rightLayout->addWidget(outputGroup, 1);


    outputLayout->addWidget(outputText);


    outputLayout->addWidget(outputText);
    outputGroup->setLayout(outputLayout);
    rightLayout->addWidget(outputGroup, 1);


    // Add panels to splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(mainSplitter);
    setCentralWidget(centralWidget);

    // Connect signals
    connect(varCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onVariableCountChanged);
    connect(constraintCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onConstraintCountChanged);
    connect(solveButton, &QPushButton::clicked, this, &MainWindow::onSolveClicked);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(objectiveTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onObjectiveTypeChanged);

    // Connecter le nouveau bouton
    connect(solveDualButton, &QPushButton::clicked, this, &MainWindow::onSolveDualClicked);
    connect(visualizeButton, &QPushButton::clicked, this, &MainWindow::onVisualizeClicked);

    // Initialize tables
    updateObjectiveTable();
    updateVariableTypeTable();
    updateConstraintTable();

    // Set window properties
    setWindowTitle("Résolveur Simplexe - USTOMB Oran");
    resize(1200, 700);
}

void MainWindow::updateObjectiveTable() {
    objectiveTable->clear();
    objectiveTable->setRowCount(1);
    objectiveTable->setColumnCount(currentVarCount);

    QStringList headers;
    for (int i = 0; i < currentVarCount; i++) {
        headers << QString("x%1").arg(i + 1);
        objectiveTable->setItem(0, i, new QTableWidgetItem("0"));
    }
    objectiveTable->setHorizontalHeaderLabels(headers);
    objectiveTable->setVerticalHeaderLabels(QStringList() << "Coeff.");
    objectiveTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::updateVariableTypeTable() {
    variableTypeTable->clear();
    variableTypeTable->setRowCount(1);
    variableTypeTable->setColumnCount(currentVarCount);

    QStringList headers;
    for (int i = 0; i < currentVarCount; i++) {
        headers << QString("x%1").arg(i + 1);
        QComboBox *combo = new QComboBox();
        combo->addItem("x ≥ 0");
        combo->addItem("x ≤ 0");
        combo->addItem("x s.r.s.");
        variableTypeTable->setCellWidget(0, i, combo);
    }
    variableTypeTable->setHorizontalHeaderLabels(headers);
    variableTypeTable->setVerticalHeaderLabels(QStringList() << "Type");
    variableTypeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::updateConstraintTable() {
    constraintTable->clear();
    constraintTable->setRowCount(currentConstraintCount);
    constraintTable->setColumnCount(currentVarCount + 2);

    QStringList headers;
    for (int i = 0; i < currentVarCount; i++) {
        headers << QString("x%1").arg(i + 1);
    }
    headers << "Type" << "b";
    constraintTable->setHorizontalHeaderLabels(headers);

    for (int i = 0; i < currentConstraintCount; i++) {
        for (int j = 0; j < currentVarCount; j++) {
            constraintTable->setItem(i, j, new QTableWidgetItem("0"));
        }

        QComboBox *typeCombo = new QComboBox();
        typeCombo->addItem("≥");
        typeCombo->addItem("≤");
        typeCombo->addItem("=");
        constraintTable->setCellWidget(i, currentVarCount, typeCombo);

        constraintTable->setItem(i, currentVarCount + 1, new QTableWidgetItem("0"));
    }

    constraintTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::onVariableCountChanged(int count) {
    currentVarCount = count;
    updateObjectiveTable();
    updateVariableTypeTable();
    updateConstraintTable();
}

void MainWindow::onConstraintCountChanged(int count) {
    currentConstraintCount = count;
    updateConstraintTable();
}

void MainWindow::onObjectiveTypeChanged(int index) {
    // Update if needed
}

void MainWindow::onSolveClicked() {
    try {
        // 1. Récupérer la fonction objectif
        std::vector<double> fobj;
        for (int i = 0; i < currentVarCount; i++) {
            bool ok;
            double val = objectiveTable->item(0, i)->text().toDouble(&ok);
            if (!ok) {
                QMessageBox::warning(this, "Erreur",
                                     QString("Coefficient invalide pour x%1 dans la fonction objectif").arg(i + 1));
                return;
            }
            fobj.push_back(val);
        }

        // 2. Récupérer les types de variables
        std::vector<TypeVariable> typesVar;
        for (int i = 0; i < currentVarCount; i++) {
            QComboBox *combo = qobject_cast<QComboBox*>(variableTypeTable->cellWidget(0, i));
            int idx = combo->currentIndex();
            if (idx == 0) typesVar.push_back(NON_NEGATIVE);
            else if (idx == 1) typesVar.push_back(NON_POSITIVE);
            else typesVar.push_back(UNRESTRICTED);
        }

        // 3. Récupérer les contraintes
        std::vector<std::vector<double>> contraintes;
        std::vector<double> Bi;
        std::vector<TypeContrainte> typesContraintes;
        std::vector<int> constraintTypes; // Pour le solveur polyèdre

        for (int i = 0; i < currentConstraintCount; i++) {
            std::vector<double> row;
            for (int j = 0; j < currentVarCount; j++) {
                bool ok;
                double val = constraintTable->item(i, j)->text().toDouble(&ok);
                if (!ok) {
                    QMessageBox::warning(this, "Erreur",
                                         QString("Coefficient invalide à la contrainte %1, variable x%2").arg(i + 1).arg(j + 1));
                    return;
                }
                row.push_back(val);
            }
            contraintes.push_back(row);

            QComboBox *typeCombo = qobject_cast<QComboBox*>(constraintTable->cellWidget(i, currentVarCount));
            int typeIdx = typeCombo->currentIndex();
            TypeContrainte type;
            if (typeIdx == 0) {
                type = GEQ;
                constraintTypes.push_back(0); // GEQ
            } else if (typeIdx == 1) {
                type = LEQ;
                constraintTypes.push_back(1); // LEQ
            } else {
                type = EQ;
                constraintTypes.push_back(2); // EQ
            }
            typesContraintes.push_back(type);

            bool ok;
            double b = constraintTable->item(i, currentVarCount + 1)->text().toDouble(&ok);
            if (!ok) {
                QMessageBox::warning(this, "Erreur",
                                     QString("Valeur RHS invalide à la contrainte %1").arg(i + 1));
                return;
            }
            Bi.push_back(b);
        }

        TypeObjectif typeObj = (objectiveTypeCombo->currentIndex() == 0) ? MAX : MIN;

        // Rediriger la sortie console
        std::stringstream buffer;
        std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

        // Résoudre avec SimplexSolver
        SimplexSolver solver(fobj, contraintes, Bi, typesContraintes, typeObj, typesVar);
        solver.solve();

        // Restaurer la sortie standard
        std::cout.rdbuf(old);

        // Afficher les résultats
        outputText->setPlainText(QString::fromStdString(buffer.str()));

        // SAUVEGARDER LES DONNÉES RÉELLES POUR LA VISUALISATION DYNAMIQUE
        currentObjectiveCoeffs = fobj;
        currentProblemType = (typeObj == MAX) ? "MAX" : "MIN";

        // Stocker les contraintes et types pour le solveur polyèdre
        currentConstraints = contraintes;
        currentBi = Bi;

        // Convertir TypeContrainte en int pour le solveur polyèdre
        currentConstraintTypes.clear();
        for (const auto& type : typesContraintes) {
            if (type == GEQ) currentConstraintTypes.push_back(0);
            else if (type == LEQ) currentConstraintTypes.push_back(1);
            else currentConstraintTypes.push_back(2); // EQ
        }

        // CALCULER LA SOLUTION RÉELLE POUR LA VISUALISATION
        if (currentVarCount >= 2) {
            // Utiliser le vrai solveur polyèdre avec les VRAIES contraintes
            DynamicPolyhedronSolver polySolver(contraintes, fobj, Bi, currentConstraintTypes);
            polySolver.solve();

            auto points = polySolver.getIntersectionPoints();
            auto optimalPoint = polySolver.getOptimalPoint();

            if (!points.empty()) {
                // Utiliser la VRAIE solution calculée
                currentSolution.clear();
                currentSolution.push_back(optimalPoint.x);
                currentSolution.push_back(optimalPoint.y);

                // Ajouter des zéros pour les variables supplémentaires si nécessaire
                for (int i = 2; i < currentVarCount; i++) {
                    currentSolution.push_back(0.0);
                }

                currentObjectiveValue = polySolver.getOptimalValue();

                outputText->append("\n🎯 GRAPHIQUE DYNAMIQUE CALCULÉ");
                outputText->append(QString("   • Points d'intersection trouvés: %1").arg(points.size()));
                outputText->append(QString("   • Solution optimale géométrique: (%1, %2)")
                                       .arg(optimalPoint.x, 0, 'f', 3)
                                       .arg(optimalPoint.y, 0, 'f', 3));
                outputText->append(QString("   • Valeur objectif: %1").arg(currentObjectiveValue, 0, 'f', 3));

            } else {
                // Fallback si aucun point trouvé
                outputText->append("\n⚠️  Région réalisable vide ou non bornée");
                outputText->append("   • Utilisation de valeurs par défaut pour la visualisation");
                generateFallbackSolution();
            }

        } else {
            outputText->append("\n⚠️  Visualisation 2D nécessite au moins 2 variables");
            generateFallbackSolution();
        }

        // Activer la visualisation
        visualizeButton->setEnabled(currentVarCount >= 2 && !currentSolution.empty());

    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Erreur",
                              QString("Une erreur s'est produite:\n%1").arg(e.what()));
        visualizeButton->setEnabled(false);
    }
}

// Méthode helper pour générer une solution de fallback
void MainWindow::generateFallbackSolution() {
    currentSolution.clear();
    currentObjectiveValue = 0.0;

    srand(static_cast<unsigned int>(time(nullptr)));
    for (int i = 0; i < currentVarCount; i++) {
        double value = (rand() % 50 + 10) / 10.0; // Valeurs entre 1.0 et 6.0
        currentSolution.push_back(value);
        if (i < currentObjectiveCoeffs.size()) {
            currentObjectiveValue += value * currentObjectiveCoeffs[i];
        }
    }
}

void MainWindow::onClearClicked() {
    for (int i = 0; i < objectiveTable->columnCount(); i++) {
        objectiveTable->item(0, i)->setText("0");
    }

    for (int i = 0; i < variableTypeTable->columnCount(); i++) {
        QComboBox *combo = qobject_cast<QComboBox*>(variableTypeTable->cellWidget(0, i));
        combo->setCurrentIndex(0);
    }

    for (int i = 0; i < constraintTable->rowCount(); i++) {
        for (int j = 0; j < currentVarCount; j++) {
            constraintTable->item(i, j)->setText("0");
        }
        QComboBox *typeCombo = qobject_cast<QComboBox*>(constraintTable->cellWidget(i, currentVarCount));
        typeCombo->setCurrentIndex(0);
        constraintTable->item(i, currentVarCount + 1)->setText("0");
    }

    outputText->clear();

    // Réinitialiser les données de visualisation
    currentSolution.clear();
    currentObjectiveCoeffs.clear();
    currentObjectiveValue = 0.0;
    visualizeButton->setEnabled(false);
}

// Menu actions
void MainWindow::onNewProblem() {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Nouveau Problème",
                                  "Voulez-vous créer un nouveau problème? Les données actuelles seront perdues.",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        onClearClicked();
        currentFilename.clear();
        setWindowTitle("Résolveur Simplexe - USTOMB Oran");
    }
}

void MainWindow::onSaveProblem() {
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Enregistrer le problème",
                                                    currentFilename.isEmpty() ? "probleme.json" : currentFilename,
                                                    "Fichiers JSON (*.json)");
    if (!filename.isEmpty()) {
        if (saveProblemToJson(filename)) {
            currentFilename = filename;
            setWindowTitle("Résolveur Simplexe - " + QFileInfo(filename).fileName());
            QMessageBox::information(this, "Succès", "Problème enregistré avec succès!");
        } else {
            QMessageBox::critical(this, "Erreur", "Impossible d'enregistrer le fichier!");
        }
    }
}

void MainWindow::onLoadProblem() {
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Charger un problème",
                                                    "",
                                                    "Fichiers JSON (*.json)");
    if (!filename.isEmpty()) {
        if (loadProblemFromJson(filename)) {
            currentFilename = filename;
            setWindowTitle("Résolveur Simplexe - " + QFileInfo(filename).fileName());
            QMessageBox::information(this, "Succès", "Problème chargé avec succès!");
        } else {
            QMessageBox::critical(this, "Erreur", "Impossible de charger le fichier!");
        }
    }
}

void MainWindow::onExit() {
    QApplication::quit();
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "À propos",
                       "<h2>Résolveur Simplexe</h2>"
                       "<p><b>Version:</b> 1.0</p>"
                       "<p><b>Auteur:</b> Guerguer Marouane et Tahkoubit Oussama Etudiant ING4 Specialité Intelligence Artificialle</p>"
                       "<p><b>Institution:</b> USTOMB Oran, Algérie</p>"
                       "<p>Interface graphique développée avec Qt pour "
                       "faciliter l'utilisation pédagogique de l'algorithme du Simplexe.</p>"
                       "<p><b>Fonctionnalités:</b></p>"
                       "<ul>"
                       "<li>Support des problèmes MAX/MIN</li>"
                       "<li>Contraintes mixtes (≤, ≥, =)</li>"
                       "<li>Variables avec restrictions de signe variées</li>"
                       "<li>Méthode du Simplexe à deux phases</li>"
                       "<li>Méthode du Dual Simplex</li>"
                       "<li>Enregistrement/Chargement de problèmes (JSON)</li>"
                       "</ul>");
}

bool MainWindow::saveProblemToJson(const QString &filename) {
    QJsonObject json;

    // Save dimensions
    json["nbVariables"] = currentVarCount;
    json["nbContraintes"] = currentConstraintCount;
    json["typeObjectif"] = objectiveTypeCombo->currentIndex();

    // Save objective function
    QJsonArray objArray;
    for (int i = 0; i < currentVarCount; i++) {
        objArray.append(objectiveTable->item(0, i)->text().toDouble());
    }
    json["fonctionObjectif"] = objArray;

    // Save variable types
    QJsonArray varTypesArray;
    for (int i = 0; i < currentVarCount; i++) {
        QComboBox *combo = qobject_cast<QComboBox*>(variableTypeTable->cellWidget(0, i));
        varTypesArray.append(combo->currentIndex());
    }
    json["typesVariables"] = varTypesArray;

    // Save constraints
    QJsonArray constraintsArray;
    for (int i = 0; i < currentConstraintCount; i++) {
        QJsonObject constraint;

        QJsonArray coeffs;
        for (int j = 0; j < currentVarCount; j++) {
            coeffs.append(constraintTable->item(i, j)->text().toDouble());
        }
        constraint["coefficients"] = coeffs;

        QComboBox *typeCombo = qobject_cast<QComboBox*>(constraintTable->cellWidget(i, currentVarCount));
        constraint["type"] = typeCombo->currentIndex();
        constraint["rhs"] = constraintTable->item(i, currentVarCount + 1)->text().toDouble();

        constraintsArray.append(constraint);
    }
    json["contraintes"] = constraintsArray;

    // Write to file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonDocument doc(json);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

bool MainWindow::loadProblemFromJson(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject json = doc.object();

    // Load dimensions
    int nbVars = json["nbVariables"].toInt();
    int nbConsts = json["nbContraintes"].toInt();

    varCountSpinBox->setValue(nbVars);
    constraintCountSpinBox->setValue(nbConsts);
    objectiveTypeCombo->setCurrentIndex(json["typeObjectif"].toInt());

    // Load objective function
    QJsonArray objArray = json["fonctionObjectif"].toArray();
    for (int i = 0; i < objArray.size() && i < currentVarCount; i++) {
        objectiveTable->item(0, i)->setText(QString::number(objArray[i].toDouble()));
    }

    // Load variable types
    QJsonArray varTypesArray = json["typesVariables"].toArray();
    for (int i = 0; i < varTypesArray.size() && i < currentVarCount; i++) {
        QComboBox *combo = qobject_cast<QComboBox*>(variableTypeTable->cellWidget(0, i));
        combo->setCurrentIndex(varTypesArray[i].toInt());
    }

    // Load constraints
    QJsonArray constraintsArray = json["contraintes"].toArray();
    for (int i = 0; i < constraintsArray.size() && i < currentConstraintCount; i++) {
        QJsonObject constraint = constraintsArray[i].toObject();

        QJsonArray coeffs = constraint["coefficients"].toArray();
        for (int j = 0; j < coeffs.size() && j < currentVarCount; j++) {
            constraintTable->item(i, j)->setText(QString::number(coeffs[j].toDouble()));
        }

        QComboBox *typeCombo = qobject_cast<QComboBox*>(constraintTable->cellWidget(i, currentVarCount));
        typeCombo->setCurrentIndex(constraint["type"].toInt());

        constraintTable->item(i, currentVarCount + 1)->setText(QString::number(constraint["rhs"].toDouble()));
    }

    return true;
}

//dual Simplex methodes
// Ajouter cette nouvelle méthode
void MainWindow::onSolveDualClicked() {
    try {
        std::vector<double> fobj;
        for (int i = 0; i < currentVarCount; i++) {
            bool ok;
            double val = objectiveTable->item(0, i)->text().toDouble(&ok);
            if (!ok) {
                QMessageBox::warning(this, "Erreur",
                                     QString("Coefficient invalide pour x%1 dans la fonction objectif").arg(i + 1));
                return;
            }
            fobj.push_back(val);
        }

        std::vector<TypeVariable> typesVar;
        for (int i = 0; i < currentVarCount; i++) {
            QComboBox *combo = qobject_cast<QComboBox*>(variableTypeTable->cellWidget(0, i));
            int idx = combo->currentIndex();
            if (idx == 0) typesVar.push_back(NON_NEGATIVE);
            else if (idx == 1) typesVar.push_back(NON_POSITIVE);
            else typesVar.push_back(UNRESTRICTED);
        }

        std::vector<std::vector<double>> contraintes;
        std::vector<double> Bi;
        std::vector<TypeContrainte> types;

        for (int i = 0; i < currentConstraintCount; i++) {
            std::vector<double> row;
            for (int j = 0; j < currentVarCount; j++) {
                bool ok;
                double val = constraintTable->item(i, j)->text().toDouble(&ok);
                if (!ok) {
                    QMessageBox::warning(this, "Erreur",
                                         QString("Coefficient invalide à la contrainte %1, variable x%2").arg(i + 1).arg(j + 1));
                    return;
                }
                row.push_back(val);
            }
            contraintes.push_back(row);

            QComboBox *typeCombo = qobject_cast<QComboBox*>(constraintTable->cellWidget(i, currentVarCount));
            int typeIdx = typeCombo->currentIndex();
            if (typeIdx == 0) types.push_back(GEQ);
            else if (typeIdx == 1) types.push_back(LEQ);
            else types.push_back(EQ);

            bool ok;
            double b = constraintTable->item(i, currentVarCount + 1)->text().toDouble(&ok);
            if (!ok) {
                QMessageBox::warning(this, "Erreur",
                                     QString("Valeur RHS invalide à la contrainte %1").arg(i + 1));
                return;
            }
            Bi.push_back(b);
        }

        TypeObjectif typeObj = (objectiveTypeCombo->currentIndex() == 0) ? MAX : MIN;

        std::stringstream buffer;
        std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

        // Utiliser DualSimplexSolver au lieu de SimplexSolver
        DualSimplexSolver solver(fobj, contraintes, Bi, types, typeObj, typesVar);
        solver.solve();

        std::cout.rdbuf(old);

        outputText->setPlainText(QString::fromStdString(buffer.str()));

    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Erreur",
                              QString("Une erreur s'est produite:\n%1").arg(e.what()));
    }
}

// Nouvelle méthode pour la visualisation

void MainWindow::onVisualizeClicked() {
    if (currentSolution.empty()) {
        QMessageBox::information(this, "Information",
                                 "Aucune solution disponible pour la visualisation.\nVeuillez d'abord résoudre le problème.");
        return;
    }

    // Use unique_ptr for automatic memory management
    auto dialog = std::make_unique<VisualizationDialog>(this);
    dialog->setSolutionData(currentSolution, currentObjectiveCoeffs,
                            currentObjectiveValue, currentProblemType);

    // Fermer la fenêtre principale
    this->hide();

    // Afficher la fenêtre de visualisation
    dialog->exec();

    // Revenir à la fenêtre principale quand la visualisation est fermée
    this->show();

    // unique_ptr will automatically delete the dialog when it goes out of scope
}

void MainWindow::onOcrFromCamera()
{
    CameraCapture *cameraDialog = new CameraCapture(this);
    connect(cameraDialog, &CameraCapture::imageCaptured, this, [this](const QImage &image) {
        m_ocrProcessor->processImage(image);
    });
    cameraDialog->exec();
    cameraDialog->deleteLater();
}

void MainWindow::onOcrFromFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Charger une image de problème", "", "Images (*.png *.jpg *.jpeg *.bmp)");

    if (!fileName.isEmpty()) {
        m_ocrProcessor->processImageFromFile(fileName);
    }
}

void MainWindow::onOcrCompleted(const QJsonObject &problemData)
{
    // Load the parsed problem into the UI
    if (loadProblemFromJsonObject(problemData)) {
        outputText->setPlainText("✅ Problème chargé avec succès depuis l'OCR!\n\n"
                                 "Veuillez vérifier les données et cliquer sur 'Résoudre'.");
        QMessageBox::information(this, "Succès", "Problème extrait avec succès de l'image!");
    } else {
        outputText->setPlainText("❌ Erreur lors du chargement du problème OCR.");
    }
}

void MainWindow::onOcrError(const QString &errorMessage)
{
    outputText->setPlainText("❌ Erreur OCR: " + errorMessage);
    QMessageBox::warning(this, "Erreur OCR", errorMessage);
}

// Helper method to load from JSON object
bool MainWindow::loadProblemFromJsonObject(const QJsonObject &json)
{
    if (!json.contains("nbVariables") || !json.contains("nbContraintes")) {
        return false;
    }

    // Load dimensions
    int nbVars = json["nbVariables"].toInt();
    int nbConsts = json["nbContraintes"].toInt();

    varCountSpinBox->setValue(nbVars);
    constraintCountSpinBox->setValue(nbConsts);

    if (json.contains("typeObjectif")) {
        objectiveTypeCombo->setCurrentIndex(json["typeObjectif"].toInt());
    }

    // Load objective function
    if (json.contains("fonctionObjectif")) {
        QJsonArray objArray = json["fonctionObjectif"].toArray();
        for (int i = 0; i < objArray.size() && i < currentVarCount; i++) {
            if (objectiveTable->item(0, i)) {
                objectiveTable->item(0, i)->setText(QString::number(objArray[i].toDouble()));
            }
        }
    }

    // Load variable types
    if (json.contains("typesVariables")) {
        QJsonArray varTypesArray = json["typesVariables"].toArray();
        for (int i = 0; i < varTypesArray.size() && i < currentVarCount; i++) {
            QComboBox *combo = qobject_cast<QComboBox*>(variableTypeTable->cellWidget(0, i));
            if (combo) {
                combo->setCurrentIndex(varTypesArray[i].toInt());
            }
        }
    }

    // Load constraints
    if (json.contains("contraintes")) {
        QJsonArray constraintsArray = json["contraintes"].toArray();
        for (int i = 0; i < constraintsArray.size() && i < currentConstraintCount; i++) {
            QJsonObject constraint = constraintsArray[i].toObject();

            if (constraint.contains("coefficients")) {
                QJsonArray coeffs = constraint["coefficients"].toArray();
                for (int j = 0; j < coeffs.size() && j < currentVarCount; j++) {
                    if (constraintTable->item(i, j)) {
                        constraintTable->item(i, j)->setText(QString::number(coeffs[j].toDouble()));
                    }
                }
            }

            QComboBox *typeCombo = qobject_cast<QComboBox*>(constraintTable->cellWidget(i, currentVarCount));
            if (typeCombo && constraint.contains("type")) {
                typeCombo->setCurrentIndex(constraint["type"].toInt());
            }

            if (constraintTable->item(i, currentVarCount + 1) && constraint.contains("rhs")) {
                constraintTable->item(i, currentVarCount + 1)->setText(QString::number(constraint["rhs"].toDouble()));
            }
        }
    }

    return true;
}
