#pragma once

#include "stdafx.h"
#include <map>
#include <limits.h>

#define MAXCELLINDEX = INT_MAX;
typedef int CellIndex;
typedef int SumIndex;
typedef int CellID;
#define MARGINAL_EQUALITY_TOLERANCE 0.01

class JJData {

public:
    
    struct Cell {
        CellID id;
        double nominal_value;
        double loss_of_information_weight;
        char status;
        double lower_bound;
        double upper_bound;
        double lower_protection_level;
        double upper_protection_level;
        double sliding_protection_level;
        int level;
    };

    struct ConsistencyEquation {
        double RHS;
        CellIndex size_of_eqtn;
        CellIndex marginal_index;
        CellIndex* cell_index;
        int* plus_or_minus;
    };

    char* name;
    CellIndex ncells;
    SumIndex nsums;
    int nlevels;
    int nprotected;
    CellIndex max_eqn_size;

    Cell* cells;
    ConsistencyEquation* consistency_eqtns;

    JJData(const char* filename);
    
    // Create a partitioned JJ file from a parent table and a list of cells
    // This constructor takes care of generating marginals and consistency equations for the partition and removes zero cells
    // partition_cells is the set of parent cells to include in the partition.  Cells may appear in any order.  Any marginals present are ignored.
    // partition_size is the size of the set of partition_cells
    // partition_id is a one-based identifier used to identify the partition in error and log messages
    JJData(JJData* parent, CellID* partition_cells, int partition_size, int partition_id);

    ~JJData();
    void write_jj_file(const char* outfilename);
    void reset();
    CellIndex get_number_of_cells();
    CellIndex get_number_of_primary_cells();
    void recombine(const char* filename);
    CellIndex cell_id_to_index(CellID id);
    CellID cell_index_to_id(CellIndex index);
    
private:
    std::map<CellID, CellIndex> map;

    bool trace(CellID id);
    bool generate_partition_consistency_equation(JJData* parent, SumIndex index, ConsistencyEquation* partition_equation);
    CellID find_marginal_id(ConsistencyEquation* equation);
    CellIndex find_marginal_index_in_equation(ConsistencyEquation* equation);

};
