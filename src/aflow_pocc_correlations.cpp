// ***************************************************************************
// *                                                                         *
// *           Aflow COREY OSES - Johns Hopkins University 2022-2026         *
// *                                                                         *
// ***************************************************************************
// aflow_pocc_correlations.cpp
// Cluster correlation functions of the POCC supercells.  //CO20261006
//
// The clusters are the pairs and triplets of the partially occupied sites of the
// parent (pairs out to DEFAULT_POCC_CORRELATION_PAIR_SHELLS distinct distances,
// triplets whose edges all lie within DEFAULT_POCC_CORRELATION_TRIPLET_SHELLS),
// grouped into orbits by the factor group of the colored parent (setColoredParent()).
// A site with M species carries the point functions of
// CALPHAD 33, 266 (2009): gamma_0=1, gamma_j(s)=-cos(2 pi ceil(j/2) s/M) for odd j
// and -sin(2 pi floor(j/2) s/M) for even j, s=0..M-1 in the order of the species
// (vacancy first, then the species of the PARTCAR). The correlation of a cluster
// orbit with functions (j_1,...,j_k) is the average of the products of the point
// functions over every cluster of the orbit in the supercell, the quantity special
// quasirandom structures match to the random alloy (A. Zunger, S.-H. Wei,
// L. G. Ferreira and J. E. Bernard, Phys. Rev. Lett. 65, 353 (1990)).
//
// Each cluster point is a site of the parent and a lattice vector, so a supercell
// with HNF matrix H finds it through the Smith normal form of H (getSupercellAction())
// without any distance tolerance. Post-processing reads H and the decoration of
// each supercell from its POSCAR title and writes, to aflow.pocc.out, the
// correlations of every supercell, of the random alloy, of the ensemble at
// infinite temperature (degeneracies) and at each temperature (probabilities),
// and the Warren-Cowley parameters of the pairs at each temperature.

#ifndef _AFLOW_POCC_CORRELATIONS_CPP_
#define _AFLOW_POCC_CORRELATIONS_CPP_

#include "aflow.h"
#include "aflow_pocc.h"
#include <set>
#include <map>

#define _DEBUG_POCC_CORRELATIONS_ false

