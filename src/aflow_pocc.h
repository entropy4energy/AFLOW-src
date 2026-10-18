// ***************************************************************************
// *                                                                         *
// *         aflow - Automatic FLOW for materials discovery project          *
// *             Stefano Curtarolo - Duke University - 2003-2021             *
// *                 Kesong Yang - Duke University 2010-2011                 *
// *                  Corey Oses - Duke University 2013-2021                 *
// *                                                                         *
// ***************************************************************************
// aflow_pocc.h and aflow_pocc*.cpp*
//
// completely revised approach to KESONG YANG's original implementation
// no recursion needed --- too memory intensive
// issues with UFF (structure comparison), multiply occupied sites, and 
// vacancies are all addressed robustly here
// 2013-2019: corey.oses@duke.edu
// 2010-2011: kesong.yang@gmail.com (LEGACY)

#ifndef _AFLOW_POCC_H_
#define _AFLOW_POCC_H_

#include "APL/aflow_apl.h"
#include <set>  //CO20261005
#include <array>  //CO20261006

//precision defines tol and paddings
//const int _AFLOW_POCC_PRECISION_ = 8;    //moved to aflow.h
//tolerances
//const double _AFLOW_POCC_ZERO_TOL_ = pow(10,-_AFLOW_POCC_PRECISION_);  //moved to aflow.h

//uff param modes
const uint BOND_MODE    = 0;
const uint NONBOND_MODE = 1;

//files
const string AFLOW_POCC_TAG="[AFLOW_POCC]";

//elements
//to make POCC chemistry-independent, calculate UFF based on standard set of elements
//we should not need to have more than 30 species, but if you want to add to list
//I recommend adding IN ORDER so that elements become more different left to right (like periodic table)
const string STD_ELEMENTS_LIST="Sc Ti V Cr Mn Fe Co Ni Cu Zn "
"Y Zr Nb Mo Tc Ru Rh Pd Ag Cd "
"La Hf Ta W Re Os Ir Pt Au Hg ";

const int TEMPERATURE_PRECISION=2;  //not really going to explore more than 2000-3000K, looks weird if decimal is larger than non-decimal part of number //4;  //this is std::fixed

namespace pocc {
  /// @brief action of the symmetry of the colored parent on the sites of one supercell (ENUM)
  /// @note slot=site*n+i, i as in v_types_config[site][i]; the translations of the parent form G=Z_snf[0] x Z_snf[1] x Z_snf[2]
  /// @authors
  /// @mod{CO,20261005,created}
  struct POccSupercellAction {
    xmatrix<long long int> H;                   //HNF matrix (rows: superlattice vectors in the parent basis)
    xmatrix<long long int> V;                   //Smith normal form S=U*H*V: lattice vector x (row) goes to g=(x*V) mod snf
    long long int snf[3]={1,1,1};               //diagonal of S
    vector<xvector<long long int> > vx;         //lattice vector of each slot
    vector<vector<uint> > slot_of;              //slot of (site,group element)
    vector<vector<uint> > translations;         //translation by group element t: slot -> translations[t][slot]
    vector<bool> keeps_superlattice;            //per factor group operation of the colored parent
    vector<vector<uint> > perms;                //distinct permutations: translations after the operations that keep the superlattice
    vector<bool> perm_translation;              //pure translation other than the identity
    uint nrotations=0;                          //factor group operations that keep the superlattice
    uint gindex(const xvector<long long int>& x) const;
  };

  //CO20261006 - cluster correlation functions (aflow_pocc_correlations.cpp)
  typedef std::array<long long int,4> POccClusterPoint;  //site of xstr_nopocc, lattice vector (parent basis)
  struct POccClusterOrbit {
    string name;                                   //p1, p2, ..., t1, ...
    vector<POccClusterPoint> points;               //representative: its points, translation-normalized and sorted
    vector<vector<POccClusterPoint> > images;      //the representative under each factor group operation of the colored parent, point by point
    vector<vector<uint> > vperm;                   //permutations of the points by the operations that fix the cluster
    vector<vector<uint> > labels;                  //point functions (j_1,...,j_k), one per class of those permutations
    uint multiplicity=0;                           //clusters per parent cell
    uint shell=0;                                  //distance shell of the longest edge (1-based)
    double size=0.0;                               //longest edge (Angstrom)
    double perimeter=0.0;                          //sum of the edges (Angstrom)
  };
  double getPointFunction(uint nvalues,uint j,uint sigma);
  bool parsePOccSupercellTitle(const string& title,int& n,xmatrix<double>& hnf,vector<vector<int> >& decoration);
} // namespace pocc

namespace pocc {
  class POccCalculator; //forward declaration
  class POccSuperCellSet; //forward declaration

