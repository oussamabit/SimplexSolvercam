#include "simplexsolver.h"

//
// SimplexSolver - Based on Algerian University Course
// Dr Mahmoud ZENNAKI - USTO Oran
// CORRECTION: Phase 2 now properly removes artificial variables
//
#include <iostream>
#include <vector>
#include <iomanip>
#include <limits>
#include <cmath>
#include <fstream>
#include <string>
#include <stdexcept>

using namespace std;


SimplexSolver::SimplexSolver(const vector<double>& fobj,
                             const vector<vector<double>>& contraintes,
                             const vector<double>& b,
                             const vector<TypeContrainte>& types,
                             TypeObjectif type,
                             const vector<TypeVariable>& typesVar)
    : fonctionObjectif(fobj), matriceContraintes(contraintes),
    Bi(b), typesContraintes(types), typeObj(type) {

    nbVariablesOriginales = fobj.size();
    nbContraintes = contraintes.size();
    etatSolution = EN_COURS;
    valeurObjectif = 0.0;

    // If no variable types provided, assume all are NON_NEGATIVE
    if (typesVar.empty()) {
        typesVariables.resize(nbVariablesOriginales, NON_NEGATIVE);
    } else {
        typesVariables = typesVar;
    }

    // Initialize solution vector (will be resized after preprocessing)
    solutionOptimale.resize(nbVariablesOriginales, 0.0);

    // NORMALISATION: S'assurer que tous les Bi >= 0
    for (int i = 0; i < nbContraintes; i++) {
        if (Bi[i] < -EPSILON) {
            for (int j = 0; j < nbVariablesOriginales; j++) {
                matriceContraintes[i][j] = -matriceContraintes[i][j];
            }
            Bi[i] = -Bi[i];

            if (typesContraintes[i] == LEQ) {
                typesContraintes[i] = GEQ;
            } else if (typesContraintes[i] == GEQ) {
                typesContraintes[i] = LEQ;
            }
        }
    }

    // Preprocess variables (handle s.r.s. and x <= 0)
    preprocessVariables();
}
void SimplexSolver::preprocessVariables() {
    // Map original variables to new variables
    variableMapping.clear();
    variableMapping.resize(nbVariablesOriginales);

    vector<double> newFonctionObjectif;
    vector<vector<double>> newMatriceContraintes(nbContraintes);

    int newVarIndex = 0;

    for (int j = 0; j < nbVariablesOriginales; j++) {
        variableMapping[j] = newVarIndex;

        if (typesVariables[j] == NON_NEGATIVE) {
            // x_j >= 0: Keep as is
            newFonctionObjectif.push_back(fonctionObjectif[j]);
            for (int i = 0; i < nbContraintes; i++) {
                newMatriceContraintes[i].push_back(matriceContraintes[i][j]);
            }
            newVarIndex++;

        } else if (typesVariables[j] == NON_POSITIVE) {
            // x_j <= 0: Replace with x_j = -x'_j where x'_j >= 0
            newFonctionObjectif.push_back(-fonctionObjectif[j]);
            for (int i = 0; i < nbContraintes; i++) {
                newMatriceContraintes[i].push_back(-matriceContraintes[i][j]);
            }
            newVarIndex++;

        } else { // UNRESTRICTED
            // x_j s.r.s.: Replace with x_j = x'_j - x''_j where x'_j, x''_j >= 0
            // Add x'_j
            newFonctionObjectif.push_back(fonctionObjectif[j]);
            for (int i = 0; i < nbContraintes; i++) {
                newMatriceContraintes[i].push_back(matriceContraintes[i][j]);
            }

            // Add x''_j
            newFonctionObjectif.push_back(-fonctionObjectif[j]);
            for (int i = 0; i < nbContraintes; i++) {
                newMatriceContraintes[i].push_back(-matriceContraintes[i][j]);
            }
            newVarIndex += 2;
        }
    }

    // Update problem data
    fonctionObjectif = newFonctionObjectif;
    matriceContraintes = newMatriceContraintes;
    nbVariablesDecision = newFonctionObjectif.size();
}


