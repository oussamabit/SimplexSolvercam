#ifndef DUALSIMPLEXSOLVER_H
#define DUALSIMPLEXSOLVER_H

#include "simplexsolver.h"
#include <vector>
#include <string>

class DualSimplexSolver {
private:
    // Problème primal original
    std::vector<double> fonctionObjectifPrimal;
    std::vector<std::vector<double>> matriceContraintesPrimal;
    std::vector<double> BiPrimal;
    std::vector<TypeContrainte> typesContraintesPrimal;
    TypeObjectif typeObjPrimal;
    std::vector<TypeVariable> typesVariablesPrimal;

    // Problème dual
    std::vector<double> fonctionObjectifDual;
    std::vector<std::vector<double>> matriceContraintesDual;
    std::vector<double> BiDual;
    std::vector<TypeContrainte> typesContraintesDual;
    TypeObjectif typeObjDual;
    std::vector<TypeVariable> typesVariablesDual;

    // Résultats
    std::vector<double> solutionPrimal;
    std::vector<double> solutionDual;
    double valeurObjectifPrimal;
    double valeurObjectifDual;
    TypeSolution etatSolution;

    const double EPSILON = 1e-10;

    void transformerPrimalVersDual();
    void resoudreDualAvecSimplex();
    void extraireSolutionPrimalDepuisDual();

public:
    DualSimplexSolver(const std::vector<double>& fobj,
                      const std::vector<std::vector<double>>& contraintes,
                      const std::vector<double>& b,
                      const std::vector<TypeContrainte>& types,
                      TypeObjectif type,
                      const std::vector<TypeVariable>& typesVar = std::vector<TypeVariable>());

    void solve();
    void afficherProblemeDual() const;
    void afficherResultatsComplets() const;

    // Getters pour l'interface
    std::vector<double> getSolutionPrimal() const { return solutionPrimal; }
    std::vector<double> getSolutionDual() const { return solutionDual; }
    double getValeurObjectifPrimal() const { return valeurObjectifPrimal; }
    double getValeurObjectifDual() const { return valeurObjectifDual; }
    TypeSolution getEtatSolution() const { return etatSolution; }
};

#endif // DUALSIMPLEXSOLVER_H
