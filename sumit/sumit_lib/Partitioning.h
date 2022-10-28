#pragma once

#include "stdafx.h"

class Partitioning {

public:
    Partitioning();
    virtual ~Partitioning();

    // Return the number of partitions created
    virtual int get_number_of_partitions();

    // Write a partitioned JJ file
    // The supplied filename is guaranteed to be different to that used to construct an instance of the class
    virtual void write_partitioned_jj_file(int index, const char* jj_filename) = 0;

    // Return the number of cells in a partition
    // This method is guaranteed to be called only during or after the equivalent call to write_partitioned_jj_file for a particular partition
    virtual int get_number_of_cells(int index) = 0;

    // Return the number of primary cells in a partition
    // This method is guaranteed to be called only after the equivalent call to write_partitioned_jj_file for a particular partition
    virtual int get_number_of_primary_cells(int index) = 0;

protected:
    int number_of_partitions;

private:

};