  string POCC_MINIMUM_CONFIGURATION(const aurostd::xoption& vpflow);
  string POCC_MINIMUM_CONFIGURATION(const string& directory="./");
  bool structuresGenerated(const string& directory=".");
  xstructure extractPARTCAR(const string& AflowIn);
  // Owns POCC supercell-name normalization, provenance-based pocc.out lookup, and weights.
  class SupercellDegeneracies {
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      SupercellDegeneracies();
      SupercellDegeneracies(const SupercellDegeneracies& b);
      //constructors - STOP
      ~SupercellDegeneracies();
      const SupercellDegeneracies& operator=(const SupercellDegeneracies& b);
      void clear();
      //NECESSARY PUBLIC CLASS METHODS - END
      bool load(const aflowlib::_aflowlib_entry& entry);
      double getWeight(const aflowlib::_aflowlib_entry& child,bool& found_weight) const;
    private:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const SupercellDegeneracies& b);
      //NECESSARY PRIVATE CLASS METHODS - END
      std::map<std::string,double> m_weights;
      static bool isASCIIDigit(char c);
      static bool isASCIIAlpha(char c);
      static bool isASCIIAlnum(char c);
      static std::string extractSupercellName(const std::string& text);
      static std::string normalizeSupercellName(const std::string& text);
      static void addUniquePath(std::vector<std::string>& paths,const std::string& path);
      static std::vector<std::string> getPoccOutPathCandidates(const aflowlib::_aflowlib_entry& entry);
  };

  vector<double> getVTemperatures(const string& temp_string);

  void parsePOccHashFromXStructureTitle(const string& title,string& pocc_hash);
  void parsePOccHashFromXStructureTitle(const string& title,string& pocc_hash,string& hnf_index_str,string& site_config_index_str);
  unsigned long long int getDGFromXStructureTitle(const string& title);
  void parsePropertyByTag(const string& line,const string& tag,double& prop);
  bool patchStructuresAllFile(const _aflags& aflags,string& structures_file,stringstream& structures_file_ss,ofstream& FileMESSAGE,ostream& oss);
  bool patchStructuresAllFile(stringstream& structures_file_ss);
  bool patchStructuresUniqueFile(const _aflags& aflags,string& structures_file,stringstream& structures_file_ss,ofstream& FileMESSAGE,ostream& oss);
  bool patchStructuresUniqueFile(stringstream& structures_file_ss);
  void patchStructuresFile(const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);

  void updateProgressBar(unsigned long long int current, unsigned long long int end, ostream& oss=cout);
  vector<string> getElementsList();

  class POccUnit;  //forward declaration
  class POccSiteConfiguration;    //forward declaration
  class UFFParamAtom;  //forward declaration
  struct UFFParamBond;  //forward declaration

  //bool sortAtoms(const _atom& a1,const _atom& a2);
  //bool getNextSiteConfiguration(vector<vector<POccSiteConfiguration> >& vv_count_configs,vector<uint>& v_config_order,vector<int>& v_config_iterators,vector<vector<int> >& v_types_config);
  bool getNextSiteConfiguration(vector<vector<POccSiteConfiguration> >& vv_count_configs,vector<int>& v_config_iterators,vector<vector<int> >& v_types_config);
  bool getNextSiteConfiguration(vector<uint>& v_config_order,vector<vector<int> >& v_types_config);
  bool getNextSiteConfiguration(vector<int>& site_config);
  unsigned long long int getDistinctPermutationsCount(const vector<int>& site_config);  //CO20261006 - how many configurations getNextSiteConfiguration() visits from site_config
  vector<int> getDistinctPermutation(const vector<int>& site_config,unsigned long long int rank);  //CO20261006 - the rank-th of them, without visiting the others
  vector<uint> getConfigOrder(vector<vector<int> >& v_types_config);
  //CO20261005 - ENUM (aflow_pocc_enumeration.cpp)
  bool areEquivalentSuperlattices(const xmatrix<double>& Bi,const xmatrix<double>& Bj,const vector<_sym_op>& pgroup);
  unsigned long long int rankMultisetPermutation(const vector<int>& seq,uint nvalues,unsigned long long int permutations);
  string getUFFParameterString(const string& element);
  vector<UFFParamAtom> getTypes2UFFParamsMap(const vector<string>& elements);
  xstructure createNonPOccStructure(const xstructure& xstr_pocc,const vector<POccUnit>& pocc_sites,bool use_automatic_volumes_in=true);             //convert pocc xstructure to non-pocc
  vector<POccUnit> getPOccSites(const xstructure& xstr_pocc,ostream& oss=cout);
  vector<POccUnit> getPOccSites(const xstructure& xstr_pocc,const _aflags& aflags,ostream& oss=cout);
  vector<POccUnit> getPOccSites(const xstructure& xstr_pocc,ofstream& FileMESSAGE,ostream& oss=cout);
  vector<POccUnit> getPOccSites(const xstructure& xstr_pocc,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
  vector<uint> getTypes2PCMap(const xstructure& xstr);

  bool sortPOccSites(const POccUnit& p1,const POccUnit& p2);
  bool sortPOccGroups(const POccUnit& p1,const POccUnit& p2);

  string getARUNString(const std::list<POccSuperCellSet>& l_supercell_sets,unsigned long long int i);
  string getARUNString(unsigned long long int index_structure_group,unsigned long long int vstructure_groups_size,unsigned long long int index_structure,unsigned long long int vstructures_size,unsigned long long int index_hnf,unsigned long long int index_site_config,bool include_strgrp=false);

  double getHmix(const xvector<double>& xv_energies,const xvector<double>& xv_dgs);
  double getHmix(const xvector<double>& xv_energies,const xvector<double>& xv_dgs,double& dg_total);
  double getEFA(const xvector<double>& xv_energies,const xvector<double>& xv_dgs);
  // Central POCC definition used by every DEED input backend.
  double getDEED(double entropy_forming_ability,double average_hull_loss);
  void setRepresentativeSpecies(const vector<string>& vspecies,xstructure& xstr);  //CO20261004 - species from vspecies when xstr names none
  void readRepresentativeStructure(const string& POSCAR_file,const vector<string>& vspecies,xstructure& xstr);  //CO20261004 - POSCAR-format file, then setRepresentativeSpecies()
  double RStarPerAtom(const xstructure& xstr);  //CO20261005 - R*: edge of the cube with the volume per atom
  double getParamGeo(double S_config,double delta);  //CO20261015 - S_config / delta^2, unset when not finite

  void poccOld2New(ostream& oss=cout);
  void poccOld2New(ofstream& FileMESSAGE,ostream& oss=cout);

  void POCC_Convolution(const aurostd::xoption& vpflow);

  string addDefaultPOCCTOL2string(const string& input);

  void getTemperatureStringParameters(int& temperature_precision,bool& temperatures_int,int& zero_padding_temperature);
  void getTemperatureStringParameters(const vector<double>& v_temperatures,int& temperature_precision,bool& temperatures_int,int& zero_padding_temperature);
  string getTemperatureString(double temperature,int precision,bool temperatures_int,int zero_padding_temperature);

  double poccDOSCAR2temperature(const string& doscar_path);
  
  // POCC sampling unit test - TL20251002
  namespace unittest {
    void deedTest(uint&, vector<vector<string> >&, vector<string>&);
    void poccSamplingTest(uint&, vector<vector<string> >&, vector<string>&);
    void poccEnumerationTest(uint&, vector<vector<string> >&, vector<string>&);  //CO20261005
    void poccSamplingSpaceTest(uint&, vector<vector<string> >&, vector<string>&);  //CO20261006
    void poccCorrelationsTest(uint&, vector<vector<string> >&, vector<string>&);  //CO20261006
  }
} // namespace pocc

namespace pocc {
  //POccUnit could be
  //1) a bunch of atoms on the same site (site)
  //2) within a site, a bunch of atoms with the same partial_occupation_value (group)
  //3) a bunch of atoms on the same equivalent site by xstr_sym.iatoms (equivalent group)
  class POccUnit: public xStream {
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      POccUnit(ostream& oss=cout);
      POccUnit(ofstream& FileMESSAGE,ostream& oss=cout);
      POccUnit(const _aflags& aflags,ostream& oss=cout);
      POccUnit(const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccUnit(const POccUnit& b);
      //constructors - STOP
      ~POccUnit();
      const POccUnit& operator=(const POccUnit& b);
      //bool operator<(const POccUnit& other) const;
      void clear();
      //NECESSARY PUBLIC CLASS METHODS - END

      bool m_initialized;
      _aflags m_aflags;
      uint site;                                        //reflexive
      bool partial_occupation_flag;
      double partial_occupation_value;
      vector<uint> v_occupants;
      vector<uint> v_types;
      vector<POccUnit> m_pocc_groups;
      bool is_inequivalent;                             //from iatoms of non-pocc structure
      uint equivalent;                                  //from iatoms of non-pocc structure

      //general setters
      bool initialize(ostream& oss);
      bool initialize(ofstream& FileMESSAGE,ostream& oss);
      bool initialize();
      bool initialize(const _aflags& aflags,ostream& oss);
      bool initialize(const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const _aflags& aflags);

      void setAFlags(const _aflags& aflags);
    private:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const POccUnit& b);
      //NECESSARY END CLASS METHODS - END
  };
}  // namespace pocc

namespace pocc {  
  //simple structure for sorting by vacancy count
  struct SiteVacancyCount {
    uint site;
    uint vacancy_count;
    bool operator<(const SiteVacancyCount& other) const;
  };

  struct StructureConfiguration {
    vector<POccSiteConfiguration> site_configs;
    double max_stoich_error;
    double max_site_error;
  };
} // namespace pocc

namespace pocc {
  class POccSuperCell {
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      POccSuperCell();
      POccSuperCell(const POccSuperCell& b);
      //constructors - STOP
      ~POccSuperCell();
      const POccSuperCell& operator=(const POccSuperCell& b);
      bool operator<(const POccSuperCell& other) const;
      void clear();
      //NECESSARY PUBLIC CLASS METHODS - END

      unsigned long long int m_hnf_index;
      unsigned long long int m_site_config_index;
      unsigned long long int m_degeneracy;
      double m_energy_uff;
    private:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const POccSuperCell& b);
      //NECESSARY END CLASS METHODS - END
  };
  bool sortPSCsUFFEnergy(const POccSuperCell& a, const POccSuperCell& b);
} // namespace pocc

namespace pocc {
  class POccSuperCellSet{
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      POccSuperCellSet();
      POccSuperCellSet(const POccSuperCellSet& b);
      //constructors - STOP
      ~POccSuperCellSet();
      const POccSuperCellSet& operator=(const POccSuperCellSet& b);
      bool operator<(const POccSuperCellSet& other) const;
      void clear();
      //NECESSARY PUBLIC CLASS METHODS - END

      vector<POccSuperCell> m_psc_set;
      double m_energy_dft;  //only calculate this for the set
      double m_probability;

      unsigned long long int getDegeneracy() const;
      const POccSuperCell& getSuperCell() const;
      double getHNFIndex() const; //ME20211006
      xmatrix<int> getHNFMatrix() const;
      double getSiteConfigIndex() const;
      vector<vector<int> > getSiteConfig() const; //ME20211006
      xstructure getStructure() const; //ME20211006
      double getUFFEnergy() const;
    private:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const POccSuperCellSet& b);
      //NECESSARY END CLASS METHODS - END
  };
  bool sortPSCSetsUFFEnergy(const POccSuperCellSet& a, const POccSuperCellSet& b);
} // namespace pocc

namespace pocc{
  class UFFParamAtom{
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      UFFParamAtom();
      UFFParamAtom(const UFFParamAtom& b);
      //constructors - STOP
      ~UFFParamAtom();
      const UFFParamAtom& operator=(const UFFParamAtom& b);
      void clear();
      //NECESSARY PUBLIC CLASS METHODS - END

