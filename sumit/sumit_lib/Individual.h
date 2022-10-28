#pragma once

#include "stdafx.h"
#include "JJData.h"

struct Individual {
    CellIndex *genes;
    double* costs;
    int number_of_costs;
    double fitness;
};
