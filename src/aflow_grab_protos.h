// ***************************************************************************
// *                                                                         *
// *          Aflow COREY OSES - Johns Hopkins University 2025-2026          *
// *           Aflow KATIE CARIAGA - Johns Hopkins University 2025           *
// *                                                                         *
// ***************************************************************************
//aflow_grab_protos.h
// Prototype redecoration (KT, reworked CO20260930): new compounds made by
// putting the given cations and anions on the cation and anion sites of
// AFLOW prototypes, keeping the charge-neutral combinations (CO20261003).

#ifndef _AFLOW_GRAB_PROTOS_H_
#define _AFLOW_GRAB_PROTOS_H_

#include "aflow.h"

namespace pflow {
    class Redecoration {
        public:
            Redecoration();
            ~Redecoration();
            Redecoration(const Redecoration& b);
            const Redecoration& operator=(const Redecoration& b);
            Redecoration(const int NARY,const vector<string> cations,const vector<string> anions);
            void clear();

            bool initialized;

            //redecoration info
            vector<xelement::xelement> vcations;
            vector<xelement::xelement> vanions;
            int NARY;
            vector<string> ANIONS2SEARCH;

            //original structure info: AFLOW prototype label (with -NNN for a
            //parameter set) and the prototype compound's elements in label
            //order (alphabetical, species A, B, ...) //CO20260930
            vector<string> vlabel;
            vector<vector<string> > velements;

            //output //CO20260930
            string aflow_bin;               //binary named in the commands (default "aflow")
            string commands_file;           //where the commands are written ("" = do not write)
            bool ldau_runs;                 //also +U runs with bands where a species takes U (special projects) //CO20261004
            vector<string> vproto_redecorated;  //label.DECORATION:El1:El2..., sorted, unique
            vector<string> redecoration_cmds;

            //runs already in CHAOS are not written //CO20261004
            aflowlib::ChaosRuns chaos;      //runs in CHAOS of the systems read
            bool chaos_checked;             //leave out the runs in chaos (set by ReadChaosRuns())
            uint nruns_in_chaos;            //runs WriteCommands() left out
            mutable std::map<string,std::map<string,string> > decoration_representative; //label -> decoration -> representative

            bool initialize(const int NARY,const vector<string> cations,const vector<string> anions);

            //functions
            vector<string> RedecoratePrototype(const string& label,const vector<string>& elements) const; //CO20260930
            void FindCompounds();           //vproto_redecorated //CO20261004
            void ReadChaosRuns(const string& chaos_runs_file=""); //runs in CHAOS of the systems of vproto_redecorated //CO20261004
            bool RunInChaos(const string& run_name) const;  //CO20261004
            vector<string> WriteCommands(); //redecoration_cmds (and commands_file) //CO20261004
            vector<string> GrabProtos();    //FindCompounds() then WriteCommands()

        private:
            void free();
            void copy(const Redecoration& b);
    };
}

#endif