void SimplexSolver::ajouterVariablesSupplementaires() {
    int nbEcart = 0, nbExcedent = 0, nbArtif = 0;

    for (auto type : typesContraintes) {
        if (type == LEQ) nbEcart++;
        else if (type == GEQ) { nbExcedent++; nbArtif++; }
        else nbArtif++;
    }

    nbVariablesTotal = nbVariablesDecision + nbEcart + nbExcedent + nbArtif;

    nomsVariables.clear();

    // Variables de décision (including transformed ones)
    int varCount = 0;
    for (int j = 0; j < nbVariablesOriginales; j++) {
        if (typesVariables[j] == UNRESTRICTED) {
            nomsVariables.push_back("x" + to_string(j + 1) + "'");
            nomsVariables.push_back("x" + to_string(j + 1) + "''");
            varCount += 2;
        } else if (typesVariables[j] == NON_POSITIVE) {
            nomsVariables.push_back("-x" + to_string(j + 1));
            varCount++;
        } else {
            nomsVariables.push_back("x" + to_string(j + 1));
            varCount++;
        }
    }

    // Variables d'écart et d'excédent
    int idxVariable = 1;
    int idxArtif = 1;

    for (auto type : typesContraintes) {
        if (type == LEQ) {
            nomsVariables.push_back("t" + to_string(idxVariable++));
        } else if (type == GEQ) {
            nomsVariables.push_back("t" + to_string(idxVariable++));
            nomsVariables.push_back("w" + to_string(idxArtif++));
        } else { // EQ
            nomsVariables.push_back("w" + to_string(idxArtif++));
        }
    }
}

void SimplexSolver::initialiserTableau() {
    ajouterVariablesSupplementaires();

    tableau.resize(nbContraintes + 1);
    for (int i = 0; i <= nbContraintes; i++) {
        tableau[i].resize(nbVariablesTotal + 1, 0.0);
    }

    base.resize(nbContraintes);
    int colActuelle = nbVariablesDecision;
    nbVariablesArtificielles = 0;

    for (int i = 0; i < nbContraintes; i++) {
        for (int j = 0; j < nbVariablesDecision; j++) {
            tableau[i][j] = matriceContraintes[i][j];
        }

        if (typesContraintes[i] == LEQ) {
            tableau[i][colActuelle] = 1.0;
            base[i] = colActuelle;
            colActuelle++;
        } else if (typesContraintes[i] == GEQ) {
            tableau[i][colActuelle] = -1.0;
            colActuelle++;
            tableau[i][colActuelle] = 1.0;
            base[i] = colActuelle;
            nbVariablesArtificielles++;
            colActuelle++;
        } else {
            tableau[i][colActuelle] = 1.0;
            base[i] = colActuelle;
            nbVariablesArtificielles++;
            colActuelle++;
        }

        tableau[i][nbVariablesTotal] = Bi[i];
    }
}

