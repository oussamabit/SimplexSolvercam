#include "dualsimplexsolver.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

DualSimplexSolver::DualSimplexSolver(const std::vector<double>& fobj,
                                     const std::vector<std::vector<double>>& contraintes,
                                     const std::vector<double>& b,
                                     const std::vector<TypeContrainte>& types,
                                     TypeObjectif type,
                                     const std::vector<TypeVariable>& typesVar) {
    // Sauvegarder le problème primal
    fonctionObjectifPrimal = fobj;
    matriceContraintesPrimal = contraintes;
    BiPrimal = b;
    typesContraintesPrimal = types;
    typeObjPrimal = type;
    typesVariablesPrimal = typesVar.empty() ?
                               std::vector<TypeVariable>(fobj.size(), NON_NEGATIVE) : typesVar;

    etatSolution = EN_COURS;
    valeurObjectifPrimal = 0.0;
    valeurObjectifDual = 0.0;
}

void DualSimplexSolver::transformerPrimalVersDual() {
    int m = matriceContraintesPrimal.size(); // nombre de contraintes primal
    int n = fonctionObjectifPrimal.size();   // nombre de variables primal

    cout << "\n=== TRANSFORMATION PRIMAL -> DUAL ===" << endl;

    // 1. Type d'objectif dual
    typeObjDual = (typeObjPrimal == MAX) ? MIN : MAX;
    cout << "Objectif primal: " << (typeObjPrimal == MAX ? "MAX" : "MIN")
         << " → Objectif dual: " << (typeObjDual == MAX ? "MAX" : "MIN") << endl;

    // 2. Fonction objectif dual = termes de droite primal
    fonctionObjectifDual = BiPrimal;
    cout << "Fonction objectif dual: coefficients = Bi du primal" << endl;

    // 3. Matrice des contraintes dual = transposée de la matrice primal
    matriceContraintesDual.resize(n, std::vector<double>(m));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            matriceContraintesDual[j][i] = matriceContraintesPrimal[i][j];
        }
    }
    cout << "Matrice des contraintes dual: transposée de la matrice primal" << endl;

    // 4. Termes de droite dual = fonction objectif primal
    BiDual = fonctionObjectifPrimal;
    cout << "Termes de droite dual: coefficients de la fonction objectif primal" << endl;

    // 5. Types de contraintes dual (basé sur les types de variables primal)
    typesContraintesDual.resize(n);
    for (int j = 0; j < n; j++) {
        if (typesVariablesPrimal[j] == NON_NEGATIVE) {
            typesContraintesDual[j] = (typeObjPrimal == MAX) ? GEQ : LEQ;
        } else if (typesVariablesPrimal[j] == NON_POSITIVE) {
            typesContraintesDual[j] = (typeObjPrimal == MAX) ? LEQ : GEQ;
        } else { // UNRESTRICTED
            typesContraintesDual[j] = EQ;
        }
    }
    cout << "Types de contraintes dual: basés sur les types de variables primal" << endl;

    // 6. Types de variables dual (basé sur les types de contraintes primal)
    typesVariablesDual.resize(m);
    for (int i = 0; i < m; i++) {
        if (typesContraintesPrimal[i] == LEQ) {
            typesVariablesDual[i] = (typeObjPrimal == MAX) ? NON_NEGATIVE : NON_POSITIVE;
        } else if (typesContraintesPrimal[i] == GEQ) {
            typesVariablesDual[i] = (typeObjPrimal == MAX) ? NON_POSITIVE : NON_NEGATIVE;
        } else { // EQ
            typesVariablesDual[i] = UNRESTRICTED;
        }
    }
    cout << "Types de variables dual: basés sur les types de contraintes primal" << endl;
}

void DualSimplexSolver::resoudreDualAvecSimplex() {
    cout << "\n=== RESOLUTION DU PROBLEME DUAL PAR SIMPLEXE ===" << endl;

    // Créer et résoudre le problème dual avec SimplexSolver
    SimplexSolver solverDual(fonctionObjectifDual, matriceContraintesDual,
                             BiDual, typesContraintesDual, typeObjDual, typesVariablesDual);

    solverDual.solve();

    // Extraire la solution duale
    // Note: Nous n'avons pas accès direct aux solutions depuis SimplexSolver,
    // donc nous devons capturer la sortie ou modifier SimplexSolver
    // Pour l'instant, nous allons utiliser une approche simplifiée

    // Cette partie nécessiterait d'étendre SimplexSolver pour exposer sa solution
    // Pour la démonstration, nous allons résoudre et afficher les résultats
}

