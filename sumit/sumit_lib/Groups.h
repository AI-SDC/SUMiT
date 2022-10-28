#pragma once

#include "stdafx.h"
#include "JJData.h"
#include "CellStore.h"

class Groups {

public:
    struct Groupings {
        CellIndex size;
        CellIndex *cell_index;
    };

    int number_of_groups;

    Groups(JJData *jjData);
    ~Groups();

    Groupings *group;

private:
    JJData *jjData;

    struct CellToConstraints {
        CellIndex number_of_constraints;
        CellIndex *constraint_number;
    };

    CellToConstraints *cell_constraints_mapping;

    struct ConstraintsToCell {
        CellIndex number_of_cells;
        double half_sum_lower_protection_levels;
        double half_sum_upper_protection_levels;
        CellIndex *cell_index;
    };

    ConstraintsToCell *constraints_cell_mapping;

    int max_groups;
    int max_groups_limit;
    int max_group_size;

    bool cell_in_constraint(CellIndex cell, CellIndex constraint);
    bool cell_already_in_group(CellIndex cell, CellIndex grp);

};
