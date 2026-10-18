// ***************************************************************************
// *                                                                         *
// *           Aflow COREY OSES - Johns Hopkins University 2022-2026         *
// *                                                                         *
// ***************************************************************************
// aflow_pocc_enumeration.cpp
// ENUM: enumeration of the POCC supercell decorations by the orbits of the
// supercell permutation groups, at fixed concentration with site restrictions.
// References: Phys. Rev. B 77, 224115 (2008); Phys. Rev. B 80, 014120 (2009);
// Comput. Mater. Sci. 59, 101 (2012).  //CO20261005
//
// Selected with --pocc_enumerator=enum (command line), [AFLOW_POCC]ENUMERATOR=ENUM
// (aflow.in) or DEFAULT_POCC_ENUMERATOR="ENUM" (.aflow.rc, the default);
// --pocc_enumerator=compare runs the UFF enumerator and this one and reports
// where they differ.
//
// Each site of the parent is colored by the species counts it takes in the
// supercells (the count options over the structure configurations), and the
// symmetry of the colored parent is used throughout, restricted to the factor
// group operations that map the structure configurations onto each other:
//  1. superlattices (HNF matrices) are reduced by the point group of those
//     operations; the list starts with the one POCC reduces by the lattice
//     point group, so the H indices of the UFF enumerator are kept, and the
//     superlattices that only the smaller point group separates follow;
//  2. the Smith normal form S=U*H*V of each HNF maps the lattice vector x of a
//     supercell site onto the group G=Z_s1 x Z_s2 x Z_s3 by g=(x*V) mod S, so the
//     translations of the parent act on the sites by addition in G;
//  3. the operations that keep the superlattice, combined with the n
//     translations, permute the sites;
//  4. the decorations POCC lists for the HNF are ranked exactly as POCC orders
//     them (site_config_index), and each orbit of the permutation group is
//     marked once in a table of that size, so every decoration is visited once
//     and the degeneracy is the size of its orbit;
//  5. a superperiodic decoration (one with a smaller period lattice P than the
//     superlattice L) is also mapped by the operations R with R^-1 L' inside P,
//     onto its own superlattice (L'=L) and onto the other ones (L' of another
//     HNF matrix); these images are the same structure, so they join its set
//     (the published algorithm drops superperiodic decorations; POCC keeps them).
// With --pocc_sample_number/--pocc_sample_rate/--pocc_sample_hash, the
// decorations are the first round of UFF (FirstRandomSamplingWholeSpace(),
// FirstRandomSampling()), built by getSiteConfiguration(); the drawn
// decorations that are the same structure share a set, and each counts once
// in its degeneracy, as UFF counts its sampled bins.

#ifndef _AFLOW_POCC_ENUMERATION_CPP_
#define _AFLOW_POCC_ENUMERATION_CPP_

#include "aflow.h"
#include "aflow_pocc.h"
#include <set>
#include <unordered_set>
#include <algorithm>
#include <chrono>

#define _DEBUG_POCC_ENUMERATION_ false

