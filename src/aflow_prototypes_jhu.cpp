// ***************************************************************************
// *                                                                         *
// *           Aflow COREY OSES - Johns Hopkins University 2022-2026         *
// *                                                                         *
// ***************************************************************************
// aflow_prototypes_jhu.cpp
// Prototypes contributed at Johns Hopkins University.
// Contributors: TL = Tianhao Li, GQ = Guotao Qiu.
// To add a prototype, append a record to aflow_prototypes_jhu_records.inc (internal) and run aflow --unit_test=jhu_proto,proto, which checks
// the records and regenerates each structure to confirm its label. //CO20260930 //CO20261004
// A new parameter set takes the next free number from 1001 up on its label (FIRST_SET);
// 1 to 999 stay with the library's own sets. //CO20261004
// Labels follow the AFLOW prototype label scheme:
//   M. J. Mehl et al., Comput. Mater. Sci. 136, S1 (2017), doi:10.1016/j.commatsci.2017.01.017
//   D. Hicks et al., Comput. Mater. Sci. 161, S1 (2019), doi:10.1016/j.commatsci.2018.10.043
//   D. Hicks et al., Comput. Mater. Sci. 199, 110450 (2021), doi:10.1016/j.commatsci.2021.110450

#include "aflow.h"
#include "aflow_prototypes_jhu.h"

namespace jhuproto {

  // label, set, parameter list, parameter values,
  // prototype, name, Strukturbericht,
  // reference, contributor, keep lattice parameter
  // (the Pearson symbol and space group are parts 2 and 3 of the label) //CO20261004
  static const std::vector<Record> vrecords = {
#if __has_include("aflow_prototypes_jhu_records.inc") //CO20261105 - the records are internal; a public build has none
#include "aflow_prototypes_jhu_records.inc"
#endif
  };

  const std::vector<Record>& records() {return vrecords;}

  // parts of the label: stoichiometry, Pearson symbol, space group, Wyckoff letters //CO20261004
  static string labelPart(const string& label, size_t ipart) {
    vector<string> tokens;
    aurostd::string2tokens(label, tokens, "_");
    return tokens.at(ipart);
  }
  static string pearsonSymbol(const string& label) {return labelPart(label, 1);} //CO20261004
  static uint spaceGroup(const string& label) {return aurostd::string2utype<uint>(labelPart(label, 2));} //CO20261004

  // number of species and atoms in the primitive cell, from the label //CO20261004
  static uint countSpecies(const string& label) {return aurostd::getElements(labelPart(label, 0)).size();} //CO20261004
  static uint countAtomsPrimitive(const string& label) {
    const string pearson = pearsonSymbol(label); //CO20261004
    const uint nconv = aurostd::string2utype<uint>(pearson.substr(2));
    char centering = pearson[1]; //CO20261004
    if (centering == 'R') {return nconv;} // the hR Pearson symbol counts the rhombohedral (primitive) cell //CO20261004
    return nconv / LATTICE::Conventional2PrimitiveRatio(centering); //CO20261004
  }

  // records of one label, ordered by set number
  static vector<const Record*> recordsOfLabel(const string& anrl_label) {
    vector<const Record*> vout;
    for (size_t i = 0; i < vrecords.size(); i++) {if (anrl_label == vrecords[i].label) {vout.push_back(&vrecords[i]);}}
    std::sort(vout.begin(), vout.end(), [](const Record* a, const Record* b) {return a->set < b->set;});
    return vout;
  }

  // labels in first-appearance order
  static vector<string> labels() {
    vector<string> vout;
    for (size_t i = 0; i < vrecords.size(); i++) {
      if (std::find(vout.begin(), vout.end(), string(vrecords[i].label)) == vout.end()) {vout.push_back(vrecords[i].label);}
    }
    return vout;
  }