void DualSimplexSolver::extraireSolutionPrimalDepuisDual() {
    // En théorie, la solution du dual donne les variables duales
    // qui correspondent aux variables d'écart du primal, et vice versa
    // Cette implémentation est simplifiée pour la démonstration

    cout << "\n=== EXTRACTION DE LA SOLUTION PRIMALE DEPUIS LE DUAL ===" << endl;
    cout << "Les variables duales optimales donnent la solution primale." << endl;
    cout << "Les coûts réduits du dual donnent les variables d'écart du primal." << endl;
}

void DualSimplexSolver::solve() {
    cout << "\n" << string(80, '=') << endl;
    cout << "RESOLUTION PAR LA METHODE DU SIMPLEXE DUAL" << endl;
    cout << string(80, '=') << "\n";

    // Afficher le problème primal
    cout << "\n--- PROBLEME PRIMAL ---" << endl;
    SimplexSolver solverPrimal(fonctionObjectifPrimal, matriceContraintesPrimal,
                               BiPrimal, typesContraintesPrimal, typeObjPrimal, typesVariablesPrimal);
    solverPrimal.afficherProbleme();

    // Transformer en dual
    transformerPrimalVersDual();

    // Afficher le problème dual
    afficherProblemeDual();

    // Résoudre le dual
    resoudreDualAvecSimplex();

    // Extraire la solution primale
    extraireSolutionPrimalDepuisDual();

    // Afficher les résultats complets
    afficherResultatsComplets();
}

void DualSimplexSolver::afficherProblemeDual() const {
    cout << "\n--- PROBLEME DUAL ---\n\n";

    cout << (typeObjDual == MAX ? "Maximiser" : "Minimiser") << " W = ";
    for (int i = 0; i < fonctionObjectifDual.size(); i++) {
        if (i > 0 && fonctionObjectifDual[i] >= 0) cout << " + ";
        else if (i > 0) cout << " ";
        cout << fonctionObjectifDual[i] << "*y" << (i + 1);
    }
    cout << "\n\nSous les contraintes:\n";

    for (int i = 0; i < matriceContraintesDual.size(); i++) {
        cout << "  ";
        for (int j = 0; j < matriceContraintesDual[i].size(); j++) {
            if (j > 0 && matriceContraintesDual[i][j] >= 0) cout << " + ";
            else if (j > 0) cout << " ";
            cout << matriceContraintesDual[i][j] << "*y" << (j + 1);
        }

        if (typesContraintesDual[i] == LEQ) cout << " <= ";
        else if (typesContraintesDual[i] == GEQ) cout << " >= ";
        else cout << " = ";

        cout << BiDual[i] << endl;
    }

    cout << "\n  Contraintes de signe:\n";
    for (int i = 0; i < typesVariablesDual.size(); i++) {
        cout << "  y" << (i + 1) << " ";
        if (typesVariablesDual[i] == NON_NEGATIVE) {
            cout << ">= 0";
        } else if (typesVariablesDual[i] == NON_POSITIVE) {
            cout << "<= 0";
        } else {
            cout << "s.r.s. (sans restriction de signe)";
        }
        cout << endl;
    }
}

void DualSimplexSolver::afficherResultatsComplets() const {
    cout << "\n" << string(80, '=') << endl;
    cout << "RESULTATS COMPLETS PRIMAL-DUAL" << endl;
    cout << string(80, '=') << "\n\n";

    cout << "*** THEOREME DE DUALITE FORTE ***\n";
    cout << "Si les deux problèmes ont des solutions optimales, alors:\n";
    cout << "Valeur optimale du primal = Valeur optimale du dual\n\n";

    cout << "*** INTERPRETATION ECONOMIQUE ***\n";
    cout << "Les variables duales y_i représentent les 'prix ombres' ou\n";
    cout << "coûts marginaux des contraintes du problème primal.\n";
    cout << "Elles indiquent de combien la fonction objectif s'améliorerait\n";
    cout << "si la contrainte correspondante était relâchée d'une unité.\n";

    cout << "\n" << string(80, '-') << endl;
}
