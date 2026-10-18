// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2023           *
// *           Aflow RICO FRIEDRICH - Duke University 2018-2021              *
// *                                                                         *
// ***************************************************************************
// Written by Rico Friedrich, Corey Oses, and Marco Esters
// rico.friedrich@duke.edu

#ifndef _AFLOW_CCE_H_
#define _AFLOW_CCE_H_

static const string CCE_allowed_functionals = "PBE,LDA,SCAN,PBE+U:ICSD,exp"; // when adding a new functional also introduce new 'offset' in get_offset function needed for reading corrections from lookup table
static const string CCE_default_output_functionals= "PBE,LDA,SCAN,exp"; // corrections are given for these functionals if only a structure is given as input for the command line and web tools (i.e. --functionals= is not set)
static const string CCE_temperatures = "298.15,0"; // needs to be extended when adding new corrections for other temperatures
static const double _CCE_NN_DIST_TOL_ = 0.5; // 0.5 Ang tolerance between shortest and longest bonds for each cation-anion pair; works best up to now; in future maybe bonding could be explicitly determined via Bader analysis
//[RF20200912 - MOVED TO AFLOW.RC]static const double _CCE_NN_DIST_TOL_MULTI_ANION_ = 0.4; // 0.4 Ang tolerance between shortest and longest bonds for each bond when testing for multi-anion compound; it was found that the standard 0.5 Ang tol. is too large such that different anions appear to be bonded, which would prevent anions to be detected as such
//[RF20200912 - MOVED TO AFLOW.RC]static const double _CCE_OX_TOL_ = 0.001; // choose small finite value since sum of oxidation states might not be exactly zero due to numerics
//[RF20200912 - MOVED TO AFLOW.RC]static const double _CCE_SELF_DIST_TOL_ = 0.001; // distance tolerance in Ang for neighbor screening to savely exclude the cation itself having distance zero to itself
//[RF20200912 - MOVED TO AFLOW.RC]static const double _CCE_perox_cutoff_=1.6; // O-O bonds in peroxides for the studied examples are all shorter than 1.6 Ang
//[RF20200912 - MOVED TO AFLOW.RC]static const double _CCE_superox_cutoff_=1.4; // O-O bonds in superoxides for the studied examples are all shorter than 1.4 Ang
//[RF20200912 - MOVED TO AFLOW.RC]static const double _CCE_O2_molecule_cutoff_=1.2; // O-O bonds in the O2 molecule is about 1.21 Ang.
static const uint CCE_num_functionals_Bader=4; // Currently, Bader charges used to determine oxidation states are available for 4 functionals: PBE, LDA, SCAN, and PBE+U:ICSD and ONLY for oxides, see get_Bader_templates function

namespace cce {
  struct CCE_Variables {
    vector<double> enthalpies_dft;
    vector<string> vfunctionals; // should be needed as long as output for corrected dft formation enthalpies is based on vfunctionals
    vector<int> offset; // needed for reading corrections from lookup table for different functionals
    vector<string> vtemperatures;
    double standard_anion_charge;
    vector<double> electronegativities;
    vector<uint> multi_anion_atoms; // vector in which elements will be 1 for multi_anion atoms and 0 otherwise
    vector<double> oxidation_states;
    string anion_species;
    vector<double> cutoffs;
    xstructure xstr_neighbors;
    deque<deque<uint> > i_neighbors;  //CO20200914
    deque<deque<double> > distances;  //CO20200914
    vector<string> multi_anion_species; // vector storing all the multi anion species
    uint num_perox_bonds;
    uint num_superox_bonds;
    vector<uint> perox_indices; // vector in which elements will be 1 for peroxide O atoms and 0 otherwise; needed for correct setting of oxidation numbers below
    vector<uint> superox_indices; // vector in which elements will be 1 for superoxide O atoms and 0 otherwise; needed for correct setting of oxidation numbers below
    vector<uint> num_neighbors;
    vector<string> species_electronegativity_sorted;
    vector<int> num_pref_ox_states_electronegativity_sorted;
    vector< vector<double> > pref_ox_states_electronegativity_sorted;
    vector<int> num_all_ox_states_electronegativity_sorted;
    vector< vector<double> > all_ox_states_electronegativity_sorted;
    vector<vector<int> > cations_map;
    double oxidation_sum; // double because for superoxides O ox. number is -0.5
    vector<double> Bader_charges;
    vector<vector<double> > corrections_atom; // 1st dim. is number of functionals*2 (vfunctionals.size()*2) i.e. 298.15 & 0K corrections for each functional; 2nd dimension for corrections for each atom
    vector<vector<vector<double> > > multi_anion_corrections_atom; // 1st dim. for multi_anion_species, 2nd dim. for functionals and temperatures as above, 3rd dim. for corrections for each atom
    vector<double> perox_correction; // peroxide correction per cell for functionals and temperatures as above
    vector<double> superox_correction; // superoxide correction per cell for functionals and temperatures as above
    vector<double> cce_correction; // total correction per cell for functionals and temperatures as above
    vector<double> enthalpy_formation_cell_cce; // CCE formation enthalpy per cell for functionals and temperatures as above
  };

