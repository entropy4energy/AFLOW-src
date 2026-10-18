// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2023           *
// *                                                                         *
// ***************************************************************************

#define __XLIBS_LINK
#include "aflow.h"
#define cdebug cerr
#include <algorithm>

//bool nocase_compare(char c1,char c2) {return toupper(c1)==toupper(c2);}

//#define MaxAflowInSize 65535
//string AflowIn; //[MaxAflowInSize];

#define VRUNS_MAX_CUTOFF 5000 //[CO20230307 - wastes too much time in the beginning]32768
#define DUKE_BETANEW_DEFAULT_KILL_MEM_CUTOFF 1.50
#define DUKE_QRATS_DEFAULT_KILL_MEM_CUTOFF 1.50
#define DUKE_QFLOW_DEFAULT_KILL_MEM_CUTOFF 1.50
#define DUKE_X_X_DEFAULT_KILL_MEM_CUTOFF 1.50 //CO20201220
#define DUKE_X_CRAY_DEFAULT_KILL_MEM_CUTOFF 1.50 //SD20221006
#define DUKE_X_OLDCRAY_DEFAULT_KILL_MEM_CUTOFF 1.50 //SD20221006
#define DUKE_X_SMB_DEFAULT_KILL_MEM_CUTOFF 1.50 //SD20221006
#define S4E_WORKSTATION_DEFAULT_KILL_MEM_CUTOFF 1.50 //CO20230825
#define MACHINE103_DEFAULT_KILL_MEM_CUTOFF 1.50 //CO20220818
#define MPCDF_EOS_DEFAULT_KILL_MEM_CUTOFF 1.50
#define MPCDF_DRACO_DEFAULT_KILL_MEM_CUTOFF 1.50
#define MPCDF_COBRA_DEFAULT_KILL_MEM_CUTOFF 1.50
#define MPCDF_HYDRA_DEFAULT_KILL_MEM_CUTOFF 1.50
#define MACHINE001_DEFAULT_KILL_MEM_CUTOFF 1.50  //DX20190509 - MACHINE001
#define MACHINE002_DEFAULT_KILL_MEM_CUTOFF 1.50  //DX20190509 - MACHINE002
#define MACHINE003_DEFAULT_KILL_MEM_CUTOFF 1.50  //DX20201005 - MACHINE003
#define MACHINE004_DEFAULT_KILL_MEM_CUTOFF 1.50  //DX20211011 - MACHINE004
#define MACHINE101_DEFAULT_KILL_MEM_CUTOFF 1.50  //CO20230512 - MACHINE101
#define MACHINE102_DEFAULT_KILL_MEM_CUTOFF 1.50  //CO20230512 - MACHINE102
#define CMU_EULER_DEFAULT_KILL_MEM_CUTOFF 1.50   //DX20190107 - CMU EULER

namespace aurostd {
  // ***************************************************************************
  // Function DirectoryAlreadyInDabatase
  // ***************************************************************************
  //CO20261004 - CHAOS is the database: 0 not in CHAOS (or not a database run, or the check is
  //off), 1 in CHAOS (marked ALREADY_IN_DATABASE, a copy of aflow.in), 2 CHAOS cannot be checked
  //CO20261005 - only --no_chaos_check turns the check off; under --force (every worker line has
  //it) an ALREADY_IN_DATABASE marker already in the folder is not trusted and the run is decided
  //from the list again (markers of earlier aflow versions came from the legacy upstream lists)
  int DirectoryInChaos(const string& directory,bool FORCE,string& reason) {
    reason="";
    if(!FORCE && (aurostd::FileExist(directory+"/ALREADY_IN_DATABASE") || aurostd::EFileExist(directory+"/ALREADY_IN_DATABASE"))) {reason="ALREADY_IN_DATABASE";return 1;}
    if(aflowlib::ChaosCheckDisabled() || !aflowlib::ChaosCheckApplies(directory)) return 0;
    aflowlib::ChaosCheck check=aflowlib::ChaosCheckDirectory(directory,vector<string>(),reason);
    if(check==aflowlib::CHAOS_CHECK_IN) {
      aurostd::CopyFile(directory+"/"+_AFLOWIN_,directory+"/ALREADY_IN_DATABASE");
      ostringstream oss;
      oss << "MMMMM  In CHAOS, not started (marked ALREADY_IN_DATABASE): " << reason << " = " << directory << endl;
      aurostd::PrintMessageStream(oss,XHOST.QUIET);
      return 1;
    }
    if(check==aflowlib::CHAOS_CHECK_UNKNOWN) {
      //once per reason: a loop over many run folders does not print the per-folder message
      static std::mutex notice_mutex;
      static string notice_last="";
      std::lock_guard<std::mutex> lock(notice_mutex);
      if(reason!=notice_last) {
        notice_last=reason;
        cerr << "WWWWW  CHAOS cannot be checked, runs are not started: " << reason << " (aflow --chaos_runs_update fetches the list; --no_chaos_check starts runs anyway)" << endl;
      }
      return 2;
    }
    return 0;
  }

  bool DirectoryAlreadyInDatabase(string directory,bool FORCE) {
    //CO20261004 - CHAOS (DirectoryInChaos) instead of the legacy upstream list (vLIBS)
    string reason="";
    return (DirectoryInChaos(directory,FORCE,reason)==1);
  }

}

using aurostd::DirectorySkipped;
using aurostd::DirectoryAlreadyInDatabase;
using aurostd::DirectoryInChaos;  //CO20261004
using aurostd::DirectoryUnwritable;

// int KBIN_MODE;

//#define KBIN_VOID_MODE 0             // just a shift
//#define KBIN_VASP_MODE 2             // for ab-initio VASP mode
//#define KBIN_XXXX_MODE 3             // for XXXX program
//#define KBIN_GRND_MODE 4             // for classical monte carlo

// GND MODE
#define KBIN_VASP_N_VPARS 32
#define _KBIN_VASP_SLEEP_ 2
#define _KBIN_LOOP_SLEEP_ 300
// PRIORITY
#define PRIORITY_PROBABILITY 0.2000
//#define PRIORITY_GREP_STRING string("grep -vi xxxxx ")
#define PRIORITY_GREP_STRING string("grep -vi PRIORITY ")

