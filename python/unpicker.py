#!/usr/bin/python3
#
# Copyright (C) 2022 Richard Preen <rpreen@gmail.com>
#

"""
Unpicker example
"""

import sumit

filename: str = "test.jj"

unpicker = sumit.Unpicker(filename)

n_cells: int = unpicker.GetNumberOfCells()
n_primary: int = unpicker.GetNumberOfPrimaryCells()
n_secondary: int = unpicker.GetNumberOfSecondaryCells()

print("BEFORE:")
print(f"n_cells = {n_cells} n_primary = {n_primary} n_secondary = {n_secondary}")

unpicker.Attack()

n_cells = unpicker.GetNumberOfCells()
n_primary = unpicker.GetNumberOfPrimaryCells()
n_secondary = unpicker.GetNumberOfSecondaryCells()

print("AFTER:")
print(f"n_cells = {n_cells} n_primary = {n_primary} n_secondary = {n_secondary}")

unpicker.print_exact_exposure("exposure_exact.txt")
unpicker.print_partial_exposure("exposure_partial.txt")