namespace pocc {
  //--------------------------------------------------------------------------------
  // enumerator selection
  //--------------------------------------------------------------------------------
  /// @brief the POCC enumerator named by input: UFF, ENUM or COMPARE in any case and spacing; empty gives UFF
  /// @authors
  /// @mod{CO,20261009,created}
  string getPOccEnumerator(const string& input) {
    string algo=aurostd::toupper(aurostd::RemoveWhiteSpaces(input));
    if(algo.empty()){algo="UFF";}
    if(!(algo=="UFF" || algo=="ENUM" || algo=="COMPARE")){
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"Unknown POCC enumerator [input="+input+"], expected UFF, ENUM or COMPARE",_INPUT_ILLEGAL_);
    }
    return algo;
  }

  /// @brief the enumerator a new POCC aflow.in records as [AFLOW_POCC]ENUMERATOR: --pocc_enumerator= if given, else DEFAULT_POCC_ENUMERATOR
  /// @note every new aflow.in carries the key, so a parent keeps the enumerator of its supercell list when the default changes; an aflow.in without the key is read as UFF
  /// @authors
  /// @mod{CO,20261009,created}
  string getPOccEnumeratorForAflowIn() {
    if(XHOST.vflag_pflow.flag("POCC_ENUMERATOR")){return getPOccEnumerator(XHOST.vflag_pflow.getattachedscheme("POCC_ENUMERATOR"));}
    return getPOccEnumerator(DEFAULT_POCC_ENUMERATOR);
  }

  /// @brief structure generation algorithm: ENUM (default), UFF or COMPARE
  /// @note precedence: --pocc_enumerator= (command line), [AFLOW_POCC]ENUMERATOR= (aflow.in; UFF when the aflow.in has no key), DEFAULT_POCC_ENUMERATOR (.aflow.rc)
  /// @authors
  /// @mod{CO,20261005,created}
  /// @mod{CO,20261009,ENUM default; validation in getPOccEnumerator()}
  string POccCalculator::getStructureGenerationAlgo() const {
    if(enumerator_mode.flag("UFF")){return "UFF";}      //set by compareEnumerators()
    if(enumerator_mode.flag("ENUM")){return "ENUM";}  //set by compareEnumerators()
    string algo="";
    if(m_p_flags.flag("POCC_ENUMERATOR")){algo=m_p_flags.getattachedscheme("POCC_ENUMERATOR");}
    else if(XHOST.vflag_pflow.flag("POCC_ENUMERATOR")){algo=XHOST.vflag_pflow.getattachedscheme("POCC_ENUMERATOR");}
    else if(!m_kflags.KBIN_POCC_ENUMERATOR_STRING.empty()){algo=m_kflags.KBIN_POCC_ENUMERATOR_STRING;}  //from aflow.in (KBIN::VASP_Get_Kflags_from_AflowIN)
    else {algo=DEFAULT_POCC_ENUMERATOR;}
    return getPOccEnumerator(algo);
  }

  //--------------------------------------------------------------------------------
  // colored parent
  //--------------------------------------------------------------------------------
  /// @brief color each site of xstr_nopocc by the species counts it takes in the supercells, and keep the symmetry of the colored parent that maps the structure configurations onto each other
  /// @note coloring by the PARTCAR occupations would separate sites whose occupations differ but round to the same counts at this n (Mg0.5Zn0.5 and Mg0.4Zn0.6 at n=4 both give 2 Mg and 2 Zn)
  /// @note sites with the same count options can still be paired differently by the structure configurations, so each operation is checked against the list
  /// @authors
  /// @mod{CO,20261005,created}
  void POccCalculator::setColoredParent() {
    bool LDEBUG=(FALSE || _DEBUG_POCC_ENUMERATION_ || XHOST.DEBUG);
    stringstream message;

    const uint nsites=xstr_nopocc.atoms.size();
    if(m_pocc_sites.size()!=nsites){
      message << "m_pocc_sites.size()=" << m_pocc_sites.size() << " != " << nsites << "=xstr_nopocc.atoms.size()";
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INDEX_MISMATCH_);
    }
    if(m_rank_counts.empty()){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"run setDecorationRanking() first",_RUNTIME_ERROR_);}
    for(uint c=0;c<m_rank_counts.size();c++){
      if(m_rank_counts[c].size()!=nsites){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"structure configuration does not cover the sites of xstr_nopocc",_INDEX_MISMATCH_);}
    }

    //signature of each site: its species counts over the structure configurations
    vector<string> vsignatures;
    m_site_colors.clear();
    stringstream ss;
    for(uint site=0;site<nsites;site++){
      std::set<vector<uint> > options;
      for(uint c=0;c<m_rank_counts.size();c++){options.insert(m_rank_counts[c][site]);}
      ss.str("");
      for(std::set<vector<uint> >::const_iterator it=options.begin();it!=options.end();++it){
        ss << (it==options.begin()?"":";") << aurostd::joinWDelimiter(*it,",");
      }
      const string signature=ss.str();
      uint color=vsignatures.size();
      for(uint i=0;i<vsignatures.size();i++){if(vsignatures[i]==signature){color=i;break;}}
      if(color==vsignatures.size()){vsignatures.push_back(signature);}
      m_site_colors.push_back(color);
      if(LDEBUG){cerr << __AFLOW_FUNC__ << " site=" << site << " counts (vacancy first)=" << signature << " color=" << color << endl;}
    }

    //colored parent: same lattice and positions as xstr_nopocc, one fake species per color
    vector<string> vnames=pflow::getFakeElements(vsignatures.size());
    deque<_atom> atoms_colored;
    for(uint site=0;site<nsites;site++){
      atoms_colored.push_back(xstr_nopocc.atoms[site]);
      atoms_colored.back().name=vnames[m_site_colors[site]];
      atoms_colored.back().cleanname=vnames[m_site_colors[site]];
      atoms_colored.back().type=m_site_colors[site];
      atoms_colored.back().partial_occupation_flag=false;
      atoms_colored.back().partial_occupation_value=1.0;
    }
    m_xstr_color=xstr_nopocc;
    m_xstr_color.ClearSymmetry();
    m_xstr_color.ReplaceAtoms(atoms_colored,false);
    m_xstr_color.sym_eps=xstr_sym.sym_eps;
    if(!pflow::PerformFullSymmetry(m_xstr_color)){
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"AFLOW-SYM failed to calculate the symmetry of the colored parent",_RUNTIME_ERROR_);
    }
    if(!m_xstr_color.fgroup_calculated || m_xstr_color.fgroup.size()==0){
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"symmetry of the colored parent is incomplete",_RUNTIME_ERROR_);
    }

    //each factor group operation, applied to the sites of xstr_nopocc: site d goes to site m_vfgroup_site_map[op][d], shifted by the lattice vector m_vfgroup_site_shift[op][d]
    m_vfgroup_site_map.clear();
    m_vfgroup_site_shift.clear();
    m_vfgroup_site_inverse.clear();
    m_vfgroup_Uf.clear();
    m_vfgroup_Uf_inv.clear();
    m_vpgroup_enum.clear();
    m_enum_fgroup_dropped=0;
    const double tol=m_xstr_color.sym_eps;
    xvector<double> fpos(3),diff(3);
    xvector<long long int> shift(3);
    vector<xmatrix<long long int> > vUf_pgroup;
    vector<vector<uint> > permuted(nsites);
    for(uint iop=0;iop<m_xstr_color.fgroup.size();iop++){
      const _sym_op& op=m_xstr_color.fgroup[iop];
      const xmatrix<long long int> Uf=aurostd::xmatrixdouble2utype<long long int>(op.Uf);  //throws if not integer
      const xmatrix<long long int> Uf_inv=aurostd::xmatrixdouble2utype<long long int>(aurostd::inverse(op.Uf));  //unimodular
      if(!aurostd::isidentity(Uf*Uf_inv)){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"factor group operation is not unimodular in the parent basis",_RUNTIME_ERROR_);}
      vector<uint> site_map(nsites,AUROSTD_MAX_UINT),site_inverse(nsites,AUROSTD_MAX_UINT);
      vector<xvector<long long int> > site_shift(nsites);
      for(uint d=0;d<nsites;d++){
        fpos=op.Uf*xstr_nopocc.atoms[d].fpos+op.ftau;
        for(uint d2=0;d2<nsites;d2++){
          if(m_site_colors[d2]!=m_site_colors[d]){continue;}
          diff=fpos-xstr_nopocc.atoms[d2].fpos;
          for(int i=diff.lrows;i<=diff.urows;i++){shift[i-diff.lrows+shift.lrows]=(long long int)aurostd::nint(diff[i]);diff[i]-=(double)shift[i-diff.lrows+shift.lrows];}
          if(aurostd::modulus(xstr_nopocc.f2c*diff)<tol){site_map[d]=d2;site_shift[d]=shift;break;}
        }
        if(site_map[d]==AUROSTD_MAX_UINT){
          message << "factor group operation " << iop << " does not map site " << d << " onto a site of the same color";
          throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_RUNTIME_ERROR_);
        }
        if(site_inverse[site_map[d]]!=AUROSTD_MAX_UINT){
          message << "factor group operation " << iop << " maps two sites onto site " << site_map[d];
          throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_RUNTIME_ERROR_);
        }
        site_inverse[site_map[d]]=d;
      }
      //keep the operation only if it maps every structure configuration onto a listed one
      bool keep=true;
      for(uint c=0;c<m_rank_counts.size()&&keep;c++){
        for(uint d=0;d<nsites;d++){permuted[site_map[d]]=m_rank_counts[c][d];}
        bool found=false;
        for(uint c2=0;c2<m_rank_counts.size()&&!found;c2++){found=(m_rank_counts[c2]==permuted);}
        keep=found;
      }
      if(!keep){
        m_enum_fgroup_dropped++;
        if(LDEBUG){cerr << __AFLOW_FUNC__ << " dropping factor group operation " << iop << " (changes the structure configurations)" << endl;}
        continue;
      }
      m_vfgroup_site_map.push_back(site_map);
      m_vfgroup_site_shift.push_back(site_shift);
      m_vfgroup_site_inverse.push_back(site_inverse);
      m_vfgroup_Uf.push_back(Uf);
      m_vfgroup_Uf_inv.push_back(Uf_inv);
      bool new_rotation=true;
      for(uint i=0;i<vUf_pgroup.size()&&new_rotation;i++){if(aurostd::isequal(vUf_pgroup[i],Uf)){new_rotation=false;}}
      if(new_rotation){vUf_pgroup.push_back(Uf);m_vpgroup_enum.push_back(op);}
    }
    if(m_vfgroup_Uf.size()==0){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"no factor group operation kept (identity missing)",_RUNTIME_ERROR_);}

    message << "Colored parent: " << vsignatures.size() << " site color(s) from the species counts, factor group " << m_xstr_color.fgroup.size();
    message << " (parent " << xstr_sym.fgroup.size() << "), " << m_enum_fgroup_dropped << " operation(s) dropped as they change the structure configurations";
    message << ", point group " << m_vpgroup_enum.size() << ", lattice point group " << xstr_sym.pgroup.size();
    pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_MESSAGE_);
  }

  //--------------------------------------------------------------------------------
  // superlattices
  //--------------------------------------------------------------------------------
  /// @brief true if superlattices Bi and Bj (rows) are related by a rotation of pgroup: Bi=H*Bj*R^T with H unimodular
  /// @note Phys. Rev. B 77, 224115 (2008), in row space as in POccCalculator::iterateHNFMatrix()
  /// @authors
  /// @mod{CO,20261005,created}
  bool areEquivalentSuperlattices(const xmatrix<double>& Bi,const xmatrix<double>& Bj,const vector<_sym_op>& pgroup) {
    const xmatrix<double> Bj_inv=aurostd::inverse(Bj);
    for(uint pg=0;pg<pgroup.size();pg++){
      if(aurostd::isinteger(Bi*aurostd::inverse(pgroup[pg].Uc)*Bj_inv)){return true;}
    }
    return false;
  }

  /// @brief HNF list of ENUM, the number of HNF matrices each one stands for, and its class under the lattice point group
  /// @note the list starts with the HNF matrices POCC reduces by the lattice point group (same H indices), then the ones only the point group of the colored parent separates
  /// @authors
  /// @mod{CO,20261005,created}
  void POccCalculator::setEnumHNFMatrices() {
    stringstream message;
    m_hnf_from_list=false;
    //HNF matrices as the UFF enumerator lists them (lattice point group)
    vector<xmatrix<double> > vhnf_lattice;
    m_hnf_reduce=true;
    resetHNFMatrices();
    while(iterateHNFMatrix()){vhnf_lattice.push_back(hnf_mat);}
    m_hnf_count_lattice=vhnf_lattice.size();
    //every HNF matrix, in the same order
    vector<xmatrix<double> > vhnf_all;
    m_hnf_reduce=false;
    resetHNFMatrices();
    while(iterateHNFMatrix()){vhnf_all.push_back(hnf_mat);}
    m_hnf_reduce=true;
    resetHNFMatrices();

    const xmatrix<double>& lattice=getLattice();
    m_vhnf_enum=vhnf_lattice;
    vector<xmatrix<double> > vsuperlattices;
    for(uint i=0;i<m_vhnf_enum.size();i++){vsuperlattices.push_back(m_vhnf_enum[i]*lattice);}
    m_vhnf_multiplicity.assign(m_vhnf_enum.size(),0);
    xmatrix<double> superlattice;
    bool found=false;
    for(uint i=0;i<vhnf_all.size();i++){
      superlattice=vhnf_all[i]*lattice;
      found=false;
      for(uint j=0;j<vsuperlattices.size()&&!found;j++){
        if(areEquivalentSuperlattices(superlattice,vsuperlattices[j],m_vpgroup_enum)){m_vhnf_multiplicity[j]++;found=true;}
      }
      if(!found){
        m_vhnf_enum.push_back(vhnf_all[i]);
        vsuperlattices.push_back(superlattice);
        m_vhnf_multiplicity.push_back(1);
      }
    }
    //test of stupidity: every listed HNF matrix stands for itself at least, and the classes cover every HNF matrix
    unsigned long long int count=0;
    for(uint i=0;i<m_vhnf_multiplicity.size();i++){
      if(m_vhnf_multiplicity[i]==0){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"HNF matrix without members",_RUNTIME_ERROR_);}
      count+=m_vhnf_multiplicity[i];
    }
    if(count!=vhnf_all.size()){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"HNF classes do not cover all HNF matrices",_RUNTIME_ERROR_);}
    //class of each listed HNF matrix under the lattice point group (its H index in the UFF enumerator)
    m_vhnf_lattice_class.clear();
    for(uint i=0;i<m_vhnf_enum.size();i++){
      if(i<m_hnf_count_lattice){m_vhnf_lattice_class.push_back(i);continue;}
      found=false;
      for(uint j=0;j<m_hnf_count_lattice&&!found;j++){
        if(areEquivalentSuperlattices(vsuperlattices[i],vsuperlattices[j],getPGroup())){m_vhnf_lattice_class.push_back(j);found=true;}
      }
      if(!found){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"HNF matrix outside the lattice point group classes",_RUNTIME_ERROR_);}
    }
    m_hnf_from_list=true;

    message << "HNF matrices of n=" << n_hnf << ": " << vhnf_all.size() << " in total, " << m_hnf_count_lattice << " inequivalent by the lattice point group, ";
    message << m_vhnf_enum.size() << " inequivalent by the point group of the colored parent";
    pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_MESSAGE_);
    for(uint j=0;j<m_hnf_count_lattice;j++){
      vector<string> vsplit;
      for(uint i=0;i<m_vhnf_enum.size();i++){
        if(m_vhnf_lattice_class[i]==j){vsplit.push_back("H"+aurostd::utype2string(i)+" ("+aurostd::utype2string(m_vhnf_multiplicity[i])+" HNF matrices)");}
      }
      if(vsplit.size()<2){continue;}
      message << "HNF " << j << " of the lattice point group splits into " << aurostd::joinWDelimiter(vsplit,", ");
      pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_MESSAGE_);
    }
  }

  /// @brief prepare ENUM (decoration ranking, colored parent, HNF list); idempotent
  /// @authors
  /// @mod{CO,20261005,created}
  void POccCalculator::prepareEnumeration() {
    if(m_enum_prepared){m_hnf_from_list=true;return;}
    if(n_hnf<1){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"run calculateHNF() first",_RUNTIME_ERROR_);}
    setDecorationRanking();  //the colors come from the species counts of the structure configurations
    setColoredParent();
    setEnumHNFMatrices();
    m_enum_prepared=true;
  }

  //--------------------------------------------------------------------------------
  // ranking of the decorations in POCC's order (getDistinctPermutationsCount() and getSiteConfiguration() count and unrank)
  //--------------------------------------------------------------------------------
  /// @brief lexicographic rank of seq among the permutations of its multiset (values -1 (vacancy) to nvalues-2)
  /// @note getNextSiteConfiguration(vector<int>&) steps through these permutations in lexicographic order (Shen, BIT 2, 228 (1962))
  unsigned long long int rankMultisetPermutation(const vector<int>& seq,uint nvalues,unsigned long long int permutations) {
    vector<uint> counts(nvalues,0);
    for(uint i=0;i<seq.size();i++){counts[seq[i]+1]++;}
    unsigned long long int rank=0;
    __uint128_t perms=permutations;  //permutations of the remaining multiset
    uint remaining=seq.size();
    for(uint pos=0;pos<seq.size();pos++){
      uint v=seq[pos]+1;
      for(uint u=0;u<v;u++){
        if(counts[u]==0){continue;}
        rank+=(unsigned long long int)((perms*counts[u])/remaining);  //permutations starting with u
      }
      perms=(perms*counts[v])/remaining;
      counts[v]--;
      remaining--;
    }
    return rank;
  }

  /// @brief tables that turn a decoration into its site_config_index, following getNextSiteConfiguration()
  /// @note within one structure configuration the site v_config_order[0] varies fastest; the configurations follow one another
  /// @authors
  /// @mod{CO,20261005,created}
  void POccCalculator::setDecorationRanking() {
    m_rank_values=1;  //vacancy
    m_rank_order.clear();m_rank_site_count.clear();m_rank_offset.clear();m_rank_key.clear();m_rank_counts.clear();
    unsigned long long int offset=0;
    vector<vector<int> > start;
    for(uint c=0;c<v_str_configs.size();c++){
      for(uint site=0;site<v_str_configs[c].site_configs.size();site++){
        start.push_back(v_str_configs[c].site_configs[site].getStartingTypesConfiguration());
        for(uint i=0;i<start.back().size();i++){m_rank_values=max(m_rank_values,(uint)(start.back()[i]+2));}
      }
    }
    for(uint c=0;c<v_str_configs.size();c++){
      start.clear();
      for(uint site=0;site<v_str_configs[c].site_configs.size();site++){
        start.push_back(v_str_configs[c].site_configs[site].getStartingTypesConfiguration());
      }
      m_rank_order.push_back(getConfigOrder(start));
      m_rank_site_count.push_back(vector<unsigned long long int>(0));
      m_rank_counts.push_back(vector<vector<uint> >(0));
      unsigned long long int total=1;
      for(uint site=0;site<start.size();site++){
        if((int)start[site].size()!=n_hnf){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"site configuration size differs from n_hnf",_INDEX_MISMATCH_);}
      }
      m_rank_key.push_back(getDecorationCountsKey(start));
      for(uint site=0;site<start.size();site++){
        vector<uint> counts(m_rank_values+2,0);
        for(uint i=0;i<start[site].size();i++){counts[start[site][i]+1]++;}
        m_rank_counts.back().push_back(counts);
        m_rank_site_count.back().push_back(getDistinctPermutationsCount(start[site]));  //as many as getNextSiteConfiguration() visits
        __uint128_t tmp=(__uint128_t)total*(__uint128_t)m_rank_site_count.back().back();
        if(tmp>(__uint128_t)AUROSTD_MAX_ULLINT){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"too many decorations",_VALUE_RANGE_);}
        total=(unsigned long long int)tmp;
      }
      m_rank_offset.push_back(offset);
      offset+=total;
    }
    for(uint c=0;c<m_rank_key.size();c++){
      for(uint c2=c+1;c2<m_rank_key.size();c2++){
        if(m_rank_key[c]==m_rank_key[c2]){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"two structure configurations share their site counts",_RUNTIME_ERROR_);}
      }
    }
    m_rank_total=offset;
  }

  /// @brief key of the per-site species counts of a decoration (one per structure configuration); two bytes per count, no stringstream, as it runs for every image
  string POccCalculator::getDecorationCountsKey(const vector<vector<int> >& v_types_config) const {
    const uint nvalues=m_rank_values+2;
    string key(2*nvalues*v_types_config.size(),'\0');
    uint value=0,pos=0;
    for(uint site=0;site<v_types_config.size();site++){
      for(uint i=0;i<v_types_config[site].size();i++){
        value=v_types_config[site][i]+1;
        if(value>=nvalues){return "";}  //species type outside the ranking tables
        pos=2*(site*nvalues+value);
        if((unsigned char)key[pos]==255){key[pos]=0;key[pos+1]=(char)((unsigned char)key[pos+1]+1);}
        else {key[pos]=(char)((unsigned char)key[pos]+1);}
      }
    }
    return key;
  }

  /// @brief site_config_index of a decoration given site by site; AUROSTD_MAX_ULLINT if POCC does not list it
  unsigned long long int POccCalculator::rankDecoration(const vector<vector<int> >& v_types_config) const {
    const string key=getDecorationCountsKey(v_types_config);
    if(key.empty()){return AUROSTD_MAX_ULLINT;}
    uint c=AUROSTD_MAX_UINT;
    for(uint i=0;i<m_rank_key.size();i++){if(m_rank_key[i]==key){c=i;break;}}
    if(c==AUROSTD_MAX_UINT){return AUROSTD_MAX_ULLINT;}
    unsigned long long int index=0,radix=1;
    uint site=0;
    for(uint k=0;k<m_rank_order[c].size();k++){
      site=m_rank_order[c][k];
      index+=radix*rankMultisetPermutation(v_types_config[site],m_rank_values+2,m_rank_site_count[c][site]);
      radix*=m_rank_site_count[c][site];
    }
    return m_rank_offset[c]+index;
  }

  //--------------------------------------------------------------------------------
  // symmetry acting on one supercell
  //--------------------------------------------------------------------------------
  /// @brief group element (index 0..n-1) of the lattice vector x (row, parent basis): (x*V) mod snf
  /// @authors
  /// @mod{CO,20261005,created}
  uint POccSupercellAction::gindex(const xvector<long long int>& x) const {
    long long int g[3];
    for(int j=0;j<3;j++){
      g[j]=0;
      for(int i=0;i<3;i++){g[j]+=x[x.lrows+i]*V[V.lrows+i][V.lcols+j];}
      g[j]%=snf[j];if(g[j]<0){g[j]+=snf[j];}
    }
    return (uint)(g[0]*snf[1]*snf[2]+g[1]*snf[2]+g[2]);
  }

  /// @brief translations and permutations of the supercell sites by the symmetry of the colored parent
  /// @param hnf HNF matrix (rows, parent basis)
  /// @param action translations, the permutation group of the operations that keep the superlattice (perms[p][s]: slot s goes to perms[p][s]), and what pullBackDecoration() needs
  /// @authors
  /// @mod{CO,20261005,created}
  void POccCalculator::getSupercellAction(const xmatrix<double>& hnf,POccSupercellAction& action) const {
    bool LDEBUG=(FALSE || _DEBUG_POCC_ENUMERATION_ || XHOST.DEBUG);
    stringstream message;
    action=POccSupercellAction();
    const uint nsites=xstr_nopocc.atoms.size();
    const uint n=(uint)n_hnf;
    const uint nslots=nsites*n;

    //Smith normal form S=U*H*V; lattice vector x (row) is in the superlattice iff (x*V) mod S == 0
    action.H=aurostd::xmatrixdouble2utype<long long int>(hnf);
    xmatrix<long long int> U,S;
    aurostd::getSmithNormalForm(action.H,U,action.V,S);
    for(int i=0;i<3;i++){action.snf[i]=S[S.lrows+i][S.lcols+i];}
    if(action.snf[0]<1 || action.snf[0]*action.snf[1]*action.snf[2]!=(long long int)n){
      message << "Smith normal form diagonal " << action.snf[0] << "," << action.snf[1] << "," << action.snf[2] << " does not multiply to n=" << n;
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_RUNTIME_ERROR_);
    }
    const long long int* s=action.snf;

    //lattice vector of every slot, from the supercell POCC builds
    vector<int> sc2pc_map,pc2sc_map;
    xstructure xstr_ss=GetSuperCell(xstr_nopocc,hnf,sc2pc_map,pc2sc_map,false,false,false,false);  //force_strict_pc2scMap==false, as in createXStructure()
    action.vx.assign(nslots,xvector<long long int>(3));
    action.slot_of.assign(nsites,vector<uint>(n,AUROSTD_MAX_UINT));
    vector<uint> vg(nslots,0);
    xvector<double> fpar(3);
    for(uint site=0;site<nsites;site++){
      for(uint i=0;i<n;i++){
        const uint j=pc2sc_map[site]+i;
        if(j>=xstr_ss.atoms.size() || sc2pc_map[j]!=(int)site){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"supercell atoms of a site are not contiguous",_INDEX_MISMATCH_);}
        fpar=xstr_nopocc.c2f*xstr_ss.atoms[j].cpos-xstr_nopocc.atoms[site].fpos;
        const uint slot=site*n+i;
        xvector<long long int>& x=action.vx[slot];
        for(int k=0;k<3;k++){
          x[x.lrows+k]=(long long int)aurostd::nint(fpar[fpar.lrows+k]);
          if(std::abs(fpar[fpar.lrows+k]-(double)x[x.lrows+k])>_AFLOW_POCC_ZERO_TOL_*1e3){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"supercell atom is not on a lattice vector of its parent site",_RUNTIME_ERROR_);}
        }
        vg[slot]=action.gindex(x);
        if(action.slot_of[site][vg[slot]]!=AUROSTD_MAX_UINT){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"two supercell atoms map onto the same group element",_RUNTIME_ERROR_);}
        action.slot_of[site][vg[slot]]=slot;
      }
    }

    //translations: g -> g+t
    action.translations.assign(n,vector<uint>(nslots));
    for(uint t=0;t<n;t++){
      const long long int tg[3]={(long long int)(t/(s[1]*s[2])),(long long int)((t/s[2])%s[1]),(long long int)(t%s[2])};
      for(uint slot=0;slot<nslots;slot++){
        const uint g=vg[slot];
        long long int gg[3]={(long long int)(g/(s[1]*s[2])),(long long int)((g/s[2])%s[1]),(long long int)(g%s[2])};
        for(int k=0;k<3;k++){gg[k]=(gg[k]+tg[k])%s[k];}
        action.translations[t][slot]=action.slot_of[slot/n][(uint)(gg[0]*s[1]*s[2]+gg[1]*s[2]+gg[2])];
      }
    }

    //factor group operations that keep the superlattice: x -> Uf*x+t_d, site d -> d'
    const xmatrix<double> hnf_inv=aurostd::inverse(hnf);
    vector<vector<uint> > vrot;
    vector<bool> vrot_identity;
    xvector<long long int> x2(3);
    action.keeps_superlattice.assign(m_vfgroup_Uf.size(),false);
    for(uint iop=0;iop<m_vfgroup_Uf.size();iop++){
      const xmatrix<long long int>& Uf=m_vfgroup_Uf[iop];
      if(!aurostd::isinteger(hnf*aurostd::trasp(aurostd::xmatrixutype2double(Uf))*hnf_inv)){continue;}
      action.keeps_superlattice[iop]=true;
      vector<uint> perm(nslots);
      for(uint slot=0;slot<nslots;slot++){
        const uint site=slot/n;
        x2=Uf*action.vx[slot]+m_vfgroup_site_shift[iop][site];
        perm[slot]=action.slot_of[m_vfgroup_site_map[iop][site]][action.gindex(x2)];
      }
      vrot.push_back(perm);
      vrot_identity.push_back(aurostd::isidentity(Uf));
    }
    action.nrotations=vrot.size();
    if(action.nrotations==0){
      message << "no factor group operation keeps the superlattice (identity missing) [factor group operations=" << m_vfgroup_Uf.size() << ", hnf=" << hnf << "]";
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_RUNTIME_ERROR_);
    }

    //group: translation after rotation, duplicates removed
    std::set<vector<uint> > sperms,strans;
    vector<uint> identity(nslots);
    for(uint slot=0;slot<nslots;slot++){identity[slot]=slot;}
    vector<uint> perm(nslots);
    for(uint r=0;r<vrot.size();r++){
      for(uint t=0;t<n;t++){
        for(uint slot=0;slot<nslots;slot++){perm[slot]=action.translations[t][vrot[r][slot]];}
        sperms.insert(perm);
        if(vrot_identity[r] && perm!=identity){strans.insert(perm);}
      }
    }
    for(std::set<vector<uint> >::const_iterator it=sperms.begin();it!=sperms.end();++it){
      action.perms.push_back(*it);
      action.perm_translation.push_back(strans.count(*it)>0);
    }
    if(LDEBUG){
      cerr << __AFLOW_FUNC__ << " hnf=" << endl << hnf << endl;
      cerr << __AFLOW_FUNC__ << " snf=" << s[0] << "," << s[1] << "," << s[2] << " rotations=" << action.nrotations << " group order=" << action.perms.size() << endl;
    }
  }

  /// @brief periods[t]: the translation by group element t leaves the decoration unchanged (periods[0] is the identity); any other true entry makes it superperiodic
  /// @authors
  /// @mod{CO,20261005,created}
  vector<bool> POccCalculator::getDecorationPeriods(const POccSupercellAction& action,const vector<vector<int> >& v_types_config) const {
    const uint n=(uint)n_hnf;
    vector<bool> periods(action.translations.size(),true);
    for(uint t=0;t<action.translations.size();t++){
      const vector<uint>& tr=action.translations[t];
      for(uint slot=0;slot<tr.size();slot++){
        if(v_types_config[slot/n][slot%n]!=v_types_config[tr[slot]/n][tr[slot]%n]){periods[t]=false;break;}
      }
    }
    return periods;
  }

  /// @brief image of a decoration of src under factor group operation iop, written on the superlattice of dst; false if the image does not repeat with dst
  /// @note the decoration repeats with its period lattice P (periods); the image repeats with Uf*P, which holds the superlattice L' of dst iff Uf^-1 L' is in P
  /// @note image(d',y)=decoration(d,Uf^-1 (y-t_d)) with d'=site_map[d]: every source the slots of dst pull from is fixed modulo P, so the image is well defined
  /// @authors
  /// @mod{CO,20261005,created}
  bool POccCalculator::pullBackDecoration(const POccSupercellAction& src,const vector<bool>& periods,const POccSupercellAction& dst,uint iop,const vector<vector<int> >& v_types_config,vector<vector<int> >& image) const {
    const uint n=(uint)n_hnf;
    const uint nsites=v_types_config.size();
    const xmatrix<long long int>& Uf_inv=m_vfgroup_Uf_inv[iop];
    xvector<long long int> h(3),x(3);
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){h[h.lrows+j]=dst.H[dst.H.lrows+i][dst.H.lcols+j];}
      if(!periods[src.gindex(Uf_inv*h)]){return false;}
    }
    image.assign(nsites,vector<int>(n,0));
    for(uint slot=0;slot<nsites*n;slot++){
      const uint site2=slot/n;
      const uint site=m_vfgroup_site_inverse[iop][site2];
      x=Uf_inv*(dst.vx[slot]-m_vfgroup_site_shift[iop][site]);
      const uint slot_src=src.slot_of[site][src.gindex(x)];
      image[site2][slot%n]=v_types_config[slot_src/n][slot_src%n];
    }
    return true;
  }

  /// @brief ranks of every decoration on the superlattice of dst that is the same structure as the decoration of src, through the operations that change the superlattice
  /// @param same_superlattice dst is src: the operations that keep the superlattice are skipped (src.perms has them)
  /// @authors
  /// @mod{CO,20261005,created}
  void POccCalculator::getEquivalentDecorations(const POccSupercellAction& src,const vector<bool>& periods,const POccSupercellAction& dst,bool same_superlattice,const vector<vector<int> >& v_types_config,std::set<unsigned long long int>& ranks) const {
    const uint n=(uint)n_hnf;
    vector<vector<int> > image,timage;
    unsigned long long int rank=0;
    for(uint iop=0;iop<m_vfgroup_Uf.size();iop++){
      if(same_superlattice && src.keeps_superlattice[iop]){continue;}
      if(!pullBackDecoration(src,periods,dst,iop,v_types_config,image)){continue;}
      timage=image;
      for(uint t=0;t<dst.translations.size();t++){
        const vector<uint>& tr=dst.translations[t];
        for(uint slot=0;slot<tr.size();slot++){timage[tr[slot]/n][tr[slot]%n]=image[slot/n][slot%n];}
        rank=rankDecoration(timage);
        if(rank>=types_config_permutations_count){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"symmetry operation takes a decoration outside the decorations POCC lists",_RUNTIME_ERROR_);}
        ranks.insert(rank);
      }
    }
  }

  //--------------------------------------------------------------------------------
  // ENUM
  //--------------------------------------------------------------------------------
  /// @brief unique supercells and their degeneracies from the orbits of the supercell permutation groups, superperiodic duplicates joined
  /// @note fills l_supercell_sets as the UFF enumerator does (every member of a set has degeneracy 1), with the UFF energy of the representative for reference
  /// @note a set can hold decorations of several HNF matrices (a superperiodic structure fits several superlattices); its representative is the smallest (H,C)
  /// @authors
  /// @mod{CO,20261005,created}
  void POccCalculator::calculateEnum() {
    bool LDEBUG=(FALSE || _DEBUG_POCC_ENUMERATION_ || XHOST.DEBUG);
    stringstream message;
    std::chrono::steady_clock::time_point t0=std::chrono::steady_clock::now();
    prepareEnumeration();
    if(m_rank_total!=types_config_permutations_count){
      message << "decoration count " << m_rank_total << " != " << types_config_permutations_count << " (types_config_permutations_count)";
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_RUNTIME_ERROR_);
    }
    l_supercell_sets.clear();
    m_enum_group_order.clear();m_enum_rotations.clear();m_enum_orbits.clear();m_enum_superperiodic.clear();
    m_enum_merged_hnf=0;m_enum_merged_across=0;m_enum_skipped=0;

    const uint nsites=xstr_nopocc.atoms.size();
    const uint n=(uint)n_hnf;
    const uint nslots=nsites*n;
    const bool uff_energies=!m_p_flags.flag("POCC_COUNT_UNIQUE");  //UFF energies of the representatives are not needed to count
    if(uff_energies){energy_analyzer.initialize(xstr_pocc,xstr_nopocc,m_species_redecoration,m_p_flags,m_aflags,*p_FileMESSAGE,*p_oss);}

    //symmetry acting on every supercell
    vector<POccSupercellAction> vaction;
    resetHNFMatrices();
    while(iterateHNFMatrix()){
      vaction.push_back(POccSupercellAction());
      getSupercellAction(hnf_mat,vaction.back());
      m_enum_group_order.push_back(vaction.back().perms.size());
      m_enum_rotations.push_back(vaction.back().nrotations);
    }
    if(vaction.size()!=hnf_count){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"HNF count mismatch",_RUNTIME_ERROR_);}
    m_enum_orbits.assign(hnf_count,0);
    m_enum_superperiodic.assign(hnf_count,0);

    //every decoration that is the same structure as config (on HNF matrix h), per HNF matrix; later_only: other HNF matrices after h only
    vector<std::set<unsigned long long int> > members(hnf_count);
    vector<vector<int> > image(nsites,vector<int>(n));
    auto getMembers=[&](uint h,const vector<vector<int> >& config,bool later_only,unsigned long long int& stabilizer_orbit) -> bool {
      for(uint i=0;i<members.size();i++){members[i].clear();}
      const POccSupercellAction& action=vaction[h];
      unsigned long long int rank=0;
      for(uint p=0;p<action.perms.size();p++){
        for(uint slot=0;slot<nslots;slot++){image[action.perms[p][slot]/n][action.perms[p][slot]%n]=config[slot/n][slot%n];}
        rank=rankDecoration(image);
        if(rank>=types_config_permutations_count){
          message << "symmetry operation takes a decoration of HNF " << h << " outside the decorations POCC lists";
          throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_RUNTIME_ERROR_);
        }
        members[h].insert(rank);
      }
      stabilizer_orbit=members[h].size();
      if(action.perms.size()%stabilizer_orbit!=0){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"orbit size does not divide the group order",_RUNTIME_ERROR_);}
      const vector<bool> periods=getDecorationPeriods(action,config);
      bool superperiodic=false;
      for(uint t=1;t<periods.size()&&!superperiodic;t++){superperiodic=periods[t];}
      if(!superperiodic){return false;}
      getEquivalentDecorations(action,periods,action,true,config,members[h]);
      for(uint h2=(later_only?h+1:0);h2<vaction.size();h2++){
        if(h2==h){continue;}
        getEquivalentDecorations(action,periods,vaction[h2],false,config,members[h2]);
      }
      return true;
    };

    string sample_string="";
    unsigned long long int sample_number=0,sample_hash_hnf=AUROSTD_MAX_ULLINT,sample_hash_site_config=AUROSTD_MAX_ULLINT;
    bool sampling=usePOccSampleNumber(sample_string,sample_number);
    sampling=(usePOccSampleRate(sample_string) || sampling);
    sampling=(usePOccSampleHash(sample_string,sample_hash_hnf,sample_hash_site_config) || sampling);

    vector<POccSuperCell> vpsc_rep;
    vector<vector<vector<int> > > vv_types_config_rep;
    POccSuperCell psc;
    psc.m_degeneracy=1;
    unsigned long long int stabilizer_orbit=0,nsampled=0;
    if(!sampling){
      //every decoration, HNF matrix by HNF matrix; decorations already in a set of an earlier HNF matrix are claimed
      vector<std::unordered_set<unsigned long long int> > claimed(hnf_count);
      vector<bool> visited;
      unsigned long long int site_config_index=0,rank=0;
      for(uint h=0;h<hnf_count;h++){
        visited.assign(types_config_permutations_count,false);
        for(std::unordered_set<unsigned long long int>::const_iterator it=claimed[h].begin();it!=claimed[h].end();++it){visited[*it]=true;}
        std::unordered_set<unsigned long long int>().swap(claimed[h]);
        resetSiteConfigurations();
        site_config_index=0;
        while(getNextSiteConfiguration()){
          rank=rankDecoration(v_types_config);
          if(rank!=site_config_index){
            message << "rank " << rank << " != " << site_config_index << " (site_config_index) for HNF " << h;
            throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_RUNTIME_ERROR_);
          }
          if(!visited[site_config_index]){
            //new set: site_config_index is its smallest member on the first HNF matrix it touches
            const bool superperiodic=getMembers(h,v_types_config,true,stabilizer_orbit);
            POccSuperCellSet pscs;
            for(uint h2=h;h2<hnf_count;h2++){
              for(std::set<unsigned long long int>::const_iterator it=members[h2].begin();it!=members[h2].end();++it){
                if(h2==h){
                  if(visited[*it]){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"decoration in two sets",_RUNTIME_ERROR_);}
                  visited[*it]=true;
                } else {
                  if(!claimed[h2].insert(*it).second){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"decoration of a later HNF matrix in two sets",_RUNTIME_ERROR_);}
                  m_enum_merged_across++;
                }
                psc.m_hnf_index=h2;
                psc.m_site_config_index=*it;
                pscs.m_psc_set.push_back(psc);
              }
            }
            m_enum_merged_hnf+=members[h].size()-stabilizer_orbit;
            std::sort(pscs.m_psc_set.begin(),pscs.m_psc_set.end());
            if(pscs.m_psc_set[0].m_hnf_index!=h || pscs.m_psc_set[0].m_site_config_index!=site_config_index){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"set does not start with its representative",_RUNTIME_ERROR_);}
            l_supercell_sets.push_back(pscs);
            if(uff_energies){
              vpsc_rep.push_back(pscs.m_psc_set[0]);
              vv_types_config_rep.push_back(v_types_config);
            }
            m_enum_orbits[h]++;
            if(superperiodic){m_enum_superperiodic[h]++;}
          }
          site_config_index++;
        }
        if(site_config_index!=types_config_permutations_count){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"decoration count mismatch",_RUNTIME_ERROR_);}
        if(LDEBUG){cerr << __AFLOW_FUNC__ << " hnf_index=" << h << " group order=" << vaction[h].perms.size() << " sets=" << m_enum_orbits[h] << endl;}
      }
    } else {
      //sampled decorations (--pocc_sample_number, --pocc_sample_rate, --pocc_sample_hash)  //CO20261006
      //the first round of UFF, so both enumerators group the same draws: FirstRandomSamplingWholeSpace() for SAMPLE_NUMBER and
      //SAMPLE_HASH (uniform over every decoration of every HNF matrix), FirstRandomSampling() for SAMPLE_RATE
      //a set holds the drawn decorations that are the same structure (found through the whole set of the first one), each counted once:
      //its degeneracy is the number drawn, as UFF counts its sampled bins, so the ensemble average stays a sample average
      //(weighting a drawn set by its whole size would count large sets twice, once in the draw and once in the weight)
      const bool whole_space=(usePOccSampleNumber(sample_string,sample_number) || usePOccSampleHash(sample_string,sample_hash_hnf,sample_hash_site_config));
      const vector<unsigned long long int> vdrawn=(whole_space ? FirstRandomSamplingWholeSpace() : FirstRandomSampling(hnf_count,types_config_permutations_count));  //hnf_index*types_config_permutations_count+site_config_index
      std::map<std::pair<unsigned long long int,unsigned long long int>,uint> set_of;  //every decoration of the sets found so far
      vector<vector<POccSuperCell> > vsampled;  //per set: the drawn decorations
      vector<bool> vsuperperiodic;
      vector<vector<int> > config;
      for(size_t idrawn=0;idrawn<vdrawn.size();idrawn++){
        const unsigned long long int h=vdrawn[idrawn]/types_config_permutations_count;
        const unsigned long long int c=vdrawn[idrawn]%types_config_permutations_count;
        if(h>=hnf_count){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"drawn decoration outside the HNF matrices",_RUNTIME_ERROR_);}
        const std::pair<unsigned long long int,unsigned long long int> key(h,c);
        if(!set_of.count(key)){
          config=getSiteConfiguration(c);
          if(rankDecoration(config)!=c){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"getSiteConfiguration() and rankDecoration() disagree",_RUNTIME_ERROR_);}
          const bool superperiodic=getMembers(h,config,false,stabilizer_orbit);
          const uint iset=vsampled.size();
          for(uint h2=0;h2<hnf_count;h2++){
            for(std::set<unsigned long long int>::const_iterator it=members[h2].begin();it!=members[h2].end();++it){
              if(!set_of.insert(std::make_pair(std::make_pair((unsigned long long int)h2,*it),iset)).second){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"decoration in two sets",_RUNTIME_ERROR_);}
              if(h2!=h){m_enum_merged_across++;}
            }
          }
          m_enum_merged_hnf+=members[h].size()-stabilizer_orbit;
          vsampled.push_back(vector<POccSuperCell>(0));
          vsuperperiodic.push_back(superperiodic);
        }
        psc.m_hnf_index=h;
        psc.m_site_config_index=c;
        vsampled[set_of[key]].push_back(psc);
        nsampled++;
      }
      for(uint iset=0;iset<vsampled.size();iset++){
        POccSuperCellSet pscs;
        pscs.m_psc_set=vsampled[iset];
        std::sort(pscs.m_psc_set.begin(),pscs.m_psc_set.end());
        const POccSuperCell& rep=pscs.m_psc_set[0];  //smallest sampled decoration
        m_enum_orbits[rep.m_hnf_index]++;
        if(vsuperperiodic[iset]){m_enum_superperiodic[rep.m_hnf_index]++;}
        if(uff_energies){
          vpsc_rep.push_back(rep);
          vv_types_config_rep.push_back(getSiteConfiguration(rep.m_site_config_index));
        }
        l_supercell_sets.push_back(pscs);
      }
      m_enum_skipped=total_permutations_count-nsampled;
    }
    m_enum_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();

    //UFF energy of each representative, for reference and the output files; not needed to count (--pocc_count_unique)
    if(!uff_energies){
      for(std::list<POccSuperCellSet>::iterator it=l_supercell_sets.begin();it!=l_supercell_sets.end();++it){
        for(uint i=0;i<(*it).m_psc_set.size();i++){(*it).m_psc_set[i].m_energy_uff=0.0;}
      }
    } else {
      //HNF matrix by HNF matrix, so a thread holds one UFF cluster at a time (the clusters of all 158 HNF matrices
      //of rocksalt (MgCoNiCuZn)O at n=30 took more than 5 GB); the clusters are built one at a time, as UFF builds them  //CO20261006
      vector<vector<size_t> > vrep_hnf(hnf_count);
      for(size_t i=0;i<vpsc_rep.size();i++){vrep_hnf[vpsc_rep[i].m_hnf_index].push_back(i);}
      vector<uint> vhnf_rep;
      for(uint h=0;h<hnf_count;h++){if(!vrep_hnf[h].empty()){vhnf_rep.push_back(h);}}
      std::mutex m_cluster;
      std::function<void(int)> fn=[&](int task){
        const uint h=vhnf_rep[task];
        POccUFFEnergyAnalyzer ea(energy_analyzer);
        {
          std::lock_guard<std::mutex> lg_cluster(m_cluster);
          xmatrix<double> hnf=m_vhnf_enum[h];
          ea.getCluster(hnf);
        }
        for(size_t j=0;j<vrep_hnf[h].size();j++){
          const size_t i=vrep_hnf[h][j];
          ea.setBonds(vv_types_config_rep[i]);
          vpsc_rep[i].m_energy_uff=aurostd::round(ea.getUFFEnergy(),12);  //as calculatePOccSuperCellUFF()
        }
      };
      xthread::xThread xt(KBIN::get_NCPUS(m_kflags),1);
      xt.run((int)vhnf_rep.size(),fn);
      uint iset=0;
      for(std::list<POccSuperCellSet>::iterator it=l_supercell_sets.begin();it!=l_supercell_sets.end();++it,++iset){
        for(uint i=0;i<(*it).m_psc_set.size();i++){(*it).m_psc_set[i].m_energy_uff=vpsc_rep[iset].m_energy_uff;}
      }
    }
    if(sampling){l_supercell_sets.sort();}  //by representative, as the full enumeration lists them

    unsigned long long int norbits=0,nsuperperiodic=0;
    for(uint i=0;i<m_enum_orbits.size();i++){norbits+=m_enum_orbits[i];nsuperperiodic+=m_enum_superperiodic[i];}
    message << "ENUM: " << norbits << " unique supercells (" << nsuperperiodic << " superperiodic) from ";
    if(sampling){message << nsampled << " sampled decorations (of " << total_permutations_count << ")";}
    else {message << total_permutations_count << " decorations";}
    message << " of " << hnf_count << " HNF matrices in " << std::fixed << std::setprecision(3) << m_enum_seconds << " s";
    pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_MESSAGE_);
    if(nsuperperiodic>0){
      message << "Superperiodic supercells: " << m_enum_merged_hnf << " decorations joined by operations that change the superlattice, ";
      message << m_enum_merged_across << " decorations joined from other HNF matrices";
      pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_MESSAGE_);
    }
    for(uint i=0;i<m_enum_orbits.size();i++){
      message << "HNF " << i << ": " << m_vhnf_multiplicity[i] << " equivalent HNF matrices, " << m_enum_rotations[i] << " operations keep the superlattice, group order " << m_enum_group_order[i];
      message << ", " << m_enum_orbits[i] << " unique (" << m_enum_superperiodic[i] << " superperiodic)";
      if(i>=m_hnf_count_lattice){message << " [separated from HNF " << m_vhnf_lattice_class[i] << " by the point group of the colored parent only]";}
      pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_MESSAGE_);
    }
  }

  //--------------------------------------------------------------------------------
  // comparison
  //--------------------------------------------------------------------------------
  /// @brief run UFF and ENUM and report where the UFF bins and the ENUM sets differ
  /// @note leaves the ENUM result in l_supercell_sets
  /// @authors
  /// @mod{CO,20261005,created}
  void POccCalculator::compareEnumerators() {
    stringstream message;
    string sample_string="";
    unsigned long long int sample_number=0,sample_hash_hnf=0,sample_hash_site_config=0;
    if(usePOccSampleNumber(sample_string,sample_number) || usePOccSampleRate(sample_string) || usePOccSampleHash(sample_string,sample_hash_hnf,sample_hash_site_config)){
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"--pocc_enumerator=compare needs every decoration; drop --pocc_sample_number, --pocc_sample_rate and --pocc_sample_hash",_INPUT_ILLEGAL_);
    }
    const bool count_unique_fast=m_count_unique_fast;
    m_count_unique_fast=false;  //the UFF bins are needed

    //UFF, as POCC runs it by default
    enumerator_mode.clear();enumerator_mode.flag("UFF",true);
    m_hnf_from_list=false;
    getTotalPermutationsCount();
    std::chrono::steady_clock::time_point t0=std::chrono::steady_clock::now();
    calculate();
    const double seconds_uff=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    const unsigned long long int hnf_count_uff=hnf_count;
    const unsigned long long int nconfigs=types_config_permutations_count;
    vector<vector<uint> > bin_of(hnf_count_uff,vector<uint>(nconfigs,AUROSTD_MAX_UINT));
    vector<double> vbin_energy;
    vector<unsigned long long int> vbin_hnf;
    uint ibin=0;
    for(std::list<POccSuperCellSet>::const_iterator it=l_supercell_sets.begin();it!=l_supercell_sets.end();++it,++ibin){
      for(uint i=0;i<(*it).m_psc_set.size();i++){bin_of[(*it).m_psc_set[i].m_hnf_index][(*it).m_psc_set[i].m_site_config_index]=ibin;}
      vbin_energy.push_back((*it).getUFFEnergy());
      vbin_hnf.push_back((*it).getSuperCell().m_hnf_index);
    }
    const uint nbins=ibin;
    l_supercell_sets.clear();

    //ENUM
    enumerator_mode.clear();enumerator_mode.flag("ENUM",true);
    getTotalPermutationsCount();
    calculate();
    m_count_unique_fast=count_unique_fast;
    if(types_config_permutations_count!=nconfigs){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"decoration counts differ between the enumerators",_RUNTIME_ERROR_);}
    vector<vector<uint> > orbit_of(hnf_count_uff,vector<uint>(nconfigs,AUROSTD_MAX_UINT));
    vector<unsigned long long int> vorbit_size,vorbit_hnf,vorbit_rep;
    uint iorbit=0,norbits_common=0;
    for(std::list<POccSuperCellSet>::const_iterator it=l_supercell_sets.begin();it!=l_supercell_sets.end();++it,++iorbit){
      vorbit_size.push_back((*it).getDegeneracy());
      vorbit_hnf.push_back((*it).getSuperCell().m_hnf_index);
      vorbit_rep.push_back((*it).getSuperCell().m_site_config_index);
      if(vorbit_hnf.back()<hnf_count_uff){norbits_common++;}  //the representative is the smallest H, so a set touching the shared HNF matrices starts on one
      for(uint i=0;i<(*it).m_psc_set.size();i++){
        if((*it).m_psc_set[i].m_hnf_index<hnf_count_uff){orbit_of[(*it).m_psc_set[i].m_hnf_index][(*it).m_psc_set[i].m_site_config_index]=iorbit;}
      }
    }
    const uint norbits=iorbit;

    //contingency between UFF bins and ENUM sets (HNF matrices both enumerators list)
    vector<std::set<uint> > orbits_in_bin(nbins);
    vector<std::map<unsigned long long int,std::set<uint> > > bins_of_orbit(norbits);  //per HNF matrix
    for(unsigned long long int h=0;h<hnf_count_uff;h++){
      for(unsigned long long int c=0;c<nconfigs;c++){
        if(bin_of[h][c]==AUROSTD_MAX_UINT || orbit_of[h][c]==AUROSTD_MAX_UINT){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"decoration missing from an enumerator",_RUNTIME_ERROR_);}
        orbits_in_bin[bin_of[h][c]].insert(orbit_of[h][c]);
        bins_of_orbit[orbit_of[h][c]][h].insert(bin_of[h][c]);
      }
    }
    uint nmerged=0,nsplit=0,nmatched=0,nacross=0;
    unsigned long long int nconfigs_merged=0,nconfigs_across=0;
    for(uint b=0;b<nbins;b++){
      if(orbits_in_bin[b].size()>1){
        nmerged++;
        message << "UFF bin " << b+1 << " (HNF " << vbin_hnf[b] << ", UFF energy " << std::fixed << std::setprecision(8) << vbin_energy[b] << ") holds " << orbits_in_bin[b].size() << " inequivalent supercells:";
        for(std::set<uint>::const_iterator it=orbits_in_bin[b].begin();it!=orbits_in_bin[b].end();++it){
          message << " H" << vorbit_hnf[*it] << "C" << vorbit_rep[*it] << " (degeneracy " << vorbit_size[*it] << ")";
          nconfigs_merged+=vorbit_size[*it];
        }
        pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_WARNING_);
      }
    }
    for(uint o=0;o<norbits;o++){
      bool split=false;
      for(std::map<unsigned long long int,std::set<uint> >::const_iterator it=bins_of_orbit[o].begin();it!=bins_of_orbit[o].end();++it){
        if(it->second.size()>1){
          split=true;
          message << "Supercell H" << vorbit_hnf[o] << "C" << vorbit_rep[o] << " (degeneracy " << vorbit_size[o] << ") is split over " << it->second.size() << " UFF bins on HNF " << it->first;
          pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_WARNING_);
        }
      }
      if(split){nsplit++;}
      if(bins_of_orbit[o].size()>1){nacross++;nconfigs_across+=vorbit_size[o];}
      if(!split && bins_of_orbit[o].size()==1){
        const std::set<uint>& bins=bins_of_orbit[o].begin()->second;
        if(orbits_in_bin[*bins.begin()].size()==1){nmatched++;}
      }
    }
    unsigned long long int norbits_extra=0,nconfigs_extra=0;
    for(uint o=0;o<norbits;o++){if(vorbit_hnf[o]>=hnf_count_uff){norbits_extra++;nconfigs_extra+=vorbit_size[o];}}

    message << "Comparison of the enumerators (n=" << n_hnf << ", " << nconfigs << " decorations per HNF matrix):" << endl;
    message << "  HNF matrices: UFF " << hnf_count_uff << " (lattice point group), ENUM " << hnf_count << " (point group of the colored parent)" << endl;
    message << "  unique supercells on the shared HNF matrices: UFF " << nbins << ", ENUM " << norbits_common << endl;
    message << "  UFF bins that hold more than one inequivalent supercell: " << nmerged << " (" << nconfigs_merged << " decorations)" << endl;
    message << "  supercells UFF splits over more than one bin of one HNF matrix: " << nsplit << endl;
    message << "  superperiodic supercells on more than one HNF matrix (UFF bins each HNF matrix separately): " << nacross << " (" << nconfigs_across << " decorations)" << endl;
    message << "  supercells that are exactly one UFF bin: " << nmatched << endl;
    message << "  supercells on HNF matrices that only the point group of the colored parent separates: " << norbits_extra << " (" << nconfigs_extra << " decorations)" << endl;
    message << "  time: UFF " << std::fixed << std::setprecision(3) << seconds_uff << " s, ENUM " << m_enum_seconds << " s";
    pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_COMPLETE_);
  }
} // namespace pocc

#endif  // _AFLOW_POCC_ENUMERATION_CPP_

// ***************************************************************************
// *                                                                         *
// *           Aflow COREY OSES - Johns Hopkins University 2022-2026         *
// *                                                                         *
// ***************************************************************************