      string symbol;
      double r1;        //bond distance
      double theta0;
      double x1;        //nonbond distance
      double D1;        //nonbond energy
      double zeta;      //scale
      double Z1;        //effective charge
      double Vi;
      double Uj;
      double ChiI;       //electronegativity
      double hard;
      double radius;
    private:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const UFFParamAtom& b);
      //NECESSARY END CLASS METHODS - END
  };

  struct UFFParamBond{
    //no need (YET) to make a class, simple ints and double, no real methods
    const UFFParamBond& operator=(const UFFParamBond& b);
    void calculate(UFFParamAtom& uffp1,UFFParamAtom& uffp2,double distij);

    double ren;
    double R0;
    double Kij;
    double Xij;
    double Dij;
    double delta;
    double X6;
    double X12;
  };
} // namespace pocc

namespace pocc {
  class POccSiteConfiguration {
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      POccSiteConfiguration();
      POccSiteConfiguration(int _site,int _i_hnf,vector<POccUnit>& pocc_groups);
      POccSiteConfiguration(const POccSiteConfiguration& b);
      //constructors - STOP

      ~POccSiteConfiguration();
      const POccSiteConfiguration& operator=(const POccSiteConfiguration& b);
      void clear();
      //NECESSARY PUBLIC CLASS METHODS - END

      //debug
      //vector<int> types_configuration_debug;                  //atom types, vacancy is -1
      //debug

      int site;                                         //reflexive
      int i_hnf;                                        //reflexive
      bool partial_occupation_flag;
      //any vector or xvector is over pocc_groups
      vector<POccUnit> m_pocc_groups;                  //reflexive
      xvector<int> xv_occupation_count_input;      //pre multiplication, from xstr_pocc
      xvector<int> xv_occupation_multiple;              //increment with each pocc_group
      xvector<int> xv_occupation_count_supercell;           //post multiplication with multiple
      xvector<double> xv_partial_occupation_value;
      xvector<double> xv_site_error;
      //sum of occupation_count_total and vacancy_count yields i_hnf (total sites)
      int occupation_count_total;
      int vacancy_count;
      double max_site_error;
      //double error_total;

      void prepareNoPOccConfig();
      void preparePOccConfig();
      int getNextOccupationMultiple(int i_hnf,xvector<int>& xv_occupation_multiple);
      int calculateOccupationCountTotal(xvector<int>& xv_next_occupation_multiple);
      void updateOccupationCounts(int _i_hnf, xvector<int> & xv_next_occupation_multiple);
      void calculateError();
      bool isPartiallyOccupied() const;
      vector<int> getStartingTypesConfiguration() const;

      //const vector<int>& getTypesConfiguration() const;
    private:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const POccSiteConfiguration& b);
      //NECESSARY END CLASS METHODS - END
  };
} // namespace pocc

namespace pocc {
  class POccCalculatorTemplate {
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      POccCalculatorTemplate();
      POccCalculatorTemplate(const POccCalculatorTemplate& b);
      //constructors - STOP
      ~POccCalculatorTemplate();
      //NECESSARY PUBLIC CLASS METHODS - END

      xstructure xstr_pocc;                   //input from PARTCAR
      aurostd::xoption m_p_flags;             //e.g., vpflow
      _aflags m_aflags;                       //standard aflow flags
      xvector<double> stoich_each_type;       //converting deque<double> to xvector<double>
      xstructure xstr_nopocc;                 //will contain symmetry objects (_sym_op, pgroups most important here)
      vector<uint> types2pc_map;              //list of atom indices where types2pc_map(0) is 1st type 0 atom, types2pc_map(1) is 1st type 1 atom, etc.
      vector<string> m_species_redecoration;  //species used to redecorate xstr's for consistent symmetry/uff energies

      void setPOccFlags(const aurostd::xoption& pocc_flags);
      void setAFlags(const _aflags& Aflags);                      //standard _aflags
      void setPOccStructure(const xstructure& xstr_pocc);
      void setNonPOccStructure(const xstructure& xstr_nonpocc);
      void setSpeciesRedecoration(const vector<string>& species_redecoration);

    protected:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const POccCalculatorTemplate& b);
      //NECESSARY PRIVATE CLASS METHODS - END
  };
} // namespace pocc

namespace pocc {
  vector<uint> getVacanciesSuperCell(const vector<int>& pc2sc_map,const vector<vector<int> >& v_types_config);
  void replaceRandomSitesSuperCell(const xstructure& xstr_pocc,const vector<uint>& types2pc_map,const vector<int>& pc2sc_map,const vector<vector<int> >& v_types_config,xstructure& supercell);
  void rebuildSuperCell(const xstructure& xstr_pocc,const vector<uint>& v_vacancies,xstructure& supercell);
} // namespace pocc

namespace pocc { 
  class POccUFFEnergyAnalyzer: public POccCalculatorTemplate, public xStream {
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      POccUFFEnergyAnalyzer(ostream& oss=cout);
      POccUFFEnergyAnalyzer(const aurostd::xoption& pocc_flags,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const _aflags& aflags,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const aurostd::xoption& pocc_flags,const _aflags& aflags,ostream& oss=cout);
      POccUFFEnergyAnalyzer(ofstream& FileMESSAGE,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const aurostd::xoption& pocc_flags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const aurostd::xoption& pocc_flags,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const _aflags& aflags,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags,const _aflags& aflags,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,ofstream& FileMESSAGE,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccUFFEnergyAnalyzer(const POccUFFEnergyAnalyzer& b);
      //POccUFFEnergyAnalyzer(xmatrix<double>*& _hnf_mat,xstructure*& _xstr_nopocc);
      //constructors - STOP
      ~POccUFFEnergyAnalyzer();
      const POccUFFEnergyAnalyzer& operator=(const POccUFFEnergyAnalyzer& b);
      void clear();
      //NECESSARY PUBLIC CLASS METHODS - END

      //underlying data structures
      bool m_initialized;
      xmatrix<double> hnf_mat;
      vector<vector<int> > m_types_config;            //the config for which we determined bonding
      vector<UFFParamAtom> types2uffparams_map;
      vector<uint> m_vacancies;
      double m_exploration_radius;
      xmatrix<double> distance_matrix;                    //references xstr_nopocc
      vector<double> v_dist_nn;                           //references xstr_nopocc
      xstructure xstr_ss;       //superstructure
      vector<int> sc2pc_map;
      vector<int> pc2sc_map;

      //initializers
      bool initialize(ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,ostream& oss);
      bool initialize(const _aflags& aflags,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,ostream& oss);
      bool initialize(ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize();
      bool initialize(const aurostd::xoption& pocc_flags);
      bool initialize(const _aflags& aflags);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const _aflags& aflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags,const _aflags& aflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const _aflags& aflags);
      bool initialize(const xstructure& xstr_pocc,const xstructure& xstr_nopocc,const vector<string>& species_redecoration,const aurostd::xoption& pocc_flags,const _aflags& aflags);

      void setSpeciesRedecoration(const vector<string>& species_redecoration);
      void setExplorationRadius();
      void getCluster(xmatrix<double>& _hnf_mat);
      void setBonds(const vector<vector<int> >& v_types_config);
      double getUFFEnergy();
      bool isVacancy(vector<uint>& v_vacancies,uint atom);
      double bondEnergyBond(const UFFParamBond& uffb);
      double bondEnergyNoBond(const UFFParamBond& uffb);
      double getUFFBondEnergy(xstructure& xstr,vector<vector<uint> >& v_bonded_atom_indices,uint MODE);


    private:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const POccUFFEnergyAnalyzer& b);
      //NECESSARY PRIVATE CLASS METHODS - END

      //for bonding, we need to create a super-superstructure (cluster)
      xstructure xstr_cluster;                                //cluster of atoms within radius
      vector<vector<uint> > v_bonded_atom_indices;            //references xstr_cluster
      vector<vector<uint> > v_nonbonded_atom_indices;         //references xstr_cluster

      bool has_vacancies;                                     //are there vacancies present in m_types_config?
      bool bonding_set;                                       //have we already found bonding for this configuration?
      double m_energy_uff;

      uint NNDistancesMapPC(uint atom);
      uint NNDistancesMapSC(uint atom);
      void calculateNNDistances(xstructure& xstr,vector<uint>& v_vacancies);
  };
} // namespace pocc

