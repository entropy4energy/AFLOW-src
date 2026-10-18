// ***************************************************************************
// *                                                                         *
// *          Aflow COREY OSES - Johns Hopkins University 2025-2026          *
// *           Aflow SHAO-YU TSENG - Johns Hopkins University 2025           *
// *                                                                         *
// ***************************************************************************
#ifndef AFLOW_BINARY_DATA_H
#define AFLOW_BINARY_DATA_H
#include "aflow.h"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <limits>
#include <iostream>
namespace xbinary {
      // ElementA-ElementB -> hmix value
      extern const std::unordered_map<std::string, double> hmix_data;
      double get_hmix_kj_mol_miedema(std::string elementA, std::string elementB);
      double get_hmix_ev_atom_miedema(std::string elementA, std::string elementB);
      double get_total_hmix_regular_solution(xstructure xstr);
}
#endif
