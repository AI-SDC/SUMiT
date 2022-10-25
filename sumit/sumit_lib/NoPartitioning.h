#pragma once

#include "stdafx.h"
#include "Partitioning.h"
#include "JJData.h"

class NoPartitioning: public Partitioning {

public:
    NoPartitioning(const char* filename);
    ~NoPartitioning();
    void write_partitioned_jj_file(int index, const char* jj_filename);
    int get_number_of_cells(int index);
    int get_number_of_primary_cells(int index);
    
private:
    JJData* jjData;

};
