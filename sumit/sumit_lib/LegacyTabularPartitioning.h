#pragma once

#include "stdafx.h"
#include "Partitioning.h"
#include "TabularData.h"

class LegacyTabularPartitioning: public Partitioning {

public:
    LegacyTabularPartitioning(char* metadataFilename, char* tabdataFilename, char* partition_by_1, char* partition_by_2);
    ~LegacyTabularPartitioning();
    void write_partitioned_jj_file(int index, const char* jj_filename);
    int get_number_of_cells(int index);
    int get_number_of_primary_cells(int index);

private:
    TabularData* tabular;
    int* number_of_cells;
    int* number_of_primary_cells;

    int partition_index(int NumberOfKeyValues);
    int number_of_required_partitions(TabularData* tabular, char* param1, char* param2);

};