namespace pocc {
  struct GroupMember {
    int basis;  //basis is int in xatom
    xvector<double> cpos;
  };
}

namespace pocc {
  class POccCalculator : public POccCalculatorTemplate, public xStream {
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      POccCalculator(ostream& oss=cout);
      POccCalculator(const _aflags& aflags,ostream& oss=cout);
      POccCalculator(const _aflags& aflags,const _kflags& kflags,ostream& oss=cout);
      POccCalculator(const _aflags& aflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _aflags& aflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _kflags& kflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _kflags& kflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _aflags& aflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const _aflags& aflags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const _aflags& aflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _aflags& aflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,const _aflags& aflags,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccCalculator(const POccCalculator& b);
      //constructors - STOP
      ~POccCalculator();
      const POccCalculator& operator=(const POccCalculator& b);
      void clear();
      //NECESSARY PUBLIC CLASS METHODS - END

      //inputs
      bool m_initialized;
      _kflags m_kflags;                         //standard aflow flags
      _vflags m_vflags;                         //standard aflow flags
      xstructure xstr_sym;                    //will contain symmetry objects (_sym_op, pgroups most important here)
      int n_hnf;
      vector<POccUnit> m_pocc_sites;                   //groupings of atoms that are on the same site, non-vacant sites only, relative to xstr_nopocc
      int pocc_atoms_total;
      vector<StructureConfiguration> v_str_configs;

      POccUFFEnergyAnalyzer energy_analyzer;
      unsigned long long int hnf_count;
      unsigned long long int types_config_permutations_count;
      unsigned long long int total_permutations_count;
      std::list<POccSuperCellSet> l_supercell_sets;
      //CO20261005 - ENUM (aflow_pocc_enumeration.cpp)
      xstructure m_xstr_color;                                    //parent with one species per site occupation
      vector<xmatrix<double> > m_vhnf_enum;                      //HNF matrices of ENUM
      vector<unsigned long long int> m_vhnf_multiplicity;         //count of HNF matrices each listed one stands for
      unsigned long long int m_hnf_count_lattice;                 //listed HNF matrices the lattice point group gives (they come first)
      vector<unsigned long long int> m_enum_group_order;         //per HNF matrix: order of the permutation group
      vector<unsigned long long int> m_enum_rotations;           //per HNF matrix: factor group operations that keep the superlattice
      vector<unsigned long long int> m_enum_orbits;              //per HNF matrix: unique supercells
      vector<unsigned long long int> m_enum_superperiodic;       //per HNF matrix: unique supercells with a smaller period
      unsigned long long int m_enum_merged_hnf;                  //decorations of superperiodic supercells found again by operations that change the superlattice (same HNF matrix)
      unsigned long long int m_enum_merged_across;               //decorations of superperiodic supercells found again on other HNF matrices
      unsigned long long int m_enum_skipped;                     //decorations outside the sampled orbits (--pocc_sample_*)
      vector<_sym_op> m_vpgroup_enum;                            //point group of the factor group operations that keep the structure configurations
      vector<unsigned long long int> m_vhnf_lattice_class;        //per listed HNF matrix: its class under the lattice point group (H index of the UFF enumerator)
      uint m_enum_fgroup_dropped;                                //factor group operations of the colored parent that change the structure configurations
      double m_enum_seconds;
      //CO20261006 - cluster correlation functions (aflow_pocc_correlations.cpp)
      vector<POccClusterOrbit> m_corr_orbits;                    //pair and triplet orbits of the partially occupied sites
      vector<vector<int> > m_corr_site_values;                   //per site of xstr_nopocc: species (decoration values, vacancy -1) in point-function order; empty if it does not mix
      vector<vector<double> > m_corr_site_conc;                  //per site: concentration of each of those species
      vector<string> m_vsupercell_titles;                        //post-processing: POSCAR title of each supercell (HNF matrix and decoration)
      vector<vector<double> > m_vsupercell_corr;                 //post-processing: correlation functions of each supercell
      vector<vector<vector<double> > > m_vsupercell_pair_probabilities;  //post-processing: per supercell and pair orbit, probability of each species pair
      //standard flags - ALL options will be handled via xoptions
      aurostd::xoption enumerator_mode;       //how do we determine duplicates - UFF, SNF, ... //CO20261005 - set to UFF, then ENUM, by compareEnumerators(); otherwise empty

      //post-processing
      bool m_convolution;
      bool m_count_unique_fast;
      vector<string> m_ARUN_directories;
      double m_Hmix; // ST20250701 this is from DFT
      double m_Hmix_miedema; // ST20250701 regular solution model https://doi.org/10.1016/S0921-5093(00)01446-5
      double m_efa;
      //CO20250321 START - other high-entropy descriptors
      //see DOI:10.1063/5.0122727
      double m_S_config_cell;       //no units
      double m_S_config_atom;       //no units
      double m_S_config_rfu;        //no units; reduced formula unit: e.g., M3Si6 becomes M1Si2; _S_config_atom*n_atoms_rfu
      double m_S_config_sublattice; //no units; _S_config_atom*n_iatoms; also called entropy metric here: DOI:10.1016/j.scriptamat.2021.113974
      double m_S_config_species;    //no units; considers nary of original/ordered prototype; _S_config_atom *2(binary) || *3(ternary) etc.

      double m_S_config_partial_atom; // ST20250530 Like m_S_config_atom, but separately treat all metal sites the same and all non-metal sites the same.
      double m_k2; // ST20250530 for calculating m_kcr1
      double m_kcr1; // ST20250530 Thermodynamic parameter from https://doi.org/10.1016/j.jallcom.2015.10.279

      uint m_nsublattices_disordered;   //number of disordered sublattices, should match size of vectors below if properties are available for those species
      vector<string> m_nsublattices_disordered_names; // ST20250604 names of disordered sites, like A*0.5,B*0.5
      //see DOI:10.1016/S1002-0071(12)60080-X
      vector<double> m_delta_electronegativity_Pauling_sublattice; //dimensionless
      vector<double> m_delta_electronegativity_Pearson_sublattice; //eV
      vector<double> m_delta_electronegativity_Ghosh_sublattice; //eV
      vector<double> m_delta_electronegativity_Allen_sublattice; //dimensionless
      //see:
      //  delta_radius_XX_sublattice: DOI:10.1107/S0567739476001551; sublattice here is different than for S_config; here it is calculated per disordered sublattice/site, hence a vector
      //  S_excess_XX: DOI:10.1063/1.1675048
      //  param_geo_XX: DOI:10.1016/j.intermet.2014.04.019
      vector<double> m_delta_radius_Saxena_sublattice; //dimensionless
      //
      vector<double> m_param_geo_radius_Saxena_cell;       //no units
      vector<double> m_param_geo_radius_Saxena_atom;       //no units
      vector<double> m_param_geo_radius_Saxena_rfu;        //no units; reduced formula unit: e.g., M3Si6 becomes M1Si2; _param_geo_radius_Saxena_atom*n_atoms_rfu
      vector<double> m_param_geo_radius_Saxena_sublattice; //no units; _param_geo_radius_Saxena_atom*n_iatoms; also called entropy metric here: DOI:10.1016/j.scriptamat.2021.113974
      vector<double> m_param_geo_radius_Saxena_species;    //no units; considers nary of original/ordered prototype; _param_geo_radius_Saxena_atom *2(binary) || *3(ternary) etc.

      vector<double> m_delta_radius_PT_sublattice; //dimensionless
      //
      //
      vector<double> m_param_geo_radius_PT_cell;       //no units
      vector<double> m_param_geo_radius_PT_atom;       //no units
      vector<double> m_param_geo_radius_PT_rfu;        //no units; reduced formula unit: e.g., M3Si6 becomes M1Si2; _param_geo_radius_PT_atom*n_atoms_rfu
      vector<double> m_param_geo_radius_PT_sublattice; //no units; _param_geo_radius_PT_atom*n_iatoms; also called entropy metric here: DOI:10.1016/j.scriptamat.2021.113974
      vector<double> m_param_geo_radius_PT_species;    //no units; considers nary of original/ordered prototype; _param_geo_radius_PT_atom *2(binary) || *3(ternary) etc.

      vector<double> m_delta_radius_covalent_PT_sublattice; //dimensionless
      //
      //
      vector<double> m_param_geo_radius_covalent_PT_cell;       //no units
      vector<double> m_param_geo_radius_covalent_PT_atom;       //no units
      vector<double> m_param_geo_radius_covalent_PT_rfu;        //no units; reduced formula unit: e.g., M3Si6 becomes M1Si2; _param_geo_radius_covalent_PT_atom*n_atoms_rfu
      vector<double> m_param_geo_radius_covalent_PT_sublattice; //no units; _param_geo_radius_covalent_PT_atom*n_iatoms; also called entropy metric here: DOI:10.1016/j.scriptamat.2021.113974
      vector<double> m_param_geo_radius_covalent_PT_species;    //no units; considers nary of original/ordered prototype; _param_geo_radius_covalent_PT_atom *2(binary) || *3(ternary) etc.

      vector<double> m_delta_radius_covalent_sublattice; //dimensionless
      //
      //
      vector<double> m_param_geo_radius_covalent_cell;       //no units
      vector<double> m_param_geo_radius_covalent_atom;       //no units
      vector<double> m_param_geo_radius_covalent_rfu;        //no units; reduced formula unit: e.g., M3Si6 becomes M1Si2; _param_geo_radius_covalent_atom*n_atoms_rfu
      vector<double> m_param_geo_radius_covalent_sublattice; //no units; _param_geo_radius_covalent_atom*n_iatoms; also called entropy metric here: DOI:10.1016/j.scriptamat.2021.113974
      vector<double> m_param_geo_radius_covalent_species;    //no units; considers nary of original/ordered prototype; _param_geo_radius_covalent_atom *2(binary) || *3(ternary) etc.

      vector<double> m_delta_radius_VanDerWaals_PT_sublattice; //dimensionless
      //
      //
      vector<double> m_param_geo_radius_VanDerWaals_PT_cell;       //no units
      vector<double> m_param_geo_radius_VanDerWaals_PT_atom;       //no units
      vector<double> m_param_geo_radius_VanDerWaals_PT_rfu;        //no units; reduced formula unit: e.g., M3Si6 becomes M1Si2; _param_geo_radius_VanDerWaals_PT_atom*n_atoms_rfu
      vector<double> m_param_geo_radius_VanDerWaals_PT_sublattice; //no units; _param_geo_radius_VanDerWaals_PT_atom*n_iatoms; also called entropy metric here: DOI:10.1016/j.scriptamat.2021.113974
      vector<double> m_param_geo_radius_VanDerWaals_PT_species;    //no units; considers nary of original/ordered prototype; _param_geo_radius_VanDerWaals_PT_atom *2(binary) || *3(ternary) etc.

      vector<double> m_delta_radii_Ghosh08_sublattice; //dimensionless
      //
      //
      vector<double> m_param_geo_radii_Ghosh08_cell;       //no units
      vector<double> m_param_geo_radii_Ghosh08_atom;       //no units
      vector<double> m_param_geo_radii_Ghosh08_rfu;        //no units; reduced formula unit: e.g., M3Si6 becomes M1Si2; _param_geo_radii_Ghosh08_atom*n_atoms_rfu
      vector<double> m_param_geo_radii_Ghosh08_sublattice; //no units; _param_geo_radii_Ghosh08_atom*n_iatoms; also called entropy metric here: DOI:10.1016/j.scriptamat.2021.113974
      vector<double> m_param_geo_radii_Ghosh08_species;    //no units; considers nary of original/ordered prototype; _param_geo_radii_Ghosh08_atom *2(binary) || *3(ternary) etc.

      vector<double> m_delta_radii_Slatter_sublattice; //dimensionless
      //
      //
      vector<double> m_param_geo_radii_Slatter_cell;       //no units
      vector<double> m_param_geo_radii_Slatter_atom;       //no units
      vector<double> m_param_geo_radii_Slatter_rfu;        //no units; reduced formula unit: e.g., M3Si6 becomes M1Si2; _param_geo_radii_Slatter_atom*n_atoms_rfu
      vector<double> m_param_geo_radii_Slatter_sublattice; //no units; _param_geo_radii_Slatter_atom*n_iatoms; also called entropy metric here: DOI:10.1016/j.scriptamat.2021.113974
      vector<double> m_param_geo_radii_Slatter_species;    //no units; considers nary of original/ordered prototype; _param_geo_radii_Slatter_atom *2(binary) || *3(ternary) etc.

      vector<double> m_delta_radii_Pyykko_sublattice; //dimensionless
      //
      //
      vector<double> m_param_geo_radii_Pyykko_cell;       //no units
      vector<double> m_param_geo_radii_Pyykko_atom;       //no units
      vector<double> m_param_geo_radii_Pyykko_rfu;        //no units; reduced formula unit: e.g., M3Si6 becomes M1Si2; _param_geo_radii_Pyykko_atom*n_atoms_rfu
      vector<double> m_param_geo_radii_Pyykko_sublattice; //no units; _param_geo_radii_Pyykko_atom*n_iatoms; also called entropy metric here: DOI:10.1016/j.scriptamat.2021.113974
      vector<double> m_param_geo_radii_Pyykko_species;    //no units; considers nary of original/ordered prototype; _param_geo_radii_Pyykko_atom *2(binary) || *3(ternary) etc.
      
      //in the future, make m_delta_XX_bond
      //
      //CO20250321 END - other high-entropy descriptors

      // ST20250528 START More HEA descriptors
      double m_solubility_parameter; // GPa * A^3
      vector<double> m_vr_star;  //CO20261004 - R* of each representative (Angstrom): edge of the cube with its volume per atom (CO20261005); empty until setRStar()
      double m_Omega;
      double m_VEC;
      double m_lat_distortion_relax_distance_stdev; // ST20250607 lattice distortion
      double m_lat_distortion_relax_distance_mean; // ST20250610 lattice distortion
      double m_lat_distortion_misfit_stdev; // ST20250607 lattice distortion
      double m_lat_distortion_misfit_mean; // ST20250610 lattice distortion
      // same size as supercell sets minus some which can't be calculated
      vector<uint> m_vlat_distortion_supercells_accounted_for;
      vector<double> m_vlat_distortion_accounted_for_probs;
      vector<structure_mapping_info> m_vmisfit_info; // ST20250610 lattice distortion
      vector<double> m_vlat_distortion_misfits; // ST20250610 lattice distortion
      vector<double> m_vlat_distortion_relax_distances; // ST20250610 lattice distortion
      // ST20250528 END More HEA descriptors
      int m_temperature_precision;
      int m_zero_padding_temperature;
      bool m_temperatures_int;
      uint m_relaxation_max;
      double m_energy_dft_ground;
      uint m_ARUN_directory_ground;
      aurostd::xmatrix<double> m_rdf_all;
      double m_rdf_rmax;
      int m_rdf_nbins;
      xDOSCAR m_xdoscar;
      vector<double> m_Egap_DOS,m_Egap;
      double m_Egap_DOS_net,m_Egap_net;
      vector<string> m_vfilenames_plasm;  //plasmonics
      vector<string> m_veps_plasm; //plasmonics
      vector<xPLASMONICS> m_vxplasm;  //plasmonics
      //vector<vector<double> > m_venergy_plasm; //plasmonics
      //vector<vector<double> > m_veels_plasm;   //plasmonics
      //vector<vector<xcomplex<double> > > m_vdielectric_plasm;  //plasmonics  //contains both real and imaginary parts

      //initializers
      bool initialize(ostream& oss);
      bool initialize(const _aflags& aflags,ostream& oss);
      bool initialize(const _aflags& aflags,const _kflags& kflags,ostream& oss);
      bool initialize(const _aflags& aflags,const _vflags& vflags,ostream& oss);
      bool initialize(const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _kflags& kflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _vflags& vflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _kflags& kflags,const _vflags& vflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,const _vflags& vflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ostream& oss);
      bool initialize(ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const _aflags& aflags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const _aflags& aflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize();
      bool initialize(const _aflags& aflags);
      bool initialize(const _aflags& aflags,const _kflags& kflags);
      bool initialize(const _aflags& aflags,const _vflags& vflags);
      bool initialize(const _aflags& aflags,const _kflags& kflags,const _vflags& vflags);
      bool initialize(const xstructure& xstr_pocc);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags);
      bool initialize(const xstructure& xstr_pocc,const _kflags& kflags);
      bool initialize(const xstructure& xstr_pocc,const _vflags& vflags);
      bool initialize(const xstructure& xstr_pocc,const _kflags& kflags,const _vflags& vflags);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,const _vflags& vflags);
      bool initialize(const xstructure& xstr_pocc,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags);
      bool initialize(const aurostd::xoption& pocc_flags,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _vflags& vflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags,const _vflags& vflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const aurostd::xoption& pocc_flags);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags);
      bool initialize(const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _vflags& vflags);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _kflags& kflags,const _vflags& vflags);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _vflags& vflags);
      bool initialize(const xstructure& xstr_pocc,const aurostd::xoption& pocc_flags,const _aflags& aflags,const _kflags& kflags,const _vflags& vflags);

      //external methods
      void setPOccFlags(const aurostd::xoption& pocc_flags);                    //input flags, e.g., vpflow
      void loadFromAFlags();                                                    //grabs from m_aflags
      void loadFromAFlags(const aurostd::xoption& loader);                      //grabs from m_aflags
      void setPOccStructure(const xstructure& xstr_pocc);
      void setAFlags(const _aflags& Aflags);                                    //standard _aflags
      void setKFlags(const _kflags& Kflags);                                    //standard _kflags
      void setVFlags(const _vflags& Vflags);                                    //standard _vflags

      //path getters
      string getARUNDirectoryPath(uint isupercell) const;
      string getOutputPath() const;


      // ST20250602 format property
      void formatProperty(stringstream& ss, vector<double> value, string name, uint32_t idx) const;
      void formatProperty(stringstream& ss, double value, string name) const;

      // Modules
      bool inputFilesFoundAnywhereAPL();  //ME20211004
      void createModuleAflowIns(const _xvasp& xvasp, const string& MODULE);  //ME20211004

      void writePARTCAR() const;
      void generateStructures(const _xvasp& xvasp);
      xstructure createXStructure(const POccSuperCell& psc,int n_hnf=0,unsigned long long int hnf_count=0,unsigned long long int types_config_permutations_count=0,bool clean_structure=false,bool primitivize=false);
      bool areEquivalentStructuresByUFF(std::list<POccSuperCellSet>::iterator it, const POccSuperCell& psc) const;
      void add2DerivativeStructuresList(const POccSuperCell& psc,std::list<POccSuperCellSet>::iterator i_start,std::list<POccSuperCellSet>::iterator i_end);
      void add2DerivativeStructuresList(const POccSuperCell& psc);
      void getHNFMatSiteConfig(const POccSuperCell& psc,xmatrix<double>& hnf_mat,vector<vector<int> >& v_types_config);
      bool areEquivalentStructures(const POccSuperCell& psc_a,const POccSuperCell& psc_b);
      bool areEquivalentStructures(const xstructure& a,const xstructure& b);
      unsigned long long int runRobustStructureComparison(std::list<POccSuperCellSet>::iterator it);
      void calculateHNF();
      void getTotalPermutationsCount();
      void calculatePOccSuperCellUFF(int thread_id, vector<POccSuperCell>& vpsc, const vector<POccUFFEnergyAnalyzer>& v_energy_analyzer, const vector<vector<vector<int>>>& vv_types_config, size_t& npsc_queue, std::mutex& m_save, std::mutex& m_job); //SD20230604
      void countUniquePOccSuperCellUFF(int thread_id, std::map<unsigned long long int, std::unordered_map<unsigned long int, unsigned long int>>& map_unique, const vector<POccSuperCell>& vpsc, const vector<POccUFFEnergyAnalyzer>& v_energy_analyzer, const vector<vector<vector<int>>>& vv_types_config, size_t& npsc_queue, std::mutex& m_save, std::mutex& m_job); //SD20230609
      void calculate();
      //CO20261005 - ENUM (aflow_pocc_enumeration.cpp)
      string getStructureGenerationAlgo() const;  //UFF (default), ENUM or COMPARE
      void prepareEnumeration();
      void calculateEnum();
      void compareEnumerators();
      void getSupercellAction(const xmatrix<double>& hnf,POccSupercellAction& action) const;
      vector<bool> getDecorationPeriods(const POccSupercellAction& action,const vector<vector<int> >& v_types_config) const;
      bool pullBackDecoration(const POccSupercellAction& src,const vector<bool>& periods,const POccSupercellAction& dst,uint iop,const vector<vector<int> >& v_types_config,vector<vector<int> >& image) const;
      void getEquivalentDecorations(const POccSupercellAction& src,const vector<bool>& periods,const POccSupercellAction& dst,bool same_superlattice,const vector<vector<int> >& v_types_config,std::set<unsigned long long int>& ranks) const;
      unsigned long long int rankDecoration(const vector<vector<int> >& v_types_config) const;
      //CO20261006 - cluster correlation functions (aflow_pocc_correlations.cpp)
      void setCorrelationClusters(uint pair_shells=0,uint triplet_shells=0);  //0: DEFAULT_POCC_CORRELATION_PAIR_SHELLS and _TRIPLET_SHELLS
      vector<string> getCorrelationLabels() const;
      vector<double> getRandomCorrelations() const;
      vector<double> getCorrelations(const POccSupercellAction& action,const vector<vector<int> >& decoration,vector<vector<double> >& vpair_probabilities) const;
      bool calculateSupercellCorrelations();  //post-processing, from the supercell titles
      void writeCorrelations(stringstream& pocc_out_ss,bool ensemble) const;
      bool getWarrenCowleyPairs(const POccClusterOrbit& orbit,vector<std::pair<uint,uint> >& vst) const;
      vector<string> getWarrenCowleyLabels() const;
      vector<double> getWarrenCowley(const vector<vector<double> >& vpair_probabilities) const;
      string getCorrelationSpeciesName(int value) const;
      POccClusterPoint applyFactorGroup(uint iop,const POccClusterPoint& point) const;
      xvector<double> getClusterPointPosition(const POccClusterPoint& point) const;
      void writePreProcessing(const _xvasp& in_xvasp);
      string getARUNString(unsigned long long int i);
      xstructure getUniqueSuperCell(unsigned long long int i);
      vector<xstructure> getUniqueDerivativeStructures();
      vector<uint> getMapToPARTCAR(unsigned long long int i, const xstructure& xstr);  //ME20211006
      unsigned long long int getUniqueSuperCellsCount() const;
      //bool printUniqueDerviativeStructures();
      //void resetMaxSLRadius();
      void resetHNFMatrices();
      void resetSiteConfigurations();

      void CleanPostProcessing();
      void convolution();
      void loadDataIntoCalculator();
      void setTemperatureStringParameters();
      void setTemperatureStringParameters(vector<double>& v_temperatures);
      void postProcessing();
      void StructuresAllFile2SupercellSets();
      void StructuresUniqueFile2SupercellSets();
      bool QMVASPsFound() const;
      void setDFTEnergies();
      void setEFA();
      void setSConfig();  //CO20250321
      void setHmixMiedema(); // ST20250701
      void setDeltaProperties();  //CO20250321
      void setSolubilityParameter(double temperature=300); // ST20250528
      bool setRStar();  //CO20261004 - R* of every representative from its relaxed structure, once
      void setThermoParameter(double temperature=300); // ST20250530
      void setLatticeDistortion(double temperature=300); // ST20250607
      void setMisfitParams(); // ST20250607
      void setOmega(); // ST20250529
      void setVEC();
      void calculateRELAXProperties(double temperature=300);
      void calculateSTATICProperties(double temperature=300);
      void calculatePlasmonicProperties(double temperature=300);
      void setPOccStructureProbabilities(double temperature=300); //room temperature
      string getTemperatureString(double temperature) const;
      void setAvgRDF(double temperature=300);  //depends on probabilities
      void setAvgDOSCAR(double temperature=300);  //depends on probabilities
      void setAvgPlasmonicData(double temperature=300);  //depends on probabilities
      void plotAvgDOSCAR(double temperature) const; //no default temperature, needs to be set inside setAvgDOSCAR()
      void plotAvgDOSCAR(const string& doscar_path,const string& directory=".") const;
      void plotAvgDOSCAR(const xDOSCAR& xdos,double temperature,const string& directory=".") const;
      void writeResults() const;
      void writeResults(double temperature) const;
      //YL20240508 for pocc_sampling
      double setPOccSampleRate(const string& pocc_sample_rate_string, int sample_round);   
      vector<unsigned long long int> FirstRandomSampling(unsigned long long int hnf_count, unsigned long long int types_config_permutations_count);
      vector<unsigned long long int> FirstRandomSamplingWholeSpace() const;  //CO20261006 - SAMPLE_NUMBER and SAMPLE_HASH: hnf_index*types_config_permutations_count+site_config_index, drawn from every configuration
      vector<vector<int> > getSiteConfiguration(unsigned long long int site_config_index) const;  //CO20261006 - the configuration getNextSiteConfiguration() reaches at site_config_index
      std::list<pocc::POccSuperCellSet> SecondRandomSamplingWithRate(std::list<pocc::POccSuperCellSet> l_supercell_sets, unsigned long long int hnf_count);
      std::list<pocc::POccSuperCellSet> SecondRandomSampling(std::list<pocc::POccSuperCellSet> l_supercell_sets);

      // ST20250612 helper function for lattice distortion
      std::map<uint, double> getAtomDisplacement(const xstructure& xstr_orig, const xstructure& xstr_relaxed);
    private:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const POccCalculator& b);
      //NECESSARY END CLASS METHODS - END

      bool usePOccSampleNumber(string& sample_number_string, unsigned long long int& sample_number) const;  //TL20260204
      bool usePOccSampleRate(string& sample_rate_string) const;  //TL20260205
      bool usePOccSampleHash(string& sample_hash_string, unsigned long long int& sample_hash_hnf, unsigned long long int& sample_hash_site_config) const;  //TL20260303
      bool usePOccSkipSiteConfigurations() const; //TL20260310

      //CO20261005 - ENUM (aflow_pocc_enumeration.cpp)
      bool m_enum_prepared;
      bool m_hnf_from_list;                                       //iterateHNFMatrix() serves m_vhnf_enum
      bool m_hnf_reduce;                                          //iterateHNFMatrix() reduces by the lattice point group (default) or lists every HNF matrix
      unsigned long long int m_ihnf_list;
      vector<uint> m_site_colors;                                 //occupation (color) of each site of xstr_nopocc
      vector<vector<uint> > m_vfgroup_site_map;                   //factor group operation: site -> site
      vector<vector<xvector<long long int> > > m_vfgroup_site_shift;  //factor group operation: lattice vector added to the site
      vector<vector<uint> > m_vfgroup_site_inverse;               //factor group operation: site -> site it comes from
      vector<xmatrix<long long int> > m_vfgroup_Uf;               //factor group operation: Uf
      vector<xmatrix<long long int> > m_vfgroup_Uf_inv;           //factor group operation: Uf^-1
      uint m_rank_values;                                         //species types + vacancy
      vector<vector<uint> > m_rank_order;                         //per structure configuration: site order of getNextSiteConfiguration()
      vector<vector<unsigned long long int> > m_rank_site_count;  //per structure configuration: permutations of each site
      vector<unsigned long long int> m_rank_offset;               //per structure configuration: first site_config_index
      vector<string> m_rank_key;                                  //per structure configuration: per-site species counts
      vector<vector<vector<uint> > > m_rank_counts;               //per structure configuration and site: count of each value (vacancy first)
      unsigned long long int m_rank_total;
      void setColoredParent();
      void setEnumHNFMatrices();
      void setDecorationRanking();
      string getDecorationCountsKey(const vector<vector<int> >& v_types_config) const;

      //hnf matrices
      int a_start, b_start, c_start;
      int d_start, e_start, f_start;
      vector<xmatrix<double> > v_unique_superlattices;
      //double max_superlattice_radius;
      //configurations
      xmatrix<double> hnf_mat;
      vector<vector<int> > v_types_config;
      //vector<int> v_config_iterators;
      uint config_iterator;
      vector<uint> v_config_order;
      double m_energy_uff_tolerance;

      void initializePOccStructure();
      void cleanPOccStructure();
      void preparePOccStructure();  //do not write out sym stuff by default

      const xmatrix<double>& getLattice() const;
      const vector<_sym_op>& getPGroup() const;
      //const StructureConfiguration& getXStrCountConfiguration(uint i) const;

      //void calculateHNF();                   //get n_hnf

      //useful internal methods
      //vector<_sym_op> getPGroups();           //fetch pgroups of xstr_nopocc
      //uint getHNFCount();                     //get count of hnf matrices, refers to v_hnf
      //xmatrix<double> getHNFMatrix(uint i);   //fetch specific hnf matrix, refers to v_hnf
      //_atom getAtom(uint i);                  //grab specific atom, refers to xstr_pocc

      //table stuff
      //set some nice printing precisions and paddings, mostly definitions
      uint getHNFTabelPOCCPrecision() const;
      uint getHNFTableGeneralPrecision() const;
      uint getHNFTableIterationPadding() const;
      uint getHNFTableErrorPadding() const;
      uint getHNFTableColumnPadding() const;
      string getHeaderMaxSiteError() const;
      string getHeaderMaxStoichError() const;
      string getHeaderStoichiometry() const;

      string hnfTableHeader();
      void writeHNFTableOutput(int i_hnf,double& stoich_error,double& site_error);
      string hnfTableLineOutput(int i_hnf,int str_config);
      //void setHNFTablePadding(int _AFLOW_POCC_PRECISION_);

      void partitionPOccSites();              //get pocc_sites
      xvector<double> calculateStoichEachType(vector<vector<int> >& v_types_config);
      void calculateSymNonPOccStructure(bool verbose=true);          //calculate symmetry of non-pocc structure
      void propagateEquivalentAtoms2POccStructure();  //propagates equivalent atoms info from xstr_sym to xstr_pocc for sorting
      void redecorateXStructures();            //redecorate xstr_nopocc for standardization of UFF energies

      //void getSiteCountConfigurations(int i_hnf,double& stoich_error);
      void getSiteCountConfigurations(int i_hnf);
      void getOptimizedSiteCountConfigurations(int site,int i_hnf,vector<POccSiteConfiguration>& v_site_configs);



      //determines site occupancy and vacancy count, given n_hnf
      bool iterateHNFMatrix();                //calculate all unique hnf's
      void setConfigOrder();
      bool getNextSiteConfiguration();
      bool getNextSiteConfiguration(vector<vector<int> >& v_site_config);

      //CT20200319 - POCC+AEL functions
      void calculateElasticProperties(const vector<double>& v_temperatures);
      void setAELOptions(bool& ael_run_postprocess, bool& ael_write_full_results);
      void generateElasticProperties(vector<double>& Bvoigt, vector<double>& Breuss, vector<double>& Bvrh, vector<double>& Gvoigt, vector<double>& Greuss, vector<double>& Gvrh,vector<double>& Poisson_ratio, vector<vector<vector<double> > >& elastic_tensor_list,  vector<vector<vector<double> > >& compliance_tensor_list);
      void getElasticProperties(vector<double>& Bvoigt, vector<double>& Breuss, vector<double>& Bvrh, vector<double>& Gvoigt, vector<double>& Greuss, vector<double>& Gvrh,vector<double>& Poisson_ratio, vector<vector<vector<double> > >& elastic_tensor_list,  vector<vector<vector<double> > >& compliance_tensor_list);
      void getAverageElasticProperties(const vector<double>& v_temperatures, bool ael_write_full_results, vector<double>& Bvoigt, vector<double>& Breuss, vector<double>& Bvrh, vector<double>& Gvoigt, vector<double>& Greuss, vector<double>& Gvrh,vector<double>& Poisson_ratio, vector<vector<vector<double> > >& elastic_tensor_list,  vector<vector<vector<double> > >& compliance_tensor_list);

      //CT20200323 - POCC+AGL functions
      void calculateDebyeThermalProperties(const vector<double>& v_temperatures);
      // [OBSOLETE] void setAGLOptions(bool& agl_run_postprocess, bool& agl_write_full_results, uint& ntemperature, double& stemperature, uint& npressure, double& spressure);
      void setAGLOptions(bool& agl_run_postprocess, bool& agl_write_full_results); //CT20200722
      // [OBSOLETE] void generateDebyeThermalProperties(uint ntemperature, double stemperature, uint npressure, double spressure, vector<double>& Debye_temperature, vector<double>& Debye_acoustic, vector<double>& Gruneisen, vector<double>& Cv300K, vector<double>& Cp300K, vector<double>& Fvib300K_atom, vector<double>& Fvib300K_cell, vector<double>& Svib300K_atom, vector<double>& Svib300K_cell, vector<double>& kappa300K, vector<vector<double> >& agl_temperatures, vector<vector<double> >& agl_gibbs_energies_atom, vector<vector<double> >& agl_vibrational_energies_atom);
      void generateDebyeThermalProperties(vector<double>& Debye_temperature, vector<double>& Debye_acoustic, vector<double>& Gruneisen, vector<double>& Cv300K, vector<double>& Cp300K, vector<double>& Fvib300K_atom, vector<double>& Fvib300K_cell, vector<double>& Svib300K_atom, vector<double>& Svib300K_cell, vector<double>& kappa300K, vector<vector<double> >& agl_temperatures, vector<vector<double> >& agl_gibbs_energies_atom, vector<vector<double> >& agl_vibrational_energies_atom); //CT20200722
      void getDebyeThermalProperties(vector<double>& Debye_temperature, vector<double>& Debye_acoustic, vector<double>& Gruneisen, vector<double>& Cv300K, vector<double>& Cp300K, vector<double>& Fvib300K_atom, vector<double>& Fvib300K_cell, vector<double>& Svib300K_atom, vector<double>& Svib300K_cell, vector<double>& kappa300K, vector<vector<double> >& agl_temperatures, vector<vector<double> >& agl_gibbs_energies_atom, vector<vector<double> >& agl_vibrational_energies_atom);
      void getAverageDebyeThermalProperties(const vector<double>& v_temperatures, bool agl_write_full_results, vector<double>& Debye_temperature, vector<double>& Debye_acoustic, vector<double>& Gruneisen, vector<double>& Cv300K, vector<double>& Cp300K, vector<double>& Fvib300K_atom, vector<double>& Fvib300K_cell, vector<double>& Svib300K_atom, vector<double>& Svib300K_cell, vector<double>& kappa300K, vector<vector<double> >& agl_temperatures, vector<vector<double> >& agl_gibbs_energies_atom, vector<vector<double> >& agl_vibrational_energies_atom);

      // ME20210927 - APL functions
      void calculatePhononPropertiesAPL(const vector<double>& v_temperatures);
      vector<apl::PhononCalculator> initializePhononCalculators();
      vector<xDOSCAR> getPhononDoscars(vector<apl::PhononCalculator>& vphcalc, xoption& dosopts, vector<int>& vexclude);