void SimplexSolver::phase1() {
    if (nbVariablesArtificielles == 0) return;

    cout << "\n=== PHASE 1: Elimination des variables artificielles ===\n";
    cout << "Objectif: Min W = somme des variables artificielles\n";

    // Identify artificial variables
    vector<bool> estArtificielle(nbVariablesTotal, false);
    int colActuelle = nbVariablesDecision;

    for (int i = 0; i < nbContraintes; i++) {
        if (typesContraintes[i] == LEQ) {
            colActuelle++;
        } else if (typesContraintes[i] == GEQ) {
            colActuelle++;
            estArtificielle[colActuelle] = true;
            colActuelle++;
        } else { // EQ
            estArtificielle[colActuelle] = true;
            colActuelle++;
        }
    }

    // Initialize Phase 1 objective: MIN W = sum of artificial variables
    for (int j = 0; j < nbVariablesTotal; j++) {
        tableau[nbContraintes][j] = estArtificielle[j] ? 1.0 : 0.0;
    }
    tableau[nbContraintes][nbVariablesTotal] = 0.0;

    // Make artificial variables in base have coefficient 0
    for (int i = 0; i < nbContraintes; i++) {
        if (base[i] < nbVariablesTotal && estArtificielle[base[i]]) {
            for (int j = 0; j <= nbVariablesTotal; j++) {
                tableau[nbContraintes][j] -= tableau[i][j];
            }
        }
    }

    int iteration = 0;
    afficherTableau(iteration++, true);

    while (!estOptimal(true)) {
        int colPivot = trouverColonnePivot(true);
        if (colPivot == -1) break;

        if (estNonBorne(colPivot)) {
            etatSolution = INFAISABLE;
            return;
        }

        int lignePivot = trouverLignePivot(colPivot);

        cout << "\nIteration " << iteration << ": Variable entrante = "
             << nomsVariables[colPivot] << ", Variable sortante = "
             << nomsVariables[base[lignePivot]] << endl;

        pivoter(lignePivot, colPivot);
        base[lignePivot] = colPivot;

        afficherTableau(iteration++, true);
    }

    // ✅ فحص قوي للـ Infeasibility

    // الفحص 1: قيمة W يجب أن تكون صفر
    if (abs(tableau[nbContraintes][nbVariablesTotal]) > EPSILON) {
        cout << "\n*** Phase 1 ECHEC: W = "
             << tableau[nbContraintes][nbVariablesTotal]
             << " > 0 ***\n";
        cout << "Les contraintes sont incompatibles!\n";
        etatSolution = INFAISABLE;
        return;
    }

    // ✅ الفحص 2: لا يجب أن تبقى متغيرات صناعية في القاعدة بقيمة > 0
    for (int i = 0; i < nbContraintes; i++) {
        if (base[i] < nbVariablesTotal && estArtificielle[base[i]]) {
            double valeurBase = tableau[i][nbVariablesTotal];
            if (valeurBase > EPSILON) {
                cout << "\n*** Phase 1 ECHEC: Variable artificielle "
                     << nomsVariables[base[i]]
                     << " reste dans la base avec valeur = "
                     << valeurBase << " > 0 ***\n";
                cout << "Les contraintes sont incompatibles!\n";
                etatSolution = INFAISABLE;
                return;
            }
        }
    }

    cout << "\nPhase 1 terminee: W = 0, Solution realisable trouvee!\n";

    // ✅ الفحص 3 (اختياري): تحذير إذا كانت متغيرات صناعية في القاعدة بقيمة صفر
    bool warningArtificielles = false;
    for (int i = 0; i < nbContraintes; i++) {
        if (base[i] < nbVariablesTotal && estArtificielle[base[i]]) {
            if (!warningArtificielles) {
                cout << "\n⚠️  ATTENTION: Les variables artificielles suivantes restent dans la base (avec valeur 0):\n";
                warningArtificielles = true;
            }
            cout << "  - " << nomsVariables[base[i]] << " (ligne " << (i+1) << ")\n";
        }
    }
    if (warningArtificielles) {
        cout << "Cela peut indiquer une redondance dans les contraintes.\n";
    }
}

