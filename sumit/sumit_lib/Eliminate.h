#pragma once

#include "stdafx.h"
#include <cstdint>
#include <time.h>
#include "JJData.h"

#define INT64_PRECISION (int_fast64_t)(1 / FLOAT_PRECISION)
#define UNPICKING_PRECISION 10

class Eliminate {

public:
    Eliminate(const char* injjfilename);
    ~Eliminate(void);
    void WriteJJFile(const char* outfilename);
    void RemoveExcessSuppression();
    double getCost();

private:
    // Some fields are not used in elimination and have been commented out to reduce memory usage
    struct Cell {
//        CellID id;
        int_fast64_t nominal_value;
        int_fast64_t loss_of_information_weight;
        char status;
//        int_fast64_t lower_bound;
//        int_fast64_t upper_bound;
        int_fast64_t lower_protection_level;
        int_fast64_t upper_protection_level;
//        int_fast64_t sliding_protection_level;
        bool removable;
    };

    struct ConsistencyEquation {
        int_fast64_t RHS;
        CellIndex size_of_eqtn;
        CellIndex *cell_index;
        int *plus_or_minus;
    };

    struct CellBounds {
        int_fast64_t lower;
        int_fast64_t upper;
    };

    struct ConsolidatedEquation {
        int_fast64_t RHS;
        int size_of_eqtn;
        int *cell_number;
        int *plus_or_minus;
    };

    struct ProcessingEquation {
        int cellnumber;
        int_fast64_t RHS;
        int no_lower_adds;
        int *cells_lower_to_add;
        int no_lower_subs;
        int *cells_lower_to_sub;
        int no_upper_adds;
        int *cells_upper_to_add;
        int no_upper_subs;
        int *cells_upper_to_sub;
    };

    Cell *cells;
    ConsistencyEquation *consistency_eqtns;
    CellIndex *primaryCells;

    int_fast64_t grand_total;

    int no_consolidated_eqtns,num_BackupConsolidatedEquations;
    int no_processing_eqtns;
    int nPrimaryCells;

    struct CellBounds* cell_bounds, *savedCellBounds;

    struct ConsolidatedEquation *consolidated_eqtns, *saved_consolidated_equations;
    struct ProcessingEquation *processing_eqtns_lower;
    struct ProcessingEquation *processing_eqtns_upper;
    int biggestConsEqn;

    int initial_nsup_found_1;
    int initial_nsup_bounds_broken_1;

    int* OrderedCells;

    int number_of_secondary_cells_removed;

    JJData *jjData;

    time_t start_seconds;
    time_t stop_seconds;

    void allocate_cell_bounds_memory();
    void init_cell_bounds();
    void release_cell_bounds_memory();
    void release_Consolidated_Eqtns_memory(void) ;
    void release_SavedConsolidated_Eqtns_memory(void) ;
    void backup_Consolidated_Equations(void);
    void restoreConsolidated_EquationsFromBackup(void);
    void backupCellBounds(void);
    void restoreSavedCellBounds(void);
    bool allocateMemoryForProcessingEquations();
    void SetUpProcessingEquationsForCurrentConsolidatedEquations(void);
    void release_processing_eqtns_memory();
    void tidy_up_the_consolidated_equations();
    void consolidate_the_consistency_equations();
    int simplify_the_consistency_equations();
    bool set_initial_lower_and_upper_bounds();
    bool getSafety();
    bool improve_lower_and_upper_bounds();
    void RecordExistingExposure(int when);
    bool SafeToRemoveSecondaryCell();
    void allocate_ordered_cells_memory();
    void release_ordered_cells_memory();
    void init_ordered_cells();
    void eliminate_secondary_suppression();
    void display_secondary_suppression();
    int index(int x, int y);

};
