// ***************************************************************************
// *                                                                         *
// *           Aflow COREY OSES - Johns Hopkins University 2022-2026         *
// *                                                                         *
// ***************************************************************************
// aflow_prototypes_jhu.h
// Prototypes contributed at Johns Hopkins University (Entropy for Energy
// Laboratory). The records live in aflow_prototypes_jhu.cpp; the prototype
// generator reads them through the hooks below. //CO20260930

#ifndef _AFLOW_PROTOTYPES_JHU_H_
#define _AFLOW_PROTOTYPES_JHU_H_

#include <string>
#include <vector>

namespace jhuproto {
  struct Record {
    const char* label;             // AFLOW prototype label
    unsigned int set;              // parameter-set number on the label: 1001 and up (the library keeps 1 to 999) //CO20261004
    const char* parameter_list;    // degrees of freedom, comma separated
    const char* parameter_values;  // values, comma separated, same order
    const char* prototype;         // composition in the label's species order
    const char* name;              // name shown with the prototype
    const char* strukturbericht;   // Strukturbericht designation or "-" (Pearson symbol and space group come from the label) //CO20261004
    const char* reference;         // source of the structure
    const char* contributor;       // initials
    bool keep_lattice_parameter;   // true: the stored lattice parameter is not rescaled
  };

  // JHU parameter sets are numbered from FIRST_SET up on each label and are looked up by number;
  // the library's sets keep 1 to 999 and are looked up by position //CO20261004
  const unsigned int FIRST_SET = 1001; //CO20261004

  const std::vector<Record>& records();
  void addToANRLList(std::vector<std::string>& vproto);
  void addParameters(const std::string& anrl_label, std::vector<std::string>& vparameters);
  int setIndex(const std::string& anrl_label, unsigned int set); //CO20261004
  std::vector<unsigned int> setNumbers(const std::string& anrl_label, unsigned int nlibrary); //CO20261004
  std::vector<unsigned int> setNumbers(const std::string& anrl_label, const std::vector<std::string>& vparameters_all); //CO20261004
  std::vector<unsigned int> setNumbers(const std::string& anrl_label); //CO20261004
  bool keepLatticeParameter(const std::string& anrl_label);
  bool moleculesInBox(const std::vector<std::string>& labels_matched); //CO20260930
  std::string listing();
}

#endif // _AFLOW_PROTOTYPES_JHU_H_
