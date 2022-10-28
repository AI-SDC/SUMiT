#include "stdafx.h"
#include "CellStore.h"

CellStore::CellStore(JJData *jjData) {
    this->jjData = jjData;

    cells = new CellIndex[jjData->ncells];
    size = 0;
}

CellStore::~CellStore() {
    delete[] cells;
}

void CellStore::store_cell(CellIndex i) {
    // Assume no duplicate cells
    cells[size] = i;
    size++;
}

void CellStore::store_selected_cells() {
    for (CellIndex i = 0; i < jjData->ncells; i++) {
        if (jjData->cells[i].status == 'u') {
            store_cell(i);
        }
    }
}

void CellStore::store_all_cells() {
    for (CellIndex i = 0; i < jjData->ncells; i++) {
        store_cell(i);
    }
}

void CellStore::store_all_potential_cells() {
    for (CellIndex i = 0; i < jjData->ncells; i++) {
        if (jjData->cells[i].status == 's') {
            store_cell(i);
        }
    }
}

void CellStore::swap_stored_cells(CellIndex i, CellIndex j) {
    CellIndex temp = cells[i];
    cells[i] = cells[j];
    cells[j] = temp;
}

void CellStore::order_cells_by_largest_protection_level() {
    for (CellIndex i = 0; i < size; i++) {
        for (CellIndex j = 0; j < size; j++) {
            CellIndex k = cells[i];
            CellIndex l = cells[j];

            if (jjData->cells[k].upper_protection_level > jjData->cells[l].upper_protection_level) {
                swap_stored_cells(i, j);
            }
        }
    }
}

void CellStore::order_cells_by_largest_weighting() {
    for (CellIndex i = 0; i < size; i++) {
        for (CellIndex j = 0; j < size; j++) {
            CellIndex k = cells[i];
            CellIndex l = cells[j];

            if (jjData->cells[k].loss_of_information_weight > jjData->cells[l].loss_of_information_weight) {
                swap_stored_cells(i, j);
            }
        }
    }
}

void CellStore::order_cells_by_smallest_protection_level() {
    for (CellIndex i = 0; i < size; i++) {
        for (CellIndex j = 0; j < size; j++) {
            CellIndex k = cells[i];
            CellIndex l = cells[j];

            if (jjData->cells[k].upper_protection_level < jjData->cells[l].upper_protection_level) {
                swap_stored_cells(i, j);
            }
        }
    }
}

void CellStore::order_cells_by_smallest_weighting() {
    for (CellIndex i = 0; i < size; i++) {
        for (CellIndex j = 0; j < size; j++) {
            CellIndex k = cells[i];
            CellIndex l = cells[j];

            if (jjData->cells[k].loss_of_information_weight < jjData->cells[l].loss_of_information_weight) {
                swap_stored_cells(i, j);
            }
        }
    }
}
