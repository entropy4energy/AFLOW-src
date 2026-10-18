// ***************************************************************************
// *                                                                         *
// *           Aflow COREY OSES - Johns Hopkins University 2022-2026         *
// *                                                                         *
// ***************************************************************************
// aflow_prototypes_jhu.cpp
// Prototypes contributed at Johns Hopkins University.
// Contributors: TL = Tianhao Li, GQ = Guotao Qiu.
// To add a prototype, append a record and run aflow --unit_test=jhu_proto,proto, which checks
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
    {"A2BC6D2_cF88_227_c_a_f_d", 1001, "a,x4",
     "10.46,0.39",
     "Bi2FO6Ta2", "Ordered pyrochlore Bi2Ta2O6F", "-",
     "ordered Bi2Ta2O6F built from BiTa2O6F with Bi on 16c at 50% occupancy (ICSD #25553); DOI:10.1002/zaac.19372350121", "TL", false},
    {"A4B12C3_oC76_41_2b_6b_ab", 1001, "a,b/a,c/a,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4,x5,y5,z5,x6,y6,z6,x7,y7,z7,x8,y8,z8,x9,y9,z9,x10,y10,z10",
     "32.83,0.16482,0.166,0.5452,0.0668,0.9978,0,0.21135,0.0199,0.9991,0.9967,0.222,0.207,0.2507,0.752,0.764,0.4404,0.475,0.573,0.3185,0.926,0.96,0.1215,0.785,0.794,0.869,0.2,0.159,0.3714,0.499,0.5533",
     "Bi4O12Ti3", "Bi4Ti3O12 (Aurivillius, m=3)", "-",
     "Aurivillius ferroelectric Bi4Ti3O12; DOI:10.1080/00150197108237680, 10.1016/0022-3697(62)90526-7", "TL", false},
    {"A3B7_mC60_12_a4i_b10i", 1001, "a,b/a,c/a,beta,x3,z3,x4,z4,x5,z5,x6,z6,x7,z7,x8,z8,x9,z9,x10,z10,x11,z11,x12,z12,x13,z13,x14,z14,x15,z15,x16,z16",
     "22.2943,0.1708957,0.5351,127.5605,0.8145,0.633,0.8914,0.1486,0.7054,0.7831,0.9229,0.4827,0.906,0.814,0.715,0.433,0.945,0.091,0.752,0.717,0.871,0.545,0.979,0.372,0.622,0.821,0.786,0.974,0.842,0.274,0.601,0.581",
     "Nb3O7", "Nb3O7 (ordered TiNb2O7)", "-",
     "parent lattice of TiNb2O7 with Nb and Ti sharing the 4i and 2a sites (ICSD #15430)", "TL", false},
    {"A17B8_oI100_46_b8c_ab3c", 1001, "a,b/a,c/a,z1,y2,z2,y3,z3,x4,y4,z4,x5,y5,z5,x6,y6,z6,x7,y7,z7,x8,y8,z8,x9,y9,z9,x10,y10,z10,x11,y11,z11,x12,y12,z12,x13,y13,z13,x14,y14,z14",
     "40.92,0.120479,0.128788,0.5,0.3564,0.1537,0.5151,0.5087,0.6976,0.8485,0.9426,0.7195,0.3082,0.811,0.6589,0.2925,0.8251,0.5956,0.2915,0.7785,0.5362,0.2505,0.8797,0.5827,0.7994,0.9251,0.5249,0.7973,0.6839,0.6419,0.8421,0.6826,0.565,0.4848,0.0894,0.62789,0.0138,0.0136,0.68965,0.4849,0.0861",
     "O17Zr8", "Nb2Zr6O17 (Nb and Zr on shared sites)", "-",
     "Nb2Zr6O17 with Nb and Zr on the same sites, giving a binary lattice (ICSD #19039)", "TL", false},
    {"A5B3_tP8_123_agh_bh", 1001, "a,c/a,z3,z4,z5",
     "1.3167,3.504671,0.25,0.125,0.375",
     "Pd5Ti3", "Pd5Ti3 (HTQC 658.BA)", "-",
     "Pd5Ti3; HTQC 658.BA, same prototype as 658.AB", "TL", false},
    {"AB2_hR3_160_a_2a", 1001, "a,c/a,x1,x2,x3",
     "1.21354,5.81481,0.7523,0.581,0",
     "MoS2", "alpha-MoS2 (HTQC 681.BA)", "-",
     "alpha-MoS2; HTQC 681.BA, same prototype as 681.AB", "TL", false},
    {"A_tP2_123_g", 1001, "a,c/a,z1",
     "20,1,0.97775",
     "H2", "H2 molecule in a 20 Angstrom box", "-",
     "H2 with bond length 0.89 Angstrom (experimental 0.741), lattice parameter kept at 20 Angstrom", "TL", true},
    {"A_tP2_123_g", 1002, "a,c/a,z1",
     "20,1,0.97255",
     "N2", "N2 molecule in a 20 Angstrom box", "-",
     "N2 with bond length 1.098 Angstrom, lattice parameter kept at 20 Angstrom", "TL", true},
    {"A_tP2_123_g", 1003, "a,c/a,z1",
     "20,1,0.96375",
     "O2", "O2 molecule in a 20 Angstrom box", "-",
     "O2 with bond length 1.45 Angstrom (experimental 1.208), lattice parameter kept at 20 Angstrom", "TL", true},
    {"A_tP2_123_g", 1004, "a,c/a,z1",
     "20,1,0.9575",
     "F2", "F2 molecule in a 20 Angstrom box", "-",
     "F2 with bond length 1.70 Angstrom (experimental 1.412), lattice parameter kept at 20 Angstrom", "TL", true},
    {"A_tP2_123_g", 1005, "a,c/a,z1",
     "20,1,0.94025",
     "Cl2", "Cl2 molecule in a 20 Angstrom box", "-",
     "Cl2 with bond length 2.39 Angstrom (experimental 1.988); also used for Br2 and I2, lattice parameter kept at 20 Angstrom", "TL", true},
    {"A6BC4_mC22_12_ij_a_gh", 1001, "a,b/a,c/a,beta,y2,y3,x4,z4,x5,y5,z5",
     "6.890000000000000,1.729462989840348,0.993033381712627,109.769999999999996,0.334900000000000,0.155000000000000,0.250800000000000,0.759100000000000,0.753600000000000,0.833800000000000,0.756200000000000",
     "Br6ErLi4", "ordered Li-Er-Br (from Li3ErBr6, ICSD #50182)", "-",
     "ordered cell derived from Li3ErBr6 (ICSD #50182)", "TL", false},
    {"A18B5C12_hP35_164_3i_a2d_gh", 1001, "a,c/a,z2,z3,x4,x5,x6,z6,x7,z7,x8,z8",
     "11.177000000000000,0.540037577167397,0.510000000000000,0.050600000000000,0.660300000000000,0.288400000000000,0.113500000000000,0.768100000000000,0.221500000000000,0.267600000000000,0.445400000000000,0.755900000000000",
     "Cl18Er5Li12", "ordered Li-Er-Cl (from Li3ErCl6, ICSD #50151)", "-",
     "ordered cell derived from Li3ErCl6 (ICSD #50151)", "TL", false},
    {"A6BC4_hP33_164_3i_ad_gh", 1001, "a,c/a,z2,x3,x4,x5,z5,x6,z6,x7,z7",
     "11.177000000000000,0.540037577167397,0.490000000000000,0.339700000000000,0.711600000000000,0.886500000000000,0.768100000000000,0.778500000000000,0.267600000000000,0.554600000000000,0.755900000000000",
     "Cl6ErLi4", "ordered Li-Er-Cl (from Li3ErCl6, ICSD #51051)", "-",
     "ordered cell derived from Li3ErCl6 (ICSD #51051): Er 0.97 set to 1, 0.03 set to vacancy", "TL", false},
    {"A4BC3_oP8_25_abcd_a_bcd", 1001, "a,b/a,c/a,z1,z2,z3,z4,z5,z6,z7,z8",
     "3.158,0.974034198860038,2.637428752374921,0.068,0.77,0.35,0.65,0.56,0.26,0.86,0.14",
     "B4MnW3", "W3MnB4", "-",
     "W3MnB4; DOI:10.1063/5.0113745", "TL", false},
    {"AB_mC12_8_3a_3a", 1001, "a,b/a,c/a,beta,x1,z1,x2,z2,x3,z3,x4,z4,x5,z5,x6,z6",
     "10.750900000000000,0.368824935586788,0.915458240705429,137.608000000000004,0.029079328465220,0.975223662779070,0.614436694793950,0.353494565653910,0.134693424893950,0.710314202287510,0.410341867505940,0.681422143922100,0.359032125796440,0.384589211851820,0.785749891877820,0.061622880172250",
     "CaCu", "CaCu", "-",
     "CaCu", "TL", false},
    {"AB2_oC12_36_a_2a", 1001, "a,b/a,c/a,y1,z1,y2,z2,y3,z3",
     "3.915,1.931,3.4618135376756,0.664,0.38,0.207,0.259,0.53,0",
     "CrI2", "CrI2 (ICSD #2596)", "-",
     "CrI2; DOI:10.1107/S0567740873005030 (ICSD #2596)", "GQ", false},
    {"AB2_mC6_12_a_i", 1001, "a,b/a,c/a,beta,x2,z2",
     "7.388,0.5366811,1.003,114,0.265,0.761",
     "CrI2", "CrI2 (ICSD #4073)", "-",
     "CrI2; Koord. Khim. 1, 670 (1975) (ICSD #4073)", "GQ", false},
    {"A3B_oP16_59_ab2e_f", 1001, "a,b/a,c/a,z1,z2,y3,z3,y4,z4,x5,z5",
     "6.4705,1.88932849,1.092574,0.923,0.4353,0.4133,0.4113,0.9071,0.9087,0.479,0.2365",
     "I3Ti", "TiI3 (ICSD #422868)", "-",
     "TiI3; DOI:10.1524/zkri.2011.1370 (ICSD #422868)", "GQ", false},
    {"A7B2_oP36_61_a3c_c", 1001, "a,b/a,c/a,x2,y2,z2,x3,y3,z3,x4,y4,z4,x5,y5,z5",
     "7.2971,0.76092,1.8784174535,0.2687,0.3006,0.37858,0.5546,0.0645,0.29002,0.6224,0.4414,0.41407,0.48564,0.20153,0.39287",
     "O7Tc2", "Tc2O7 (ICSD #140216)", "-",
     "Tc2O7; DOI:10.1021/acs.inorgchem.8b02368 (ICSD #140216)", "GQ", false},
    {"A2B3_oP80_61_ab3c_6c", 1001, "a,b/a,c/a,x3,y3,z3,x4,y4,z4,x5,y5,z5,x6,y6,z6,x7,y7,z7,x8,y8,z8,x9,y9,z9,x10,y10,z10,x11,y11,z11",
     "9.4233,0.99878,0.9994,0.7476,0.28469,0.00623,0.71506,0.00448,0.75398,0.99866,0.25285,0.71469,0.08454,0.13378,0.84918,0.87183,0.14472,0.08506,0.85426,0.91968,0.876,0.58282,0.62466,0.35471,0.37263,0.648556,0.57996,0.35084,0.41156,0.3655",
     "Mn2O3", "Mn2O3 (ICSD #24342)", "-",
     "Mn2O3; DOI:10.3891/acta.chem.scand.21-2871 (ICSD #24342)", "GQ", false},
    {"A2B_tP6_137_d_a", 1001, "a,c/a,z2",
     "4.2496,2.961714,0.8761",
     "I2Zn", "ZnI2 (ICSD #139054)", "-",
     "ZnI2; DOI:10.1103/PhysRevMaterials.5.105003 (ICSD #139054)", "GQ", false},
    {"A2B_tI96_142_deg_g", 1001, "a,c/a,z1,x2,x3,y3,z3,x4,y4,z4",
     "12.284,1.919733,0.0047,0.262,0.0113,0.9993,0.1279,0.3749,0.3625,0.0627",
     "I2Zn", "ZnI2 (ICSD #2404)", "-",
     "ZnI2; DOI:10.1107/S0567740878010390 (ICSD #2404)", "GQ", false},
    {"A2B_hR3_166_c_a", 1001, "a,c/a,x2",
     "3.927,5.04965,0.767",
     "I2Ni", "NiI2 (ICSD #197996)", "-",
     "NiI2; NBS Spec. Publ. 301, 233 (1969) (ICSD #197996)", "GQ", false},
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
