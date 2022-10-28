#include "stdafx.h"
#include <string.h>
#include "NoPartitioning.h"
#include "JJData.h"

// Partition JJ data
NoPartitioning::NoPartitioning(const char* filename) {
    logger->log(3, "No partitioning");

    jjData = new JJData(filename);

    number_of_partitions = 1;
}

NoPartitioning::~NoPartitioning() {
    delete jjData;
}

// Write a partitioned JJ file
// The supplied filename is guaranteed to be different to that used to construct an instance of the class
void NoPartitioning::write_partitioned_jj_file(int index, const char* jj_filename) {
    if (index >= number_of_partitions) {
        logger->error(1, "Partition index out of bounds");
    }

    jjData->write_jj_file(jj_filename);
}

// Return the number of cells in a partition
// This method is guaranteed to be called only during or after the equivalent call to write_partitioned_jj_file for a particular partition
int NoPartitioning::get_number_of_cells(int index) {
    if (index >= number_of_partitions) {
        logger->error(1, "Partition index out of bounds");
    }

    return jjData->get_number_of_cells();
}


// Return the number of primary cells in a partition
// This method is guaranteed to be called only after the equivalent call to write_partitioned_jj_file for a particular partition
int NoPartitioning::get_number_of_primary_cells(int index) {
    if (index >= number_of_partitions) {
        logger->error(1, "Partition index out of bounds");
    }

    return jjData->get_number_of_primary_cells();
}