// ***************************************************************************
// KBIN::Legitimate_krun
// ***************************************************************************
namespace KBIN {
  bool Legitimate_krun(const _aflags& aflags,const bool osswrite,ostringstream& oss) {
    string aflowindir=aflags.Directory,filename;
    aurostd::StringSubst(aflowindir,"//","/");
    if(!Legitimate_aflowdir(aflowindir,aflags,osswrite,oss)){ // aflowdir not legitimate
      return FALSE;
    }
    if(aflags.KBIN_GEN_AFLOWIN_FROM_VASP){
      filename=aflowindir+"/INCAR";
    }
    else if(aflags.KBIN_RUN_AFLOWIN || aflags.KBIN_GEN_VASP_FROM_AFLOWIN || aflags.KBIN_GEN_AIMS_FROM_AFLOWIN){
      filename=aflowindir+"/"+_AFLOWIN_;
    }
    else { //SD20220319 - catch-all
      return FALSE;
    }
    if(!aurostd::FileExist(filename)){ // file does not exist
      if(osswrite) {oss << "MMMMM  File does not exist = " << filename << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    return TRUE;
  }
}

namespace KBIN {
  bool Legitimate_krun(const _aflags& aflags) { ostringstream aus; return KBIN::Legitimate_krun(aflags,FALSE,aus);};
}

// ***************************************************************************
// KBIN::Legitimate_aflowin
// ***************************************************************************
namespace KBIN {
  bool Legitimate_aflowin(const string& _aflowindir,const bool osswrite,ostringstream& oss) {
    string aflowindir=_aflowindir; //SD20220321 - aflowindir = aflowdir/_AFLOWIN_
    aurostd::StringSubst(aflowindir,"//","/");
    if(!aurostd::FileExist(aflowindir)){ // file does not exist
      if(osswrite) {oss << "MMMMM  File does not exist = " << aflowindir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    if(aurostd::FileEmpty(aflowindir)){ // file is empty
      if(osswrite) {oss << "MMMMM  File is empty = " << aflowindir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    if(!aurostd::substring2bool(aflowindir,_AFLOWIN_)){ // file name does not contain _AFLOWIN_
      if(osswrite) {oss << "MMMMM  File name does not contain _AFLOWIN_ " << _AFLOWIN_ << " = " << aflowindir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    aurostd::StringSubst(aflowindir,_AFLOWIN_,"");
    if(aurostd::DirectoryLocked(aflowindir,_AFLOWLOCK_)){ // LOCK file exists
      if(osswrite) {oss << "MMMMM  Directory locked = " << aflowindir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    return TRUE;
  }
}

namespace KBIN {
  bool Legitimate_aflowin(const string aflowindir) { ostringstream aus; return KBIN::Legitimate_aflowin(aflowindir,FALSE,aus);};
}

// ***************************************************************************
// KBIN::machineName
// ***************************************************************************
//CO20261004 - the machine name AFLOW prints (LOCK file, messages). The code names no machine of ours: a run uses
//a generic template (--machine=machine103, for example) and says which machine it is with --machine_name. The
//name comes first and the template after it, e.g. "Lab-Cluster (machine template MACHINE::MACHINE103)". Without
//--machine_name, a generic template says that it is unnamed.
namespace KBIN {
  string machineName(const string& name_from_argv,const string& machine_templates) {
    const string name=aurostd::RemoveWhiteSpacesFromTheFrontAndBack(name_from_argv);
    if(name.empty()) {
      if(aurostd::substring2bool(machine_templates,"MACHINE::MACHINE")) return machine_templates+" (unnamed; set --machine_name)";
      return machine_templates;
    }
    if(machine_templates.empty()) return name;
    return name+" (machine template "+machine_templates+")";
  }
}

// ***************************************************************************
// KBIN::Legitimate_aflowdir
// ***************************************************************************
namespace KBIN {
  bool Legitimate_aflowdir(const string& aflowdir,const _aflags& aflags,const bool osswrite,ostringstream& oss) {
    if(!aurostd::IsDirectory(aflowdir)){ // directory does not exist
      if(osswrite) {oss << "MMMMM  Directory does not exist = " << aflowdir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    if(aurostd::DirectoryUnwritable(aflowdir)){ // directory unwritable
      if(osswrite) {oss << "MMMMM  Directory unwritable = " << aflowdir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    if(aurostd::DirectoryLocked(aflowdir,_AFLOWLOCK_)){ // directory locked
      if(osswrite) {oss << "MMMMM  Directory locked = " << aflowdir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    if(aurostd::DirectorySkipped(aflowdir)){ // directory skipped
      if(osswrite) {oss << "MMMMM  Directory skipped = " << aflowdir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    string chaos_reason="";  //CO20261004 - CHAOS is the database
    int in_chaos=DirectoryInChaos(aflowdir,aflags.AFLOW_FORCE_RUN,chaos_reason);
    if(in_chaos==1){ // directory already in database
      if(osswrite) {oss << "MMMMM  Directory already in database (CHAOS: " << chaos_reason << ") = " << aflowdir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    if(in_chaos==2){ // CHAOS cannot be checked: the run is not started
      if(osswrite) {oss << "MMMMM  Directory not started, CHAOS cannot be checked (" << chaos_reason << "; --no_chaos_check starts it anyway) = " << aflowdir << Message(__AFLOW_FILE__) << endl;aurostd::PrintMessageStream(oss,XHOST.QUIET);};
      return FALSE;
    }
    return TRUE;
  }
}

namespace KBIN {
  bool Legitimate_aflowdir(const string& aflowdir,const _aflags& aflags) { ostringstream aus; return KBIN::Legitimate_aflowdir(aflowdir,aflags,FALSE,aus);};
}

namespace KBIN {
  void getAflowInFromAFlags(const _aflags& aflags,string& AflowIn_file,string& AflowIn,ostream& oss) {ofstream FileMESSAGE;return getAflowInFromAFlags(aflags,AflowIn_file,AflowIn,FileMESSAGE,oss);}  //CO20191110
  void getAflowInFromAFlags(const _aflags& aflags,string& AflowIn_file,string& AflowIn,ofstream& FileMESSAGE,ostream& oss) { //CO20191110
    return getAflowInFromDirectory(aflags.Directory,AflowIn_file,AflowIn,FileMESSAGE,oss);
  }
  void getAflowInFromDirectory(const string& directory,string& AflowIn_file,string& AflowIn,ostream& oss) {ofstream FileMESSAGE;return getAflowInFromDirectory(directory,AflowIn_file,AflowIn,FileMESSAGE,oss);}  //CO20191110
  void getAflowInFromDirectory(const string& directory,string& AflowIn_file,string& AflowIn,ofstream& FileMESSAGE,ostream& oss) { //CO20191110
    AflowIn_file=aurostd::CleanFileName(directory+"/"+_AFLOWIN_); //CO20200624
    if(!aurostd::FileExist(AflowIn_file)){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"Input file does not exist: "+AflowIn_file,_INPUT_ERROR_);}
    pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,"Using input file: "+AflowIn_file,FileMESSAGE,oss,_LOGGER_MESSAGE_);
    aurostd::file2string(AflowIn_file,AflowIn);
    AflowIn=aurostd::RemoveComments(AflowIn); // NOW Clean AFLOWIN
    pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,"AflowIn.size()="+aurostd::utype2string(AflowIn.size()),FileMESSAGE,oss,_LOGGER_MESSAGE_); //CO20200624 - check size!=0
  }
}

// ***************************************************************************
// KBIN::Main
// ***************************************************************************
namespace KBIN {
  _aflags loadAflagsFromARGV(const vector<string>& argv){ //CO20240620 - loading from argv
    _aflags aflags;
    aflags.Directory=XHOST.vflag_control.getattachedscheme("DIRECTORY_CLEAN");  //CO20190629 - a good default until we get down into vDirectory

    aflags.KBIN_RUN_AFLOWIN=TRUE;
    aflags.KBIN_GEN_VASP_FROM_AFLOWIN=FALSE;
    aflags.KBIN_GEN_AFLOWIN_FROM_VASP=FALSE;
    //DX
    aflags.KBIN_GEN_SYMMETRY_OF_AFLOWIN=FALSE;
    //DX
    aflags.KBIN_DELETE_AFLOWIN=FALSE;

    //--generate could be for VASP/AIMS/etc., need to read aflow.in later
    aflags.KBIN_GEN_GENERAL                  = aurostd::args2flag(argv,"--generate");                             //CO20180402 - we will use this to modify other GEN flags later, when reading AFLOWIN
    aflags.KBIN_GEN_VASP_FROM_AFLOWIN        = aurostd::args2flag(argv,"--generate_vasp_from_aflowin|--generate");//CO20180402 - --generate assumes vasp generation FOR NOW
    //[CO20240620 - OBSOLETE]if(aflags.KBIN_GEN_AFLOWIN_FROM_VASP) {aflags.KBIN_RUN_AFLOWIN=FALSE;}
    aflags.KBIN_GEN_AIMS_FROM_AFLOWIN        = aurostd::args2flag(argv,"--generate_aims_from_aflowin");//CO20180402
    aflags.KBIN_GEN_AFLOWIN_FROM_VASP        = aurostd::args2flag(argv,"--generate_aflowin_from_vasp");
    aflags.KBIN_GEN_SYMMETRY_OF_AFLOWIN      = aurostd::args2flag(argv,"--generate_symmetry|--generate_sym"); //DX


    if(aflags.KBIN_GEN_VASP_FROM_AFLOWIN || aflags.KBIN_GEN_AFLOWIN_FROM_VASP || aflags.KBIN_GEN_SYMMETRY_OF_AFLOWIN || aflags.KBIN_GEN_AIMS_FROM_AFLOWIN){ //CO20180409
      if(aflags.KBIN_GEN_AFLOWIN_FROM_VASP) aflags.KBIN_RUN_AFLOWIN=FALSE;
    }

    aflags.KBIN_DELETE_AFLOWIN=aurostd::args2flag(argv,"--delete_aflowin");

    aflags.AFLOW_MODE_QSUB_MODE1 = aurostd::args2flag(argv,"--qsub1|-qsub1");
    aflags.AFLOW_MODE_QSUB_MODE2 = aurostd::args2flag(argv,"--qsub2|-qsub2");
    aflags.AFLOW_MODE_QSUB_MODE3 = aurostd::args2flag(argv,"--qsub3|-qsub3");

    // [OBSOLETE]  MPI=aurostd::args2flag(argv,"--MPI|--mpi");  // ABSOLUTELY otherwise the multithreads kicks
    aflags.AFLOW_FORCE_MPI = XHOST.MPI; //[CO20210315]aurostd::args2flag(argv,"--MPI|--mpi");
    aflags.AFLOW_FORCE_SERIAL = aurostd::args2flag(argv,"--nompi|-nompi|--serial|-serial");
    // [OBSOLETE]  aflags.AFLOW_GLOBAL_NCPUS=aurostd::args2utype(argv,"--np",(int) 0);
    aflags.AFLOW_GLOBAL_NCPUS=aurostd::args2attachedutype<int>(argv,"--np=",(int) 0);

    aflags.AFLOW_MACHINE_GLOBAL.clear();
    // "MACHINE::DUKE_BETA_MPICH"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_BETA_MPICH",
        aurostd::args2flag(argv,"--machine=beta|--machine=duke_beta|--beta|--duke_beta|--machine=beta_mpich|--machine=duke_beta_mpich"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_BETA_MPICH")) XHOST.maxmem=DUKE_BETANEW_DEFAULT_KILL_MEM_CUTOFF;
    // "MACHINE::DUKE_BETA_OPENMPI"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_BETA_OPENMPI",aurostd::args2flag(argv,"--machine=beta_openmpi|--machine=duke_beta_openmpi"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_BETA_OPENMPI")) XHOST.maxmem=DUKE_BETANEW_DEFAULT_KILL_MEM_CUTOFF;
    // "MACHINE::DUKE_QRATS_MPICH"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_QRATS_MPICH",aurostd::args2flag(argv,"--machine=qrats|--machine=duke_qrats|--machine=qrats_mpich|--machine=duke_qrats_mpich"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_QRATS_MPICH")) XHOST.maxmem=DUKE_QRATS_DEFAULT_KILL_MEM_CUTOFF;
    // "MACHINE::DUKE_QFLOW_OPENMPI"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_QFLOW_OPENMPI",aurostd::args2flag(argv,"--machine=qflow|--machine=duke_qflow|--machine=qflow_openmpi|--machine=duke_qflow_openmpi|--machine=quser|--machine=duke_quser|--machine=quser_openmpi|--machine=duke_quser_openmpi")); //backwards compatible //CO20180409
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_QFLOW_OPENMPI")) XHOST.maxmem=DUKE_QFLOW_DEFAULT_KILL_MEM_CUTOFF;
    //CO20201220 X START
    // "MACHINE::DUKE_X_X"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_X_X",aurostd::args2flag(argv,"--machine=x_x|--machine=duke_x_x")); //backwards compatible //CO20180409
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_X_X")) XHOST.maxmem=DUKE_X_X_DEFAULT_KILL_MEM_CUTOFF;
    // "MACHINE::DUKE_X_CRAY"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_X_CRAY",aurostd::args2flag(argv,"--machine=x_cray|--machine=duke_x_cray")); //SD20221006
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_X_CRAY")) XHOST.maxmem=DUKE_X_CRAY_DEFAULT_KILL_MEM_CUTOFF;
    // "MACHINE::DUKE_X_OLDCRAY"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_X_OLDCRAY",aurostd::args2flag(argv,"--machine=x_oldcray|--machine=duke_x_oldcray")); //SD20221006
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_X_OLDCRAY")) XHOST.maxmem=DUKE_X_OLDCRAY_DEFAULT_KILL_MEM_CUTOFF;
    // "MACHINE::DUKE_X_SMB"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_X_SMB",aurostd::args2flag(argv,"--machine=x_smb|--machine=duke_x_smb")); //SD20221006
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_X_SMB")) XHOST.maxmem=DUKE_X_SMB_DEFAULT_KILL_MEM_CUTOFF;
    //CO20201220 X STOP
    //CO20230825 START - S4E_WORKSTATION
    // "MACHINE::S4E_WORKSTATION"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::S4E_WORKSTATION",aurostd::args2flag(argv,"--machine=s4e|--machine=jhu")); //SD20221006
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::S4E_WORKSTATION")) XHOST.maxmem=S4E_WORKSTATION_DEFAULT_KILL_MEM_CUTOFF;
    //CO20230825 STOP - S4E_WORKSTATION
    //CO20261004 - MACHINE103 - START (generic template; name the machine with --machine_name)
    // "MACHINE::MACHINE103"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE103",aurostd::args2flag(argv,"--machine=machine103")); //CO20261004
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE103")) XHOST.maxmem=MACHINE103_DEFAULT_KILL_MEM_CUTOFF;
    //CO20261004 - MACHINE103 - STOP
    // "MACHINE::MPCDF_EOS"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MPCDF_EOS",aurostd::args2flag(argv,"--machine=eos|--machine=mpcdf_eos|--machine=eos_mpiifort|--machine=mpcdf_eos_mpiifort"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MPCDF_EOS")) XHOST.maxmem=MPCDF_EOS_DEFAULT_KILL_MEM_CUTOFF;
    // "MACHINE::MPCDF_DRACO"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MPCDF_DRACO",aurostd::args2flag(argv,"--machine=draco|--machine=mpcdf_draco|--machine=draco_mpiifort|--machine=mpcdf_draco_mpiifort"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MPCDF_DRACO")) XHOST.maxmem=MPCDF_DRACO_DEFAULT_KILL_MEM_CUTOFF;
    // "MACHINE::MPCDF_COBRA"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MPCDF_COBRA",aurostd::args2flag(argv,"--machine=cobra|--machine=mpcdf_cobra|--machine=cobra_mpiifort|--machine=mpcdf_cobra_mpiifort"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MPCDF_COBRA")) XHOST.maxmem=MPCDF_COBRA_DEFAULT_KILL_MEM_CUTOFF;
    // "MACHINE::MPCDF_HYDRA"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MPCDF_HYDRA",aurostd::args2flag(argv,"--machine=hydra|--machine=mpcdf_hydra|--machine=hydra_mpiifort|--machine=mpcdf_hydra_mpiifort"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MPCDF_HYDRA")) XHOST.maxmem=MPCDF_HYDRA_DEFAULT_KILL_MEM_CUTOFF;
    //DX20190509 - MACHINE001 - START
    // "MACHINE::MACHINE001"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE001",aurostd::args2flag(argv,"--machine=machine001"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE001")) XHOST.maxmem=MACHINE001_DEFAULT_KILL_MEM_CUTOFF;
    //DX20190509 - MACHINE001 - END
    //DX20190509 - MACHINE002 - START
    // "MACHINE::MACHINE002"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE002",aurostd::args2flag(argv,"--machine=machine002"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE002")) XHOST.maxmem=MACHINE002_DEFAULT_KILL_MEM_CUTOFF;
    //DX20190509 - MACHINE002 - END
    //DX20201005 - MACHINE003 - START
    // "MACHINE::MACHINE003"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE003",aurostd::args2flag(argv,"--machine=machine003"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE003")) XHOST.maxmem=MACHINE003_DEFAULT_KILL_MEM_CUTOFF;
    //DX20201005 - MACHINE003 - END
    //DX20211011 - MACHINE004 - START
    // "MACHINE::MACHINE004"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE004",aurostd::args2flag(argv,"--machine=machine004"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE004")) XHOST.maxmem=MACHINE004_DEFAULT_KILL_MEM_CUTOFF;
    //DX20211011 - MACHINE004 - END
    //CO20230512 - MACHINE101 - START
    // "MACHINE::MACHINE101"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE101",aurostd::args2flag(argv,"--machine=machine101"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE101")) XHOST.maxmem=MACHINE101_DEFAULT_KILL_MEM_CUTOFF;
    //CO20230512 - MACHINE101 - END
    //CO20230512 - MACHINE102 - START
    // "MACHINE::MACHINE102"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE102",aurostd::args2flag(argv,"--machine=machine102"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE102")) XHOST.maxmem=MACHINE102_DEFAULT_KILL_MEM_CUTOFF;
    //CO20230512 - MACHINE102 - END
    // DUKE_MATERIALS
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_MATERIALS",aurostd::args2flag(argv,"--machine=materials|--machine=duke_materials"));
    // DUKE_AFLOWLIB
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_AFLOWLIB",aurostd::args2flag(argv,"--machine=aflowlib|--machine=duke_aflowlib"));
    // DUKE_HABANA
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::DUKE_HABANA",aurostd::args2flag(argv,"--machine=habana|--machine=duke_habana"));
    // FULTON_MARYLOU
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::FULTON_MARYLOU",aurostd::args2flag(argv,"--machine=marylou|--machine=fulton_marylou"));
    // MACHINE2
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::OHAD",aurostd::args2flag(argv,"--machine=ohad|--machine=machine2")); //CO20181113
    // MACHINE1
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::HOST1",aurostd::args2flag(argv,"--machine=host1|--machine=machine1")); //CO20181113
    //DX20190107 - CMU EULER - START
    // "MACHINE::CMU_EULER"
    aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::CMU_EULER",aurostd::args2flag(argv,"--machine=euler|--machine=cmu_euler"));
    if(aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::CMU_EULER")) XHOST.maxmem=CMU_EULER_DEFAULT_KILL_MEM_CUTOFF;
    //DX20190107 - CMU EULER - END

    //HE20220309 - Machine Name - START
    // allow the use of generic machine templates while using descriptive names
    stringstream machine_templates;
    machine_templates << aflags.AFLOW_MACHINE_GLOBAL;
    string argv_machine_name = aurostd::args2attachedstring(argv, "--machine_name=", "");
    aflags.AFLOW_MACHINE_GLOBAL.addattachedscheme("NAME", KBIN::machineName(argv_machine_name, machine_templates.str()), true);  //CO20261004
    //HE20220309 - Machine Name - END
    
    aflags.AFLOW_PERFORM_CLEAN=XHOST.vflag_aflow.flag("CLEAN");// || XHOST.vflag_aflow.flag("XCLEAN"));
    // [OBSOLETE] aflags.AFLOW_PERFORM_DIRECTORY=aurostd::args2flag(argv,"--DIRECTORY|--D|--d|./");
    aflags.AFLOW_PERFORM_DIRECTORY=XHOST.vflag_control.flag("VDIR");
    // [OBSOLETE] aflags.AFLOW_PERFORM_FILE=aurostd::args2flag(argv,"--FILE|--F|--f");
    aflags.AFLOW_PERFORM_FILE=XHOST.vflag_control.flag("FILE");
    aflags.AFLOW_PERFORM_FILE_MIX=XHOST.vflag_control.flag("FILE_MIX"); //CO20260101
    aflags.AFLOW_PERFORM_ORDER_SORT=aurostd::args2flag(argv,"--sort|-sort");                    // Sorts the _AFLOWIN_ in the list
    aflags.AFLOW_PERFORM_ORDER_REVERSE=aurostd::args2flag(argv,"--reverse|--rsort|-reverse|-rsort"); // Reverse the _AFLOWIN_ in the list
    aflags.AFLOW_PERFORM_ORDER_RANDOM=aurostd::args2flag(argv,"--random|--rnd|-random|-rnd"); // Randomize the _AFLOWIN_ in the list
    aflags.AFLOW_FORCE_RUN = aurostd::args2flag(argv,"--force|-force");

    return aflags;
  }

  int KBIN_Main(const vector<string>& argv) {        // AFLOW_FUNCTION_IMPLEMENTATION
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    if (argv.empty()){return 0;} //SD2022024 - check argv!=0
    string GENERIC;
    //  string Directory;
    int i;
    ostringstream aus;
    ifstream FileAUS;

    _aflags aflags=KBIN::loadAflagsFromARGV(argv);  //CO20240620 - extracting all options from argv, a bit from XHOST but these are detangled from options set below

    aurostd::StringstreamClean(aus);
    bool _VERBOSE_=FALSE;
    int XHOST_AFLOW_RUNXnumber_multiplier=3;

    //[CO20240620 - not used]std::deque<_aflags> qaflags;

    AFLOW_PTHREADS::Clean_Threads();                                    // clean threads
    // _aflags taflags[MAX_ALLOCATABLE_PTHREADS];
    // _threaded_KBIN_params params[MAX_ALLOCATABLE_PTHREADS];

    // cerr << "GMODE" << endl;
    // check BlackList **************************************************
    if(AFLOW_BlackList(XHOST.hostname)) {
      aus << "MMMMM  HOSTNAME BLACKLISTED = " << XHOST.hostname << " - " << Message(__AFLOW_FILE__,aflags) << endl;
      aurostd::PrintMessageStream(aus,XHOST.QUIET);
      return 0;
    }
    // get KBIN **************************************************
    //  aus << "MMMMM  AFLOW: running " << _AFLOWIN_ << ": " << " "  << " - " << Message(__AFLOW_FILE__,aflags) << endl;   // too much verbosity is annoying
    //  aurostd::PrintMessageStream(aus,XHOST.QUIET);                                                  // too much verbosity is annoying

    // do some running

    //   cerr << __AFLOW_FUNC__ << " XHOST.AFLOW_RUNXflag=" << XHOST.AFLOW_RUNXflag << endl; //

    if(XHOST.AFLOW_RUNXflag) {
      vector<string> tokens;
      if(aurostd::args2attachedflag(argv,"--run=")) {
        aurostd::string2tokens(aurostd::args2attachedstring(argv,"--run=","1"),tokens,"=");
        XHOST.AFLOW_RUNXnumber=aurostd::string2utype<uint>(tokens[tokens.size()-1]);
      }
      if(aurostd::args2attachedflag(argv,"-run=")) {
        aurostd::string2tokens(aurostd::args2attachedstring(argv,"-run=","1"),tokens,"=");
        XHOST.AFLOW_RUNXnumber=aurostd::string2utype<uint>(tokens[tokens.size()-1]);
      }
      if(XHOST.AFLOW_RUNXnumber<1) XHOST.AFLOW_RUNXnumber=1;
    }

    if(aurostd::args2flag(argv,"-runone|--runone|-run_one|--run_one|-run1|--run1|--run=1|-run=1")) { // RUNONE COMPATIBILITY
      XHOST.AFLOW_RUNXflag=TRUE;XHOST.AFLOW_RUNXnumber=1;
    }

    if(XHOST.AFLOW_RUNXflag)  {
      XHOST.AFLOW_RUNDIRflag=FALSE;
      XHOST.AFLOW_MULTIflag=FALSE;
    }
    if(XHOST.AFLOW_MULTIflag)  {
      XHOST.AFLOW_RUNDIRflag=FALSE;
      XHOST.AFLOW_RUNXflag=FALSE;
    }

    //    cerr << __AFLOW_FUNC__ << " XHOST.AFLOW_RUNXflag=" << XHOST.AFLOW_RUNXflag << endl; //

    if(XHOST.vflag_aflow.flag("LOOP")) {aus << "MMMMM  KBIN option XHOST.vflag_aflow.flag(\"LOOP\") - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
    if(XHOST.AFLOW_RUNDIRflag) {aus << "MMMMM  KBIN option [--run] (XHOST.AFLOW_RUNDIRflag=TRUE) - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
    if(XHOST.AFLOW_MULTIflag) {aus << "MMMMM  KBIN option [--run=multi] (XHOST.AFLOW_MULTIflag=TRUE) - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
    if(XHOST.AFLOW_RUNXflag && XHOST.AFLOW_RUNXnumber==1) {aus << "MMMMM  KBIN option [--run=1] (XHOST.AFLOW_RUNXflag=TRUE, XHOST.AFLOW_RUNXnumber=" << XHOST.AFLOW_RUNXnumber << ") - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
    if(XHOST.AFLOW_RUNXflag && XHOST.AFLOW_RUNXnumber>1) {aus << "MMMMM  KBIN option [--run=N] (XHOST.AFLOW_RUNXflag=TRUE, XHOST.AFLOW_RUNXnumber=" << XHOST.AFLOW_RUNXnumber << ") - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}

    //DX
    //DX if(aflags.KBIN_GEN_VASP_FROM_AFLOWIN || aflags.KBIN_GEN_AFLOWIN_FROM_VASP)
    if(aflags.KBIN_GEN_VASP_FROM_AFLOWIN || aflags.KBIN_GEN_AFLOWIN_FROM_VASP || aflags.KBIN_GEN_SYMMETRY_OF_AFLOWIN || aflags.KBIN_GEN_AIMS_FROM_AFLOWIN) //CO20180409
    { //CO20200106 - patching for auto-indenting
      //DX
      XHOST.AFLOW_RUNXflag=TRUE;XHOST.AFLOW_RUNXnumber=1;
      XHOST.AFLOW_RUNDIRflag=TRUE;
    }

    // if(aflags.AFLOW_GLOBAL_NCPUS && !MPI && !aflags.AFLOW_FORCE_MPI) {}
    if(XHOST.MPI || aflags.AFLOW_FORCE_MPI) AFLOW_PTHREADS::No_Threads();

    // turn off multi pthreads on specific machines
    if(aflags.AFLOW_MACHINE_GLOBAL.flag()) {
      AFLOW_PTHREADS::No_Threads();
      AFLOW_PTHREADS::FLAG=FALSE; // safety...
      AFLOW_PTHREADS::MAX_PTHREADS=1; // safety...
      //    kflags.KBIN_MPI=TRUE; // overrides the MPI for machines
      XHOST.MPI=TRUE;
    }

    if(aflags.AFLOW_PERFORM_DIRECTORY) {aus << "MMMMM  KBIN option PERFORM_DIRECTORY - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
    if(aflags.AFLOW_PERFORM_FILE) {aus << "MMMMM  KBIN option PERFORM_FILE - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
    if(aflags.AFLOW_PERFORM_FILE_MIX) {aus << "MMMMM  KBIN option PERFORM_FILE_MIX - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);} //CO20260101
    if(aflags.AFLOW_PERFORM_ORDER_SORT) {aus << "MMMMM  KBIN option ORDER_SORT - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
    if(aflags.AFLOW_PERFORM_ORDER_REVERSE) {aus << "MMMMM  KBIN option ORDER_REVERSE - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
    if(aflags.AFLOW_PERFORM_ORDER_RANDOM) {aus << "MMMMM  KBIN option ORDER_RANDOM - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
    if(aflags.AFLOW_FORCE_RUN) {aus << "MMMMM  KBIN option FORCE_RUN - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}

    vector<string> vruns;

    uint maxcheck=VRUNS_MAX_CUTOFF;
    if(XHOST.AFLOW_MULTIflag) maxcheck=VRUNS_MAX_CUTOFF;
    if(XHOST.AFLOW_RUNXflag) maxcheck=XHOST.AFLOW_RUNXnumber;
    if(maxcheck==0) maxcheck=1; // safety

    // simple commands
    if(XHOST.vflag_aflow.flag("XCLEAN")) {
      KBIN::XClean(XHOST.vflag_aflow.getattachedscheme("XCLEAN"));
      return 1;
    }

    // for directory mode load them all
    if(aflags.AFLOW_PERFORM_DIRECTORY) {
      // [OBSOLETE] vruns=aurostd::args2vectorstring(argv,"--DIRECTORY|--D|--d","./");
      aurostd::string2tokens(XHOST.vflag_control.getattachedscheme("VDIR"),vruns,",");
      if(LDEBUG) { for(uint i=0;i<vruns.size();i++) cerr << XPID << "KBIN::Main: vruns[i]=" << vruns[i] << endl;}
    } else {
      if(!aflags.AFLOW_PERFORM_FILE)
        vruns.push_back(aurostd::getPWD()); //CO20191112
      //[CO20191112 - OBSOLETE]vruns.push_back(aurostd::execute2string(XHOST.command("pwd")));
      // vruns.push_back(aurostd::execute2string(XHOST.command("pwd"))+" ./");
    }
    // if file found
    if(aflags.AFLOW_PERFORM_FILE) {
      // [OBSOLETE]   string file_name=aurostd::args2string(argv,"--FILE|--F|--f","xxxx");
      string file_name=XHOST.vflag_control.getattachedscheme("FILE");
      if(!aurostd::FileExist(file_name)) {
        aus << "EEEEE  FILE_NOT_FOUND = " << file_name  << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);return 1;  //CO20200624 - previously exit
      }
      if(aurostd::FileEmpty(file_name)) {
        aus << "EEEEE  FILE_EMPTY = " << file_name  << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);return 1;  //CO20200624 - previously exit
      }
      aus << "MMMMM  Loading File = " << file_name << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
      vector<string> vlines;
      vlines.clear();
      aurostd::file2vectorstring(file_name,vlines);

      if(aflags.AFLOW_PERFORM_FILE_MIX){  //CO20260101
        string file_mix_name=XHOST.vflag_control.getattachedscheme("FILE_MIX");
        if(!aurostd::FileExist(file_mix_name)) {
          aus << "EEEEE  FILE_NOT_FOUND = " << file_mix_name  << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);return 1;  //CO20200624 - previously exit
        }
        if(aurostd::FileEmpty(file_mix_name)) {
          aus << "EEEEE  FILE_EMPTY = " << file_mix_name  << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);return 1;  //CO20200624 - previously exit
        }
        aus << "MMMMM  Loading File = " << file_mix_name << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
        vector<string> vlines_new;
        vlines_new.clear();
        aurostd::file2vectorstring(file_mix_name,vlines_new);
        vlines.insert(vlines.end(), vlines_new.begin(), vlines_new.end());
      }

      aus << "MMMMM  " <<  aurostd::PaddedPOST("Legitimate VLINES = "+aurostd::utype2string(vlines.size()),40) << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
      aus << "MMMMM  " <<  aurostd::PaddedPOST("         maxcheck = "+aurostd::utype2string(maxcheck),40) << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);

      if(aflags.AFLOW_PERFORM_ORDER_SORT) {   // SORT do something
        aus << "MMMMM  Requested SORT [aflags.AFLOW_PERFORM_ORDER_SORT=1] - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
        aurostd::sort(vlines);}
      if(aflags.AFLOW_PERFORM_ORDER_REVERSE) { // REVERSE do something
        aus << "MMMMM  Requested REVERSE_SORT [aflags.AFLOW_PERFORM_ORDER_REVERSE=1] - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
        aurostd::rsort(vlines);}
      if(aflags.AFLOW_PERFORM_ORDER_RANDOM) { // RANDOM do something
        aus << "MMMMM  Requested RANDOM [aflags.AFLOW_PERFORM_ORDER_RANDOM=1] start (XHOST.AFLOW_RUNXnumber=" << XHOST.AFLOW_RUNXnumber << ") - " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(aus,XHOST.QUIET);
        aurostd::random_shuffle(vlines);  // uses the std library but the seed is initialized in xrandom too
        aus << "MMMMM  Requested RANDOM [aflags.AFLOW_PERFORM_ORDER_RANDOM=1] stop (XHOST.AFLOW_RUNXnumber=" << XHOST.AFLOW_RUNXnumber << ") - " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(aus,XHOST.QUIET);
      }

      for(uint i=0;(i<vlines.size() && vruns.size()<XHOST_AFLOW_RUNXnumber_multiplier*maxcheck && vruns.size()<VRUNS_MAX_CUTOFF);i++) {  // XHOST_AFLOW_RUNXnumber_multiplier times more... for safety
        if(Legitimate_aflowin(vlines[i],FALSE,aus)) {vruns.push_back(vlines[i]);} // TRUE puts too much verbosity
      }

      aus << "MMMMM  " <<  aurostd::PaddedPOST("Legitimate VRUNS = "+aurostd::utype2string(vruns.size()),40) << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
      // aus << "MMMMM  Legitimate VRUNS = " << vruns.size() << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
      // cerr << vlines.size() << endl;
    }

    if(aflags.AFLOW_PERFORM_CLEAN && !XHOST.AFLOW_RUNDIRflag && !XHOST.AFLOW_MULTIflag && !XHOST.AFLOW_RUNXflag) {
      XHOST.AFLOW_RUNDIRflag=TRUE; // give something to clean
    }

    if(aflags.AFLOW_PERFORM_CLEAN && !aflags.AFLOW_PERFORM_DIRECTORY) {
      throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"to use --clean, you must specify one or more directories",_INPUT_MISSING_);  //CO20200624
    }
    if(aflags.AFLOW_PERFORM_CLEAN && aflags.AFLOW_PERFORM_DIRECTORY) {
      //    cout << "DEBUG CLEAN = " << vruns.size() << endl;
    }

    bool STOP_DEBUG=aurostd::args2flag(argv,"--STOP|--stop");

    // cdebug << "aflags.KBIN_RUN_AFLOWIN=" << aflags.KBIN_RUN_AFLOWIN << endl;
    // cdebug << "aflags.KBIN_GEN_VASP_FROM_AFLOWIN=" << aflags.KBIN_GEN_VASP_FROM_AFLOWIN << endl;
    // cdebug << "aflags.KBIN_GEN_AFLOWIN_FROM_VASP=" << aflags.KBIN_GEN_AFLOWIN_FROM_VASP << endl;

    // ------------------------------------------------------------------------------------------------------------------------------------
    // nothing to run
    if(!XHOST.AFLOW_RUNDIRflag && !XHOST.AFLOW_MULTIflag && !XHOST.AFLOW_RUNXflag && !aflags.AFLOW_PERFORM_CLEAN && !aflags.AFLOW_PERFORM_DIRECTORY) {
      aus << "MMMMM  KBIN option nothing to run [--run , --run=multi, --run=1, --run=N] - " << Message(__AFLOW_FILE__,aflags) << endl;
      aurostd::PrintMessageStream(aus,XHOST.QUIET);
      //    return 0;
    }

    // ------------------------------------------------------------------------------------------------------------------------------------
    // nothing specified : CHECK IF DAEMON
    if(XHOST.AFLOW_RUNDIRflag) { // check if daemaon aflowd
      string progname=argv[0];
      if(aurostd::substring2bool(progname,"aflowd")) {
        XHOST.AFLOW_MULTIflag=TRUE;
        XHOST.AFLOW_RUNXflag=FALSE;
        XHOST.vflag_aflow.flag("LOOP",TRUE); // add automatically
        aus << "MMMMM  AFLOW: running as DAEMON (aflow --kmode --multi --loop): " << " - " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(aus,XHOST.QUIET);
      }
    }

    // ------------------------------------------------------------------------------------------------------------------------------------
    // run specified directory: XHOST.AFLOW_RUNDIRflag
    if(XHOST.AFLOW_RUNDIRflag) {
      //    bool krun=TRUE;
      if(LDEBUG) cerr << __AFLOW_FUNC__ << " STEP0b" << endl;
      // [OBSOLETE]   vector<string> vDirectory(aurostd::args2vectorstring(argv,"--DIRECTORY|--D|--d","./"));
      vector<string> vDirectory=vruns;
      // fix the RUNS
      for(uint ii=0;ii<vDirectory.size();ii++)
        if(aurostd::substring2bool(vDirectory[ii],_AFLOWIN_))
          aurostd::StringSubst(vDirectory[ii],_AFLOWIN_,"");

      // if(aflags.AFLOW_PERFORM_ORDER_SORT) {   // SORT do something
      //   aus << "MMMMM  Requested SORT [aflags.AFLOW_PERFORM_ORDER_SORT=1] - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
      //   aurostd::sort(vDirectory);}
      // if(aflags.AFLOW_PERFORM_ORDER_REVERSE) { // REVERSE do something
      //   aus << "MMMMM  Requested REVERSE_SORT [aflags.AFLOW_PERFORM_ORDER_REVERSE=1] - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
      //   aurostd::rsort(vDirectory);}
      // if(aflags.AFLOW_PERFORM_ORDER_RANDOM) { // RANDOM do something
      //   aus << "MMMMM  Requested RANDOM [aflags.AFLOW_PERFORM_ORDER_RANDOM=1] - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
      //   aurostd::random_shuffle(vDirectory);}  // uses the std library but the seed is initialized in xrandom too

      // aurostd::random_shuffle(vDirectory);
      // std::random_shuffle(vDirectory.begin(),vDirectory.end());

      aurostd::xoption opts_clean;  //CO20210716
      opts_clean.flag("SAVE_CONTCAR",aurostd::args2flag(argv,"--contcar_save|--save_contcar"));  //CO20210716 - saves contcar no matter what
      opts_clean.flag("SAVE_CONTCAR_OUTCAR_COMPLETE",aurostd::args2flag(argv,"--contcar_save_outcar_complete|--save_contcar_outcar_complete"));  //CO20210716 - saves contcar only if outcar is complete
      opts_clean.flag("KEEP_RELAX",aurostd::args2flag(argv,"--keep_relax"));  //CO20261006 - keeps what VASP computed, see KBIN::Clean()

      for(uint idir=0;idir<vDirectory.size();idir++) {
        bool krun=TRUE;
        aflags.Directory=vDirectory[idir];
        aus << "MMMMM  AFLOW: running " << _AFLOWIN_ << ", directory" << "=\""  << aflags.Directory << "\" - " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(aus,XHOST.QUIET);
        // If necessary PERFORM CLEAN
        if(krun && aflags.AFLOW_PERFORM_CLEAN) {
          aflags.Directory=vDirectory[idir];
          //[CO20210716 - OBSOLETE]aurostd::StringSubst(aflags.Directory,"/"+_AFLOWLOCK_,"");
          //[CO20210716 - OBSOLETE]for(uint iext=1;iext<XHOST.vext.size();iext++) { // SKIP uncompressed
          //[CO20210716 - OBSOLETE]  aurostd::StringSubst(aflags.Directory,"/OUTCAR.relax1"+XHOST.vext[iext],"");    // CLEAN UP A LITTLE
          //[CO20210716 - OBSOLETE]  aurostd::StringSubst(aflags.Directory,"/OUTCAR.relax2"+XHOST.vext[iext],"");    // CLEAN UP A LITTLE
          //[CO20210716 - OBSOLETE]  aurostd::StringSubst(aflags.Directory,"/OUTCAR.static"+XHOST.vext[iext],"");    // CLEAN UP A LITTLE
          //[CO20210716 - OBSOLETE]  aurostd::StringSubst(aflags.Directory,"/OUTCAR.bands"+XHOST.vext[iext],"");     // CLEAN UP A LITTLE
          //[CO20210716 - OBSOLETE]}
          //[CO20210716 - OBSOLETE]for(uint iext=1;iext<XHOST.vext.size();iext++) {  // SKIP uncompressed
          //[CO20210716 - OBSOLETE]  aurostd::StringSubst(aflags.Directory,"/EIGENVAL.relax1"+XHOST.vext[iext],"");  // CLEAN UP A LITTLE
          //[CO20210716 - OBSOLETE]  aurostd::StringSubst(aflags.Directory,"/EIGENVAL.relax2"+XHOST.vext[iext],"");  // CLEAN UP A LITTLE
          //[CO20210716 - OBSOLETE]  aurostd::StringSubst(aflags.Directory,"/EIGENVAL.static"+XHOST.vext[iext],"");  // CLEAN UP A LITTLE
          //[CO20210716 - OBSOLETE]  aurostd::StringSubst(aflags.Directory,"/EIGENVAL.bands"+XHOST.vext[iext],"");   // CLEAN UP A LITTLE
          //[CO20210716 - OBSOLETE]}
          //[CO20210716 - OBSOLETE]aurostd::StringSubst(aflags.Directory,"/OUTCAR","");  // so it is easier to search
          //[CO20210716 - OBSOLETE]aurostd::StringSubst(aflags.Directory,"/"+_AFLOWIN_,"");  // so it is easier to search
          //  cerr << aflags.Directory << endl;
          KBIN::Clean(aflags,opts_clean);
          krun=FALSE;
        }
        // RUN
        // cerr << "STEP0b" << endl;
        if(krun) {
          //[SD20220224 - OBSOLETE]if(LDEBUG) cerr << "STEP1b" << endl;  //CO20170622 - should be debug
          //[SD20220224 - OBSOLETE]ifstream FileCHECK;string FileNameCHECK;
          //[SD20220224 - OBSOLETE]// check for directory
          //[SD20220224 - OBSOLETE]FileNameCHECK=aflags.Directory;
          //[SD20220224 - OBSOLETE]FileCHECK.open(FileNameCHECK.c_str(),std::ios::in);
          //[SD20220224 - OBSOLETE]FileCHECK.clear();FileCHECK.close();
          //[SD20220224 - OBSOLETE]if(!FileCHECK) {                                                                        // ******* Directory is non existent
          //[SD20220224 - OBSOLETE]  aus << "EEEEE  DIRECTORY_NOT_FOUND = "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]  aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]} else {                                                                                // ******* Directory EXISTS
          //[SD20220224 - OBSOLETE]  if(LDEBUG) cerr << __AFLOW_FUNC__ << " STEP1c" << endl;
          //[SD20220224 - OBSOLETE]  if(aurostd::DirectoryLocked(aflags.Directory,_AFLOWLOCK_)) {                                               // ******* Directory is locked
          //[SD20220224 - OBSOLETE]    aus << "LLLLL  DIRECTORY_LOCKED ...bzzzz... !MULTI = "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]    aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]  } else {
          //[SD20220224 - OBSOLETE]    if(LDEBUG) cerr << __AFLOW_FUNC__ << " STEP1d" << endl;
          //[SD20220224 - OBSOLETE]    if(DirectorySkipped(aflags.Directory)) {                                            // ******* Directory is skipped
          //[SD20220224 - OBSOLETE]      aus << "LLLLL  DIRECTORY_SKIPPED ...bzzzz... !MULTI = "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]      aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]    } else {        
          //[SD20220224 - OBSOLETE]      if(LDEBUG) cerr << __AFLOW_FUNC__ << " STEP1e" << endl;
          //[SD20220224 - OBSOLETE]      if(DirectoryAlreadyInDatabase(aflags.Directory,aflags.AFLOW_FORCE_RUN)) {         // ******* Directory is already in the database
          //[SD20220224 - OBSOLETE]        aus << "LLLLL  DIRECTORY_ALREADY_IN_DATABASE ...bzzzz... !MULTI = "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]        aus << "LLLLL  DIRECTORY_ALREADY_IN_DATABASE ... use \"aflow --multi\" to force the calculation of this entry "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]        aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]      } else {        
          //[SD20220224 - OBSOLETE]        if(LDEBUG) cerr << __AFLOW_FUNC__ << " STEP1f" << endl;
          //[SD20220224 - OBSOLETE]        if(DirectoryUnwritable(aflags.Directory)) {                                     // ******* Directory is unwritable
          //[SD20220224 - OBSOLETE]          aus << "LLLLL  DIRECTORY_UNWRITABLE ...bzzzz... !MULTI = "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]          aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]        } else {                                                                        // ******* Directory is ok
          //[SD20220224 - OBSOLETE]          if(LDEBUG) cerr << __AFLOW_FUNC__ << " STEP1g" << endl;
          //[SD20220224 - OBSOLETE]          if(_VERBOSE_) aus << "LLLLL  GOOD ...bzzz... !MULTI = "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]          if(_VERBOSE_) aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]          if(aflags.KBIN_RUN_AFLOWIN)               FileNameCHECK=aflags.Directory+"/"+_AFLOWIN_;
          //[SD20220224 - OBSOLETE]          if(aflags.KBIN_GEN_VASP_FROM_AFLOWIN)     FileNameCHECK=aflags.Directory+"/"+_AFLOWIN_;
          //[SD20220224 - OBSOLETE]          if(aflags.KBIN_GEN_AIMS_FROM_AFLOWIN)     FileNameCHECK=aflags.Directory+"/"+_AFLOWIN_; //CO20180409
          //[SD20220224 - OBSOLETE]          if(aflags.KBIN_GEN_AFLOWIN_FROM_VASP)     FileNameCHECK=aflags.Directory+"/INCAR";
          //[SD20220224 - OBSOLETE]          FileCHECK.open(FileNameCHECK.c_str(),std::ios::in);
          //[SD20220224 - OBSOLETE]          FileCHECK.clear();FileCHECK.close();
          //[SD20220224 - OBSOLETE]          if(!FileCHECK) {                                                                    // ******* _AFLOWIN_ does not exist
          //[SD20220224 - OBSOLETE]            if(LDEBUG) cerr << __AFLOW_FUNC__ << " STEP1h" << endl;
          //[SD20220224 - OBSOLETE]            if(aflags.KBIN_RUN_AFLOWIN)               aus << "EEEEE  " << _AFLOWIN_ << " not found  = "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]            if(aflags.KBIN_GEN_VASP_FROM_AFLOWIN)     aus << "EEEEE  " << _AFLOWIN_ << " not found  = "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]            if(aflags.KBIN_GEN_AIMS_FROM_AFLOWIN)     aus << "EEEEE  " << _AFLOWIN_ << " not found  = "  << Message(__AFLOW_FILE__,aflags) << endl; //CO20180409
          //[SD20220224 - OBSOLETE]            if(aflags.KBIN_GEN_AFLOWIN_FROM_VASP)     aus << "EEEEE  INCAR not found     = "  << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]            aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]          } else {                                                                            // ******* _AFLOWIN_ exists RUN
          //[SD20220224 - OBSOLETE]            if(LDEBUG) cerr << __AFLOW_FUNC__ << " STEP1i" << endl;
          //[SD20220224 - OBSOLETE]            if(aflags.KBIN_RUN_AFLOWIN)  {
          //[SD20220224 - OBSOLETE]              KBIN::RUN_Directory(aflags);
          //[SD20220224 - OBSOLETE]            }
          //[SD20220224 - OBSOLETE]            // if(aflags.KBIN_GEN_VASP_FROM_AFLOWIN)    KBIN::RUN_Directory(aflags);
          //[SD20220224 - OBSOLETE]            if(aflags.KBIN_GEN_AFLOWIN_FROM_VASP)     KBIN::GenerateAflowinFromVASPDirectory(aflags);
          //[SD20220224 - OBSOLETE]            aus << "MMMMM  AFLOW: Run Done " << " - " << Message(__AFLOW_FILE__,aflags) << endl;
          //[SD20220224 - OBSOLETE]            aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]          }
          //[SD20220224 - OBSOLETE]        } // DIRECTORY WRITABLE
          //[SD20220224 - OBSOLETE]      } // DIRECTORY NOT IN THE DATABASE
          //[SD20220224 - OBSOLETE]    } // DIRECTORY UN SKIPPED
          //[SD20220224 - OBSOLETE]  } // DIRECTORY UN LOCKED
          //[SD20220224 - OBSOLETE]} // DIRECTORY FOUND
          if(Legitimate_krun(aflags)){
            if(aflags.KBIN_RUN_AFLOWIN){KBIN::RUN_Directory(aflags);}
            if(aflags.KBIN_GEN_AFLOWIN_FROM_VASP){KBIN::GenerateAflowinFromVASPDirectory(aflags);}
            aus << "MMMMM  AFLOW: Done " << " - " << Message(__AFLOW_FILE__,aflags) << endl;
            aurostd::PrintMessageStream(aus,XHOST.QUIET);
          }
        }
      } // idir
      // aus << "MMMMM  AFLOW: Done " << " - " << Message(__AFLOW_FILE__,aflags) << endl;
      // aurostd::PrintMessageStream(aus,XHOST.QUIET);
      if(LDEBUG) cerr << __AFLOW_FUNC__ << " STEP2" << endl;
    }

    // ERRORS ------------------------------------------------------------------------------------------------

    // ------------------------------------------------------------------------------------------------------------------------------------
    // run MULTI and XHOST.AFLOW_RUNXflag (in XHOST.AFLOW_RUNXflag, runs only XHOST.AFLOW_RUNXnumber and then dies)
    // MULTI with SINGLE AND MULTI THREAD VERSION -----------------------------------------------------------------------------------------
    if(XHOST.AFLOW_MULTIflag || XHOST.AFLOW_RUNXflag) {
      uint RUN_times=0;
      bool MULTI_DEBUG=FALSE;
      if(MULTI_DEBUG) {aus << "MMMMM  AFLOW_PTHREADS::FLAG=" << AFLOW_PTHREADS::FLAG << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
      if(MULTI_DEBUG) {aus << "MMMMM  AFLOW_PTHREADS::MAX_PTHREADS=" << AFLOW_PTHREADS::MAX_PTHREADS << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
      bool EMPTY=FALSE,FOUND=FALSE;
      bool free_thread;
      int ithread=0;
      if(AFLOW_PTHREADS::FLAG && AFLOW_PTHREADS::MAX_PTHREADS<=1) {
        aus << "EEEEE  ERROR PTHREADS" << endl;
        aus << "MMMMM  AFLOW_PTHREADS::FLAG=" << AFLOW_PTHREADS::FLAG << endl;
        aus << "MMMMM  AFLOW_PTHREADS::MAX_PTHREADS=" << AFLOW_PTHREADS::MAX_PTHREADS << endl;
        aurostd::PrintMessageStream(aus,XHOST.QUIET);
        return 1; //CO20200624 - previously exit
      }
      if(AFLOW_PTHREADS::MAX_PTHREADS>1) {
        aus << "MMMMM  AFLOW: MULTI THREAD START: phread_max=" << AFLOW_PTHREADS::MAX_PTHREADS << " - " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(aus,XHOST.QUIET);
      }
      aus << "MMMMM  AFLOW: searching subdirectories [d1] " << " - " << Message(__AFLOW_FILE__,aflags) << endl;
      aurostd::PrintMessageStream(aus,XHOST.QUIET);
      while(!EMPTY) {
        vector<string> vaflowin;
        //   cerr << "aflags.AFLOW_PERFORM_DIRECTORY=" << aflags.AFLOW_PERFORM_DIRECTORY << endl;
        // cerr << "aflags.AFLOW_PERFORM_FILE=" << aflags.AFLOW_PERFORM_FILE << endl;

        if(aflags.AFLOW_PERFORM_FILE==FALSE) { // NO FILE SPECIFIED = standard  RUN DIRECTORY
          aus << "MMMMM  AFLOW: aflags.AFLOW_PERFORM_FILE==FALSE" << " - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);

          string FileNameSUBDIR=aurostd::TmpFileCreate("RUN");
          stringstream strstream;
          aus << "rm -f " <<  FileNameSUBDIR << endl;
          aurostd::execute(aus);  // RESET  // RESET

          bool isPRIORITY=FALSE;
          // NEW, the sorting is done internally (speed and reliability)
          for(int ifind=0;ifind<(int)vruns.size();ifind++) {
            isPRIORITY=(aurostd::substring2bool(vruns[ifind],"PRIORITY") || aurostd::substring2bool(vruns[ifind],"priority"));
            if((isPRIORITY && aurostd::uniform(1.0)<=PRIORITY_PROBABILITY) || !isPRIORITY) {
              aus << "find " << vruns[ifind] << " " << XHOST.Find_Parameters;
              if(aflags.KBIN_RUN_AFLOWIN) aus << " -name \"" << _AFLOWIN_ << "\" ";
              // if(aflags.KBIN_RUN_AFLOWIN) aus << " -name \"" << _AFLOWIN_ << "\" ";
              // if(aflags.KBIN_RUN_AFLOWIN) aus << " -name \"" << _AFLOWIN_ << "\" ";
              // if(aflags.KBIN_GEN_VASP_FROM_AFLOWIN)  aus << " -name \"" << _AFLOWIN_ << "\" ";   // IS THIS CORRECT ?? CHECK !!!
              if(aflags.KBIN_GEN_AFLOWIN_FROM_VASP)     aus << " -name \"INCAR\" ";
              aus << " | " << PRIORITY_GREP_STRING << " >> ";
              aus << FileNameSUBDIR << endl;
            }
          }
          // perform the command
          aurostd::execute(aus);  // RESET  // RESET

          if(_VERBOSE_) aus << "MMMMM  AFLOW: searching subdirectories [d2] " << " - " << Message(__AFLOW_FILE__,aflags) << endl;
          if(_VERBOSE_) aurostd::PrintMessageStream(aus,XHOST.QUIET);
          aurostd::string2tokens(aurostd::file2string(FileNameSUBDIR),vaflowin,"\n");
          for(i=0;i<(int) vaflowin.size();i++) aurostd::StringSubst(vaflowin[i],_AFLOWIN_,"");
          for(i=0;i<(int) vaflowin.size();i++) aurostd::StringSubst(vaflowin[i],"INCAR","");
          if(STOP_DEBUG) for(i=0;i<(int) vaflowin.size();i++) cout << vaflowin[i] << endl;
          // RANDOMIZING priority
          if(vaflowin.size()>1) { // only if I can poll
            // if(aurostd::substring2bool(vaflowin[0],"PRIORITY") || aurostd::substring2bool(vaflowin[0],"priority")) aurostd::random_shuffle(vaflowin);
          }
          // loaded up
          aus << "rm -f " << FileNameSUBDIR << endl;
          aurostd::execute(aus);  // RESET  // RESET
        }

        // FILE SPECIFIED
        if(aflags.AFLOW_PERFORM_FILE) {
          aus << "MMMMM  AFLOW: aflags.AFLOW_PERFORM_FILE==TRUE" << " - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
          vaflowin.clear();
          for(uint i=0;(i<vruns.size() && vaflowin.size()<maxcheck);i++) {
            if(Legitimate_aflowin(vruns[i],FALSE,aus)) {vaflowin.push_back(vruns[i]);} // TRUE puts too much verbosity
            // vaflowin.push_back(vruns[i]); // just load them up... they were checked before //OLD MUST RECHECH THEM as things change on the fly
          }
        }
        // NOW TIME OF SORTING/RANDOMIZING
        if(aflags.AFLOW_PERFORM_ORDER_SORT) {   // SORT do something
          aus << "MMMMM  Requested SORT [aflags.AFLOW_PERFORM_ORDER_SORT=1] - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
          aurostd::sort(vaflowin);}
        if(aflags.AFLOW_PERFORM_ORDER_REVERSE) { // REVERSE do something
          aus << "MMMMM  Requested REVERSE_SORT [aflags.AFLOW_PERFORM_ORDER_REVERSE=1] - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
          aurostd::sort(vaflowin); //PN modification 
          aurostd::rsort(vaflowin);}
        if(aflags.AFLOW_PERFORM_ORDER_RANDOM) { // RANDOM do something
          aus << "MMMMM  Requested RANDOM [aflags.AFLOW_PERFORM_ORDER_RANDOM=1] - " << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
          aurostd::random_shuffle(vaflowin);}  // uses the std library but the seed is initialized in xrandom too
        aus << "MMMMM  " <<  aurostd::PaddedPOST("Legitimate VAFLOWIN = "+aurostd::utype2string(vaflowin.size()),40) << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);
        //      aus << "MMMMM  Legitimate VAFLOWIN = " << vaflowin.size() << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);

        //	for(uint i=0;i<vaflowin.size();i++) cerr << i << " " << vaflowin[i] << endl;

        // clean AFLOWIN // SAFETY
        for(uint i=0;i<vaflowin.size();i++) aurostd::StringSubst(vaflowin[i],_AFLOWIN_,"");  

        if(MULTI_DEBUG) {aus << "MMMMM  SIZE vaflowin=" << vaflowin.size() << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}
        if(MULTI_DEBUG) {for(uint i=0;i<vaflowin.size();i++) {aus << "MMMMM  vaflowin[i]=" << vaflowin[i] << Message(__AFLOW_FILE__,aflags) << endl;aurostd::PrintMessageStream(aus,XHOST.QUIET);}}
        // cerr << vaflowin.at(10) << " " << vaflowin.at(20) << " " << vaflowin.at(30) << endl;
        // if(aurostd::substring2bool(vaflowin[0],"PRIORITY") || aurostd::substring2bool(vaflowin[0],"priority")) aurostd::random_shuffle(vaflowin);

        FOUND=FALSE;
        for(uint i=0;i<vaflowin.size()&& !FOUND;i++) {
          aflags.Directory="NULL";
          //[SD20220224 - OBSOLETE]if(aurostd::DirectoryLocked(vaflowin[i],_AFLOWLOCK_)) {
          //[SD20220224 - OBSOLETE]  if(_VERBOSE_ || STOP_DEBUG) aus << "LLLLL  LOCKED ...bzzz... MULTI "  << vaflowin[i] << " " << XHOST.hostname << " " << aflow_get_time_string() << endl;
          //[SD20220224 - OBSOLETE]  if(_VERBOSE_ || STOP_DEBUG) aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]  FOUND=FALSE;
          //[SD20220224 - OBSOLETE]} else {
          //[SD20220224 - OBSOLETE]  if(DirectorySkipped(vaflowin[i])) {
          //[SD20220224 - OBSOLETE]    if(_VERBOSE_ || STOP_DEBUG) aus << "LLLLL  SKIPPED ...bzzz... MULTI "  << vaflowin[i] << " " << XHOST.hostname << " " << aflow_get_time_string() << endl;
          //[SD20220224 - OBSOLETE]    if(_VERBOSE_ || STOP_DEBUG) aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]    FOUND=FALSE;
          //[SD20220224 - OBSOLETE]  } else {
          //[SD20220224 - OBSOLETE]    if(DirectoryAlreadyInDatabase(vaflowin[i],aflags.AFLOW_FORCE_RUN)) {
          //[SD20220224 - OBSOLETE]      if(_VERBOSE_ || STOP_DEBUG) aus << "LLLLL  DIRECTORY_ALREADY_IN_DATABASE ...bzzz... MULTI "  << vaflowin[i] << " " << XHOST.hostname << " " << aflow_get_time_string() << endl;
          //[SD20220224 - OBSOLETE]      if(_VERBOSE_ || STOP_DEBUG) aus << "LLLLL  DIRECTORY_ALREADY_IN_DATABASE ... use \"aflow --multi\" to force the calculation of this entry...  MULTI "  << vaflowin[i] << " " << XHOST.hostname << " " << aflow_get_time_string() << endl;
          //[SD20220224 - OBSOLETE]      if(_VERBOSE_ || STOP_DEBUG) aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]      FOUND=FALSE;
          //[SD20220224 - OBSOLETE]    } else {
          //[SD20220224 - OBSOLETE]      if(DirectoryUnwritable(vaflowin[i])) {
          //[SD20220224 - OBSOLETE]        if(_VERBOSE_ || STOP_DEBUG) aus << "LLLLL  UNWRITABLE ...bzzz... MULTI "  << vaflowin[i] << " " << XHOST.hostname << " " << aflow_get_time_string() << endl;
          //[SD20220224 - OBSOLETE]        if(_VERBOSE_ || STOP_DEBUG) aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]        FOUND=FALSE;
          //[SD20220224 - OBSOLETE]      } else {
          //[SD20220224 - OBSOLETE]        if(_VERBOSE_ || STOP_DEBUG) aus << "LLLLL  GOOD ...bzzz... MULTI "  << vaflowin[i] << " " << XHOST.hostname << " " << aflow_get_time_string() << endl;
          //[SD20220224 - OBSOLETE]        if(_VERBOSE_ || STOP_DEBUG) aurostd::PrintMessageStream(aus,XHOST.QUIET);
          //[SD20220224 - OBSOLETE]        aflags.Directory=vaflowin[i];
          //[SD20220224 - OBSOLETE]        FOUND=TRUE;
          //[SD20220224 - OBSOLETE]      } // DIRECTORY WRITABLE
          //[SD20220224 - OBSOLETE]    } // DIRECTORY NOT IN THE DATABASE
          //[SD20220224 - OBSOLETE]  } // DIRECTORY UN SKIPPED
          //[SD20220224 - OBSOLETE]} // DIRECTORY UN LOCKED
          if(Legitimate_aflowdir(vaflowin[i],aflags)){
            aflags.Directory=vaflowin[i];
            FOUND=TRUE;
          }
        }
        // exiting if STOP_DEBUG
        if(STOP_DEBUG) {cout << "aflow_kbin.cpp: STOP_DEBUG" << endl; return 1;}  //CO20200624 - previously exit

        // FOUND SOMETHING
        if(FOUND==FALSE) {
          EMPTY=TRUE;
          if(AFLOW_PTHREADS::FLAG) {
            aus << "MMMMM  AFLOW: MULTI-THREADED: FLUSHING PTHREADS - " << Message(__AFLOW_FILE__,aflags) << endl;
            aurostd::PrintMessageStream(aus,XHOST.QUIET);
            for(ithread=0;ithread<AFLOW_PTHREADS::MAX_PTHREADS;ithread++)
              if(AFLOW_PTHREADS::vpthread_busy[ithread]) {
                aus << "MMMMM  AFLOW: MULTI-THREADED: Flushing   pthread=" << ithread << "   pthread_max=" << AFLOW_PTHREADS::MAX_PTHREADS << " - " << " - " << Message(__AFLOW_FILE__,aflags) << endl;
                aurostd::PrintMessageStream(aus,XHOST.QUIET);
                pthread_join(AFLOW_PTHREADS::vpthread[ithread],NULL);
              }
          }
        }
        // again another check for LOCK, because NFS (network file system might be slow in concurrent seaches
        //     if(aurostd::DirectoryLocked(aflags.Directory,_AFLOWLOCK_) && FOUND) {cerr << "AFLOW EXCEPTION on concurrent LOCK: " << aflags.Directory << endl; FOUND=FALSE;}
        if(aurostd::DirectoryLocked(aflags.Directory,_AFLOWLOCK_) && FOUND) {
          aus << "AFLOW EXCEPTION on concurrent LOCK: " << aflags.Directory << endl;
          aurostd::PrintMessageStream(aus,XHOST.QUIET);
          FOUND=FALSE;
        }

        //  ---------------------------------------------------------------------------- START RUNNING
        if(FOUND) {
          // again another check for LOCK, because NFS (network file system might be slow in concurrent seaches
          EMPTY=FALSE;
          //  ---------------------------------------------------------------------------- KIN_RUN_AFLOWIN
          if(aflags.KBIN_RUN_AFLOWIN) {
            //	  bool PHONONS;
            //  -------------------------------------------------------------------------- KIN_RUN_AFLOWIN multithreaded
            // 	  bool found;
            // 	  for(uint ii=0;ii<qaflags.size()&&!found;ii++)
            // 	    found=(qaflags[ii].Directory==aflags.Directory);       // look in all the list of operations
            // 	  if(found==FALSE) {                                 // new operation, generate and save it
            // 	    qaflags.push_back(aflags);
            // 	  }
            //	  cerr << qaflags.size() << endl;
            if(AFLOW_PTHREADS::FLAG) {
              // there is something to run in aflags.
              // wait and put in ithread there is the number of the thread
              free_thread=AFLOW_PTHREADS::Wait_Available_Free_Threads(ithread,_VERBOSE_);        // WAIT A WHILE !!
              if(free_thread) {
                aus << "MMMMM  AFLOW: Found subdirectory to run " << aflags.Directory<< " - " << Message(__AFLOW_FILE__,aflags) << endl;
                aus << "MMMMM  AFLOW: MULTI-THREADED: Starting    pthread_free=" << ithread << "   pthread_max=" << AFLOW_PTHREADS::MAX_PTHREADS << " - " << " - " << Message(__AFLOW_FILE__,aflags) << endl;
                aurostd::PrintMessageStream(aus,XHOST.QUIET);
                aflags.AFLOW_PTHREADS_NUMBER=ithread;
                KBIN::RUN_Directory_PTHREADS(aflags);
                RUN_times++;
                if(XHOST.AFLOW_RUNXflag) {
                  aus << "MMMMM  AFLOW: RUNFX finished running " << RUN_times <<  "/" << XHOST.AFLOW_RUNXnumber << " " << aflags.Directory<< " - " << Message(__AFLOW_FILE__,aflags) << endl;
                  aurostd::PrintMessageStream(aus,XHOST.QUIET);}
                if(XHOST.AFLOW_RUNXflag && RUN_times==XHOST.AFLOW_RUNXnumber) EMPTY=TRUE; // force to end if RUXN reached
              }
            }
            //  -------------------------------------------------------------------------- KIN_RUN_AFLOWIN normal
            if(!AFLOW_PTHREADS::FLAG) {
              KBIN::RUN_Directory(aflags);
              RUN_times++;
              if(XHOST.AFLOW_RUNXflag) {
                aus << "MMMMM  AFLOW: RUNFX finished running " << RUN_times <<  "/" << XHOST.AFLOW_RUNXnumber << " " << aflags.Directory<< " - " << Message(__AFLOW_FILE__,aflags) << endl;
                aurostd::PrintMessageStream(aus,XHOST.QUIET);}
              if(XHOST.AFLOW_RUNXflag && RUN_times==XHOST.AFLOW_RUNXnumber) EMPTY=TRUE; // force to end if RUXN reached
            }
          }
          //  ---------------------------------------------------------------------------- KBIN_GEN_VASP_FROM_AFLOWIN normal
          //	if(aflags.KBIN_GEN_VASP_FROM_AFLOWIN)     KBIN::RUN_Directory(argv,aflags);  // IS THIS CORRECT ?? CHECK !!!
          //  ---------------------------------------------------------------------------- KBIN_GEN_AFLOWIN_FROM_VASP normal
          if(aflags.KBIN_GEN_AFLOWIN_FROM_VASP) {
            KBIN::GenerateAflowinFromVASPDirectory(aflags);
          }
        }
        if(XHOST.vflag_aflow.flag("LOOP") && EMPTY && XHOST.AFLOW_RUNXflag==FALSE) {
          EMPTY=FALSE;
          aus << "MMMMM  AFLOW: waiting for new subdirectories: " << (int) _KBIN_LOOP_SLEEP_/60 << "mins ";
          aus << " - " << XHOST.hostname << " - " << aflow_get_time_string() << endl;//endl;
          aurostd::PrintMessageStream(aus,XHOST.QUIET);
          aurostd::Sleep(_KBIN_LOOP_SLEEP_);
        }
      }
      aus << "MMMMM  AFLOW: no more subdirectories to run " << " - " << Message(__AFLOW_FILE__,aflags) << endl;
      aurostd::PrintMessageStream(aus,XHOST.QUIET);
    }

    // ------------------------------------------------------------------------------------------------------------------------------------

    return 1;
  }
} // namespace end of MAIN


// ***************************************************************************
// KBIN::MPI_Extract
// ***************************************************************************
// This function extracts from _AFLOWIN_ the parameters for MPI run
namespace KBIN {
  void MPI_Extract(string AflowIn,ofstream &FileMESSAGE,_aflags &aflags,_kflags &kflags) {
    ostringstream aus;
    bool Kmpi=TRUE;
    kflags.KBIN_MPI_NCPUS=0;
    aus << "00000  [AFLOW_MODE_MPI] found in " << _AFLOWIN_ << " " << Message(__AFLOW_FILE__,aflags) << endl;
    aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);
    // get (integer) kflags.KBIN_MPI_NCPUS

    if(aflags.AFLOW_GLOBAL_NCPUS<1) {
      if(Kmpi && !aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]NCPUS=",TRUE) && !aflags.AFLOW_MACHINE_LOCAL.flag()) {                      // DEFAULT NO CPU SPECIFIED
        kflags.KBIN_MPI_NCPUS=MPI_NCPUS_DEFAULT;
        aus << "00000  MESSAGE MPI: NCPUS=NNNN is missing, taking NCPUS=" << kflags.KBIN_MPI_NCPUS << "  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);  
        Kmpi=FALSE;
      }
      if(Kmpi && (aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]NCPUS=MAX",TRUE) || aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]NCPUS=AUTO",TRUE) || aflags.AFLOW_MACHINE_LOCAL.flag())) { // DEFAULT NCPUS=MAX
        kflags.KBIN_MPI_NCPUS=XHOST.CPU_Cores;
        if (aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]NCPUS=MAX",TRUE)) kflags.KBIN_MPI_NCPUS_STRING = "MAX";
        if (aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]NCPUS=AUTO",TRUE)) kflags.KBIN_MPI_NCPUS_STRING = "AUTO";

        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::DUKE_BETA_MPICH")) kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN;	//CO
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::DUKE_BETA_OPENMPI")) kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN;        // with DUKE_BETA force NCPUS from QUEUE
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::DUKE_QRATS_MPICH")) kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN; //CO
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::DUKE_MATERIALS")) kflags.KBIN_MPI_NCPUS=XHOST.CPU_Cores;   // with DUKE_MATERIALS force NCPUS
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::DUKE_AFLOWLIB")) kflags.KBIN_MPI_NCPUS=XHOST.CPU_Cores;   // with DUKE_AFLOWLIB force NCPUS
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::DUKE_HABANA")) kflags.KBIN_MPI_NCPUS=XHOST.CPU_Cores;   // with DUKE_HABANA force NCPUS
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MPCDF_EOS")) kflags.KBIN_MPI_NCPUS=XHOST.SLURM_NTASKS; // [OBSOLETE] XHOST.SLURM_CPUS_ON_NODE; no CPUS because it gets fooled by HT
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MPCDF_DRACO")) kflags.KBIN_MPI_NCPUS=XHOST.SLURM_NTASKS; // [OBSOLETE] XHOST.SLURM_CPUS_ON_NODE; no CPUS because it gets fooled by HT
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MPCDF_COBRA")) kflags.KBIN_MPI_NCPUS=XHOST.SLURM_NTASKS; // [OBSOLETE] XHOST.SLURM_CPUS_ON_NODE; no CPUS because it gets fooled by HT
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MPCDF_HYDRA")) kflags.KBIN_MPI_NCPUS=XHOST.SLURM_NTASKS; // [OBSOLETE] XHOST.SLURM_CPUS_ON_NODE; no CPUS because it gets fooled by HT
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MACHINE001")) kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN;   // with MACHINE001; DX added 20190509
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MACHINE002")) kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN;   // with MACHINE002; DX added 20190509
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MACHINE003")) kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN;   // with MACHINE003; DX added 20201005
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MACHINE004")) kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN;   // with MACHINE004; DX added 20211011
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MACHINE101")) kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN;   // with MACHINE101; //CO20230512
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::MACHINE102")) kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN;   // with MACHINE102; //CO20230512
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::CMU_EULER"))  kflags.KBIN_MPI_NCPUS=XHOST.PBS_NUM_PPN;;  //DX20190107 - CMU EULER // with CMU_EULER force NCPUS //DX20181113
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::OHAD")) kflags.KBIN_MPI_NCPUS=XHOST.CPU_Cores;           // MACHINE2 has only NCPUS //CO20181113
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::HOST1")) kflags.KBIN_MPI_NCPUS=XHOST.CPU_Cores;          // MACHINE1 has only NCPUS //CO20181113
        if(aflags.AFLOW_MACHINE_LOCAL.flag("MACHINE::FULTON_MARYLOU"))  kflags.KBIN_MPI_NCPUS=XHOST.SLURM_NTASKS;        // with FULTON_MARYLOU force NCPUS
        if(kflags.KBIN_MPI_NCPUS<1) kflags.KBIN_MPI_NCPUS=XHOST.CPU_Cores; // SAFE

        aus << "00000  MESSAGE MPI: found NCPUS=MAX  NCPUS="<<kflags.KBIN_MPI_NCPUS<<" " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
        Kmpi=FALSE;
      }
      if(Kmpi && aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]NCPUS=",TRUE)) {                     // DEFAULT NCPUS=XXX
        kflags.KBIN_MPI_NCPUS=aurostd::substring2utype<int>(AflowIn,"[AFLOW_MODE_MPI_MODE]NCPUS=",TRUE);
        if(kflags.KBIN_MPI_NCPUS>0) {
          kflags.KBIN_MPI_NCPUS_STRING = aurostd::utype2string<int>(kflags.KBIN_MPI_NCPUS); //ME20181113
          aus << "00000  MESSAGE MPI: found NCPUS="<<kflags.KBIN_MPI_NCPUS<<" " << Message(__AFLOW_FILE__,aflags) << endl;
          aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
        }
        Kmpi=FALSE;
      }
    } else {
      kflags.KBIN_MPI_NCPUS=aflags.AFLOW_GLOBAL_NCPUS;
      aus << "00000  MESSAGE MPI: NCPUS is overriden, taking NCPUS=" << kflags.KBIN_MPI_NCPUS << "  " << Message(__AFLOW_FILE__,aflags) << endl;
      aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);  
    }

    if(kflags.KBIN_MPI_NCPUS<1) kflags.KBIN_MPI_NCPUS=1;                                              // DEFAULT NCPUS=troubles

    if(kflags.KBIN_MPI) {
      // get (string) kflags.KBIN_MPI_START
      if(!aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]START=",TRUE)) {
        kflags.KBIN_MPI_START=MPI_START_DEFAULT;
        aus << "00000  MESSAGE MPI: START string is missing, taking START=\"" << kflags.KBIN_MPI_START << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      } else {
        kflags.KBIN_MPI_START=aurostd::RemoveCharacter(aurostd::substring2string(AflowIn,"[AFLOW_MODE_MPI_MODE]START=",1,TRUE),'"');
        aus << "00000  MESSAGE MPI: found START=\"" << kflags.KBIN_MPI_START << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      }
      // get (string) kflags.KBIN_MPI_STOP
      if(!aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]STOP=",TRUE)) {
        kflags.KBIN_MPI_STOP=MPI_STOP_DEFAULT;
        aus << "00000  MESSAGE MPI: STOP string is missing, taking STOP=\"" << kflags.KBIN_MPI_STOP << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      } else {
        kflags.KBIN_MPI_STOP=aurostd::RemoveCharacter(aurostd::substring2string(AflowIn,"[AFLOW_MODE_MPI_MODE]STOP=",1,TRUE),'"');
        aus << "00000  MESSAGE MPI: found STOP=\"" << kflags.KBIN_MPI_STOP << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      }
      // get (string) kflags.KBIN_MPI_COMMAND
      if(!aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]COMMAND=",TRUE)) {
        kflags.KBIN_MPI_COMMAND=MPI_COMMAND_DEFAULT;
        aus << "00000  MESSAGE MPI: COMMAND string is missing, taking COMMAND=\"" << kflags.KBIN_MPI_COMMAND << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      } else {
        kflags.KBIN_MPI_COMMAND=aurostd::RemoveCharacter(aurostd::substring2string(AflowIn,"[AFLOW_MODE_MPI_MODE]COMMAND=",1,TRUE),'"');
        aus << "00000  MESSAGE MPI: found COMMAND=\"" << kflags.KBIN_MPI_COMMAND << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      }
      kflags.KBIN_MPI_AUTOTUNE=aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]AUTOTUNE",TRUE);
      if(kflags.KBIN_MPI_AUTOTUNE) {
        aus << "00000  MESSAGE MPI: found AUTOTUNE option " << Message(__AFLOW_FILE__,aflags) << endl;
        aus << "00000  MESSAGE MPI: input files WILL be auto-tuned for PARALLEL execution with " << kflags.KBIN_MPI_NCPUS << " CPUs "  << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      } else {
        aus << "00000  MESSAGE MPI: AUTOTUNE option NOT found " << Message(__AFLOW_FILE__,aflags) << endl;
        aus << "00000  MESSAGE MPI: input files MUST be appropriate for PARALLEL execution with " << kflags.KBIN_MPI_NCPUS << " CPUs "  << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      }
      // get (string) kflags.KBIN_MPI_BIN
      if(!aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]BINARY=",TRUE)) {
        kflags.KBIN_MPI_BIN=DEFAULT_VASP_MPI_BIN;
        aus << "00000  MESSAGE MPI: BINARY string is missing, taking BIN=\"" << kflags.KBIN_MPI_BIN << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      } else {
        kflags.KBIN_MPI_BIN=aurostd::RemoveCharacter(aurostd::substring2string(AflowIn,"[AFLOW_MODE_MPI_MODE]BINARY=",1,TRUE),'"');
        aus << "00000  MESSAGE MPI: found BINARY=\"" << kflags.KBIN_MPI_BIN << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      }
      //ME20190107 - Grab the serial binary to propagate into child aflow.in files
      if (aurostd::substring2bool(AflowIn, "[AFLOW_MODE_BINARY]")) {
        kflags.KBIN_SERIAL_BIN = aurostd::substring2string(AflowIn, "[AFLOW_MODE_BINARY]");
      } else if (aurostd::substring2bool(AflowIn, "[AFLOW_MODE_BINARY=")) {
        kflags.KBIN_SERIAL_BIN = aurostd::RemoveCharacter(aurostd::substring2string(AflowIn, "[AFLOW_MODE_BINARY="), ']');
      }
      aus << "00000  MESSAGE MPI: Overriding BINARY=\"" << kflags.KBIN_BIN << "\" to BINARY =\"" << kflags.KBIN_MPI_BIN << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
      aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      kflags.KBIN_BIN=kflags.KBIN_MPI_BIN;

      // get (string) kflags.KBIN_MPI_OPTIONS
      if(!aurostd::substring2bool(AflowIn,"[AFLOW_MODE_MPI_MODE]OPTIONS=",TRUE)) {
        kflags.KBIN_MPI_OPTIONS=VASP_OPTIONS_MPI_DEFAULT;
        aus << "00000  MESSAGE MPI: OPTIONS string is missing, taking OPTIONS=\"" << kflags.KBIN_MPI_OPTIONS << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      } else {
        kflags.KBIN_MPI_OPTIONS=aurostd::RemoveCharacter(aurostd::substring2string(AflowIn,"[AFLOW_MODE_MPI_MODE]OPTIONS=",1,TRUE),'"');
        aus << "00000  MESSAGE MPI: found OPTIONS=\"" << kflags.KBIN_MPI_OPTIONS << "\"  " << Message(__AFLOW_FILE__,aflags) << endl;
        aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
      }
    }
    if(XHOST.vflag_control.flag("MPI_HOSTLIST_OFFSETS")){ //CO20231201
      kflags.KBIN_MPI_HOSTLIST_OFFSETS=XHOST.vflag_control.getattachedutype<int>("MPI_HOSTLIST_OFFSETS"); //CO20231201
      aus << "00000  MESSAGE MPI: MPI_HOSTLIST_OFFSETS option found; KBIN_MPI_HOSTLIST_OFFSETS=" << kflags.KBIN_MPI_HOSTLIST_OFFSETS << Message(__AFLOW_FILE__,aflags) << endl;
      aurostd::PrintMessageStream(FileMESSAGE,aus,XHOST.QUIET);    
    }
  }
} // namespace KBIN


// ***************************************************************************
// KBIN::StartStopCheck
// ***************************************************************************
namespace KBIN {
  void StartStopCheck(const string &AflowIn,string str1,string str2,bool &flag,bool &flagS) {
    flag =
      aurostd::substring2bool(AflowIn,str1) || aurostd::substring2bool(AflowIn,str2) ;
    flagS= (aurostd::substring2bool(AflowIn,str1+"START") && aurostd::substring2bool(AflowIn,str1+"STOP")) ||
      (aurostd::substring2bool(AflowIn,str2+"_START") && aurostd::substring2bool(AflowIn,str2+"_STOP"));
    if(flagS) flag=FALSE;
  }
}

namespace KBIN {
  void StartStopCheck(const string &AflowIn,string str1,bool &flag,bool &flagS) {
    flag = aurostd::substring2bool(AflowIn,str1);
    flagS= aurostd::substring2bool(AflowIn,str1+"START") && aurostd::substring2bool(AflowIn,str1+"STOP");
    if(flagS) flag=FALSE;
  }
}

// ***************************************************************************
// KBIN::MoveRun2NewDirectory() //DX20210901
// ***************************************************************************
// Moves an aflow run (i.e., aflow.in) to a new directory and adds a LOCK
// to the original directory to prevent aflow from re-running the system.
// Some machines have scrubbers on certain filesystems/workspaces that remove
// old/untouched files, i.e., removes/deletes aflow.ins that are waiting to be
// run. To avoid this, we store the aflow.in in a filesystem/workspace "safe"
// from the scrubber, and move the aflow.in to the "run" filesystem/workspace
// once it has been selected from the aflow daemon.
// This is needed for machine001, machine002, machine003
// SD20220319 - changed function to return a bool
namespace KBIN {
  bool MoveRun2NewDirectory(_aflags& aflags, const string& subdirectory_orig, const string& subdirectory_new){

    bool LDEBUG=(FALSE || XHOST.DEBUG);
    ostringstream aus,message;
    //[NOT NEEDED, ALREADY CHECKED IN Legitimate_aflowin()]if(aurostd::DirectoryLocked(aflags.Directory,_AFLOWLOCK_)){return FALSE;} //CO20250101 - helps with AEL/AGL, we have LOCK.agl but don't want LOCK.agl.init

    // ---------------------------------------------------------------------------
    // create lock immediately // CO20210901
    //[SD20220224 - OBSOLETE]aus << "touch " << aflags.Directory << "/" << _AFLOWLOCK_;
    if(!aurostd::LinkFileAtomic(aflags.Directory+"/"+_AFLOWIN_,aflags.Directory+"/"+_AFLOWLOCK_+_LOCK_LINK_SUFFIX_,false)){return FALSE;} // create LOCK link //SD20220224
    //[CO20221024 - OBSOLETE as above]message <<    "MMMMM  Executing: \"" << aus.str() << "\"" << Message(__AFLOW_FILE__,aflags,"user,host,time") << endl;aurostd::PrintMessageStream(message,XHOST.QUIET);message.clear();message.str(std::string());
    //[CO20221024 - OBSOLETE as above]aurostd::execute(aus);
    //[CO20221024 - OBSOLETE as above]aus.clear();aus.str(std::string());
    if(!aurostd::FileExist(aflags.Directory+"/"+_AFLOWIN_+".ORIG")){aurostd::CopyFile(aflags.Directory+"/"+_AFLOWIN_,aflags.Directory+"/"+_AFLOWIN_+".ORIG");}  //CO20231127 - keep original aflow.in in the directory so it's easy to find
    aurostd::StringstreamClean(aus);

    // ---------------------------------------------------------------------------
    //Changing the run directory from the "original" to a "new" directory
    string directory_orig = aflags.Directory;
    if(LDEBUG){
      cerr << __AFLOW_FUNC__ << " original full directory " << directory_orig << endl;
      cerr << __AFLOW_FUNC__ << " changing subdirectory " << subdirectory_orig << " to " << subdirectory_new << endl;
    }
    aurostd::StringSubst(aflags.Directory,subdirectory_orig,subdirectory_new);

    // ---------------------------------------------------------------------------
    // make new directory
    aus << "mkdir -p " << aflags.Directory;
    message <<    "MMMMM  Executing: \"" << aus.str() << "\"" << Message(__AFLOW_FILE__,aflags,"user,host,time") << endl;aurostd::PrintMessageStream(message,XHOST.QUIET);message.clear();message.str(std::string());aurostd::execute(aus);aus.clear();aus.str(std::string());

    // ---------------------------------------------------------------------------
    // copy aflow.in to new directory
    aus << "cp " << aurostd::CleanFileName(directory_orig+"/"+_AFLOWIN_) << " " << aflags.Directory;
    message <<    "MMMMM  Executing: \"" << aus.str() << "\"" << Message(__AFLOW_FILE__,aflags,"user,host,time") << endl;aurostd::PrintMessageStream(message,XHOST.QUIET);message.clear();message.str(std::string());aurostd::execute(aus);aus.clear();aus.str(std::string());

    // ---------------------------------------------------------------------------
    //CO20261006 - the kept files of an earlier job (KBIN::VASP_RestartPackName()) go with it; KBIN::VASP_ResumePrepare() unpacks them.
    //The data and work folders are on different file systems: the archive is copied under a hidden name, renamed, and only
    //then removed from the data folder, so a job killed on the way leaves the data folder's copy whole.
    const string pack=KBIN::VASP_RestartPackName(_AFLOWIN_);
    if(aurostd::FileExist(directory_orig+"/"+pack)){
      message << "MMMMM  Moving " << pack << " to " << aflags.Directory << Message(__AFLOW_FILE__,aflags,"user,host,time") << endl;aurostd::PrintMessageStream(message,XHOST.QUIET);message.clear();message.str(std::string());
      const string moving=aflags.Directory+"/."+pack+".moving";
      aurostd::CopyFile(directory_orig+"/"+pack,moving);
      if(aurostd::FileExist(moving) && aurostd::FileSize(moving)==aurostd::FileSize(directory_orig+"/"+pack) &&
          std::rename(moving.c_str(),(aflags.Directory+"/"+pack).c_str())==0){
        aurostd::RemoveFile(directory_orig+"/"+pack);
      }
      else{
        if(aurostd::FileExist(moving)){aurostd::RemoveFile(moving);}
        message << "WWWWW  WARNING: " << pack << " could not be moved to " << aflags.Directory << "; it stays in " << directory_orig << Message(__AFLOW_FILE__,aflags,"user,host,time") << endl;aurostd::PrintWarningStream(message,XHOST.QUIET);message.clear();message.str(std::string());
      }
    }

    if(LDEBUG){ cerr << __AFLOW_FUNC__ << " new full directory " << aflags.Directory << endl; }
    return TRUE;
  }
}

// ***************************************************************************
// KBIN::RUN_Directory
// ***************************************************************************
namespace KBIN {
  void RUN_Directory(_aflags& aflags) {        // AFLOW_FUNCTION_IMPLEMENTATION
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    ostringstream aus;

    // ---------------------------------------------------------------------------
    // Move aflow run (i.e., aflow.in) to a new directory and add a LOCK
    // to the original directory to prevent machine scrubbers from removing
    // DX20210901
    if(aflags.AFLOW_PERFORM_FILE && ( //CO20230528 - only run in a different directory if aflags.AFLOW_PERFORM_FILE, we should be benchmarking non-HT
        aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE001") ||
        aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE002") ||
        aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE003") ||
        aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE004") ||
        aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE101") ||  //CO20230512
        aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE102") ||  //CO20230512
        aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE103") ||  //CO20221027
        FALSE)
      ){
      //CO20221027 - $HOME and $WORKDIR are for very specific environments
      //$HOME_AFLOW and $WORK_HOME can be generalized for any system
      string subdirectory_orig = aurostd::getenv2string("HOME_AFLOW");   // $HOME    : environment variable pointing to "home" filesystem (specific to machine001/002/003/004)
      string subdirectory_new = aurostd::getenv2string("WORK_AFLOW"); // $WORKDIR : environment variable pointing to "work" filesystem (specific to machine001/002/003/004)
      if((subdirectory_orig.empty() && subdirectory_new.empty()) && ( //CO20230528 - if HOME_XXX env variables not already found
          aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE001") ||
          aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE002") ||
          aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE003") ||
          aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE004") ||
          aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE101") ||  //CO20230512
          aflags.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE102") ||  //CO20230512
          FALSE)
          ){
        subdirectory_orig = aurostd::getenv2string("HOME");   // $HOME    : environment variable pointing to "home" filesystem (specific to machine001/002/003/004)
        subdirectory_new = aurostd::getenv2string("WORKDIR"); // $WORKDIR : environment variable pointing to "work" filesystem (specific to machine001/002/003/004)
      }
      if(subdirectory_orig.empty()||subdirectory_new.empty()){return;}  //CO20221027
      if(!KBIN::MoveRun2NewDirectory(aflags, subdirectory_orig, subdirectory_new)){return;}
    }

    //[SD20220224 - OBSOLETE]ifstream FileSUBDIR;string FileNameSUBDIR;
    //[SD20220224 - OBSOLETE]FileNameSUBDIR=aflags.Directory;
    //[SD20220224 - OBSOLETE]FileSUBDIR.open(FileNameSUBDIR.c_str(),std::ios::in);
    //[SD20220224 - OBSOLETE]FileSUBDIR.clear();FileSUBDIR.close();
    // string::size_type sub_size1,sub_size2;
    string AflowIn,AflowInMode,subS,subS1,subS2;
    //[CO20230626 - clang warning not used]string::iterator pos;
    bool Krun=TRUE;
    //  int i;

    if(LDEBUG){cerr << __AFLOW_FUNC__ << " BEGIN" << endl;}

    if(aflags.Directory.empty() || (aflags.Directory[0]!='/' && aflags.Directory[0]!='.' && aflags.Directory[0]!=' ')){aflags.Directory="./"+aflags.Directory;}

    //[SD20220224 - OBSOLETE]if(!FileSUBDIR) {                                                                                           // ******* Directory is non existent
    //[SD20220224 - OBSOLETE]  aus << "EEEEE  DIRECTORY_NOT_FOUND = "  << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]  aurostd::PrintMessageStream(aus,XHOST.QUIET);
    //[SD20220224 - OBSOLETE]} else //CO20220830 - removing opening bracket for indenter
    //[SD20220224 - OBSOLETE]  // ***************************************************************************
    //[SD20220224 - OBSOLETE]  // Check LOCK again
    //[SD20220224 - OBSOLETE]  if(aurostd::DirectoryLocked(aflags.Directory,_AFLOWLOCK_) || DirectorySkipped(aflags.Directory) || DirectoryAlreadyInDatabase(aflags.Directory,aflags.AFLOW_FORCE_RUN) || DirectoryUnwritable(aflags.Directory)) //CO20220830 - removing opening bracket for indenter
    //[SD20220224 - OBSOLETE]    // ******* Directory is locked/skipped/unwritable
    //[SD20220224 - OBSOLETE]    // LOCK/SKIP/UNWRITABLE exist, then RUN already RUN
    //[SD20220224 - OBSOLETE]    if(aurostd::DirectoryLocked(aflags.Directory,_AFLOWLOCK_)) {
    //[SD20220224 - OBSOLETE]      aus << "LLLLL  LOCKED ... bzzz ... KBIN::RUN_Directory "  << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]      aus << "LLLLL  LOCKED ... Probably other aflows are concurring with this. KBIN::RUN_Directory " << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]      aurostd::PrintMessageStream(aus,XHOST.QUIET);
    //[SD20220224 - OBSOLETE]    }
    //[SD20220224 - OBSOLETE]    if(DirectorySkipped(aflags.Directory)) {
    //[SD20220224 - OBSOLETE]      aus << "LLLLL  SKIPPED ... bzzz ... KBIN::RUN_Directory "  << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]      aus << "LLLLL  SKIPPED ... Probably other aflows are concurring with this. KBIN::RUN_Directory " << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]      aurostd::PrintMessageStream(aus,XHOST.QUIET);
    //[SD20220224 - OBSOLETE]    }
    //[SD20220224 - OBSOLETE]    if(DirectoryAlreadyInDatabase(aflags.Directory,aflags.AFLOW_FORCE_RUN)) {
    //[SD20220224 - OBSOLETE]      aus << "LLLLL  ALREADY_IN_DATABASE ... bzzz ... KBIN::RUN_Directory "  << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]      aus << "LLLLL  ALREADY_IN_DATABASE ... use \"aflow --multi\" to force the calculation of this entry "  << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]      aus << "LLLLL  ALREADY_IN_DATABASE ... Probably other aflows are concurring with this. KBIN::RUN_Directory " << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]      aurostd::PrintMessageStream(aus,XHOST.QUIET);
    //[SD20220224 - OBSOLETE]    }
    //[SD20220224 - OBSOLETE]    if(DirectoryUnwritable(aflags.Directory)) {
    //[SD20220224 - OBSOLETE]      aus << "LLLLL  UNWRITABLE ... bzzz ... KBIN::RUN_Directory "  << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]      aus << "LLLLL  UNWRITABLE ... Probably other aflows are concurring with this. KBIN::RUN_Directory " << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]      aurostd::PrintMessageStream(aus,XHOST.QUIET);
    //[SD20220224 - OBSOLETE]    }
    if(!Legitimate_aflowdir(aflags.Directory,aflags) || !aurostd::LinkFileAtomic(aflags.Directory+"/"+_AFLOWIN_,aflags.Directory+"/"+_AFLOWLOCK_+_LOCK_LINK_SUFFIX_,false)){return;} // create LOCK link //SD20220224
    // SD20220224 - Dumb LOCK is now made with LinkFileAtomic, also we do not need to check if _AFLOWIN_ exists
    // because LinkFileAtomic will throw an error if that is the case
    //[SD20220224 - OBSOLETE]// make a dumb lock as soon as possible -------------------------------------
    //[SD20220224 - OBSOLETE]aus.clear();aus.str(std::string());
    //[SD20220224 - OBSOLETE]aus << "echo \"NNNNN  KBIN LOCK ASAP for NFS concurrent jobs (aflow" << string(AFLOW_VERSION) << ")\" >> " << aflags.Directory+"/"+_AFLOWLOCK_ << endl;
    //[SD20220224 - OBSOLETE]// aus << "/home/auro/bin/aflow -machine >> " << aflags.Directory+"/"+_AFLOWLOCK_ << endl;
    //[SD20220224 - OBSOLETE]// aus << XHOST.command("sensors") << " >> " << aflags.Directory+"/"+_AFLOWLOCK_ << endl;
    //[SD20220224 - OBSOLETE]aurostd::execute(aus);
    //[SD20220224 - OBSOLETE]// now change its permission
    //[SD20220224 - OBSOLETE]aurostd::ChmodFile("664",string(aflags.Directory+"/"+_AFLOWLOCK_));
    //[SD20220224 - OBSOLETE]// now the lock should be done ----------------------------------------------
    //[SD20220224 - OBSOLETE]ifstream FileAFLOWIN;string FileNameAFLOWIN;
    //[SD20220224 - OBSOLETE]FileNameAFLOWIN=aflags.Directory+"/"+_AFLOWIN_;
    //[SD20220224 - OBSOLETE]FileAFLOWIN.open(FileNameAFLOWIN.c_str(),std::ios::in);
    //[SD20220224 - OBSOLETE]if(!FileAFLOWIN) {                                                                                      // ******* _AFLOWIN_ does not exist
    //[SD20220224 - OBSOLETE]  aus << "EEEEE  " << _AFLOWIN_ << " ABSENT   = "  << Message(__AFLOW_FILE__,aflags) << endl;
    //[SD20220224 - OBSOLETE]  aurostd::PrintMessageStream(aus,XHOST.QUIET);
    //[SD20220224 - OBSOLETE]  FileAFLOWIN.clear();FileAFLOWIN.close();
    //[SD20220224 - OBSOLETE]  return false;
    //[SD20220224 - OBSOLETE]}
    // ***************************************************************************
    // RESET LOCK
    ofstream FileLOCK;
    string FileNameLOCK=aflags.Directory+"/"+_AFLOWLOCK_;
    //	FileLOCK.open(FileNameLOCK.c_str(),std::ios::out);
    FileLOCK.open(FileNameLOCK.c_str(),std::ios::app);
    // ***************************************************************************
    // WRITE LOCK
    //[CO20240608 - OBSOLETE with Banner()]if(0) {
    //[CO20240608 - OBSOLETE with Banner()]  aus <<    "MMMMM  AFLOW VERSION " << string(AFLOW_VERSION) << " Automatic-Flow - " << Message(__AFLOW_FILE__,aflags) << endl;
    //[CO20240608 - OBSOLETE with Banner()]  aus <<    "MMMMM  (C) "<<XHOST.Copyright_Years<<", Stefano Curtarolo - Duke University   - " << Message(__AFLOW_FILE__,aflags) << endl;
    //[CO20240608 - OBSOLETE with Banner()]  aus <<    "MMMMM  High-Throughput ab-initio Computing - " << Message(__AFLOW_FILE__,aflags) << endl;
    //[CO20240608 - OBSOLETE with Banner()]  aurostd::PrintMessageStream(FileLOCK,aus,XHOST.QUIET);
    //[CO20240608 - OBSOLETE with Banner()]  // ***************************************************************************
    //[CO20240608 - OBSOLETE with Banner()]  // WRITE AFLOW VERSION
    //[CO20240608 - OBSOLETE with Banner()]  // aus << "MMMMM  AFLOW VERSION " << string(AFLOW_VERSION) << " Automatic-Flow " << Message(__AFLOW_FILE__,aflags) << endl;
    //[CO20240608 - OBSOLETE with Banner()]  // aurostd::PrintMessageStream(FileLOCK,aus,XHOST.QUIET);
    //[CO20240608 - OBSOLETE with Banner()]}
    // ***************************************************************************
    // START DIRECTORY
    //[CO20240612 - very verbose, not necessary]aus      << aflow::Banner("BANNER_NORMAL") << endl;
    aus      << "XXXXX  KBIN DIRECTORY BEGIN (aflow" << string(AFLOW_VERSION) << ")  "  << Message(__AFLOW_FILE__,aflags) << endl;
    aus      << "XXXXX  KBIN AFLOWIN=" << _AFLOWIN_ << endl;  //CO20240608 - for monitor to read
    aus      << "XXXXX  KBIN AFLOWLOCK=" << _AFLOWLOCK_ << endl;  //CO20240608 - for monitor to read
    aus      << "XXXXX  KBIN AFLOWENDOUT=" << _AFLOWENDOUT_ << endl;  //CO20240608 - for monitor to read
    //	aurostd::PrintMessageStream(FileLOCK,aus,XHOST.QUIET);
    aus      << "XXXXX  KBIN XHOST.CPU_Model : "<<  XHOST.CPU_Model << "" << endl;// << Message(__AFLOW_FILE__,aflags) << endl;
    aus      << "XXXXX  KBIN XHOST.CPU_Cores : "<<  XHOST.CPU_Cores << "" << endl;// << Message(__AFLOW_FILE__,aflags) << endl;
    aus      << "XXXXX  KBIN XHOST.CPU_MHz   : "<<  XHOST.CPU_MHz << "" << endl;// << Message(__AFLOW_FILE__,aflags) << endl;
    aus      << "XXXXX  KBIN XHOST.RAM_GB    : "<<  XHOST.RAM_GB << "" << endl;// << Message(__AFLOW_FILE__,aflags) << endl;
    aurostd::PrintMessageStream(FileLOCK,aus,XHOST.QUIET);
    // ***************************************************************************
    // FLUSH & REOPEN to avoid double writing
    FileLOCK.flush();FileLOCK.close();FileLOCK.open(FileNameLOCK.c_str(),std::ios::app);
    aurostd::UnlinkFile(aflags.Directory+"/"+_AFLOWLOCK_+_LOCK_LINK_SUFFIX_); // Remove LOCK link //SD20220207
    // ***************************************************************************
    // NOW Digest AFLOWIN
    //[SD20220224 - OBSOLETE]FileAFLOWIN.clear();FileAFLOWIN.seekg(0);
    ifstream FileAFLOWIN;
    string FileNameAFLOWIN=aflags.Directory+"/"+_AFLOWIN_; //SD20220224
    FileAFLOWIN.open(FileNameAFLOWIN.c_str(),std::ios::in); //SD20220224
    //DX20190125 [OBSOLETE] - need to remove null bytes : AflowIn.clear();char c; while (FileAFLOWIN.get(c)) AflowIn+=c;               // READ _AFLOWIN_ and put into AflowIn
    AflowIn.clear();char c; while (FileAFLOWIN.get(c)) if(c!='\0'){AflowIn+=c;}               // READ _AFLOWIN_ and put into AflowIn //DX20190125 - remove null bytes from AflowIn
    FileAFLOWIN.clear();FileAFLOWIN.seekg(0);
    AflowIn=aurostd::RemoveComments(AflowIn); // NOW Clean AFLOWIN
    vector<string> vAflowIn;aurostd::string2vectorstring(AflowIn,vAflowIn); //CO20181226

    _kflags kflags=KBIN::VASP_Get_Kflags_from_AflowIN(AflowIn,FileLOCK,aflags); //CO20200624 - made separate function for getting kflags
    
    // ***************************************************************************
    // FINALIZE LOCK
    aus << "XXXXX  KBIN DIRECTORY END (aflow" << string(AFLOW_VERSION) << ")  "  << Message(__AFLOW_FILE__,aflags) << endl;
    aurostd::PrintMessageStream(FileLOCK,aus,XHOST.QUIET);
    // ***************************************************************************
    // PREPARE MESSAGE FOR LOG TO BE INTERCEPTED IN COMPRESSION
    aus << "XXXXX  KBIN_DIRECTORY_END " << aflags.Directory << endl;
    aurostd::PrintMessageStream(FileLOCK,aus,XHOST.QUIET);
    // ***************************************************************************

    if(kflags.AFLOW_MODE_PRESCRIPT_EXPLICIT || kflags.AFLOW_MODE_PRESCRIPT_EXPLICIT_START_STOP) {  // [AFLOW_MODE_PRESCRIPT] construction
      aurostd::stringstream2file(kflags.AFLOW_MODE_PRESCRIPT,string(aflags.Directory+"/"+DEFAULT_AFLOW_PRESCRIPT_COMMAND));
      aurostd::ChmodFile("755",string(aflags.Directory+"/"+DEFAULT_AFLOW_PRESCRIPT_COMMAND));
    }
    if(kflags.AFLOW_MODE_POSTSCRIPT_EXPLICIT || kflags.AFLOW_MODE_POSTSCRIPT_EXPLICIT_START_STOP) {  // [AFLOW_MODE_POSTSCRIPT] construction
      aurostd::stringstream2file(kflags.AFLOW_MODE_POSTSCRIPT,string(aflags.Directory+"/"+DEFAULT_AFLOW_POSTSCRIPT_COMMAND));
      aurostd::ChmodFile("755",string(aflags.Directory+"/"+DEFAULT_AFLOW_POSTSCRIPT_COMMAND));
    }
    // ************************************************************************************************************************************
    // ALIEN MODE
    if(Krun && kflags.AFLOW_MODE_ALIEN) {
      // ***************************************************************************
      // ALIEN MODE  // must contain EMAIL perform
      if(Krun) {
        Krun=(Krun && ALIEN::Run_Directory(FileLOCK,aflags,kflags));
      }
      // ***************************************************************************
      // COMPRESS
      if(Krun && kflags.KZIP_COMPRESS) {
        Krun=(Krun && KBIN::CompressDirectory(aflags,kflags));
      }
    }
    // ************************************************************************************************************************************
    // MATLAB MODE
    if(Krun && kflags.AFLOW_MODE_MATLAB) {
      if(Krun) {
        aurostd::CommandRequired(DEFAULT_KBIN_MATLAB_BIN); // MATLAB MUST BE AVAILABLE
        Krun=(Krun && KBIN_MATLAB_Directory(FileLOCK,aflags,kflags));
      }
      // ***************************************************************************
      // COMPRESS
      if(Krun && kflags.KZIP_COMPRESS)
        Krun=(Krun && KBIN::CompressDirectory(aflags,kflags));
      Krun=FALSE;
    }
    // ************************************************************************************************************************************
    // AIMS MODE
    if(Krun && kflags.AFLOW_MODE_AIMS) {
      // ***************************************************************************
      // AIMS MODE  // must contain EMAIL perform
      if(Krun) {
        Krun=(Krun && KBIN::AIMS_Directory(FileLOCK,aflags,kflags));
      }
      // ***************************************************************************
      // COMPRESS
      if(Krun && kflags.KZIP_COMPRESS)
        Krun=(Krun && KBIN::CompressDirectory(aflags,kflags));
    }
    // ************************************************************************************************************************************
    // ************************************************************************************************************************************
    // VASP MODE
    if(Krun && kflags.AFLOW_MODE_VASP) {
      // ***************************************************************************
      // VASP MODE  // must contain EMAIL perform
      if(Krun) {
        Krun=(Krun && KBIN::VASP_Directory(FileLOCK,aflags,kflags));
      }
      // ***************************************************************************
      // COMPRESS	    
      if(Krun && kflags.KZIP_COMPRESS) {
        // cerr << aurostd::execute2string("ls -las "+aflags.Directory) << endl;
        Krun=(Krun && KBIN::CompressDirectory(aflags,kflags));
        // cerr << aurostd::execute2string("ls -las "+aflags.Directory) << endl;
      }
    }
    // ************************************************************************************************************************************
    // MATLAB MODE
    if(Krun && kflags.KBIN_PHONONS_CALCULATION_FROZSL && !kflags.AFLOW_MODE_VASP) {
      // PRESCRIPT
      if(Krun && (kflags.AFLOW_MODE_PRESCRIPT_EXPLICIT || kflags.AFLOW_MODE_PRESCRIPT_EXPLICIT_START_STOP))
        KBIN::RUN_DirectoryScript(aflags,DEFAULT_AFLOW_PRESCRIPT_COMMAND,DEFAULT_AFLOW_PRESCRIPT_OUT);
      // POSTSCRIPT
      if(Krun && (kflags.AFLOW_MODE_POSTSCRIPT_EXPLICIT || kflags.AFLOW_MODE_POSTSCRIPT_EXPLICIT_START_STOP))
        KBIN::RUN_DirectoryScript(aflags,DEFAULT_AFLOW_POSTSCRIPT_COMMAND,DEFAULT_AFLOW_POSTSCRIPT_OUT);
    }
    // ***************************************************************************
    // FINALIZE AFLOWIN
    // DONE, turn OFF the flag
    kflags.AFLOW_MODE_VASP=FALSE;    
    // ***************************************************************************
    // NFS cache-cleaning HACK, opendir() and closedir() to invalidate cache
    // https://stackoverflow.com/questions/8311710/nfs-cache-cleaning-command
    //CO20171106
    vector<string> vfiles_dummyls;
    aurostd::DirectoryLS(aflags.Directory,vfiles_dummyls);
    // ***************************************************************************
    // WRITE END
    aurostd::string2file("AFLOW calculation complete"+string(Message(__AFLOW_FILE__,aflags)+"\n"),string(aflags.Directory+"/"+_AFLOWENDOUT_));
    // ***************************************************************************
    // MAKE READEABLE
    aurostd::ChmodFile("664",string(aflags.Directory+"/"+_AFLOWLOCK_));
    aus.clear();aus.str(std::string());
    aus << "cp " << aflags.Directory << "/" << _AFLOWLOCK_ << " " << aflags.Directory << "/" << _AFLOWLOCK_ << "." << string(AFLOW_VERSION) << endl;  
    aurostd::execute(aus);  
    aurostd::ChmodFile("664",string(aflags.Directory+"/*"));
    aurostd::ChmodFile("777",string(aflags.Directory+"/*"));

    FileAFLOWIN.clear();FileAFLOWIN.close();

  };
} // namespace

// *******************************************************************************************
namespace KBIN {
  void AFLOW_RUN_Directory(const _aflags& aflags) {
    aurostd::execute(XHOST.command("aflow")+" --run=1 --DIRECTORY="+aflags.Directory);  // run it OUTSIDE
    // this is cool as if the particula program explodes, there is still aflow running
  }
} // namespace

// *******************************************************************************************
namespace KBIN {
  void RUN_DirectoryScript(const _aflags& aflags,const string& script,const string& output) {        // AFLOW_FUNCTION_IMPLEMENTATION
    ostringstream aus;
    aurostd::StringstreamClean(aus);
    aus << "cd " << aflags.Directory << endl;
    aus <<  "chmod 755 " << script << endl;
    aus << "./" << script << " >> " << output << endl;
    aurostd::execute(aus);
  }
} // namespace

// *******************************************************************************************
namespace KBIN {
  //ME20210927 Added string variant
  bool CompressDirectory(const _aflags& aflags,const _kflags& kflags) {        // AFLOW_FUNCTION_IMPLEMENTATION
    return CompressDirectory(aflags.Directory, kflags);
  }

  bool CompressDirectory(const string& directory, const _kflags& kflags) {
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    if(LDEBUG){cerr << __AFLOW_FUNC__ << " BEGIN" << endl;}
    //DX+CO START
    // [OBSOLETE] aus << "cd " << aflags.Directory << " && ";
    // [OBSOLETE]  aus << "ls | grep -v .EXT | ";                                //CO, skip anything with bzip extension
    // [OBSOLETE]  aus << "grep -v LOCK | grep -v " << _AFLOWLOCK_ << " | ";     //CO never zip LOCK or .LOCK (agl.LOCK) newly defined LOCK
    // [OBSOLETE] aus << "grep -v SKIP | ";
    // [OBSOLETE] aus << "grep -v " << KBIN_SUBDIRECTORIES << " | ";
    // [OBSOLETE]  aus << "grep -v aflow.in | grep -v " << _AFLOWIN_ << " ";;    //CO, never zip aflow.in or _aflow.in (agl_aflow.in) or newly defined aflow.in
    if(LDEBUG){cerr << __AFLOW_FUNC__ << " directory=" << directory << endl;}
    vector<string> _vfiles,vfiles;
    string compressed_variant="";
    aurostd::DirectoryLS(directory,_vfiles);
    if(LDEBUG){cerr << __AFLOW_FUNC__ << "_vfiles=" << aurostd::joinWDelimiter(_vfiles,",") << endl;}
    string file_path="";
    uint i=0;
    for(i=0;i<_vfiles.size();i++){
      if(aurostd::IsCompressed(_vfiles[i])){continue;}  //doesn't need full path, just substring2bool for zip variants, e.g., .EXT
      if(_vfiles[i].size() && _vfiles[i][0]=='.'){continue;}  //do not try to compress any hidden files, .nfs stuff is particularly problematic to compress
      if(aurostd::substring2bool(_vfiles[i],KBIN_SUBDIRECTORIES)){continue;}
      if(aurostd::substring2bool(_vfiles[i],"LOCK")){continue;}
      if(aurostd::substring2bool(_vfiles[i],_AFLOWLOCK_)){continue;}
      if(aurostd::substring2bool(_vfiles[i],"SKIP")){continue;}
      if(aurostd::substring2bool(_vfiles[i],"aflow.in")){continue;}
      if(aurostd::substring2bool(_vfiles[i],"aflow_") &&
          aurostd::substring2bool(_vfiles[i],".in")){continue;} //AS20201023 do not compress files like aflow_qha.in
      if(aurostd::substring2bool(_vfiles[i],_AFLOWENDOUT_) || aurostd::substring2bool(_vfiles[i],"aflow.end.out")){continue;}  //CO20170613, file is special because it gets written after compression
      if(aurostd::substring2bool(_vfiles[i],_AFLOWIN_)){continue;}
      file_path=directory + "/" + _vfiles[i];
      if(LDEBUG) {cerr << __AFLOW_FUNC__ << " file_path=" << file_path << endl;}
      if(aurostd::IsDirectory(file_path)){continue;}  //compress files only
      // [OBSOLETE]  if(aurostd::EFileExist(file_path,compressed_variant)){ //SC20200408
      // [OBSOLETE]  //need full path here, also, notice the placement here, actual compressed variant would have been skipped, this is for the uncompressed variant //SC20200408
      // [OBSOLETE]  //both compressed and uncompressed variants exists //SC20200408
      // [OBSOLETE]  //assume compressed is from a previous run, hence obsolete //SC20200408
      // [OBSOLETE]  //delete it before compressing uncompressed variant //SC20200408
      // [OBSOLETE]  aurostd::RemoveFile(compressed_variant);  //  //SC20200408 = no needed because it will overwrite it -f  //SC20200408
      // [OBSOLETE] }
      vfiles.push_back(_vfiles[i]);
    }
    std::sort(vfiles.begin(),vfiles.end()); //sort in order
    if(LDEBUG){cerr << __AFLOW_FUNC__ << " vfiles=" << aurostd::joinWDelimiter(vfiles,",") << endl;}
    // [OBSOLETE] aurostd::string2vectorstring(aurostd::execute2string(aus),vfiles);
    // [OBSOLETE] cerr << vfiles.size() << endl;
    // [OBSOLETE] aurostd::StringstreamClean(aus);
    // [OBSOLETE] cerr << "CO " << aurostd::joinWDelimiter(vfiles," ") << endl;
    if(vfiles.size()){  //ATTEMPT 1 - xargs parallelization with -T4
      //find . -name 'README*' -print0 | xargs -0 -P16 -I{} xz -9fq -T4 {}
      //-T does not work with -9
      //big compression (space) difference between -8 and -9, negligible between -6 and -8
      //big speed up between -6 and -8, not too much between -8 and -9
      uint ncpus_total=(uint)KBIN::get_NCPUS(kflags);
      uint ncpus_xz=4;  //default
      uint nprocessors=(uint) floor(((double)ncpus_total)/((double)ncpus_xz));  //floor is safe
      stringstream aus;
      for(i=0;i<vfiles.size();i++){aus << directory+"/"+vfiles[i] << '\0';} //emulates -print0
      if(LDEBUG){cerr << __AFLOW_FUNC__ << " contents of tmpfile=\"" << aus.str() << "\"" << endl;}
      string tmpfile=aurostd::TmpFileCreate("CompressDirectory");
      aurostd::stringstream2file(aus,tmpfile);
      aurostd::StringstreamClean(aus);
      aus << "cat " << tmpfile << " | xargs -0 -P" << nprocessors << " -I{} " << kflags.KZIP_BIN << " -9fq";
      if(aurostd::substring2bool(kflags.KZIP_BIN,"xz")){aus << " -T" << ncpus_xz;}
      aus << " {}";
      if(LDEBUG){cerr << __AFLOW_FUNC__ << " command=\"" << aus.str() << "\"" << endl;}
      aurostd::execute(aus);
#ifndef _AFLOW_TEMP_PRESERVE_
      aurostd::RemoveFile(tmpfile);
#endif
      //CO20211130 - test if files were successfully compressed
      //if not, try again one by one
      _vfiles.clear();
      for(i=0;i<vfiles.size();i++){ //better than doing it all in one shot
        //it would be good to check if the file is corrupt, how to check is specific to the file
        //do not check if the file is empty, WAVECAR/REPORT are empty
        //-f will overwrite the original compressed file if an uncompressed variant exists
        if(aurostd::EFileExist(directory+"/"+vfiles[i])==true && aurostd::FileExist(directory+"/"+vfiles[i])==false){continue;}
        _vfiles.push_back(vfiles[i]);
      }
      vfiles.clear();
      for(i=0;i<_vfiles.size();i++){vfiles.push_back(_vfiles[i]);}
    }
    if(vfiles.size()){  //ATTEMPT 2 - one-by-one with -T0
      stringstream aus;
      aurostd::StringstreamClean(aus);
      //[CO+ME20211208 - add directory directly to path of file]aus << "cd " << directory << " && " << endl;
      for(i=0;i<vfiles.size();i++){ //better than doing it all in one shot
        aus << kflags.KZIP_BIN << " -9fq " << (aurostd::substring2bool(kflags.KZIP_BIN,"xz")?"-T0 ":"") << directory << "/" << vfiles[i] << "; " << endl;  // semi-colon is important, keeps going if it stalls on one //CO20211130 - added q
      }
      if(LDEBUG){cerr << __AFLOW_FUNC__ << " command=\"" << aus.str() << "\"" << endl;}
      aurostd::execute(aus);
      //CO20211130 - test if files were successfully compressed
      //if not, try again one by one
      _vfiles.clear();
      for(i=0;i<vfiles.size();i++){ //better than doing it all in one shot
        //it would be good to check if the file is corrupt, how to check is specific to the file
        //do not check if the file is empty, WAVECAR/REPORT are empty
        //-f will overwrite the original compressed file if an uncompressed variant exists
        if(aurostd::EFileExist(directory+"/"+vfiles[i])==true && aurostd::FileExist(directory+"/"+vfiles[i])==false){continue;}
        _vfiles.push_back(vfiles[i]);
      }
      vfiles.clear();
      for(i=0;i<_vfiles.size();i++){vfiles.push_back(_vfiles[i]);}
    }
    if(vfiles.size()){  //ATTEMPT 3 - one-by-one
      stringstream aus;
      aurostd::StringstreamClean(aus);
      //[CO+ME20211208 - add directory directly to path of file]aus << "cd " << directory << " && " << endl;
      for(uint i=0;i<vfiles.size();i++){ //better than doing it all in one shot
        aus << kflags.KZIP_BIN << " -9fq " << directory << "/" << vfiles[i] << "; " << endl;  // semi-colon is important, keeps going if it stalls on one //CO20211130 - added q
      }
      if(LDEBUG){cerr << __AFLOW_FUNC__ << " command=\"" << aus.str() << "\"" << endl;}
      // aus << kflags.KZIP_BIN << " " << aurostd::joinWDelimiter(vfiles," ") << endl; //AVOID, because if one fails, the whole command stops
      aurostd::execute(aus);
      // cerr << aus.str() << endl;
    }
    // [OBSOLETE] aus << kflags.KZIP_BIN << " `find . " << XHOST.Find_Parameters << " -name \"*\" | grep -v LOCK | grep -v SKIP | grep \"./\"` " << endl;
    // [OBSOLETE] aus << kflags.KZIP_BIN << " `ls | grep -v LOCK | grep -v SKIP | grep -v " << KBIN_SUBDIRECTORIES << "| grep -v " << _AFLOWIN_ << " | grep -v apl.xml ` " << endl;
    // [OBSOLETE] apl.xml can now be zipped
    // [OBSOLETE] set up is slightly redundant (LOCK vs. agl.LOCK), but very safe
    // [OBSOLETE] aus << kflags.KZIP_BIN << " `ls | grep -v .EXT | ";                 //CO, skip anything with bzip extension
    // [OBSOLETE] aus << "grep -v LOCK | grep -v " << _AFLOWLOCK_ << " | ";           //CO never zip LOCK or .LOCK (agl.LOCK) newly defined LOCK
    // [OBSOLETE] aus << "grep -v SKIP | ";
    // [OBSOLETE] aus << "grep -v " << KBIN_SUBDIRECTORIES << " | ";
    // [OBSOLETE] aus << "grep -v aflow.in | grep -v " << _AFLOWIN_ << " ` " << endl; //CO, never zip aflow.in or _aflow.in (agl_aflow.in) or newly defined aflow.in
    // [OBSOLETE] cerr << aus.str() << endl;
    // [OBSOLETE] aurostd::execute(aus);
    //DX+CO END
    return TRUE;
  }
}

// *******************************************************************************************
namespace KBIN {
  bool CompressDirectory(const _aflags& aflags) {        // AFLOW_FUNCTION_IMPLEMENTATION
    _kflags kflags;
    kflags.KZIP_BIN=DEFAULT_KZIP_BIN;   //[CO20211130, moving -9q above as default]+" -9q";
    return KBIN::CompressDirectory(aflags,kflags);
  }
  bool CompressDirectory(const string& directory) {        // AFLOW_FUNCTION_IMPLEMENTATION
    _kflags kflags;
    kflags.KZIP_BIN=DEFAULT_KZIP_BIN; //[CO20211130, moving -9q above as default]+" -9q";
    return KBIN::CompressDirectory(directory,kflags);
  }
}

// *******************************************************************************************
// KBIN::Clean
// *******************************************************************************************
namespace KBIN {
  void Clean(const _aflags& aflags){aurostd::xoption opts_clean;return KBIN::Clean(aflags,opts_clean);}          // AFLOW_FUNCTION_IMPLEMENTATION
  void Clean(const _aflags& aflags,const aurostd::xoption& opts_clean) {          // AFLOW_FUNCTION_IMPLEMENTATION  //CO20210901
    //    cerr << XPID << "KBIN::Clean: aflags.Directory=" << aflags.Directory << endl;
    return KBIN::Clean(aflags.Directory,opts_clean);
  }
}

namespace KBIN {
  void Clean(const string _directory) {aurostd::xoption opts_clean;return KBIN::Clean(_directory,opts_clean);}        // AFLOW_FUNCTION_IMPLEMENTATION
  void Clean(const string _directory,const aurostd::xoption& opts_clean) {        // AFLOW_FUNCTION_IMPLEMENTATION  //CO20210901
    string directory=_directory;
    //    cerr << XPID << "KBIN::Clean: directory=" << aflags.Directory << endl;

    aurostd::StringSubst(directory,"/"+_AFLOWIN_,"");  // so it is easier to search
    aurostd::StringSubst(directory,"/"+_AFLOWLOCK_,"");  // so it is easier to search

    for(uint iext=1;iext<XHOST.vext.size();iext++) { // SKIP uncompressed
      aurostd::StringSubst(directory,"/OUTCAR.bands"+XHOST.vext[iext],"");  // so it is easier to search
      aurostd::StringSubst(directory,"/OUTCAR.static"+XHOST.vext[iext],"");  // so it is easier to search    
      aurostd::StringSubst(directory,"/OUTCAR.relax2"+XHOST.vext[iext],"");  // so it is easier to search
      aurostd::StringSubst(directory,"/OUTCAR.relax1"+XHOST.vext[iext],"");  // so it is easier to search
      aurostd::StringSubst(directory,"/OUTCAR.relax"+XHOST.vext[iext],"");  // so it is easier to search
    }   
    aurostd::StringSubst(directory,"/OUTCAR","");      // so it is easier to search

    for(uint iext=1;iext<XHOST.vext.size();iext++) { // SKIP uncompressed
      aurostd::StringSubst(directory,"/EIGENVAL.bands"+XHOST.vext[iext],"");  // so it is easier to search
      aurostd::StringSubst(directory,"/EIGENVAL.static"+XHOST.vext[iext],"");  // so it is easier to search    
      aurostd::StringSubst(directory,"/EIGENVAL.relax2"+XHOST.vext[iext],"");  // so it is easier to search
      aurostd::StringSubst(directory,"/EIGENVAL.relax1"+XHOST.vext[iext],"");  // so it is easier to search
      aurostd::StringSubst(directory,"/EIGENVAL.relax"+XHOST.vext[iext],"");  // so it is easier to search
    }   
    aurostd::StringSubst(directory,"/EIGENVAL","");      // so it is easier to search

    vector<string> vaflowin_variants_aelagl;  //CO20240407
    aurostd::string2tokens(DEFAULT_FILE_AFLOWIN_VARIANTS_AELAGL,vaflowin_variants_aelagl,",");  //CO20240407
    bool exists_aflowin_aelagl=false; //CO20240407 - for the if statement below
    for(uint i=0;i<vaflowin_variants_aelagl.size();i++){
      for(uint iext=1;iext<XHOST.vext.size();iext++) { // SKIP uncompressed
        aurostd::StringSubst(directory,"/"+vaflowin_variants_aelagl[i]+XHOST.vext[iext],"");  // so it is easier to search
      }
      aurostd::StringSubst(directory,"/"+vaflowin_variants_aelagl[i],"");  // so it is easier to search
      exists_aflowin_aelagl=(exists_aflowin_aelagl || aurostd::FileExist(string(directory+"/"+vaflowin_variants_aelagl[i])));  //CO20240407
    }
    //[CO20240407 - OBSOLETE]aurostd::StringSubst(directory,"/"+_AFLOWIN_AGL_DEFAULT_,"");  // so it is easier to search
    //[CO20240407 - OBSOLETE]aurostd::StringSubst(directory,"/"+_AFLOWIN_AEL_DEFAULT_,"");  // so it is easier to search    
    for(uint iext=1;iext<XHOST.vext.size();iext++) { // SKIP uncompressed
      //[CO20240407 - OBSOLETE]aurostd::StringSubst(directory,"/"+_AFLOWIN_AGL_DEFAULT_+XHOST.vext[iext],"");  // so it is easier to search
      //[CO20240407 - OBSOLETE]aurostd::StringSubst(directory,"/"+_AFLOWIN_AEL_DEFAULT_+XHOST.vext[iext],"");  // so it is easier to search    
      aurostd::StringSubst(directory,"/aflow.in"+XHOST.vext[iext],"");      // so it is easier to search
    }   
    aurostd::StringSubst(directory,"/aflow.in","");      // so it is easier to search

    if(!aurostd::FileExist(string(directory+"/"+"NOCLEAN")) &&
        !aurostd::FileExist(string(directory+"/"+"NOCLEAR")) &&
        !aurostd::FileExist(string(directory+"/"+"noclean")) &&
        !aurostd::FileExist(string(directory+"/"+"noclear")) &&
        !aurostd::FileExist(string(directory+"/"+"Makefile")) &&
        !aurostd::FileExist(string(directory+"/"+"aflow.h")) &&
        !aurostd::FileExist(string(directory+"/"+"paper.tex")) &&
        !aurostd::FileExist(string(directory+"/"+"manuscript.tex")) &&
        !aurostd::FileExist(string(directory+"/"+"supplementary_information.tex")) &&
        !aurostd::FileExist(string(directory+"/"+"supplementary_materials.tex")) &&
        !aurostd::FileExist(string(directory+"/"+"review.tex"))) {
      if(aurostd::FileExist(string(directory+"/"+_AFLOWIN_)) ||    // normal aflow.in or specified it
          aurostd::FileExist(string(directory+"/aflow.in")) ||      // normal aflow.in
          exists_aflowin_aelagl  //CO20240407
          //[CO20240407 - OBSOLETE]aurostd::FileExist(string(directory+"/"+_AFLOWIN_AGL_DEFAULT_)) ||  // normal agl_aflow.in
          //[CO20240407 - OBSOLETE]aurostd::FileExist(string(directory+"/"+_AFLOWIN_AEL_DEFAULT_)) 
          ) { // normal ael_aflow.in

        //CO20261006 START - KEEP_RELAX (--keep_relax): the folder is made ready to run again and keeps what VASP computed
        //(KBIN::VASP_KeptFile()); the next run files the interrupted execution and picks the structure to restart from
        //(KBIN::VASP_ResumePrepare()), so the CONTCAR is not saved here. DEMOTE_RELAX: a finished run that is sent back
        //(xplug rejected it) keeps its stages as parts (KBIN::VASP_DemoteStages()). The job log becomes <LOCK>.job<N>, and
        //the vasp.out copies of that job's fixes (aflow.error.<fix>) become aflow.error.<fix>.job<N>, so the next job's
        //own aflow.error files tell what happened to it.
        const bool keep_relax=opts_clean.flag("KEEP_RELAX");
        if(keep_relax){
          if(opts_clean.flag("DEMOTE_RELAX")){KBIN::VASP_DemoteStages(directory);}
          //N: one more than any earlier job's number, in its log or in its fix logs
          vector<string> vnames;
          aurostd::DirectoryLS(directory,vnames);
          vector<string> verrors;  //this job's fix logs
          uint njob=1;
          string name="";
          for(uint i=0;i<vnames.size();i++){
            const bool error=(vnames[i].find("aflow.error.")==0);
            if(!error && vnames[i].find(_AFLOWLOCK_+".job")!=0){continue;}
            name=vnames[i];
            aurostd::IsCompressed(vnames[i],name);  //name without its compression extension
            const string::size_type pos=name.rfind(".job");
            if(pos!=string::npos && pos+4<name.size() && name.find_first_not_of("0123456789",pos+4)==string::npos){
              njob=std::max(njob,aurostd::string2utype<uint>(name.substr(pos+4))+1);
              continue;
            }
            if(error){verrors.push_back(vnames[i]);}
          }
          const string job=".job"+aurostd::utype2string<uint>(njob);
          if(aurostd::FileExist(directory+"/"+_AFLOWLOCK_)){aurostd::file2file(directory+"/"+_AFLOWLOCK_,directory+"/"+_AFLOWLOCK_+job);}
          for(uint i=0;i<verrors.size();i++){
            name=verrors[i];
            aurostd::IsCompressed(verrors[i],name);
            aurostd::file2file(directory+"/"+verrors[i],directory+"/"+name+job+aurostd::GetCompressionExtension(verrors[i]));
          }
        }
        //CO20261006 STOP
        //CO20210716 - save contcar
        bool save_contcar=(opts_clean.flag("SAVE_CONTCAR") && !keep_relax);  //CO20261006 - not with KEEP_RELAX
        bool save_contcar_outcar_complete=(opts_clean.flag("SAVE_CONTCAR_OUTCAR_COMPLETE") && !keep_relax);  //CO20261006 - not with KEEP_RELAX
        if(save_contcar||save_contcar_outcar_complete){
          bool outcar_complete=false;
          if(save_contcar_outcar_complete){
            _xvasp xvasp;xvasp.Directory=directory;
            _aflags aflags;ofstream FileMESSAGE;
            bool verbose=false;
            outcar_complete=KBIN::VASP_RunFinished(xvasp,aflags,FileMESSAGE,verbose);
          }
          if(save_contcar||outcar_complete){KBIN::VASP_CONTCAR_Save(directory);}
        }

        // CLEAN directory
        //DX+CO START
        vector<string> vfiles;  //not only files, includes EVERYTHING
        string file_path;
        aurostd::DirectoryLS(directory,vfiles);
        bool found_aflowin_aelagl=false;  //CO20240407
        for(uint i=0;i<vfiles.size();i++) {
          file_path=directory + "/" + vfiles[i];
          if(aurostd::substring2bool(vfiles[i],_AFLOWIN_)){continue;}
          if(aurostd::substring2bool(vfiles[i],"aflow.in")){continue;}
          found_aflowin_aelagl=false; //CO20240407
          for(uint j=0;j<vaflowin_variants_aelagl.size();j++){  //CO20240407
            if(aurostd::substring2bool(vfiles[i],vaflowin_variants_aelagl[j])){found_aflowin_aelagl=true;break;}
          }
          if(found_aflowin_aelagl){continue;}
          //[CO20240407 - OBSOLETE]if(aurostd::substring2bool(vfiles[i],_AFLOWIN_AGL_DEFAULT_)){continue;}
          //[CO20240407 - OBSOLETE]if(aurostd::substring2bool(vfiles[i],_AFLOWIN_AEL_DEFAULT_)){continue;}
          if(aurostd::substring2bool(vfiles[i],DEFAULT_AFLOW_FROZSL_INPUT_OUT)){continue;}
          if(keep_relax && (aurostd::IsDirectory(file_path) || KBIN::VASP_KeptFile(vfiles[i]))){continue;}  //CO20261006 - children and kept files stay
          if(aurostd::IsDirectory(file_path)){                 
            if(aurostd::substring2bool(vfiles[i],KBIN_SUBDIRECTORIES)){ // only directories we don't ignore
              aurostd::RemoveDirectory(file_path);
            }
            continue;                                                   // ignore all other directories
          }
          if(aurostd::IsFile(file_path)){                 
            if(vfiles[i].size() && vfiles[i][0]=='.'){
              if(aurostd::substring2bool(vfiles[i],".nfs")){            // only hidden files we don't ignore
                aurostd::RemoveFile(file_path);
              }
              continue;                                                 // ignore all other hidden files
            }
            aurostd::RemoveFile(file_path);
          }
        }
        aurostd::execute("rm -f "+directory+"/.pam*");
        aurostd::execute("rm -f "+directory+"/*~");

        //DX+CO END
        // now DECOMPRESS _AFLOWIN_.EXT if a mistake was made
        for(uint iext=1;iext<XHOST.vext.size();iext++) { // SKIP uncompressed
          ifstream FileCHECK;string FileNameCHECK;
          FileNameCHECK=directory+"/" + _AFLOWIN_ + XHOST.vext[iext];                    // _AFLOWIN_.EXT
          FileCHECK.open(FileNameCHECK.c_str(),std::ios::in);                         // _AFLOWIN_.EXT
          FileCHECK.clear();FileCHECK.close();                                        // _AFLOWIN_.EXT
          if(FileCHECK) {                                                             // _AFLOWIN_.EXT
            aurostd::execute(XHOST.vzip[iext]+" -dqf "+_AFLOWIN_+XHOST.vext[iext]); // _AFLOWIN_.EXT
          }
        } // _AFLOWIN_.EXT
      }
    }
  }
}

// *******************************************************************************************
// KBIN::XClean
// *******************************************************************************************
namespace KBIN {
  void XClean(string options) {
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    if(LDEBUG) cerr << __AFLOW_FUNC__ << " BEGIN" << endl;  
    vector<string> tokens;
    aurostd::string2tokens(options,tokens,",");
    if(tokens.size()!=0) {
      init::ErrorOption(options,__AFLOW_FUNC__,"aflow --xclean");
    }

    vector<string> vcheck1;aurostd::string2tokens(string("OUTCAR.static,OUTCAR.relax2,OUTCAR.relax1"),vcheck1,",");
    vector<string> vcheck2;aurostd::string2tokens(string("OUTCAR,OUTCAR,OUTCAR"),vcheck2,",");

    for(uint iext=1;iext<XHOST.vext.size();iext++) { // SKIP uncompressed
      vcheck1.push_back("OUTCAR.relax1"+XHOST.vext[iext]);
    }

    for(uint iext=1;iext<XHOST.vext.size();iext++) { // SKIP uncompressed
      vcheck2.push_back("OUTCAR.relax2"+XHOST.vext[iext]);
    }

    vector<string> vfile;
    bool test=false;

    cout << __AFLOW_FUNC__ << " checking missing " << "OUTCAR*" << " with " << _AFLOWLOCK_ << endl;  // check OUTCAR.static
    aurostd::string2vectorstring(aurostd::execute2string(XHOST.command("find")+" ./ -name "+_AFLOWLOCK_),vfile);
    for(uint j=0;j<vfile.size();j++) {
      aurostd::StringSubst(vfile[j],_AFLOWLOCK_,"");
      if(!aurostd::FileExist(vfile[j]+"OUTCAR") && !aurostd::EFileExist(vfile[j]+"OUTCAR.relax1")) {
        cout << __AFLOW_FUNC__ << " cleaning=" << vfile[j] << endl;
        if(!test) KBIN::Clean(vfile[j]);
      }
    }
    for(uint i=0;i<vcheck1.size();i++) {
      cout << __AFLOW_FUNC__ << " checking missing " << vcheck2[i] << " with " << vcheck1[i] << endl;  // check OUTCAR.static
      aurostd::string2vectorstring(aurostd::execute2string(XHOST.command("find")+" ./ -name "+vcheck1[i]),vfile);
      for(uint j=0;j<vfile.size();j++) {
        aurostd::StringSubst(vfile[j],vcheck1[i],"");
        if(!aurostd::FileExist(vfile[j]+vcheck2[i])) {
          cout << __AFLOW_FUNC__ << " cleaning=" << vfile[j] << endl;
          if(!test) KBIN::Clean(vfile[j]);
        }
      }
    }    
    if(LDEBUG) cerr << __AFLOW_FUNC__ << " END" << endl;  
  }
} // namespace KBIN

// *******************************************************************************************
// KBIN::XClean
// *******************************************************************************************

//ME20200219 - based on CO's code in old PhononCalculator
namespace KBIN {
  int get_NCPUS() {
    _kflags kflags;
    return get_NCPUS(kflags);
  }

  int get_NCPUS(const _kflags& kflags) {  //CO20220630 - rewritten
    int ncpus=1;
    if(kflags.KBIN_MPI_NCPUS>0){ncpus=kflags.KBIN_MPI_NCPUS;} //aflow.in is lowest priority
    if(XHOST.vflag_control.flag("XPLUG_NUM_THREADS")){  //command line argument overrides
      string ncpus_str=XHOST.vflag_control.getattachedscheme("XPLUG_NUM_THREADS");
      if(aurostd::isfloat(ncpus_str)){ncpus=aurostd::string2utype<int>(ncpus_str);}
      if(ncpus_str=="MAX"){ncpus=MPI_NCPUS_MAX;}
    }
    if(ncpus<1){ncpus=1;}
    return ncpus;
  }
}  // namespace KBIN

// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2023           *
// *                                                                         *
// ***************************************************************************
