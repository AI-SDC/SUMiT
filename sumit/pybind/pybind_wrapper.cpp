/**
 * @file pybind_wrapper.cpp
 * @author Richard Preen <rpreen@gmail.com>
 * @copyright The Authors.
 * @date 2022.
 * @brief Python library wrapper functions.
 */

#ifdef _WIN32 // Try to work around https://bugs.python.org/issue11566
    #define _hypot hypot
#endif

#include <fstream>
#include <iostream>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <sstream>
#include <string>
#include <vector>

#include <Unpicker.h>

System sys;
Logger *logger = new Logger(1, "Log.txt");
bool debugging = false;

namespace py = pybind11;

PYBIND11_MODULE(sumit, m)
{
    m.doc() = "SUMiT: Statistical Uncertainty Management Toolkit";

    py::class_<Unpicker>(m, "Unpicker")
        .def(py::init<const char *>(), "Creates a new Unpicker",
             py::arg("filename"))
        .def("Attack", &Unpicker::Attack, "Executes an attack.")
        .def("GetNumberOfCells", &Unpicker::GetNumberOfCells,
             "Returns the number of cells.")
        .def("GetNumberOfPrimaryCells", &Unpicker::GetNumberOfPrimaryCells,
             "Returns the number of primary cells.")
        .def("GetNumberOfSecondaryCells", &Unpicker::GetNumberOfSecondaryCells,
             "Returns the number of secondary cells.")
        .def("GetNumberOfPrimaryCells_ValueKnownExactly",
             &Unpicker::GetNumberOfPrimaryCells_ValueKnownExactly,
             "Returns the number of primary cells where the value is known "
             "exactly.")
        .def("print_exact_exposure", &Unpicker::print_exact_exposure,
             "Prints the exact exposure.", py::arg("filename"))
        .def("print_partial_exposure", &Unpicker::print_exact_exposure,
             "Prints the partial exposure.", py::arg("filename"));
}