#ifdef AFLOW_MULTITHREADS_ENABLE
      void calculatePhononDOSThread(uint i, const vector<uint>& vcalc, const aurostd::xoption& aplopts, vector<apl::DOSCalculator>& vphdos, vector<xDOSCAR>& vxdos, std::mutex& m);
#else
      void calculatePhononDOSThread(uint i, const vector<uint>& vcalc, const aurostd::xoption& aplopts, vector<apl::DOSCalculator>& vphdos, vector<xDOSCAR>& vxdos);
#endif
      xDOSCAR getAveragePhononDos(double T, const vector<xDOSCAR>& vxdos);

      //AS20210204 QHA
      void calculateQHAProperties();
      void calculateQHAPropertiesAVG(const vector<double>& v_temperatures);
  };
} // namespace pocc

namespace pocc {
  class POccStructuresFile : public xStream {
    public:
      //NECESSARY PUBLIC CLASS METHODS - START
      //constructors - START
      POccStructuresFile(ostream& oss=cout);
      POccStructuresFile(ofstream& FileMESSAGE,ostream& oss=cout);
      POccStructuresFile(const string& fileIN,ostream& oss=cout);
      POccStructuresFile(const string& fileIN,ofstream& FileMESSAGE,ostream& oss=cout);
      POccStructuresFile(const _aflags& aflags,ostream& oss=cout);
      POccStructuresFile(const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);
      POccStructuresFile(const string& fileIN,const _aflags& aflags,ostream& oss=cout);
      POccStructuresFile(const string& fileIN,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss=cout);

