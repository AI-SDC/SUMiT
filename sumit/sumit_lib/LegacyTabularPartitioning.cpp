#include "stdafx.h"
#include <string.h>
#include "LegacyTabularPartitioning.h"

int required_partitions[6][6] = {
    { 4, 4, 4, 4, 4, 4},
    { 5, 5, 5, 5, 5, 5},
    { 6, 6, 7, 7, 6, 6},
    { 7, 7, 7, 7, 6, 6},
    { 22, 14, 10, 9, 8, 8},
    { 27, 14, 12, 12, 8, 9}
};

int LegacyTabularPartitioning::partition_index(int NumberOfKeyValues) {
    if (NumberOfKeyValues <= 20) {
        return 0;
    } else if (NumberOfKeyValues <= 50) {
        return 1;
    } else if (NumberOfKeyValues <= 100) {
        return 2;
    } else if (NumberOfKeyValues <= 200) {
        return 3;
    } else if (NumberOfKeyValues <= 500) {
        return 4;
    } else {
        return 5;
    }
}

int LegacyTabularPartitioning::number_of_required_partitions(TabularData* tabular, char* param1, char* param2) {
    int number = required_partitions[partition_index(tabular->GetFieldSize(param1))][partition_index(tabular->GetFieldSize(param2))];
    int number_high_level = tabular->get_highest_level_field_size(param1);

    if (number < number_high_level) {
        return number;
    } else {
        return number_high_level;
    }
}

// Partition tabular data
LegacyTabularPartitioning::LegacyTabularPartitioning(char* metadataFilename, char* tabdataFilename, char* partition_by_1, char* partition_by_2) {
    int partition_1_index;
    int partition_2_index;

    logger->log(3, "Legacy tabular partitioning");
    
    tabular = new TabularData(metadataFilename, tabdataFilename);
    logger->log(3, "%d dimensions", tabular->GetNumberOfDimensions());
    logger->log(3, "%d cells", tabular->GetNumberOfCells());
    
    partition_1_index = tabular->GetFieldIndex(partition_by_1);

    if (tabular->GetFieldType(partition_by_1) != FIELD_TYPE_RECODEABLE) {
        logger->error(1, "Partitioning parameter (%s) is not a key field", partition_by_1);
    }

    logger->log(3, "Partition parameter 1 (%s) has index %d", partition_by_1, partition_1_index);

    if (tabular->IsHierarchical(partition_by_1)) {
        logger->log(3, "Partition parameter 1 (%s) is hierarchical", partition_by_1);
    } else {
        logger->log(3, "Partition parameter 1 (%s) is not hierarchical", partition_by_1);
    }

    logger->log(3, "Partition parameter 1 (%s) has %d values", partition_by_1, tabular->GetFieldSize(partition_by_1));
    logger->log(3, "Partition parameter 1 (%s) has %d high level values", partition_by_1, tabular->get_highest_level_field_size(partition_by_1));

    int density_1;
    if (tabular->get_highest_level_field_size(partition_by_1)) {
        density_1 = tabular->GetFieldSize(partition_by_1) / tabular->get_highest_level_field_size(partition_by_1);
    } else {
        density_1 = 0;
    }

    logger->log(3, "Partition parameter 1 (%s) has density of %d", partition_by_1, density_1);
    logger->log(3, "Partition parameter 1 (%s) requires %d partitions", partition_by_1, number_of_required_partitions(tabular, partition_by_1, partition_by_2));

    partition_2_index = tabular->GetFieldIndex(partition_by_2);

    if (tabular->GetFieldType(partition_by_2) != FIELD_TYPE_RECODEABLE) {
        logger->error(1, "Partitioning parameter (%s) is not a key field", partition_by_2);
    }
        
    logger->log(3, "Partition parameter 2 (%s) has index %d", partition_by_2, partition_2_index);

    if (tabular->IsHierarchical(partition_by_2)) {
        logger->log(3, "Partition parameter 2 (%s) is hierarchical", partition_by_2);
    } else {
        logger->log(3, "Partition parameter 2 (%s) is not hierarchical", partition_by_2);
    }

    logger->log(3, "Partition parameter 2 (%s) has %d values", partition_by_2, tabular->GetFieldSize(partition_by_2));
    logger->log(3, "Partition parameter 2 (%s) has %d high level values", partition_by_2, tabular->get_highest_level_field_size(partition_by_2));

    int density_2;
    if (tabular->get_highest_level_field_size(partition_by_2)) {
        density_2 = tabular->GetFieldSize(partition_by_2) / tabular->get_highest_level_field_size(partition_by_2);
    } else {
        density_2 = 0;
    }

    logger->log(3, "Partition parameter 2 (%s) has density of %d", partition_by_2, density_2);
    logger->log(3, "Partition parameter 2 (%s) requires %d partitions", partition_by_2, number_of_required_partitions(tabular, partition_by_2, partition_by_1));

    tabular->CreatePartitionedHierarchyFiles(partition_by_1, number_of_required_partitions(tabular, partition_by_1, partition_by_2));
    tabular->CreatePartitionedHierarchyFiles(partition_by_2, number_of_required_partitions(tabular, partition_by_2, partition_by_1));

    tabular->CreatePartitionedMetadataFiles(partition_by_1, number_of_required_partitions(tabular, partition_by_1, partition_by_2), partition_by_2, number_of_required_partitions(tabular, partition_by_2, partition_by_1));

    number_of_partitions = number_of_required_partitions(tabular, partition_by_1, partition_by_2) * number_of_required_partitions(tabular, partition_by_2, partition_by_1);
    
    number_of_cells = new int[number_of_partitions];
    number_of_primary_cells = new int[number_of_partitions];
}

LegacyTabularPartitioning::~LegacyTabularPartitioning() {
    delete[] number_of_primary_cells;
    delete[] number_of_cells;
    delete tabular;
}

// Write a partitioned JJ file
void LegacyTabularPartitioning::write_partitioned_jj_file(int index, const char* jj_filename) {
    if (index >= number_of_partitions) {
        logger->error(1, "Partition index out of bounds");
    }

    char partitioned_metadata_filename[sizeof("Metadata_XXXXX.rda")];
    sprintf(partitioned_metadata_filename, "Metadata_%d.rda", index + 1);

    TabularData* partitioned_tabular = new TabularData(partitioned_metadata_filename, tabular->get_tab_data_filename());

    number_of_cells[index] = partitioned_tabular->GetNumberOfCells();
    number_of_primary_cells[index] = partitioned_tabular->GetNumberOfPrimaryCells();

    partitioned_tabular->PrintJJFile(jj_filename);

    char partitioned_map_filename[sizeof("Map_XXXXX.txt")];
    sprintf(partitioned_map_filename, "Map_%d.txt", index + 1);
    
    partitioned_tabular->PrintJJFileMapping(partitioned_map_filename);

    delete partitioned_tabular;
}

// Return the number of cells in a partition
int LegacyTabularPartitioning::get_number_of_cells(int index) {
    if (index >= number_of_partitions) {
        logger->error(1, "Partition index out of bounds");
    }
    
    return number_of_cells[index];
}


// Return the number of primary cells in a partition
int LegacyTabularPartitioning::get_number_of_primary_cells(int index) {
    if (index >= number_of_partitions) {
        logger->error(1, "Partition index out of bounds");
    }
    
    return number_of_primary_cells[index];
}
