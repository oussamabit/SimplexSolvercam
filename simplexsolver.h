#ifndef SIMPLEXSOLVER_H
#define SIMPLEXSOLVER_H

#include <vector>
#include <string>

enum TypeObjectif { MAX, MIN };
enum TypeContrainte { LEQ, GEQ, EQ };
enum TypeSolution { OPTIMALE, NON_BORNE, INFAISABLE, EN_COURS };
enum TypeVariable {
    NON_NEGATIVE,  // x â‰¥ 0
    NON_POSITIVE,  // x â‰¤ 0
    UNRESTRICTED   // x sans restriction (s.r.s.)
};

class SimplexSolver {
private:
    std::vector<double> fonctionObjectif;
    std::vector<std::vector<double>> matriceContraintes;
    std::vector<double> Bi;
    std::vector<TypeContrainte> typesContraintes;
    TypeObjectif typeObj;

    std::vector<TypeVariable> typesVariables;
    std::vector<int> variableMapping;
    int nbVariablesOriginales;

    std::vector<std::vector<double>> tableau;
    std::vector<int> base;
    std::vector<std::string> nomsVariables;

    int nbVariablesDecision;
    int nbContraintes;
    int nbVariablesTotal;
    int nbVariablesArtificielles;

    TypeSolution etatSolution;
    double valeurObjectif;
    std::vector<double> solutionOptimale;

    const double EPSILON = 1e-10;

    void preprocessVariables();
    void initialiserTableau();
    void ajouterVariablesSupplementaires();
    void phase1();
    void phase2();
    bool estOptimal(bool isPhase1);
    bool estNonBorne(int colPivot);
    int trouverColonnePivot(bool isPhase1);
    int trouverLignePivot(int colPivot);
    void pivoter(int lignePivot, int colPivot);
    void extraireSolution();

    void afficherFormeStandard() const;
    void afficherIntroductionVariablesArtificielles() const;

    bool estZero(double val) const { return std::abs(val) < EPSILON; }

public:
    SimplexSolver(const std::vector<double>& fobj,
                  const std::vector<std::vector<double>>& contraintes,
                  const std::vector<double>& b,
                  const std::vector<TypeContrainte>& types,
                  TypeObjectif type,
                  const std::vector<TypeVariable>& typesVar = std::vector<TypeVariable>());

    void solve();
    void afficherProbleme() const;
    void afficherTableau(int iteration, bool isPhase1 = false) const;
    void afficherSolution() const;
};

#endif // SIMPLEXSOLVER_H
