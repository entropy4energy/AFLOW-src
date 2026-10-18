// ***************************************************************************
// *                                                                         *
// *          Aflow COREY OSES - Johns Hopkins University 2025-2026          *
// *           Aflow KATIE CARIAGA - Johns Hopkins University 2025           *
// *                                                                         *
// ***************************************************************************
//aflow_grab_protos.cpp
// Prototype redecoration (KT20250210, reworked CO20260930).
//
// For every n-ary AFLOW prototype (ANRL list, each parameter set) whose
// compound contains one of O, S, Se, Te, F, Cl, Br, I:
//  1. build the prototype compound itself (species in label order, i.e.,
//     the compound's elements in alphabetical order);
//  2. get the oxidation state of each species from CCE (single-anion
//     systems with one integer oxidation state per species only); its sign
//     makes the species a cation site (> 0) or an anion site (< 0);
//  3. put any given cation on a cation site and any given anion on an anion
//     site, without repeating an element, and keep the combination if some
//     choice of oxidation states makes the cell neutral: all known states of
//     the cations (xelement::oxidation_states), the preferred states of the
//     anions (xelement::oxidation_states_preferred, e.g., O only as -2)
//     (CO20261003 - was: the same oxidation state as the prototype's species);
//  4. write each new compound as label.DECORATION:El1:El2:..., the elements in
//     alphabetical order and DECORATION[p] the letter of the element on
//     species p, reduced to one representative per class of equivalent
//     decorations (XtalFinder, the same classes as aflow --unique_atom_decorations).

#include "aflow.h"
#include "aflow_pflow.h"
#include "aflow_grab_protos.h"
#include "aflow_cce.h"

using std::vector;
using std::string;
using std::map;

namespace {
  //CO20260930 - stoichiometry of an AFLOW label, e.g., A2BC4_cF56_... -> 2,1,4
  bool labelStoichiometry(const string& label,vector<uint>& stoich) {
    stoich.clear();
    string s=label.substr(0,label.find('_'));
    size_t i=0;
    while(i<s.size()){
      if(!isupper(s[i])){return false;}
      i++;
      string num="";
      while(i<s.size() && isdigit(s[i])){num+=s[i];i++;}
      stoich.push_back(num.empty()?1:aurostd::string2utype<uint>(num));
    }
    return !stoich.empty();
  }

  //CO20260930 - plain formulas only (TiO2, O2W1, Al18Cr2Mg3): elements in
  //alphabetical order with their counts; false for anything else (groups,
  //fractions, hydrates, names)
  bool parseCompound(const string& compound,vector<string>& elements,vector<uint>& counts) {
    elements.clear();counts.clear();
    map<string,uint> comp;
    size_t i=0;
    while(i<compound.size()){
      if(!isupper(compound[i])){return false;}
      string el(1,compound[i]);i++;
      while(i<compound.size() && islower(compound[i])){el+=compound[i];i++;}
      string num="";
      while(i<compound.size() && isdigit(compound[i])){num+=compound[i];i++;}
      if(xelement::xelement::isElement(el)==0){return false;}
      uint n=(num.empty()?1:aurostd::string2utype<uint>(num));
      if(n==0){return false;}
      comp[el]+=n;
    }
    for(map<string,uint>::const_iterator it=comp.begin();it!=comp.end();++it){elements.push_back(it->first);counts.push_back(it->second);}  //map: alphabetical
    return !elements.empty();
  }

  uint gcd2(uint a,uint b) {while(b!=0){uint t=a%b;a=b;b=t;} return a;}

  //CO20261004 - does a +U run of these elements differ from the --noldau run? AFLOW sets +U
  //only if the U+J of some species is nonzero (AVASP_ADD_LDAU(), aflow_avasp.cpp) and always
  //for Gd and Ce (AVASP_MakeSingleAFLOWIN()); otherwise --ldau writes the same aflow.in into
  //the same directory as --noldau
  bool speciesTakeU(const vector<string>& elements) {
    bool LDAU=false;
    vector<string> vLDAUspecies;vector<uint> vLDAUtype;vector<int> vLDAUL;vector<double> vLDAUU,vLDAUJ;
    for(size_t i=0;i<elements.size();i++){
      if(elements[i]=="Gd" || elements[i]=="Ce"){return true;}
      AVASP_Get_LDAU_Parameters(elements[i],LDAU,vLDAUspecies,vLDAUtype,vLDAUL,vLDAUU,vLDAUJ);
    }
    double UJSUM=0.0;
    for(size_t i=0;i<vLDAUU.size()&&i<vLDAUJ.size();i++){UJSUM+=aurostd::abs(vLDAUU[i])+aurostd::abs(vLDAUJ[i]);}
    return (UJSUM>0.01);
  }

