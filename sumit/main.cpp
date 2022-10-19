#include "Unpicker.h"
#include <iostream>

int
main(int argc, char **argv)
{
    if (argc > 2) {
        std::cout << "filename = " << argv[1] << std::endl;

        Unpicker *p = new Unpicker(argv[1]);

        std::cout << "BEFORE:" << std::endl;
        std::cout << "n_cells = " << p->GetNumberOfCells() << std::endl;
        std::cout << "n_primary = " << p->GetNumberOfPrimaryCells()
                  << std::endl;
        std::cout << "n_secondary = " << p->GetNumberOfSecondaryCells()
                  << std::endl;

        p->Attack();

        std::cout << "AFTER:" << std::endl;
        std::cout << "n_cells = " << p->GetNumberOfCells() << std::endl;
        std::cout << "n_primary = " << p->GetNumberOfPrimaryCells()
                  << std::endl;
        std::cout << "n_secondary = " << p->GetNumberOfSecondaryCells()
                  << std::endl;

        p->print_exact_exposure(argv[2]);

        delete p;
    }
    return 0;
}