  // adds our labels to the ANRL list; for a label already in the list, the
  // per-set fields (Strukturbericht, prototype, name) are extended with "/"
  void addToANRLList(vector<string>& vproto) {
    const vector<string> vlabels = labels();
    for (size_t l = 0; l < vlabels.size(); l++) {
      const vector<const Record*> vrec = recordsOfLabel(vlabels[l]);
      vector<string> vstruk, vproto_name, vname;
      for (size_t r = 0; r < vrec.size(); r++) {
        vstruk.push_back(vrec[r]->strukturbericht);
        vproto_name.push_back(vrec[r]->prototype);
        vname.push_back(vrec[r]->name);
      }
      // fields of the ANRL line, read with anrl::vproto2tokens() for a label already in the list //CO20261004
      string label = vlabels[l], pearson = "", params = "", strukturbericht = "", prototype = "", name = ""; //CO20261004
      uint nspecies = 0, natoms = 0, space_group = 0, nunderscores = 0, nparameters = 0; //CO20261004
      const string prefix = vlabels[l] + ";";
      size_t iexisting = vproto.size();
      for (size_t i = 0; i < vproto.size(); i++) {if (vproto[i].compare(0, prefix.size(), prefix) == 0) {iexisting = i; break;}}
      if (iexisting < vproto.size()) {
        anrl::vproto2tokens(vproto[iexisting], label, nspecies, natoms, space_group, nunderscores, nparameters, pearson, params, strukturbericht, prototype, name); //CO20261004
        strukturbericht += "/" + aurostd::joinWDelimiter(vstruk, "/"); //CO20261004
        prototype += "/" + aurostd::joinWDelimiter(vproto_name, "/"); //CO20261004
        name += "/" + aurostd::joinWDelimiter(vname, "/"); //CO20261004
      } else {
        // the label gives the species, atoms, Pearson symbol and space group //CO20261004
        vector<string> vparams;
        params = vrec[0]->parameter_list; //CO20261004
        aurostd::string2tokens(params, vparams, ",");
        nspecies = countSpecies(label); //CO20261004
        natoms = countAtomsPrimitive(label); //CO20261004
        space_group = spaceGroup(label); //CO20261004
        nunderscores = std::count(label.begin(), label.end(), '_'); //CO20261004
        nparameters = vparams.size(); //CO20261004
        pearson = pearsonSymbol(label); //CO20261004
        strukturbericht = aurostd::joinWDelimiter(vstruk, "/"); //CO20261004
        prototype = aurostd::joinWDelimiter(vproto_name, "/"); //CO20261004
        name = aurostd::joinWDelimiter(vname, "/"); //CO20261004
      }
      stringstream entry;
      entry << label << ";" << nspecies << ";" << natoms << ";" << space_group << ";"; //CO20261004
      entry << nunderscores << ";" << nparameters << ";" << pearson << ";" << params << ";"; //CO20261004
      entry << strukturbericht << ";" << prototype << ";" << name; //CO20261004
      if (iexisting < vproto.size()) {vproto[iexisting] = entry.str();} else {vproto.push_back(entry.str());} //CO20261004
    }
  }

  // appends our parameter sets for a label after the library's sets, in the order of their numbers;
  // setIndex() and setNumbers() map between those positions and the set numbers //CO20261004
  void addParameters(const string& anrl_label, vector<string>& vparameters) {
    const vector<const Record*> vrec = recordsOfLabel(anrl_label);
    for (size_t r = 0; r < vrec.size(); r++) {vparameters.push_back(vrec[r]->parameter_values);}
  }

  // position of set number "set" among our sets of the label (as appended by addParameters()), -1 if there is none //CO20261004
  int setIndex(const string& anrl_label, unsigned int set) {
    const vector<const Record*> vrec = recordsOfLabel(anrl_label);
    for (size_t r = 0; r < vrec.size(); r++) {if (vrec[r]->set == set) {return (int) r;}}
    return -1;
  }