void SimplexSolver::phase2() {
    cout << "\n=== PHASE 2: Optimisation de la fonction objectif ===\n";

    // Identify artificial variables and remove them
    vector<bool> estArtificielle(nbVariablesTotal, false);
    int colActuelle = nbVariablesDecision;

    for (int i = 0; i < nbContraintes; i++) {
        if (typesContraintes[i] == LEQ) {
            colActuelle++;
        } else if (typesContraintes[i] == GEQ) {
            colActuelle++;
            estArtificielle[colActuelle] = true;
            colActuelle++;
        } else { // EQ
            estArtificielle[colActuelle] = true;
            colActuelle++;
        }
    }

    // Create new tableau without artificial variables
    vector<int> nouvelIndice(nbVariablesTotal, -1);
    int nouvelleCol = 0;
    for (int j = 0; j < nbVariablesTotal; j++) {
        if (!estArtificielle[j]) {
            nouvelIndice[j] = nouvelleCol++;
        }
    }

    int nbVariablesSansArtif = nouvelleCol;

    vector<vector<double>> nouveauTableau(nbContraintes + 1);
    for (int i = 0; i <= nbContraintes; i++) {
        nouveauTableau[i].resize(nbVariablesSansArtif + 1, 0.0);
    }

    // Copy constraint rows (without artificial columns)
    for (int i = 0; i < nbContraintes; i++) {
        for (int j = 0; j < nbVariablesTotal; j++) {
            if (!estArtificielle[j]) {
                nouveauTableau[i][nouvelIndice[j]] = tableau[i][j];
            }
        }
        nouveauTableau[i][nbVariablesSansArtif] = tableau[i][nbVariablesTotal];
    }

    // Update base indices
    for (int i = 0; i < nbContraintes; i++) {
        if (base[i] < nbVariablesTotal && !estArtificielle[base[i]]) {
            base[i] = nouvelIndice[base[i]];
        }
    }

    // Update variable names
    vector<string> nouveauxNoms;
    for (int j = 0; j < nbVariablesTotal; j++) {
        if (!estArtificielle[j]) {
            nouveauxNoms.push_back(nomsVariables[j]);
        }
    }
    nomsVariables = nouveauxNoms;
    tableau = nouveauTableau;
    nbVariablesTotal = nbVariablesSansArtif;

    // **CRITICAL FIX**: Initialize Phase 2 objective row correctly
    for (int j = 0; j <= nbVariablesTotal; j++) {
        tableau[nbContraintes][j] = 0.0;
    }

    // ✅ Store -c_j for both MAX and MIN after transformation
    // This creates a unified MIN problem: MIN (-Z) for MAX, MIN Z for MIN
    for (int j = 0; j < nbVariablesDecision; j++) {
        if (typeObj == MAX) {
            tableau[nbContraintes][j] = -fonctionObjectif[j];  // MIN (-Z)
        } else {
            tableau[nbContraintes][j] = fonctionObjectif[j];   // MIN Z
        }
    }

    // Adjust for basic variables using row operations
    for (int i = 0; i < nbContraintes; i++) {
        if (base[i] < nbVariablesDecision) {
            double coefBase;
            if (typeObj == MAX) {
                coefBase = -fonctionObjectif[base[i]];  // MIN (-Z)
            } else {
                coefBase = fonctionObjectif[base[i]];   // MIN Z
            }

            if (!estZero(coefBase)) {
                for (int j = 0; j <= nbVariablesTotal; j++) {
                    tableau[nbContraintes][j] -= coefBase * tableau[i][j];
                }
            }
        }
    }

    int iteration = 0;
    afficherTableau(iteration++);

    while (!estOptimal(false)) {
        int colPivot = trouverColonnePivot(false);
        if (colPivot == -1) break;

        if (estNonBorne(colPivot)) {
            etatSolution = NON_BORNE;
            return;
        }

        int lignePivot = trouverLignePivot(colPivot);

        cout << "\nIteration " << iteration << ": Variable entrante = "
             << nomsVariables[colPivot] << ", Variable sortante = "
             << nomsVariables[base[lignePivot]] << endl;

        pivoter(lignePivot, colPivot);
        base[lignePivot] = colPivot;

        afficherTableau(iteration++);
    }

    etatSolution = OPTIMALE;
}