namespace pocc {
  //--------------------------------------------------------------------------------
  // point functions
  //--------------------------------------------------------------------------------
  /// @brief point function j of a site with nvalues species, at species sigma (CALPHAD 33, 266 (2009))
  /// @authors
  /// @mod{CO,20261006,created}
  double getPointFunction(uint nvalues,uint j,uint sigma) {
    if(nvalues<2 || j>=nvalues || sigma>=nvalues){
      stringstream message;
      message << "point function out of range [nvalues=" << nvalues << ", j=" << j << ", sigma=" << sigma << "]";
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INDEX_BOUNDS_);
    }
    if(j==0){return 1.0;}
    const double arg=2.0*PI*(double)((j+1)/2)*(double)sigma/(double)nvalues;  //(j+1)/2: ceil(j/2) for odd j, floor(j/2) for even j
    return ((j%2)==1 ? -std::cos(arg) : -std::sin(arg));
  }

  //--------------------------------------------------------------------------------
  // POCC supercell titles
  //--------------------------------------------------------------------------------
  /// @brief HNF matrix and decoration of a POCC supercell from its POSCAR title (createXStructure())
  /// @param title "... HNF(n=4,2/7)=[1 0 0; 0 1 1; 0 0 4] site_config(1/6)=[0 0 1 1; 2 2 2 2] DG=4 ..."
  /// @return false if the title does not carry them
  /// @authors
  /// @mod{CO,20261006,created}
  bool parsePOccSupercellTitle(const string& title,int& n,xmatrix<double>& hnf,vector<vector<int> >& decoration) {
    n=0;hnf=xmatrix<double>(3,3);decoration.clear();
    string::size_type pos=title.rfind("HNF(n=");
    if(pos==string::npos){return false;}
    string::size_type comma=title.find(',',pos);
    if(comma==string::npos){return false;}
    const string n_str=title.substr(pos+6,comma-(pos+6));
    if(!aurostd::isfloat(n_str)){return false;}
    n=aurostd::string2utype<int>(n_str);
    string::size_type open=title.find("=[",pos),close=string::npos;
    if(open==string::npos){return false;}
    close=title.find(']',open);
    if(close==string::npos){return false;}
    vector<string> vrows,vtokens;
    aurostd::string2tokens(title.substr(open+2,close-(open+2)),vrows,";");
    if(vrows.size()!=3){return false;}
    for(uint i=0;i<3;i++){
      aurostd::string2tokens(vrows[i],vtokens," ");
      if(vtokens.size()!=3){return false;}
      for(uint j=0;j<3;j++){
        if(!aurostd::isfloat(vtokens[j])){return false;}
        hnf[hnf.lrows+i][hnf.lcols+j]=aurostd::string2utype<double>(vtokens[j]);
      }
    }
    pos=title.find("site_config(",close);
    if(pos==string::npos){return false;}
    open=title.find("=[",pos);
    if(open==string::npos){return false;}
    close=title.find(']',open);
    if(close==string::npos){return false;}
    aurostd::string2tokens(title.substr(open+2,close-(open+2)),vrows,";");
    if(vrows.empty()){return false;}
    for(uint i=0;i<vrows.size();i++){
      aurostd::string2tokens(vrows[i],vtokens," ");
      decoration.push_back(vector<int>(0));
      for(uint j=0;j<vtokens.size();j++){
        if(!aurostd::isfloat(vtokens[j])){decoration.clear();return false;}
        decoration.back().push_back(aurostd::string2utype<int>(vtokens[j]));
      }
      if(decoration.back().size()!=(uint)n){decoration.clear();return false;}
    }
    return n>0;
  }

  //--------------------------------------------------------------------------------
  // clusters
  //--------------------------------------------------------------------------------
  namespace {
    /// @brief the same cluster up to a lattice translation: each point moved to the origin in turn, the points sorted, the smallest kept
    vector<POccClusterPoint> canonicalCluster(const vector<POccClusterPoint>& points) {
      vector<POccClusterPoint> best,shifted;
      for(uint i=0;i<points.size();i++){
        shifted=points;
        for(uint m=0;m<shifted.size();m++){for(uint k=1;k<4;k++){shifted[m][k]-=points[i][k];}}
        std::sort(shifted.begin(),shifted.end());
        if(best.empty() || shifted<best){best=shifted;}
      }
      return best;
    }
  }

  /// @brief image of a cluster point (site, lattice vector) under factor group operation iop of the colored parent
  /// @authors
  /// @mod{CO,20261006,created}
  POccClusterPoint POccCalculator::applyFactorGroup(uint iop,const POccClusterPoint& point) const {
    const xmatrix<long long int>& Uf=m_vfgroup_Uf[iop];
    const uint site=(uint)point[0];
    const xvector<long long int>& shift=m_vfgroup_site_shift[iop][site];
    POccClusterPoint image;
    image[0]=(long long int)m_vfgroup_site_map[iop][site];
    for(int i=0;i<3;i++){
      image[i+1]=shift[shift.lrows+i];
      for(int j=0;j<3;j++){image[i+1]+=Uf[Uf.lrows+i][Uf.lcols+j]*point[j+1];}
    }
    return image;
  }

  /// @brief Cartesian position of a cluster point
  xvector<double> POccCalculator::getClusterPointPosition(const POccClusterPoint& point) const {
    const _atom& atom=xstr_nopocc.atoms[(uint)point[0]];
    xvector<double> fpos=atom.fpos;
    for(int i=0;i<3;i++){fpos[fpos.lrows+i]+=(double)point[i+1];}
    return xstr_nopocc.f2c*fpos;
  }

  /// @brief species of each partially occupied site (concentrations from the structure configurations, each weighted by its decorations) and the pair and triplet orbits of those sites
  /// @param pair_shells pairs out to this many distinct distances (0: DEFAULT_POCC_CORRELATION_PAIR_SHELLS)
  /// @param triplet_shells triplets whose three edges lie within this many distinct distances (DEFAULT_POCC_CORRELATION_TRIPLET_SHELLS if pair_shells is 0)
  /// @authors
  /// @mod{CO,20261006,created}
  void POccCalculator::setCorrelationClusters(uint pair_shells,uint triplet_shells) {
    bool LDEBUG=(FALSE || _DEBUG_POCC_CORRELATIONS_ || XHOST.DEBUG);
    stringstream message;
    if(pair_shells==0){
      pair_shells=(uint)std::max(0,DEFAULT_POCC_CORRELATION_PAIR_SHELLS);
      triplet_shells=(uint)std::max(0,DEFAULT_POCC_CORRELATION_TRIPLET_SHELLS);
    }
    m_corr_orbits.clear();
    m_corr_site_values.clear();
    m_corr_site_conc.clear();
    if(n_hnf<1){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"run calculateHNF() first",_RUNTIME_ERROR_);}
    if(m_rank_counts.empty()){setDecorationRanking();}
    if(m_vfgroup_Uf.empty()){setColoredParent();}
    const uint nsites=xstr_nopocc.atoms.size();

    //species of each site and their concentrations
    vector<double> wconfig(m_rank_counts.size(),0.0);
    double wtotal=0.0;
    for(uint c=0;c<m_rank_counts.size();c++){
      wconfig[c]=(double)((c+1<m_rank_offset.size() ? m_rank_offset[c+1] : m_rank_total)-m_rank_offset[c]);
      wtotal+=wconfig[c];
    }
    if(wtotal<=0.0){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"no decorations",_RUNTIME_ERROR_);}
    m_corr_site_values.assign(nsites,vector<int>(0));
    m_corr_site_conc.assign(nsites,vector<double>(0));
    vector<uint> vmixing;
    for(uint site=0;site<nsites;site++){
      vector<double> conc(m_rank_counts[0][site].size(),0.0);
      for(uint c=0;c<m_rank_counts.size();c++){
        for(uint v=0;v<m_rank_counts[c][site].size();v++){conc[v]+=wconfig[c]*(double)m_rank_counts[c][site][v]/(double)n_hnf;}
      }
      vector<int> values;
      vector<double> vconc;
      for(uint v=0;v<conc.size();v++){
        if(conc[v]>0.0){values.push_back((int)v-1);vconc.push_back(conc[v]/wtotal);}  //value v-1: vacancy (-1) first
      }
      if(values.size()<2){continue;}
      m_corr_site_values[site]=values;
      m_corr_site_conc[site]=vconc;
      vmixing.push_back(site);
    }
    if(vmixing.empty() || pair_shells==0){return;}

    //distinct distances between partially occupied sites, from a box of lattice vectors that holds every one of them
    const double tol=1e-4;  //Angstrom
    const uint nshells=std::max(pair_shells,triplet_shells);
    vector<double> vrownorm(3,0.0);
    for(int i=0;i<3;i++){
      for(int j=0;j<3;j++){vrownorm[i]+=std::pow(xstr_nopocc.c2f[xstr_nopocc.c2f.lrows+i][xstr_nopocc.c2f.lcols+j],2.0);}
      vrownorm[i]=std::sqrt(vrownorm[i]);
    }
    vector<double> vdist;
    long long int box=2;
    POccClusterPoint origin,other;
    while(true){
      vdist.clear();
      const double radius=(double)(box-1)/std::max(vrownorm[0],std::max(vrownorm[1],vrownorm[2]));  //every point within it has fractional coordinates inside the box
      for(uint i=0;i<vmixing.size();i++){
        origin={(long long int)vmixing[i],0,0,0};
        const xvector<double> r0=getClusterPointPosition(origin);
        for(uint i2=0;i2<vmixing.size();i2++){
          for(long long int x=-box;x<=box;x++){
            for(long long int y=-box;y<=box;y++){
              for(long long int z=-box;z<=box;z++){
                other={(long long int)vmixing[i2],x,y,z};
                const double d=aurostd::modulus(getClusterPointPosition(other)-r0);
                if(d<tol || d>radius){continue;}
                vdist.push_back(d);
              }
            }
          }
        }
      }
      std::sort(vdist.begin(),vdist.end());
      vector<double> vshell;
      for(uint i=0;i<vdist.size();i++){if(vshell.empty() || vdist[i]-vshell.back()>tol){vshell.push_back(vdist[i]);}}
      if(vshell.size()>=nshells){vdist=vshell;break;}
      box++;
      if(box>12){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"cannot find the requested distance shells",_RUNTIME_ERROR_);}
    }
    vdist.resize(nshells);
    const double rpair=vdist[pair_shells-1]+tol;
    const double rtriplet=(triplet_shells>0 ? vdist[triplet_shells-1]+tol : 0.0);
    auto shellOf=[&](double d) -> uint {for(uint s=0;s<vdist.size();s++){if(std::abs(d-vdist[s])<=tol){return s;}} return AUROSTD_MAX_UINT;};

    //clusters around each partially occupied site of the cell, grouped into orbits
    std::map<vector<POccClusterPoint>,uint> orbit_of;
    vector<POccClusterOrbit> vorbits;
    auto addCluster=[&](const vector<POccClusterPoint>& points) {
      const vector<POccClusterPoint> key=canonicalCluster(points);
      if(orbit_of.count(key)){return;}
      POccClusterOrbit orbit;
      orbit.points=key;
      const uint iorbit=vorbits.size();
      vector<POccClusterPoint> image(key.size());
      for(uint iop=0;iop<m_vfgroup_Uf.size();iop++){
        for(uint m=0;m<key.size();m++){image[m]=applyFactorGroup(iop,key[m]);}
        orbit.images.push_back(image);
        const vector<POccClusterPoint> key2=canonicalCluster(image);
        if(!orbit_of.count(key2)){orbit_of[key2]=iorbit;orbit.multiplicity++;}
        //operations that map the cluster onto itself permute its points
        if(key2==key){
          for(uint i=0;i<image.size();i++){
            vector<POccClusterPoint> shifted=image;
            for(uint m=0;m<shifted.size();m++){for(uint k=1;k<4;k++){shifted[m][k]-=image[i][k]-key[0][k];}}
            vector<POccClusterPoint> sorted_points=shifted;
            std::sort(sorted_points.begin(),sorted_points.end());
            if(sorted_points!=key){continue;}
            vector<uint> perm(key.size());
            for(uint m=0;m<shifted.size();m++){perm[m]=(uint)(std::find(key.begin(),key.end(),shifted[m])-key.begin());}
            if(std::find(orbit.vperm.begin(),orbit.vperm.end(),perm)==orbit.vperm.end()){orbit.vperm.push_back(perm);}
            break;
          }
        }
      }
      if(orbit.vperm.empty()){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"the identity does not fix a cluster",_RUNTIME_ERROR_);}
      //edges
      for(uint a=0;a<key.size();a++){
        for(uint b=a+1;b<key.size();b++){
          const double d=aurostd::modulus(getClusterPointPosition(key[b])-getClusterPointPosition(key[a]));
          orbit.size=std::max(orbit.size,d);
          orbit.perimeter+=d;
          orbit.shell=std::max(orbit.shell,shellOf(d)+1);
        }
      }
      vorbits.push_back(orbit);
    };
    long long int nbox[3];  //lattice vectors that reach every neighbor within the cutoff
    for(int i=0;i<3;i++){nbox[i]=(long long int)std::ceil(std::max(rpair,rtriplet)*vrownorm[i])+1;}
    for(uint i=0;i<vmixing.size();i++){
      origin={(long long int)vmixing[i],0,0,0};
      const xvector<double> r0=getClusterPointPosition(origin);
      vector<POccClusterPoint> vneighbors;
      for(uint i2=0;i2<vmixing.size();i2++){
        for(long long int x=-nbox[0];x<=nbox[0];x++){
          for(long long int y=-nbox[1];y<=nbox[1];y++){
            for(long long int z=-nbox[2];z<=nbox[2];z++){
              other={(long long int)vmixing[i2],x,y,z};
              const double d=aurostd::modulus(getClusterPointPosition(other)-r0);
              if(d<tol || d>std::max(rpair,rtriplet)){continue;}
              vneighbors.push_back(other);
              if(d<=rpair){addCluster({origin,other});}
            }
          }
        }
      }
      if(triplet_shells==0){continue;}
      for(uint a=0;a<vneighbors.size();a++){
        const xvector<double> ra=getClusterPointPosition(vneighbors[a]);
        if(aurostd::modulus(ra-r0)>rtriplet){continue;}
        for(uint b=a+1;b<vneighbors.size();b++){
          const xvector<double> rb=getClusterPointPosition(vneighbors[b]);
          if(aurostd::modulus(rb-r0)>rtriplet || aurostd::modulus(rb-ra)>rtriplet){continue;}
          addCluster({origin,vneighbors[a],vneighbors[b]});
        }
      }
    }

    //order: pairs, then triplets, by size; names p1, p2, ..., t1, t2, ...
    std::sort(vorbits.begin(),vorbits.end(),[](const POccClusterOrbit& a,const POccClusterOrbit& b){
        if(a.points.size()!=b.points.size()){return a.points.size()<b.points.size();}
        if(std::abs(a.size-b.size)>1e-4){return a.size<b.size;}
        if(std::abs(a.perimeter-b.perimeter)>1e-4){return a.perimeter<b.perimeter;}
        return a.points<b.points;
        });
    uint npairs=0,ntriplets=0;
    for(uint io=0;io<vorbits.size();io++){
      POccClusterOrbit& orbit=vorbits[io];
      orbit.name=(orbit.points.size()==2 ? "p"+aurostd::utype2string(++npairs) : "t"+aurostd::utype2string(++ntriplets));
      //point functions: every tuple (j_1..j_k), j>=1, kept once per permutation of the points by the operations that fix the cluster
      vector<uint> nvalues(orbit.points.size());
      for(uint m=0;m<orbit.points.size();m++){nvalues[m]=m_corr_site_values[(uint)orbit.points[m][0]].size();}
      vector<uint> labels(orbit.points.size(),1),permuted(orbit.points.size());
      while(true){
        bool canonical=true;
        for(uint p=0;p<orbit.vperm.size()&&canonical;p++){
          for(uint m=0;m<labels.size();m++){permuted[orbit.vperm[p][m]]=labels[m];}
          if(permuted<labels){canonical=false;}
        }
        if(canonical){orbit.labels.push_back(labels);}
        uint m=0;
        while(m<labels.size()){
          labels[m]++;
          if(labels[m]<nvalues[m]){break;}
          labels[m]=1;
          m++;
        }
        if(m==labels.size()){break;}
      }
      if(LDEBUG){cerr << __AFLOW_FUNC__ << " " << orbit.name << " size=" << orbit.size << " multiplicity=" << orbit.multiplicity << " functions=" << orbit.labels.size() << endl;}
    }
    m_corr_orbits=vorbits;
    message << "Correlation functions: " << npairs << " pair and " << ntriplets << " triplet clusters of " << vmixing.size() << " partially occupied site(s), " << getCorrelationLabels().size() << " functions";
    pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_MESSAGE_);
  }

  /// @brief names of the correlation functions, in the order getCorrelations() returns them: cluster(j_1,...,j_k)
  /// @authors
  /// @mod{CO,20261006,created}
  vector<string> POccCalculator::getCorrelationLabels() const {
    vector<string> vlabels;
    for(uint io=0;io<m_corr_orbits.size();io++){
      for(uint l=0;l<m_corr_orbits[io].labels.size();l++){vlabels.push_back(m_corr_orbits[io].name+"("+aurostd::joinWDelimiter(m_corr_orbits[io].labels[l],",")+")");}
    }
    return vlabels;
  }

  /// @brief correlations of the random alloy at the concentrations of the sites: products of the averaged point functions
  /// @authors
  /// @mod{CO,20261006,created}
  vector<double> POccCalculator::getRandomCorrelations() const {
    vector<double> vcorr;
    for(uint io=0;io<m_corr_orbits.size();io++){
      const POccClusterOrbit& orbit=m_corr_orbits[io];
      for(uint l=0;l<orbit.labels.size();l++){
        double product=1.0;
        for(uint m=0;m<orbit.points.size();m++){
          const uint site=(uint)orbit.points[m][0];
          const uint nvalues=m_corr_site_values[site].size();
          double average=0.0;
          for(uint s=0;s<nvalues;s++){average+=m_corr_site_conc[site][s]*getPointFunction(nvalues,orbit.labels[l][m],s);}
          product*=average;
        }
        vcorr.push_back(product);
      }
    }
    return vcorr;
  }

  /// @brief correlation functions of one supercell, and for each pair orbit the probability of each species pair (index s*M2+t)
  /// @param action translations and slots of the supercell (getSupercellAction())
  /// @param decoration species of each slot (v_types_config: site, then its n slots)
  /// @authors
  /// @mod{CO,20261006,created}
  vector<double> POccCalculator::getCorrelations(const POccSupercellAction& action,const vector<vector<int> >& decoration,vector<vector<double> >& vpair_probabilities) const {
    stringstream message;
    vpair_probabilities.clear();
    const uint n=(uint)n_hnf;
    const uint nsites=xstr_nopocc.atoms.size();
    if(decoration.size()!=nsites){
      message << "decoration has " << decoration.size() << " sites, the parent " << nsites;
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INDEX_MISMATCH_);
    }
    //species index of every slot
    vector<int> slot_sigma(nsites*n,-1);
    for(uint site=0;site<nsites;site++){
      if(decoration[site].size()!=n){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"decoration and n_hnf disagree",_INDEX_MISMATCH_);}
      if(m_corr_site_values[site].empty()){continue;}
      for(uint i=0;i<n;i++){
        const vector<int>& values=m_corr_site_values[site];
        const vector<int>::const_iterator it=std::find(values.begin(),values.end(),decoration[site][i]);
        if(it==values.end()){
          message << "species " << decoration[site][i] << " is not one of site " << site;
          throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_VALUE_ILLEGAL_);
        }
        slot_sigma[site*n+i]=(int)(it-values.begin());
      }
    }
    //translations: lattice vectors of the n group elements
    vector<xvector<long long int> > vy(n);
    for(uint t=0;t<n;t++){vy[t]=action.vx[t];}
    vector<double> vcorr;
    xvector<long long int> x(3);
    for(uint io=0;io<m_corr_orbits.size();io++){
      const POccClusterOrbit& orbit=m_corr_orbits[io];
      const uint k=orbit.points.size();
      vector<uint> nvalues(k);
      vector<vector<vector<double> > > gamma(k);  //point m: function j at species s
      for(uint m=0;m<k;m++){
        nvalues[m]=m_corr_site_values[(uint)orbit.points[m][0]].size();
        gamma[m].assign(nvalues[m],vector<double>(nvalues[m]));
        for(uint j=0;j<nvalues[m];j++){for(uint s=0;s<nvalues[m];s++){gamma[m][j][s]=getPointFunction(nvalues[m],j,s);}}
      }
      vector<double> vsum(orbit.labels.size(),0.0);
      vector<double> vprob;
      if(k==2){vprob.assign(nvalues[0]*nvalues[1],0.0);}
      vector<uint> sigma(k);
      for(uint ii=0;ii<orbit.images.size();ii++){
        const vector<POccClusterPoint>& image=orbit.images[ii];
        for(uint t=0;t<n;t++){
          for(uint m=0;m<k;m++){
            for(int i=0;i<3;i++){x[x.lrows+i]=image[m][i+1]+vy[t][vy[t].lrows+i];}
            const uint slot=action.slot_of[(uint)image[m][0]][action.gindex(x)];
            if(slot_sigma[slot]<0){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"cluster point on a site that does not mix",_RUNTIME_ERROR_);}
            sigma[m]=(uint)slot_sigma[slot];
          }
          for(uint l=0;l<orbit.labels.size();l++){
            double product=1.0;
            for(uint m=0;m<k;m++){product*=gamma[m][orbit.labels[l][m]][sigma[m]];}
            vsum[l]+=product;
          }
          if(k==2){vprob[sigma[0]*nvalues[1]+sigma[1]]+=1.0;}
        }
      }
      const double norm=(double)(orbit.images.size()*n);
      for(uint l=0;l<vsum.size();l++){vcorr.push_back(vsum[l]/norm);}
      if(k==2){
        for(uint i=0;i<vprob.size();i++){vprob[i]/=norm;}
        vpair_probabilities.push_back(vprob);
      }
    }
    return vcorr;
  }

  /// @brief post-processing: correlation functions of every supercell from its POSCAR title (HNF matrix and decoration)
  /// @return false, with a warning, when the titles or the parent do not allow it; post-processing goes on without correlations
  /// @authors
  /// @mod{CO,20261006,created}
  bool POccCalculator::calculateSupercellCorrelations() {
    stringstream message;
    m_vsupercell_corr.clear();
    m_vsupercell_pair_probabilities.clear();
    m_corr_orbits.clear();
    try{
      if(m_vsupercell_titles.size()!=l_supercell_sets.size()){
        message << "Correlation functions skipped: " << m_vsupercell_titles.size() << " supercell titles for " << l_supercell_sets.size() << " supercells";
        pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_WARNING_);
        return false;
      }
      calculateHNF();  //n_hnf and the structure configurations, as at generation (post-processing does not set them)
      setCorrelationClusters();
      if(m_corr_orbits.empty()){return false;}
      int n=0;
      xmatrix<double> hnf;
      vector<vector<int> > decoration;
      vector<vector<double> > vprob;
      POccSupercellAction action;
      for(uint i=0;i<m_vsupercell_titles.size();i++){
        if(!parsePOccSupercellTitle(m_vsupercell_titles[i],n,hnf,decoration) || n!=n_hnf){
          message << "Correlation functions skipped: supercell " << i+1 << " has no HNF matrix and decoration for n=" << n_hnf << " in its title";
          pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_WARNING_);
          m_vsupercell_corr.clear();m_vsupercell_pair_probabilities.clear();
          return false;
        }
        getSupercellAction(hnf,action);
        m_vsupercell_corr.push_back(getCorrelations(action,decoration,vprob));
        m_vsupercell_pair_probabilities.push_back(vprob);
      }
    }
    catch(aurostd::xerror& err){
      message << "Correlation functions skipped: " << err.what();
      pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_WARNING_);
      m_vsupercell_corr.clear();m_vsupercell_pair_probabilities.clear();
      return false;
    }
    catch(std::exception& err){  //lib2raw goes on without the correlations
      message << "Correlation functions skipped: " << err.what();
      pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_WARNING_);
      m_vsupercell_corr.clear();m_vsupercell_pair_probabilities.clear();
      return false;
    }
    return true;
  }

  namespace {
    string correlationString(double value) {
      if(!std::isfinite(value)){return "nan";}
      if(std::abs(value)<5e-11){value=0.0;}
      stringstream ss;
      ss << std::fixed << std::setprecision(10) << value;
      return ss.str();
    }
    string correlationVectorString(const vector<double>& v) {
      vector<string> vs;
      for(uint i=0;i<v.size();i++){vs.push_back(correlationString(v[i]));}
      return aurostd::joinWDelimiter(vs,",");
    }
    double meanAbsoluteDeviation(const vector<double>& a,const vector<double>& b) {
      if(a.empty() || a.size()!=b.size()){return 0.0;}
      double sum=0.0;
      for(uint i=0;i<a.size();i++){sum+=std::abs(a[i]-b[i]);}
      return sum/(double)a.size();
    }
  }

  /// @brief species name of a decoration value (vacancy "Va")
  string POccCalculator::getCorrelationSpeciesName(int value) const {
    if(value<0){return "Va";}
    if((uint)value<xstr_pocc.species.size()){return KBIN::VASP_PseudoPotential_CleanName(xstr_pocc.species[value]);}
    return aurostd::utype2string(value);
  }

  /// @brief species pairs (s,t) of the Warren-Cowley parameters of a pair cluster
  /// @return true when the two points lie on one site or on sites that the factor group of the colored parent exchanges:
  /// then the parameters are symmetric and only s<=t is listed; otherwise s is on the first site of the cluster and t on the second
  /// @authors
  /// @mod{CO,20261006,created}
  bool POccCalculator::getWarrenCowleyPairs(const POccClusterOrbit& orbit,vector<std::pair<uint,uint> >& vst) const {
    vst.clear();
    if(orbit.points.size()!=2){return false;}
    const uint site0=(uint)orbit.points[0][0],site1=(uint)orbit.points[1][0];
    bool symmetric=(site0==site1);
    for(uint iop=0;iop<m_vfgroup_site_map.size()&&!symmetric;iop++){if(site0<m_vfgroup_site_map[iop].size() && m_vfgroup_site_map[iop][site0]==site1){symmetric=true;}}
    symmetric=symmetric && (m_corr_site_values[site0]==m_corr_site_values[site1]);
    for(uint s=0;s<m_corr_site_values[site0].size();s++){
      for(uint t=(symmetric ? s : 0);t<m_corr_site_values[site1].size();t++){vst.push_back(std::make_pair(s,t));}
    }
    return symmetric;
  }

  /// @brief names of the Warren-Cowley parameters, in the order of getWarrenCowley(): p1(Mg-Ni), ...
  vector<string> POccCalculator::getWarrenCowleyLabels() const {
    vector<string> vlabels;
    vector<std::pair<uint,uint> > vst;
    for(uint io=0;io<m_corr_orbits.size();io++){
      const POccClusterOrbit& orbit=m_corr_orbits[io];
      if(orbit.points.size()!=2){continue;}
      getWarrenCowleyPairs(orbit,vst);
      const uint site0=(uint)orbit.points[0][0],site1=(uint)orbit.points[1][0];
      for(uint i=0;i<vst.size();i++){
        vlabels.push_back(orbit.name+"("+getCorrelationSpeciesName(m_corr_site_values[site0][vst[i].first])+"-"+getCorrelationSpeciesName(m_corr_site_values[site1][vst[i].second])+")");
      }
    }
    return vlabels;
  }

  /// @brief Warren-Cowley parameters alpha_st=1-P(s,t)/(c_s c_t) of every pair cluster, from the pair probabilities of getCorrelations() or their average over supercells
  /// @note c_s and c_t are the marginals of P, so a supercell is measured against its own composition and an ensemble against its average composition;
  /// a species missing from both points gives nan
  /// @authors
  /// @mod{CO,20261006,created}
  vector<double> POccCalculator::getWarrenCowley(const vector<vector<double> >& vpair_probabilities) const {
    vector<double> valpha;
    vector<std::pair<uint,uint> > vst;
    uint ip=0;
    for(uint io=0;io<m_corr_orbits.size();io++){
      const POccClusterOrbit& orbit=m_corr_orbits[io];
      if(orbit.points.size()!=2){continue;}
      if(ip>=vpair_probabilities.size()){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"fewer pair probabilities than pair clusters",_INDEX_MISMATCH_);}
      const vector<double>& P=vpair_probabilities[ip++];
      const bool symmetric=getWarrenCowleyPairs(orbit,vst);
      const uint M0=m_corr_site_values[(uint)orbit.points[0][0]].size(),M1=m_corr_site_values[(uint)orbit.points[1][0]].size();
      if(P.size()!=M0*M1){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"pair probabilities do not match the species of "+orbit.name,_INDEX_MISMATCH_);}
      vector<double> c0(M0,0.0),c1(M1,0.0);  //marginals: first and second point
      for(uint s=0;s<M0;s++){for(uint t=0;t<M1;t++){c0[s]+=P[s*M1+t];c1[t]+=P[s*M1+t];}}
      if(symmetric){for(uint s=0;s<M0;s++){c0[s]=c1[s]=0.5*(c0[s]+c1[s]);}}
      for(uint i=0;i<vst.size();i++){
        const uint s=vst[i].first,t=vst[i].second;
        const double Pst=(symmetric ? 0.5*(P[s*M1+t]+P[t*M1+s]) : P[s*M1+t]);
        const double cc=c0[s]*c1[t];
        valpha.push_back(cc>_ZERO_TOL_ ? 1.0-Pst/cc : std::numeric_limits<double>::quiet_NaN());
      }
    }
    return valpha;
  }

  /// @brief aflow.pocc.out lines of the correlation functions and Warren-Cowley parameters: temperature-independent (ensemble=false) or at the probabilities of the current temperature
  /// @authors
  /// @mod{CO,20261006,created}
  void POccCalculator::writeCorrelations(stringstream& pocc_out_ss,bool ensemble) const {
    if(m_corr_orbits.empty() || m_vsupercell_corr.size()!=l_supercell_sets.size() || m_vsupercell_pair_probabilities.size()!=l_supercell_sets.size() || l_supercell_sets.empty()){return;}
    const vector<double> vrandom=getRandomCorrelations();
    const uint nfunctions=vrandom.size();
    const vector<string> vwc_labels=getWarrenCowleyLabels();
    const bool wc=!vwc_labels.empty();
    //weights: degeneracies (infinite temperature) or the probabilities of this temperature
    vector<double> vweight;
    for(std::list<POccSuperCellSet>::const_iterator it=l_supercell_sets.begin();it!=l_supercell_sets.end();++it){
      const double w=(ensemble ? (*it).m_probability : (double)(*it).getDegeneracy());
      vweight.push_back(std::isfinite(w) ? w : 0.0);
    }
    vector<double> vavg(nfunctions,0.0);
    vector<vector<double> > vprob_avg;
    for(uint ip=0;ip<m_vsupercell_pair_probabilities.front().size();ip++){vprob_avg.push_back(vector<double>(m_vsupercell_pair_probabilities.front()[ip].size(),0.0));}
    double wtotal=0.0;
    for(uint isc=0;isc<vweight.size();isc++){
      for(uint f=0;f<nfunctions;f++){vavg[f]+=vweight[isc]*m_vsupercell_corr[isc][f];}
      for(uint ip=0;ip<vprob_avg.size();ip++){for(uint i=0;i<vprob_avg[ip].size();i++){vprob_avg[ip][i]+=vweight[isc]*m_vsupercell_pair_probabilities[isc][ip][i];}}
      wtotal+=vweight[isc];
    }
    if(wtotal<=0.0){return;}
    for(uint f=0;f<nfunctions;f++){vavg[f]/=wtotal;}
    for(uint ip=0;ip<vprob_avg.size();ip++){for(uint i=0;i<vprob_avg[ip].size();i++){vprob_avg[ip][i]/=wtotal;}}
    if(ensemble){
      pocc_out_ss << "correlation_ensemble=" << correlationVectorString(vavg) << "  (probability-weighted)" << endl;
      pocc_out_ss << "correlation_deviation_ensemble=" << correlationString(meanAbsoluteDeviation(vavg,vrandom)) << endl;
      if(wc){pocc_out_ss << "warren_cowley_ensemble=" << correlationVectorString(getWarrenCowley(vprob_avg)) << "  (probability-weighted)" << endl;}
      return;
    }
    for(uint site=0;site<m_corr_site_values.size();site++){
      if(m_corr_site_values[site].empty()){continue;}
      vector<string> vnames;
      for(uint s=0;s<m_corr_site_values[site].size();s++){vnames.push_back(getCorrelationSpeciesName(m_corr_site_values[site][s])+":"+correlationString(m_corr_site_conc[site][s]));}
      pocc_out_ss << "correlation_site_" << site+1 << "=" << aurostd::joinWDelimiter(vnames,",") << "  (species in point-function order: concentration)" << endl;
    }
    for(uint io=0;io<m_corr_orbits.size();io++){
      const POccClusterOrbit& orbit=m_corr_orbits[io];
      vector<uint> vsites;
      for(uint m=0;m<orbit.points.size();m++){vsites.push_back((uint)orbit.points[m][0]+1);}
      pocc_out_ss << "correlation_cluster_" << orbit.name << "=" << (orbit.points.size()==2 ? "pair" : "triplet");
      pocc_out_ss << " sites=" << aurostd::joinWDelimiter(vsites,",") << " shell=" << orbit.shell;
      pocc_out_ss << " size=" << std::fixed << std::setprecision(6) << orbit.size << " multiplicity=" << orbit.multiplicity << "  (Angstrom; clusters per parent cell)" << endl;
      pocc_out_ss.unsetf(std::ios_base::floatfield);
    }
    pocc_out_ss << "correlation_functions=" << aurostd::joinWDelimiter(getCorrelationLabels(),",") << endl;
    pocc_out_ss << "correlation_random=" << correlationVectorString(vrandom) << endl;
    if(wc){pocc_out_ss << "warren_cowley_pairs=" << aurostd::joinWDelimiter(vwc_labels,",") << "  (1-P(s,t)/(c_s c_t); zero in the random alloy)" << endl;}
    for(uint isc=0;isc<vweight.size();isc++){
      const string index=aurostd::PaddedNumString(isc+1,aurostd::getZeroPadding(l_supercell_sets.size()));
      const string arun=(isc<m_ARUN_directories.size() ? "  ["+m_ARUN_directories[isc]+"]" : "");
      pocc_out_ss << "correlation_supercell_" << index << "=" << correlationVectorString(m_vsupercell_corr[isc]) << arun << endl;
      pocc_out_ss << "correlation_deviation_supercell_" << index << "=" << correlationString(meanAbsoluteDeviation(m_vsupercell_corr[isc],vrandom)) << arun << endl;
      if(wc){pocc_out_ss << "warren_cowley_supercell_" << index << "=" << correlationVectorString(getWarrenCowley(m_vsupercell_pair_probabilities[isc])) << arun << endl;}
    }
    pocc_out_ss << "correlation_infinite_temperature=" << correlationVectorString(vavg) << "  (degeneracy-weighted)" << endl;
    pocc_out_ss << "correlation_deviation_infinite_temperature=" << correlationString(meanAbsoluteDeviation(vavg,vrandom)) << endl;
    if(wc){pocc_out_ss << "warren_cowley_infinite_temperature=" << correlationVectorString(getWarrenCowley(vprob_avg)) << "  (degeneracy-weighted)" << endl;}
  }
} // namespace pocc

#endif  // _AFLOW_POCC_CORRELATIONS_CPP_

// ***************************************************************************
// *                                                                         *
// *           Aflow COREY OSES - Johns Hopkins University 2022-2026         *
// *                                                                         *
// ***************************************************************************