  // set numbers in the order of anrl::getANRLParameters(label,"all"): the library's nlibrary sets (1, 2, ...),
  // then ours (1001 and up) //CO20261004
  vector<unsigned int> setNumbers(const string& anrl_label, unsigned int nlibrary) {
    vector<unsigned int> vnumbers;
    for (unsigned int i = 1; i <= nlibrary; i++) {vnumbers.push_back(i);}
    const vector<const Record*> vrec = recordsOfLabel(anrl_label);
    for (size_t r = 0; r < vrec.size(); r++) {vnumbers.push_back(vrec[r]->set);}
    return vnumbers;
  }
  // same, from the sets the caller already has from anrl::getANRLParameters(label,"all") //CO20261004
  vector<unsigned int> setNumbers(const string& anrl_label, const vector<string>& vparameters_all) {
    const size_t njhu = recordsOfLabel(anrl_label).size();
    return setNumbers(anrl_label, (unsigned int) (vparameters_all.size() - njhu)); //CO20261004
  }
  vector<unsigned int> setNumbers(const string& anrl_label) {
    return setNumbers(anrl_label, anrl::getANRLParameters(anrl_label, "all")); //CO20261004
  }

  bool keepLatticeParameter(const string& anrl_label) {
    const vector<const Record*> vrec = recordsOfLabel(anrl_label);
    for (size_t r = 0; r < vrec.size(); r++) {if (vrec[r]->keep_lattice_parameter) {return true;}}
    return false;
  }

  // true when every matched parameter set is one of the molecules in a fixed box (keep_lattice_parameter):
  // they share one structure type and differ only in bond length, so the prototype check may match them
  // with each other by design //CO20260930
  bool moleculesInBox(const vector<string>& labels_matched) {
    if (labels_matched.size() < 2) {return false;}
    vector<string> tokens; //CO20261004
    for (size_t i = 0; i < labels_matched.size(); i++) {
      // label-NNNN, split as anrl::getANRLParameters() does //CO20261004
      if (aurostd::string2tokens(labels_matched[i], tokens, "-") != 2) {return false;} //CO20261004
      const string& label = tokens[0]; //CO20261004
      const unsigned int set = aurostd::string2utype<unsigned int>(tokens[1]); //CO20261004
      bool found = false;
      for (size_t r = 0; r < vrecords.size() && !found; r++) {
        found = (label == vrecords[r].label && set == vrecords[r].set && vrecords[r].keep_lattice_parameter);
      }
      if (!found) {return false;}
    }
    return true;
  }

  // table appended to the prototype help (aflow --protos)
  string listing() {
    if (vrecords.empty()) {return "";} //CO20261105 - a build without the internal records
    stringstream out;
    out << string(120, '*') << endl;
    out << "JHU PROTOTYPES (TL = Tianhao Li, GQ = Guotao Qiu)" << endl << endl;
    out << aurostd::PaddedPOST("Label", 40) << aurostd::PaddedPOST("Pearson", 10) << aurostd::PaddedPOST("SG #", 7);
    out << aurostd::PaddedPOST("Prototype", 14) << aurostd::PaddedPOST("By", 4) << "Name" << endl;
    out << string(120, '-') << endl;
    for (size_t i = 0; i < vrecords.size(); i++) {
      const Record& r = vrecords[i];
      const string label = string(r.label) + "-" + aurostd::PaddedNumString(r.set, 3); //CO20261004
      out << aurostd::PaddedPOST(label, 40) << aurostd::PaddedPOST(pearsonSymbol(r.label), 10) << aurostd::PaddedPOST("#" + aurostd::utype2string(spaceGroup(r.label)), 7); //CO20261004
      out << aurostd::PaddedPOST(r.prototype, 14) << aurostd::PaddedPOST(r.contributor, 4) << r.name << endl;
      out << aurostd::PaddedPOST("", 40) << "source: " << r.reference << endl;
    }
    out << endl;
    return out.str();
  }

}