bool SimplexSolver::estOptimal(bool isPhase1) {
    int nbColonnes = tableau[0].size() - 1;

    // ✅ Unified optimality test after transformation
    // For both Phase 1 (MIN W) and Phase 2 (MIN or transformed MAX -> MIN)
    // We check if all coefficients in objective row are non-negative
    for (int j = 0; j < nbColonnes; j++) {
        if (tableau[nbContraintes][j] < -EPSILON) {
            return false;
        }
    }
    return true;
}

bool SimplexSolver::estNonBorne(int colPivot) {
    for (int i = 0; i < nbContraintes; i++) {
        if (tableau[i][colPivot] > EPSILON) {
            return false;
        }
    }
    return true;
}

int SimplexSolver::trouverColonnePivot(bool isPhase1) {
    int colPivot = -1;
    int nbColonnes = tableau[0].size() - 1;

    // ✅ Unified pivot selection after transformation
    // For both Phase 1 and Phase 2 (after MAX->MIN conversion)
    // We look for the most negative coefficient
    double minVal = 0.0;
    for (int j = 0; j < nbColonnes; j++) {
        if (tableau[nbContraintes][j] < minVal - EPSILON) {
            minVal = tableau[nbContraintes][j];
            colPivot = j;
        }
    }

    return colPivot;
}

int SimplexSolver::trouverLignePivot(int colPivot) {
    int lignePivot = -1;
    double minRatio = numeric_limits<double>::max();

    int nbColonnes = tableau[0].size() - 1;
    for (int i = 0; i < nbContraintes; i++) {
        if (tableau[i][colPivot] > EPSILON) {
            double ratio = tableau[i][nbColonnes] / tableau[i][colPivot];
            if (ratio >= 0 && ratio < minRatio) {
                minRatio = ratio;
                lignePivot = i;
            }
        }
    }

    return lignePivot;
}

void SimplexSolver::pivoter(int lignePivot, int colPivot) {
    double pivot = tableau[lignePivot][colPivot];
    int nbColonnes = tableau[0].size();


    if (abs(pivot) < EPSILON) {
        throw runtime_error("ERREUR: Element pivot est proche de zero!");
    }

    for (int j = 0; j < nbColonnes; j++) {
        tableau[lignePivot][j] /= pivot;
    }

    for (int i = 0; i <= nbContraintes; i++) {
        if (i != lignePivot && !estZero(tableau[i][colPivot])) {
            double facteur = tableau[i][colPivot];
            for (int j = 0; j < nbColonnes; j++) {
                tableau[i][j] -= facteur * tableau[lignePivot][j];
            }
        }
    }
}

void SimplexSolver::extraireSolution() {
    int nbColonnes = tableau[0].size() - 1;

    // Get objective value
    double tableauValue = tableau[nbContraintes][nbColonnes];

    // ✅ Correct extraction for both MAX and MIN
    if (typeObj == MAX) {
        valeurObjectif = -tableauValue;  // Because we solved MIN (-Z)
    } else {
        valeurObjectif = tableauValue;   // Because we solved MIN Z
    }

    // Temporary solution for transformed variables
    vector<double> transformedSolution(nbVariablesDecision, 0.0);

    for (int i = 0; i < nbContraintes; i++) {
        if (base[i] < nbVariablesDecision) {
            transformedSolution[base[i]] = tableau[i][nbColonnes];
        }
    }

    // Map back to original variables
    solutionOptimale.resize(nbVariablesOriginales);

    int transformedIndex = 0;
    for (int j = 0; j < nbVariablesOriginales; j++) {
        if (typesVariables[j] == NON_NEGATIVE) {
            solutionOptimale[j] = transformedSolution[transformedIndex];
            transformedIndex++;
        } else if (typesVariables[j] == NON_POSITIVE) {
            solutionOptimale[j] = -transformedSolution[transformedIndex];
            transformedIndex++;
        } else { // UNRESTRICTED
            double xPrime = transformedSolution[transformedIndex];
            double xDoublePrime = transformedSolution[transformedIndex + 1];
            solutionOptimale[j] = xPrime - xDoublePrime;
            transformedIndex += 2;
        }
    }
}