      POccStructuresFile(const POccStructuresFile& b);
      //constructors - STOP
      ~POccStructuresFile();
      const POccStructuresFile& operator=(const POccStructuresFile& b);
      void clear();

      bool m_initialized;
      string m_content;vector<string> m_vcontent;string m_filename;
      _aflags m_aflags;
      //vector<string> m_ARUN_directories;
      std::list<POccSuperCellSet> l_supercell_sets;
      aurostd::xoption m_fileoptions;
      vector<vector<vector<uint> > > m_vPOSCAR_lines;  //contains start/stop indices for POSCARs in m_vcontent

      //initializers
      bool initialize(ostream& oss);
      bool initialize(ofstream& FileMESSAGE,ostream& oss);
      bool initialize();
      bool initialize(const string& fileIN,ostream& oss);
      bool initialize(const string& fileIN,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const string& fileIN);
      bool initialize(const _aflags& aflags,ostream& oss);
      bool initialize(const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const _aflags& aflags);
      bool initialize(const string& fileIN,const _aflags& aflags,ostream& oss);
      bool initialize(const string& fileIN,const _aflags& aflags,ofstream& FileMESSAGE,ostream& oss);
      bool initialize(const string& fileIN,const _aflags& aflags);

      void setAFlags(const _aflags& aflags);
      void readFile(const string& fileIN);
      void processFile();
      bool getARUNDirectories(vector<string>& ARUN_directories,bool tryDirectoryLS=true);
      bool loadDataIntoCalculator(POccCalculator& pcalc,bool tryDirectoryLS=true);