  // main CCE functions
  // for command line use, 
  // use inside AFLOW providing directory path or xstructure & functional string or flags and istream for web tool, 
  // and CCE core function called by all other main CCE functions
  void run(aurostd::xoption& flags,ostream& oss=std::cout);  //CO20201105
  void run(aurostd::xoption& flags,std::istream& ist,ostream& oss=std::cout); //CO20201105
  void run(aurostd::xoption& flags,ofstream& FileMESSAGE,ostream& oss=std::cout);  //KC+CO20220902 - added ofstream
  void run(aurostd::xoption& flags,ofstream& FileMESSAGE,std::istream& ist,ostream& oss=std::cout); //KC+CO20220902 - added ofstream
  void initialize(xstructure& structure,aurostd::xoption& flags,CCE_Variables& cce_vars,aurostd::xoption& cce_flags,ofstream& FileMESSAGE,istream& ist=std::cin); //KC20220830
  void getFlags(xstructure& structure,aurostd::xoption& flags,CCE_Variables& cce_vars,aurostd::xoption& cce_flags,ofstream& FileMESSAGE);//KC20220830 
  bool is_correctable(aurostd::xoption& flags,xstructure& structure,_aflags& aflags,ofstream& FileMESSAGE,ostream& oss=std::cout); //KC20220829
  void calcCorrectedEnthalpies(CCE_Variables& cce_vars); //KC20220830 
  void getCationCoordinations(xstructure& structure,aurostd::xoption& flags,CCE_Variables& cce_vars,aurostd::xoption& cce_flags,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE; previously get_cation_coordination_numbers()
  void getOxidationNumbers(xstructure& structure,aurostd::xoption& flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE - previously get_oxidation_numbers()
  vector<double> getCorrections(const string& directory_path);//KC20220829 - previously calculate_corrections
  vector<double> getCorrections(const xstructure& structure,string functional,const string& directory_path=aurostd::getPWD(),ostream& oss=std::cout);//KC20220829 - previously calculate_corrections
  vector<double> getCorrections(const xstructure& structure,string functional,ofstream& FileMESSAGE,const string& directory_path=aurostd::getPWD(),ostream& oss=std::cout);//KC20220829 - previously calculate_corrections
  vector<double> getCorrections(const xstructure& structure,aurostd::xoption& cce_flags,string functional,const string& directory_path=aurostd::getPWD(),ostream& oss=std::cout);//KC20220829 - previously calculate_corrections
  vector<double> getCorrections(const xstructure& structure,aurostd::xoption& cce_flags,string functional,ofstream& FileMESSAGE,const string& directory_path=aurostd::getPWD(),ostream& oss=std::cout);//KC20220829 - previously calculate_corrections
  //void CCE_core(const xstructure& structure, xoption& cce_flags, CCE_Variables& cce_vars, const string& directory_path=aurostd::getPWD());
  void analyze(const xstructure& structure,CCE_Variables& cce_vars,xoption& cce_flags,vector<vector<uint> >& multi_anion_num_neighbors,ofstream& FileMESSAGE,const string& directory_path=aurostd::getPWD());//KC20220830
  // read user input (from command line or directory path)
  void prepareStructure(xstructure& structure,xoption& flags,const string& structure_file,int=IOAFLOW_AUTO); // set xstructure mode argument only here and it is automoatically recognized in the main CCE cpp file//KC+CO20220830
  void prepareStructure(xstructure& structure,xoption& flags,std::istream& ist);//KC+CO20220830
  void optimizeStructure(xstructure& structure,xoption& cce_flags); //KC20220830
  void getDFTFormEnthalpiesFunctionals(const string& enthalpies_dft_input_str,const string& functionals_input_str,CCE_Variables& cce_vars,aurostd::xoption& cce_flags,ofstream& FileMESSAGE);//KC+CO20220902 - previously get_dft_form_enthalpies_functionals
  int get_offset(const string& functional);
  vector<double> get_oxidation_states(const string& oxidation_numbers_input_str,const xstructure& structure,CCE_Variables& cce_vars,aurostd::xoption& cce_flags,ostream& oss=std::cout);//KC+CO20220830
  string readFunctionalFromVASPFiles(const xstructure& structure,aurostd::xoption& cce_flags,ofstream& FileMESSAGE,string& aflowin_file,string& outcar_file);//KC+CO20220902 - added FileMESSAGE - previously get_funcitonal_from_aflow_in_outcar
  // initialise flags and variables
  aurostd::xoption init_flags(); //ME20200213
  CCE_Variables init_variables(const xstructure&); //ME20200213
  // structural analysis
  string determine_anion_species(aurostd::xoption& cce_flags,const xstructure& structure,CCE_Variables& cce_vars);//KC20220829
  vector<uint> check_for_multi_anion_system(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,double tolerance=DEFAULT_CCE_NN_DIST_TOL_MULTI_ANION);
  vector<uint> getNumNeighbors(const xstructure& structure,double tolerance=_CCE_NN_DIST_TOL_); //KC+CO20220902 - previously getNumNeighbors
  vector<uint> getNumNeighbors(const xstructure& structure,const string& anion_species,double tolerance=_CCE_NN_DIST_TOL_);//KC+CO20220902 - previously getNumNeighbors
  vector<uint> getNumNeighbors(const xstructure& structure,const string& anion_species,xoption& cce_flags,CCE_Variables& cce_vars,double tolerance=_CCE_NN_DIST_TOL_);//KC_CO20220902 - added ofstream//KC+CO20220902 - previously getNumNeighbors
  vector<uint> getNumNeighbors(const xstructure& structure,const string& anion_species,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,double tolerance=_CCE_NN_DIST_TOL_); //KC+CO202209202 - added ofstream//KC+CO20220902 - previously getNumNeighbors
  vector<double> get_dist_cutoffs(const xstructure& structure);
  void checkPerSuperOxides(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE);//KC+CO20220902 - added FileMESSAGE - previously check_per_super_oxides
  // determine oxidation numbers from electronegativities
  vector<double> getOxStatesFromENs(xstructure& structure,ofstream& FileMESSAGE);//KC+CO20220902 - added FileMESSAGE -  previously get_oxidation_states_from_electronegativities 
  vector<double> getOxStatesFromENs(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE -  previously get_oxidation_states_from_electronegativities
  void set_anion_oxidation_states(const xstructure& structure,CCE_Variables& cce_vars);
  void sort_species_by_electronegativity(const xstructure& structure,CCE_Variables& cce_vars);
  void load_ox_states_templates_each_species(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars);
  void try_preferred_oxidation_states(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars);
  void treatSbO2SpecialCase(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout); //KC+CO20220902 - added FileMESSAGE - previously treat_SbO2_special_case 
  void treatPb3O4SpecialCase(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE - previously treat_Pb3O4_special_case 
  void treatTiOMagneliPhaseSpecialCase(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE - previously treat_Ti_O_Magneli_phase_special_case 
  void treatFe3O4SpecialCase(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE - previously treat_Fe3O4_special_case 
  void treatX3O4SpecialCase(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,const string& cation_species,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE - previously treat_X3O4_special_case 
  void treatAlkaliSesquioxideSpecialCase(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE - previously treat_alkali_sesquioxide_special_case 
  // following special cases only needed when determining oxidation states from Bader charges
  void treatMnMoO4SpecialCase(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE - previously treat_MnMoO4_special_case 
  void treatCa2Fe2O5_CaFe2O4LDASpecialCase(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE - previously treat_Ca2Fe2O5_CaFe2O4_LDA_Special_Case
  void treatFeTiO3LDASpecialCase(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE,ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE - previously  treat_FeTiO3_LDA_special_case
  void check_ox_nums_special_case(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ostream& oss=std::cout);
  void try_all_oxidation_states(const xstructure& structure,CCE_Variables& cce_vars);
  void determine_cation_oxidation_states(const xstructure& structure,CCE_Variables& cce_vars,const vector<vector<double> >& possible_ox_states); // ME20191101
  double get_oxidation_states_sum(CCE_Variables& cce_vars);
  // determine oxidation numbers from Bader charges (outdated)
  vector<double> get_oxidation_states_from_Bader(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE ,const string& directory_path=aurostd::getPWD(),ostream& oss=std::cout);//KC+CO20220902 - added FileMESSAGE
  void get_system_name_functional_from_aflow_in(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,string& system_name,string& functional,ofstream& FileMESSAGE ,const string& directory_path=aurostd::getPWD());//KC+CO20220902 - added FileMESSAGE
  vector<double> get_Bader_charges_from_Bader_file(const xstructure& structure,CCE_Variables& cce_vars,const string& Bader_file);
  vector<double> Bader_charges_to_oxidation_states(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,string& functional,ostream& oss=std::cout);
  void general_attempt_fixing_oxidation_states(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,ofstream& FileMESSAGE);//KC+_CO20220902 - added FileMESSAGE
  // assign corrections
  void get_corrections(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,const string& considered_anion_species,const vector<uint>& num_neighbors,vector<vector<double> >& corrections_atom,ostream& oss=std::cout);
  void load_cation_corrections(const xstructure& structure,CCE_Variables& cce_vars,const string& corrections_line,vector<vector<double> >& corrections_atom,uint i);
  void set_anion_corrections(const xstructure& structure,CCE_Variables& cce_vars,vector<vector<double> >& corrections_atom,uint i);
  void check_get_per_super_ox_corrections(CCE_Variables& cce_vars);
  // apply corrections and get corrected formation enthalpies
  void check_apply_per_super_ox_corrections(CCE_Variables& cce_vars);
  void apply_pbe_u_icsd_shifts(aurostd::xoption& cce_flags,const xstructure& structure,CCE_Variables& cce_vars,ostream& oss=std::cout);//KC20220829
  // print output and citation
  string print_output(const xstructure& structure,aurostd::xoption& flags,xoption& cce_flags,CCE_Variables& cce_vars,vector<vector<uint> >& multi_anion_num_neighbors);
  string print_JSON_cation_coordination_numbers(const xstructure& structure,xoption& cce_flags,const CCE_Variables& cce_vars,vector<vector<uint> >& multi_anion_num_neighbors);
  string print_JSON_ox_nums(const xstructure& structure,const CCE_Variables& cce_vars);
  string print_JSON_corrections(const xstructure& structure,const CCE_Variables& cce_vars); //ME20200213
  string print_output_cation_coordination_numbers(const xstructure& structure,xoption& cce_flags,CCE_Variables& cce_vars,vector<vector<uint> >& multi_anion_num_neighbors);
  string print_output_oxidation_numbers(const xstructure& structure,CCE_Variables& cce_vars);
  string print_output_corrections(const xstructure& structure,CCE_Variables& cce_vars,const vector<double>& enthalpy_formation_cell_cce);
  string print_test_output(CCE_Variables& cce_vars,const vector<double>& enthalpy_formation_cell_cce);
  string print_citation();
  // print user instructions
  string print_usage();
  // look up tables to load corrections and other data
  string get_corrections_line(const string& considered_anion_species,const string& cor_identifier);
  string get_corrections_line_O(const string& cor_identifier);
  string get_corrections_line_N(const string& cor_identifier);
  string get_corrections_line_I(const string& cor_identifier);  //GQ20240611
  string get_Bader_templates(const string& element);
  double get_ref_enthalpy_shift_pbe_u_icsd(const string& element);
}

#endif  // _AFLOW_CCE_H_

// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2023           *
// *           Aflow RICO FRIEDRICH - Duke University 2018-2021              *
// *                                                                         *
// ***************************************************************************