  //CO20261003 - is there a choice of one state per species (from ion_states of the
  //element on it) such that sum(count*state)=0?
  bool chargeNeutralPossible(const vector<string>& combo,const map<string,vector<double> >& ion_states,const std::deque<int>& counts,uint isp=0,double charge=0.0) {
    if(isp==combo.size()){return aurostd::isequal(charge,0.0,1e-6);}
    map<string,vector<double> >::const_iterator it=ion_states.find(combo[isp]);
    if(it==ion_states.end()){return false;}
    for(size_t k=0;k<it->second.size();k++){
      if(chargeNeutralPossible(combo,ion_states,counts,isp+1,charge+it->second[k]*(double)counts[isp])){return true;}
    }
    return false;
  }

  vector<uint> reduceByGCD(const vector<uint>& v) {
    uint g=0;
    for(size_t i=0;i<v.size();i++){g=gcd2(g,v[i]);}
    vector<uint> out=v;
    if(g>1){for(size_t i=0;i<out.size();i++){out[i]/=g;}}
    return out;
  }
}

namespace pflow {
    Redecoration::Redecoration() {
        free();
    }

    Redecoration::Redecoration(const int NARY,const vector<string> cations,const vector<string> anions) {
        initialize(NARY,cations,anions);
    }

    Redecoration::~Redecoration() {
        free();
    }

    void Redecoration::free() {
        initialized=false;

        NARY=AUROSTD_MAX_INT;
        vcations.clear();
        vanions.clear();
        ANIONS2SEARCH.clear();

        vlabel.clear();
        velements.clear();

        aflow_bin="aflow";  //CO20260930
        commands_file="";  //CO20260930
        ldau_runs=false;  //CO20261004 - --noldau runs only by default
        vproto_redecorated.clear();  //CO20260930
        redecoration_cmds.clear();
        chaos=aflowlib::ChaosRuns();  //CO20261004
        chaos_checked=false;
        nruns_in_chaos=0;
        decoration_representative.clear();
    }

    void Redecoration::clear() {
        free();
    }

    //copy constructor
    Redecoration::Redecoration(const Redecoration& b) {
        copy(b);
    }

    //copy constructor
    const Redecoration& Redecoration::operator=(const Redecoration& b) {
        if(this!=&b){
            free();
            copy(b);
        }
        return *this;
    }

    //copy contructor
    void Redecoration::copy(const Redecoration& b) {
        if(this!=&b){
            initialized=b.initialized;

            NARY=b.NARY;
            vcations=b.vcations;
            vanions=b.vanions;
            ANIONS2SEARCH=b.ANIONS2SEARCH;

            vlabel=b.vlabel;
            velements=b.velements;

            aflow_bin=b.aflow_bin;
            commands_file=b.commands_file;
            ldau_runs=b.ldau_runs;
            vproto_redecorated=b.vproto_redecorated;
            redecoration_cmds=b.redecoration_cmds;
            chaos=b.chaos;
            chaos_checked=b.chaos_checked;
            nruns_in_chaos=b.nruns_in_chaos;
            decoration_representative=b.decoration_representative;
        }
    }

    bool Redecoration::initialize(const int _NARY,const vector<string> cations,const vector<string> anions) {
        free();
        bool LDEBUG=(FALSE || XHOST.DEBUG);
        if(LDEBUG){cerr << __AFLOW_FUNC__ << " BEGIN" << endl;}

        if(_NARY<1){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"the number of species must be at least 1",_INPUT_ILLEGAL_);}  //CO20260930
        NARY = _NARY;
        ANIONS2SEARCH = {
            "O", "S", "Se", "Te", "F", "Cl", "Br", "I"
        };

