#pragma once

#include "JJData.h"

class Unpicker
{

  public:
    Unpicker(const char *injjfilename);

    ~Unpicker(void);

    void
    Attack();

    int
    GetNumberOfCells();

    int
    GetNumberOfPrimaryCells();

    int
    GetNumberOfSecondaryCells();

    int
    GetNumberOfPrimaryCells_ValueKnownExactly();

    int
    GetNumberOfPrimaryCells_ValueKnownWithinProtection();

    void
    print_exact_exposure(const char *filename);

    void
    print_partial_exposure(const char *filename);

  private:
    struct CellBounds {
        double lower;
        double upper;
    };

    struct ConsolidatedEquation {
        double RHS;
        int size_of_eqtn;
        CellIndex *cell_index;
        int *plus_or_minus;
    };

    struct ProcessingEquation {
        int cellnumber;
        double RHS;
        int no_lower_adds;
        int *cells_lower_to_add;
        int no_lower_subs;
        int *cells_lower_to_sub;
        int no_upper_adds;
        int *cells_upper_to_add;
        int no_upper_subs;
        int *cells_upper_to_sub;
    };

    struct CellBounds *cell_bounds;
    struct ConsolidatedEquation *consolidated_eqtns;
    struct ProcessingEquation *processing_eqtns_lower;
    struct ProcessingEquation *processing_eqtns_upper;

    int no_consolidated_eqtns;
    int nos_processing_eqtns_lower;
    int nos_processing_eqtns_upper;

    int NumberOfPrimaryCells;
    int NumberOfSecondaryCells;
    int NumberOfPrimaryCells_ValueKnownExactly;
    int NumberOfPrimaryCells_ValueKnownWithinProtection;

    JJData *jjData;

    void
    tidy_up_the_consolidated_equations();

    void
    consolidate_the_consistency_equations();

    int
    simplify_the_consistency_equations();

    bool
    set_initial_lower_and_upper_bounds();

    void
    evaluate_exposure();

    void
    release_processing_eqtns_memory();

    bool
    improve_lower_and_upper_bounds();
};