void SimplexSolver::afficherFormeStandard() const {
    cout << "\n--- FORME STANDARD (apres transformation) ---\n\n";

    cout << (typeObj == MAX ? "Max" : "Min") << " Z = ";
    for (int i = 0; i < nbVariablesDecision; i++) {
        if (i > 0 && fonctionObjectif[i] >= 0) cout << " + ";
        else if (i > 0) cout << " ";

        cout << fonctionObjectif[i] << "*";

        // Generate variable name on-the-fly
        int varCount = 0;
        bool found = false;
        for (int j = 0; j < nbVariablesOriginales; j++) {
            if (typesVariables[j] == UNRESTRICTED) {
                if (i == varCount) {
                    cout << "x" << (j + 1) << "'";
                    found = true;
                    break;
                } else if (i == varCount + 1) {
                    cout << "x" << (j + 1) << "''";
                    found = true;
                    break;
                }
                varCount += 2;
            } else if (typesVariables[j] == NON_POSITIVE) {
                if (i == varCount) {
                    cout << "(-x" << (j + 1) << ")";
                    found = true;
                    break;
                }
                varCount++;
            } else { // NON_NEGATIVE
                if (i == varCount) {
                    cout << "x" << (j + 1);
                    found = true;
                    break;
                }
                varCount++;
            }
        }

        if (!found) {
            cout << "x" << (i + 1); // fallback
        }
    }

    int idxVariable = 1;
    for (auto type : typesContraintes) {
        if (type == LEQ || type == GEQ) {
            cout << " + 0*t" << idxVariable++;
        }
    }

    cout << "\n\nAvec:\n";

    idxVariable = 1;
    for (int i = 0; i < nbContraintes; i++) {
        cout << "  ";
        for (int j = 0; j < nbVariablesDecision; j++) {
            if (j > 0 && matriceContraintes[i][j] >= 0) cout << " + ";
            else if (j > 0) cout << " ";

            cout << matriceContraintes[i][j] << "*";

            // Generate variable name on-the-fly
            int varCount = 0;
            bool found = false;
            for (int k = 0; k < nbVariablesOriginales; k++) {
                if (typesVariables[k] == UNRESTRICTED) {
                    if (j == varCount) {
                        cout << "x" << (k + 1) << "'";
                        found = true;
                        break;
                    } else if (j == varCount + 1) {
                        cout << "x" << (k + 1) << "''";
                        found = true;
                        break;
                    }
                    varCount += 2;
                } else if (typesVariables[k] == NON_POSITIVE) {
                    if (j == varCount) {
                        cout << "(-x" << (k + 1) << ")";
                        found = true;
                        break;
                    }
                    varCount++;
                } else { // NON_NEGATIVE
                    if (j == varCount) {
                        cout << "x" << (k + 1);
                        found = true;
                        break;
                    }
                    varCount++;
                }
            }

            if (!found) {
                cout << "x" << (j + 1); // fallback
            }
        }

        if (typesContraintes[i] == LEQ) {
            cout << " + t" << idxVariable++ << " = " << Bi[i];
        } else if (typesContraintes[i] == GEQ) {
            cout << " - t" << idxVariable++ << " = " << Bi[i];
        } else {
            cout << " = " << Bi[i];
        }
        cout << endl;
    }

    cout << "\n  Toutes les variables transformees >= 0\n";
}

