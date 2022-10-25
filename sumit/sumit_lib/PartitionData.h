#pragma once

#include "stdafx.h"
#include <string.h>
#include "GAProtection.h"

class PartitionData {
    
public:
    int index;
    GAProtection* protection;
    char in_jj_file[sizeof("JJ_XXXXX.jj")];
    char out_jj_file[sizeof("JJ_XXXXX_Protected.jj")];
    char sam_file[sizeof("JJ_XXXXX.sam")];
    char map_file[sizeof("Map_XXXXX.txt")]; // Used for legacy partitioning only
    char csv_file[sizeof("Csv_XXXXX.csv")]; // Used for legacy partitioning only
    char metadata_file[sizeof("Metadata_XXXXX.rda")]; // Used for legacy partitioning only
    int number_of_primary_cells;
    int execution_time_seconds;
    double cost;
        
    PartitionData(void);
    ~PartitionData(void);
    void initialise(int id);

};
