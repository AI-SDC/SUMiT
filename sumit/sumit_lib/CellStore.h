#pragma once

#include "stdafx.h"
#include "JJData.h"

class CellStore {

public:
    CellIndex *cells;
    CellIndex size;

    CellStore(JJData *jjData);
    ~CellStore();
    void store_selected_cells();
    void store_all_cells();
    void store_all_potential_cells();
    void order_cells_by_largest_protection_level();
    void order_cells_by_largest_weighting();
    void order_cells_by_smallest_protection_level();
    void order_cells_by_smallest_weighting();

private:
    JJData *jjData;

    void store_cell(CellIndex i);
    void swap_stored_cells(CellIndex i, CellIndex j);

};