void SimplexSolver::afficherIntroductionVariablesArtificielles() const {
    if (nbVariablesArtificielles == 0) {
        cout << "\n--- PAS DE VARIABLES ARTIFICIELLES NECESSAIRES ---\n";
        cout << "Toutes les contraintes sont de type <=, donc pas besoin de Phase 1.\n";
        return;
    }

    cout << "\n--- INTRODUCTION DES VARIABLES ARTIFICIELLES ---\n\n";

    int idxVariable = 1, idxArtif = 1;
    for (int i = 0; i < nbContraintes; i++) {
        cout << "  ";
        for (int j = 0; j < nbVariablesDecision; j++) {
            if (j > 0 && matriceContraintes[i][j] >= 0) cout << " + ";
            else if (j > 0) cout << " ";

            cout << matriceContraintes[i][j] << "*";

            // Generate variable name
            int varCount = 0;
            bool found = false;
            for (int k = 0; k < nbVariablesOriginales; k++) {
                if (typesVariables[k] == UNRESTRICTED) {
                    if (j == varCount) {
                        cout << "x" << (k + 1) << "'";
                        found = true;
                        break;
                    } else if (j == varCount + 1) {
                        cout << "x" << (k + 1) << "''";
                        found = true;
                        break;
                    }
                    varCount += 2;
                } else if (typesVariables[k] == NON_POSITIVE) {
                    if (j == varCount) {
                        cout << "(-x" << (k + 1) << ")";
                        found = true;
                        break;
                    }
                    varCount++;
                } else {
                    if (j == varCount) {
                        cout << "x" << (k + 1);
                        found = true;
                        break;
                    }
                    varCount++;
                }
            }

            if (!found) cout << "x" << (j + 1);
        }

        if (typesContraintes[i] == LEQ) {
            cout << " + t" << idxVariable++ << " = " << Bi[i];
        } else if (typesContraintes[i] == GEQ) {
            cout << " - t" << idxVariable++ << " + w" << idxArtif++ << " = " << Bi[i];
        } else {
            cout << " + w" << idxArtif++ << " = " << Bi[i];
        }
        cout << endl;
    }

    // Print all variable names
    cout << "\n  ";
    int varCount = 0;
    for (int j = 0; j < nbVariablesOriginales; j++) {
        if (j > 0) cout << ", ";

        if (typesVariables[j] == UNRESTRICTED) {
            cout << "x" << (j + 1) << "', x" << (j + 1) << "''";
        } else if (typesVariables[j] == NON_POSITIVE) {
            cout << "(-x" << (j + 1) << ")";
        } else {
            cout << "x" << (j + 1);
        }
    }

    // Count slack variables
    int totalVariablesEcart = 0;
    for (auto type : typesContraintes) {
        if (type == LEQ || type == GEQ) totalVariablesEcart++;
    }

    if (totalVariablesEcart > 0) {
        for (int i = 1; i <= totalVariablesEcart; i++) {
            cout << ", t" << i;
        }
    }

    if (idxArtif > 1) {
        for (int i = 1; i < idxArtif; i++) {
            cout << ", w" << i;
        }
    }

    cout << " >= 0\n";
}

void SimplexSolver::solve() {
    cout << "\n" << string(80, '=') << endl;
    cout << "RESOLUTION PAR L'ALGORITHME DU SIMPLEXE" << endl;
    cout << string(80, '=') << "\n";

    afficherProbleme();
    afficherFormeStandard();
    initialiserTableau();
    afficherIntroductionVariablesArtificielles();

    if (nbVariablesArtificielles > 0) {
        phase1();
        if (etatSolution == INFAISABLE) {
            afficherSolution();
            return;
        }
    }

    phase2();
    extraireSolution();
    afficherSolution();
}