      friend ostream& operator<<(ostream&, const POccStructuresFile&);
    private:
      //NECESSARY PRIVATE CLASS METHODS - START
      void free();
      void copy(const POccStructuresFile& b);
      //NECESSARY END CLASS METHODS - END
  };
}

namespace pocc {
  /// This class is used to calculate  thermodynamic properties over the ensemble,
  /// defined by structures generated using POCC method.
  class EnsembleThermo : public xStream {
    public:
      EnsembleThermo(ostream &oss=std::cout);
      EnsembleThermo(const EnsembleThermo &ens);
      EnsembleThermo(const string & currentDir, vector<string> &directories,
          const string &filename, const string &calc_type,
          apl::EOSmethod eos_method, bool isFVTprovided,
          ofstream &FileMESSAGE, ostream &oss=std::cout);
      const EnsembleThermo& operator=(const EnsembleThermo &ens);
      ~EnsembleThermo();
      apl::QHA qha;
      apl::EOSmethod eos_method;
      uint Nstructures;
      int Nvolumes;
      int nrows;
      double Ensemble_Vmin, Ensemble_Vmax;
      string currentDirectory;
      xvector<double> T;
      xmatrix<double> FV;
      xvector<double> volumes;
      vector<int> degeneracies;
      vector<xmatrix<double> > coeffs_list;
      xvector<double> Veq, Feq, B, Bprime, Cv, Cp, gamma, beta;
      double logZ(const xvector<double> &E, const vector<int> &degeneracies, double T);
      xvector<double> calcThermalExpansionSG(const xvector<double> &volumes, double dT);
      xvector<double> calcIsobaricSpecificHeatSG(const xvector<double> &free_energies, double dT);
      void calculateThermodynamicProperties();
      void writeThermodynamicProperties();
      void clear();
    private:
      void readFVTParameters(const string &filename, const string &blockname,
          uint &Nvolumes, uint &Ntemperatures);
      void readFVTdata(const string & dirname, const string& filename,
          const string& blockname, uint n_volumes, uint n_temperatures, xvector<double> &t,
          xmatrix<double> &c, double &Vmin, double &Vmax);
      bool readCoeffData(const string& filename, const string& blockname,
          xvector<double> &T, xmatrix<double> &coeffs);
      void readCoeffParameters(const string& filename, double &Vmin, double &Vmax);
      // mandatory
      void free();
      void copy(const EnsembleThermo &ens);
  };
}

#endif  // _AFLOW_POCC_H_

// ***************************************************************************
// *                                                                         *
// *         aflow - Automatic FLOW for materials discovery project          *
// *             Stefano Curtarolo - Duke University - 2003-2021             *
// *                 Kesong Yang - Duke University 2010-2011                 *
// *                  Corey Oses - Duke University 2013-2021                 *
// *                                                                         *
// ***************************************************************************
