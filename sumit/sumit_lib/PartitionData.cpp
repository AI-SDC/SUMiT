#include "stdafx.h"
#include <string.h>
#include "PartitionData.h"

PartitionData::PartitionData(void) {
    protection = NULL;
    in_jj_file[0] = '\0';
    out_jj_file[0] = '\0';
    sam_file[0] = '\0';
    map_file[0] = '\0';
    csv_file[0] = '\0';
    metadata_file[0] = '\0';
    number_of_primary_cells = 0;
    execution_time_seconds = 1000;
    cost = 0.0;
}

PartitionData::~PartitionData(void) {
}

void PartitionData::initialise(int id) {
    this->index = id;
    sprintf(in_jj_file, "JJ_%d.jj", id);
    sprintf(out_jj_file, "JJ_%d_Protected.jj", id);
    sprintf(sam_file, "JJ_%d.sam", id);
    sprintf(map_file, "Map_%d.txt", id);
    sprintf(csv_file, "Csv_%d.csv", id);
    sprintf(metadata_file, "Metadata_%d.rda", id);
}