void SimplexSolver::afficherProbleme() const {
    cout << "\n--- PROBLEME DE PROGRAMMATION LINEAIRE ---\n\n";

    cout << (typeObj == MAX ? "Maximiser" : "Minimiser") << " Z = ";
    for (int i = 0; i < nbVariablesOriginales; i++) {
        if (i > 0 && fonctionObjectif[i] >= 0) cout << " + ";
        else if (i > 0) cout << " ";
        cout << fonctionObjectif[i] << "*x" << (i + 1);
    }
    cout << "\n\nSous les contraintes:\n";

    for (int i = 0; i < nbContraintes; i++) {
        cout << "  ";
        for (int j = 0; j < nbVariablesOriginales; j++) {
            if (j > 0 && matriceContraintes[i][j] >= 0) cout << " + ";
            else if (j > 0) cout << " ";
            cout << matriceContraintes[i][j] << "*x" << (j + 1);
        }

        if (typesContraintes[i] == LEQ) cout << " <= ";
        else if (typesContraintes[i] == GEQ) cout << " >= ";
        else cout << " = ";

        cout << Bi[i] << endl;
    }

    cout << "\n  Contraintes de signe:\n";
    for (int i = 0; i < nbVariablesOriginales; i++) {
        cout << "  x" << (i + 1) << " ";
        if (typesVariables[i] == NON_NEGATIVE) {
            cout << ">= 0";
        } else if (typesVariables[i] == NON_POSITIVE) {
            cout << "<= 0";
        } else {
            cout << "s.r.s. (sans restriction de signe)";
        }
        cout << endl;
    }
}

void SimplexSolver::afficherTableau(int iteration, bool isPhase1) const {
    string phaseLabel = isPhase1 ? " (Phase 1)" : " (Phase 2)";

    if (iteration == 0) {
        cout << "\n--- TABLEAU INITIAL" << phaseLabel << " ---\n";
    } else {
        cout << "\n--- Iteration " << iteration << phaseLabel << " ---\n";
    }

    cout << setw(8) << "Base" << " | ";

    int nbColonnes = tableau[0].size() - 1;

    for (int j = 0; j < nbColonnes; j++) {
        cout << setw(10) << nomsVariables[j];
    }
    cout << setw(10) << "b" << endl;
    cout << string(12 + 10 * (nbColonnes + 1), '-') << endl;

    for (int i = 0; i < nbContraintes; i++) {
        cout << setw(8) << nomsVariables[base[i]] << " | ";
        for (int j = 0; j < nbColonnes; j++) {
            cout << setw(10) << fixed << setprecision(3) << tableau[i][j];
        }
        cout << setw(10) << fixed << setprecision(3) << tableau[i][nbColonnes] << endl;
    }

    cout << string(12 + 10 * (nbColonnes + 1), '-') << endl;

    if (isPhase1) {
        cout << setw(8) << "W" << " | ";
    } else if (typeObj == MIN) {
        cout << setw(8) << "zj-cj" << " | ";
    } else {
        cout << setw(8) << "cj-zj" << " | ";
    }

    for (int j = 0; j < nbColonnes; j++) {
        cout << setw(10) << fixed << setprecision(3) << tableau[nbContraintes][j];
    }
    cout << setw(10) << fixed << setprecision(3) << tableau[nbContraintes][nbColonnes] << endl;
}

void SimplexSolver::afficherSolution() const {
    cout << "\n" << string(80, '=') << endl;
    cout << "SOLUTION FINALE" << endl;
    cout << string(80, '=') << "\n\n";

    if (etatSolution == OPTIMALE) {
        cout << "*** SOLUTION OPTIMALE ATTEINTE ***\n\n";

        cout << "Valeur optimale de Z = " << fixed << setprecision(4)
             << valeurObjectif << "\n\n";

        cout << "Variables de decision:\n";
        // FIX: Loop over ORIGINAL variables, not transformed ones!
        for (int i = 0; i < nbVariablesOriginales; i++) {
            cout << "  x" << (i + 1) << " = " << fixed << setprecision(4)
            << solutionOptimale[i] << endl;
        }
    } else if (etatSolution == NON_BORNE) {
        cout << "*** PROBLEME NON BORNE ***\n";
        cout << "La fonction objectif peut etre amelioree indefiniment.\n";
    } else if (etatSolution == INFAISABLE) {
        cout << "*** AUCUNE SOLUTION REALISABLE ***\n";
        cout << "Les contraintes sont incompatibles (W > 0 en Phase 1).\n";
    }

    cout << "\n" << string(80, '=') << endl;
}