        //new ions //CO20260930 - unknown elements are an input error
        for(size_t i=0;i<cations.size();i++){
          uint Z=xelement::xelement::isElement(cations[i]);
          if(Z==0){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"unknown element "+cations[i]+" in the cations",_INPUT_ILLEGAL_);}
          vcations.push_back(xelement::xelement(Z));
        }
        for(size_t i=0;i<anions.size();i++){
          uint Z=xelement::xelement::isElement(anions[i]);
          if(Z==0){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"unknown element "+anions[i]+" in the anions",_INPUT_ILLEGAL_);}
          vanions.push_back(xelement::xelement(Z));
        }

        //prototypes //CO20260930 - from the ANRL list (one label per parameter set)
        //instead of the text of README_PROTO.TXT; the compound's elements are
        //put in alphabetical order, the species order of the label (TiO2 is
        //A2B_tP6_136_f_a with A=O, B=Ti), and must match the label's stoichiometry
        vector<string> vproto,vproto_label,vproto_Pearson_symbol,vproto_params,vproto_Strukturbericht,vproto_prototype,vproto_dialect;
        vector<uint> vproto_nspecies,vproto_natoms,vproto_spacegroup,vproto_nunderscores,vproto_nparameters;
        anrl::PrototypeANRL_LoadList(vproto,vproto_label,vproto_nspecies,vproto_natoms,vproto_spacegroup,vproto_nunderscores,
            vproto_nparameters,vproto_Pearson_symbol,vproto_params,vproto_Strukturbericht,vproto_prototype,vproto_dialect);
        vector<string> compounds,elements;
        vector<uint> counts,stoich;
        for(size_t i=0;i<vproto_label.size();i++){
          if((int)vproto_nspecies[i]!=NARY){continue;}
          if(!labelStoichiometry(vproto_label[i],stoich) || (int)stoich.size()!=NARY){continue;}
          aurostd::string2tokens(vproto_prototype[i],compounds,"/");
          //a prototype whose only parameter is the lattice parameter takes no
          //suffix (and has one parameter set); any other takes -NNN, even with
          //one set (anrl::extractANRLPrototypeParameterValues())
          bool lattice_parameter_only=(vproto_params[i]=="a");
          if(lattice_parameter_only && compounds.size()>1){compounds.resize(1);}
          for(size_t j=0;j<compounds.size();j++){
            if(!parseCompound(compounds[j],elements,counts)){continue;}
            if((int)elements.size()!=NARY){continue;}
            if(reduceByGCD(counts)!=reduceByGCD(stoich)){
              if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << vproto_label[i] << ": " << compounds[j] << " does not match the label, skipped" << endl;}
              continue;
            }
            bool found=false;
            for(size_t k=0;k<ANIONS2SEARCH.size()&&!found;k++){found=aurostd::WithinList(elements,ANIONS2SEARCH[k]);}
            if(!found){continue;}
            string label=vproto_label[i];
            if(!lattice_parameter_only){label+="-"+aurostd::PaddedPRE(aurostd::utype2string(j+1),3,"0");}
            vlabel.push_back(label);
            velements.push_back(elements);
          }
        }
        if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << vlabel.size() << " prototypes with " << NARY << " species" << endl;}

        initialized=true;
        return initialized;
    }

    //CO20260930 - the new compounds of one prototype, as label.DECORATION:El1:El2...
    vector<string> Redecoration::RedecoratePrototype(const string& label,const vector<string>& elements) const {
        bool LDEBUG=(FALSE || XHOST.DEBUG);
        vector<string> vout;
        uint nspecies=elements.size();
        if(nspecies==0){return vout;}

        //the prototype compound
        xstructure xstr;
        try {
          aurostd::xoption proto_flags;
          proto_flags.push_attached("PROTO",label+":"+aurostd::joinWDelimiter(elements,":"));
          xstr=pflow::PROTO_LIBRARIES(proto_flags);
        }
        catch(aurostd::xerror& excpt) {
          if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << label << ": " << excpt.what() << endl;}
          return vout;
        }
        if(xstr.species.size()!=nspecies){return vout;}
        for(uint isp=0;isp<nspecies;isp++){
          if(KBIN::VASP_PseudoPotential_CleanName(xstr.species[isp])!=elements[isp]){
            if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << label << ": species " << aurostd::joinWDelimiter(xstr.species,",") << " differ from " << aurostd::joinWDelimiter(elements,",") << endl;}
            return vout;
          }
        }

        //oxidation states (CCE), quietly; steps of cce::getOxStatesFromENs(xstructure&,ofstream&)
        ofstream FileMESSAGE;
        ostringstream oss_quiet;
        xstructure xstr_cce=xstr;
        vector<double> ox_atoms;
        try {
          aurostd::xoption cce_flags=cce::init_flags();
          cce::optimizeStructure(xstr_cce,cce_flags);
          cce::CCE_Variables cce_vars=cce::init_variables(xstr_cce);
          cce_vars.anion_species=cce::determine_anion_species(cce_flags,xstr_cce,cce_vars);
          cce_vars.multi_anion_atoms=cce::check_for_multi_anion_system(xstr_cce,cce_flags,cce_vars);
          if(cce_vars.multi_anion_atoms.empty() || cce_vars.multi_anion_species.size()>0){  //multi-anion systems are not redecorated
            if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << label << ": multi-anion system, skipped" << endl;}
            return vout;
          }
          cce_vars.multi_anion_corrections_atom.resize(cce_vars.multi_anion_species.size(),vector<vector<double> >(cce_vars.vfunctionals.size()*2,vector<double>(xstr_cce.atoms.size())));
          if(cce_vars.anion_species=="O" || cce_flags.flag("O_MULTI_ANION_SPECIES")){cce::checkPerSuperOxides(xstr_cce,cce_flags,cce_vars,FileMESSAGE);}
          cce_flags.flag("CORRECTABLE",TRUE);
          ox_atoms=cce::getOxStatesFromENs(xstr_cce,cce_flags,cce_vars,FileMESSAGE,oss_quiet);
        }
        catch(aurostd::xerror& excpt) {
          if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << label << ": CCE: " << excpt.what() << endl;}
          return vout;
        }
        if(ox_atoms.size()!=xstr_cce.atoms.size()){return vout;}

        //one integer oxidation state per species, summing to zero
        vector<double> ox_species(nspecies,0.0);
        vector<bool> ox_set(nspecies,false);
        for(uint iat=0;iat<xstr_cce.atoms.size();iat++){
          uint isp=xstr_cce.atoms[iat].type;
          if(isp>=nspecies){return vout;}
          if(!ox_set[isp]){ox_species[isp]=ox_atoms[iat];ox_set[isp]=true;}
          else if(!aurostd::isequal(ox_species[isp],ox_atoms[iat],1e-6)){
            if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << label << ": " << elements[isp] << " has more than one oxidation state, skipped" << endl;}
            return vout;
          }
        }
        double charge=0.0;
        vector<int> ox(nspecies,0);
        for(uint isp=0;isp<nspecies;isp++){
          if(!ox_set[isp]){return vout;}
          ox[isp]=(int)std::round(ox_species[isp]);
          if(ox[isp]==0 || !aurostd::isequal(ox_species[isp],(double)ox[isp],1e-6)){
            if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << label << ": oxidation state " << ox_species[isp] << " of " << elements[isp] << ", skipped" << endl;}
            return vout;
          }
          charge+=ox[isp]*(double)xstr_cce.num_each_type[isp];
        }
        if(!aurostd::isequal(charge,0.0,1e-6)){return vout;}
        if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << label << ": oxidation states " << aurostd::joinWDelimiter(ox,",") << endl;}

        //candidate ions per species (CO20261003 - charge-neutral rule): a cation site takes
        //any given cation with a positive state among all its known states, an anion site any
        //given anion with a negative state among its preferred states (O only as -2)
        vector<vector<string> > candidates(nspecies);
        map<string,vector<double> > ion_states;  //symbol -> states usable on its kind of site
        for(uint isp=0;isp<nspecies;isp++){
          bool cation_site=(ox[isp]>0);
          const vector<xelement::xelement>& vions=(cation_site?vcations:vanions);
          for(size_t ion=0;ion<vions.size();ion++){
            const vector<double>& vstates=(cation_site?vions[ion].oxidation_states:vions[ion].oxidation_states_preferred);
            vector<double> states;
            for(size_t k=0;k<vstates.size();k++){
              if(aurostd::isequal(vstates[k],(double)NNN,1e-6)){continue;}  //no data
              if((cation_site && vstates[k]>1e-6) || (!cation_site && vstates[k]<-1e-6)){states.push_back(vstates[k]);}
            }
            if(states.empty()){continue;}
            ion_states[vions[ion].symbol]=states;
            candidates[isp].push_back(vions[ion].symbol);
          }
          if(candidates[isp].empty()){return vout;}
        }

        //combinations without a repeated element (a repeated element would merge two species)
        //and with a choice of states that makes the cell neutral
        vector<vector<string> > combinations;
        vector<uint> index(nspecies,0);
        while(true){
          vector<string> combo;
          for(uint isp=0;isp<nspecies;isp++){combo.push_back(candidates[isp][index[isp]]);}
          vector<string> sorted=combo;std::sort(sorted.begin(),sorted.end());
          if(std::unique(sorted.begin(),sorted.end())==sorted.end() && chargeNeutralPossible(combo,ion_states,xstr_cce.num_each_type)){combinations.push_back(combo);}
          int isp=(int)nspecies-1;
          while(isp>=0){
            index[isp]++;
            if(index[isp]<candidates[isp].size()){break;}
            index[isp]=0;isp--;
          }
          if(isp<0){break;}
        }
        if(combinations.empty()){return vout;}

        //classes of equivalent decorations of this prototype (txt output of XtalFinder)
        map<string,string> representative;
        {
          bool print_json=XHOST.vflag_control.flag("PRINT_MODE::JSON");
          XHOST.vflag_control.flag("PRINT_MODE::JSON",false);
          try {
            ostringstream oss_xtalfinder;
            XtalFinderCalculator xtal_finder(DEFAULT_XTALFINDER_MISFIT_MATCH,DEFAULT_XTALFINDER_MISFIT_FAMILY,FileMESSAGE,1,oss_xtalfinder);
            aurostd::xoption comparison_options=compare::loadDefaultComparisonOptions("permutation");
            stringstream results_ss;
            xstructure xstr_perm=xstr;
            xtal_finder.getUniquePermutations(xstr_perm,comparison_options,results_ss,1,false);
            vector<string> lines,members;
            aurostd::string2vectorstring(results_ss.str(),lines);
            for(size_t i=0;i<lines.size();i++){
              if(aurostd::substring2bool(lines[i],"Unique atom decorations")){continue;}
              aurostd::string2tokens(lines[i],members,"=");
              for(size_t j=0;j<members.size();j++){members[j]=aurostd::RemoveWhiteSpaces(members[j]);}
              if(members.empty() || members[0].empty()){continue;}
              for(size_t j=0;j<members.size();j++){representative[members[j]]=members[0];}
            }
          }
          catch(aurostd::xerror& excpt) {
            if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << label << ": XtalFinder: " << excpt.what() << endl;}
          }
          XHOST.vflag_control.flag("PRINT_MODE::JSON",print_json);
        }
        decoration_representative[label]=representative;  //CO20261004 - equivalent decorations of runs in CHAOS

        //label.DECORATION:El1:El2... with the elements in alphabetical order
        for(size_t ic=0;ic<combinations.size();ic++){
          vector<string> sorted=combinations[ic];std::sort(sorted.begin(),sorted.end());
          string dec="";
          for(uint isp=0;isp<nspecies;isp++){
            size_t k=std::find(sorted.begin(),sorted.end(),combinations[ic][isp])-sorted.begin();
            dec+=(char)('A'+k);
          }
          if(representative.count(dec)){dec=representative[dec];}
          vout.push_back(label+"."+dec+":"+aurostd::joinWDelimiter(sorted,":"));
        }
        std::sort(vout.begin(),vout.end());
        vout.erase(std::unique(vout.begin(),vout.end()),vout.end());
        return vout;
    }

    vector<string> Redecoration::GrabProtos() {
        FindCompounds();
        return WriteCommands();
    }

    //CO20261004 - the new compounds of every prototype
    void Redecoration::FindCompounds() {
        bool LDEBUG=(FALSE || XHOST.DEBUG);
        vproto_redecorated.clear();
        for(size_t xp=0;xp<vlabel.size();xp++){
          vector<string> vproto=RedecoratePrototype(vlabel[xp],velements[xp]);
          if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << vlabel[xp] << " (" << aurostd::joinWDelimiter(velements[xp],"") << "): " << vproto.size() << " new compounds" << endl;}
          vproto_redecorated.insert(vproto_redecorated.end(),vproto.begin(),vproto.end());
        }
        std::sort(vproto_redecorated.begin(),vproto_redecorated.end());
        vproto_redecorated.erase(std::unique(vproto_redecorated.begin(),vproto_redecorated.end()),vproto_redecorated.end());
    }

    //CO20261004 - the runs in CHAOS of every chemical system of the new compounds: from the list
    //file if given (aflow --chaos_runs), then one query to CHAOS per system the file does not
    //have; throws if CHAOS cannot be read, so that no run is written as new by mistake
    void Redecoration::ReadChaosRuns(const string& chaos_runs_file) {
        if(!chaos_runs_file.empty()){chaos.readFile(chaos_runs_file);}
        else{  //CO20261004 - the list of every run in CHAOS, if it is fresh; otherwise CHAOS is asked per system
          string list=aflowlib::ChaosRuns::defaultListFile(),problem="";
          if(aurostd::FileExist(list)){
            aflowlib::ChaosRuns full;
            if(full.readListFile(list,problem)){full.url=chaos.url;chaos=full;}
            else{cerr << __AFLOW_FUNC__ << " " << problem << "; asking CHAOS per system instead" << endl;}
          }
        }
        vector<vector<string> > vsystems;
        for(size_t i=0;i<vproto_redecorated.size();i++){
          vector<string> tokens;aurostd::string2tokens(vproto_redecorated[i],tokens,":");
          if(tokens.size()<2){continue;}
          tokens.erase(tokens.begin());
          vsystems.push_back(tokens);
        }
        chaos.queryMissing(vsystems);
        chaos_checked=true;
    }

    //CO20261004 - label.DECORATION:El1:El2...[:LDAU2] is in CHAOS under its own name or under an
    //equivalent decoration of the same prototype
    bool Redecoration::RunInChaos(const string& run_name) const {
        if(chaos.hasRun(run_name)){return true;}
        size_t colon=run_name.find(':');
        string head=run_name.substr(0,colon),tail=(colon==string::npos?string(""):run_name.substr(colon));
        size_t dot=head.find_last_of('.');
        if(dot==string::npos){return false;}
        string label=head.substr(0,dot),decoration=head.substr(dot+1);
        std::map<string,std::map<string,string> >::const_iterator it=decoration_representative.find(label);
        if(it==decoration_representative.end()){return false;}
        std::map<string,string>::const_iterator jt=it->second.find(decoration);
        string representative=(jt==it->second.end()?decoration:jt->second);
        for(jt=it->second.begin();jt!=it->second.end();++jt){
          if(jt->second==representative && chaos.hasRun(label+"."+jt->first+tail)){return true;}
        }
        return false;
    }

    //CO20261004 - the runs of the new compounds that are not in CHAOS
    vector<string> Redecoration::WriteCommands() {
        bool LDEBUG=(FALSE || XHOST.DEBUG);
        redecoration_cmds.clear();
        nruns_in_chaos=0;
        for(size_t i=0;i<vproto_redecorated.size();i++){
          const string& proto=vproto_redecorated[i];
          //CO20261004 - --noldau runs (relaxation and static) for the convex hull; +U runs only
          //with ldau_runs (special projects), only where a species takes a U value (otherwise
          //AFLOW writes the --noldau run again), and on to the band structure
          //(AFLOW appends :LDAU2 to the directory of a +U run)
          string flags=" --bader --keep_nomix --aflow_db_run --force";
          string base=aflow_bin+" --aflow_proto="+proto;
          vector<string> tokens;aurostd::string2tokens(proto,tokens,":");
          vector<string> elements(tokens.begin()+(tokens.empty()?0:1),tokens.end());
          vector<std::pair<string,string> > runs;  //run name, command
          if(ldau_runs && speciesTakeU(elements)){runs.push_back(std::make_pair(proto+":LDAU2",base+" --run_relax_static_bands"+flags+" --ldau"));}
          runs.push_back(std::make_pair(proto,base+" --run_relax_static"+flags+" --noldau"));
          for(size_t r=0;r<runs.size();r++){
            if(chaos_checked && RunInChaos(runs[r].first)){
              if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << runs[r].first << " is in CHAOS" << endl;}
              nruns_in_chaos++;
              continue;
            }
            redecoration_cmds.push_back(runs[r].second);
          }
        }

        if(!commands_file.empty()){
          aurostd::string2file(aurostd::joinWDelimiter(redecoration_cmds,"\n")+(redecoration_cmds.empty()?"":"\n"),commands_file);
        }
        return redecoration_cmds;
    }
}
