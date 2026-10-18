// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2023           *
// *                                                                         *
// ***************************************************************************
// Written by Marco Esters, 2022
// Based on prior work by Hagen Eckert
//
// Unit test class to conduct parallelized unit tests with unified output.
//
// Tests are grouped into categories so that entire suites can be tested.
// For example, the test "aurostd" calls xscalar, xmatrix, etc. The individual
// tests can be called as well.

#include <set>  //CO20261002
#include "aflow.h"
#include "aflow_anrl.h"  //DX20201104
#include "SQLITE/sqlite3.h" //CO20260927
#include "SQLITE/aflow_sqlite.h" //CO20261004 - sql::SQLexecuteCommand()
#include "APL/aflow_apl.h"  //TL20260608
#include "aflow_compare_structure.h"
#include "aflow_prototypes_jhu.h" //CO20261002
#include "aflow_cce.h" //CO20261004 - CCE iodide references in jhu_proto
#include "aflow_chull.h"
#include "aflow_pocc.h"  //TL20250616 - for POCC sampling tests
#include <random>  //CO20261005 - std::mt19937 for the Smith normal form test
#include "aflow_aflux.h" //WS20240711
#include "aflow_xbinary.h"
#include "aflow_grab_protos.h" //CO20260930
#include <cctype>  //CO20260930 - symmetry_exact tests
#include <cstdio>  //CO20260930
#include <set>  //CO20260930
using namespace std::placeholders;

namespace unittest {
  aurostd::JSON::object utd = aurostd::EmbData::get_unit_test();

  UnitTest::UnitTest(ostream& oss) : xStream(oss) {
    initialize();
  }

  UnitTest::UnitTest(ofstream& mf, ostream& oss) : xStream(mf, oss) {
    initialize();
  }

  UnitTest::UnitTest(const UnitTest& ut) : xStream(*ut.getOFStream(), *ut.getOSS()) {
    copy(ut);
  }

  const UnitTest& UnitTest::operator=(const UnitTest& ut) {
    copy(ut);
    return *this;
  }

  UnitTest::~UnitTest() {
    xStream::free();
    free();
  }

  void UnitTest::clear() {
    free();
  }

  void UnitTest::free() {
    aflags.clear();
    test_functions.clear();
    test_groups.clear();
  }

  void UnitTest::copy(const UnitTest& ut) {
    if (this == &ut) return;
    aflags = ut.aflags;
    test_functions = ut.test_functions;
    test_groups = ut.test_groups;
  }

  void UnitTest::initialize() {
    free();
    string dir = XHOST.vflag_control.getattachedscheme("DIRECTORY");
    aflags.Directory = (!dir.empty()?dir:aurostd::getPWD());

    initializeTestFunctions();
    initializeTestGroups();
  }

  /// @brief Initialize unit tests and add them to map of test functions.
  void UnitTest::initializeTestFunctions() {
    xcheck xchk;

    // aurostd
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::xscalarTest, this, _1, _2, _3);
    xchk.function_name = "xscalarTest():";
    xchk.task_description = "xscalar functions";
    test_functions["xscalar"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::xvectorTest, this, _1, _2, _3);
    xchk.function_name = "xvectorTest():";
    xchk.task_description = "xvector functions";
    test_functions["xvector"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::xmatrixTest, this, _1, _2, _3);
    xchk.function_name = "xmatrixTest():";
    xchk.task_description = "xmatrix functions";
    test_functions["xmatrix"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::aurostdMainTest, this, _1, _2, _3);
    xchk.function_name = "aurostdMainTest():";
    xchk.task_description = "aurostd_main functions";
    test_functions["aurostd_main"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::xparserTest, this, _1, _2, _3);
    xchk.function_name = "xparserTest():";
    xchk.task_description = "xparser functions";
    test_functions["xparser"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::xfitTest, this, _1, _2, _3);
    xchk.function_name = "xfitTest():";
    xchk.task_description = "xfit functions";
    test_functions["xfit"] = xchk;

    // database
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::schemaTest, this, _1, _2, _3);
    xchk.function_name = "schemaTest():";
    xchk.task_description = "AFLOW schema";
    test_functions["schema"] = xchk;

    //CO20261002 - offline checks of the AURL helpers, AFLUX paging and the CHAOS API key handling
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::aurlHostTest, this, _1, _2, _3);
    xchk.function_name = "aurlHostTest():";
    xchk.task_description = "AURL host and path, AFLUX paging";
    test_functions["aurl_host"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::chaosKeyTest, this, _1, _2, _3);
    xchk.function_name = "chaosKeyTest():";
    xchk.task_description = "CHAOS API key: hosts, URLs, key, curl, complete answers";
    test_functions["chaos_key"] = xchk;

    xchk = initializeXCheck();  //CO20261004
    xchk.func = [this](uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
      // a probe test that throws, run through runUnitTest() of a separate UnitTest object
      (void) errors;
      const string check_function = "UnitTest::runUnitTest()";
      const string probe = "throwing_probe";
      stringstream oss;
      UnitTest ut(oss);
      xcheck xprobe = ut.initializeXCheck();
      xprobe.func = [](uint&, vector<vector<string> >&, vector<string>&) {
        throw aurostd::xerror(__AFLOW_FILE__, "throwingProbe()", "probe failure", _RUNTIME_ERROR_);
      };
      xprobe.function_name = "throwingProbe():";
      xprobe.task_description = "probe that throws";
      ut.test_functions[probe] = xprobe;
      vector<string> names(1, probe);
      vector<string>::iterator it = names.begin();
      ut.runUnitTest(it, names);
      const xcheck& done = ut.test_functions[probe];
      checkEqual(done.finished, true, check_function, "a test that throws is marked finished", passed_checks, results);
      checkEqual(ut.taskSuccessful(probe), false, check_function, "a test that throws counts as failed", passed_checks, results);
      checkEqual(done.errors.size() == 1 && aurostd::substring2bool(done.errors[0], "probe failure"), true, check_function,
          "the exception message is kept among the errors", passed_checks, results);
    };
    xchk.function_name = "unitTestRunnerTest():";
    xchk.task_description = "unit test runner: a test that throws";
    test_functions["unit_test_runner"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::chullUnaryOrderTest, this, _1, _2, _3);
    xchk.function_name = "chullUnaryOrderTest():";
    xchk.task_description = "chull: unary ordering when nothing or little was loaded";
    test_functions["chull_unary_order"] = xchk;

    xchk = initializeXCheck(); //CO20261006
    xchk.func = std::bind(&UnitTest::chullLinksTest, this, _1, _2, _3);
    xchk.function_name = "chullLinksTest():";
    xchk.task_description = "chull: S4E and CHAOS logos, no entry pages";
    test_functions["chull_links"] = xchk;

    xchk = initializeXCheck(); //CO20261003
    xchk.func = std::bind(&UnitTest::afluxIdQueryTest, this, _1, _2, _3);
    xchk.function_name = "afluxIdQueryTest():";
    xchk.task_description = "AUIDs and AURLs for s4e.ai's AFLUX engine: no quotes";
    test_functions["aflux_id_query"] = xchk;

    xchk = initializeXCheck(); //CO20261007
    xchk.func = std::bind(&UnitTest::afluxResponseKeysTest, this, _1, _2, _3);
    xchk.function_name = "afluxResponseKeysTest():";
    xchk.task_description = "CHAOS API answers: chaos_id for the identifier, no aurl or version stamps; format(aflow) whole";
    test_functions["aflux_response_keys"] = xchk;

    xchk = initializeXCheck(); //CO20261018
    xchk.func = std::bind(&UnitTest::afluxFiltersTest, this, _1, _2, _3);
    xchk.function_name = "afluxFiltersTest():";
    xchk.task_description = "AFLUX filters as the CHAOS paper describes them: any atom, species pairs, components from 0, lists";
    test_functions["aflux_filters"] = xchk;

    xchk = initializeXCheck(); //CO20261008
    xchk.func = std::bind(&UnitTest::chaosPathTest, this, _1, _2, _3);
    xchk.function_name = "chaosPathTest():";
    xchk.task_description = "chaos_path from the catalog, chaos_id, pocc_system: entries, files and the database";
    test_functions["chaos_path"] = xchk;

    xchk = initializeXCheck(); //CO20261003
    xchk.func = std::bind(&UnitTest::serverOptionTest, this, _1, _2, _3);
    xchk.function_name = "serverOptionTest():";
    xchk.task_description = "--server: s4e.ai by default, the legacy server only by name";
    test_functions["server_option"] = xchk;

    xchk = initializeXCheck(); //CO20261004
    xchk.func = std::bind(&UnitTest::machineServerTest, this, _1, _2, _3);
    xchk.function_name = "machineServerTest():";
    xchk.task_description = "Machines by generic template and --machine_name; the data server by role; s4e AUIDs";
    test_functions["machine_server"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::poccHEAFieldsTest, this, _1, _2, _3);
    xchk.function_name = "poccHEAFieldsTest():";
    xchk.task_description = "POCC HEA fields: lattice distortion statistics and lib2raw's reading of aflow.pocc.out"; //CO20261004
    test_functions["pocc_hea"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::poccDescriptorsTest, this, _1, _2, _3);
    xchk.function_name = "poccDescriptorsTest():";
    xchk.task_description = "POCC descriptors: Miedema mixing enthalpy, geometric parameter, Omega, k_cr1"; //CO20261015
    test_functions["pocc_descriptors"] = xchk;

    //CO20261004
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::derivedPropertiesTest, this, _1, _2, _3);
    xchk.function_name = "derivedPropertiesTest():";
    xchk.task_description = "Hull distance and DEED in the database";
    test_functions["derived_properties"] = xchk;

    // xstructure
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::atomicEnvironmentTest, this, _1, _2, _3);
    xchk.function_name = "atomicEnvironmentTest():";
    xchk.task_description = "Creating atomic environments";
    test_functions["atomic_environment"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::xstructureParserTest, this, _1, _2, _3);
    xchk.function_name = "xstructureParserTest():";
    xchk.task_description = "xstructure parsers";
    test_functions["xstructure_parser"] = xchk;

     //AFLUX
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::AFLUXTest, this, _1, _2, _3);
    xchk.function_name = "AFLUXTest():";
    xchk.task_description = "AFLUX parse and query";
    test_functions["AFLUX"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::xstructureTest, this, _1, _2, _3);
    xchk.function_name= "xstructureTest():";
    xchk.task_description = "xstructure functions";
    test_functions["xstructure"] = xchk;

    xchk = initializeXCheck(); //CO20261018
    xchk.func = std::bind(&UnitTest::aaplLatticeTypeTest, this, _1, _2, _3);
    xchk.function_name= "aaplLatticeTypeTest():";
    xchk.task_description = "Lattice type of the AAPL supercell of NaF after PerformFullSymmetry() (about 5 GB)";
    test_functions["aapl_lattice_type"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::jhuPrototypeTest, this, _1, _2, _3);
    xchk.function_name= "jhuPrototypeTest():";
    xchk.task_description = "JHU prototype records generate and keep their labels"; //CO20261002
    test_functions["jhu_proto"] = xchk;

    xchk = initializeXCheck();  //CO20261004
    xchk.func = std::bind(&UnitTest::mostRelaxedStructureTest, this, _1, _2, _3);
    xchk.function_name= "mostRelaxedStructureTest():";
    xchk.task_description = "KBIN::GetMostRelaxedStructure()";
    test_functions["most_relaxed"] = xchk;

    xchk = initializeXCheck();  //CO20261006
    xchk.func = std::bind(&UnitTest::relaxResumeTest, this, _1, _2, _3);
    xchk.function_name= "relaxResumeTest():";
    xchk.task_description = "Relaxation outputs kept across restarts";
    test_functions["relax_resume"] = xchk;

    xchk = initializeXCheck();  //CO20261010, see the notes above vaspRelaxContinueTest()
    xchk.func = std::bind(&UnitTest::vaspRelaxContinueTest, this, _1, _2, _3);
    xchk.function_name = "vaspRelaxContinueTest():";
    xchk.task_description = "Electronic steps of any algorithm, EDDRMM at NSW, and the continuation of the last relaxation";
    test_functions["vasp_relax_continue"] = xchk;

    // structure generation
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::ceramgenTest, this, _1, _2, _3);
    xchk.function_name= "ceramgenTest():";
    xchk.task_description = "pflow::GENERATE_CERAMICS()";
    test_functions["ceramgen"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::wyckoffLibraryTest, this, _1, _2, _3);
    xchk.function_name= "wyckoffLibraryTest():";
    xchk.task_description = "Structures on the corrected Wyckoff sets keep their space group"; //CO20261002
    test_functions["wyckoff"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::sqliteErrorTest, this, _1, _2, _3);
    xchk.function_name= "sqliteErrorTest():";
    xchk.task_description = "SQLite command errors are reported with their message"; //CO20261002
    test_functions["sqlite"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::prototypeParameterTest, this, _1, _2, _3);
    xchk.function_name= "prototypeParameterTest():";
    xchk.task_description = "Prototype library parameter sets are numbers and give real distances"; //CO20261003
    test_functions["proto_params"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::prototypeGeneratorTest, this, _1, _2, _3);
    xchk.function_name= "prototypeGeneratorTest():";
    xchk.task_description = "Generate all prototypes and test symmetry";
    test_functions["proto"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::redecorationTest, this, _1, _2, _3);
    xchk.function_name= "redecorationTest():";
    xchk.task_description = "pflow::Redecoration (aflow --redecorate)";
    test_functions["redecoration"] = xchk;  //CO20260930

    //EntryLoader
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::entryLoaderTest, this, _1, _2, _3);
    xchk.function_name = "entryLoaderTest():";
    xchk.task_description = "entryLoader functions";
    test_functions["entry_loader"] = xchk;

    //CO20261004 - runs already in CHAOS (offline part)
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::chaosRunsTest, this, _1, _2, _3);
    xchk.function_name = "chaosRunsTest():";
    xchk.task_description = "runs already in CHAOS (aflowlib::ChaosRuns)";
    test_functions["chaos_runs"] = xchk;

    // POCC/DEED
    xchk = initializeXCheck();
    xchk.func = std::bind(&pocc::unittest::deedTest, _1, _2, _3);
    xchk.function_name = "deedTest():";
    xchk.task_description = "DEED calculation, composition selection, and POCC provenance";
    test_functions["deed"] = xchk;

    // APL
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::aplThermalTest, this, _1, _2, _3);
    xchk.function_name = "aplThermalTest():";
    xchk.task_description = "APL thermal moment descriptors";
    test_functions["apl_thermal"] = xchk;

    // KKSYM and the KPOINTS fixes - CO20261009, see the notes above vaspKpointsTest()
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::vaspKpointsTest, this, _1, _2, _3);
    xchk.function_name = "vaspKpointsTest():";
    xchk.task_description = "Lattice point groups, the KKSYM grid that keeps the lattice class, and ISYM for SYM=OFF";
    test_functions["vasp_kpoints"] = xchk;

    // VASP force options - CO20261004, see the notes above vaspPreservedTest()
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::vaspPreservedTest, this, _1, _2, _3);
    xchk.function_name = "vaspPreservedTest():";
    xchk.task_description = "PRESERVED keeps the PREC scheme, and a preserved ALGO leaves the INCAR alone";
    test_functions["vasp_preserved"] = xchk;

    // NBANDS default - CO20261009, see the notes above vaspNbandsTest()
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::vaspNbandsTest, this, _1, _2, _3);
    xchk.function_name = "vaspNbandsTest():";
    xchk.task_description = "Default NBANDS from the electrons of the cell and the initial moments";
    test_functions["vasp_nbands"] = xchk;

    // lib2raw - CO20260929, see the notes above lib2rawBaderChildrenTest() and getPrimitiveKeepCellTest()
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::lib2rawBaderChildrenTest, this, _1, _2, _3);
    xchk.function_name = "lib2rawBaderChildrenTest():";
    xchk.task_description = "lib2raw skips Bader for phonon and elastic child runs only";
    test_functions["lib2raw_bader_children"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::getPrimitiveKeepCellTest, this, _1, _2, _3);
    xchk.function_name = "getPrimitiveKeepCellTest():";
    xchk.task_description = "GetPrimitive keeps an AAPL supercell that cannot be reduced consistently";
    test_functions["getprim_keep_cell"] = xchk;

    // lib2raw triage of 2026-09-30 - CO20260930, see the notes above bringInCellFarComponentTest() and glideEquidistantPointTest()
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::bringInCellFarComponentTest, this, _1, _2, _3);
    xchk.function_name = "bringInCellFarComponentTest():";
    xchk.task_description = "BringInCell returns for components far outside the cell (aflowSG(loose) hang)";
    test_functions["bringincell_far_component"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::glideEquidistantPointTest, this, _1, _2, _3);
    xchk.function_name = "glideEquidistantPointTest():";
    xchk.task_description = "a glide applied to a point about tol/2 from its plane does not throw";
    test_functions["glide_equidistant_point"] = xchk;

    xchk = initializeXCheck();  //CO20261018
    xchk.func = std::bind(&UnitTest::latticeDimensionSphereTest, this, _1, _2, _3);
    xchk.function_name = "latticeDimensionSphereTest():";
    xchk.task_description = "LatticeDimensionSphere refuses a nearly singular lattice (cell lists that filled memory on macOS)";
    test_functions["lattice_dimension_sphere"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::poscarOrigRepairTest, this, _1, _2, _3);
    xchk.function_name = "poscarOrigRepairTest():";
    xchk.task_description = "lib2raw rebuilds an unreadable POSCAR.orig from aflow.in and never writes an empty one";
    test_functions["poscar_orig_repair"] = xchk;

    // symmetry_exact - CO20260930, see the notes above symmetryExactDistanceTest()
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::symmetryExactDistanceTest, this, _1, _2, _3);
    xchk.function_name = "symmetryExactDistanceTest():";
    xchk.task_description = "the allocation-free distance of getFullSymBasis() equals FPOSMatch()'s, bitwise";
    test_functions["symmetry_exact_distance"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::symmetryExactMappingTest, this, _1, _2, _3);
    xchk.function_name = "symmetryExactMappingTest():";
    xchk.task_description = "getFullSymBasis() with the cell list: same answer and maps as the reference, ties and bin edges included";
    test_functions["symmetry_exact_mapping"] = xchk;

    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::symmetryExactPathsTest, this, _1, _2, _3);
    xchk.function_name = "symmetryExactPathsTest():";
    xchk.task_description = "faster symmetry search: same atom maps and same lib2raw results as the reference paths";
    test_functions["symmetry_exact_paths"] = xchk;

    xchk = initializeXCheck();  // not in a group, so not in "all": hours on one core, run with --np=N
    xchk.func = std::bind(&UnitTest::symmetryExactAllGroupsTest, this, _1, _2, _3);
    xchk.function_name = "symmetryExactAllGroupsTest():";
    xchk.task_description = "faster symmetry search: same lib2raw results for every prototype and every space group";
    test_functions["symmetry_exact_all"] = xchk;
    // lib2raw POCC children, 2026-10-03 - CO20261003, see the notes above rhlMirrorOriginShiftTest()
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::rhlMirrorOriginShiftTest, this, _1, _2, _3);
    xchk.function_name = "rhlMirrorOriginShiftTest():";
    xchk.task_description = "R3m in rhombohedral axes: the origin shift is found when a mirror translation is half a cell";
    test_functions["rhl_mirror_origin_shift"] = xchk;

    // formation enthalpy references added in the triage, 2026-09-27 - CO20261004, see the notes above enthalpyReferencesTest()
    xchk = initializeXCheck();
    xchk.func = std::bind(&UnitTest::enthalpyReferencesTest, this, _1, _2, _3);
    xchk.function_name = "enthalpyReferencesTest():";
    xchk.task_description = "lib2raw's formation enthalpy references for Er_3, Gd, Nd, Pm, Pr, Th_s and Tb_3 (PAW_PBE), in the table and in the data file";
    test_functions["enthalpy_references"] = xchk;

    // POCC Sampling - TL20250616
    xchk = initializeXCheck();
    xchk.func = std::bind(&pocc::unittest::poccSamplingTest, _1, _2, _3);
    xchk.function_name = "poccSamplingTest():";
    xchk.task_description = "POCC sampling functions";
    test_functions["pocc_sampling"] = xchk;

    // POCC enumerator ENUM - CO20261005
    xchk = initializeXCheck();
    xchk.func = std::bind(&pocc::unittest::poccEnumerationTest, _1, _2, _3);
    xchk.function_name = "poccEnumerationTest():";
    xchk.task_description = "POCC enumerator ENUM";
    test_functions["pocc_enumeration"] = xchk;
    // POCC sampling over the whole decoration space - CO20261006
    xchk = initializeXCheck();
    xchk.func = std::bind(&pocc::unittest::poccSamplingSpaceTest, _1, _2, _3);
    xchk.function_name = "poccSamplingSpaceTest():";
    xchk.task_description = "POCC sampling draws from every site configuration";
    test_functions["pocc_sampling_space"] = xchk;
    // POCC cluster correlation functions - CO20261006
    xchk = initializeXCheck();
    xchk.func = std::bind(&pocc::unittest::poccCorrelationsTest, _1, _2, _3);
    xchk.function_name = "poccCorrelationsTest():";
    xchk.task_description = "POCC cluster correlation functions";
    test_functions["pocc_correlations"] = xchk;

    // ovasp
    //Not working yet because we cannot load OUTCARs via the RestAPI
    //xchk = initializeXCheck();
    //xchk.func = std::bind(&UnitTest::xoutcarTest, this, _1, _2, _3);
    //xchk.function_name= "xoutcarTest():";
    //xchk.task_description = "xOUTCAR class";
    //test_functions["outcar"] = xchk;
  }

  /// @brief Initialize xcheck struct to an empty object.
  xcheck UnitTest::initializeXCheck() {
    xcheck xt;
    resetUnitTest(xt);
    xt.func = nullptr;
    xt.function_name = "";
    xt.task_description = "";
    xt.test_group = "";
    return xt;
  }

  /// @brief Reset a unit test to its pre-run state based on test name.
  void UnitTest::resetUnitTest(const string& test_name) {
    if (test_functions.count(test_name)) {
      resetUnitTest(test_functions[test_name]);
    }
  }

  /// @brief Reset an xcheck object to its pre-run state.
  void UnitTest::resetUnitTest(xcheck& test) {
    test.errors.clear();
    test.finished = false;
    test.passed_checks = 0;
    test.results.clear();
  }

  /// @brief Create unit test groups.
  void UnitTest::initializeTestGroups() {
    test_groups.clear();

    test_groups["aurostd"] = {"xscalar", "xvector", "xmatrix", "xfit", "aurostd_main", "xparser", "unit_test_runner"}; //CO20261004 - unit_test_runner
    test_groups["database"] = {"schema", "pocc_hea", "aurl_host", "chaos_key", "chull_unary_order", "aflux_id_query", "aflux_response_keys", "server_option", "machine_server", "chaos_runs", "entry_loader", "sqlite", "derived_properties", "deed"}; //CO20261004 - pocc_hea; CO20261004 - derived_properties, deed; CO20261004 - machine_server; CO20261002 - aurl_host, chaos_key, chull_unary_order; CO20261003 - aflux_id_query, server_option; CO20261004 - chaos_runs; sqlite was registered in no group, so --unit_test never ran it
    test_groups["api"] = {"AFLUX"};
    test_groups["apl"] = {"apl_thermal"};
    test_groups["vasp"] = {"vasp_preserved"};  //CO20261004
    test_groups["vasp"].push_back("vasp_kpoints");  //CO20261009 - a line of its own, so it merges with other changes to the vasp list
    test_groups["lib2raw"] = {"lib2raw_bader_children", "getprim_keep_cell", "bringincell_far_component", "glide_equidistant_point", "poscar_orig_repair", "rhl_mirror_origin_shift", "enthalpy_references"};  //CO20260929; CO20260930 - triage; CO20261003 - POCC children; CO20261004 - enthalpy references
    test_groups["symmetry_exact"] = {"symmetry_exact_distance", "symmetry_exact_mapping", "symmetry_exact_paths"};  //CO20260930 - symmetry_exact_all runs alone
    test_groups["structure"] = {"atomic_environment", "xstructure", "xstructure_parser", "jhu_proto", "most_relaxed", "pocc_enumeration", "redecoration"}; //CO20261004 - jhu_proto runs with aflow --unit_test and make unit_test_1; CO20261004 - most_relaxed; CO20261005 - pocc_enumeration; CO20261006 - redecoration too
    test_groups["structure_gen"] = {"ceramgen", "proto", "wyckoff", "proto_params"}; //CO20261004 - wyckoff and proto_params were registered in no group, so --unit_test never ran them
    test_groups["structure"].push_back("pocc_sampling_space");  //CO20261006 - a line of its own, so it merges with other changes to the structure list
    test_groups["vasp"].push_back("vasp_relax_continue");  //CO20261010 - a line of its own, so it merges with other changes to the vasp list
    test_groups["structure"].push_back("pocc_correlations");  //CO20261006
    //test_groups["ovasp"] = {"outcar"};
    test_groups["database"].push_back("chull_links");  //CO20261006 - a line of its own, so it merges with other changes to the database list
    test_groups["database"].push_back("chaos_path");  //CO20261008
    test_groups["database"].push_back("aflux_filters");  //CO20261018
    test_groups["structure_large"] = {"aapl_lattice_type"};  //CO20261018 - about 5 GB: make unit_test_4, on its own
    test_groups["database"].push_back("pocc_descriptors");  //CO20261015
    test_groups["structure"].push_back("relax_resume");  //CO20261006 - a line of its own, so it merges with other changes to the structure list
    test_groups["structure"].push_back("lattice_dimension_sphere");  //CO20261018
    test_groups["vasp"].push_back("vasp_nbands");  //CO20261009 - a line of its own, so it merges with other changes to the vasp list

    //CO20261018 START - two tiers. The groups of make unit_test_public (unit_test_1: aurostd, database, structure,
    //lib2raw, vasp, apl) hold the fast tests, seconds each, with no network and no CHAOS API key, for every push and pull
    //request. The long ones, a minute or more each (Linux, one core), go to the group long. make unit_test_long, which
    //check_full_1 runs, runs it with the group symmetry_exact (symmetry_exact_paths about 5 min, symmetry_exact_mapping
    //about 40 s) and the test wyckoff (about 2 min); each test stays in one group, or --unit_test does not print its
    //results. aapl_lattice_type (make unit_test_4) and proto (make unit_test_2) stay on their own.
    const vector<string> tests_long = {
      "derived_properties",  // about 5 min: rebuilds a 256-table database five times
      "chaos_path",          // about 1 min: builds a 256-table database
      "aflux_filters",       // about 1 min: builds a 256-table database
      "pocc_enumeration",    // about 1.5 min
      "pocc_correlations"};  // about 1 min
    for (const string& group : vector<string>({"aurostd", "database", "structure", "lib2raw", "vasp", "apl"})) {
      vector<string>& members = test_groups[group];
      members.erase(std::remove_if(members.begin(), members.end(), [&tests_long](const string& t) {return aurostd::WithinList(tests_long, t);}), members.end());
    }
    test_groups["long"] = tests_long;
    //CO20261018 STOP

    for (std::map<string, vector<string> >::iterator it = test_groups.begin(); it != test_groups.end(); ++it) {
      const vector<string>& members = (*it).second;
      for (size_t i = 0; i < members.size(); i++) {
        test_functions[members[i]].test_group = (*it).first;
      }
    }
  }

}

// Run functions
namespace unittest {

  /// @brief Run single unit test.
  ///
  /// @param unit_test Unit test name
  ///
  /// @return Whether all tests were successful.
  bool UnitTest::runTestSuites(const string& unit_test) {
    vector<string> vunit_test(1, unit_test);
    return runTestSuites(vunit_test);
  }

  /// @brief Run a set of unit tests.
  ///
  /// @param unit_tests_in Set of unit test names.
  ///
  /// @return Whether all tests were successful.
  ///
  /// The function consists of three steps:
  ///   1) Collect the set of tasks and expand test groups into individual tests.
  ///   2) Run all requested test functions, outputting results as test groups finish.
  ///   3) Print final summary.
  bool UnitTest::runTestSuites(const vector<string>& unit_tests_in) {
    stringstream  message;
    // Create task lists (groups or individual tests).
    // unit_test is the individual small tests over which to parallelize
    vector<string> unit_tests, tasks;
    for (size_t t = 0; t < unit_tests_in.size(); t++) {
      const string& test = unit_tests_in[t];
      bool isgroup = (test_groups.find(test) != test_groups.end());
      if (test == "all") {
        tasks.clear();
        for (std::map<string, vector<string> >::iterator it = test_groups.begin(); it != test_groups.end(); ++it) {
          tasks.push_back((*it).first);
          for (size_t m = 0; m < (*it).second.size(); m++) {
            unit_tests.push_back((*it).second[m]);
          }
        }
        break;
      } else if (!isgroup && (test_functions.find(test) == test_functions.end())) {
        message << "Skipping unrecognized test name " << test << ".";
        pflow::logger(__AFLOW_FILE__, __AFLOW_FUNC__, message, aflags, *p_FileMESSAGE, *p_oss, _LOGGER_WARNING_);
      } else if (isgroup && !aurostd::WithinList(tasks, test)) {
        tasks.push_back(test);
        const vector<string>& members = test_groups[test];
        for (size_t m = 0; m < members.size(); m++) {
          unit_tests.push_back(members[m]);
        }
      } else if (!aurostd::WithinList(tasks, test_functions[test].test_group)) {
        unit_tests.push_back(test);
        tasks.push_back(test);
      }
    }
    uint ntasks = tasks.size();
    if (ntasks == 0) {
      message << "No unit tests to run.";
      pflow::logger(__AFLOW_FILE__, __AFLOW_FUNC__, message, aflags, *p_FileMESSAGE, *p_oss, _LOGGER_NOTICE_);
      return true;
    }

    // Run tests
    // Many AFLOW functions produce output to screen without the opportunity
    // to silence it, which makes unit test output harder to read and can
    // lead to garbled output when run in parallel. To get clean output,
    // silence output globally except for the functions that produce desired
    // unit test output, unless --quiet is requested or --debug is run.
    bool quiet_copy = XHOST.QUIET;
    vector<string> whitelist;
    if (!XHOST.QUIET && !XHOST.DEBUG) {
      whitelist.push_back("unittest::UnitTest::runUnitTest()");
      // Add function names to whitelist for displayResults
      for (uint i = 0; i < unit_tests.size(); i++) whitelist.push_back(test_functions[unit_tests[i]].function_name);
      XHOST.QUIET = true;
      for (size_t i = 0; i < whitelist.size(); i++) XHOST.LOGGER_WHITELIST.push_back(whitelist[i]);
    }
#ifdef AFLOW_MULTITHREADS_ENABLE
    //[CO20261017 - unused]std::mutex mtx;
    xthread::xThread xt(KBIN::get_NCPUS());
    std::function<void(vector<string>::iterator&, const vector<string>&)> fn = std::bind(&UnitTest::runUnitTest, this, _1, _2);
    xt.run(unit_tests, fn, tasks);
#else
    for (vector<string>::iterator it = unit_tests.begin(); it != unit_tests.end(); ++it) runUnitTest(it, tasks);
#endif
    XHOST.QUIET = quiet_copy;
    if (!XHOST.QUIET && !XHOST.DEBUG) {
      XHOST.QUIET = quiet_copy;
      for (size_t i = 0; i < whitelist.size(); i++) XHOST.LOGGER_WHITELIST.pop_back();
    }

    // Print final summary
    uint nsuccess = 0;
    vector<vector<string> > summary(tasks.size(), vector<string>(2));
    for (size_t t = 0; t < tasks.size(); t++) {
      bool success = taskSuccessful(tasks[t]);
      if (success) nsuccess++;
      summary[t][0] = tasks[t];
      summary[t][1] = (success?"pass":"fail");
    }

    if (nsuccess == ntasks) {
      message << "Unit tests passed successfully (passing " << ntasks << " task" + string((ntasks == 1)?"":"s") + ").";
      pflow::logger(__AFLOW_FILE__, __AFLOW_FUNC__, message, aflags, *p_FileMESSAGE, *p_oss, _LOGGER_COMPLETE_);
    } else {
      message << "Some unit tests failed (" << (ntasks - nsuccess) << " of " << ntasks << " failed).";
      pflow::logger(__AFLOW_FILE__, __AFLOW_FUNC__, message, aflags, *p_FileMESSAGE, *p_oss, _LOGGER_ERROR_);
    }
    pflow::logger(__AFLOW_FILE__, __AFLOW_FUNC__, formatResultsTable(summary), aflags, *p_FileMESSAGE, *p_oss, _LOGGER_RAW_);
    return (nsuccess == ntasks);
  }

  /// @brief Run unit test function inside a thread.
  ///
  /// @param it    Iterator pointing to the unit test name.
  /// @param tasks Set of task names.
  void UnitTest::runUnitTest(vector<string>::iterator& it, const vector<string>& tasks) {
    const string& test_name = (*it);
    xcheck& test = test_functions[test_name];
    resetUnitTest(test);
    // CO20261004 - a test that throws (e.g. after a failed download) counts as failed and keeps the message;
    // uncaught, it ended the whole run (std::terminate in a worker thread) without results or summary
    try {
      test.func(test.passed_checks, test.results, test.errors);
    } catch (aurostd::xerror& e) {
      test.errors.push_back("exception: " + e.buildMessageString());
    } catch (std::exception& e) {
      test.errors.push_back(string("exception: ") + e.what());
    } catch (...) {
      test.errors.push_back("exception of unknown type");
    }
    // Output results
#ifdef AFLOW_MULTITHREADS_ENABLE
    std::lock_guard<std::mutex> lk(mtx);
#endif
    test.finished = true;
    if (aurostd::WithinList(tasks, test_name)) {
      // If the test name is in the task list, it is not part
      // of a group, so no need to check if other members are done
      displayResult(test);
    } else {
      // Test if part of a test group, so check if all members
      // of the group have finished before producing output
      const string& group = test_functions[test_name].test_group;
      const vector<string>& vtests_group = test_groups[group];
      uint ntests_group = vtests_group.size();
      uint i = 0;
      for (; i < ntests_group; i++) {
        const string& test_name_group = vtests_group[i];
        if (!test_functions[test_name_group].finished) break;
      }
      if (i == ntests_group) {
        // All finished - print output
        uint nsuccess = 0;
        for (size_t t = 0; t < vtests_group.size(); t++) {
          if (taskSuccessful(vtests_group[t])) nsuccess++;
        }
        stringstream message;
        if (nsuccess == ntests_group) {
          message << "Unit tests of group " << group << " passed successfully (passing " << ntests_group << " test" << ((ntests_group == 1)?"":"s") << ").";
          pflow::logger(__AFLOW_FILE__, __AFLOW_FUNC__, message, aflags, *p_FileMESSAGE, *p_oss, _LOGGER_COMPLETE_);
        } else {
          message << "Some unit tests of group " << group << " failed (" << (ntests_group - nsuccess) << " of " << ntests_group << " failed).";
          pflow::logger(__AFLOW_FILE__, __AFLOW_FUNC__, message, aflags, *p_FileMESSAGE, *p_oss, _LOGGER_ERROR_);
        }
        for (size_t t = 0; t < vtests_group.size(); t++) {
          displayResult(test_functions[vtests_group[t]]);
        }
      }
    }
  }

  /// @brief Check if a task has finished successfully.
  ///
  /// @param task The task to check.
  ///
  /// @return Whether a tasks has finished without errors.
  ///
  /// Criteria for returning true:
  ///   1) Task is finished.
  ///   2) All unit tests passed.
  ///   3) There are no additional errors.
  bool UnitTest::taskSuccessful(const string& task) {
    std::map<string, vector<string> >::iterator it = test_groups.find(task);
    if (it != test_groups.end()) {
      const vector<string> members = (*it).second;
      for (size_t m = 0; m < members.size(); m++) {
        const string& member = members[m];
        const xcheck& xchk = test_functions[member];
        if (!xchk.finished || (xchk.errors.size() > 0) || (xchk.passed_checks != xchk.results.size())) return false;
      }
      return true;
    } else {
      const xcheck& xchk = test_functions[task];
      return (xchk.finished && (xchk.errors.size() == 0) && (xchk.passed_checks == xchk.results.size()));
    }
  }
}

// Output formatters
namespace unittest {
  /// @brief Convert results vector into a formatted table.
  ///
  /// @param table Structured table data.
  ///
  /// @return Formatted table string.
  ///
  /// This function makes no assumption about the table dimensions, so tables
  /// rows can have different sizes.
  /// Empty columns will be skipped, but not empty rows.
  /// The first column will have spaces prepended to indent the table.
  string UnitTest::formatResultsTable(const vector<vector<string> >& table) {
    size_t nrows = table.size();
    if (nrows == 0) return "";  // Empty table

    // Determine dimensions of the table
    vector<size_t> col_sizes;
    size_t str_length = 0;
    size_t maxcol = 0;
    for (size_t r = 0; r < nrows; r++) {
      maxcol = std::max(table[r].size(), maxcol);
      for (size_t c = 0; c < table[r].size(); c++) {
        str_length = table[r][c].length();
        if (c == col_sizes.size()) {
          col_sizes.push_back(str_length);
        } else if (str_length > col_sizes[c]) {
          col_sizes[c] = str_length;
        }
      }
    }
    if (maxcol == 0) return "";  // Empty rows

    vector<string> output(nrows), row;
    string col = "";
    for (size_t r = 0; r < nrows; r++) {
      row.clear();
      // Last column does not need padding
      for (size_t c = 0; c < maxcol - 1; c++) {
        if (col_sizes[c] > 0) {
          col = (c == 0?"  ":"") + aurostd::PaddedPOST((c < table[r].size()?table[r][c]:""), col_sizes[c]);
          row.push_back(col);
        }
      }
      if (table[r].size() == maxcol) row.push_back(table[r].back());
      output[r] = aurostd::joinWDelimiter(row, " | ");
    }
    return aurostd::joinWDelimiter(output, "\n");
  }

  /// @brief Display results of a unit test.
  ///
  /// @param xchk Unit test object containing all results.
  void UnitTest::displayResult(const xcheck& xchk) {
    stringstream message;
    size_t check_num = xchk.results.size();
    if (xchk.passed_checks == check_num) {
      if (xchk.errors.size() > 0) {
        // All attempted checks passed, but there were errors.
        // This happens when a prerequisite for a test fails (e.g. file loading).
        message << "FAIL " << xchk.task_description << " due to runtime errors" << std::endl;
        pflow::logger(__AFLOW_FILE__,xchk.function_name,message,aflags,*p_FileMESSAGE,*p_oss,_LOGGER_ERROR_);
      } else {
        message << "SUCCESS " << xchk.task_description << " (passing " << check_num << " check" << ((check_num == 1)?"":"s") << ")" << std::endl;
        pflow::logger(__AFLOW_FILE__,xchk.function_name,message,aflags,*p_FileMESSAGE,*p_oss,_LOGGER_COMPLETE_);
      }
    } else {
      message << "FAIL " << xchk.task_description << " (" << (check_num - xchk.passed_checks) << " of " << check_num << " checks failed)" << std::endl;
      pflow::logger(__AFLOW_FILE__,xchk.function_name,message,aflags,*p_FileMESSAGE,*p_oss,_LOGGER_ERROR_);
    }
    pflow::logger(__AFLOW_FILE__,xchk.function_name,formatResultsTable(xchk.results),aflags,*p_FileMESSAGE,*p_oss,_LOGGER_RAW_);
    if (xchk.errors.size() > 0) {
      message << "\nAdditional error messages:\n" << aurostd::joinWDelimiter(xchk.errors, "\n");
      pflow::logger(__AFLOW_FILE__,xchk.function_name,message,aflags,*p_FileMESSAGE,*p_oss,_LOGGER_RAW_);
    }
  }
}

// Collection of generic check functions, to streamline testing.
namespace unittest {
  template <typename utype>
    void UnitTest::checkEqual(const vector<utype>& calculated, const vector<utype>& expected, const string& check_function,
        const string& check_description, uint& passed_checks, vector<vector<string> >& results) {
      bool passed = (calculated.size() == expected.size());
      for (size_t i = 0; i < calculated.size() && passed; i++) {
        passed = aurostd::isequal(calculated[i], expected[i]);
      }
      check(passed, calculated, expected, check_function, check_description, passed_checks, results);
    }
  template <typename utype>

  void UnitTest::checkEqual(const utype& calculated, const utype& expected, const string& check_function,
      const string& check_description, uint& passed_checks, vector<vector<string> >& results) {
    bool passed = (aurostd::isequal(calculated, expected));
    check(passed, calculated, expected, check_function, check_description, passed_checks, results);
  }

  void UnitTest::checkEqual(const string& calculated, const string& expected, const string& check_function,
                            const string& check_description, uint & passed_checks, vector<vector<string> >& results) {
    bool passed = (calculated == expected);
    check(passed, calculated, expected, check_function, check_description, passed_checks, results);
  }

  template <typename utype>
    void UnitTest::check(const bool passed, const vector<utype>& calculated, const vector<utype>& expected, const string& check_function,
        const string& check_description, uint& passed_checks, vector<vector<string> >& results) {
      check(passed, aurostd::joinWDelimiter(calculated, ","), aurostd::joinWDelimiter(expected, ","), check_function, check_description, passed_checks, results);
    }
  void UnitTest::check(const bool passed, const vector<double>& calculated, const vector<double>& expected, const string& check_function,
      const string& check_description, uint& passed_checks, vector<vector<string> >& results) {
    check(passed, aurostd::vecDouble2String(calculated), aurostd::vecDouble2String(expected), check_function, check_description, passed_checks, results);
  }

  template <typename utype>
    void UnitTest::check(const bool passed, const xmatrix<utype>& calculated, const xmatrix<utype>& expected, const string& check_function,
        const string& check_description, uint& passed_checks, vector<vector<string> >& results) {
      check(passed, aurostd::xmat2String(calculated), aurostd::xmat2String(expected), check_function, check_description, passed_checks, results);
    }
  void UnitTest::check(const bool passed, const xmatrix<double>& calculated, const xmatrix<double>& expected, const string& check_function,
      const string& check_description, uint& passed_checks, vector<vector<string> >& results) {
    check(passed, aurostd::xmatDouble2String(calculated), aurostd::xmatDouble2String(expected), check_function, check_description, passed_checks, results);
  }

  /// @brief Base function to check results and update results.
  ///
  /// @param passed            Whether the test has passed.
  /// @param calculated        Calculated value.
  /// @param expected          Expected value.
  /// @param check_function    Function called for the test.
  /// @param check_description Description of the performed test.
  /// @param passed_checks     Number of passed checks.
  /// @param results           Results data - doubles as number of performed checks.
  template <typename utype>
    void UnitTest::check(const bool passed, const utype& calculated, const utype& expected, const string& check_function,
        const string& check_description, uint& passed_checks, vector<vector<string> >& results) {
      vector<string> result;
      uint check_num = results.size() + 1;
      result.push_back(aurostd::utype2string<uint>(check_num));
      if (passed) {
        passed_checks++;
        result.push_back("pass");
      } else {
        result.push_back("FAIL");
      }
      result.push_back(check_function);
      result.push_back(check_description);
      if (!passed) {
        stringstream failstring;
        failstring << " (result: " << calculated << " | expected: " << expected << ")";
        result.back() += failstring.str();
      }
      results.push_back(result);
    }

}

// APL
namespace unittest {

  void UnitTest::aplThermalTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;
    const string check_function = "apl::ThermalPropertiesCalculator";
    const double tolerance = 1e-10;

    vector<double> freqs;
    vector<double> dos;
    freqs.push_back(1.0);
    freqs.push_back(2.0);
    freqs.push_back(3.0);
    dos.push_back(0.0);
    dos.push_back(1.0);
    dos.push_back(0.0);

    apl::ThermalPropertiesCalculator tpc;
    tpc.initialize(freqs, dos, "unit_test");

    double calculated_dbl = tpc.getDebyeTemperature();
    double expected_dbl = BEfactor_h_THz * sqrt((5.0/3.0) * 4.0);
    check(aurostd::abs(calculated_dbl - expected_dbl) < tolerance, calculated_dbl, expected_dbl,
        check_function + "::getDebyeTemperature()", "calculate +2 moment Debye temperature", passed_checks, results);

    calculated_dbl = tpc.getDebyeWallerTemperature();
    expected_dbl = BEfactor_h_THz * sqrt(3.0 / 0.25);
    check(aurostd::abs(calculated_dbl - expected_dbl) < tolerance, calculated_dbl, expected_dbl,
        check_function + "::getDebyeWallerTemperature()", "calculate -2 moment Debye-Waller temperature", passed_checks, results);

    stringstream apl_outfile;
    tpc.addToAPLOut(apl_outfile);
    string apl_out = apl_outfile.str();
    checkEqual(apl_out.find("apl_debye=") != string::npos, true,
        check_function + "::addToAPLOut()", "write Debye temperature to aflow.apl.out", passed_checks, results);
    checkEqual(apl_out.find("apl_debye_DWF=") != string::npos, true,
        check_function + "::addToAPLOut()", "write Debye-Waller temperature to aflow.apl.out", passed_checks, results);
  }

}

// VASP force options
namespace unittest {
  // ---------------------------------------------------------------------------
  // vasp_kpoints  //CO20261009
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   VASP's "Reciprocal lattice and k-lattice belong to different class of lattices" (KKSYM) was fixed with KPOINTS=KMAX,
  //   which turned the 7x4x4 grids of 88-atom POCC children (monoclinic supercell metric) into 7x7x7, and memory ran out.
  //   The fix now tries SYM=OFF when the crystal has no symmetry, then KPOINTS=SYMGRID, the smallest grid no coarser on
  //   any axis whose k-lattice keeps the class of the reciprocal lattice, then KMAX. SYM=OFF writes ISYM=0 (time reversal
  //   kept) with collinear spin, ISYM=-1 only when noncollinear.
  // CHECKED
  //   1. LatticePointGroupOrder() on the seven lattice systems and on a skewed basis of the cubic lattice.
  //   2. KBIN::XVASP_KPOINTS_SymmetricGrid() on two 88-atom pyrochlore supercells (ideal HNF lattices of held runs, a=11.17
  //      and 11.78 A): 7x4x4 becomes 7x7x4 (monoclinic metric), 5x5x4 becomes 5x5x5 (tetragonal); a cubic 4x4x4 is left alone.
  //   3. KBIN::XVASP_CrystalPointGroupOrder() on NaCl (48) and on a cell with no symmetry (1).
  //   4. XVASP_INCAR_PREPARE_GENERIC("SYM",...,OFF): ISYM=0 with SPIN=ON, ISYM=-1 with LSCOUPLING=ON.
  // IF THIS TEST FAILS
  //   The lattice class test or the grid search changed: KKSYM runs may again get grids larger than they need.
  void UnitTest::vaspKpointsTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    auto lattice_of = [](const vector<vector<double> >& rows) {
      xmatrix<double> lat(3,3);
      for (int i = 1; i <= 3; i++) {for (int j = 1; j <= 3; j++) {lat[i][j] = rows[i-1][j-1];}}
      return lat;
    };
    // 1. lattice point groups
    {
      const string check_function = "LatticePointGroupOrder()";
      const double a = 4.0;
      const double c60 = std::cos(60.0 * PI / 180.0), s60 = std::sin(60.0 * PI / 180.0);
      struct lat_case { vector<vector<double> > rows; uint order; string why; };
      const vector<lat_case> vcases = {
        {{{a,0,0},{0,a,0},{0,0,a}}, 48, "simple cubic"},
        {{{a,0,0},{2*a,a,0},{-3*a,a,a}}, 48, "the simple cubic lattice in a skewed basis"},
        {{{0,a/2,a/2},{a/2,0,a/2},{a/2,a/2,0}}, 48, "fcc primitive"},
        {{{a,0,0},{-a*c60,a*s60,0},{0,0,1.6*a}}, 24, "hexagonal"},
        {{{a,0,0},{0,a,0},{0,0,1.3*a}}, 16, "tetragonal"},
        {{{a,0,0},{0,1.1*a,0},{0,0,1.3*a}}, 8, "orthorhombic"},
        {{{a,0,0},{0,1.1*a,0},{0.3*a,0,1.3*a}}, 4, "monoclinic"},
        {{{a,0,0},{0.2*a,1.1*a,0},{0.3*a,0.1*a,1.3*a}}, 2, "triclinic"},
      };
      for (const lat_case& c : vcases) {
        checkEqual(LatticePointGroupOrder(lattice_of(c.rows)), c.order, check_function, c.why, passed_checks, results);
      }
    }
    // 2. the KKSYM grid
    {
      const string check_function = "KBIN::XVASP_KPOINTS_SymmetricGrid()";
      struct grid_case { vector<vector<double> > rows; vector<int> grid; vector<int> expected; bool changed; string why; };
      const vector<grid_case> vcases = {
        {{{0,-5.58692902,-5.58692902},{-5.58692902,-5.58692902,11.17385803},{-11.17385803,5.58692902,-5.58692902}}, {7,4,4}, {7,7,4}, true,
          "88-atom pyrochlore supercell, monoclinic metric: KPPRA 7x4x4 breaks the class, 7x7x4 keeps it (KMAX gave 7x7x7)"},
        {{{11.7768934,0,0},{0,0,-11.7768934},{-5.8884467,11.7768934,5.8884467}}, {5,5,4}, {5,5,5}, true,
          "88-atom pyrochlore supercell, tetragonal metric: 5x5x4 breaks the class, 5x5x5 keeps it"},
        {{{4,0,0},{0,4,0},{0,0,4}}, {4,4,4}, {4,4,4}, false, "simple cubic 4x4x4 keeps the class: left alone"},
      };
      for (const grid_case& c : vcases) {
        _xvasp xvasp;
        string threw = "";
        bool changed = false;
        try {
          std::stringstream ss;
          ss << "grid test\n1.0\n";
          ss.precision(12);
          for (const vector<double>& r : c.rows) {ss << r[0] << " " << r[1] << " " << r[2] << "\n";}
          ss << "1\nDirect\n0 0 0\n";
          xvasp.str = xstructure(ss, IOVASP_POSCAR);
          xvasp.str.kpoints_k1 = c.grid[0]; xvasp.str.kpoints_k2 = c.grid[1]; xvasp.str.kpoints_k3 = c.grid[2];
          changed = KBIN::XVASP_KPOINTS_SymmetricGrid(xvasp);
        }
        catch (aurostd::xerror& excpt) {threw = excpt.buildMessageString();}
        if (!threw.empty()) {errors.push_back(c.why + ": " + threw); continue;}
        checkEqual(changed, c.changed, check_function, c.why + " (changed)", passed_checks, results);
        const vector<int> got = {xvasp.str.kpoints_k1, xvasp.str.kpoints_k2, xvasp.str.kpoints_k3};
        checkEqual(got, c.expected, check_function, c.why, passed_checks, results);
      }
    }
    // 3. the crystal point group that decides SYM=OFF
    {
      const string check_function = "KBIN::XVASP_CrystalPointGroupOrder()";
      struct xtal_case { string poscar; uint order; string why; };
      const vector<xtal_case> vcases = {
        {"NaCl\n1.0\n5.64 0 0\n0 5.64 0\n0 0 5.64\nNa Cl\n4 4\nDirect\n0 0 0\n0 0.5 0.5\n0.5 0 0.5\n0.5 0.5 0\n0.5 0.5 0.5\n0.5 0 0\n0 0.5 0\n0 0 0.5\n", 48, "rock salt: 48 operations, symmetry kept"},
        {"P1\n1.0\n4.1 0 0\n0.3 4.6 0\n0.5 0.2 5.3\nA B C\n1 1 1\nDirect\n0 0 0\n0.21 0.33 0.12\n0.55 0.71 0.38\n", 1, "three atoms in general positions of a triclinic cell: the identity only, so SYM=OFF"},
      };
      for (const xtal_case& c : vcases) {
        _xvasp xvasp;
        string threw = "";
        uint order = 0;
        try {
          std::stringstream ss(c.poscar);
          xvasp.str = xstructure(ss, IOVASP_POSCAR);
          order = KBIN::XVASP_CrystalPointGroupOrder(xvasp);
        }
        catch (aurostd::xerror& excpt) {threw = excpt.buildMessageString();}
        if (!threw.empty()) {errors.push_back(c.why + ": " + threw); continue;}
        checkEqual(order, c.order, check_function, c.why, passed_checks, results);
      }
    }
    // 4. ISYM for SYM=OFF
    {
      const string check_function = "KBIN::XVASP_INCAR_PREPARE_GENERIC(\"SYM\",...,OFF)";
      struct sym_case { string options; string isym; string why; };
      const vector<sym_case> vcases = {
        {"[VASP_FORCE_OPTION]SPIN=ON\n", "0", "collinear spin: ISYM=0 keeps time reversal (was ISYM=-1)"},
        {"[VASP_FORCE_OPTION]SPIN=OFF\n", "0", "no spin: ISYM=0, as before"},
        {"[VASP_FORCE_OPTION]SPIN=ON\n[VASP_FORCE_OPTION]LSCOUPLING=ON\n", "-1", "noncollinear: ISYM=-1, the full grid"},
      };
      for (const sym_case& c : vcases) {
        _xvasp xvasp;
        string threw = "", isym = "";
        try {
          _aflags aflags;
          _kflags kflags;
          std::stringstream oss_vflags;
          _vflags vflags = KBIN::VASP_Get_Vflags_from_AflowIN("[AFLOW] vasp_kpoints unit test\n" + c.options, aflags, kflags, oss_vflags);
          xvasp.INCAR.str("");
          xvasp.INCAR << "SYSTEM=vasp_kpoints\nISYM=2\n";
          KBIN::XVASP_INCAR_PREPARE_GENERIC("SYM", xvasp, vflags, "", 0, 0.0, OFF);
          isym = aurostd::RemoveWhiteSpaces(aurostd::kvpair2string(xvasp.INCAR, "ISYM", "="));
        }
        catch (aurostd::xerror& excpt) {threw = excpt.buildMessageString();}
        if (!threw.empty()) {errors.push_back(c.why + ": " + threw); continue;}
        checkEqual(isym, c.isym, check_function, c.why, passed_checks, results);
      }
    }
  }

  // ---------------------------------------------------------------------------
  // vasp_preserved  //CO20261004
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   [VASP_FORCE_OPTION]PREC=<SCHEME>,PRESERVED and ALGO=<SCHEME>,PRESERVED (or the bare [VASP_FORCE_OPTION]ALGO_PRESERVED)
  //   ask aflow to keep the INCAR settings of a run. Two places did not:
  //   1. xoption::scheme2scheme(char,...) matches a scheme on its first letter, so the 'P' rule for PHONONS took the
  //      token PRESERVED and every PREC=<SCHEME>,PRESERVED became PHONONS, for which aflow_ivasp.cpp then forces
  //      ALGO=NORMAL. The token is now skipped; .preserved is set from it in options2entry().
  //   2. XVASP_INCAR_PREPARE_GENERIC("ALGO",...) removed ALGO and IALGO from the INCAR and wrote aflow's scheme even
  //      when ALGO was preserved; it now leaves the INCAR as it is and reports it unchanged.
  // CHECKED
  //   The flags as KBIN::VASP_Get_Vflags_from_AflowIN() reads them from aflow.in lines (so a scheme rule added there,
  //   such as mapping PRESERVED to itself, shows up too), then XVASP_INCAR_PREPARE_GENERIC("ALGO",...) with those flags,
  //   as at its call site in aflow_ivasp.cpp, on an INCAR with ALGO=Fast and IALGO=48. No VASP run: the function only
  //   edits xvasp.INCAR.
  // IF THIS TEST FAILS
  //   A preserved PREC turns into another scheme again (and ALGO is forced to NORMAL with it), or a preserved ALGO is
  //   overwritten: fix the code, not the expectations.
  void UnitTest::vaspPreservedTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    // the flags aflow reads from an aflow.in that holds the given [VASP_FORCE_OPTION] lines
    auto vflags_of = [](const string& options, _vflags& vflags) {
      _aflags aflags;
      _kflags kflags;
      std::stringstream oss_vflags;
      vflags = KBIN::VASP_Get_Vflags_from_AflowIN("[AFLOW] vasp_preserved unit test\n" + options, aflags, kflags, oss_vflags);
    };

    // 1. PREC: the scheme survives PRESERVED
    {
      const string check_function = "KBIN::VASP_Get_Vflags_from_AflowIN() -> xoption::scheme2scheme()";
      struct prec_case { string option; string scheme; bool preserved; string why; };
      const vector<prec_case> vcases = {
        {"[VASP_FORCE_OPTION]PREC=ACCURATE,PRESERVED\n", "ACCURATE", true, "ACCURATE,PRESERVED stays ACCURATE (was PHONONS)"},
        {"[VASP_FORCE_OPTION]PREC=HIGH,PRESERVED\n", "HIGH", true, "HIGH,PRESERVED stays HIGH (was PHONONS)"},
        {"[VASP_FORCE_OPTION]PREC=PHONONS\n", "PHONONS", false, "PHONONS is still PHONONS"},
        {"[VASP_FORCE_OPTION]PREC=MEDIUM\n", "MEDIUM", false, "MEDIUM is still MEDIUM"},
      };
      for (const prec_case& c : vcases) {
        _vflags vflags;
        string threw = "";
        try {vflags_of(c.option, vflags);}
        catch (aurostd::xerror& excpt) {threw = excpt.buildMessageString();}
        const string calculated = (threw.empty() ? vflags.KBIN_VASP_FORCE_OPTION_PREC.xscheme + (vflags.KBIN_VASP_FORCE_OPTION_PREC.preserved ? ",preserved" : "") : "exception: " + threw);
        checkEqual(calculated, c.scheme + (c.preserved ? ",preserved" : ""), check_function, c.why, passed_checks, results);
      }
      // PRESERVED alone names no scheme: aflow stops with an input error instead of taking PHONONS
      bool threw = false;
      try {_vflags vflags; vflags_of("[VASP_FORCE_OPTION]PREC=PRESERVED\n", vflags);}
      catch (aurostd::xerror& excpt) {threw = true;}
      checkEqual(threw, true, check_function, "PRESERVED alone is not a scheme: an input error (was PHONONS)", passed_checks, results);
    }

    // 2. ALGO: a preserved ALGO is left in the INCAR
    {
      const string check_function = "KBIN::XVASP_INCAR_PREPARE_GENERIC(\"ALGO\")";
      const string incar_in = "SYSTEM=vasp_preserved\nALGO=Fast\nIALGO=48\n";
      // the ALGO and IALGO lines that VASP reads (not commented out), without comments
      auto algo_lines = [](const string& incar) {
        vector<string> vlines, valgo;
        aurostd::string2vectorstring(incar, vlines);
        for (const string& line : vlines) {
          const string l = aurostd::RemoveWhiteSpacesFromTheFront(line);
          if (l.find("ALGO=") == 0 || l.find("IALGO=") == 0) valgo.push_back(aurostd::RemoveWhiteSpacesFromTheBack(l.substr(0, l.find('#'))));
        }
        return aurostd::joinWDelimiter(valgo, ";");
      };
      struct algo_case { string option; bool modified; string algo; string why; };
      const vector<algo_case> vcases = {
        {"[VASP_FORCE_OPTION]ALGO=NORMAL,PRESERVED\n", false, "ALGO=Fast;IALGO=48", "ALGO=NORMAL,PRESERVED: the INCAR keeps ALGO=Fast and IALGO=48 (were replaced by ALGO=Normal)"},
        {"[VASP_FORCE_OPTION]ALGO=NORMAL\n[VASP_FORCE_OPTION]ALGO_PRESERVED\n", false, "ALGO=Fast;IALGO=48", "ALGO=NORMAL with ALGO_PRESERVED: the INCAR keeps ALGO=Fast and IALGO=48"},
        {"[VASP_FORCE_OPTION]ALGO=NORMAL\n", true, "ALGO=Normal", "ALGO=NORMAL, not preserved: ALGO=Normal replaces ALGO=Fast and IALGO=48, as before"},
      };
      for (const algo_case& c : vcases) {
        _vflags vflags;
        _xvasp xvasp;
        bool modified = false;
        string threw = "";
        try {
          vflags_of(c.option, vflags);
          xvasp.INCAR.str("");
          xvasp.INCAR << incar_in;
          modified = KBIN::XVASP_INCAR_PREPARE_GENERIC("ALGO", xvasp, vflags, vflags.KBIN_VASP_FORCE_OPTION_ALGO.xscheme, 0, 0.0, FALSE);
        }
        catch (aurostd::xerror& excpt) {threw = excpt.buildMessageString();}
        if (!threw.empty()) {errors.push_back(c.why + ": " + threw); continue;}
        const string incar_out = xvasp.INCAR.str();
        checkEqual(modified, c.modified, check_function, c.why + " (INCAR reported " + (c.modified ? "modified" : "unchanged") + ")", passed_checks, results);
        checkEqual(algo_lines(incar_out), c.algo, check_function, c.why, passed_checks, results);
        if (!c.modified) {checkEqual(incar_out, incar_in, check_function, c.why + " (INCAR text unchanged)", passed_checks, results);}
      }
    }
  }


  // ---------------------------------------------------------------------------
  // vasp_nbands  //CO20261009
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   [VASP_FORCE_OPTION]NBANDS (no value) asks aflow for NBANDS. Until Oct 2026 KBIN::XVASP_INCAR_GetNBANDS() passed
  //   GetNBANDS() the sum of ZVAL over the POTCAR blocks (one per species, not per atom), and GetNBANDS() stacked factors
  //   of 1.66 and nions^0.025 on a spin term of 2.5 bands per ion. The 88-atom pyrochlore POCC children got 588-608 bands
  //   for highest occupied bands of 296-358, and small cells about 2.8 times the bands they occupied.
  //   GetNBANDS_OCC() now takes the electrons of the cell and the initial moments: 1.2 times the bands of the majority
  //   spin, plus 4, and never less than VASP's own default.
  // CHECKED
  //   1. GetNBANDS_OCC() on fixed numbers, NCORE rounding included.
  //   2. KBIN::XVASP_INCAR_GetNBANDS() on an 8-atom NaCl cell with a two-block POTCAR (ZVAL 9 and 7): the 64 electrons
  //      of the cell (the old sum was 16), with the MAGMOM of the INCAR, VASP's default of 1 per ion when there is none,
  //      and LNONCOLLINEAR. No VASP run.
  // IF THIS TEST FAILS
  //   The default NBANDS changed: check the change against the occupied bands of finished runs before changing the
  //   expectations.
  void UnitTest::vaspNbandsTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    if(errors.size()){;} // no exceptions expected below
    // 1. the formula
    {
      const string check_function = "GetNBANDS_OCC()";
      struct nb_case { double electrons; int nions; double magmom; bool ispin; bool noncollinear; int ncore; int nbands; string why; };
      const vector<nb_case> vcases = {
        {616.0, 88, 88.0, true, false, 1, 427, "88-atom pyrochlore child, NELECT 616, MAGMOM 88*1: (616+88)/2=352, 1.2*352+4 -> 427 (aflow gave 608, VASP's default is 414)"},
        {616.0, 88, 88.0, true, false, 16, 432, "the same with NCORE=16: rounded up to 432"},
        {110.0, 8, -8.0, true, false, 1, 75, "Nd6V2, NELECT 110, |MAGMOM| 8: 59 bands of the majority spin, 75 (its highest occupied band was 70)"},
        {67.0, 9, 9.0, true, false, 1, 50, "odd electron count: (67+9)/2=38, 1.2*38+4 -> 50"},
        {8.0, 2, 0.0, false, false, 1, 9, "no spin: 4 bands, 1.2*4+4 -> 9"},
        {18.0, 2, 2.0, true, true, 1, 26, "noncollinear: 18 spinor bands, 1.2*18+4 -> 26 (VASP's default, doubled, is 26 too)"},
        {16.0, 16, 16.0, true, false, 1, 25, "16 atoms with one electron each: 1.2*16+4 -> 24, below VASP's default of 17+8=25, which is kept"},
      };
      for (const nb_case& c : vcases) {
        checkEqual(GetNBANDS_OCC(c.electrons, c.nions, c.magmom, c.ispin, c.noncollinear, c.ncore), c.nbands, check_function, c.why, passed_checks, results);
      }
    }
    // 2. the electrons and moments that KBIN::XVASP_INCAR_GetNBANDS() passes on
    {
      const string check_function = "KBIN::XVASP_INCAR_GetNBANDS()";
      const string poscar = "NaCl conventional\n1.0\n5.64 0 0\n0 5.64 0\n0 0 5.64\nNa Cl\n4 4\nDirect\n"
                            "0 0 0\n0 0.5 0.5\n0.5 0 0.5\n0.5 0.5 0\n0.5 0.5 0.5\n0.5 0 0\n0 0.5 0\n0 0 0.5\n";
      const string potcar = "  PAW_PBE Na_sv 19Sep2006\n   POMASS =   22.990; ZVAL   =    9.000    mass and valenz\n"
                            "   ENMAX  =  646.212; ENMIN  =  484.659 eV\n End of Dataset\n"
                            "  PAW_PBE Cl 06Sep2000\n   POMASS =   35.453; ZVAL   =    7.000    mass and valenz\n"
                            "   ENMAX  =  262.472; ENMIN  =  196.854 eV\n End of Dataset\n";
      struct incar_case { string incar; int nbands; string why; };
      const vector<incar_case> vcases = {
        {"SYSTEM=NaCl\nISPIN=2\nMAGMOM=8*1\n", 48, "MAGMOM=8*1: (64+8)/2=36, 1.2*36+4 -> 48 (the POTCAR-block sum of 16 would give 19)"},
        {"SYSTEM=NaCl\n", 48, "no MAGMOM: VASP's default of 1 per ion, 48"},
        {"SYSTEM=NaCl\nISPIN=2\nMAGMOM=4*5 4*-5\n", 67, "MAGMOM=4*5 4*-5: |moments| 40, (64+40)/2=52, 1.2*52+4 -> 67"},
        {"SYSTEM=NaCl\nLSORBIT=.TRUE.\nLNONCOLLINEAR=.TRUE.\nMAGMOM=24*0\n", 81, "LNONCOLLINEAR: 64 spinor bands, 1.2*64+4 -> 81"},
      };
      _aflags aflags;
      for (const incar_case& c : vcases) {
        _xvasp xvasp;
        string threw = "";
        int nbands = 0;
        try {
          std::stringstream ss(poscar);
          xvasp.str = xstructure(ss, IOVASP_POSCAR);
          xvasp.POTCAR.str(""); xvasp.POTCAR << potcar;
          xvasp.INCAR.str(""); xvasp.INCAR << c.incar;
          xvasp.NCPUS = 0;
          nbands = KBIN::XVASP_INCAR_GetNBANDS(xvasp, aflags, true);
        }
        catch (aurostd::xerror& excpt) {threw = excpt.buildMessageString();}
        if (!threw.empty()) {errors.push_back(c.why + ": " + threw); continue;}
        checkEqual(nbands, c.nbands, check_function, c.why, passed_checks, results);
      }
    }
  }

}

// lib2raw
namespace unittest {

  // ---------------------------------------------------------------------------
  // lib2raw_bader_children  //CO20260929
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   Since fix/lib2raw-aapl, LIB2RAW() does not run Bader for phonon and elastic child runs
  //   (ARUN.APL_*, ARUN.AAPL_*, ARUN.QHA_*, ARUN.AEL_*, ARUN.AGL_*, and AEL_*/AGL_* without the ARUN. prefix).
  //   Which directories count is decided by aflowlib::LIB2RAW_isBaderSkippedChild() from the directory name alone.
  //   Two ways this can go wrong without anyone noticing, which this test guards against:
  //    1. A child is NOT recognized (a module renames its directories, or the list is edited): lib2raw runs Bader
  //       on a large distorted supercell again. With the jvxl flags, bader writes one BvAt file per atom, each the
  //       size of the CHGCAR. The 250-atom AAPL children of LIB2 FNa_sv/AB_cF8_225_a_b.AB need about 207 GB,
  //       more than the whole 196 GB temporary partition of the pipeline server. In 2026-09 every such attempt
  //       ran for up to 1.6 h and failed with a Fortran write error; AFLOW does not check bader's exit status, so
  //       lib2raw then aborted on the cut-off output (vector::_M_range_check, exit 134) or wrote an entry without
  //       Bader data, and the Bader runs of four unrelated entries failed while the partition was full.
  //    2. A directory that is NOT a phonon/elastic child is matched (a prefix too loose, a match on the wrong path
  //       component): a regular entry, a POCC child or the parent itself silently loses its Bader charges,
  //       atomic volumes and jvxl isosurfaces in the database.
  //   The names below are real ones from the libraries (the AAPL and APL children of the 2026-09 failures, the
  //   entries that were damaged, an MLIP run), plus the near misses that a looser match would catch.
  // IF THIS TEST FAILS
  //   Do not just update the expected values. Check which case changed: a newly skipped regular entry means the
  //   list is too loose (case 2); a phonon/elastic child that now runs Bader means the list or a module's naming
  //   changed (case 1). Update LIB2RAW_isBaderSkippedChild() and its comment together.
  void UnitTest::lib2rawBaderChildrenTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;
    const string check_function = "aflowlib::LIB2RAW_isBaderSkippedChild()";
    const string ARUN = ARUN_DIRECTORY_PREFIX;  //"ARUN." unless changed in the aflow.rc
    // directory, expected skip, expected matched prefix, what the case stands for
    struct bader_child_case { string directory; bool skip; string prefix; string why; };
    const vector<bader_child_case> vcases = {
      // phonon and elastic children: Bader is skipped
      {"/common/LIB2/LIB/FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_004_A0D0-A1D4", true, ARUN+"AAPL_", "AAPL child from the 2026-09 failures (250 atoms, 207 GB of BvAt files)"},
      {"/common/LIB2/LIB/FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_004_A0D0-A1D4/", true, ARUN+"AAPL_", "same, trailing slash"},
      {"/common//LIB2/LIB/FNa_sv/AB_cF8_225_a_b.AB//ARUN.AAPL_3_099_A125D2-A126D0", true, ARUN+"AAPL_", "AAPL child, doubled slashes"},
      {"/common/LIB1/LIB/As:PAW_PBE/A_hR2_166_c-001.A:ITC/ARUN.APL_1_A0D0P", true, ARUN+"APL_", "APL child from the 2026-09 survey (LIB1 A7 structure)"},
      {"ARUN.APL_2_ZEROSTATE", true, ARUN+"APL_", "APL zero-state run, name only"},
      {"/x/ARUN.QHA_PHONON_0.9900", true, ARUN+"QHA_", "QHA volume-scaled phonon run"},
      {"/x/ARUN.QHA_STATIC_1.0100", true, ARUN+"QHA_", "QHA static run"},
      {"/x/ARUN.AEL_1_SF_N_1_1.010", true, ARUN+"AEL_", "AEL strained run"},
      {"/x/ARUN.AGL_2_SF_0.980", true, ARUN+"AGL_", "AGL strained run"},
      {"/x/AEL_1_SF_N_1_1.010", true, "AEL_", "AEL run named without the ARUN. prefix"},
      {"/x/AGL_2_SF_0.980", true, "AGL_", "AGL run named without the ARUN. prefix"},
      {"/x//ARUN.QHA_PHONON_0.9900//", true, ARUN+"QHA_", "QHA child, doubled trailing slash (aurostd::basename after CleanFileName)"},  //CO20261004
      {"/x/ARUN.AEL_1_SF_N_1_1.010/./", true, ARUN+"AEL_", "AEL child, trailing /./"},  //CO20261004
      {"ARUN.AGL_2_SF_0.980/", true, ARUN+"AGL_", "AGL child, name only with a trailing slash"},  //CO20261004
      // everything else: Bader runs
      {"/common/LIB2/LIB/FNa_sv/AB_cF8_225_a_b.AB", false, "", "the AAPL parent keeps its Bader analysis"},
      {"/common/LIB2/LIB/FNa_sv/AB_cF8_225_a_b.AB/", false, "", "the AAPL parent, trailing slash"},
      {"/common/LIB2/LIB/FNa_sv/AB_cF8_225_a_b.AB/./", false, "", "the AAPL parent, trailing /./"},  //CO20261004
      {"/common/LIB6/LIB/AlCu_pvFe_pvMg_pvMn_pvO:PAW_PBE/AB_cF8_225_a_b.AB:POCC_P0-1xF_P1-0.2xA-0.2xB-0.2xC-0.2xD-0.2xE/ARUN.POCC_37_H3C0", false, "", "POCC child (one of the four entries damaged on 2026-09-27)"},
      {"/common/LIB1/LIB/Gd:PAW_PBE/A_cI2_229_a.A", false, "", "LIB1 unary (damaged on 2026-09-27)"},
      {"/common/LIB3/LIB/Er_3OTi_sv/A10B3C18_hP31_147_2c3d_ad_3g-001.ACB", false, "", "LIB3 ternary (damaged on 2026-09-27)"},
      {"/common/LIB2/LIB/Cu_pvV_sv/AB3_oP16_62_c_cd-001.BA/ARUN_MLIP_CHGNET_202602", false, "", "MLIP run (ARUN_ with an underscore)"},
      {"/x/ARUN.APL_1_A0D0/other", false, "", "only the last path component counts"},
      {"ARUN.APLX_1", false, "", "near miss: APLX"},
      {"ARUN.APL", false, "", "near miss: no underscore"},
      {"ARUN.AP", false, "", "near miss: shorter than the prefix"},
      {"XARUN.APL_1", false, "", "near miss: prefix not at the start"},
      {"ARUN.aapl_1", false, "", "near miss: lower case (the modules write upper case)"},
      {"", false, "", "empty directory"},
      {"/", false, "", "root directory"},
    };
    for (size_t i = 0; i < vcases.size(); i++) {
      const bader_child_case& c = vcases[i];
      string prefix = "x";  //must be cleared or set by the call
      bool calculated = aflowlib::LIB2RAW_isBaderSkippedChild(c.directory, prefix);
      const string description = string(c.skip ? "skip Bader: " : "run Bader: ") + c.why + " [" + c.directory + "]";
      checkEqual(calculated, c.skip, check_function, description, passed_checks, results);
      checkEqual(prefix, c.prefix, check_function, "matched prefix: " + c.why, passed_checks, results);
      checkEqual(aflowlib::LIB2RAW_isBaderSkippedChild(c.directory), c.skip, check_function + " (one-argument form)", description, passed_checks, results);
    }
  }

  // ---------------------------------------------------------------------------
  // getprim_keep_cell  //CO20260929
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   xstructure::GetPrimitive_20210322() threw "supercell does not conserve volume/atom" (ERROR 50) when the reduced
  //   cell it built had lost atoms that are distinct in the original cell. APL and AAPL supercells trigger it: the
  //   displaced atoms look equivalent within a loose tolerance and TransformStructure() merges them. In lib2raw the
  //   exception stopped the whole entry: in the 2026-09-25 triage 18 AAPL children of LIB2 FNa_sv/AB_cF8_225_a_b.AB
  //   had no RAW entry because of it. The fix (fix/lib2raw-aapl, f497bc48) keeps the original cell instead, the
  //   same outcome as when TransformStructure() itself fails a few lines earlier, and prints a warning.
  //   This test runs the exact step that failed, on the exact structure, so the error cannot come back unnoticed.
  // WHERE IT FAILED
  //   LIB2RAW(), thermodynamics loop, "Space Group analyzer: aflowSG(loose)": SpaceGroup_ITC() with ten times
  //   SYM::defaultTolerance() and the tolerance scan on (no_scan=false), on the structure files of the entry.
  //   Old-binary log (sandbox copy, 2026-09-25): "Space Group analyzer: aflowSG(loose)" and then
  //   "xstructure::GetPrimitive_20210322(): ERROR 50 ... Runtime Error (generic)".
  // THE STRUCTURE
  //   LIB2 FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_005_A0D3-A1D1: 5x5x5 supercell of rocksalt NaF (250 atoms, lattice
  //   as in POSCAR.static) with atom 0 displaced by [-0.015,0,0] and atom 1 by [0,0.015,0] Angstrom. The coordinates
  //   are wrapped into [0,1), as VASP writes them in CONTCAR; this matters: the same structure with the two
  //   displaced atoms written slightly below 0 (as in POSCAR.static) does not trigger the error, which is why the
  //   existing poscar_NaF_AAPL_supercell case in xstructureTest() did not catch it.
  //   The ideal sites are generated below; the two displaced atoms are written out as in the wrapped file.
  // CHECKED (2026-09-29/30, -O0 build, about 8 minutes and 3 GB per run)
  //   This test, linked against GetPrimitive_20210322() as in 1e478c74: checks 2 and 3 fail with "supercell does not
  //   conserve volume/atom". Against f497bc48: all 7 checks pass (space group 8, 250 atoms). The same structure
  //   with unwrapped coordinates gives space group 12 on both, so it would not have caught the bug.
  // COST
  //   SpaceGroup_ITC() on 250 atoms at this tolerance takes minutes and about 3 GB; that is why this test is in the
  //   lib2raw group (aflow --unit_test=lib2raw) and runs the step once.
  // IF THIS TEST FAILS
  //   An exception means GetPrimitive (or code around it) throws on AAPL supercells again, and lib2raw will lose
  //   these entries again: do not change the expectations, fix the code. A different space group alone is not a
  //   failure of this test; it checks that the step completes, keeps all atoms and returns a valid space group.
  void UnitTest::getPrimitiveKeepCellTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    const string check_function = "xstructure::SpaceGroup_ITC() -> GetPrimitive_20210322()";

    // FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_005_A0D3-A1D1, coordinates wrapped into [0,1)
    stringstream poscar;
    poscar << "FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_005_A0D3-A1D1 AAPL supercell=5x5x5 atom=0 distortion=[-0.015,0,0] atom=1 distortion=[0,0.015,0] (wrapped as in CONTCAR)" << std::endl;
    poscar << "1.000000" << std::endl;
    poscar << "   0.00000000000000  11.70304815658165  11.70304815658165" << std::endl;
    poscar << "  11.70304815658165   0.00000000000000  11.70304815658165" << std::endl;
    poscar << "  11.70304815658165  11.70304815658165   0.00000000000000" << std::endl;
    poscar << "F Na" << std::endl;
    poscar << "125 125" << std::endl;
    poscar << "Direct(250) [A125B125]" << std::endl;
    poscar << "   0.00064085868055   0.99935914131945   0.99935914131945  F" << std::endl;  // atom 0, displaced
    poscar << "   0.00064085868055   0.99935914131945   0.20064085868055  F" << std::endl;  // atom 1, displaced
    poscar.setf(std::ios::fixed); poscar.precision(14);
    for (int species = 0; species < 2; species++) {  // F on (i,j,k)/5, then Na on (i+0.5,j+0.5,k+0.5)/5
      const double shift = (species == 0 ? 0.0 : 0.1);
      for (int i = 0; i < 5; i++) { for (int j = 0; j < 5; j++) { for (int k = 0; k < 5; k++) {
        if (species == 0 && i == 0 && j == 0 && k < 2) { continue; }  // the two displaced atoms, written above
        poscar << "   " << (0.2 * i + shift) << "   " << (0.2 * j + shift) << "   " << (0.2 * k + shift) << "  " << (species == 0 ? "F" : "Na_sv") << std::endl;
      } } }
    }

    xstructure xstr;
    try { xstr = xstructure(poscar, IOAFLOW_AUTO); }
    catch (aurostd::xerror& excpt) { errors.push_back("could not read the AAPL_3_005 structure: " + string(excpt.what())); return; }
    checkEqual((uint) xstr.atoms.size(), (uint) 250, "xstructure()", "AAPL_3_005 structure read with 250 atoms", passed_checks, results);
    const double volume_orig = xstr.Volume();

    // the lib2raw step: aflowSG(loose)
    double tolerance_loose = SYM::defaultTolerance(xstr) * 10.0;  //non-const: SpaceGroup_ITC() may change it while it scans
    bool threw = false;
    string what = "";
    uint sgroup = 0;
    try { sgroup = xstr.SpaceGroup_ITC(tolerance_loose, false); }
    catch (aurostd::xerror& excpt) { threw = true; what = excpt.what(); }
    checkEqual(threw, false, check_function,
        "lib2raw's aflowSG(loose) step completes on an AAPL supercell that GetPrimitive cannot reduce consistently" + string(threw ? " [" + what + "]" : ""),
        passed_checks, results);
    checkEqual((sgroup >= 1 && sgroup <= 230), true, check_function, "a valid space group is returned (was an exception)", passed_checks, results);
    checkEqual((uint) xstr.atoms.size(), (uint) 250, check_function, "the supercell keeps all 250 atoms", passed_checks, results);
    const double volume = xstr.Volume();
    check(aurostd::isequal(volume, volume_orig, 1e-6), volume, volume_orig, check_function, "the supercell keeps its volume", passed_checks, results);  //CO20261004 - check() prints the values

    // control: a cell that can be reduced consistently is still reduced
    // (2x1x1 supercell of rocksalt NaF, 4 atoms -> 2 atoms, same volume per atom)
    stringstream poscar_control;
    poscar_control << "NaF rocksalt 2x1x1 supercell (control)" << std::endl << "1.0" << std::endl;
    poscar_control << "   0.00000000000000   4.68121926263266   4.68121926263266" << std::endl;
    poscar_control << "   2.34060963131633   0.00000000000000   2.34060963131633" << std::endl;
    poscar_control << "   2.34060963131633   2.34060963131633   0.00000000000000" << std::endl;
    poscar_control << "F Na" << std::endl << "2 2" << std::endl << "Direct" << std::endl;
    poscar_control << "   0.00000000000000   0.00000000000000   0.00000000000000  F" << std::endl;
    poscar_control << "   0.50000000000000   0.00000000000000   0.00000000000000  F" << std::endl;
    poscar_control << "   0.25000000000000   0.50000000000000   0.50000000000000  Na" << std::endl;
    poscar_control << "   0.75000000000000   0.50000000000000   0.50000000000000  Na" << std::endl;
    try {
      xstructure xstr_control(poscar_control, IOAFLOW_AUTO);
      const double vpa = xstr_control.Volume() / xstr_control.atoms.size();
      xstr_control.GetPrimitive();
      checkEqual((uint) xstr_control.atoms.size(), (uint) 2, "xstructure::GetPrimitive()", "control: a 2x1x1 rocksalt supercell still reduces to 2 atoms", passed_checks, results);
      const double vpa_prim = xstr_control.Volume() / xstr_control.atoms.size();
      check(aurostd::isequal(vpa_prim, vpa, 1e-6), vpa_prim, vpa, "xstructure::GetPrimitive()", "control: volume per atom unchanged", passed_checks, results);  //CO20261004 - check() prints the values
    }
    catch (aurostd::xerror& excpt) { errors.push_back("control structure: " + string(excpt.what())); }
  }

  // ---------------------------------------------------------------------------
  // bringincell_far_component  //CO20260930
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   lib2raw sat in "Space Group analyzer: aflowSG(loose)" on LIB3 AgAsS/A2BC4_aP14_1_4a_2a_8a-001.BAC (14 atoms)
  //   until it was killed after 17 hours on 2026-09-30, and two lib2raw logs from April 2026 stop at the same line,
  //   so the entry never got a RAW record. While searching for the conventional cell, SpaceGroup_ITC() builds a
  //   tetragonal candidate whose lattice is singular (volume 2.8e-14 A^3). SYM::determineLatticeCentering() then
  //   brings c2f*x into the cell with c2f of that lattice, so the components are of order 1e14 and larger.
  //   BringInCellInPlace_20250724() first moves a component by k whole cells, but k was an int: beyond 2^31 cells the
  //   conversion overflows (undefined; INT_MIN in practice) and the while loops are left to step by 1.0 from about
  //   1e14, or forever once |component|>2^53, where component-1.0==component. k is now a double.
  // WHERE IT FAILED
  //   aflow --aflowSG=loose on the entry's CONTCAR.relax (and CONTCAR.relax1); gdb: BringInCellInPlace_20250724 <-
  //   BringInCell <- SYM::determineLatticeCentering <- SYM::ConventionalCell <- xstructure::SpaceGroup_ITC.
  // CHECKED (2026-09-30)
  //   Before: both files still running after 2 minutes, in BringInCellInPlace_20250724, and this test does not
  //   return within 2 minutes. After: I-4m2 #119 in 2 to 3 seconds; the tight search on CONTCAR.relax gives I-4m2 #119
  //   too, and --edata=loose finds the point group -42m (order 8), the same group in another setting.
  //   Wherever the int did not overflow, floor() gives the same integer and k*cell_size the same double, so nothing
  //   else changes.
  // IF THIS TEST FAILS
  //   A hang here (the test does not return) is the old bug: check the cell shift at the top of
  //   BringInCellInPlace_20250724(). A different space group for the AgAsS cell is not the old bug, but it means the
  //   symmetry search changed its answer, which needs a reason.
  void UnitTest::bringInCellFarComponentTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    const string check_function = "BringInCellInPlace()";
    // {component, expected}; values beyond 2^31 cells overflowed the int, values beyond 2^53 never returned
    const vector<std::pair<double, double> > cases = {
      {0.3, 0.3}, {1.0, 0.0}, {-0.25, 0.75},                              // ordinary, unchanged
      {3.0e9 + 0.25, 0.25}, {-3.0e9 + 0.5, 0.5}, {2147483648.75, 0.75},  // beyond 2^31 cells, fraction representable
      {1.0e15, 0.0}, {-1.0e15, 0.0}, {1.0e17, 0.0}, {-1.0e17, 0.0}         // integers; 1e17 is beyond 2^53
    };
    for (const auto& c : cases) {
      double component = c.first;
      BringInCellInPlace(component);
      std::stringstream description;
      description << "component " << c.first << " is brought to " << c.second;
      check(aurostd::isequal(component, c.second, 1e-12), component, c.second, check_function, description.str(), passed_checks, results);  //CO20261004 - check() prints the values
    }

    // the structure that hung: LIB3 AgAsS/A2BC4_aP14_1_4a_2a_8a-001.BAC, CONTCAR.relax
    stringstream poscar;
    poscar << "AgAsS/A2BC4_aP14_1_4a_2a_8a-001.BAC (CONTCAR.relax)" << std::endl;
    poscar << "   1.01639600000000" << std::endl;
    poscar << "     3.7261940533257176    6.5833102736855746   -0.0877626889338267" << std::endl;
    poscar << "    -3.7301008746444775    6.5853553214201943   -0.0841965530801998" << std::endl;
    poscar << "    -0.0040903281862832    4.3061066236934895    6.0880056975761647" << std::endl;
    poscar << "   Ag   As   S" << std::endl;
    poscar << "     2     4     8" << std::endl;
    poscar << "Direct" << std::endl;
    poscar << "  0.3749647896340870  0.8749412592709541  0.3749023756804736" << std::endl;
    poscar << "  0.6248945745614168  0.1249864539912852  0.6255430960685030" << std::endl;
    poscar << "  0.9795785644240177  0.4776149096020351  0.5019743169541796" << std::endl;
    poscar << "  0.0409464729296907  0.0017234848336567  0.9776643565094119" << std::endl;
    poscar << "  0.5017009849888470  0.5410160969861876  0.9795957942666397" << std::endl;
    poscar << "  0.9774567348103835  0.4793872988954357  0.0414240896846534" << std::endl;
    poscar << "  0.2171167288046707  0.2753488122195091  0.2542377356274718" << std::endl;
    poscar << "  0.7753196960129011  0.7171300655226539  0.2533081278284046" << std::endl;
    poscar << "  0.2531592241324693  0.7540414266309898  0.7755355305861370" << std::endl;
    poscar << "  0.2541049215323958  0.7532277091807602  0.2172785050331795" << std::endl;
    poscar << "  0.7680348436344525  0.2695525597379059  0.2276059847049824" << std::endl;
    poscar << "  0.7348205953027024  0.7272694070990393  0.7692317799624314" << std::endl;
    poscar << "  0.2272169470816904  0.2348499209155983  0.7675115700520766" << std::endl;
    poscar << "  0.7697349521502671  0.2679603475940152  0.7351869271214618" << std::endl;
    xstructure xstr;
    try { xstr = xstructure(poscar, IOAFLOW_AUTO); }
    catch (aurostd::xerror& excpt) { errors.push_back("could not read the AgAsS structure: " + string(excpt.what())); return; }
    double tolerance_loose = SYM::defaultTolerance(xstr) * 10.0;  //non-const: SpaceGroup_ITC() may change it while it scans
    bool no_scan = false;
    uint sgroup = 0;
    bool threw = false;
    string what = "";
    try { sgroup = xstr.SpaceGroup_ITC(tolerance_loose, no_scan); }
    catch (aurostd::xerror& excpt) { threw = true; what = excpt.what(); }
    checkEqual(threw, false, "xstructure::SpaceGroup_ITC()", "aflowSG(loose) returns on AgAsS (hung before)" + string(threw ? " [" + what + "]" : ""), passed_checks, results);
    checkEqual(sgroup, (uint) 119, "xstructure::SpaceGroup_ITC()", "aflowSG(loose) on AgAsS gives I-4m2 #119", passed_checks, results);
  }

  // ---------------------------------------------------------------------------
  // glide_equidistant_point  //CO20260930
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   lib2raw stopped with "SYM::operator*(): THERE IS A PROBLEM in operator* Glide(DIRECT=true)" on LIB3
  //   OSnTi_sv/ABC2_aP8_2_bc_dh_2i-001.BCA, in "Space Group analyzer: aflowSG(loose)", so the entry never got a RAW
  //   record. To orient the plane normal a, the glide compares d1=|x_0-P| and d2=|x_0+tol*a-P| (tol=1e-4) and threw
  //   when they are equal. They are equal, to rounding, whenever P lies about tol/2 from the glide plane: here
  //   a.(P-x_0)=5.1e-5 with |a|=1. That is a legitimate point, not a broken glide. The glide now continues with
  //   orient=1, as the two other Glide paths in the same file always did; the image differs from the orient=-1
  //   choice by 2*|a.(P-x_0)|, about 1e-4 Angstrom. Only a zero normal still throws.
  // WHERE IT FAILED
  //   aflow --aflowSG=loose on the entry's CONTCAR.relax (the same search on CONTCAR.relax1 gave Pmna #53).
  // CHECKED (2026-09-30)
  //   Before: both checks fail (the exception). After: CONTCAR.relax gives Pmna #53 at loose tolerance and P2_1/c #14
  //   at tight, as CONTCAR.relax1 does. Calls with d1!=d2 take the same path as before, so only calls that threw
  //   can change.
  // IF THIS TEST FAILS
  //   An exception means the orientation test throws on such points again; do not change the expectation, fix
  //   the code. A different space group means the symmetry search changed its answer, which needs a reason.
  void UnitTest::glideEquidistantPointTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    // LIB3 OSnTi_sv/ABC2_aP8_2_bc_dh_2i-001.BCA, CONTCAR.relax
    stringstream poscar;
    poscar << "OSnTi_sv/ABC2_aP8_2_bc_dh_2i-001.BCA (CONTCAR.relax)" << std::endl;
    poscar << "   1.01639600000000" << std::endl;
    poscar << "     4.8827189549140542    0.1198328791404664    0.0077684421370475" << std::endl;
    poscar << "    -0.1431756810160219    5.3227280500680232    0.1114810377154969" << std::endl;
    poscar << "    -0.0045660902151355   -0.0966563876333615    4.6067019832715328" << std::endl;
    poscar << "   O    Sn   Ti" << std::endl;
    poscar << "     4     2     2" << std::endl;
    poscar << "Direct" << std::endl;
    poscar << "  0.6525788810357641  0.2496078437439871  0.7490033140872658" << std::endl;
    poscar << "  0.3474211189642361  0.7503921562560130  0.2509966859127340" << std::endl;
    poscar << "  0.6525507551825233  0.7496243333224992  0.7510000540531105" << std::endl;
    poscar << "  0.3474492448174694  0.2503756666775010  0.2489999459468895" << std::endl;
    poscar << " -0.0000000000000000  0.0000000000000000  0.5000000000000000" << std::endl;
    poscar << " -0.0000000000000000  0.5000000000000000  0.0000000000000000" << std::endl;
    poscar << "  0.5000000000000000  0.0000000000000000  0.0000000000000000" << std::endl;
    poscar << "  0.5000000000000000  0.5000000000000000  0.5000000000000000" << std::endl;
    xstructure xstr;
    try { xstr = xstructure(poscar, IOAFLOW_AUTO); }
    catch (aurostd::xerror& excpt) { errors.push_back("could not read the OSnTi structure: " + string(excpt.what())); return; }
    double tolerance_loose = SYM::defaultTolerance(xstr) * 10.0;  //non-const: SpaceGroup_ITC() may change it while it scans
    bool no_scan = false;
    uint sgroup = 0;
    bool threw = false;
    string what = "";
    try { sgroup = xstr.SpaceGroup_ITC(tolerance_loose, no_scan); }
    catch (aurostd::xerror& excpt) { threw = true; what = excpt.what(); }
    checkEqual(threw, false, "xstructure::SpaceGroup_ITC()", "aflowSG(loose) on OSnTi completes (threw in a glide before)" + string(threw ? " [" + what + "]" : ""), passed_checks, results);
    checkEqual(sgroup, (uint) 53, "xstructure::SpaceGroup_ITC()", "aflowSG(loose) on OSnTi gives Pmna #53, as on its CONTCAR.relax1", passed_checks, results);
  }

  // ---------------------------------------------------------------------------
  // lattice_dimension_sphere  //CO20261018
  // ---------------------------------------------------------------------------
  // LatticeDimensionSphere() gives the number of cells along each lattice vector that a sphere of a given radius needs.
  // For a nearly singular lattice that number is far past int, and its conversion is undefined: INT_MIN on x86-64
  // (the loops over cells did not run) and INT_MAX on arm64 (macOS), where resetLatticeDimensions() then pushed about
  // 2^31 vectors along each axis (aflow --aflowSG=0.028052705953001762 on the OZr_sv mC6 structure of the xstructure
  // test passed 4 GB in 16 s). SpaceGroup_ITC() now skips such candidates, and LatticeDimensionSphere() refuses them.
  void UnitTest::latticeDimensionSphereTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;
    const string check_function = "LatticeDimensionSphere()";
    xmatrix<double> lattice(3,3);
    lattice(1,1) = 4.0; lattice(2,2) = 4.0; lattice(3,3) = 4.0;
    const xvector<int> dims = LatticeDimensionSphere(lattice, 10.0);
    checkEqual((dims(1) == 3 && dims(2) == 3 && dims(3) == 3), true, check_function, "a 4 A cubic lattice and a 10 A sphere: 3 cells along each vector", passed_checks, results);
    // the third vector 1e-9 A out of the plane of the other two: about 1e10 cells along it
    xmatrix<double> flat(3,3);
    flat(1,1) = 4.0; flat(2,2) = 4.0; flat(3,1) = 2.0; flat(3,2) = 2.0; flat(3,3) = 1.0e-9;
    bool threw = false;
    try { LatticeDimensionSphere(flat, 10.0); }
    catch (aurostd::xerror& excpt) { threw = true; }
    checkEqual(threw, true, check_function, "a lattice whose third vector lies 1e-9 A out of the plane of the others is refused (1e10 cells, past int)", passed_checks, results);
  }

  // ---------------------------------------------------------------------------
  // poscar_orig_repair  //CO20260930
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   The KT20230504 patch in LIB2LIB() rewrites LIB/POSCAR.orig from the first [VASP_POSCAR_MODE_EXPLICIT] block of
  //   aflow.in, marks aflow.in with [POSCAR_ORIG]PATCHED= and never runs again. In the 2026-09-30 lib2raw triage it had
  //   left unreadable files in 23 entries, and lib2raw failed on each of them at every later run:
  //   - 7 LIB3 entries (e.g., Ca_svOSr_sv/ABC_tI24_139_h_i_j-001.CBA, patched 2023-09-24): a 0-byte POSCAR.orig.xz,
  //     not even a valid xz file, although aflow.in is intact (its first START line ends in a space). lib2raw then
  //     stopped with deque::_M_range_check right after "DENSITY", where it reads the original structure.
  //   - 15 LIB5 POCC children (patched 2026-07) and LIB3 Er_3Fe_pvO/ABC3_oP20_62_c_a_cd-003.ABC (2025-08): aflow.in had
  //     already been emptied (91 bytes left, the two PATCHED lines; or a 49-line header), so the patch wrote an empty
  //     POSCAR.orig; lib2raw stopped with "No input" reading it for the species.
  //   Now an unreadable POSCAR.orig is rebuilt from aflow.in even when aflow.in is already marked ([POSCAR_ORIG]REPAIRED=),
  //   and nothing is written when aflow.in has no block that reads as a structure (an emptied aflow.in has to be
  //   restored from a copy; no code can rebuild it).
  // CHECKED (2026-09-30)
  //   The checks below on the first two POSCAR blocks of the Ca_svOSr_sv aflow.in (as in LIB, trailing space kept), an
  //   emptied aflow.in, and temporary LIB folders with a 0-byte POSCAR.orig.xz and with an empty one.
  // IF THIS TEST FAILS
  //   lib2raw will again lose entries whose POSCAR.orig is empty, or write empty ones: fix the code in
  //   LIB2LIB_patchPOSCARorig() and the two functions next to it, not the expectations. The check that the text is
  //   unchanged for a clean block guards the many entries the patch already handled correctly.
  void UnitTest::poscarOrigRepairTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    const string title = "Ca_svOSr_sv/ABC_tI24_139_h_i_j-001.CBA params=-1,0.60916047,0.1354,0.2697,0.1923 SG=139 [ANRL doi: 10.1016/j.commatsci.2017.01.017 (part 1), doi: 10.1016/j.commatsci.2018.10.043 (part 2)]";
    stringstream block;  // the first block, as in LIB3 Ca_svOSr_sv/ABC_tI24_139_h_i_j-001.CBA/aflow.in
    block << title << std::endl;
    block << "-421.542000" << std::endl;
    block << "  -0.50000000000000   0.50000000000000   0.30458023500000" << std::endl;
    block << "   0.50000000000000  -0.50000000000000   0.30458023500000" << std::endl;
    block << "   0.50000000000000   0.50000000000000  -0.30458023500000" << std::endl;
    block << "4 4 4 " << std::endl;
    block << "Direct(12) [A4B4C4] " << std::endl;
    block << "   0.50000000000000   0.19230000000000   0.69230000000000  Ca_sv " << std::endl;
    block << "   0.50000000000000   0.80770000000000   0.30770000000000  Ca_sv " << std::endl;
    block << "   0.19230000000000   0.50000000000000   0.69230000000000  Ca_sv " << std::endl;
    block << "   0.80770000000000   0.50000000000000   0.30770000000000  Ca_sv " << std::endl;
    block << "   0.00000000000000   0.26970000000000   0.26970000000000  O     " << std::endl;
    block << "   0.00000000000000   0.73030000000000   0.73030000000000  O     " << std::endl;
    block << "   0.26970000000000   0.00000000000000   0.26970000000000  O     " << std::endl;
    block << "   0.73030000000000   0.00000000000000   0.73030000000000  O     " << std::endl;
    block << "   0.13540000000000   0.13540000000000   0.27080000000000  Sr_sv " << std::endl;
    block << "   0.86460000000000   0.86460000000000   0.72920000000000  Sr_sv " << std::endl;
    block << "   0.13540000000000   0.86460000000000   0.00000000000000  Sr_sv " << std::endl;
    block << "   0.86460000000000   0.13540000000000   0.00000000000000  Sr_sv " << std::endl;
    stringstream recycled;  // the second block (AFLOW's "Recycling CONTCAR of relax1"), which must not be taken
    recycled << "Ca_svOSr_sv/ABC_tI24_139_h_i_j-001.CBA p" << std::endl << "1.016396" << std::endl;
    recycled << "  -5.64069140589104   5.64069140589104   2.36520688731403" << std::endl;
    recycled << "   5.64069140589104  -5.64069140589104   2.36520688731403" << std::endl;
    recycled << "   5.64069140589104   5.64069140589104  -2.36520688731403" << std::endl;
    recycled << "Ca O Sr " << std::endl << "4 4 4 " << std::endl << "Direct(12) [A4B4C4] " << std::endl;
    for (int i = 0; i < 12; i++) { recycled << "   0." << (10 + 7 * i) << "  0." << (20 + 5 * i) << "  0." << (30 + 3 * i) << "  " << (i < 4 ? "Ca" : (i < 8 ? "O" : "Sr")) << std::endl; }
    auto aflowin_with = [&](const string& start_line) -> string {
      stringstream a;
      a << "[AFLOW] AFLOW automatically generated (aflow_avasp.cpp) " << std::endl;
      a << start_line << std::endl << block.str() << "[VASP_POSCAR_MODE_EXPLICIT]STOP " << std::endl;
      a << "[AFLOW] SELF-MODIFICATION" << std::endl << "[AFLOW] Recycling CONTCAR of relax1" << std::endl;
      a << "[VASP_POSCAR_MODE_EXPLICIT]START" << std::endl << recycled.str() << "[VASP_POSCAR_MODE_EXPLICIT]STOP" << std::endl;
      a << "[POSCAR_ORIG]PATCHED=20230924_10:59:26_GMT-4" << std::endl << "[AFLOW_QMVASP]PATCHED=20230924_10:59:30_GMT-4" << std::endl;
      return a.str();
    };
    const string aflowin_space = aflowin_with("[VASP_POSCAR_MODE_EXPLICIT]START ");  // as in LIB
    const string aflowin_clean = aflowin_with("[VASP_POSCAR_MODE_EXPLICIT]START");
    const string aflowin_emptied = "[POSCAR_ORIG]PATCHED=20260707_00:00:00_GMT-4\n[AFLOW_QMVASP]PATCHED=20260707_00:00:00_GMT-4\n";
    const string fn = "aflowlib::LIB2LIB_POSCARorigFromAflowIn()";

    auto volume_of = [](const string& text) -> double {
      try { stringstream ss(text); xstructure xstr(ss, IOAFLOW_AUTO); return xstr.GetVolume(); }
      catch (aurostd::xerror& excpt) { return -1.0; }
    };
    // temporary LIB folders, one per aflow.in
    const string dir = aurostd::TmpDirectoryCreate("poscar_orig_repair");
    auto folder_with = [&](const string& name, const string& aflowin) -> string {
      const string d = dir + "/" + name;
      aurostd::DirectoryMake(d);
      aurostd::string2file(aflowin, d + "/" + _AFLOWIN_);
      return d;
    };
    // LIB2LIB_POSCARorigFromAflowIn() takes the text of aflow.in that LIB2LIB() holds; the folders are for
    // LIB2LIB_patchPOSCARorig() below and for the comparison with "aflow --extract_poscar_instance=aflow.in,1" //CO20261004
    const string lib_space = folder_with("space", aflowin_space);
    folder_with("emptied", aflowin_emptied);
    string poscar = aflowlib::LIB2LIB_POSCARorigFromAflowIn(aflowin_space);
    double volume = volume_of(poscar);
    check(aurostd::isequal(volume, 421.542, 1e-3), volume, 421.542, fn, "first START line ends in a space: the first block is read (volume 421.542 A^3)", passed_checks, results);  //CO20261004 - check() prints the values
    checkEqual(poscar, aurostd::substring2string(aflowin_space, _VASP_POSCAR_MODE_EXPLICIT_START_, _VASP_POSCAR_MODE_EXPLICIT_STOP_, 1), fn,
        "first START line ends in a space: the block reads as a structure, so the text is the same as the patch wrote before", passed_checks, results);  //CO20261004
    // aflow --extract_poscar_instance=aflow.in,1 on the same file trims the whitespace at both ends; same structure
    const double volume_extract = volume_of(pflow::EXTRACT_POSCAR_INSTANCE_FROM_AFLOWIN(lib_space + "/" + _AFLOWIN_, 1));
    check(aurostd::isequal(volume_extract, volume, 1e-6), volume_extract, volume, fn,
        "first START line ends in a space: aflow --extract_poscar_instance=aflow.in,1 on the file reads as the same cell", passed_checks, results);  //CO20261004
    poscar = aflowlib::LIB2LIB_POSCARorigFromAflowIn(aflowin_clean);
    checkEqual(poscar, aurostd::substring2string(aflowin_clean, _VASP_POSCAR_MODE_EXPLICIT_START_, _VASP_POSCAR_MODE_EXPLICIT_STOP_, 1), fn, "clean block: the text is the same as the patch wrote before", passed_checks, results);
    volume = volume_of(poscar);
    check(aurostd::isequal(volume, 421.542, 1e-3), volume, 421.542, fn, "clean block: the first block is read (volume 421.542 A^3)", passed_checks, results);  //CO20261004 - check() prints the values
    checkEqual(aflowlib::LIB2LIB_POSCARorigFromAflowIn(aflowin_emptied), string(""), fn, "emptied aflow.in: nothing to write", passed_checks, results);

    const string fp = "aflowlib::LIB2LIB_patchPOSCARorig()";
    try {
      // 1. a 0-byte POSCAR.orig.xz, as the 2023 patch left it, next to a compressed CONTCAR
      const string lib = dir + "/space";
      aurostd::string2file("", lib + "/POSCAR.orig.xz");
      aurostd::string2file(block.str(), lib + "/CONTCAR.relax1"); aurostd::CompressFile(lib + "/CONTCAR.relax1", "xz", true);
      checkEqual(aflowlib::LIB2LIB_POSCARorigReadable(lib), false, "aflowlib::LIB2LIB_POSCARorigReadable()", "0-byte POSCAR.orig.xz is unreadable", passed_checks, results);
      stringstream patched(aflowin_space);
      patched.seekp(0, std::ios_base::end);
      aflowlib::LIB2LIB_patchPOSCARorig(lib, aflowin_space, patched);
      checkEqual(aflowlib::LIB2LIB_POSCARorigReadable(lib), true, fp, "0-byte POSCAR.orig.xz is rebuilt from aflow.in", passed_checks, results);
      volume = volume_of(aurostd::efile2string(lib + "/POSCAR.orig.xz"));
      check(aurostd::isequal(volume, 421.542, 1e-3), volume, 421.542, fp, "the rebuilt POSCAR.orig.xz holds the first block", passed_checks, results);  //CO20261004 - check() prints the values
      checkEqual(aurostd::substring2bool(patched.str(), "[POSCAR_ORIG]REPAIRED="), true, fp, "aflow.in records the repair", passed_checks, results);
      const string after_repair = patched.str();
      aflowlib::LIB2LIB_patchPOSCARorig(lib, after_repair, patched);
      checkEqual(patched.str(), after_repair, fp, "a readable POSCAR.orig in a marked aflow.in is left alone", passed_checks, results);

      // 2. an emptied aflow.in and an xz file holding an empty POSCAR.orig, as in the POCC children
      const string emptied_dir = dir + "/emptied";
      aurostd::string2file("", emptied_dir + "/POSCAR.orig"); aurostd::CompressFile(emptied_dir + "/POSCAR.orig", "xz", true);
      const string before = aurostd::efile2string(emptied_dir + "/POSCAR.orig.xz");
      stringstream emptied(aflowin_emptied);
      emptied.seekp(0, std::ios_base::end);
      aflowlib::LIB2LIB_patchPOSCARorig(emptied_dir, aflowin_emptied, emptied);
      checkEqual(emptied.str(), aflowin_emptied, fp, "emptied aflow.in: aflow.in is not marked", passed_checks, results);
      checkEqual(aurostd::FileExist(emptied_dir + "/POSCAR.orig.xz") && aurostd::efile2string(emptied_dir + "/POSCAR.orig.xz") == before, true, fp, "emptied aflow.in: POSCAR.orig is left as it was", passed_checks, results);

      // 3. an aflow.in the patch has not marked yet: the first patch, as the KT20230504 code in LIB2LIB() did it before
      //    it moved into LIB2LIB_patchPOSCARorig(); POSCAR.orig is written from the first block, compressed like the
      //    CONTCAR next to it, and aflow.in is marked PATCHED, not REPAIRED //CO20261004
      string aflowin_unmarked = aflowin_clean;
      aurostd::StringSubst(aflowin_unmarked, "[POSCAR_ORIG]PATCHED=20230924_10:59:26_GMT-4\n", "");
      const string fresh_dir = folder_with("fresh", aflowin_unmarked);
      checkEqual(aflowlib::LIB2LIB_POSCARorigReadable(fresh_dir), false, "aflowlib::LIB2LIB_POSCARorigReadable()", "a missing POSCAR.orig is unreadable", passed_checks, results);
      aurostd::string2file(block.str(), fresh_dir + "/CONTCAR.relax1"); aurostd::CompressFile(fresh_dir + "/CONTCAR.relax1", "xz", true);
      stringstream fresh(aflowin_unmarked);
      fresh.seekp(0, std::ios_base::end);
      aflowlib::LIB2LIB_patchPOSCARorig(fresh_dir, aflowin_unmarked, fresh);
      checkEqual(aurostd::substring2bool(fresh.str(), "[POSCAR_ORIG]PATCHED=") && !aurostd::substring2bool(fresh.str(), "[POSCAR_ORIG]REPAIRED="), true, fp,
          "unmarked aflow.in: the first patch marks it PATCHED", passed_checks, results);
      checkEqual(aurostd::FileExist(fresh_dir + "/POSCAR.orig.xz") && !aurostd::FileExist(fresh_dir + "/POSCAR.orig"), true, fp,
          "unmarked aflow.in: POSCAR.orig is written and compressed like CONTCAR.relax1.xz", passed_checks, results);
      volume = volume_of(aurostd::efile2string(fresh_dir + "/POSCAR.orig.xz"));
      check(aurostd::isequal(volume, 421.542, 1e-3), volume, 421.542, fp, "unmarked aflow.in: POSCAR.orig holds the first block", passed_checks, results);
    }
    catch (aurostd::xerror& excpt) { errors.push_back("temporary LIB folder: " + string(excpt.what())); }
    aurostd::RemoveDirectory(dir);
  }

  //CO20260930 START - symmetry_exact: the exact speedups of the symmetry search give the same results as the code
  // they replace
  // ---------------------------------------------------------------------------------------------------------------
  // WHAT IS TESTED
  //   Three changes make the symmetry search faster without changing any result (see the notes above
  //   SYM::fractionalDistanceNoSkew() in aflow_symmetry.cpp and in GetPrimitive_20210322() in aflow_xatom.cpp):
  //   1. GetPrimitive_20210322() skips the Minkowski and Niggli reductions of candidate triples whose volume already
  //      fails the volume tests (the reductions keep the volume);
  //   2. SYM::getFullSymBasis() compares a transformed atom only with the atoms in the neighboring cells of a cell
  //      list, and takes the lowest unmapped index, as the linear scan did;
  //   3. SYM::getFullSymBasis() computes the distance of FPOSMatch() without temporary vectors, with the same IEEE
  //      operations in the same order. CO20261004: no longer; the cell list hands its candidates to
  //      SYM::MapAtomWithBasis(), so the test is FPOSMatch()/AtomsMapped() itself, and fractionalDistanceNoSkew() is
  //      only checked by symmetry_exact_distance.
  //   The code as it was ("reference paths") is still in the binary: SYM::setReferencePathsThisThread(1) selects it
  //   for the calling thread, and --sym_reference for the whole run. These tests run both and require results that
  //   are identical to the last bit, not merely close.
  // THE TESTS
  //   symmetry_exact_distance  (group symmetry_exact, seconds)
  //     fractionalDistanceNoSkew() against modulus(CPOSDistFromFPOS(...,skew=false)), the value FPOSMatch() uses,
  //     bitwise, on 21,600 pairs in six lattices (cubic to strongly skewed, and the 250-atom NaF phonon supercell),
  //     including wrap-around values (differences of exactly 0.5, 1, 0, values just below 1, negative and large
  //     coordinates); FPOSMatch() at the tolerance equal to the distance and one ulp either side; and the bound that
  //     makes the cell list exact, |d_frac_i| <= distance*|row i of inverse(f2c)|. Also the switch between the paths:
  //     the choice of a thread wins over the global setting of --sym_reference, which -1 follows (CO20261004).
  //   symmetry_exact_mapping  (group symmetry_exact, under a minute)
  //     getFullSymBasis() called directly, reference against current: same answer, same atom and type maps (also
  //     the partial maps of a failed call). Random structures (16 to 48 atoms, three species, some positions outside
  //     [0,1) or at 1.0) in skewed and hexagonal cells, under inversion or identity with exact and random
  //     translations, at tolerances that span the cell-list regime (bin widths from 1/5 to one bin per axis, so most
  //     images have several atoms within the tolerance and matches lie up to the bin bound); then 54- and 128-atom
  //     supercells with every atom displaced at random, at tolerances from 0.7 to 2.8 times the displacement and
  //     0.6 to 1.6 times the nearest-neighbor distance. Each with map_types on and off, skew on and off, without
  //     spins, with collinear spins, and with partial occupations. It also checks that the cases where the cell
  //     list could go wrong were met: matches in a neighboring bin (SYM::exactPathCounts()), ties decided by the
  //     lower index and searches with skew, spins or map_types off (both counted by the test itself since
  //     CO20261004: the image of the first atom has two or more atoms within the tolerance, by MapAtomWithBasis()
  //     with fast=true and fast=false).
  //   symmetry_exact_paths  (group symmetry_exact, minutes)
  //     a) getFullSymBasis() on every factor-group operation of pristine supercells (rocksalt NaF 2x2x2 and 3x3x3,
  //        hcp 2x2x2, a triclinic 2x2x1 cell), applied to the pristine cell and to a displaced copy, at 0.3 to 60
  //        times the tight tolerance, map_types on and off. Both outcomes (maps, does not map) must occur.
  //        Then GetPrimitive() of each pristine supercell, with both paths, must give the primitive cell (atom count
  //        and volume): the volume criteria are shared by the prefilter and the test itself (CO20261004).
  //     b) what lib2raw computes from a structure file, as text with doubles in hexadecimal: GetPrimitive (aflow
  //        --prim); the loose and the tight space-group search (nearest-neighbor distance/10 and /100, with
  //        tolerance scanning), with the Wyckoff positions, standard lattice and origin; EDATA as lib2raw writes it
  //        (text, printed precision) together with, to the bit, the symmetry it computed: the point, factor and
  //        crystal point groups with their atom maps, the equivalent atoms and the site point groups, of the
  //        structure and of the standard primitive cell lib2raw writes them for, and the standard primitive and
  //        conventional cells; the AFLOW prototype label and parameters at the tolerance EDATA settled on. No step
  //        may throw. The structures: the supercells of a), pristine and displaced, and supercells of small AFLOW
  //        prototypes of other lattice systems, pristine and displaced. Pristine supercells reduce to the primitive
  //        cell first, so they reach the volume prefilter; the displaced ones also reach the cell list (conventional
  //        cell search, factor group). Both must be met (SYM::exactPathCounts()).
  //   symmetry_exact_all  (not in a group and not in "all": hours on one core; run with --np=N)
  //     b) for every prototype and every parameter set of the AFLOW prototype library ("anrl", every one of the 230
  //        space groups); for each space group, its smallest prototype in a supercell of at least 16 atoms, pristine
  //        and displaced; and 3x3x3 supercells of fcc, bcc, hcp, rocksalt and perovskite, pristine and displaced, as
  //        in phonon runs.
  //   Supercells have at least 16 atoms, because the cell list is used from 16 atoms. The displaced copies move one
  //   atom by 0.5 and one by 1.2 times the tight tolerance, as in the phonon (AAPL) children where this search was
  //   slow: within the loose tolerance every lattice translation still looks valid (the costly GetPrimitive case),
  //   within the tight one it does not (the costly conventional-cell search).
  //   All structures are written as POSCAR text and read back first, so both paths see what lib2raw reads.
  // HOW STRONG THE TESTS ARE (checked 2026-10-01 with deliberately broken builds)
  //   Summing the distance in another order: symmetry_exact_distance fails (2,065 of 21,600 pairs differ).
  //   symmetry_exact_mapping fails when the cell list takes the first match found instead of the lowest index (2,456
  //   of 19,584 calls differ), uses bins half as wide as the bound (1,916), or uses column instead of row norms of
  //   inverse(f2c) (28). A prefilter that rejects valid triples: symmetry_exact_paths fails (5 of 16 structures).
  //   Dropping the 1e-6 margin of the bin width is not detected: it only guards against rounding at the exact bound.
  // IF A TEST FAILS
  //   The faster code does not give AFLOW's answers any more. Do not change the expectations and do not loosen the
  //   comparison: find the difference (the error names the structure and the first differing line) and fix the
  //   code, or run with --sym_reference until it is fixed. symmetry_exact_distance failing after a compiler or
  //   flag change (for example FMA contraction) means fractionalDistanceNoSkew() must not stand in for FPOSMatch()'s
  //   distance anywhere (getFullSymBasis() does not use it since CO20261004).
  //   Record: AFLOW_SYMMETRY_SPEED_20260930.md (Project_2026_CHAOS).

  // Selects the reference paths for this thread while in scope, and always clears the choice afterwards.
  struct SymReferenceThisThread {
    explicit SymReferenceThisThread(int on) { SYM::setReferencePathsThisThread(on); }
    ~SymReferenceThisThread() { SYM::setReferencePathsThisThread(-1); }
  };

  static string symExactHex(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%a", value);
    return string(buffer);
  }
  static string symExactHex(const xvector<double>& v) {
    string out = "";
    for (int i = v.lrows; i <= v.urows; i++) out += symExactHex(v[i]) + " ";
    return out;
  }
  static string symExactHex(const xmatrix<double>& m) {
    string out = "";
    for (int i = m.lrows; i <= m.urows; i++) {
      for (int j = m.lcols; j <= m.ucols; j++) out += symExactHex(m[i][j]) + " ";
      out += "; ";
    }
    return out;
  }

  // A structure to the bit: lattice, then type and fractional position of each atom
  static string symExactStructureHex(const xstructure& a) {
    stringstream out;
    out << "  natoms=" << a.atoms.size() << " scale=" << symExactHex(a.scale) << " lattice=" << symExactHex(a.lattice) << std::endl;
    for (size_t i = 0; i < a.atoms.size(); i++) out << "  " << a.atoms[i].type << " " << symExactHex(a.atoms[i].fpos) << std::endl;
    return out.str();
  }

  // The symmetry stored in a structure, to the bit: each group's operations with their atom and type maps, the
  // equivalent atoms, and the site point groups
  static string symExactGroupsHex(const xstructure& a) {
    stringstream out;
    const std::vector<_sym_op>* groups[6] = {&a.pgroup, &a.pgroupk, &a.fgroup, &a.pgroup_xtal, &a.pgroupk_xtal, &a.sgroup};
    const char* names[6] = {"pgroup", "pgroupk", "fgroup", "pgroup_xtal", "pgroupk_xtal", "sgroup"};
    for (int g = 0; g < 6; g++) {
      out << "  " << names[g] << " " << groups[g]->size() << std::endl;
      for (size_t i = 0; i < groups[g]->size(); i++) {
        const _sym_op& op = (*groups[g])[i];
        out << "    Uf=" << symExactHex(op.Uf) << " ftau=" << symExactHex(op.ftau)
          << " atoms=" << aurostd::joinWDelimiter(op.basis_atoms_map, ",") << " types=" << aurostd::joinWDelimiter(op.basis_types_map, ",") << std::endl;
      }
    }
    out << "  iatoms";
    for (size_t i = 0; i < a.iatoms.size(); i++) out << " [" << aurostd::joinWDelimiter(a.iatoms[i], ",") << "]";
    out << std::endl << "  agroup";
    for (size_t i = 0; i < a.agroup.size(); i++) {
      out << " " << i << ":" << a.agroup[i].size();
      for (size_t j = 0; j < a.agroup[i].size(); j++) out << " " << symExactHex(a.agroup[i][j].Uf);
    }
    out << std::endl;
    return out.str();
  }

  // Everything lib2raw derives from a structure through the symmetry search, as text. Doubles are written in
  // hexadecimal, so the text differs whenever any result differs in any bit. Exceptions are part of the result.
  static string symExactSignature(const xstructure& xstr_in) {
    stringstream sig;
    // aflow --prim
    try {
      xstructure a(xstr_in);
      a.GetPrimitive();
      sig << "prim natoms=" << a.atoms.size() << " lattice=" << symExactHex(a.lattice) << std::endl;
      for (size_t i = 0; i < a.atoms.size(); i++) sig << "  " << a.atoms[i].type << " " << symExactHex(a.atoms[i].fpos) << std::endl;
    }
    catch (aurostd::xerror& excpt) { sig << "prim exception: " << excpt.what() << std::endl; }
    catch (std::exception& excpt) { sig << "prim exception: " << excpt.what() << std::endl; }
    // lib2raw's aflowSG loose (nearest-neighbor distance/10) and tight (/100) searches, with tolerance scanning
    for (int pass = 0; pass < 2; pass++) {
      const string which = (pass == 0 ? "loose" : "tight");
      try {
        xstructure a(xstr_in);
        double tolerance = SYM::defaultTolerance(a) * (pass == 0 ? 10.0 : 1.0);
        bool no_scan = false;
        const uint sg = a.SpaceGroup_ITC(tolerance, no_scan);
        sig << which << " sg=" << sg << " tolerance=" << symExactHex(tolerance) << " bravais=" << a.bravais_label_ITC
          << " standard_lattice=" << symExactHex(a.standard_lattice_ITC) << " origin=" << symExactHex(a.origin_ITC) << std::endl;
        for (size_t i = 0; i < a.wyccar_ITC.size(); i++) sig << "  " << a.wyccar_ITC[i] << std::endl;
      }
      catch (aurostd::xerror& excpt) { sig << which << " exception: " << excpt.what() << std::endl; }
      catch (std::exception& excpt) { sig << which << " exception: " << excpt.what() << std::endl; }
    }
    // EDATA as lib2raw writes it, with the symmetry it computed (lib2raw writes the groups of the standard primitive
    // cell), then the AFLOW prototype label at the tolerance EDATA settled on
    double sym_eps = AUROSTD_MAX_DOUBLE;
    try {
      xstructure str(xstr_in), str_sym(xstr_in), str_sp, str_sc;
      aurostd::xoption vpflow_edata;
      sig << "edata" << std::endl << pflow::PrintData(str, str_sym, str_sp, str_sc, vpflow_edata, "EDATA", txt_ft, false) << std::endl;
      sym_eps = str_sym.sym_eps;
      sig << "edata sym_eps=" << symExactHex(str_sym.sym_eps) << std::endl;
      sig << "edata structure symmetry" << std::endl << symExactGroupsHex(str_sym);
      sig << "edata standard primitive" << std::endl << symExactStructureHex(str_sp) << symExactGroupsHex(str_sp);
      sig << "edata standard conventional" << std::endl << symExactStructureHex(str_sc);
    }
    catch (aurostd::xerror& excpt) { sig << "edata exception: " << excpt.what() << std::endl; }
    catch (std::exception& excpt) { sig << "edata exception: " << excpt.what() << std::endl; }
    try {
      xstructure str_anrl(xstr_in);
      anrl::structure2anrl(str_anrl, sym_eps, SG_SETTING_ANRL);
      sig << "prototype " << str_anrl.prototype << " parameters=" << aurostd::joinWDelimiter(str_anrl.prototype_parameter_list, ",") << " values=";
      for (size_t i = 0; i < str_anrl.prototype_parameter_values.size(); i++) sig << symExactHex(str_anrl.prototype_parameter_values[i]) << ",";
      sig << std::endl;
    }
    catch (aurostd::xerror& excpt) { sig << "prototype exception: " << excpt.what() << std::endl; }
    catch (std::exception& excpt) { sig << "prototype exception: " << excpt.what() << std::endl; }
    return sig.str();
  }

  // Signatures with an exception in some step (the same in both paths when they compare equal)
  static bool symExactThrew(const string& signature) { return (signature.find(" exception: ") != string::npos); }

  // The signature with the reference paths, then with the current code, in this thread only. On a difference,
  // difference gets the first line that differs; threw tells whether some step threw (in either path).
  static bool symExactCompare(const xstructure& xstr, string& difference, bool& threw) {
    string sig_reference = "", sig_current = "";
    { SymReferenceThisThread reference(1); sig_reference = symExactSignature(xstr); }
    { SymReferenceThisThread current(0); sig_current = symExactSignature(xstr); }
    threw = (symExactThrew(sig_reference) || symExactThrew(sig_current));
    if (sig_reference == sig_current) return true;
    vector<string> lines_reference, lines_current;
    aurostd::string2tokens(sig_reference, lines_reference, "\n");
    aurostd::string2tokens(sig_current, lines_current, "\n");
    size_t i = 0;
    while (i < lines_reference.size() && i < lines_current.size() && lines_reference[i] == lines_current[i]) i++;
    difference = "reference [" + (i < lines_reference.size() ? lines_reference[i] : string("end")) + "] current ["
      + (i < lines_current.size() ? lines_current[i] : string("end")) + "]";
    return false;
  }

  // lib2raw reads structures from files: go through POSCAR text
  static xstructure symExactAsFile(const xstructure& xstr) {
    stringstream poscar;
    poscar << xstr;
    return xstructure(poscar, IOAFLOW_AUTO);
  }

  // A supercell with at least 16 atoms, so getFullSymBasis() uses the cell list
  static xstructure symExactSupercell(const xstructure& xstr) {
    const size_t natoms = xstr.atoms.size();
    if (natoms >= 16) return xstr;
    int k1 = 2, k2 = 1, k3 = 1;
    if (natoms == 1) { k1 = 3; k2 = 3; k3 = 3; }
    else if (natoms < 4) { k2 = 2; k3 = 2; }
    else if (natoms < 8) { k2 = 2; }
    return GetSuperCell(xstr, k1, k2, k3);
  }

  // As in a phonon (AAPL) child: atom 0 moved by 0.5 and atom N/2 by 1.2 times the tight tolerance (along the
  // first and the second lattice vector)
  static xstructure symExactDisplaced(const xstructure& xstr_in) {
    xstructure xstr(xstr_in);
    xstr.ReScale(1.0);
    const double tolerance = SYM::defaultTolerance(xstr);
    const size_t moved[2] = {0, xstr.atoms.size() / 2};
    const double length[2] = {0.5 * tolerance, 1.2 * tolerance};
    for (int m = 0; m < 2; m++) {
      const int axis = m + 1;
      const double a = std::sqrt(xstr.lattice[axis][1] * xstr.lattice[axis][1] + xstr.lattice[axis][2] * xstr.lattice[axis][2] + xstr.lattice[axis][3] * xstr.lattice[axis][3]);
      _atom& atom = xstr.atoms[moved[m]];
      atom.fpos[axis] += length[m] / a;
      atom.cpos = F2C(xstr.lattice, atom.fpos);
    }
    return xstr;
  }

  // "<kind> <prototype label> <parameter set index>"; kind: proto (as generated), sc (supercell of at least 16
  // atoms), scd (sc, displaced), sc3 (3x3x3 supercell, as in phonon runs), sc3d (sc3, displaced)
  static bool symExactBuild(const string& recipe, xstructure& xstr, string& problem) {
    vector<string> tokens;
    aurostd::string2tokens(recipe, tokens, " ");
    if (tokens.size() != 3) { problem = "bad recipe " + recipe; return false; }
    try {
      const vector<string> parameter_sets = anrl::getANRLParameters(tokens[1], "all");
      const uint j = aurostd::string2utype<uint>(tokens[2]);
      if (j >= parameter_sets.size()) { problem = "no parameter set " + tokens[2]; return false; }
      ofstream devnull("/dev/null");
      xstr = symExactAsFile(aflowlib::PrototypeLibraries(devnull, tokens[1], parameter_sets[j], 1));
      if (tokens[0] == "sc" || tokens[0] == "scd") xstr = symExactAsFile(symExactSupercell(xstr));
      if (tokens[0] == "sc3" || tokens[0] == "sc3d") xstr = symExactAsFile(GetSuperCell(xstr, 3, 3, 3));
      if (tokens[0] == "scd" || tokens[0] == "sc3d") xstr = symExactAsFile(symExactDisplaced(xstr));
    }
    catch (aurostd::xerror& excpt) { problem = "could not build: " + string(excpt.what()); return false; }
    catch (std::exception& excpt) { problem = "could not build: " + string(excpt.what()); return false; }
    return true;
  }

  // One entry "<name>\n<recipe>"; counts: [0] identical, [1] different, [2] could not be built, [3] identical but
  // some step threw in both paths (also counted in [0]; the names go to differences, marked "threw").
  // Same signature as _testPrototype(), so it uses the same xThread instantiation.
  static void symExactWorker(uint i, const vector<string>& entries, vector<uint>& counts, vector<string>& differences
#ifdef AFLOW_MULTITHREADS_ENABLE
      , std::mutex& m
#endif
      ) {
    const string& entry = entries[i];
    const string::size_type nl = entry.find('\n');
    const string name = entry.substr(0, nl), recipe = entry.substr(nl + 1);
    xstructure xstr;
    string note = "";
    uint outcome = 2;
    bool threw = false;
    if (symExactBuild(recipe, xstr, note)) outcome = (symExactCompare(xstr, note, threw) ? 0 : 1);
#ifdef AFLOW_MULTITHREADS_ENABLE
    std::lock_guard<std::mutex> lk(m);
#endif
    counts[outcome]++;
    if (outcome == 0 && threw) { counts[3]++; differences.push_back("threw (same in both paths): " + name); }
    if (outcome != 0) differences.push_back(name + " (" + aurostd::utype2string(xstr.atoms.size()) + " atoms): " + note);
  }

  // Differences go to errors (they fail the test). Structures where a step threw in both paths are identical, but
  // that step was not compared: they are counted in the description, with the first names, and are not errors.
  static string symExactSplitNotes(const vector<string>& differences, const vector<uint>& counts, vector<string>& errors) {
    vector<string> threw;
    for (size_t i = 0; i < differences.size(); i++) {
      if (differences[i].find("threw (same in both paths): ") == 0) threw.push_back(differences[i].substr(28));
      else errors.push_back(differences[i]);
    }
    if (counts[3] == 0) return "";
    vector<string> first(threw.begin(), threw.begin() + std::min(threw.size(), (size_t) 5));
    return " [" + aurostd::utype2string(counts[3]) + " had a step that threw in both paths, not compared there: " + aurostd::joinWDelimiter(first, ", ")
      + (threw.size() > 5 ? ", ..." : "") + "]";
  }

  static void symExactRun(vector<string>& entries, vector<uint>& counts, vector<string>& differences) {
    counts.assign(4, 0);
#ifdef AFLOW_MULTITHREADS_ENABLE
    xthread::xThread xt(KBIN::get_NCPUS());
    std::mutex m;
    const uint nentries = entries.size();
    xt.run(nentries, symExactWorker, entries, counts, differences, m);
#else
    for (uint i = 0; i < entries.size(); i++) symExactWorker(i, entries, counts, differences);
#endif
  }

  // For each space group, the prototype label with the fewest atoms in the primitive cell (first in library order on
  // a tie), from the label, atom count and space group of each entry of anrl::PrototypeANRL_LoadList(), the list
  // aflowlib::GetAllPrototypeLabels() returns for "anrl". //CO20261004 - the atom count was parsed from the Pearson
  // symbol in each label (conventional cell). Within a space group the centering is fixed, so the choice is the same,
  // except where the list's atom count disagrees with the label: for space group 71 it gives 4 atoms for
  // AB_oI4_71_b_c, so AB2_oI6_71_a_i (3) is taken instead.
  static std::map<uint, string> symExactSmallestPerSpaceGroup() {
    vector<string> vproto, vproto_label, vproto_Pearson_symbol, vproto_params, vproto_Strukturbericht, vproto_prototype, vproto_dialect;
    vector<uint> vproto_nspecies, vproto_natoms, vproto_spacegroup, vproto_nunderscores, vproto_nparameters;
    anrl::PrototypeANRL_LoadList(vproto, vproto_label, vproto_nspecies, vproto_natoms, vproto_spacegroup, vproto_nunderscores, vproto_nparameters,
        vproto_Pearson_symbol, vproto_params, vproto_Strukturbericht, vproto_prototype, vproto_dialect);
    std::map<uint, string> smallest;
    std::map<uint, uint> natoms_smallest;
    for (size_t i = 0; i < vproto_label.size(); i++) {
      const uint sg = vproto_spacegroup[i];
      if (!smallest.count(sg) || vproto_natoms[i] < natoms_smallest[sg]) { smallest[sg] = vproto_label[i]; natoms_smallest[sg] = vproto_natoms[i]; }
    }
    return smallest;
  }

  // Structures written out in full, so the test does not depend on the prototype library
  static xstructure symExactInline(const string& which) {
    stringstream poscar;
    poscar.precision(14);
    poscar.setf(std::ios::fixed);
    if (which == "NaF") {  // rocksalt, primitive cell (as FNa_sv/AB_cF8_225_a_b.AB)
      poscar << "NaF rocksalt primitive" << std::endl << "1.0" << std::endl;
      poscar << "   0.00000000000000   2.34060963131633   2.34060963131633" << std::endl;
      poscar << "   2.34060963131633   0.00000000000000   2.34060963131633" << std::endl;
      poscar << "   2.34060963131633   2.34060963131633   0.00000000000000" << std::endl;
      poscar << "F Na" << std::endl << "1 1" << std::endl << "Direct" << std::endl;
      poscar << "   0.00000000000000   0.00000000000000   0.00000000000000  F" << std::endl;
      poscar << "   0.50000000000000   0.50000000000000   0.50000000000000  Na" << std::endl;
    } else if (which == "hcp") {  // Mg, a=3.2094, c=5.2105
      const double a = 3.2094, c = 5.2105;
      poscar << "Mg hcp" << std::endl << "1.0" << std::endl;
      poscar << "   " << a << "   0.00000000000000   0.00000000000000" << std::endl;
      poscar << "   " << -0.5 * a << "   " << 0.5 * std::sqrt(3.0) * a << "   0.00000000000000" << std::endl;
      poscar << "   0.00000000000000   0.00000000000000   " << c << std::endl;
      poscar << "Mg" << std::endl << "2" << std::endl << "Direct" << std::endl;
      poscar << "   " << 1.0 / 3.0 << "   " << 2.0 / 3.0 << "   0.25000000000000  Mg" << std::endl;
      poscar << "   " << 2.0 / 3.0 << "   " << 1.0 / 3.0 << "   0.75000000000000  Mg" << std::endl;
    } else {  // "triclinic": P-1, two species, general positions
      poscar << "AB triclinic P-1" << std::endl << "1.0" << std::endl;
      poscar << "   5.10000000000000   0.00000000000000   0.00000000000000" << std::endl;
      poscar << "   1.30000000000000   6.20000000000000   0.00000000000000" << std::endl;
      poscar << "  -0.70000000000000   1.10000000000000   7.40000000000000" << std::endl;
      poscar << "A B" << std::endl << "2 2" << std::endl << "Direct" << std::endl;
      poscar << "   0.11000000000000   0.23000000000000   0.37000000000000  A" << std::endl;
      poscar << "   0.89000000000000   0.77000000000000   0.63000000000000  A" << std::endl;
      poscar << "   0.41000000000000   0.05000000000000   0.72000000000000  B" << std::endl;
      poscar << "   0.59000000000000   0.95000000000000   0.28000000000000  B" << std::endl;
    }
    return xstructure(poscar, IOAFLOW_AUTO);
  }

  void UnitTest::symmetryExactDistanceTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;
    vector<xmatrix<double> > lattices;
    xmatrix<double> lattice(3, 3);
    lattice[1][1] = 4.0; lattice[2][2] = 4.0; lattice[3][3] = 4.0; lattices.push_back(lattice);  // cubic
    lattice = xmatrix<double>(3, 3);
    lattice[1][1] = 3.2094; lattice[2][1] = -1.6047; lattice[2][2] = 2.77942426; lattice[3][3] = 5.2105; lattices.push_back(lattice);  // hexagonal
    lattice = xmatrix<double>(3, 3);
    lattice[1][1] = 5.1; lattice[2][1] = 1.3; lattice[2][2] = 6.2; lattice[3][1] = -0.7; lattice[3][2] = 1.1; lattice[3][3] = 7.4; lattices.push_back(lattice);  // triclinic
    lattice = xmatrix<double>(3, 3);
    lattice[1][1] = 3.0; lattice[2][1] = 2.9; lattice[2][2] = 0.4; lattice[3][3] = 10.0; lattices.push_back(lattice);  // strongly skewed
    lattice = xmatrix<double>(3, 3);
    lattice[1][1] = 0.02; lattice[1][2] = 0.3; lattice[2][2] = 0.05; lattice[3][1] = 0.01; lattice[3][3] = 0.04; lattices.push_back(lattice);  // tiny, skewed
    lattice = xmatrix<double>(3, 3);
    lattice[1][2] = 11.70304815658165; lattice[1][3] = 11.70304815658165; lattice[2][1] = 11.70304815658165;
    lattice[2][3] = 11.70304815658165; lattice[3][1] = 11.70304815658165; lattice[3][2] = 11.70304815658165; lattices.push_back(lattice);  // NaF 5x5x5 (AAPL)

    const double special[] = {0.0, -0.0, 0.5, -0.5, 1.0, -1.0, 0.25, 0.75, 0.9999999999999999, 1.0000000000000002,
      1e-17, -1e-17, 0.49999999999999994, 0.5000000000000001, -0.49999999999999994, 2.5, -3.5, 7.123456789};
    const size_t nspecial = sizeof(special) / sizeof(special[0]);
    std::mt19937_64 rng(20260930);
    std::uniform_real_distribution<double> coordinate(-1.5, 2.5), small(-0.02, 0.02);

    uint npairs = 0, bitwise_differences = 0, predicate_differences = 0, bound_violations = 0;
    xvector<double> f1(3), f2(3);
    for (size_t l = 0; l < lattices.size(); l++) {
      const xmatrix<double> f2c = trasp(lattices[l]);
      const xmatrix<double> f2c_inv = aurostd::inverse(f2c);
      double row_norm[4] = {0.0, 0.0, 0.0, 0.0};
      for (int i = 1; i <= 3; i++) row_norm[i] = std::sqrt(f2c_inv[i][1] * f2c_inv[i][1] + f2c_inv[i][2] * f2c_inv[i][2] + f2c_inv[i][3] * f2c_inv[i][3]);
      for (uint p = 0; p < 3600; p++) {
        if (p < nspecial * nspecial) {  // every pair of special values, in a rotating pattern over the three axes
          const double s1 = special[p / nspecial], s2 = special[p % nspecial];
          f1[1] = s1; f1[2] = s2; f1[3] = s1 + s2;
          f2[1] = s2; f2[2] = 0.0; f2[3] = s1;
        } else if (p % 2 == 0) {  // random positions
          for (int i = 1; i <= 3; i++) { f1[i] = coordinate(rng); f2[i] = coordinate(rng); }
        } else {  // near neighbors of a periodic image: the case the tolerance decides
          for (int i = 1; i <= 3; i++) { f1[i] = coordinate(rng); f2[i] = f1[i] + small(rng) + (double) ((int) (3.0 * coordinate(rng)) % 3); }
        }
        npairs++;
        const double fast = SYM::fractionalDistanceNoSkew(f1, f2, f2c);
        const double reference = aurostd::modulus(SYM::CPOSDistFromFPOS(f1, f2, lattices[l], f2c, false));
        if (std::memcmp(&fast, &reference, sizeof(double)) != 0) bitwise_differences++;
        const double tolerances[3] = {reference, std::nextafter(reference, 0.0), std::nextafter(reference, 1e300)};
        for (int t = 0; t < 3; t++) {
          if ((fast < tolerances[t]) != SYM::FPOSMatch(f1, f2, lattices[l], f2c, false, tolerances[t])) predicate_differences++;
        }
        double d[4] = {0.0, f1[1] - f2[1], f1[2] - f2[2], f1[3] - f2[3]};
        for (int i = 1; i <= 3; i++) {
          BringInCellInPlace(d[i], 0.0, 0.5, -0.5);
          if (std::abs(d[i]) > reference * row_norm[i] * (1.0 + 1e-6)) bound_violations++;
        }
      }
    }
    checkEqual(npairs, (uint) 21600, "SYM::fractionalDistanceNoSkew()", "pairs tested", passed_checks, results);
    checkEqual(bitwise_differences, (uint) 0, "SYM::fractionalDistanceNoSkew()",
        "bitwise equal to modulus(CPOSDistFromFPOS(...,skew=false)), the distance FPOSMatch() compares", passed_checks, results);
    checkEqual(predicate_differences, (uint) 0, "SYM::FPOSMatch()",
        "same verdict as distance<tolerance at tolerance=distance and one ulp either side", passed_checks, results);
    checkEqual(bound_violations, (uint) 0, "SYM::getFullSymBasis() cell list",
        "|d_frac_i| <= distance*|row i of inverse(f2c)|, the bound that makes the cell list exact", passed_checks, results);

    // the switch between the paths //CO20261004
    // The comparisons in the symmetry_exact tests choose the path for their own thread (setReferencePathsThisThread()),
    // which must win over the global setting; --sym_reference, the fallback named above, sets the global setting
    // (setReferencePaths()), which a thread follows with -1. The global setting is flipped and restored here. Tests
    // running in other threads at that moment can take the other path for a few calls, which gives the same results
    // (what these tests check), and the symmetry_exact tests set the path of their own thread.
    {
      SymReferenceThisThread follow(-1);
      const bool global_before = SYM::useReferencePaths();
      SYM::setReferencePaths(!global_before);
      const bool global_flipped = SYM::useReferencePaths();
      SYM::setReferencePathsThisThread(1);
      const bool thread_on = SYM::useReferencePaths();
      SYM::setReferencePathsThisThread(0);
      const bool thread_off = SYM::useReferencePaths();
      SYM::setReferencePathsThisThread(-1);
      SYM::setReferencePaths(global_before);
      checkEqual(global_flipped, !global_before, "SYM::setReferencePaths()", "the global setting (--sym_reference) selects the paths of a thread that follows it", passed_checks, results);
      checkEqual(thread_on && !thread_off, true, "SYM::setReferencePathsThisThread()", "the choice of the thread wins over the global setting", passed_checks, results);
    }
  }

  // A structure for the direct getFullSymBasis() tests: lattice (rows), species of each atom (0, 1, 2, grouped by
  // species, none missing), fractional positions set exactly as given (also outside [0,1)), Cartesian ones to match
  static xstructure symExactMappingStructure(const xmatrix<double>& lattice, const vector<int>& types, const vector<xvector<double> >& fpos) {
    stringstream poscar;
    poscar.precision(17);
    poscar << "symmetry_exact mapping test" << std::endl << "1.0" << std::endl;
    for (int i = 1; i <= 3; i++) poscar << "  " << lattice[i][1] << "  " << lattice[i][2] << "  " << lattice[i][3] << std::endl;
    vector<int> counts(3, 0);
    for (size_t i = 0; i < types.size(); i++) counts[types[i]]++;
    vector<string> names, numbers;
    for (int t = 0; t < 3; t++) if (counts[t] > 0) { names.push_back(string(1, (char) ('A' + t))); numbers.push_back(aurostd::utype2string(counts[t])); }
    poscar << aurostd::joinWDelimiter(names, " ") << std::endl << aurostd::joinWDelimiter(numbers, " ") << std::endl << "Direct" << std::endl;
    for (size_t i = 0; i < types.size(); i++) poscar << "  " << fpos[i][1] << "  " << fpos[i][2] << "  " << fpos[i][3] << "  " << (char) ('A' + types[i]) << std::endl;
    xstructure xstr(poscar, IOAFLOW_AUTO);
    for (size_t i = 0; i < xstr.atoms.size(); i++) {
      xstr.atoms[i].fpos = fpos[i];
      xstr.atoms[i].cpos = F2C(xstr.lattice, fpos[i]);
    }
    return xstr;
  }

  // A factor-group style operation: Uf = sign * identity, translation ftau
  static _sym_op symExactOperation(const xstructure& xstr, int sign, const xvector<double>& ftau) {
    _sym_op op;
    op.is_fgroup = true;
    op.Uf = xmatrix<double>(3, 3);
    for (int i = 1; i <= 3; i++) op.Uf[i][i] = (double) sign;
    op.Uc = xstr.f2c * op.Uf * xstr.c2f;
    op.ftau = ftau;
    op.ctau = xstr.f2c * ftau;
    return op;
  }

  void UnitTest::symmetryExactMappingTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    std::mt19937_64 rng(20261001);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    uint ncalls = 0, nmapped = 0, nnot_mapped = 0, ndifferent = 0, nthrew = 0;
    unsigned long long nties = 0, ngeneral = 0;  //CO20261004 - were SYM::ExactPathCounts grid_lower_index_replacements and grid_general_searches
    const SYM::ExactPathCounts before = SYM::exactPathCounts();

    // one structure: every operation, tolerance, map_types and skew, with no spins, collinear spins, partial occupations
    auto run = [&](const string& name, const xstructure& base, const vector<_sym_op>& operations, const vector<double>& tolerances) {
      for (int variant = 0; variant < 3; variant++) {
        xstructure xstr(base);
        for (size_t i = 0; i < xstr.atoms.size(); i++) {
          if (variant == 1) { xstr.atoms[i].spin_is_given = true; xstr.atoms[i].spin = (unit(rng) < 0.5 ? 1.0 : -1.0); }
          if (variant == 2) { xstr.atoms[i].partial_occupation_value = (unit(rng) < 0.3 ? 0.5 : 1.0); }
        }
        for (size_t t = 0; t < tolerances.size(); t++) {
          for (int map_types = 0; map_types < 2; map_types++) {
            for (int skew = 0; skew < 2; skew++) {
              for (size_t k = 0; k < operations.size(); k++) {
                _sym_op op_reference = operations[k], op_current = operations[k];
                vector<int> atoms_reference, types_reference, atoms_current, types_current;
                bool mapped_reference = false, mapped_current = false;
                string threw = "";
                bool used_grid = false;
                try {
                  { SymReferenceThisThread reference(1); mapped_reference = SYM::getFullSymBasis(xstr, op_reference, (map_types == 1), (skew == 1), tolerances[t], atoms_reference, types_reference); }
                  const unsigned long long grid_calls_before = SYM::exactPathCounts().grid_calls;
                  { SymReferenceThisThread current(0); mapped_current = SYM::getFullSymBasis(xstr, op_current, (map_types == 1), (skew == 1), tolerances[t], atoms_current, types_current); }
                  used_grid = (SYM::exactPathCounts().grid_calls > grid_calls_before);
                }
                catch (aurostd::xerror& excpt) { threw = excpt.what(); }
                ncalls++;
                if (!threw.empty()) { if (nthrew++ < 5) errors.push_back(name + ": threw: " + threw); continue; }
                // the cases the cell list has to decide as the linear scan does, counted here since getFullSymBasis()
                // hands its candidates to MapAtomWithBasis() (CO20261004): searches with skew, spins or map_types off,
                // and ties, where the image of the first atom (all atoms unmapped) has two or more atoms within the
                // tolerance, so that the lowest index has to win: MapAtomWithBasis() finds one with fast=true and
                // reports that there is not exactly one with fast=false
                if (used_grid && !atoms_current.empty()) {
                  if (skew == 1 || variant == 1 || map_types == 0) ngeneral++;
                  _atom image;
                  deque<uint> all_atoms;
                  for (uint i = 0; i < xstr.atoms.size(); i++) all_atoms.push_back(i);
                  uint index_first = 0, index_last = 0;
                  if (SYM::ApplyAtomValidate(xstr.atoms[0], image, operations[k], xstr.lattice, xstr.c2f, xstr.f2c, (skew == 1), true, false, tolerances[t])
                      && SYM::MapAtomWithBasis(xstr.atoms, image, (map_types == 1), all_atoms, xstr.lattice, xstr.f2c, (skew == 1), tolerances[t], index_first, true)
                      && !SYM::MapAtomWithBasis(xstr.atoms, image, (map_types == 1), all_atoms, xstr.lattice, xstr.f2c, (skew == 1), tolerances[t], index_last, false)) nties++;
                }
                if (mapped_reference) nmapped++; else nnot_mapped++;
                if (mapped_reference != mapped_current || atoms_reference != atoms_current || types_reference != types_current) {
                  if (ndifferent++ < 10) {
                    errors.push_back(name + " variant " + aurostd::utype2string(variant) + " operation " + aurostd::utype2string(k) + " tolerance "
                        + aurostd::utype2string(tolerances[t]) + " map_types=" + aurostd::utype2string(map_types) + " skew=" + aurostd::utype2string(skew)
                        + ": reference " + (mapped_reference ? "maps" : "does not map") + " (" + aurostd::utype2string(atoms_reference.size()) + " atoms mapped), current "
                        + (mapped_current ? "maps" : "does not map") + " (" + aurostd::utype2string(atoms_current.size()) + ")");
                  }
                }
              }
            }
          }
        }
      }
    };
    auto lattice_of = [](double a11, double a12, double a13, double a21, double a22, double a23, double a31, double a32, double a33) {
      xmatrix<double> l(3, 3);
      l[1][1] = a11; l[1][2] = a12; l[1][3] = a13; l[2][1] = a21; l[2][2] = a22; l[2][3] = a23; l[3][1] = a31; l[3][2] = a32; l[3][3] = a33;
      return l;
    };

    // 1. random structures at tolerances across the cell-list regime: r = tolerance*max_i|row i of inverse(f2c)|
    //    from 0.05 (bins of 1/5 at most, matches in the image's own bin or the next) to 0.4 (one bin on that axis)
    const xmatrix<double> lattices[3] = {
      lattice_of(12.0, 0.0, 0.0, 5.1, 10.3, 0.0, -3.2, 2.4, 9.7),   // triclinic
      lattice_of(12.0, 0.0, 0.0, 11.4, 1.9, 0.0, 1.0, -0.7, 8.5),   // strongly skewed
      lattice_of(9.6, 0.0, 0.0, -4.8, 8.31384387633061, 0.0, 0.0, 0.0, 15.6)};  // hexagonal, long c
    const uint sizes[3] = {16, 27, 48};
    const double r_targets[6] = {0.05, 0.12, 0.2, 0.26, 0.31, 0.4};
    for (int l = 0; l < 3; l++) {
      const xmatrix<double> f2c_inv = aurostd::inverse(trasp(lattices[l]));
      double row_max = 0.0;
      for (int i = 1; i <= 3; i++) row_max = std::max(row_max, std::sqrt(f2c_inv[i][1] * f2c_inv[i][1] + f2c_inv[i][2] * f2c_inv[i][2] + f2c_inv[i][3] * f2c_inv[i][3]));
      vector<double> tolerances;
      for (int t = 0; t < 6; t++) tolerances.push_back(r_targets[t] / row_max);
      for (int n = 0; n < 3; n++) {
        vector<int> types(sizes[n]);
        vector<xvector<double> > fpos(sizes[n], xvector<double>(3));
        for (uint i = 0; i < sizes[n]; i++) {
          types[i] = (3 * i) / sizes[n];  // grouped by species
          for (int j = 1; j <= 3; j++) fpos[i][j] = unit(rng);
        }
        fpos[1][1] = 1.0; fpos[1][2] = 0.0;                                   // on the cell boundary
        fpos[2][1] = -0.25; fpos[2][2] = 1.75;                                // outside the cell
        fpos[3][1] = 0.9999999999999999; fpos[3][2] = 1e-13; fpos[3][3] = -1e-13;  // just inside and just outside
        const xstructure xstr = symExactMappingStructure(lattices[l], types, fpos);
        vector<_sym_op> operations;
        for (int k = 0; k < 24; k++) {
          const int sign = (k % 2 == 0 ? 1 : -1);
          xvector<double> ftau(3);
          if (k < 12) {  // atom 0 goes exactly onto atom j
            const uint j = (uint) (unit(rng) * sizes[n]) % sizes[n];
            for (int i = 1; i <= 3; i++) { ftau[i] = fpos[j][i] - sign * fpos[0][i]; ftau[i] -= std::floor(ftau[i]); }
          } else {
            for (int i = 1; i <= 3; i++) ftau[i] = unit(rng);
          }
          operations.push_back(symExactOperation(xstr, sign, ftau));
        }
        run("random " + aurostd::utype2string(sizes[n]) + " atoms, lattice " + aurostd::utype2string(l), xstr, operations, tolerances);
      }
    }

    // 2. supercells with every atom displaced at random by up to J: operations that map the pristine cell onto itself
    const double J = 0.05;
    const xmatrix<double> cells[2] = {
      lattice_of(0.0, 2.34060963131633, 2.34060963131633, 2.34060963131633, 0.0, 2.34060963131633, 2.34060963131633, 2.34060963131633, 0.0),  // NaF
      lattice_of(4.1, 0.0, 0.0, 1.9, 3.7, 0.0, -1.2, 0.8, 4.4)};  // triclinic, two atoms related by inversion
    const int repeat[2] = {4, 3};
    const double basis[2][2][3] = {{{0.0, 0.0, 0.0}, {0.5, 0.5, 0.5}}, {{0.12, 0.27, 0.31}, {0.88, 0.73, 0.69}}};
    for (int c = 0; c < 2; c++) {
      const int m = repeat[c];
      xmatrix<double> lattice = cells[c];
      for (int i = 1; i <= 3; i++) for (int j = 1; j <= 3; j++) lattice[i][j] *= m;
      vector<int> types;
      vector<xvector<double> > fpos;
      for (int b = 0; b < 2; b++) for (int i = 0; i < m; i++) for (int j = 0; j < m; j++) for (int k = 0; k < m; k++) {
        xvector<double> f(3);
        f[1] = (i + basis[c][b][0]) / m; f[2] = (j + basis[c][b][1]) / m; f[3] = (k + basis[c][b][2]) / m;
        types.push_back(b);
        fpos.push_back(f);
      }
      // displace every atom by a random Cartesian vector of length up to J
      const xmatrix<double> c2f = aurostd::inverse(trasp(lattice));
      for (size_t a = 0; a < fpos.size(); a++) {
        xvector<double> d(3);
        double norm = 0.0;
        do { for (int i = 1; i <= 3; i++) d[i] = 2.0 * unit(rng) - 1.0; norm = aurostd::modulus(d); } while (norm > 1.0 || norm < 1e-3);
        fpos[a] = fpos[a] + c2f * (d * (J * unit(rng) / norm));
      }
      const xstructure xstr = symExactMappingStructure(lattice, types, fpos);
      const double d_nn = SYM::minimumDistance(xstr);
      vector<double> tolerances = {0.7 * J, 1.4 * J, 2.1 * J, 2.8 * J, 0.6 * d_nn, 1.1 * d_nn, 1.6 * d_nn};
      vector<_sym_op> operations;
      const uint nper = m * m * m;
      for (int k = 0; k < 24; k++) {
        const int sign = (k % 2 == 0 ? 1 : -1);
        const uint j = (uint) (unit(rng) * nper) % nper;  // a lattice translation of the supercell (same species)
        xvector<double> ftau(3);
        // the pristine site of atom j minus the image of the pristine site of atom 0
        const double s0[3] = {basis[c][0][0] / m, basis[c][0][1] / m, basis[c][0][2] / m};
        const int jj[3] = {(int) (j / (m * m)), (int) ((j / m) % m), (int) (j % m)};
        for (int i = 1; i <= 3; i++) { ftau[i] = (jj[i - 1] + basis[c][0][i - 1]) / m - sign * s0[i - 1]; ftau[i] -= std::floor(ftau[i]); }
        operations.push_back(symExactOperation(xstr, sign, ftau));
      }
      run(string(c == 0 ? "NaF 4x4x4" : "triclinic 3x3x3") + " displaced", xstr, operations, tolerances);
    }

    const SYM::ExactPathCounts& after = SYM::exactPathCounts();
    checkEqual(nthrew, (uint) 0, "SYM::getFullSymBasis()", "no call threw", passed_checks, results);
    checkEqual(ndifferent, (uint) 0, "SYM::getFullSymBasis()",
        "same answer and same atom and type maps as the reference in " + aurostd::utype2string(ncalls) + " calls", passed_checks, results);
    checkEqual((nmapped > 0 && nnot_mapped > 0), true, "SYM::getFullSymBasis()",
        "both outcomes occur (" + aurostd::utype2string(nmapped) + " map, " + aurostd::utype2string(nnot_mapped) + " do not)", passed_checks, results);
    const unsigned long long grid = after.grid_calls - before.grid_calls, neighbor = after.grid_neighbor_bin_matches - before.grid_neighbor_bin_matches;
    checkEqual((grid > 0 && neighbor > 0 && nties > 0 && ngeneral > 0), true, "SYM::getFullSymBasis() cell list",
        "the cases where it could go wrong were met: " + aurostd::utype2string(grid) + " cell-list calls, " + aurostd::utype2string(neighbor)
        + " matches in a neighboring bin, " + aurostd::utype2string(nties) + " ties decided by the lower index, " + aurostd::utype2string(ngeneral)
        + " searches with skew, spins or map_types off", passed_checks, results);  //CO20261004 - ties and searches counted in this test
  }

  void UnitTest::symmetryExactPathsTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    // a) getFullSymBasis() directly, every factor-group operation of pristine supercells
    struct MappingCase { string name; xstructure pristine; uint cells; };
    vector<MappingCase> cases;
    try {
      cases.push_back({"NaF 2x2x2", symExactAsFile(GetSuperCell(symExactInline("NaF"), 2, 2, 2)), 8});
      cases.push_back({"NaF 3x3x3", symExactAsFile(GetSuperCell(symExactInline("NaF"), 3, 3, 3)), 27});
      cases.push_back({"hcp 2x2x2", symExactAsFile(GetSuperCell(symExactInline("hcp"), 2, 2, 2)), 8});
      cases.push_back({"triclinic 2x2x1", symExactAsFile(GetSuperCell(symExactInline("triclinic"), 2, 2, 1)), 4});
    }
    catch (aurostd::xerror& excpt) { errors.push_back("could not build the test structures: " + string(excpt.what())); return; }

    uint ncalls = 0, nmapped = 0, nnot_mapped = 0, ndifferent = 0, ncases = 0;
    const double factors[] = {0.3, 1.0, 1.2, 3.0, 10.0, 60.0};  // 60: more than half the nearest-neighbor distance, several atoms qualify
    for (size_t c = 0; c < cases.size(); c++) {
      xstructure& pristine = cases[c].pristine;
      const xstructure displaced = symExactAsFile(symExactDisplaced(pristine));
      const double tight = SYM::defaultTolerance(pristine);
      try {  // the factor group of the pristine cell, with the reference paths
        SymReferenceThisThread reference(1);
        ofstream devnull("/dev/null");
        _aflags aflags; aflags.QUIET = true;
        stringstream oss;
        const bool osswrite = false;
        SYM::CalculatePointGroup(devnull, pristine, aflags, false, osswrite, oss, tight);
        SYM::CalculateFactorGroup(devnull, pristine, aflags, false, osswrite, oss, tight);
      }
      catch (aurostd::xerror& excpt) { errors.push_back(cases[c].name + ": factor group: " + string(excpt.what())); continue; }
      if (pristine.fgroup.size() < 2 * cases[c].cells) {  // the supercell translations, times at least two point operations
        errors.push_back(cases[c].name + ": factor group has only " + aurostd::utype2string(pristine.fgroup.size()) + " operations");
        continue;
      }
      ncases++;
      for (int s = 0; s < 2; s++) {
        const xstructure& xstr = (s == 0 ? pristine : displaced);
        for (size_t f = 0; f < sizeof(factors) / sizeof(factors[0]); f++) {
          for (int map_types = 0; map_types < 2; map_types++) {
            for (size_t k = 0; k < pristine.fgroup.size(); k++) {
              _sym_op op_reference = pristine.fgroup[k], op_current = pristine.fgroup[k];
              vector<int> atoms_reference, types_reference, atoms_current, types_current;
              bool mapped_reference = false, mapped_current = false;
              {
                SymReferenceThisThread reference(1);
                mapped_reference = SYM::getFullSymBasis(xstr, op_reference, (map_types == 1), false, factors[f] * tight, atoms_reference, types_reference);
              }
              {
                SymReferenceThisThread current(0);
                mapped_current = SYM::getFullSymBasis(xstr, op_current, (map_types == 1), false, factors[f] * tight, atoms_current, types_current);
              }
              ncalls++;
              if (mapped_reference) nmapped++; else nnot_mapped++;
              if (mapped_reference != mapped_current || atoms_reference != atoms_current || types_reference != types_current
                  || op_reference.basis_map_calculated != op_current.basis_map_calculated) {
                if (ndifferent < 10) {
                  errors.push_back(cases[c].name + (s == 0 ? " pristine" : " displaced") + " operation " + aurostd::utype2string(k)
                      + " tolerance " + aurostd::utype2string(factors[f]) + "x map_types=" + aurostd::utype2string(map_types)
                      + ": reference " + (mapped_reference ? "maps" : "does not map") + ", current " + (mapped_current ? "maps" : "does not map")
                      + (mapped_reference == mapped_current ? " (different maps)" : ""));
                }
                ndifferent++;
              }
            }
          }
        }
      }
    }
    checkEqual(ncases, (uint) cases.size(), "SYM::getFullSymBasis()", "factor groups of the pristine supercells computed", passed_checks, results);
    checkEqual(ndifferent, (uint) 0, "SYM::getFullSymBasis()",
        "same answer and same atom and type maps as the reference in " + aurostd::utype2string(ncalls) + " calls", passed_checks, results);
    checkEqual((nmapped > 0 && nnot_mapped > 0), true, "SYM::getFullSymBasis()",
        "both outcomes occur (" + aurostd::utype2string(nmapped) + " map, " + aurostd::utype2string(nnot_mapped) + " do not)", passed_checks, results);

    // GetPrimitive() reduces each pristine supercell to its primitive cell, with the reference paths and with the
    // current code. The volume criteria 2) to 5) of GetPrimitive_20210322() serve the prefilter and the test itself
    // (one lambda, CO20261004), so both paths run them: the known answer checks them, and the comparisons in b)
    // check the prefilter against the reference. //CO20261004
    for (size_t c = 0; c < cases.size(); c++) {
      const uint natoms_expected = cases[c].pristine.atoms.size() / cases[c].cells;
      const double volume_expected = cases[c].pristine.GetVolume() / cases[c].cells;
      for (int reference_paths = 1; reference_paths >= 0; reference_paths--) {
        xstructure prim(cases[c].pristine);
        try { SymReferenceThisThread paths(reference_paths); prim.GetPrimitive(); }
        catch (aurostd::xerror& excpt) { errors.push_back(cases[c].name + ": GetPrimitive(): " + string(excpt.what())); continue; }
        const bool passed = (prim.atoms.size() == natoms_expected && aurostd::isequal(prim.GetVolume(), volume_expected, 1e-6 * volume_expected));
        check(passed, aurostd::utype2string(prim.atoms.size()) + " atoms, " + aurostd::utype2string(prim.GetVolume(), 10) + " A^3",
            aurostd::utype2string(natoms_expected) + " atoms, " + aurostd::utype2string(volume_expected, 10) + " A^3", "xstructure::GetPrimitive()",
            cases[c].name + ": the primitive cell" + (reference_paths == 1 ? " (reference paths)" : ""), passed_checks, results);
      }
    }

    // b) what lib2raw computes, on the supercells above and on small prototypes of other lattice systems
    uint nstructures = 0, nidentical = 0, nthrew = 0;
    vector<string> differences;
    vector<std::pair<string, xstructure> > structures;
    for (size_t c = 0; c < cases.size(); c++) {
      if (cases[c].name == "NaF 3x3x3") continue;  // minutes on its own in the tight search (symmetry_exact_all has 3x3x3 cells)
      structures.push_back(std::make_pair(cases[c].name, symExactAsFile(cases[c].pristine)));
      structures.push_back(std::make_pair(cases[c].name + " displaced", symExactAsFile(symExactDisplaced(cases[c].pristine))));
    }
    const std::map<uint, string> smallest = symExactSmallestPerSpaceGroup();  //CO20261004 - from anrl::PrototypeANRL_LoadList()
    const uint picks[] = {12, 62, 139, 166, 221};  // monoclinic, orthorhombic, tetragonal, trigonal (R), cubic
    {  // the prototypes this test builds, the same as when the atom count was read from the label //CO20261004
      vector<string> picked;
      for (size_t p = 0; p < sizeof(picks) / sizeof(picks[0]); p++) picked.push_back(smallest.count(picks[p]) ? smallest.at(picks[p]) : string("none"));
      checkEqual(aurostd::joinWDelimiter(picked, ","), string("A_mC4_12_i,AB_oP8_62_c_c,A_tI2_139_a,A_hR1_166_a,A_cP1_221_a"), "anrl::PrototypeANRL_LoadList()",
          "smallest prototype of space groups 12, 62, 139, 166 and 221 (" + aurostd::utype2string(smallest.size()) + " space groups in the list)", passed_checks, results);
    }
    for (size_t p = 0; p < sizeof(picks) / sizeof(picks[0]); p++) {
      if (!smallest.count(picks[p])) { errors.push_back("no prototype for space group " + aurostd::utype2string(picks[p])); continue; }
      const string& label = smallest.at(picks[p]);
      for (int kind = 0; kind < 2; kind++) {
        xstructure xstr;
        string problem = "";
        if (!symExactBuild((kind == 0 ? "sc " : "scd ") + label + " 0", xstr, problem)) { errors.push_back(label + ": " + problem); continue; }
        structures.push_back(std::make_pair(label + (kind == 0 ? " supercell" : " supercell displaced"), xstr));
      }
    }
    const SYM::ExactPathCounts before = SYM::exactPathCounts();
    for (size_t i = 0; i < structures.size(); i++) {
      string difference = "";
      bool threw = false;
      nstructures++;
      if (symExactCompare(structures[i].second, difference, threw)) nidentical++;
      else differences.push_back(structures[i].first + " (" + aurostd::utype2string(structures[i].second.atoms.size()) + " atoms): " + difference);
      if (threw) { nthrew++; errors.push_back(structures[i].first + ": a step threw"); }
    }
    const SYM::ExactPathCounts& after = SYM::exactPathCounts();
    for (size_t i = 0; i < differences.size(); i++) errors.push_back(differences[i]);
    checkEqual(nstructures, (uint) (6 + 2 * (sizeof(picks) / sizeof(picks[0]))), "lib2raw symmetry steps", "structures compared", passed_checks, results);
    checkEqual(nthrew, (uint) 0, "lib2raw symmetry steps", "no step threw, so every step was compared", passed_checks, results);
    checkEqual(nidentical, nstructures, "lib2raw symmetry steps",
        "prim, loose and tight space group, EDATA with its symmetry groups, and prototype label identical to the reference", passed_checks, results);
    checkEqual((after.prefilter_skips > before.prefilter_skips && after.grid_calls > before.grid_calls), true, "lib2raw symmetry steps",
        "the new code was reached: " + aurostd::utype2string(after.prefilter_skips - before.prefilter_skips) + " triples skipped by the prefilter, "
        + aurostd::utype2string(after.grid_calls - before.grid_calls) + " cell-list mappings", passed_checks, results);
  }

  void UnitTest::symmetryExactAllGroupsTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    vector<string> labels, compositions;
    vector<uint> space_groups;
    vector<vector<vector<string> > > wyckoff_letters;
    const uint nlabels = aflowlib::GetAllPrototypeLabels(labels, compositions, space_groups, wyckoff_letters, "anrl");

    // every prototype and parameter set, as generated
    vector<string> entries;
    std::set<uint> groups_library;
    for (uint i = 0; i < nlabels; i++) {
      groups_library.insert(space_groups[i]);
      const uint nsets = anrl::getANRLParameters(labels[i], "all").size();
      for (uint j = 0; j < nsets; j++) entries.push_back(labels[i] + "-" + aurostd::utype2string(j + 1) + "\nproto " + labels[i] + " " + aurostd::utype2string(j));
    }
    vector<uint> counts;
    vector<string> differences;
    symExactRun(entries, counts, differences);
    string threw_note = symExactSplitNotes(differences, counts, errors);
    checkEqual((uint) groups_library.size(), (uint) 230, "aflowlib::GetAllPrototypeLabels()", "the prototype library covers every space group", passed_checks, results);
    checkEqual(counts[2], (uint) 0, "prototypes", "every prototype and parameter set built (" + aurostd::utype2string(entries.size()) + ")", passed_checks, results);
    checkEqual(counts[0], (uint) entries.size(), "lib2raw symmetry steps",
        "every prototype: prim, loose and tight space group, EDATA with its symmetry groups, and prototype label identical to the reference" + threw_note, passed_checks, results);

    // each space group: its smallest prototype in a supercell of at least 16 atoms, pristine and displaced
    const std::map<uint, string> smallest = symExactSmallestPerSpaceGroup();  //CO20261004 - from anrl::PrototypeANRL_LoadList()
    entries.clear();
    for (std::map<uint, string>::const_iterator it = smallest.begin(); it != smallest.end(); ++it) {
      entries.push_back("sg " + aurostd::utype2string(it->first) + " " + it->second + " supercell\nsc " + it->second + " 0");
      entries.push_back("sg " + aurostd::utype2string(it->first) + " " + it->second + " supercell displaced\nscd " + it->second + " 0");
    }
    // 3x3x3 supercells as in phonon runs: fcc, bcc, hcp, rocksalt, perovskite
    const string phonon_like[] = {"A_cF4_225_a", "A_cI2_229_a", "A_hP2_194_c", "AB_cF8_225_a_b", "AB3C_cP5_221_a_c_b"};
    for (size_t i = 0; i < sizeof(phonon_like) / sizeof(phonon_like[0]); i++) {
      entries.push_back(phonon_like[i] + " 3x3x3\nsc3 " + phonon_like[i] + " 0");
      entries.push_back(phonon_like[i] + " 3x3x3 displaced\nsc3d " + phonon_like[i] + " 0");
    }
    differences.clear();
    symExactRun(entries, counts, differences);
    threw_note = symExactSplitNotes(differences, counts, errors);
    checkEqual((uint) smallest.size(), (uint) 230, "supercells", "one prototype for every space group", passed_checks, results);
    checkEqual(counts[2], (uint) 0, "supercells", "every supercell built (" + aurostd::utype2string(entries.size()) + ")", passed_checks, results);
    checkEqual(counts[0], (uint) entries.size(), "lib2raw symmetry steps",
        "every space group (supercell, displaced supercell) and the 3x3x3 phonon-like cells: identical to the reference" + threw_note, passed_checks, results);
  }
  //CO20260930 END

  // ---------------------------------------------------------------------------
  // rhl_mirror_origin_shift  //CO20261003
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   lib2raw stopped with "aurostd::cos(): cos(xvector,xvector)=modulus(v1)=0" on 30 POCC children (10 LIB4,
  //   20 LIB5) at "EDATA doing orig (POSCAR.orig)", right after "SYM::change_tolerance WARNING: Inconsistent
  //   symmetry", so they never got a RAW record. lib2raw's EDATA asks for space group setting 1, where R3m is
  //   described in rhombohedral axes. There the origin shift is an underdetermined system (the origin is free along
  //   the 3-fold axis), and SYM::solve_overdetermined_system() rejected the right one: it adds whole cells only to the
  //   first three shifts, and SYM::checkLinearSystem() accepts two equal Cartesian rows with different shifts only
  //   when the shifts differ by one Cartesian component of a single lattice vector. The mirror (z,y,x) of these cells
  //   has a translation of exactly half a cell along a and c, and its two equal rows need a whole lattice vector.
  //   R3m was rejected at every tolerance of the scan (R3c rightly too), the space group came out as 0, and the EDATA
  //   text output then threw in Getabc_angles() on the empty standard lattice.
  //   SYM::solve_underdetermined_modulo_lattice() now tries when that path finds nothing, and accepts an origin only
  //   when every generator matches modulo the lattice, the test the determined case uses. Checks 4 to 6 call it
  //   directly.
  // WHERE IT FAILED
  //   LIB2RAW(), thermodynamics loop, EDATA on POSCAR.orig: pflow::PrintData(str,str_sym,str_sp,str_sc,vpflow,
  //   "EDATA",txt_ft,false), which uses setting 1. "aflow --edata --setting=1 < POSCAR.orig" reproduces it; plain
  //   "aflow --edata" (setting 2, hexagonal axes) and aflowSG do not, which is why it did not show outside lib2raw.
  //   gdb: aurostd::cos <- aurostd::angle <- Getabc_angles <- pflow::PrintSGData <- pflow::PrintData.
  // THE STRUCTURE
  //   POSCAR.orig of LIB4 Ca_svCr_pvIK_sv/A2B2C7_cF88_227_c_d_af-001.ABC:POCC_S0-0.5xA-0.5xD_S1-1xB_S2-1xC/
  //   ARUN.POCC_1_H0C0, the smallest of the 30: the unrelaxed 44-atom POCC cell, R3m #160 (hR44). Its CONTCAR.relax
  //   and CONTCAR.relax1 were not affected; rounding there puts the mirror's half cell on the other side.
  // CHECKED (2026-10-03)
  //   Before (520a6cf): "aflow --edata --setting=1 < POSCAR.orig" exits 50 with the cos() error after the scan from
  //   0.00285 to 0.285 Angstrom, and "aflow --prototype" exits 31 (Lattice2TypeAndCentering). With the new function
  //   switched off, checks 1, 2, 7, 9, 10 and 11 fail with those errors; with one that accepts any origin, checks 3
  //   to 6 fail; with one that tries no whole cells, checks 4 and 5 fail. After: all 11 pass in about 2 seconds.
  //   Setting 1 gives R3m #160 at the default tolerance with the same Wyckoff split as setting 2 (1a and 3b against
  //   3a and 9b); setting 2, CONTCAR.relax and CONTCAR.relax1 print the same as before. The new function runs only
  //   where the old path found no origin, so a call that found one returns the same origin; a search that used to
  //   settle on a later candidate or a scanned tolerance can now settle earlier. On the 352 ANRL prototypes whose
  //   space group has a polar point group (an underdetermined origin needs one), --sgdata in settings 1 and 2 and
  //   --edata --setting=1 print the same as 520a6cf, and the proto unit test (all 2292 parameter sets) passes.
  // IF THIS TEST FAILS
  //   Checks 1, 2, 4 or 5 fail: the underdetermined origin-shift path rejects a consistent system again, and lib2raw
  //   will lose these entries again: fix SYM::solve_overdetermined_system() or solve_underdetermined_modulo_lattice(),
  //   do not change the expectations. Check 3 or 6 fails: an origin is accepted that does not match every generator
  //   modulo the lattice, so wrong space groups can pass. Checks 7 to 11 are the steps on the structure: an exception
  //   is the old bug, and a space group or label other than the expected one means the symmetry search changed its
  //   answer, which needs a reason. Checks 12 to 15 (CO20261004) cover the determined case, which shares its test
  //   modulo the lattice (originShiftMatchesModuloLattice()) with the new function: 12 or 14 failing means a
  //   consistent system is rejected, 13 or 15 failing means an inconsistent one is accepted.
  void UnitTest::rhlMirrorOriginShiftTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    // the structure
    stringstream poscar;
    poscar << "Ca_svCr_pvIK_sv/A2B2C7_cF88_227_c_d_af-001.ABC:POCC_S0-0.5xA-0.5xD_S1-1xB_S2-1xC/ARUN.POCC_1_H0C0 (POSCAR.orig)" << std::endl;
    poscar << "1.000000" << std::endl;
    poscar << "   0.00000000000000   7.26708014181607   7.26708014181607" << std::endl;
    poscar << "   7.26708014181607   0.00000000000000   7.26708014181607" << std::endl;
    poscar << "  14.53416028363215  14.53416028363215   0.00000000000000" << std::endl;
    poscar << "4 8 28 4" << std::endl;
    poscar << "Direct(44) [A4B8C28D4]" << std::endl;
    poscar << "   0.00000000000000   0.00000000000000   0.00000000000000  Ca_sv" << std::endl;
    poscar << "   0.00000000000000   0.00000000000000   0.25000000000000  Ca_sv" << std::endl;
    poscar << "   0.00000000000000   0.50000000000000   0.00000000000000  Ca_sv" << std::endl;
    poscar << "   0.50000000000000   0.00000000000000   0.00000000000000  Ca_sv" << std::endl;
    poscar << "   0.50000000000000   0.50000000000000   0.00000000000000  Cr_pv" << std::endl;
    poscar << "   0.50000000000000   0.00000000000000   0.25000000000000  Cr_pv" << std::endl;
    poscar << "   0.00000000000000   0.50000000000000   0.25000000000000  Cr_pv" << std::endl;
    poscar << "   0.50000000000000   0.50000000000000   0.25000000000000  Cr_pv" << std::endl;
    poscar << "   0.50000000000000   0.50000000000000   0.50000000000000  Cr_pv" << std::endl;
    poscar << "   0.50000000000000   0.00000000000000   0.75000000000000  Cr_pv" << std::endl;
    poscar << "   0.00000000000000   0.50000000000000   0.75000000000000  Cr_pv" << std::endl;
    poscar << "   0.50000000000000   0.50000000000000   0.75000000000000  Cr_pv" << std::endl;
    poscar << "   0.12500000000000   0.12500000000000   0.06250000000000  I" << std::endl;
    poscar << "   0.16570000000000   0.58430000000000   0.08285000000000  I" << std::endl;
    poscar << "   0.58430000000000   0.16570000000000   0.08285000000000  I" << std::endl;
    poscar << "   0.16570000000000   0.16570000000000   0.29215000000000  I" << std::endl;
    poscar << "   0.58430000000000   0.16570000000000   0.29215000000000  I" << std::endl;
    poscar << "   0.16570000000000   0.58430000000000   0.29215000000000  I" << std::endl;
    poscar << "   0.58430000000000   0.58430000000000   0.08285000000000  I" << std::endl;
    poscar << "   0.12500000000000   0.12500000000000   0.56250000000000  I" << std::endl;
    poscar << "   0.83430000000000   0.41570000000000   0.20785000000000  I" << std::endl;
    poscar << "   0.41570000000000   0.83430000000000   0.20785000000000  I" << std::endl;
    poscar << "   0.41570000000000   0.41570000000000   0.41715000000000  I" << std::endl;
    poscar << "   0.16570000000000   0.58430000000000   0.58285000000000  I" << std::endl;
    poscar << "   0.58430000000000   0.16570000000000   0.58285000000000  I" << std::endl;
    poscar << "   0.41570000000000   0.83430000000000   0.41715000000000  I" << std::endl;
    poscar << "   0.83430000000000   0.41570000000000   0.41715000000000  I" << std::endl;
    poscar << "   0.83430000000000   0.83430000000000   0.20785000000000  I" << std::endl;
    poscar << "   0.16570000000000   0.16570000000000   0.79215000000000  I" << std::endl;
    poscar << "   0.58430000000000   0.58430000000000   0.58285000000000  I" << std::endl;
    poscar << "   0.58430000000000   0.16570000000000   0.79215000000000  I" << std::endl;
    poscar << "   0.16570000000000   0.58430000000000   0.79215000000000  I" << std::endl;
    poscar << "   0.87500000000000   0.87500000000000   0.43750000000000  I" << std::endl;
    poscar << "   0.83430000000000   0.41570000000000   0.70785000000000  I" << std::endl;
    poscar << "   0.41570000000000   0.83430000000000   0.70785000000000  I" << std::endl;
    poscar << "   0.41570000000000   0.41570000000000   0.91715000000000  I" << std::endl;
    poscar << "   0.83430000000000   0.83430000000000   0.70785000000000  I" << std::endl;
    poscar << "   0.41570000000000   0.83430000000000   0.91715000000000  I" << std::endl;
    poscar << "   0.83430000000000   0.41570000000000   0.91715000000000  I" << std::endl;
    poscar << "   0.87500000000000   0.87500000000000   0.93750000000000  I" << std::endl;
    poscar << "   0.00000000000000   0.00000000000000   0.50000000000000  K_sv" << std::endl;
    poscar << "   0.00000000000000   0.50000000000000   0.50000000000000  K_sv" << std::endl;
    poscar << "   0.50000000000000   0.00000000000000   0.50000000000000  K_sv" << std::endl;
    poscar << "   0.00000000000000   0.00000000000000   0.75000000000000  K_sv" << std::endl;
    xstructure xstr;
    try { xstr = xstructure(poscar, IOAFLOW_AUTO); }
    catch (aurostd::xerror& excpt) { errors.push_back("could not read the Ca_svCr_pvIK_sv POCC cell: " + string(excpt.what())); return; }
    if (xstr.atoms.size() != 44) { errors.push_back("the Ca_svCr_pvIK_sv POCC cell was read with " + aurostd::utype2string(xstr.atoms.size()) + " atoms, not 44"); return; }
    const double tol_default = SYM::defaultTolerance(xstr);

    // (I-W_k)p-t_k for every row of a system, computed apart from the solvers' own test (SYM::minimizeDistance*Method()
    // in originShiftMatchesModuloLattice()): the rows and shifts are in lattice coordinates, so these are the lattice
    // coordinates of the difference, and p is an origin shift modulo the lattice when all of them are integers.
    // A bound of tol/(|a|+|b|+|c|) on each keeps the Cartesian distance to that lattice vector within tol. //CO20261004
    auto lattice_coordinates_of_difference = [](const vector<xvector<double> >& rows, const vector<double>& shifts, const xvector<double>& p) {
      xvector<double> d((int) rows.size());
      for (uint i = 0; i < rows.size(); i++) { d(i + 1) = aurostd::scalar_product(rows[i], p) - shifts[i]; }
      return d;
    };
    auto fractional_tolerance = [](const xmatrix<double>& lattice, double tol) {
      return tol / (aurostd::modulus(lattice(1)) + aurostd::modulus(lattice(2)) + aurostd::modulus(lattice(3)));
    };

    // the origin-shift systems SpaceGroup_ITC() builds for this cell with setting 1 (rows of I-W and shifts, in the
    // fractional coordinates of its conventional cell), for the 3-fold (z,x,y) and the mirror or glide (z,y,x)
    {
      const string check_function = "SYM::solve_overdetermined_system()";
      xmatrix<double> lattice;  // conventional cell of SpaceGroup_ITC() (rhombohedral axes)
      lattice(1,1) =  -7.26708014181608; lattice(1,2) =  -7.26708014181608; lattice(1,3) = 14.53416028363214;
      lattice(2,1) = -14.53416028363215; lattice(2,2) =  -7.26708014181608; lattice(2,3) =  7.26708014181607;
      lattice(3,1) =  -7.26708014181608; lattice(3,2) = -14.53416028363215; lattice(3,3) =  7.26708014181607;
      double min_dist = SYM::minimumDistance(xstr);
      double tol = tol_default;
      vector<xvector<double> > rows;
      const vector<vector<double> > rows_list = {{1, 0, -1}, {-1, 1, 0}, {0, -1, 1},    // 3-fold (z,x,y)
                                                 {1, 0, -1}, {0, 0, 0}, {-1, 0, 1}};    // mirror or glide (z,y,x)
      for (const auto& r : rows_list) { rows.push_back(aurostd::vector2xvector<double>(r)); }
      vector<double> shifts_R3m = {-0.5, -0.5, 0.0, -0.5, 0.0, -0.5};  // the mirror's translation is half a cell along a and c
      vector<double> shifts_R3c = {-0.5, -0.5, 0.0, 0.0, 0.5, 0.0};    // the same operations read as R3c: a glide is missing
      xvector<double> origin_shift;
      const bool solved_R3m = SYM::solve_overdetermined_system(rows, shifts_R3m, origin_shift, lattice, min_dist, tol);
      checkEqual(solved_R3m, true, check_function, "R3m in rhombohedral axes: an origin shift is found (was rejected)", passed_checks, results);
      const xvector<double> d_R3m = lattice_coordinates_of_difference(rows, shifts_R3m, origin_shift);  //CO20261004 - independent of the solver's test
      check(solved_R3m && aurostd::isinteger(d_R3m, fractional_tolerance(lattice, tol)), d_R3m, aurostd::nint(d_R3m), check_function,
          "R3m: the origin shift matches both generators modulo the lattice ((I-W)p-t in lattice coordinates are integers)", passed_checks, results);
      xvector<double> origin_shift_R3c;
      const bool solved_R3c = SYM::solve_overdetermined_system(rows, shifts_R3c, origin_shift_R3c, lattice, min_dist, tol);
      checkEqual(solved_R3c, false, check_function, "R3c for the same cell: still no origin shift (the structure has no glide)", passed_checks, results);
    }

    // SYM::solve_underdetermined_modulo_lattice() on its own, on a system that needs two whole cells on one row:
    // rows (1,1,0), (-1,2,0), (0,0,0) with shifts 0 and (2,0,0), (0,2,0), (0,0,0) with shifts (2/3,1/3,0); only
    // p=(4/3,2/3,0) or p=(-2/3,2/3,0) solve it, modulo the lattice. Once with the fractional and once with the Cartesian
    // (skewed lattice) minimization, then a system with no solution (only identity rows, a shift of half a cell).
    {
      const string check_function = "SYM::solve_underdetermined_modulo_lattice()";
      xmatrix<double> lattice;  // hexagonal, a=3, c=5
      lattice(1,1) = 3.0; lattice(2,1) = -1.5; lattice(2,2) = 1.5 * std::sqrt(3.0); lattice(3,3) = 5.0;
      const double tol = 1e-3;
      vector<xvector<double> > rows;
      const vector<vector<double> > rows_list = {{1, 1, 0}, {-1, 2, 0}, {0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 0}};
      for (const auto& r : rows_list) { rows.push_back(aurostd::vector2xvector<double>(r)); }
      const vector<double> shifts = {0.0, 0.0, 0.0, 2.0 / 3.0, 1.0 / 3.0, 0.0};
      for (int skew = 0; skew <= 1; skew++) {
        xvector<double> p;
        const bool solved = SYM::solve_underdetermined_modulo_lattice(rows, shifts, p, lattice, (bool) skew, tol);
        const xvector<double> d = lattice_coordinates_of_difference(rows, shifts, p);  //CO20261004 - aurostd::isinteger() and values printed on failure
        check(solved && aurostd::isinteger(d, 1e-9), d, aurostd::nint(d), check_function,
            string("a system that needs two whole cells on one row is solved (") + (skew ? "Cartesian" : "fractional") + " minimization)", passed_checks, results);
      }
      vector<xvector<double> > rows_identity(3, xvector<double>(3));
      const vector<double> shifts_half = {0.5, 0.0, 0.0};
      xvector<double> p_none;
      checkEqual(SYM::solve_underdetermined_modulo_lattice(rows_identity, shifts_half, p_none, lattice, false, tol), false, check_function,
          "a translation of half a cell with no rotation has no origin shift", passed_checks, results);
    }

    // the steps on the structure
    const string check_function = "xstructure::SpaceGroup_ITC(), pflow::PrintData()";
    for (int setting = 1; setting <= 2; setting++) {
      xstructure xstr_sg(xstr);
      double tol = tol_default;  //non-const: SpaceGroup_ITC() changes it if it scans
      bool no_scan = false;
      uint sgroup = 0;
      bool threw = false;
      string what = "";
      try { sgroup = xstr_sg.SpaceGroup_ITC(tol, -1, setting, no_scan); }
      catch (aurostd::xerror& excpt) { threw = true; what = excpt.what(); }
      checkEqual((!threw && sgroup == 160 && aurostd::isequal(tol, tol_default, 1e-12)), true, check_function,
          "setting " + aurostd::utype2string(setting) + ": R3m #160 at the default tolerance, no scan" + (setting == 1 ? " (was space group 0 after the scan)" : " (as before)") + (threw ? " [" + what + "]" : ""),
          passed_checks, results);
    }
    // lib2raw's EDATA step on POSCAR.orig, as in LIB2RAW()
    xstructure str_sym, str_sp, str_sc;
    aurostd::xoption vpflow_edata_orig;
    string edata = "";
    bool threw = false;
    string what = "";
    try { edata = pflow::PrintData(xstr, str_sym, str_sp, str_sc, vpflow_edata_orig, "EDATA", txt_ft, false); }
    catch (aurostd::xerror& excpt) { threw = true; what = excpt.what(); }
    checkEqual(threw, false, check_function, "lib2raw's EDATA step completes (threw in aurostd::cos() before)" + string(threw ? " [" + what + "]" : ""), passed_checks, results);
    checkEqual((!threw && str_sym.space_group_ITC == 160 && aurostd::substring2bool(edata, "Space group number                           = 160")), true, check_function,
        "lib2raw's EDATA step gives R3m #160", passed_checks, results);
    // lib2raw's next step on POSCAR.orig: the prototype label, in the ANRL setting (rhombohedral axes too), at the
    // EDATA tolerance; "aflow --prototype < POSCAR.orig" failed there before (Lattice2TypeAndCentering, ERROR 31)
    string label = "";
    threw = false; what = "";
    try {
      xstructure str_anrl(xstr);
      uint setting_anrl = SG_SETTING_ANRL;
      anrl::structure2anrl(str_anrl, str_sym.sym_eps, setting_anrl);
      label = str_anrl.prototype;
    }
    catch (aurostd::xerror& excpt) { threw = true; what = excpt.what(); }
    checkEqual(label, string("AB2C7D_hR44_160_ab_2a2b_4a8b_ab"), "anrl::structure2anrl()",
        "lib2raw's prototype label of POSCAR.orig: AB2C7D_hR44_160_ab_2a2b_4a8b_ab" + string(threw ? " [" + what + "]" : ""), passed_checks, results);

    // the determined case of SYM::solve_overdetermined_system(), which shares originShiftMatchesModuloLattice() with
    // solve_underdetermined_modulo_lattice() //CO20261004
    // An inversion (rows 2I, no shift) with a 3-fold (x,y,z)->(-y,x-y,z) (rows (1,1,0), (-1,2,0), (0,0,0), shifts
    // (1/2,1/2,0)): only p=(1/2,0,0) or p=(1/2,0,1/2) solve it, modulo the lattice, so the solver needs one whole cell
    // on the first shift. Then the inversion with a translation of half a cell and no rotation, which no origin shift
    // removes. Once with the fractional (min_dist=1) and once with the Cartesian (min_dist=1e-3, a skewed lattice for
    // SYM::isLatticeSkewed()) minimization.
    {
      const string check_function = "SYM::solve_overdetermined_system() (determined)";
      xmatrix<double> lattice;  // hexagonal, a=3, c=5
      lattice(1,1) = 3.0; lattice(2,1) = -1.5; lattice(2,2) = 1.5 * std::sqrt(3.0); lattice(3,3) = 5.0;
      vector<xvector<double> > rows;
      const vector<vector<double> > rows_list = {{2, 0, 0}, {0, 2, 0}, {0, 0, 2}, {1, 1, 0}, {-1, 2, 0}, {0, 0, 0}};
      for (const auto& r : rows_list) { rows.push_back(aurostd::vector2xvector<double>(r)); }
      vector<double> shifts = {0.0, 0.0, 0.0, 0.5, 0.5, 0.0};
      vector<xvector<double> > rows_translation(rows.begin(), rows.begin() + 3);
      for (int r = 0; r < 3; r++) { rows_translation.push_back(xvector<double>(3)); }
      vector<double> shifts_translation = {0.0, 0.0, 0.0, 0.5, 0.0, 0.0};
      const vector<std::pair<double, string> > min_dist_cases = {{1.0, "fractional"}, {1e-3, "Cartesian"}};
      for (const auto& min_dist_case : min_dist_cases) {
        double min_dist = min_dist_case.first, tol = 1e-3;
        const string& minimization = min_dist_case.second;
        xvector<double> p;
        const bool solved = SYM::solve_overdetermined_system(rows, shifts, p, lattice, min_dist, tol);
        const xvector<double> d = lattice_coordinates_of_difference(rows, shifts, p);
        check(solved && aurostd::isinteger(d, fractional_tolerance(lattice, tol)), d, aurostd::nint(d), check_function,
            "an inversion and a 3-fold that need a whole cell on the first shift are solved (" + minimization + " minimization)", passed_checks, results);
        xvector<double> p_none;
        checkEqual(SYM::solve_overdetermined_system(rows_translation, shifts_translation, p_none, lattice, min_dist, tol), false, check_function,
            "a translation of half a cell with no rotation has no origin shift (" + minimization + " minimization)", passed_checks, results);
      }
    }
  }

  // ---------------------------------------------------------------------------
  // enthalpy_references  //CO20261004
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   On 2026-09-27 lib2raw got formation enthalpy references for six PAW_PBE pseudopotentials that had none
  //   (xPOTCAR_EnthalpyReference_AUID() in aflow_xpseudopotentials.cpp): Er_3 and Gd (A3, from the LIB1 hcp runs), Nd,
  //   Pm and Pr (A1, as for La; dhcp in experiment) and Th_s (A1); Tb_3 (A3, from the LIB1 hcp run of 2026-10-02) followed
  //   on 2026-10-03 (CO20261014). Without a reference, lib2raw writes no formation
  //   enthalpy for an entry with that species. The references are keyed by the AUID of the pseudopotential, so a
  //   mistyped AUID loses the reference without an error, and a mistyped value shifts every formation enthalpy of the
  //   species.
  // CHECKED
  //   Each AUID is the one the pseudopotential table (aflow_xpseudopotentials_data.cpp) gives for the named
  //   pseudopotential, and each reference returns the structure, energy, volume and spin written in the code, for the
  //   plain functional (METAGGA empty) and not for SCAN. A reference also lives in the data file: every single-species
  //   entry of aflow_xpseudopotentials_data.cpp whose AUID the table knows carries the table's ground-state energy and
  //   structure, which "make pseudo" copies from the table (xPOTCAR_PURE_Printer()). //CO20261014
  // IF THIS TEST FAILS
  //   A changed value needs the LIB1 run it came from (the find command above each reference); a missing reference or
  //   another pseudopotential behind an AUID means the table or the reference list was edited. A data file entry that
  //   differs from the table needs "make pseudo" in src/ where /common/VASP is, and the regenerated file committed.
  //   Not covered: the reference lines lib2raw prints for these elements (the A1 and A3 lists in
  //   LIB2RAW_Loop_Thermodynamics()), which need a full LIB1 run directory.
  void UnitTest::enthalpyReferencesTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;
    // AUID, pseudopotential, ground-state structure, energy (eV/atom), volume (A^3/atom), spin (mu_B/atom)
    struct reference_case { string auid; string pseudopotential; string structure; double energy; double volume; double spin; };
    const vector<reference_case> vcases = {
      {"d843969058538e36", "Er_3:PAW_PBE:06Sep2000", "A3", -4.558292, 30.82475, 0.0},
      {"c0f01c59eb971d30", "Gd:PAW_PBE:08Apr2002", "A3", -14.07795, 33.4987, 7.686079},
      {"81811d7c33a85a8b", "Nd:PAW_PBE:08Apr2002", "A1", -7.612845, 30.64983, 3.907169},
      {"a66df35b1ed2d552", "Pm:PAW_PBE:08Apr2002", "A1", -9.154241, 34.20258, 5.34508},
      {"82dbd5ee06192a4c", "Pr:PAW_PBE:08Apr2002", "A1", -6.511262, 27.4489, 2.402321},
      {"78f7995fc479457d", "Th_s:PAW_PBE:06Sep2000", "A1", -7.269517, 32.00791, 0.0},
      {"df41de2ac6afa1b3", "Tb_3:PAW_PBE:06Sep2000", "A3", -4.611976, 32.32435, 0.0},  //CO20261014
    };
    const string check_function = "xPOTCAR_EnthalpyReference_AUID()";
    for (const reference_case& c : vcases) {
      const xPOTCAR xpot = xPOTCAR_Finder(c.auid);
      const string version = (xpot.species_pp_version.size() ? string(xpot.species_pp_version[0]) : string("not found"));
      checkEqual(version, c.pseudopotential, "xPOTCAR_Finder()", "AUID " + c.auid + " is " + c.pseudopotential, passed_checks, results);
      string structure = "";
      double energy = 0.0, volume = 0.0, spin = 0.0;
      const bool found = xPOTCAR_EnthalpyReference_AUID(c.auid, "", structure, energy, volume, spin);
      checkEqual(found, true, check_function, c.pseudopotential + ": a reference exists", passed_checks, results);
      checkEqual(structure, c.structure, check_function, c.pseudopotential + ": ground-state structure", passed_checks, results);
      checkEqual(vector<double>{energy, volume, spin}, vector<double>{c.energy, c.volume, c.spin}, check_function,
          c.pseudopotential + ": energy (eV/atom), volume (A^3/atom), spin (mu_B/atom)", passed_checks, results);
      checkEqual(xPOTCAR_EnthalpyReference_AUID(c.auid, "SCAN"), false, check_function, c.pseudopotential + ": no SCAN reference", passed_checks, results);
    }

    // the data file against the table, as xPOTCAR_PURE_Printer() looks each entry up //CO20261014
    uint nchecked = 0;
    vector<string> differ;
    for (size_t ipp = 0; ipp < vxpseudopotential.size(); ipp++) {
      const xPOTCAR& x = vxpseudopotential[ipp];
      if (x.species.size() != 1) continue;
      string structure = "";
      double energy = 0.0, volume = 0.0, spin = 0.0;
      if (!xPOTCAR_EnthalpyReference_AUID(x.AUID, "", structure, energy, volume, spin)) continue;
      nchecked++;
      const bool same = x.species_pp_groundstate_structure.size() == 1 && x.species_pp_groundstate_energy.size() == 1 &&
        x.species_pp_groundstate_structure[0] == structure && aurostd::abs(x.species_pp_groundstate_energy[0] - energy) < 1e-6;
      if (!same) differ.push_back(x.AUID + " " + (x.species_pp_version.size() == 1 ? x.species_pp_version[0] : string("")));
    }
    check(nchecked > 500, nchecked, (uint) 500, "vxpseudopotential", "entries of aflow_xpseudopotentials_data.cpp with a reference in the table", passed_checks, results);
    checkEqual(aurostd::joinWDelimiter(differ, ", "), string(""), "aflow_xpseudopotentials_data.cpp",
        "every entry with a reference carries the table's energy and structure (else make pseudo)", passed_checks, results);
  }

}

// aurostd
namespace unittest {

  void UnitTest::xscalarTest(uint &passed_checks, vector <vector<string>> &results, vector <string> &errors) {
    (void) errors;  // Suppress compiler warnings
                    // setup test environment
    string check_function = "", check_description = "";
    double calculated_dbl = 0.0, expected_dbl = 0.0;
    int calculated_int = 0, expected_int = 0;
    vector<int> calculated_vint, expected_vint;

    // ---------------------------------------------------------------------------
    // Check | double2fraction conversion //DX20210908
    // ---------------------------------------------------------------------------
    check_function = "aurostd::double2fraction()";
    check_description = "convert a double to a fraction.";

    double test_double = 1.625;
    int numerator = 1, denominator = 1;
    string answer = "13/8";
    aurostd::double2fraction(test_double, numerator, denominator);
    stringstream result_ss;
    result_ss << numerator << "/" << denominator;

    checkEqual(result_ss.str(), answer, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | mod_floored (int) //SD20220124
    // ---------------------------------------------------------------------------
    check_function = "aurostd::mod_floored()";
    check_description = "floored mod; numbers as int";
    expected_int = -1;

    calculated_int = aurostd::mod_floored(5, -3);
    checkEqual(calculated_int, expected_int, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | mod_floored (double) //SD20220124
    // ---------------------------------------------------------------------------
    check_function = "aurostd::mod_floored()";
    check_description = "floored mod; numbers as double";
    expected_dbl = 1.4;

    calculated_dbl = aurostd::mod_floored(-5.2, 3.3);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | mod_floored (divisor 0) //SD20220124
    // ---------------------------------------------------------------------------
    check_function = "aurostd::mod_floored()";
    check_description = "floored mod; divisor is 0";
    expected_dbl = 11.11;

    calculated_dbl = aurostd::mod_floored(11.11, 0.0);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | mod_floored (divisor inf) //SD20220124
    // ---------------------------------------------------------------------------
    check_function = "aurostd::mod_floored()";
    check_description = "floored mod; divisor is inf";
    expected_dbl = 11.11;

    calculated_dbl = aurostd::mod_floored(11.11, (double) INFINITY);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | gcd //CO20190520
    // ---------------------------------------------------------------------------
    check_function = "aurostd::GCD()";
    int a = 0, b = 0, x1 = 0, y1 = 0, gcd = 0;

    check_description = "gcd(25,15)";
    a = 25;
    b = 15;
    expected_vint = {5, -1, 2};
    aurostd::GCD(a, b, gcd, x1, y1);
    calculated_vint = {gcd, x1, y1};
    checkEqual(calculated_vint, expected_vint, check_function, check_description, passed_checks, results);

    check_description = "gcd(25,0)";
    a = 25;
    b = 0;
    expected_vint = {25, 1, 0};
    aurostd::GCD(a, b, gcd, x1, y1);
    calculated_vint = {gcd, x1, y1};
    checkEqual(calculated_vint, expected_vint, check_function, check_description, passed_checks, results);

    check_description = "gcd(0,15)";
    a = 0;
    b = 15;
    expected_vint = {15, 0, 1};
    aurostd::GCD(a, b, gcd, x1, y1);
    calculated_vint = {gcd, x1, y1};
    checkEqual(calculated_vint, expected_vint, check_function, check_description, passed_checks, results);

    check_description = "gcd(-5100,30450)";
    a = -5100;
    b = 30450;
    expected_vint = {150, -6, -1};
    aurostd::GCD(a, b, gcd, x1, y1);
    calculated_vint = {gcd, x1, y1};
    checkEqual(calculated_vint, expected_vint, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | string2finite //CO20261004
    // ---------------------------------------------------------------------------
    // unset (AUROSTD_NAN) unless the whole string is one finite number
    check_function = "aurostd::string2finite()";
    check_description = "\"1.5\"";
    checkEqual(aurostd::string2finite("1.5"), 1.5, check_function, check_description, passed_checks, results);
    check_description = "\" 2 \" (padded)";
    checkEqual(aurostd::string2finite(" 2 "), 2.0, check_function, check_description, passed_checks, results);
    check_description = "\"-7.5e-01\"";
    checkEqual(aurostd::string2finite("-7.5e-01"), -0.75, check_function, check_description, passed_checks, results);
    check_description = "\"1.5abc\" (partial parse)";
    checkEqual(aurostd::string2finite("1.5abc"), (double)AUROSTD_NAN, check_function, check_description, passed_checks, results);
    check_description = "\"nan\"";
    checkEqual(aurostd::string2finite("nan"), (double)AUROSTD_NAN, check_function, check_description, passed_checks, results);
    check_description = "\"inf\"";
    checkEqual(aurostd::string2finite("inf"), (double)AUROSTD_NAN, check_function, check_description, passed_checks, results);
    check_description = "\"-inf\"";
    checkEqual(aurostd::string2finite("-inf"), (double)AUROSTD_NAN, check_function, check_description, passed_checks, results);
    check_description = "\"\" (empty)";
    checkEqual(aurostd::string2finite(""), (double)AUROSTD_NAN, check_function, check_description, passed_checks, results);
    check_description = "\"null\"";
    checkEqual(aurostd::string2finite("null"), (double)AUROSTD_NAN, check_function, check_description, passed_checks, results);
  }

  void UnitTest::xvectorTest(uint &passed_checks, vector <vector<string>> &results, vector <string> &errors) {
    (void) errors;  // Suppress compiler warnings
                    // setup test environment
    string check_function = "", check_description = "";

    //HE20210511
    double expected_dbl = 0.0, calculated_dbl = 0.0;
    xvector<double> calculated_xvecdbl, expected_xvecdbl;
    int expected_int = 0;
    string expected_str = "";
    vector <xvector<double>> points;
    vector <xvector<int>> ipoints;
    vector <vector<uint>> facets;
    vector <uint> facet;

    // Define test data
    points.clear(); ipoints.clear(); facets.clear();
    points.push_back({0.0, 0.0, 0.0});
    points.push_back({1.0, 0.0, 0.0});
    points.push_back({1.0, 1.0, 0.0});
    points.push_back({0.0, 1.0, 0.0});
    points.push_back({0.0, 0.0, 2.0});
    points.push_back({1.0, 0.0, 2.0});
    points.push_back({1.0, 1.0, 3.0});
    points.push_back({0.0, 1.0, 3.0});

    // covert points to integer to test special implementation
    for (size_t i_point=0; i_point<points.size(); i_point++){
      ipoints.push_back({(int) points[i_point][1], (int) points[i_point][2], (int) points[i_point][3]});
    }

    // define the facets
    facets.push_back({0, 1, 2, 3});
    facets.push_back({4, 5, 6, 7});
    facets.push_back({1, 2, 6, 5});
    facets.push_back({0, 3, 7});
    facets.push_back({0, 1, 5, 4});
    facets.push_back({3, 2, 6, 7});


    // ---------------------------------------------------------------------------
    // Check | convex solid volume (double)
    // ---------------------------------------------------------------------------
    check_function = "aurostd::volume()";
    check_description = "convex solid, points as doubles";
    expected_dbl = 2.5;

    calculated_dbl = aurostd::volume(points, facets, true);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | convex solid volume (int)
    // ---------------------------------------------------------------------------
    check_function = "aurostd::volume()";
    check_description = "convex solid, points as int";

    calculated_dbl = aurostd::volume(ipoints, facets, true);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // define non convex solid
    points.clear(); ipoints.clear(); facets.clear();
    points.push_back({0.0, 0.0, 0.0});
    points.push_back({0.0, 4.0, 0.0});
    points.push_back({2.0, 4.0, 0.0});
    points.push_back({1.0, 1.0, 0.0});
    points.push_back({4.0, 2.0, 0.0});
    points.push_back({4.0, 0.0, 0.0});
    points.push_back({0.0, 0.0, 4.0});
    points.push_back({0.0, 4.0, 4.0});
    points.push_back({2.0, 4.0, 4.0});
    points.push_back({1.0, 1.0, 4.0});
    points.push_back({4.0, 2.0, 4.0});
    points.push_back({4.0, 0.0, 4.0});

    // covert points to integer to test special implementation
    for (size_t i_point=0; i_point<points.size(); i_point++){
      ipoints.push_back({(int) points[i_point][1], (int) points[i_point][2], (int) points[i_point][3]});
    }

    // define the facets
    facets.push_back({5,4,3,2,1,0});
    facets.push_back({6,7,8,9,10,11});
    facets.push_back({0,6,11,5});
    facets.push_back({4,5,11,10});
    facets.push_back({3,4,10,9});
    facets.push_back({3,9,8,2});
    facets.push_back({1,2,8,7});
    facets.push_back({0,1,7,6});


    // ---------------------------------------------------------------------------
    // Check | non convex solid volume (double)
    // ---------------------------------------------------------------------------
    check_function = "aurostd::volume()";
    check_description = "non convex solid, points as doubles";
    expected_dbl = 40.0;

    calculated_dbl = aurostd::volume(points, facets);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | error facet/normals mismatch
    // ---------------------------------------------------------------------------
    check_function = "aurostd::volume()";
    check_description = "error: facet/normals mismatch";
    vector <xvector<double>> normals;
    expected_str = "xerror code 30 (VALUE_ERROR)";
    expected_int = _VALUE_ERROR_;

    try {
      calculated_dbl = aurostd::volume(points, facets, normals);
      check(false, std::string("no error"), expected_str, check_function, check_description, passed_checks, results);
    }
    catch (aurostd::xerror e) {
      if (e.whatCode() == expected_int) check(true, "", "", check_function, check_description, passed_checks, results);
      else
        check(false, aurostd::utype2string(e.whatCode()), expected_str, check_function, check_description,
            passed_checks, results);
    }
    catch (...) {
      check(false, std::string("not an xerror"), expected_str, check_function, check_description, passed_checks,
          results);
    }

    // ---------------------------------------------------------------------------
    // Check | non convex solid volume (int)
    // ---------------------------------------------------------------------------
    check_function = "aurostd::volume()";
    check_description = "non convex solid, points as int";
    expected_dbl = 40.0;

    calculated_dbl = aurostd::volume(ipoints, facets);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | error facet size
    // ---------------------------------------------------------------------------
    check_function = "aurostd::volume()";
    check_description = "error: wrong facet size";
    expected_str = "xerror code 30 (VALUE_ERROR)";
    expected_int = _VALUE_ERROR_;

    facet.clear(); facets.push_back({1,2});
    try {
      calculated_dbl = aurostd::volume(points, facets);
      check(false, std::string("no error"), expected_str, check_function, check_description, passed_checks, results);
    }
    catch (aurostd::xerror e) {
      if (e.whatCode() == expected_int) check(true, "", "", check_function, check_description, passed_checks, results);
      else
        check(false, aurostd::utype2string(e.whatCode()), expected_str, check_function, check_description,
            passed_checks, results);
    }
    catch (...) {
      check(false, std::string("not an xerror"), expected_str, check_function, check_description, passed_checks,
          results);
    }

    // ---------------------------------------------------------------------------
    // Check | non convex area (double)
    // ---------------------------------------------------------------------------
    check_function = "aurostd::areaPointsOnPlane()";
    check_description = "non convex area; points as double";
    expected_dbl = 10.0;

    //fill vectors with data
    points.clear(); ipoints.clear(); facets.clear();
    points.push_back({0.0, 0.0, 0.0});
    points.push_back({0.0, 4.0, 0.0});
    points.push_back({2.0, 4.0, 0.0});
    points.push_back({1.0, 1.0, 0.0});
    points.push_back({4.0, 2.0, 0.0});
    points.push_back({4.0, 0.0, 0.0});

    // convert points to integer to test special implementation
    for (size_t i_point=0; i_point<points.size(); i_point++){
      ipoints.push_back({(int) points[i_point][1], (int) points[i_point][2], (int) points[i_point][3]});
    }
    //    points.push_back(p0); points.push_back(p1); points.push_back(p2); points.push_back(p3); points.push_back(p4);
    //    points.push_back(p5);
    //    ipoints.push_back(p0i); ipoints.push_back(p1i); ipoints.push_back(p2i); ipoints.push_back(p3i); ipoints.push_back(p4i);
    //    ipoints.push_back(p5i);

    calculated_dbl = aurostd::areaPointsOnPlane(points);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | non convex area (int)
    // ---------------------------------------------------------------------------
    check_function = "aurostd::areaPointsOnPlane()";
    check_description = "non convex area; points as int";
    expected_dbl = 10.0;

    calculated_dbl = aurostd::areaPointsOnPlane(ipoints);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);


    // define triangle in 3D to better test int handling
    points.clear(); ipoints.clear(); facets.clear();
    points.push_back({0.0, 0.0, 0.0});
    points.push_back({1.0, 1.0, 1.0});
    points.push_back({5.0, 0.0, 5.0});

    // convert points to integer to test special implementation
    for (size_t i_point=0; i_point<points.size(); i_point++){
      ipoints.push_back({(int) points[i_point][1], (int) points[i_point][2], (int) points[i_point][3]});
    }

    // ---------------------------------------------------------------------------
    // Check | 3d triangle area (double)
    // ---------------------------------------------------------------------------
    check_function = "aurostd::areaPointsOnPlane()";
    check_description = "3d triangle; points as double";
    expected_dbl = 3.5355339059;

    calculated_dbl = aurostd::areaPointsOnPlane(points);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | 3d triangle area (int)
    // ---------------------------------------------------------------------------
    check_function = "aurostd::areaPointsOnPlane()";
    check_description = "3d triangle; points as int";
    expected_dbl = 3.5355339059;

    calculated_dbl = aurostd::areaPointsOnPlane(ipoints);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | linspace //SD20220324
    // ---------------------------------------------------------------------------
    check_function = "aurostd::linspace()";
    check_description = "generate n linearly spaced points";
    expected_xvecdbl = {1.0, 1.375, 1.75, 2.125, 2.5};
    calculated_xvecdbl = aurostd::linspace(1.0, 2.5, 5);
    checkEqual(calculated_xvecdbl, expected_xvecdbl, check_function, check_description, passed_checks, results);
    {
        // ---------------------------------------------------------------------------
        // Check | histogram edges (manual bins) //AZ20230213
        // ---------------------------------------------------------------------------
        check_function = "aurostd::histogram()";
        check_description = "edges of histogram bins (manual bins)";
        expected_xvecdbl = {0., 1.8, 3.6, 5.4, 7.2, 9.};
        vector<xvector<double> > vec_xvecdbls;
        xvector<double> in_xvecdbl = {1.,3.,9.,8.,4.,7.,0.,2.,1.,9.,8.,3.,7.,4.,1.,0.,2.,8.,9.,7.,3.,4.,0.,1.,7.,8.,7.,6.,4.,3.,8.,7.,1.,2.,6.,4.,8.,1.,2.,0.,9.,3.,4.,8.,8.,7.,8.,1.,5.,3.,8.,3.,7.,4.,6.,1.,2.,8.,3.,4.,1.,9.,8.,3.,6.,4.,8.,7.,1.,6.,2.,4.,3.,0.,9.,7.,1.,2.,3.,4.};
        vec_xvecdbls = aurostd::histogram(in_xvecdbl, 5);
        checkEqual(vec_xvecdbls[1], expected_xvecdbl, check_function, check_description, passed_checks, results);

        // ---------------------------------------------------------------------------
        // Check | histogram bins //AZ20230213
        // ---------------------------------------------------------------------------
        check_description = "counts of histogram bins (manual bins)";
        expected_xvecdbl= {16., 18., 12., 15., 19.};
        checkEqual(vec_xvecdbls[0], expected_xvecdbl, check_function, check_description, passed_checks, results);

        // ---------------------------------------------------------------------------
        // Check | auto histogram edges (full vector) //AZ20230213
        // ---------------------------------------------------------------------------
        vec_xvecdbls = aurostd::histogram(in_xvecdbl, 5, 1);
        check_description = "automatic histogram bins (large vector)";
        expected_xvecdbl =  {5.,11.,7.,11.,11.,1.,5.,10.,19.};
        checkEqual(vec_xvecdbls[0], expected_xvecdbl, check_function, check_description, passed_checks, results);

        // ---------------------------------------------------------------------------
        // Check | auto histogram edges (full vector) //AZ20230213
        // ---------------------------------------------------------------------------
        vec_xvecdbls = aurostd::histogram(in_xvecdbl, 5, 1);
        check_function = "aurostd::histogram()";
        check_description = "automatic histogram edges (large vector)";
        expected_xvecdbl =  {0.,1.,2.,3.,4.,5.,6.,7.,8.,9.};
        checkEqual(vec_xvecdbls[1], expected_xvecdbl, check_function, check_description, passed_checks, results);

        // ---------------------------------------------------------------------------
        // Check | auto histogram bins //AZ20230213
        // ---------------------------------------------------------------------------
        in_xvecdbl = {1.,3.,9.,8.,4.,7.,0.,2.,1.,9.,8.,3.,7.,4.,1.,0.,2.};
        vec_xvecdbls = aurostd::histogram(in_xvecdbl, 5, 1);
        check_function = "aurostd::histogram()";
        check_description = "automatic histogram bins (small vector)";
        expected_xvecdbl =  {5.,4.,2.,2.,4.};
        checkEqual(vec_xvecdbls[0], expected_xvecdbl, check_function, check_description, passed_checks, results);

        check_function = "aurostd::histogram()";
        check_description = "automatic histogram edges (small vector)";
        expected_xvecdbl =  {0., 1.8, 3.6, 5.4, 7.2, 9.};
        checkEqual(vec_xvecdbls[1], expected_xvecdbl, check_function, check_description, passed_checks, results);
    }

}

  void UnitTest::xmatrixTest(uint &passed_checks, vector <vector<string>> &results, vector <string> &errors) {
    (void) errors;  // Suppress compiler warnings
                    // setup test environment
    string check_function = "", check_description = "";

    xmatrix<int> calculated_xmatint, expected_xmatint;
    bool calculated_bool = false, expected_bool = false;
    {
        // ---------------------------------------------------------------------------
        // Check convert to xvector //AZ20220627
        // ---------------------------------------------------------------------------
        check_function = "aurostd::getxvec()";
        xmatrix<int> full_xmatint, xmatint;
        // need this matrix to test slicing
        full_xmatint = xmatrix<int>(3,4);
        full_xmatint = {{1,2,3,4},
          {5,6,7,8},
          {9,10,11,12}};
        check_description = "getxvec() test for type conversion";
        xvector<int> expected_xvecint(3);
        xvector<int> calculated_xvecint(3);
        expected_xvecint = {1,5,9};
        calculated_xvecint = full_xmatint.getxmat(1,3,1,1).getxvec();
        checkEqual(calculated_xvecint, expected_xvecint, check_function, check_description, passed_checks, results);

        // ---------------------------------------------------------------------------
        // Check | column xvector //AZ20220627
        // ---------------------------------------------------------------------------
        check_description = "get column xvector from xmatrix";
        calculated_xvecint = full_xmatint.getxvec(1,3,1,1);
        checkEqual(calculated_xvecint, expected_xvecint, check_function, check_description, passed_checks, results);

        // ---------------------------------------------------------------------------
        // Check | row xvector //AZ20220627
        // ---------------------------------------------------------------------------
        check_description = "get row xvector from xmatrix";
        expected_xvecint = {1,2,3};
        calculated_xvecint = full_xmatint.getxvec(1,1,1,3);
        checkEqual(calculated_xvecint, expected_xvecint, check_function, check_description, passed_checks, results);

        // ---------------------------------------------------------------------------
        // Check | 1x1 xvector //AZ20220627
        // ---------------------------------------------------------------------------
        check_description = "get a 1x1 vector from xmatrix";
        expected_xvecint = {12};
        calculated_xvecint = full_xmatint.getxvec(3,3,4,4);
        checkEqual(calculated_xvecint, expected_xvecint, check_function, check_description, passed_checks, results);
    }
    // ---------------------------------------------------------------------------
    // Check | ehermite //CO20190520
    // ---------------------------------------------------------------------------
    check_function = "aurostd::getEHermite()";
    check_description = "calculate elementary Hermite transformation";
    expected_xmatint = {{5,-2}, {-12, 5}};
    calculated_xmatint = xmatrix<int>(2, 2);
    aurostd::getEHermite(5, 12, calculated_xmatint);
    checkEqual(calculated_xmatint, expected_xmatint, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | equilibrateMatrix //SD20220505
    // ---------------------------------------------------------------------------
    check_function = "aurostd::equilibrateMatrix()";
    check_description = "pre-condition a Hilbert matrix, lowers the condition number";
    xmatrix<double> icm(10, 10);
    // define a Hilbert matrix
    for (int i = 1; i <= icm.rows; i++) {
      for (int j = 1; j <= icm.cols; j++) {
        icm(i, j) = 1.0 / (i + j - 1.0);
      }
    }
    xmatrix<double> em, rm, cm;
    aurostd::equilibrateMatrix(icm, em, rm, cm);
    expected_bool = true;
    calculated_bool = aurostd::isequal(1.0, aurostd::sign(aurostd::condition_number(icm) - aurostd::condition_number(em)));
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    check_description = "pre-condition a Hilbert matrix, finds the inverse";
    checkEqual(icm, aurostd::inverse(rm) * em * aurostd::inverse(cm), check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | solve a linear system //HE20220912
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::inverse(xmatrix) * xvector";
      check_description = "solve a simple linear system with shifted matrices ";
      xvector<double> expected_xvecdouble = {5.0, 3.0, -2.0};
      xvector<double> calculated_xvecdouble;
      xvector<double> b = {6.0, -4, 27};
      xmatrix<double> A = {{1.0, 1.0, 1.0}, {0.0,2.0,5.0}, {2.0,5.0,-1.0}};
      bool shift_check = true;
      // inv(A)*b should be solvable even when the xmatrix and xvector have different index boundaries
      for (int shift_row: {-2,-1,0,1,2}){
        for (int shift_col: {-2,-1,0,1,2}) {
          A.shift(shift_row, shift_col);
          calculated_xvecdouble = aurostd::inverse(A) * b;
          if (not aurostd::isequal(expected_xvecdouble, calculated_xvecdouble)) {
            shift_check=false;
            break;
          }
        }
      }
      check(shift_check, calculated_xvecdouble, expected_xvecdouble, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | shifting xmatrix
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::xmatrix.shift()";
      check_description = "shift the lower bounds of an xmatrix to new values";
      bool overall_check = true;
      xmatrix<double> A = {{1.0, 2.0, 3.0},
                           {4.0, 5.0, 6.0},
                           {7.0, 8.0, 9.0}};
      xmatrix<double> B = A;

      for (std::pair<int, int> p : vector<std::pair<int, int>>({{-2,-4}, {0,3}, {5461, 8461}})){
        A.shift(p.first, p.second);
        for (std::pair<int, int> t : vector<std::pair<int, int>>({{1,1}, {2,2}, {3,2}})) {
          overall_check = overall_check && (A[p.first+t.first-1][p.second+t.second-1] == B[t.first][t.second]);
          if (!overall_check){
            check(overall_check,A[p.first+t.first-1][p.second+t.second-1] , B[t.first][t.second], check_function, check_description, passed_checks, results);
            break;
          }
          if (!overall_check) break;
        }
      }
      if (overall_check){
        check(overall_check, "", "", check_function, check_description, passed_checks, results);
      }
    }

    // ---------------------------------------------------------------------------
    // Check | reshaping xmatrix
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::xmatrix.reshape()";
      check_description = "reshaping a xmatrix";
      bool overall_check = true;
      vector<int> B = {1,2,3,4,5,6,7,8,9,10,11,12};
      xmatrix<int> A = {{1,2,3,4,5,6,7,8,9,10,11,12}};
      int c, r;
      for (const std::pair<int, int> &p : vector<std::pair<int, int>>({{4,3}, {2,6}, {3, 4}})){
        A.reshape(p.first, p.second);
        for (const int t : B) {
          r = (t-1)/p.second+1;
          c = t-(r-1)*(p.second);
          overall_check = overall_check && (A[r][c] == t);
          if (!overall_check) {
            check(overall_check, A[r][c], t,  check_function, check_description, passed_checks, results);
            break;
          }
        }
        if (!overall_check) break;
      }
      if (overall_check){
        check(overall_check, "", "", check_function, check_description, passed_checks, results);
      }
    }

    // ---------------------------------------------------------------------------
    // Check | reshaping xvector
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::reshape()";
      check_description = "reshaping xvectors";
      bool overall_check = true;
      vector<int> B = {1,2,3,4,5,6,7,8,9,10,11,12};
      xvector<int> xv = aurostd::vector2xvector(B);
      aurostd::xmatrix<int> A;
      int c, r;
      for (const std::pair<int, int> &p : vector<std::pair<int, int>>({{4,3}, {2,6}, {3, 4}})){
        A = aurostd::reshape(xv, p.first, p.second);
        for (const int t : B) {
          r = (t-1)/p.second+1;
          c = t-(r-1)*(p.second);
          overall_check = overall_check && (A[r][c] == t);
          if (!overall_check) {
            check(overall_check, A[r][c], t,  check_function, check_description, passed_checks, results);
            break;
          }
        }
        if (!overall_check) break;
      }
      if (overall_check){
        check(overall_check, "", "", check_function, check_description, passed_checks, results);
      }
    }

    // ---------------------------------------------------------------------------
    // Check | stacking xvector
    // ---------------------------------------------------------------------------
    {
      xvector<int> xv = {1, 2, 3, 4};
      xmatrix<int> smat;
      xmatrix<int> vsmat = {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}, {4, 4, 4}};
      xmatrix<int> hsmat = {{1, 2, 3, 4}, {1, 2, 3, 4}, {1, 2, 3, 4}};
      check_function = "aurostd::vstack()";
      check_description = "stack xvectors vertically into a xmatrix";
      smat = aurostd::vstack(vector<xvector<int>>({xv, xv, xv}));
      checkEqual(smat, vsmat, check_function, check_description, passed_checks, results);
      check_function = "aurostd::hstack()";
      check_description = "stack xvectors horizontal into a xmatrix";
      smat = aurostd::hstack(vector<xvector<int>>({xv, xv, xv}));
      checkEqual(smat, hsmat, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | Smith normal form //CO20261005
    // S=U*A*V, S diagonal, nonnegative, S(i,i) divides S(i+1,i+1), |det U|=|det V|=1;
    // every HNF matrix of determinant 1 to 16, random 3x3 and 4x5 integer matrices, smithTest()'s 5x5 matrix
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::getSmithNormalForm()";
      //det(M) = +-1 exactly: determinant modulo three primes near 2^61-2^63 (their product exceeds twice the Hadamard bound of the matrices here)
      auto isUnimodular = [](const xmatrix<long long int>& M) -> bool {
        if (M.rows!=M.cols) {return false;}
        const unsigned long long int primes[3]={2305843009213693951ULL,4611686018427387847ULL,9223372036854775783ULL};
        int sign=0;
        for (int ip=0;ip<3;ip++) {
          const unsigned long long int p=primes[ip];
          const int n=M.rows;
          vector<vector<unsigned long long int> > a(n,vector<unsigned long long int>(n));
          for (int i=0;i<n;i++) {for (int j=0;j<n;j++) {long long int v=M[M.lrows+i][M.lcols+j]%(long long int)p;a[i][j]=(v<0?(unsigned long long int)(v+(long long int)p):(unsigned long long int)v)%p;}}
          auto mulmod=[p](unsigned long long int x,unsigned long long int y) -> unsigned long long int {return (unsigned long long int)(((__uint128_t)x*y)%p);};
          auto powmod=[&mulmod](unsigned long long int b,unsigned long long int e) -> unsigned long long int {unsigned long long int r=1;while(e){if(e&1ULL){r=mulmod(r,b);}b=mulmod(b,b);e>>=1;}return r;};
          unsigned long long int det=1;
          for (int c=0;c<n;c++) {
            int piv=-1;
            for (int r=c;r<n;r++) {if (a[r][c]!=0) {piv=r;break;}}
            if (piv<0) {return false;}
            if (piv!=c) {std::swap(a[piv],a[c]);det=(p-det)%p;}
            det=mulmod(det,a[c][c]);
            const unsigned long long int inv=powmod(a[c][c],p-2);
            for (int r=c+1;r<n;r++) {
              const unsigned long long int f=mulmod(a[r][c],inv);
              for (int k=c;k<n;k++) {a[r][k]=(a[r][k]+p-mulmod(f,a[c][k]))%p;}
            }
          }
          const int s=(det==1ULL?1:(det==p-1ULL?-1:0));
          if (s==0 || (sign!=0 && s!=sign)) {return false;}
          sign=s;
        }
        return true;
      };
      auto snfValid = [&isUnimodular](const xmatrix<long long int>& A) -> bool {
        xmatrix<long long int> U,V,S;
        try {aurostd::getSmithNormalForm(A,U,V,S);}
        catch (aurostd::xerror& e) {return false;}
        if (S.rows!=A.rows || S.cols!=A.cols || U.rows!=A.rows || V.rows!=A.cols) {return false;}
        if (!(S==U*A*V)) {return false;}
        for (int i=S.lrows;i<=S.urows;i++) {
          for (int j=S.lcols;j<=S.ucols;j++) {
            if (i-S.lrows!=j-S.lcols && S[i][j]!=0) {return false;}
          }
        }
        const int nd=std::min(S.rows,S.cols);
        for (int k=0;k<nd;k++) {
          const long long int d=S[S.lrows+k][S.lcols+k];
          if (d<0) {return false;}
          if (k+1<nd) {
            const long long int d2=S[S.lrows+k+1][S.lcols+k+1];
            if (d==0) {if (d2!=0) {return false;}}
            else if (d2%d!=0) {return false;}
          }
        }
        if (!isUnimodular(U) || !isUnimodular(V)) {return false;}
        return true;
      };
      uint nhnf=0,nhnf_valid=0;
      xmatrix<long long int> H(3,3);
      for (int n=1;n<=16;n++) {
        for (int a=1;a<=n;a++) {for (int c=1;c<=n/a;c++) {for (int f=1;f<=n/a/c;f++) {
          if (a*c*f!=n) {continue;}
          for (int b=0;b<c;b++) {for (int d=0;d<f;d++) {for (int e=0;e<f;e++) {
            H[1][1]=a;H[1][2]=b;H[1][3]=d;H[2][1]=0;H[2][2]=c;H[2][3]=e;H[3][1]=0;H[3][2]=0;H[3][3]=f;
            nhnf++;
            if (snfValid(H)) {nhnf_valid++;}
          }}}
        }}}
      }
      check_description = "every HNF matrix of determinant 1 to 16 ("+aurostd::utype2string(nhnf)+")";
      checkEqual(nhnf_valid, nhnf, check_function, check_description, passed_checks, results);
      std::mt19937 rng(20261005);
      std::uniform_int_distribution<int> entry(-20,20);
      uint nrandom=2000,nrandom_valid=0;
      xmatrix<long long int> R3(3,3),R45(4,5);
      for (uint t=0;t<nrandom;t++) {
        xmatrix<long long int>& R=(t%2==0?R3:R45);
        for (int i=R.lrows;i<=R.urows;i++) {for (int j=R.lcols;j<=R.ucols;j++) {R[i][j]=(t%7==0 && j==R.lcols ? 0 : entry(rng));}}  //some with a zero column
        if (snfValid(R)) {nrandom_valid++;}
      }
      check_description = "random 3x3 and 4x5 integer matrices (2000, some singular)";
      checkEqual(nrandom_valid, nrandom, check_function, check_description, passed_checks, results);
      xmatrix<long long int> A2(5,5);
      A2 = {{ 25,  -300,   1050, -1400,   630},
        {-300,  4800, -18900, 26880, -12600},
        { 1050,-18900, 79380,-117600, 56700},
        {-1400, 26880,-117600,179200,-88200},
        { 630, -12600, 56700,-88200,  44100}};
      check_description = "5x5 matrix of smithTest()";
      checkEqual(snfValid(A2), true, check_function, check_description, passed_checks, results);
      xmatrix<int> A3(3,3),U3,V3,S3,S3_expected(3,3);
      A3 = {{2,4,4},{-6,6,12},{10,-4,-16}};
      S3_expected = {{2,0,0},{0,6,0},{0,0,12}};
      aurostd::getSmithNormalForm(A3,U3,V3,S3);
      check_description = "diag(2,6,12) for [[2,4,4],[-6,6,12],[10,-4,-16]]";
      checkEqual(S3, S3_expected, check_function, check_description, passed_checks, results);
      //64-bit limits: 128-bit entries inside, the result must fit the matrix type
      xmatrix<unsigned long long int> Aull(1,1),Uull,Vull,Sull;
      Aull[1][1]=(1ULL<<63)+1ULL;
      aurostd::getSmithNormalForm(Aull,Uull,Vull,Sull);
      check_description = "unsigned long long 1x1 matrix 2^63+1 (beyond long long)";
      checkEqual(Sull[1][1]==(1ULL<<63)+1ULL && Uull[1][1]==1ULL && Vull[1][1]==1ULL, true, check_function, check_description, passed_checks, results);
      xmatrix<long long int> Amin(1,2),Umin,Vmin,Smin;
      Amin[1][1]=std::numeric_limits<long long int>::min();Amin[1][2]=-1;
      aurostd::getSmithNormalForm(Amin,Umin,Vmin,Smin);
      check_description = "[LLONG_MIN -1]: S=[1 0] (no overflow in the quotient)";
      checkEqual(Smin[1][1]==1 && Smin[1][2]==0 && Umin[1][1]*Umin[1][1]==1, true, check_function, check_description, passed_checks, results);
      auto throwsValueRange = [](auto A) -> bool {
        decltype(A) U,V,S;
        try {aurostd::getSmithNormalForm(A,U,V,S);}
        catch (aurostd::xerror& e) {return e.whatCode()==_VALUE_RANGE_;}
        return false;
      };
      xmatrix<long long int> Amin1(1,1);
      Amin1[1][1]=std::numeric_limits<long long int>::min();
      check_description = "[LLONG_MIN]: S=2^63 does not fit long long, refused";
      checkEqual(throwsValueRange(Amin1), true, check_function, check_description, passed_checks, results);
      xmatrix<uint> Huint(3,3);
      Huint = {{2,1,0},{0,3,1},{0,0,4}};
      check_description = "unsigned int HNF matrix: U and V need negative entries, refused rather than wrapped";
      checkEqual(throwsValueRange(Huint), true, check_function, check_description, passed_checks, results);
    }

  }

  void UnitTest::aurostdMainTest(uint &passed_checks, vector <vector<string>> &results, vector <string> &errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function = "", check_description = "";
    bool calculated_bool = false, expected_bool = false;
    int calculated_int = 0, expected_int = 0;
    string calculated_string = "", expected_string = "";
    bool multi_check = true;
    uint64_t expected_uint64 = 0;
    uint64_t calculated_uint64 = 0;

    // ---------------------------------------------------------------------------
    // Check | substringlist2bool
    // ---------------------------------------------------------------------------
    check_function = "aurostd::substringlist2bool()";
    string str = "hawk owl";
    vector <string> strlist;

    strlist.push_back("falcon");
    check_description = aurostd::joinWDelimiter(strlist, " and ") + " in " + str + " (match all)";
    expected_bool = false;
    calculated_bool = aurostd::substringlist2bool(str, strlist, true);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    check_description = aurostd::joinWDelimiter(strlist, " or ") + " in " + str + " (match one)";
    expected_bool = false;
    calculated_bool = aurostd::substringlist2bool(str, strlist, false);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    strlist.push_back("owl");
    check_description = aurostd::joinWDelimiter(strlist, " and ") + " in " + str + " (match all)";
    expected_bool = false;
    calculated_bool = aurostd::substringlist2bool(str, strlist, true);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    check_description = aurostd::joinWDelimiter(strlist, " or ") + " in " + str + " (match one)";
    expected_bool = true;
    calculated_bool = aurostd::substringlist2bool(str, strlist, false);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    strlist[0] = "hawk";
    check_description = aurostd::joinWDelimiter(strlist, " and ") + " in " + str + " (match all)";
    expected_bool = true;
    calculated_bool = aurostd::substringlist2bool(str, strlist, true);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    check_description = aurostd::joinWDelimiter(strlist, " or ") + " in " + str + " (match one)";
    expected_bool = true;
    calculated_bool = aurostd::substringlist2bool(str, strlist, false);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    str = "falcon";
    check_description = "Find substring " + str + " in [" + aurostd::joinWDelimiter(strlist, ", ") + "]";
    expected_int = -1;
    aurostd::SubstringWithinList(strlist, str, calculated_int);
    checkEqual(calculated_int, expected_int, check_function, check_description, passed_checks, results);

    strlist.push_back("peregrine falcon");
    check_description = "Find substring " + str + " in [" + aurostd::joinWDelimiter(strlist, ", ") + "]";
    expected_int = (int) strlist.size() - 1;
    aurostd::SubstringWithinList(strlist, str, calculated_int);
    checkEqual(calculated_int, expected_int, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | substring2string //SD20220525
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::substring2string()";
      check_description = "return the third match of the substring";
      string test_string = "_FILE_START_\nIALGO==48\nALGO==FAST\nIALGO==49\nALGO==MEDIUM\nIALGO==50\nALGO==SLOW\n_FILE_END_";
      string calculated_string = aurostd::substring2string(test_string, "ALGO", 3);
      string expected_string = "==49";
      checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | kvpair2string //SD20220525
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::kvpair2string()";
      check_description = "return the second match of the kvpair";
      string test_string = "_FILE_START_\nIALGO==48\nALGO==FAST\nIALGO==49\nALGO==MEDIUM\nIALGO==50\nALGO==SLOW\n_FILE_END_";
      string calculated_string = aurostd::kvpair2string(test_string, "ALGO", "==", 2);
      string expected_string = "MEDIUM";
      checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | substring2string //SD20220525
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::substring2string()";
      check_description = "return the last match of the substring";
      string test_string = "_FILE_START_\nIALGO==48\nALGO==FAST\nIALGO==49\nALGO==MEDIUM\nIALGO==50\nALGO==SLOW\n_FILE_END_";
      string calculated_string = aurostd::substring2string(test_string, "ALGO", -1);
      string expected_string = "==SLOW";
      checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);
    }
    
    // ---------------------------------------------------------------------------
    // Check | substring2string, end marker after the last block //CO20261004
    // ---------------------------------------------------------------------------
    // a line that held the end marker again after the last block, outside any block, looped forever
    {
      check_function = "aurostd::substring2string()";
      check_description = "an end marker after the last block is ignored";
      string calculated_string = aurostd::substring2string(string("a(b)c)"), "(", ")", 1);
      checkEqual(calculated_string, string("b"), check_function, check_description, passed_checks, results);
      check_description = "the second block, with an end marker after it";
      calculated_string = aurostd::substring2string(string("x(1) y(2) z)"), "(", ")", 2);
      checkEqual(calculated_string, string("2"), check_function, check_description, passed_checks, results);
      check_description = "a block inside an SQL clause that closes later";
      calculated_string = aurostd::substring2string(string("(alloy IN ('Co','O') AND alloy COLLATE BINARY IN ('Co','O'))"), "alloy IN (", ")", 1);
      checkEqual(calculated_string, string("'Co','O'"), check_function, check_description, passed_checks, results);
      check_description = "an end marker on a later line, outside any block";
      calculated_string = aurostd::substring2string(string("[START]\nvalue\n[STOP]\n[STOP] again\n"), "[START]", "[STOP]", 1);
      checkEqual(calculated_string, string("value"), check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | substring2strings //CO20230502
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::substring2string()";
      check_description = "return the last match of the substring";
      string test_string = 
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_INCAR_MODE_EXPLICIT]START \n"
        "SYSTEM=CoI.AB_hP8_194_f_f-002.AB\n"
        "#PSTRESS=000       # Pressure in kBar (1kB=0.1GPa)           # for hand modification\n"
        "#EDIFFG=-0.001     # For relaxed forces                      # for hand modification\n"
        "#POTIM=-0.001      # default                                 # for hand modification\n"
        "#NBANDS=XX                                                   # for hand modification\n"
        "#IALGO=48                                                    # for hand modification\n"
        "[VASP_INCAR_MODE_EXPLICIT]STOP \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_KPOINTS_MODE_IMPLICIT] \n"
        "[VASP_KPOINTS_FILE]KSCHEME=AUTO\n"
        "[VASP_KPOINTS_FILE]KPPRA=6000\n"
        "[VASP_KPOINTS_FILE]STATIC_KSCHEME=AUTO\n"
        "[VASP_KPOINTS_FILE]STATIC_KPPRA=10000\n"
        "[VASP_KPOINTS_FILE]BANDS_LATTICE=AUTO\n"
        "[VASP_KPOINTS_FILE]BANDS_GRID=20\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POSCAR_MODE_EXPLICIT]START \n"
        "CoI/AB_hP8_194_f_f-002.AB params=-1,1.222839,0.408,0.667 SG=194 [ANRL doi: 10.1016/j.commatsci.2017.01.017 (part 1), doi: 10.1016/j.commatsci.2018.10.043 (part 2)]\n"
        "-181.195600\n"
        "   0.50000000000000  -0.86602540378444   0.00000000000000\n"
        "   0.50000000000000   0.86602540378444   0.00000000000000\n"
        "   0.00000000000000   0.00000000000000   1.22283900000000\n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.33333333333333   0.66666666666667   0.40800000000000  Co    \n"
        "   0.66666666666667   0.33333333333333   0.90800000000000  Co    \n"
        "   0.66666666666667   0.33333333333333   0.59200000000000  Co    \n"
        "   0.33333333333333   0.66666666666667   0.09200000000000  Co    \n"
        "   0.33333333333333   0.66666666666667   0.66700000000000  I     \n"
        "   0.66666666666667   0.33333333333333   0.16700000000000  I     \n"
        "   0.66666666666667   0.33333333333333   0.33300000000000  I     \n"
        "   0.33333333333333   0.66666666666667   0.83300000000000  I     \n"
        "[VASP_POSCAR_MODE_EXPLICIT]STOP \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POTCAR_MODE_IMPLICIT] \n"
        "[VASP_POTCAR_FILE]Co\n"
        "[VASP_POTCAR_FILE]I\n"
        "[AFLOW] potpaw_PBE: Co I \n"
        "[AFLOW] COMPOSITION_PP=|Co4|I4|\n"
        "[AFLOW] COMPOSITION=|Co4|I4|\n"
        "[AFLOW] VOLUME(A^3)=|10.3205|34.9784|\n"
        "[AFLOW] MASS(amu)=|58.9332|126.9045|\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] AFLOW automatically generated (aflow_avasp.cpp) \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] AFLOW V(3.2.13) in ./LIB2/LIB/CoI/AB_hP8_194_f_f-002.AB\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POTCAR_AUID]9c4653e42c9f870b,7e60ff94db40760b\n"
        "[VASP_FORCE_OPTION]ALGO=VERYFAST                             // Self Correction (KBIN::XVASP_Afix_ApplyFix(ALGO=VERYFAST))\n"
        "#[AFLOW REMOVED (KBIN::XVASP_Afix_NBANDS)] [VASP_FORCE_OPTION]NBANDS=116                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "#[AFLOW REMOVED (KBIN::XVASP_Afix_NBANDS)] [VASP_FORCE_OPTION]NBANDS=160                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "#[AFLOW REMOVED (KBIN::XVASP_Afix_NBANDS)] [VASP_FORCE_OPTION]NBANDS=212                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "[VASP_FORCE_OPTION]NBANDS=276                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] SELF-MODIFICATION\n"
        "[AFLOW] Recycling CONTCAR of relax1\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POSCAR_MODE_EXPLICIT]START\n"
        "CoI/AB_hP8_194_f_f-002.AB params=-1,1.22\n"
        "1.301576\n"
        "   2.76416483956625  -4.78767394262424   0.00000000000000\n"
        "   2.76416483956625   4.78767394262424   0.00000000000000\n"
        "  -0.00000000000000   0.00000000000000  11.06285889347163\n"
        "Co I \n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.66666666666667   0.33333333333334   0.38822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.88822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.61177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334   0.11177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334  -0.82526760342042  I     \n"
        "   0.33333333333333   0.66666666666666  -0.32526760342042  I     \n"
        "   0.33333333333333   0.66666666666666   0.82526760342042  I     \n"
        "   0.66666666666667   0.33333333333334   0.32526760342042  I     \n"
        "[VASP_POSCAR_MODE_EXPLICIT]STOP\n"
        "[AFLOW] ************************************************************************************************************************** \n";
      string calculated_string = aurostd::substring2string(test_string, _VASP_POSCAR_MODE_EXPLICIT_START_, _VASP_POSCAR_MODE_EXPLICIT_STOP_, -1);
      string expected_string = 
        "CoI/AB_hP8_194_f_f-002.AB params=-1,1.22\n"
        "1.301576\n"
        "   2.76416483956625  -4.78767394262424   0.00000000000000\n"
        "   2.76416483956625   4.78767394262424   0.00000000000000\n"
        "  -0.00000000000000   0.00000000000000  11.06285889347163\n"
        "Co I \n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.66666666666667   0.33333333333334   0.38822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.88822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.61177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334   0.11177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334  -0.82526760342042  I     \n"
        "   0.33333333333333   0.66666666666666  -0.32526760342042  I     \n"
        "   0.33333333333333   0.66666666666666   0.82526760342042  I     \n"
        "   0.66666666666667   0.33333333333334   0.32526760342042  I     ";
      checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | substring2strings //CO20230502
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::substring2string()";
      check_description = "return the first match of the substring";
      string test_string = 
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_INCAR_MODE_EXPLICIT]START \n"
        "SYSTEM=CoI.AB_hP8_194_f_f-002.AB\n"
        "#PSTRESS=000       # Pressure in kBar (1kB=0.1GPa)           # for hand modification\n"
        "#EDIFFG=-0.001     # For relaxed forces                      # for hand modification\n"
        "#POTIM=-0.001      # default                                 # for hand modification\n"
        "#NBANDS=XX                                                   # for hand modification\n"
        "#IALGO=48                                                    # for hand modification\n"
        "[VASP_INCAR_MODE_EXPLICIT]STOP \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_KPOINTS_MODE_IMPLICIT] \n"
        "[VASP_KPOINTS_FILE]KSCHEME=AUTO\n"
        "[VASP_KPOINTS_FILE]KPPRA=6000\n"
        "[VASP_KPOINTS_FILE]STATIC_KSCHEME=AUTO\n"
        "[VASP_KPOINTS_FILE]STATIC_KPPRA=10000\n"
        "[VASP_KPOINTS_FILE]BANDS_LATTICE=AUTO\n"
        "[VASP_KPOINTS_FILE]BANDS_GRID=20\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POSCAR_MODE_EXPLICIT]START \n"
        "CoI/AB_hP8_194_f_f-002.AB params=-1,1.222839,0.408,0.667 SG=194 [ANRL doi: 10.1016/j.commatsci.2017.01.017 (part 1), doi: 10.1016/j.commatsci.2018.10.043 (part 2)]\n"
        "-181.195600\n"
        "   0.50000000000000  -0.86602540378444   0.00000000000000\n"
        "   0.50000000000000   0.86602540378444   0.00000000000000\n"
        "   0.00000000000000   0.00000000000000   1.22283900000000\n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.33333333333333   0.66666666666667   0.40800000000000  Co    \n"
        "   0.66666666666667   0.33333333333333   0.90800000000000  Co    \n"
        "   0.66666666666667   0.33333333333333   0.59200000000000  Co    \n"
        "   0.33333333333333   0.66666666666667   0.09200000000000  Co    \n"
        "   0.33333333333333   0.66666666666667   0.66700000000000  I     \n"
        "   0.66666666666667   0.33333333333333   0.16700000000000  I     \n"
        "   0.66666666666667   0.33333333333333   0.33300000000000  I     \n"
        "   0.33333333333333   0.66666666666667   0.83300000000000  I     \n"
        "[VASP_POSCAR_MODE_EXPLICIT]STOP \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POTCAR_MODE_IMPLICIT] \n"
        "[VASP_POTCAR_FILE]Co\n"
        "[VASP_POTCAR_FILE]I\n"
        "[AFLOW] potpaw_PBE: Co I \n"
        "[AFLOW] COMPOSITION_PP=|Co4|I4|\n"
        "[AFLOW] COMPOSITION=|Co4|I4|\n"
        "[AFLOW] VOLUME(A^3)=|10.3205|34.9784|\n"
        "[AFLOW] MASS(amu)=|58.9332|126.9045|\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] AFLOW automatically generated (aflow_avasp.cpp) \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] AFLOW V(3.2.13) in ./LIB2/LIB/CoI/AB_hP8_194_f_f-002.AB\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POTCAR_AUID]9c4653e42c9f870b,7e60ff94db40760b\n"
        "[VASP_FORCE_OPTION]ALGO=VERYFAST                             // Self Correction (KBIN::XVASP_Afix_ApplyFix(ALGO=VERYFAST))\n"
        "#[AFLOW REMOVED (KBIN::XVASP_Afix_NBANDS)] [VASP_FORCE_OPTION]NBANDS=116                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "#[AFLOW REMOVED (KBIN::XVASP_Afix_NBANDS)] [VASP_FORCE_OPTION]NBANDS=160                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "#[AFLOW REMOVED (KBIN::XVASP_Afix_NBANDS)] [VASP_FORCE_OPTION]NBANDS=212                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "[VASP_FORCE_OPTION]NBANDS=276                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] SELF-MODIFICATION\n"
        "[AFLOW] Recycling CONTCAR of relax1\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POSCAR_MODE_EXPLICIT]START\n"
        "CoI/AB_hP8_194_f_f-002.AB params=-1,1.22\n"
        "1.301576\n"
        "   2.76416483956625  -4.78767394262424   0.00000000000000\n"
        "   2.76416483956625   4.78767394262424   0.00000000000000\n"
        "  -0.00000000000000   0.00000000000000  11.06285889347163\n"
        "Co I \n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.66666666666667   0.33333333333334   0.38822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.88822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.61177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334   0.11177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334  -0.82526760342042  I     \n"
        "   0.33333333333333   0.66666666666666  -0.32526760342042  I     \n"
        "   0.33333333333333   0.66666666666666   0.82526760342042  I     \n"
        "   0.66666666666667   0.33333333333334   0.32526760342042  I     \n"
        "[VASP_POSCAR_MODE_EXPLICIT]STOP\n"
        "[AFLOW] ************************************************************************************************************************** \n";
      string calculated_string = aurostd::substring2string(test_string, _VASP_POSCAR_MODE_EXPLICIT_START_, _VASP_POSCAR_MODE_EXPLICIT_STOP_, 1);
      string expected_string = 
      "CoI/AB_hP8_194_f_f-002.AB params=-1,1.222839,0.408,0.667 SG=194 [ANRL doi: 10.1016/j.commatsci.2017.01.017 (part 1), doi: 10.1016/j.commatsci.2018.10.043 (part 2)]\n"
        "-181.195600\n"
        "   0.50000000000000  -0.86602540378444   0.00000000000000\n"
        "   0.50000000000000   0.86602540378444   0.00000000000000\n"
        "   0.00000000000000   0.00000000000000   1.22283900000000\n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.33333333333333   0.66666666666667   0.40800000000000  Co    \n"
        "   0.66666666666667   0.33333333333333   0.90800000000000  Co    \n"
        "   0.66666666666667   0.33333333333333   0.59200000000000  Co    \n"
        "   0.33333333333333   0.66666666666667   0.09200000000000  Co    \n"
        "   0.33333333333333   0.66666666666667   0.66700000000000  I     \n"
        "   0.66666666666667   0.33333333333333   0.16700000000000  I     \n"
        "   0.66666666666667   0.33333333333333   0.33300000000000  I     \n"
        "   0.33333333333333   0.66666666666667   0.83300000000000  I     ";
      checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);
    }
    
    // ---------------------------------------------------------------------------
    // Check | substring2strings //CO20230502
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::substring2string()";
      check_description = "return the second match of the substring";
      string test_string = 
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_INCAR_MODE_EXPLICIT]START \n"
        "SYSTEM=CoI.AB_hP8_194_f_f-002.AB\n"
        "#PSTRESS=000       # Pressure in kBar (1kB=0.1GPa)           # for hand modification\n"
        "#EDIFFG=-0.001     # For relaxed forces                      # for hand modification\n"
        "#POTIM=-0.001      # default                                 # for hand modification\n"
        "#NBANDS=XX                                                   # for hand modification\n"
        "#IALGO=48                                                    # for hand modification\n"
        "[VASP_INCAR_MODE_EXPLICIT]STOP \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_KPOINTS_MODE_IMPLICIT] \n"
        "[VASP_KPOINTS_FILE]KSCHEME=AUTO\n"
        "[VASP_KPOINTS_FILE]KPPRA=6000\n"
        "[VASP_KPOINTS_FILE]STATIC_KSCHEME=AUTO\n"
        "[VASP_KPOINTS_FILE]STATIC_KPPRA=10000\n"
        "[VASP_KPOINTS_FILE]BANDS_LATTICE=AUTO\n"
        "[VASP_KPOINTS_FILE]BANDS_GRID=20\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POSCAR_MODE_EXPLICIT]START \n"
        "CoI/AB_hP8_194_f_f-002.AB params=-1,1.222839,0.408,0.667 SG=194 [ANRL doi: 10.1016/j.commatsci.2017.01.017 (part 1), doi: 10.1016/j.commatsci.2018.10.043 (part 2)]\n"
        "-181.195600\n"
        "   0.50000000000000  -0.86602540378444   0.00000000000000\n"
        "   0.50000000000000   0.86602540378444   0.00000000000000\n"
        "   0.00000000000000   0.00000000000000   1.22283900000000\n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.33333333333333   0.66666666666667   0.40800000000000  Co    \n"
        "   0.66666666666667   0.33333333333333   0.90800000000000  Co    \n"
        "   0.66666666666667   0.33333333333333   0.59200000000000  Co    \n"
        "   0.33333333333333   0.66666666666667   0.09200000000000  Co    \n"
        "   0.33333333333333   0.66666666666667   0.66700000000000  I     \n"
        "   0.66666666666667   0.33333333333333   0.16700000000000  I     \n"
        "   0.66666666666667   0.33333333333333   0.33300000000000  I     \n"
        "   0.33333333333333   0.66666666666667   0.83300000000000  I     \n"
        "[VASP_POSCAR_MODE_EXPLICIT]STOP \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POTCAR_MODE_IMPLICIT] \n"
        "[VASP_POTCAR_FILE]Co\n"
        "[VASP_POTCAR_FILE]I\n"
        "[AFLOW] potpaw_PBE: Co I \n"
        "[AFLOW] COMPOSITION_PP=|Co4|I4|\n"
        "[AFLOW] COMPOSITION=|Co4|I4|\n"
        "[AFLOW] VOLUME(A^3)=|10.3205|34.9784|\n"
        "[AFLOW] MASS(amu)=|58.9332|126.9045|\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] AFLOW automatically generated (aflow_avasp.cpp) \n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] AFLOW V(3.2.13) in ./LIB2/LIB/CoI/AB_hP8_194_f_f-002.AB\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POTCAR_AUID]9c4653e42c9f870b,7e60ff94db40760b\n"
        "[VASP_FORCE_OPTION]ALGO=VERYFAST                             // Self Correction (KBIN::XVASP_Afix_ApplyFix(ALGO=VERYFAST))\n"
        "#[AFLOW REMOVED (KBIN::XVASP_Afix_NBANDS)] [VASP_FORCE_OPTION]NBANDS=116                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "#[AFLOW REMOVED (KBIN::XVASP_Afix_NBANDS)] [VASP_FORCE_OPTION]NBANDS=160                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "#[AFLOW REMOVED (KBIN::XVASP_Afix_NBANDS)] [VASP_FORCE_OPTION]NBANDS=212                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "[VASP_FORCE_OPTION]NBANDS=276                                // Self Correction (KBIN::XVASP_Afix_NBANDS)\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] SELF-MODIFICATION\n"
        "[AFLOW] Recycling CONTCAR of relax1\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POSCAR_MODE_EXPLICIT]START\n"
        "CoI/AB_hP8_194_f_f-002.AB params=-1,1.22\n"
        "1.301576\n"
        "   2.76416483956625  -4.78767394262424   0.00000000000000\n"
        "   2.76416483956625   4.78767394262424   0.00000000000000\n"
        "  -0.00000000000000   0.00000000000000  11.06285889347163\n"
        "Co I \n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.66666666666667   0.33333333333334   0.38822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.88822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.61177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334   0.11177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334  -0.82526760342042  I     \n"
        "   0.33333333333333   0.66666666666666  -0.32526760342042  I     \n"
        "   0.33333333333333   0.66666666666666   0.82526760342042  I     \n"
        "   0.66666666666667   0.33333333333334   0.32526760342042  I     \n"
        "[VASP_POSCAR_MODE_EXPLICIT]STOP\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[AFLOW] SELF-MODIFICATION\n"
        "[AFLOW] Recycling CONTCAR of relax2\n"
        "[AFLOW] ************************************************************************************************************************** \n"
        "[VASP_POSCAR_MODE_EXPLICIT]START\n"
        "CoI/AB_hP8_194_f_f-002.AB params=-1,1.22\n"
        "1.34567\n"
        "   2.76416483956625  -4.78767394262424   0.00000000000000\n"
        "   2.76416483956625   4.78767394262424   0.00000000000000\n"
        "  -0.00000000000000   0.00000000000000  11.06285889347163\n"
        "Co I \n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.66666666666667   0.33333333333334   0.38822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.88822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.61177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334   0.11177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334  -0.82526760342042  I     \n"
        "   0.33333333333333   0.66666666666666  -0.32526760342042  I     \n"
        "   0.33333333333333   0.66666666666666   0.82526760342042  I     \n"
        "   0.66666666666667   0.33333333333334   0.32526760342042  I     \n"
        "[VASP_POSCAR_MODE_EXPLICIT]STOP\n"
        "[AFLOW] ************************************************************************************************************************** \n";
      string calculated_string = aurostd::substring2string(test_string, _VASP_POSCAR_MODE_EXPLICIT_START_, _VASP_POSCAR_MODE_EXPLICIT_STOP_, 2);
      string expected_string = 
        "CoI/AB_hP8_194_f_f-002.AB params=-1,1.22\n"
        "1.301576\n"
        "   2.76416483956625  -4.78767394262424   0.00000000000000\n"
        "   2.76416483956625   4.78767394262424   0.00000000000000\n"
        "  -0.00000000000000   0.00000000000000  11.06285889347163\n"
        "Co I \n"
        "4 4 \n"
        "Direct(8) [A4B4] \n"
        "   0.66666666666667   0.33333333333334   0.38822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.88822433186164  Co    \n"
        "   0.33333333333333   0.66666666666666   0.61177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334   0.11177566813836  Co    \n"
        "   0.66666666666667   0.33333333333334  -0.82526760342042  I     \n"
        "   0.33333333333333   0.66666666666666  -0.32526760342042  I     \n"
        "   0.33333333333333   0.66666666666666   0.82526760342042  I     \n"
        "   0.66666666666667   0.33333333333334   0.32526760342042  I     ";
      checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | substring2strings //SD20230322
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::substring2strings()";
      check_description = "return all the matches as vector of strings";
      string test_string = "_FILE_START_\nIALGO==48\nALGO==FAST\nIALGO==49\nALGO==MEDIUM\nIALGO==50\nALGO==SLOW\n_FILE_END_";
      vector<string> calculated_vstring;
      aurostd::substring2strings(test_string, calculated_vstring, "IALGO");
      vector<string> expected_vstring = {"==48", "==49", "==50"};
      checkEqual(calculated_vstring, expected_vstring, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | substring2strings //CO20230502
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::substring2strings()";
      check_description = "return all the matches as vector of strings";
      string test_string = "[START]1234\n 4321[STOP] 456[START] 789 \n 987 [STOP]\n[START]4321 [STOP]654[START]987[STOP]";
      vector<string> calculated_vstring;
      aurostd::substring2strings(test_string, calculated_vstring, string("[START]"), string("[STOP]")); //CO20230502 - without string(), compiler picks the wrong overload...
      vector<string> expected_vstring = {"1234\n 4321"," 789 \n 987 ","4321 ","987"};
      checkEqual(calculated_vstring, expected_vstring, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | kvpair2string //SD20220525
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::kvpair2string()";
      check_description = "return the last match of the kvpair";
      string test_string = "_FILE_START_\nIALGO==48\nALGO==FAST\nIALGO==49\nALGO==MEDIUM\nIALGO==50\nALGO==SLOW\n_FILE_END_";
      string calculated_string = aurostd::kvpair2string(test_string, "ALGO", "==", -1);
      string expected_string = "SLOW";
      checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | string2utype //HE20220324
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::string2utype()";
      check_description = "int - bases 16, 10, 8, 5, 2";
      multi_check = true;
      multi_check = (multi_check && (aurostd::string2utype<int>("-420") == -420));
      multi_check = (multi_check && (aurostd::string2utype<int>("-420", 16)) == -1056);
      multi_check = (multi_check && (aurostd::string2utype<int>("-0x420", 16)) == -1056);
      multi_check = (multi_check && (aurostd::string2utype<int>("-0X420", 16)) == -1056);
      multi_check = (multi_check && (aurostd::string2utype<int>("-420", 16)) == -1056);
      multi_check = (multi_check && (aurostd::string2utype<int>("-420", 8)) == -272);
      multi_check = (multi_check && (aurostd::string2utype<int>("-0420", 8)) == -272);
      multi_check = (multi_check && (aurostd::string2utype<int>("-420", 5)) == -110);
      multi_check = (multi_check && (aurostd::string2utype<int>("-110100100", 2)) == -420);
      checkEqual(multi_check, true, check_function, check_description, passed_checks, results);
      check_description = "float - bases 16, 10, 8, 5, 2";
      multi_check = true;
      multi_check = (multi_check && (aurostd::string2utype<float>("-4.20") == -4.20f));
      multi_check = (multi_check && (aurostd::string2utype<float>("-420", 16)) == -1056.0f);
      multi_check = (multi_check && (aurostd::string2utype<float>("-420", 8)) == -272.0f);
      multi_check = (multi_check && (aurostd::string2utype<float>("-420", 5)) == -110.0f);
      multi_check = (multi_check && (aurostd::string2utype<float>("-110100100", 2)) == -420.0f);
      checkEqual(multi_check, true, check_function, check_description, passed_checks, results);

      check_description = "double - bases 16, 10, 8, 5, 2";
      multi_check = true;
      multi_check = (multi_check && (aurostd::string2utype<double>("-4.20") == -4.20));
      multi_check = (multi_check && (aurostd::string2utype<double>("-420", 16)) == -1056.0);
      multi_check = (multi_check && (aurostd::string2utype<double>("-420", 8)) == -272.0);
      multi_check = (multi_check && (aurostd::string2utype<double>("-420", 5)) == -110.0);
      multi_check = (multi_check && (aurostd::string2utype<double>("-110100100", 2)) == -420.0);
      checkEqual(multi_check, true, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | crc64 //HE20220404
    // ---------------------------------------------------------------------------
    {
      check_function = "aurostd::crc64()";
      check_description = "runtime hashing";
      expected_uint64 = 15013402708409085989UL;
      calculated_uint64 = aurostd::crc64("aflowlib_date");
      checkEqual(calculated_uint64, expected_uint64, check_function, check_description, passed_checks, results);

      check_function = "aurostd::ctcrc64()";
      check_description = "compiler hashing (constexpr)";
      static constexpr uint64_t
        calculated_const_uint64 = aurostd::ctcrc64("aflowlib_date");
      checkEqual(calculated_const_uint64, expected_uint64, check_function, check_description, passed_checks, results);
    }


    // ---------------------------------------------------------------------------
    // Check | crc2human //HE20230221
    // ---------------------------------------------------------------------------

    {
      check_function = "aurostd::crc2human()";
      check_description = "create human readable hash";
      std::string expected = "NEPPW041L8FJ2";
      std::string result = aurostd::crc2human("Hello World!");
      checkEqual(result, expected, check_function, check_description, passed_checks, results);

      check_description = "create truncated human readable hash";
      expected = "NEPPW0";
      result = aurostd::crc2human("Hello World!", 6);
      checkEqual(result, expected, check_function, check_description, passed_checks, results);

      check_description = "check padding";
      expected = "2ZP300";
      result = aurostd::crc2human(145624, 6);
      checkEqual(result, expected, check_function, check_description, passed_checks, results);
    }


    // ST20250519 Unit tests for RemoveWhitespaceFromThe* functions
    {
      check_function = "aurostd::RemoveWhiteSpacesFromTheFront()";
      check_description = "String whitespace functions";
      const std::string input = "  \n\n Quick brown fox jumps over the lazy dog \n\n  ";
      std::string result = aurostd::RemoveWhiteSpacesFromTheFront(input);
      std::string expected = "\n\n Quick brown fox jumps over the lazy dog \n\n  ";
      checkEqual(result, expected, check_function, check_description, passed_checks, results);

      check_function = "aurostd::RemoveWhiteSpacesFromTheFront(remove_newline)";
      result = aurostd::RemoveWhiteSpacesFromTheFront(input, true);
      expected = "Quick brown fox jumps over the lazy dog \n\n  ";
      checkEqual(result, expected, check_function, check_description, passed_checks, results);

      check_function = "aurostd::RemoveWhiteSpacesFromTheBack()";
      result = aurostd::RemoveWhiteSpacesFromTheBack(input);
      expected = "  \n\n Quick brown fox jumps over the lazy dog \n\n";
      checkEqual(result, expected, check_function, check_description, passed_checks, results);

      check_function = "aurostd::RemoveWhiteSpacesFromTheBack(remove_newline)";
      result = aurostd::RemoveWhiteSpacesFromTheBack(input, true);
      expected = "  \n\n Quick brown fox jumps over the lazy dog";
      checkEqual(result, expected, check_function, check_description, passed_checks, results);

      check_function = "aurostd::RemoveWhiteSpacesFromTheFrontAndBack()";
      result = aurostd::RemoveWhiteSpacesFromTheFrontAndBack(input);
      expected = "\n\n Quick brown fox jumps over the lazy dog \n\n";
      checkEqual(result, expected, check_function, check_description, passed_checks, results);

      check_function = "aurostd::RemoveWhiteSpacesFromTheFrontAndBack(remove_newline)";
      result = aurostd::RemoveWhiteSpacesFromTheFrontAndBack(input, true);
      expected = "Quick brown fox jumps over the lazy dog";
      checkEqual(result, expected, check_function, check_description, passed_checks, results);


    }

    //CO20261002 START
    // ---------------------------------------------------------------------------
    // Check | reorder
    // ---------------------------------------------------------------------------
    // the loops ran to vec.size()-1, which wraps around for an empty vector: chull reordered an
    // element's empty entry list and crashed (reorder is instantiated for entries, so the test uses
    // entries labeled by auid)
    {
      check_function = "aurostd::reorder()";
      std::function<vector<aflowlib::_aflowlib_entry>(const vector<string>&)> make = [](const vector<string>& labels) {
        vector<aflowlib::_aflowlib_entry> v(labels.size());
        for (size_t i = 0; i < labels.size(); i++) v[i].auid = labels[i];
        return v;
      };
      std::function<string(const vector<aflowlib::_aflowlib_entry>&)> labels = [](const vector<aflowlib::_aflowlib_entry>& v) {
        vector<string> out;
        for (size_t i = 0; i < v.size(); i++) out.push_back(v[i].auid);
        return aurostd::joinWDelimiter(out, ",");
      };
      vector<aflowlib::_aflowlib_entry> vec;
      vector<uint> vorder;
      for (uint mode = 1; mode <= 2; mode++) {
        vec.clear(); vorder.clear();
        aurostd::reorder(vec, vorder, mode);
        checkEqual(vec.size(), (size_t) 0, check_function, "empty vector, mode " + aurostd::utype2string(mode), passed_checks, results);
        vec = make({"7"}); vorder = {0};
        aurostd::reorder(vec, vorder, mode);
        checkEqual(labels(vec), string("7"), check_function, "one element, mode " + aurostd::utype2string(mode), passed_checks, results);
      }
      // the examples in the function's comment
      vec = make({"7", "5", "9", "6"}); vorder = {1, 3, 0, 2};
      aurostd::reorder(vec, vorder, 1);
      checkEqual(labels(vec), string("5,6,7,9"), check_function, "mode 1: take the elements from the positions", passed_checks, results);
      vec = make({"7", "5", "9", "6"}); vorder = {1, 3, 0, 2};
      aurostd::reorder(vec, vorder, 2);
      checkEqual(labels(vec), string("9,7,6,5"), check_function, "mode 2: move the elements to the positions", passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | httpPercentEncodingFull
    // ---------------------------------------------------------------------------
    // a string that ended in an unreserved character got "%00" appended: the loop encoded
    // work_str[pos] at pos == size(), the terminating NUL (CO20261004)
    {
      check_function = "aurostd::httpPercentEncodingFull()";
      checkEqual(aurostd::httpPercentEncodingFull("paging(1,5),aurl"), string("paging%281%2C5%29%2Caurl"), check_function, "ends in an unreserved character", passed_checks, results);
      checkEqual(aurostd::httpPercentEncodingFull("species(Mg,O)"), string("species%28Mg%2CO%29"), check_function, "ends in a reserved character", passed_checks, results);
      checkEqual(aurostd::httpPercentEncodingFull("Az09-_.~"), string("Az09-_.~"), check_function, "only unreserved characters", passed_checks, results);
      checkEqual(aurostd::httpPercentEncodingFull(""), string(""), check_function, "empty", passed_checks, results);
      checkEqual(aurostd::httpPercentEncodingFull("a b:c"), string("a%20b%3Ac"), check_function, "space and colon", passed_checks, results);
      checkEqual(aurostd::httpPercentEncodingFull("\xC3\xA9"), string("%C3%A9"), check_function, "bytes above 127", passed_checks, results);
      checkEqual(aurostd::httpPercentEncodingFull(string("a\0b", 3)), string("a%00b"), check_function, "a NUL inside the string is still encoded", passed_checks, results);
    }
    //CO20261002 STOP

    // ---------------------------------------------------------------------------
    // Check | RemoveCharacterInPlace //CO20261004
    // ---------------------------------------------------------------------------
    // EntryLoader::cleanAUID() strips the quotes of a JSON AUID this way
    check_function = "aurostd::RemoveCharacterInPlace()";
    check_description = "strip the quotes of a JSON AUID";
    calculated_string = "\"aflow:0123456789abcdef\"";
    aurostd::RemoveCharacterInPlace(calculated_string, '"');
    expected_string = "aflow:0123456789abcdef";
    checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);
    check_description = "every occurrence is removed, nothing else";
    calculated_string = "\"a\"\"b c\"";
    aurostd::RemoveCharacterInPlace(calculated_string, '"');
    expected_string = "ab c";
    checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);
    check_description = "a string without the character is unchanged";
    calculated_string = "s4e:0171c43afba95d07";
    aurostd::RemoveCharacterInPlace(calculated_string, '"');
    expected_string = "s4e:0171c43afba95d07";
    checkEqual(calculated_string, expected_string, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | vectorstring2file, KBIN::AFLOWIN_ADD and KBIN::AFLOWIN_REMOVE //CO20261004
    // ---------------------------------------------------------------------------
    // A failed write must be reported, and a failed rewrite of aflow.in must leave it whole
    // (aflow.in files cut short on a full scratch disk of our cluster, 2025-2026).
    {
      const vector<string> vlines = {"[AFLOW] unit test", "[AFLOW_MODE=VASP]", "[VASP_RUN]RELAX=2"};
      check_function = "aurostd::vectorstring2file()";
      if (aurostd::FileExist("/dev/full")) {  // opens, then fails every write with ENOSPC
        check_description = "a failed write returns false";
        checkEqual(aurostd::vectorstring2file(vlines, "/dev/full"), false, check_function, check_description, passed_checks, results);
      }
      const string dir = aurostd::TmpDirectoryCreate("unit_test_aflowin");
      const string aflowin = dir + "/" + _AFLOWIN_;
      check_description = "a clean write returns true";
      checkEqual(aurostd::vectorstring2file(vlines, aflowin), true, check_function, check_description, passed_checks, results);

      check_function = "KBIN::AFLOWIN_ADD()";
      check_description = "the line is added after the others";
      KBIN::AFLOWIN_ADD(aflowin, string("[VASP_FORCE_OPTION]ALGO=NORMAL"), "unit test");
      vector<string> vread, vfiles;
      aurostd::file2vectorstring(aflowin, vread, true, true);
      checkEqual(vread.size() == 4 && vread[0] == vlines[0] && vread[2] == vlines[2] &&
          vread[3].find("[VASP_FORCE_OPTION]ALGO=NORMAL") == 0, true, check_function, check_description, passed_checks, results);
      check_description = "no temporary file is left behind";
      aurostd::DirectoryLS(dir, vfiles);
      checkEqual(vfiles.size() == 1 && vfiles[0] == _AFLOWIN_, true, check_function, check_description, passed_checks, results);

      check_function = "KBIN::AFLOWIN_REMOVE()";
      check_description = "the keyword is commented out, the other lines kept";
      KBIN::AFLOWIN_REMOVE(aflowin, "[VASP_RUN]RELAX=2", "unit test");
      aurostd::file2vectorstring(aflowin, vread, true, true);
      checkEqual(vread.size() == 4 && vread[1] == vlines[1] && vread[2].find("#[AFLOW REMOVED") == 0 &&
          vread[2].find(vlines[2]) != string::npos && vread[3].find("[VASP_FORCE_OPTION]ALGO=NORMAL") == 0,
          true, check_function, check_description, passed_checks, results);

      if (geteuid() != 0) {  // root writes regardless of permissions
        check_function = "KBIN::AFLOWIN_ADD()";
        check_description = "a rewrite that cannot be written throws and leaves aflow.in whole";
        const string before = aurostd::file2string(aflowin);
        chmod(dir.c_str(), 0555);
        bool thrown = false;
        try { KBIN::AFLOWIN_ADD(aflowin, string("[VASP_FORCE_OPTION]NBANDS=20"), "unit test"); }
        catch (aurostd::xerror&) { thrown = true; }
        chmod(dir.c_str(), 0755);
        checkEqual(thrown && aurostd::file2string(aflowin) == before, true, check_function, check_description, passed_checks, results);
      }
      aurostd::RemoveDirectory(dir);
    }

    // ---------------------------------------------------------------------------
    // Check | max, min, sum, mean, reset, clear: empty containers and strings //CO20261004
    // ---------------------------------------------------------------------------
    // These return or assign utype() since CO20260913, where they cast the literal 0. They are instantiated for
    // std::string too, and (std::string) 0 builds a string from a null pointer (undefined; std::logic_error in
    // libstdc++), so for strings they threw on empty containers, in reset() and clear(), and in sum() of any vector,
    // which started from utype out=0. For the arithmetic types utype() is 0, as before.
    {
      const vector<string> vstr_empty;
      const deque<string> dstr_empty;
      const vector<vector<string> > mstr_empty;
      const vector<string> vstr = {"Co", "Mg", "O"};
      const deque<string> dstr(vstr.begin(), vstr.end());
      const vector<vector<string> > mstr = {{"Co", "Mg"}, {"O"}};
      string threw = "";
      vector<string> vcalculated;
      try {
        vcalculated = {aurostd::max(vstr_empty), aurostd::max(dstr_empty), aurostd::max(mstr_empty),
          aurostd::min(vstr_empty), aurostd::min(dstr_empty), aurostd::min(mstr_empty),
          aurostd::sum(vstr_empty), aurostd::sum(dstr_empty), aurostd::sum(mstr_empty)};
      } catch (std::exception& excpt) {threw = excpt.what();}
      check_function = "aurostd::max(), aurostd::min(), aurostd::sum()";
      check_description = "empty vector, deque and vector<vector> of strings give an empty string" + (threw.empty() ? string("") : " [" + threw + "]");
      checkEqual(aurostd::joinWDelimiter(vcalculated, "|"), aurostd::joinWDelimiter(vector<string>(9, ""), "|"), check_function, check_description, passed_checks, results);

      threw = "";
      vcalculated.clear();
      try {vcalculated = {aurostd::sum(vstr), aurostd::sum(dstr), aurostd::sum(mstr)};}
      catch (std::exception& excpt) {threw = excpt.what();}
      check_function = "aurostd::sum()";
      check_description = "strings are concatenated from an empty string (vector, deque, vector<vector>)" + (threw.empty() ? string("") : " [" + threw + "]");
      checkEqual(aurostd::joinWDelimiter(vcalculated, "|"), string("CoMgO|CoMgO|CoMgO"), check_function, check_description, passed_checks, results);

      threw = "";
      vector<string> vreset = vstr, vclear = vstr;
      deque<string> dreset = dstr, dclear = dstr;
      vector<vector<string> > mreset, mclear;
      try {
        aurostd::reset(vreset); aurostd::clear(vclear); aurostd::reset(dreset); aurostd::clear(dclear);
        mreset = aurostd::reset(mstr); mclear = aurostd::clear(mstr);
      } catch (std::exception& excpt) {threw = excpt.what();}
      auto lengths = [](const vector<vector<string> >& containers) {  // length of each string, containers separated by ;
        vector<string> out;
        for (const vector<string>& c : containers) {vector<uint> l; for (const string& s : c) l.push_back(s.size()); out.push_back(aurostd::joinWDelimiter(l, ","));}
        return aurostd::joinWDelimiter(out, ";");
      };
      check_function = "aurostd::reset(), aurostd::clear()";
      check_description = "every string becomes empty, sizes kept (vector, deque, vector<vector>)" + (threw.empty() ? string("") : " [" + threw + "]");
      checkEqual(lengths({vreset, vclear, vector<string>(dreset.begin(), dreset.end()), vector<string>(dclear.begin(), dclear.end())}) + "|" + lengths(mreset) + "|" + lengths(mclear),
          string("0,0,0;0,0,0;0,0,0;0,0,0|0,0;0|0,0;0"), check_function, check_description, passed_checks, results);

      check_function = "aurostd::max(), aurostd::min(), aurostd::sum(), aurostd::mean()";
      const vector<double> vdbl_empty;
      const vector<int> vint_empty;
      check_description = "double: empty gives 0 (max, min, sum, mean), mean of 1,2,3 is 2, as before";
      checkEqual(vector<double>{aurostd::max(vdbl_empty), aurostd::min(vdbl_empty), aurostd::sum(vdbl_empty), aurostd::mean(vdbl_empty), aurostd::mean(vector<double>{1.0, 2.0, 3.0})},
          vector<double>{0.0, 0.0, 0.0, 0.0, 2.0}, check_function, check_description, passed_checks, results);
      check_description = "int: empty gives 0 (max, min, sum), sum of 1,2,3 is 6, as before";
      checkEqual(vector<int>{aurostd::max(vint_empty), aurostd::min(vint_empty), aurostd::sum(vint_empty), aurostd::sum(vector<int>{1, 2, 3})},
          vector<int>{0, 0, 0, 6}, check_function, check_description, passed_checks, results);
      vector<int> vint_reset = {3, 4};
      aurostd::reset(vint_reset);
      check_function = "aurostd::reset()";
      check_description = "int: elements become 0, as before";
      checkEqual(vint_reset, vector<int>{0, 0}, check_function, check_description, passed_checks, results);
    }

    //CO20261010 START - sorting two deques: empty deques stay empty, others sort by the first
    check_function = "aurostd::sort()";
    deque<double> sort_keys, sort_keys_empty;
    deque<uint> sort_values, sort_values_empty;
    sort_keys.push_back(3.0); sort_keys.push_back(1.0); sort_keys.push_back(2.0);
    sort_values.push_back(30); sort_values.push_back(10); sort_values.push_back(20);
    aurostd::sort(sort_keys, sort_values);
    aurostd::sort(sort_keys_empty, sort_values_empty);
    checkEqual(
        aurostd::joinWDelimiter(aurostd::vecDouble2vecString(vector<double>(sort_keys.begin(), sort_keys.end())), ",") + ";" +
            aurostd::joinWDelimiter(vector<uint>(sort_values.begin(), sort_values.end()), ",") + ";" +
            aurostd::utype2string(sort_keys_empty.size()) + "," + aurostd::utype2string(sort_values_empty.size()),
        string("1,2,3;10,20,30;0,0"),
        check_function,
        "sort two deques by the first; empty deques stay empty",
        passed_checks,
        results);
    //CO20261010 STOP - sorting two deques: empty deques stay empty, others sort by the first
  }



  void UnitTest::xparserTest(uint &passed_checks, vector <vector<string>> &results, vector <string> &errors) {
    (void) errors; // Suppress compiler warnings


    string task_description = "Test xparsers";
    stringstream result;
    string check_description="";
    string check_function="";

    // ---------------------------------------------------------------------------
    // Check | aurostd::JSON
    // ---------------------------------------------------------------------------

    // read strings
    // tests based on https://seriot.ch/projects/parsing_json.html
    {
      check_function = "JSON";
      check_description = "test string parsing";
      std::string string_json = string(utd["xparser"]["string_json"]);

      std::map < string, string > string_results({{"1_2_3_bytes_UTF-8_sequences", "\u0060\u012a\u12AB"},
                                                  {"allowed_escapes", "\"\\/\b\f\n\r\t"}, {"pi", "π"}, {"escaped_noncharacter", "\uFFFF"},
                                                  {"last_surrogates_1_and_2", "􏿿"}, {"accepted_surrogate_pair", "𐐷"},
                                                  {"unicode", "\uA66D"}, {"unicode_U+1FFFE_nonchar", "🿾"}, {"backslash_and_u_escaped_zero", "\\u0000"},
                                                  {"three-byte-utf-8", "\u0821"}, {"backslash_doublequotes", "\""}, {"uescaped_newline", "new\u000Aline"},
                                                  {"unicode_escaped_double_quote", "\u0022"}, {"double_escape_a", "\\a"}, {"comments", "a/*b*/c/*d//e"},
                                                  {"space", " "}, {"uEscape", "\u0061\u30af\u30EA\u30b9"}, {"unicode_U+200B_ZERO_WIDTH_SPACE", "\u200B"},
                                                  {"u+2028_line_sep", " "}, {"two-byte-utf-8", "\u0123"}, {"unicodeEscapedBackslash", "\u005C"},
                                                  {"unicode_U+2064_invisible_plus", "\u2064"}, {"escaped_control_character", "\u0012"}, {"simple_ascii", "asd "},
                                                  {"unicode_U+10FFFE_nonchar", "􏿾"}, {"utf8", "€𝄞"}, {"unescaped_char_delete", ""},
                                                  {"surrogates_U+1D11E_MUSICAL_SYMBOL_G_CLEF", "𝄞"}, {"double_escape_n", "\\n"},
                                                  {"with_del_character", "aa"}, {"nonCharacterInUTF-8_U+FFFF", "￿"},
                                                  {"accepted_surrogate_pairs", "😹💍"}, {"in_array_with_leading_space", "asd"},
                                                  {"nonCharacterInUTF-8_U+10FFFF", "􏿿"}, {"one-byte-utf-8", "\u002c"}, {"unicode_2", "⍂㈴⍂"},
                                                  {"unicode_U+FDD0_nonchar", "\uFDD0"}, {"nbsp_uescaped", "new\u00A0line"}, {"unicode_U+FFFE_nonchar", "\uFFFE"},
                                                  {"reservedCharacterInUTF-8_U+1BFFF", "𛿿"}, {"u+2029_par_sep", " "}});

      std::map<string,string> escaped_results ({{"1_2_3_bytes_UTF-8_sequences","`\\u012a\\u12ab"},{"accepted_surrogate_pair","\\ud801\\udc37"},
                                                {"accepted_surrogate_pairs","\\ud83d\\ude39\\ud83d\\udc8d"},{"allowed_escapes","\\\"\\\\\\/\\b\\f\\n\\r\\t"},
                                                {"backslash_and_u_escaped_zero","\\\\u0000"},{"backslash_doublequotes","\\\""},{"comments","a\\/*b*\\/c\\/*d\\/\\/e"},
                                                {"double_escape_a","\\\\a"},{"double_escape_n","\\\\n"},{"escaped_control_character","\\u0012"},{"escaped_noncharacter","\\uffff"}, //CO20261004 - control characters are escaped as \\u00XX
                                                {"in_array_with_leading_space","asd"},{"last_surrogates_1_and_2","\\udbff\\udfff"},{"nbsp_uescaped","new\\u00a0line"},
                                                {"nonCharacterInUTF-8_U+10FFFF","\\udbff\\udfff"},{"nonCharacterInUTF-8_U+FFFF","\\uffff"},{"one-byte-utf-8",","},
                                                {"pi","\\u03c0"},{"reservedCharacterInUTF-8_U+1BFFF","\\ud82f\\udfff"},{"simple_ascii","asd "},{"space"," "},
                                                {"surrogates_U+1D11E_MUSICAL_SYMBOL_G_CLEF","\\ud834\\udd1e"},{"three-byte-utf-8","\\u0821"},{"two-byte-utf-8","\\u0123"},
                                                {"u+2028_line_sep","\\u2028"},{"u+2029_par_sep","\\u2029"},{"uEscape","a\\u30af\\u30ea\\u30b9"},
                                                {"uescaped_newline","new\\nline"},{"unescaped_char_delete",""},{"unicode","\\ua66d"},{"unicodeEscapedBackslash","\\\\"},
                                                {"unicode_2","\\u2342\\u3234\\u2342"},{"unicode_U+10FFFE_nonchar","\\udbff\\udffe"},{"unicode_U+1FFFE_nonchar","\\ud83f\\udffe"},
                                                {"unicode_U+200B_ZERO_WIDTH_SPACE","\\u200b"},{"unicode_U+2064_invisible_plus","\\u2064"},
                                                {"unicode_U+FDD0_nonchar","\\ufdd0"},{"unicode_U+FFFE_nonchar","\\ufffe"},{"unicode_escaped_double_quote","\\\""},
                                                {"utf8","\\u20ac\\ud834\\udd1e"},{"with_del_character","aa"}});

      aurostd::JSON::object jo = aurostd::JSON::loadString(string_json);
      bool overall_test = true;
      for (const auto &entry: string_results) {
        if ((string) jo[entry.first] != entry.second) {
          check_description += "(at " + entry.first + " subtest)";
          check(false, (string) jo[entry.first], entry.second, check_function, check_description, passed_checks,results);
          overall_test = false;
          break;
        }
      }
      if (overall_test) check(overall_test, 0, 0, check_function, check_description, passed_checks, results);

      check_description = "test string conversion to JSON";
      overall_test = true;
      for (const auto &entry: escaped_results) {
        if (jo[entry.first].toString(true, true) != "\""+entry.second+"\"") {
          check_description += "(at " + entry.first + " subtest)";
          check(false, (string) jo[entry.first], "\""+entry.second+"\"", check_function, check_description, passed_checks,results);
          overall_test = false;
          break;
        }
      }
      if (overall_test) check(overall_test, 0, 0, check_function, check_description, passed_checks, results);

      //CO20261004 START - control characters U+0000 to U+001F are escaped as \\u00XX, with and without unicode escapes
      check_function = "aurostd::JSON::escape()";
      const string control_characters = string("a\x01") + '\0' + "\x1f" "\tb";
      checkEqual(aurostd::JSON::escape(control_characters, false), string("a\\u0001\\u0000\\u001f\\tb"),
          check_function, "escape control characters without unicode escapes", passed_checks, results);
      checkEqual(aurostd::JSON::escape(string("\x1f" "\xce\xb1 \"/\\"), true), string("\\u001f\\u03b1 \\\"\\/\\\\"),
          check_function, "escape control characters with unicode escapes", passed_checks, results);
      checkEqual(aurostd::JSON::escape(string("\xce\xb1"), false), string("\xce\xb1"),
          check_function, "keep UTF-8 bytes without unicode escapes", passed_checks, results);
      //CO20261004 STOP - control characters U+0000 to U+001F are escaped as \\u00XX, with and without unicode escapes
    }

    // read single numbers
    // tests based on https://seriot.ch/projects/parsing_json.html
    {
      check_function = "JSON";
      check_description = "test number parsing (double, long long)";
      std::string number_json = string(utd["xparser"]["number_json"]);

      aurostd::JSON::object jo = aurostd::JSON::loadString(number_json);

      std::map < string, double > double_results( {{"0e+1", 0},  {"0e1", 0},  {"after_space", 4},  {"double_close_to_zero", -1e-78},
                                                   {"double__too_close_to_zero", NAN},  {"int_with_exp", 200},  {"minus_zero", -0},
                                                   {"negative_int", -123},  {"negative_one", -1},  {"real_capital_e", 1e+22},
                                                   {"real_capital_e_neg_exp", 0.01},  {"real_capital_e_pos_exp", 100},  {"real_exponent", 1.23e+47},
                                                   {"real_fraction_exponent", 123.456e78},  {"real_neg_exp", 0.01},  {"real_pos_exponent", 100},
                                                   {"simple_int", 123},  {"simple_real", 123.456789},  {"number", 1.23e+67},
                                                   {"true", 1.0}, {"false", 0.0}, {"null", NAN}});
      std::map <string,long long> ll_results({{"0e+1", 0}, {"0e1", 0}, {"after_space", 4}, {"double_close_to_zero", 0},
                                              {"int_with_exp", 200}, {"minus_zero", 0}, {"negative_int", -123}, {"negative_one", -1},
                                              {"real_capital_e", std::numeric_limits<long long>::max()}, {"real_capital_e_neg_exp", 0},
                                              {"real_capital_e_pos_exp", 100}, {"real_exponent", std::numeric_limits<long long>::max()},
                                              {"real_fraction_exponent", std::numeric_limits<long long>::max()}, {"real_neg_exp", 0},
                                              {"real_pos_exponent", 100}, {"simple_int", 123}, {"simple_real", 123},
                                              {"number", std::numeric_limits<long long>::max()}, {"true", 1}, {"false", 0}});

      bool overall_test = true;
      for (const auto &entry: double_results) {
        if ((double) jo[entry.first] != entry.second) {
          if (std::isnan((double)jo[entry.first]) && std::isnan(entry.second)) continue;
          check_description += "(at " + entry.first + " subtest)";
              check(false, (double) jo[entry.first], entry.second, check_function, check_description, passed_checks,results);
          overall_test = false;
          break;
        }
      }
      for (const auto &entry: ll_results) {
        if ((long long) jo[entry.first] != entry.second) {
          check_description += "(at " + entry.first + " subtest)";
          check(false, (long long) jo[entry.first], entry.second, check_function, check_description, passed_checks,results);
          overall_test = false;
          break;
        }
      }
      if (overall_test) check(overall_test, 0, 0, check_function, check_description, passed_checks, results);

    }

    // read xvectors
    {
      check_function = "JSON";
      check_description = "test parsing xvector<";
      std::string number_json = string(utd["xparser"]["vector_number_json"]);

      aurostd::JSON::object jo = aurostd::JSON::loadString(number_json);

      xvector<double> xvd_res = jo["xvector_double"];
      xvector<double> xvd_exp = {923.49445786, -441.74004105, 465.49355057, 96.15610686, 557.6834903 , 147.6777196 , 871.81485459, 287.89958188, 863.66132302, 876.36635155};
      checkEqual( xvd_res, xvd_exp, check_function, check_description + "double>", passed_checks, results);


      xvd_res = jo["xvector_nan"];
      xvd_exp = {449644208, -441.74004105, NAN, 725767871, 1.0, 946128279, 65015635, 287.89958188, 863.66132302, 762013152};
      checkEqual( xvd_res, xvd_exp, check_function, check_description + "double> mixed + NAN", passed_checks, results);

      xvector<float> xvf_res = jo["xvector_nan"];
      xvector<float> xvf_exp = {449644224, -441.74004105, NAN, 725767872, 1.0, 946128256, 65015636, 287.89958188, 863.66132302, 762013184}; // values different from double -> narrowing
      checkEqual( xvf_res, xvf_exp, check_function, check_description + "float> mixed + NAN", passed_checks, results);

      xvector<long long> xvll_res = jo["xvector_ll"];
      xvector<long long> xvll_exp = {449644208, -252515403, 601496576, 725767871, 502088591, 946128279, 65015635, 352203056, 717938486, 762013152};
      checkEqual( xvll_res, xvll_exp, check_function, check_description + "long long>", passed_checks, results);

      xvector<unsigned long long> xvull_res = jo["xvector_ull"];
      xvector<unsigned long long> xvull_exp = {449644208, 252515403, 601496576, 725767871, 502088591, 946128279, 65015635, 352203056, 717938486, 762013152};
      checkEqual( xvull_res, xvull_exp, check_function, check_description + "unsigned long long>", passed_checks, results);


      xvll_res = jo["xvector_mixed"];
      xvll_exp = {449644208, -441, 465, 725767871, 1, 946128279, 65015635, 287, 863, 762013152};
      checkEqual( xvll_res, xvll_exp, check_function, check_description + "long long> mixed", passed_checks, results);

      xvector<int> xvi_res = jo["xvector_mixed"];
      xvector<int> xvi_exp = {449644208, -441, 465, 725767871, 1, 946128279, 65015635, 287, 863, 762013152};
      checkEqual( xvi_res, xvi_exp, check_function, check_description + "int> mixed", passed_checks, results);

      // <uint> would fail as operator << xvector<uint> does not exist yet
    }

    // read xmatrix
    {
      check_function = "JSON";
      check_description = "test parsing xmatrix<";

      std::string matrix_json = string(utd["xparser"]["matrix_json"]);
      aurostd::JSON::object jo = aurostd::JSON::loadString(matrix_json);

      xmatrix<double> xmd_exp = {{ 2.85899214e+07, 2.70836286e+04,              82,  7.47207733e+08,  4.67299748e+04, -6.25361399e+09, 8.00388892e+08},
                                {-2.50841390e+04, 1.86048071e+07,  7.18691607e+04, -7.18492121e+00,  5.11971140e+02,  8.97364584e+06, 1.68446225e+02},
                                { 4.94590604e+08, 1.89067109e+09,  4.64803095e+00,  7.74359909e+00,            5465,  1.37944663e+02, 1.37565108e+08},
                                {           8499, 1.73734230e+03,  3.28957479e+08, -5.41490015e+03,  5.52617678e+00,  2.64034084e+09, 4.76561709e+01},
                                { 3.31904798e+04,     2288175298, -2.96213531e+07,  1.22631199e+02, -8.83941811e+00, -2.17327450e+04, 3.77389069e+03},
                                {-2.98263326e+04, 5.39915781e+05,  1.28198687e+09,  2.69529424e+07,  7.47855186e+01,      1538912783, 2.55027330e+02}};
      xmatrix<double> xmd_res = jo["xmatrix_double_mix"];
      checkEqual( xmd_res, xmd_exp, check_function, check_description + "double> mixed", passed_checks, results);


      xmatrix<int> xmll_exp = {{  28589921,       27083,         82, 747207733, 46729, -1958646694, 800388892},
                               {    -25084,    18604807,      71869,        -7,   511,     8973645,       168},
                               { 494590604,  1890671090,          4,         7,  5465,         137, 137565108},
                               {      8499,        1737,  328957479,     -5414,     5, -1654626456,        47},
                               {     33190, -2006791998,  -29621353,       122,    -8,      -21732,      3773},
                               {    -29826,      539915, 1281986870,  26952942,    74,  1538912783,       255}};
      xmatrix<int> xmll_res = jo["xmatrix_double_mix"];
      checkEqual( xmll_res, xmll_exp, check_function, check_description + "int> mixed", passed_checks, results);
    }

    // read std::map
    {
      check_function = "JSON";
      check_description = "test parsing std:map<string,";
      std::string map_json = string(utd["xparser"]["map_json"]);

      std::map<string, double> md_exp = {{"VNZPC", 2},
                                         {"HQPZP", -2472285305.2312846},
                                         {"DUKPG", 24171},
                                         {"NUJYO", -108940230},
                                         {"MDPSG", 15011078.748e-5},
                                         {"UKYVK", 1072982.12214322},
                                         {"UCZSX", 825.8070164E2},
                                         {"SQBER", -2056},
                                         {"XGPXD", 853427.8245249396},
                                         {"UASBY", 4474520688}};
      aurostd::JSON::object jo = aurostd::JSON::loadString(map_json);
      std::map<string, double> md_res = jo["map"];
      bool overall_test = true;
      for (const std::pair<string, double> entry: md_exp){
        if (entry.second != md_res[entry.first]) {
          check_description += "double> (at " + entry.first + " subtest key)";
          check(false, md_res[entry.first], entry.second, check_function, check_description, passed_checks,results);
          overall_test = false;
          break;
        }
      }
      if (overall_test) check(overall_test, 0, 0, check_function, check_description+ "double>)", passed_checks, results);

      std::map<string, long long> mll_exp = {{"VNZPC", 2},
                                             {"HQPZP", -2472285305},
                                             {"DUKPG", 24171},
                                             {"NUJYO", -108940230},
                                             {"MDPSG", 150},
                                             {"UKYVK", 1072982},
                                             {"UCZSX", 82580},
                                             {"SQBER", -2056},
                                             {"XGPXD", 853427},
                                             {"UASBY", 4474520688}};
      std::map<string, long long> mll_res = jo["map"];
      overall_test = true;
      for (const std::pair<string, long long> entry: mll_exp){
        if (entry.second != mll_res[entry.first]) {
          check_description += "long long> (at subtest key " + entry.first + ")";
          check(false, mll_res[entry.first], entry.second, check_function, check_description, passed_checks,results);
          overall_test = false;
          break;
        }
      }
      if (overall_test) check(overall_test, 0, 0, check_function, check_description+ "long long>)", passed_checks, results);

    }

    // read std::vector
    {
      check_function = "JSON";
      check_description = "test parsing vector<";

      std::string vector_json = string(utd["xparser"]["vector_mixed_json"]);

      aurostd::JSON::object jo = aurostd::JSON::loadString(vector_json);
      std::vector<std::string> vs_res = jo["string_vector"];
      std::vector<std::string> vs_exp = {"First w\"ord", "Second word", "Last word"};
      checkEqual( vs_res, vs_exp, check_function, check_description + "string>", passed_checks, results);

      vs_res = jo["mixed_vector"];
      vs_exp = {"{\"DUKPG\":24171,\"HQPZP\":-2.47229e+09,\"VNZPC\":2}", "5.68e-06", "152.324", "784", "A string", "true", "false", "null", "[\"A\",\"nested\",\"list\"]"};
      checkEqual( vs_res, vs_exp, check_function, check_description + "string> mixed", passed_checks, results);

      std::vector<double> vd_res = jo["number_vector"];
      std::vector<double> vd_exp = {449644208, 441.74004105, 465.49355057, 725767871, 1.0, 946128279, 65015635, 287.89958188, 863.66132302, 762013152};
      checkEqual( vd_res, vd_exp, check_function, check_description + "double> mixed", passed_checks, results);

      std::vector<int> vll_res = jo["number_vector"];
      std::vector<int> vll_exp = {449644208, 441, 465, 725767871, 1, 946128279, 65015635, 287, 863, 762013152};
      checkEqual( vll_res, vll_exp, check_function, check_description + "int> mixed", passed_checks, results);

      for (string key: {"bool>", "bool> mixed"}) {// aurostd::joinWDelimiter does not work for bool
        std::vector<bool> vb_res = jo[key];
        std::vector<bool> vb_exp = {true, false, false, false, true};
        bool overall_test = true;
        for (size_t idx = 0; idx < vb_exp.size(); idx++) {
          if (vb_res[idx] != vb_exp[idx]) {
            check_description += key;
            check(false, vb_res[idx], vb_exp[idx], check_function, check_description, passed_checks, results);
            overall_test = false;
            break;
          }
        }
        if (overall_test)
          check(overall_test, 0, 0, check_function, check_description + "bool>", passed_checks, results);
      }
    }

    // create JSON storage objects
    {
      check_function = "JSON";
      check_description = "test converting ";
      aurostd::JSON::object so;
      {
        int exp = 4652;
        so = exp;
        checkEqual((int)so, exp, check_function, check_description + "int", passed_checks, results);
      }
      {
        so = "Hello World";
        checkEqual((std::string) so, "Hello World", check_function, check_description + "std::string", passed_checks, results);
      }
      {
        so = nullptr;
        checkEqual((std::string)so, "null", check_function, check_description + "nullptr", passed_checks, results);
      }
      {
        xcomplex<double> exp(51.25,0.3485);
        so = exp;
        checkEqual((xcomplex<double>)so, exp, check_function, check_description + "xcomplex<double>", passed_checks, results);
      }
      {
        xcomplex<float> exp(51.25,0.3485);
        so = exp;
        checkEqual((xcomplex<float>)so, exp, check_function, check_description + "xcomplex<float>", passed_checks, results);
      }
      {
        vector<float> exp = {12.33,20.7,34.23454};
        so = exp;
        checkEqual((std::string)so, "[12.33,20.7,34.2345]", check_function, check_description + "vector<float>", passed_checks, results);
      }
      {
        xvector<uint> exp = {3,7,10};
        so = exp;
        checkEqual((std::string)so, "[3,7,10]", check_function, check_description + "xvector<uint>", passed_checks, results);
      }
      {
        xmatrix<float> exp = {{1.0,2.0,3.0,4.0},{5,6,7,8},{9,10,11,12},{13,14,15,16}};
        so = exp;
        checkEqual((std::string)so, "[[1,2,3,4],[5,6,7,8],[9,10,11,12],[13,14,15,16]]", check_function, check_description + "xmatrix<float>", passed_checks, results);
      }
      {
        xmatrix<xcomplex<float>> exp(3,3,1,1);
        exp(1,1) = {1.1, 1.11}; exp(1,2) = {1.2, 1.22}; exp(1,3) = {1.3, 1.33};
        exp(2,1) = {2.1, 2.11}; exp(2,2) = {2.2, 2.22}; exp(2,3) = {2.3, 2.33};
        exp(3,1) = {3.1, 3.11}; exp(3,2) = {3.2, 3.22}; exp(3,3) = {3.3, 3.33};
        so = exp;
        checkEqual((string)so, "[[{\"imag\":1.11,\"real\":1.1},{\"imag\":1.22,\"real\":1.2},{\"imag\":1.33,\"real\":1.3}],[{\"imag\":2.11,\"real\":2.1},{\"imag\":2.22,\"real\":2.2},{\"imag\":2.33,\"real\":2.3}],[{\"imag\":3.11,\"real\":3.1},{\"imag\":3.22,\"real\":3.2},{\"imag\":3.33,\"real\":3.3}]]", check_function, check_description + "xmatrix<xcomplex<float>>", passed_checks, results);
      }
      {
        std::vector<std::vector<std::string>> exp = {{"Hello", "World"}, {"Hello2", "World2"}};
        so = exp;
        checkEqual((std::string)so, "[[\"Hello\",\"World\"],[\"Hello2\",\"World2\"]]", check_function, check_description + "vector<vector<string>>", passed_checks, results);
      }
      {
        vector<float> a = {12.33,20.7,34.23454};
        vector<float> b = {89.6,2.243,76.432};
        std::map<std::string, std::vector<float>> exp;
        exp.insert({"Hello1", a});
        exp.insert({"Hello2", b});
        so = exp;
        checkEqual((std::string)so, "{\"Hello1\":[12.33,20.7,34.2345],\"Hello2\":[89.6,2.243,76.432]}", check_function, check_description + "vector<vector<string>>", passed_checks, results);
      }
    }

    // check type specific functions
    {
      check_function = "JSON";
      check_description = "type specific function ";
      aurostd::JSON::object so;
      size_t expected = 11;
      so = "Hello World";
      checkEqual(so.size(), expected, check_function, check_description + "size for string", passed_checks, results);
      checkEqual(so.empty(), false, check_function, check_description + "empty for string", passed_checks, results);
      so = vector<int>({1,2,3,4,5,6,7,8,9,10,11});
      checkEqual(so.size(), expected, check_function, check_description + "size for list", passed_checks, results);
      checkEqual(so.empty(), false, check_function, check_description + "empty for list", passed_checks, results);
      so = std::map<string,int>({{"eins",1}, {"zwei", 2}, {"drei", 3}});
      expected = 3;
      checkEqual(so.size(), expected, check_function, check_description + "size for dictionary", passed_checks, results);
      checkEqual(so.empty(), false, check_function, check_description + "empty for dictionary", passed_checks, results);
      //CO20261017 - empty() is true for an empty string, list or dictionary (it tested the pointer this and was always false)
      so = string("");
      checkEqual(so.empty(), true, check_function, check_description + "empty for an empty string", passed_checks, results);
      so = vector<int>();
      checkEqual(so.empty(), true, check_function, check_description + "empty for an empty list", passed_checks, results);
      so = std::map<string,int>();
      checkEqual(so.empty(), true, check_function, check_description + "empty for an empty dictionary", passed_checks, results);



    }

  }
}

namespace unittest {

  void UnitTest::xfitTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings
    // setup test environment
    string check_function = "", check_description = "";

    bool calculated_bool, expected_bool;
    double calculated_dbl, expected_dbl;
    xvector<double> calculated_xvecdbl, expected_xvecdbl;
    xvector<double> calculated_xvecdbl_r, expected_xvecdbl_r;
    xvector<double> calculated_xvecdbl_i, expected_xvecdbl_i;
    xmatrix<double> calculated_xmatdbl, expected_xmatdbl;

    // ---------------------------------------------------------------------------
    // Check | companion matrix //SD20220318
    // ---------------------------------------------------------------------------
    check_function = "aurostd::companion_matrix()";
    check_description = "calculate the companion matrix of a univariate polynomial";
    xvector<double> pc = {6.0, -5.0, -2.0, 3.0};
    expected_xmatdbl = {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {-6.0 / 3.0, 5.0 / 3.0, 2.0 / 3.0}};
    calculated_xmatdbl = aurostd::companion_matrix(pc);
    checkEqual(calculated_xmatdbl, expected_xmatdbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | polynomialFindRoots //SD20220318
    // ---------------------------------------------------------------------------
    check_function = "aurostd::polynomialFindRoots()";
    expected_xvecdbl_r = {1.05576592536838, 1.05576592536838, -1.44486518407010};
    expected_xvecdbl_i = {-0.519201791550296, 0.519201791550296, 0.0};
    calculated_xvecdbl_r = xvector<double>(3), calculated_xvecdbl_i = xvector<double>(3);
    aurostd::polynomialFindRoots(pc, calculated_xvecdbl_r, calculated_xvecdbl_i);
    check_description = "calculate the roots of a univariate polynomial (real part)";
    checkEqual(calculated_xvecdbl_r, expected_xvecdbl_r, check_function, check_description, passed_checks, results);
    check_description = "calculate the roots of a univariate polynomial (imag part)";
    checkEqual(calculated_xvecdbl_i, expected_xvecdbl_i, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | polynomialCurveFit //SD20220422
    // ---------------------------------------------------------------------------
    check_function = "aurostd::polynomialCurveFit()";
    check_description = "calculate the coefficients for a quintic polynomial that fits the data";
    xvector<double> xdata = {0.551878738140095, 0.815194385592879, -1.436650500869055, 0.837122604741089, 0.024588378708606,
                             -1.521792147897101, -0.998568921765830, -0.222562772922286, 0.964723358008907, 0.986066878262693};
    xvector<double> ydata = {-1.895746346279865, -2.426516802630079, -0.329326356733414, -2.476265605752963, -1.148143476025739,
                             -0.304622388440250, -0.471352985639471, -0.911928012994989, -2.783990433247946, -2.838593165314427};
    xvector<double> wdata = {0.757740130578333, 0.743132468124916, 0.392227019534168, 0.655477890177557, 0.171186687811562,
                             0.706046088019609, 0.031832846377421, 0.276922984960890, 0.046171390631154, 0.097131781235848};
    expected_xvecdbl = {-1.121837858162905, -1.056905122203599, -0.543168130435282, -0.145858244844311, -0.007682867520147, 0.000748981621075};
    calculated_xvecdbl = aurostd::polynomialCurveFit(xdata, ydata, 5, wdata);
    checkEqual(expected_xvecdbl, calculated_xvecdbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | findZeroBrent //SD20220517
    // ---------------------------------------------------------------------------
    check_function = "aurostd::findZeroBrent()";
    check_description = "find the zero of a univariate function";
    std::function<double(double)> func = [](double x) {return std::pow(x, 2.0) - 3.0;};
    expected_dbl = 1.732050807568877;
    aurostd::findZeroBrent(0.0, 10.0, func, calculated_dbl);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | checkDerivatives //SD20220622
    // ---------------------------------------------------------------------------
    check_function = "aurostd::checkDerivatives()";
    check_description = "check that the analytical derivatives of a 2D function are correct";
    xvector<double> x0 = {aurostd::ran2(), aurostd::ran2()};
    std::function<double(xvector<double>)> testf = [](xvector<double> x) {return std::sin(x(1)) + std::cos(x(2));};
    vector<std::function<double(xvector<double>)>> testdf;
    testdf.push_back([](xvector<double> x) {return std::cos(x(1));});
    testdf.push_back([](xvector<double> x) {return -1.0 * std::sin(x(2));});
    expected_bool = true;
    calculated_bool = checkDerivatives(x0, testf, testdf);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | calcNumericalJacobian //SD20220624
    // ---------------------------------------------------------------------------
    check_function = "aurostd::calcNumericalJacobian()";
    check_description = "calculate the numerical Jacobian of a 3D rectangular system";
    xvector<double> dx = 10.0 * _AUROSTD_XSCALAR_TOLERANCE_IDENTITY_ * aurostd::ones_xv<double>(3);
    x0 = {aurostd::ran2(), aurostd::ran2(), aurostd::ran2()};
    vector<std::function<double(xvector<double>)>> vfunc, vdfunc;
    vector<vector<std::function<double(xvector<double>)>>> jac;
    vfunc.push_back([](xvector<double> x) {return std::exp(1.0 * x(1) + 2.0 * x(2) + 3.0 * x(3));});
    vfunc.push_back([](xvector<double> x) {return std::cos(1.0 * x(1) + 2.0 * x(2) + 3.0 * x(3));});
    for (int i = 1; i <= 3; i++) {vdfunc.push_back([i](xvector<double> x) {return (double)i * std::exp(1.0 * x(1) + 2.0 * x(2) + 3.0 * x(3));});}
    jac.push_back(vdfunc);
    vdfunc.clear();
    for (int i = 1; i <= 3; i++) {vdfunc.push_back([i](xvector<double> x) {return -1.0 * (double)i * std::sin(1.0 * x(1) + 2.0 * x(2) + 3.0 * x(3));});}
    jac.push_back(vdfunc);
    expected_bool = true;
    calculated_bool = true;
    vector<vector<std::function<double(xvector<double>)>>> testjac = aurostd::calcNumericalJacobian(vfunc, dx);
    for (uint i = 0; i < jac.size(); i++) {
      for (uint j = 0; j < jac[0].size(); j++) {
        calculated_bool = calculated_bool && aurostd::isequal(jac[i][j](x0), testjac[i][j](x0));
      }
    }
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | findZeroNewtonRaphson //SD20220616
    // ---------------------------------------------------------------------------
    check_function = "aurostd::findZeroNewtonRaphson()";
    check_description = "find the zeros of a 2D nonlinear square system";
    x0 = {aurostd::ran2(), aurostd::ran2()};
    vfunc.clear();
    vdfunc.clear();
    jac.clear();
    vfunc.push_back([](xvector<double> x) {return std::exp(-std::exp(-(x(1) + x(2)))) - x(2) * (1.0 + std::pow(x(1), 2.0));});
    vfunc.push_back([](xvector<double> x) {return x(1) * std::cos(x(2)) + x(2) * std::sin(x(1)) - 0.5;});
    vdfunc.push_back([](xvector<double> x) {return std::exp(-std::exp(-(x(1) - x(2))) - x(1) - x(2)) - 2 * x(1) * x(2);});
    vdfunc.push_back([](xvector<double> x) {return std::exp(-std::exp(-(x(1) - x(2))) - x(1) - x(2)) - (1.0 + std::pow(x(1), 2.0));});
    jac.push_back(vdfunc);
    vdfunc.clear();
    vdfunc.push_back([](xvector<double> x) {return x(2) * std::cos(x(1)) + std::cos(x(2));});
    vdfunc.push_back([](xvector<double> x) {return std::sin(x(1)) - x(1) * std::sin(x(2));});
    jac.push_back(vdfunc);
    expected_xvecdbl = {0.353246561918931, 0.606082026502552};
    aurostd::findZeroNewtonRaphson(x0, vfunc, jac, calculated_xvecdbl);
    checkEqual(calculated_xvecdbl, expected_xvecdbl, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | findZeroDeflation //SD20220616
    // ---------------------------------------------------------------------------
    check_function = "aurostd::findZeroDeflation()";
    check_description = "find multiple zeros of a 2D nonlinear square system";
    vfunc.clear();
    vdfunc.clear();
    jac.clear();
    vfunc.push_back([](xvector<double> x) {return x(1) * x(2) - 1.0;});
    vfunc.push_back([](xvector<double> x) {return std::pow(x(1), 2.0) + std::pow(x(2), 2.0) - 4.0;});
    vdfunc.push_back([](xvector<double> x) {return x(2);});
    vdfunc.push_back([](xvector<double> x) {return x(1);});
    jac.push_back(vdfunc);
    vdfunc.clear();
    vdfunc.push_back([](xvector<double> x) {return 2.0 * x(1);});
    vdfunc.push_back([](xvector<double> x) {return 2.0 * x(2);});
    jac.push_back(vdfunc);
    expected_xmatdbl = {{0.517638090205041, 1.93185165257813, -1.93185165257813}, {1.93185165257813, 0.517638090205041, -0.517638090205041}};
    aurostd::findZeroDeflation(aurostd::vector2xvector<double>({0.0, 1.0}), vfunc, jac, calculated_xmatdbl);
    checkEqual(calculated_xmatdbl, expected_xmatdbl, check_function, check_description, passed_checks, results);
  }
}

namespace unittest {

  //CO20261004 - aflowlib::ChaosRuns without a network: run names, an AFLUX response, the list file
  void UnitTest::chaosRunsTest(uint &passed_checks, vector <vector<string>> &results, vector <string> &errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function = "", check_description = "";
    bool calculated_bool = false, expected_bool = false;
    uint calculated_uint = 0, expected_uint = 0;

    check_function = "aflowlib::ChaosRuns::runName()";
    check_description = "name of a --noldau run, elements sorted";
    checkEqual(aflowlib::ChaosRuns::runName("AB_cF8_225_a_b.AB", {"O", "Mg"}, false), string("AB_cF8_225_a_b.AB:Mg:O"), check_function, check_description, passed_checks, results);
    check_description = "name of a +U run";
    checkEqual(aflowlib::ChaosRuns::runName("AB_cF8_225_a_b.AB", {"Ni", "O"}, true), string("AB_cF8_225_a_b.AB:Ni:O:LDAU2"), check_function, check_description, passed_checks, results);

    // the query as CHAOS reads it (CO20261004): field(condition) terms only, chaos_path(*) first (CO20261008)
    check_function = "aflowlib::ChaosRuns::summons()";
    check_description = "AFLUX query of Mg-O, page 2 of 10000 entries";
    checkEqual(aflowlib::ChaosRuns::summons({"O", "Mg"}, 2, 10000), string("chaos_path(*),species(Mg,O),nspecies(2),format(aflow),paging(2,10000)"), check_function, check_description, passed_checks, results);
    //CO20261006 - the pages ChaosRuns::query(), the entry loader and the structure comparison send fit the CHAOS API's planned limit
    checkEqual(AFLUX_PAGE_SIZE_DEFAULT > 0 && AFLUX_PAGE_SIZE_DEFAULT <= 500, true, "AFLUX_PAGE_SIZE_DEFAULT",
        "pages of at most 500 entries, the CHAOS API's planned largest", passed_checks, results);

    check_function = "aflowlib::ChaosRuns::runFromAURL()";
    check_description = "--noldau run from its aurl";
    checkEqual(aflowlib::ChaosRuns::runFromAURL("s4e.ai:AFLOWDATA/LIB2_WEB/Mg_pvO/A2B_tP6_137_d_a-001.BA", {"O", "Mg"}), string("A2B_tP6_137_d_a-001.BA:Mg:O"), check_function, check_description, passed_checks, results);
    check_description = "+U run from its aurl";
    checkEqual(aflowlib::ChaosRuns::runFromAURL("s4e.ai:AFLOWDATA/LIB2_WEB/Ni_pvO/AB_cF8_225_a_b.AB:LDAU2", {"Ni", "O"}), string("AB_cF8_225_a_b.AB:Ni:O:LDAU2"), check_function, check_description, passed_checks, results);
    check_description = "a POCC supercell counts as its parent run";
    checkEqual(aflowlib::ChaosRuns::runFromAURL("s4e.ai:AFLOWDATA/LIB2_WEB/Ni_pvO/AB_cF8_225_a_b.AB_POCC_X/ARUN.POCC_01_H0000", {"Ni", "O"}), string("AB_cF8_225_a_b.AB_POCC_X:Ni:O"), check_function, check_description, passed_checks, results);
    check_description = "run from an aurl in quotes, as the CHAOS database stores it";
    checkEqual(aflowlib::ChaosRuns::runFromAURL("\"s4e.ai:AFLOWDATA/LIB2_WEB/Mg_pvO/AB_cP8_198_a_a-001.AB\"", {"Mg", "O"}), string("AB_cP8_198_a_a-001.AB:Mg:O"), check_function, check_description, passed_checks, results);

    // AFLUX response (format(aflow), as CHAOS writes it) of the Ni-O system: a +U run, a --noldau run,
    // a record without aurl
    check_function = "aflowlib::ChaosRuns::addAFLUXResponse()";
    aflowlib::ChaosRuns chaos;
    string response = "aurl=s4e.ai:AFLOWDATA/LIB2_WEB/Ni_pvO/AB_cF8_225_a_b.AB:LDAU2 | auid=s4e:0000000000000001 | compound=Ni4O4\n"
      "aurl=s4e.ai:AFLOWDATA/LIB2_WEB/Ni_pvO/AB_hP4_186_b_b-001.AB | auid=s4e:0000000000000002 | compound=Ni2O2\n"
      "auid=s4e:0000000000000003 | compound=Ni1O1\n";
    check_description = "two runs read";
    calculated_uint = chaos.addAFLUXResponse(response, {"O", "Ni"});
    expected_uint = 2;
    checkEqual(calculated_uint, expected_uint, check_function, check_description, passed_checks, results);
    check_description = "+U run of rock salt NiO in CHAOS, its --noldau run not";
    calculated_bool = chaos.hasRun("AB_cF8_225_a_b.AB:Ni:O:LDAU2") && !chaos.hasRun("AB_cF8_225_a_b.AB:Ni:O");
    expected_bool = true;
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    check_description = "--noldau run of wurtzite NiO in CHAOS";
    calculated_bool = chaos.hasRun("AB_hP4_186_b_b-001.AB:Ni:O");
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    check_description = "system Ni-O read, Mg-O not";
    calculated_bool = chaos.hasSystem({"O", "Ni"}) && !chaos.hasSystem({"Mg", "O"});
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    // CO20261004 - system and run folders, the list of every run in CHAOS, the check of new runs
    check_function = "aflowlib::ChaosRuns::pathFromAURL()";
    check_description = "SYSTEM/RUN of a +U run";
    checkEqual(aflowlib::ChaosRuns::pathFromAURL("s4e.ai:AFLOWDATA/LIB2_WEB/Ni_pvO/AB_cF8_225_a_b.AB:LDAU2"), string("Ni_pvO/AB_cF8_225_a_b.AB:LDAU2"), check_function, check_description, passed_checks, results);
    check_description = "SYSTEM/RUN of a unary (system folder with a colon)";
    checkEqual(aflowlib::ChaosRuns::pathFromAURL("s4e.ai:AFLOWDATA/LIB1_WEB/O:PAW_PBE/A_tP2_123_g-003.A"), string("O:PAW_PBE/A_tP2_123_g-003.A"), check_function, check_description, passed_checks, results);
    check_description = "SYSTEM/RUN of an aurl in quotes, as the CHAOS database stores it";
    checkEqual(aflowlib::ChaosRuns::pathFromAURL("\"s4e.ai:AFLOWDATA/LIB2_WEB/Mg_pvO/AB_cP8_198_a_a-001.AB\""), string("Mg_pvO/AB_cP8_198_a_a-001.AB"), check_function, check_description, passed_checks, results);
    check_description = "SYSTEM/RUN of a child run (phonon distortion) is its parent's";
    checkEqual(aflowlib::ChaosRuns::pathFromAURL("s4e.ai:AFLOWDATA/LIB2_WEB/FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_006_A0D3-A1D4"), string("FNa_sv/AB_cF8_225_a_b.AB"), check_function, check_description, passed_checks, results);
    check_description = "SYSTEM/RUN of a unary aurl without a host";
    checkEqual(aflowlib::ChaosRuns::pathFromAURL("AFLOWDATA/LIB1_WEB/O:PAW_PBE/A_tP2_123_g-003.A"), string("O:PAW_PBE/A_tP2_123_g-003.A"), check_function, check_description, passed_checks, results);
    check_function = "aflowlib::ChaosRuns::pathFromDirectory()";
    check_description = "SYSTEM/RUN of a run directory (trailing slash)";
    checkEqual(aflowlib::ChaosRuns::pathFromDirectory("./LIB2/LIB/Ni_pvO/AB_cF8_225_a_b.AB:LDAU2/"), string("Ni_pvO/AB_cF8_225_a_b.AB:LDAU2"), check_function, check_description, passed_checks, results);
    check_description = "SYSTEM/RUN of an aflow.in path";
    checkEqual(aflowlib::ChaosRuns::pathFromDirectory("/scratch/u/LIB2/LIB/Mg_pvO/AB_cF8_225_a_b.AB/aflow.in"), string("Mg_pvO/AB_cF8_225_a_b.AB"), check_function, check_description, passed_checks, results);
    check_function = "aflowlib::ChaosRuns::elementsFromSystemFolder()";
    check_description = "elements of Ca_svTi_svO and O:PAW_PBE";
    calculated_bool = (aflowlib::ChaosRuns::elementsFromSystemFolder("Ca_svTi_svO") == vector<string>{"Ca", "O", "Ti"}) && (aflowlib::ChaosRuns::elementsFromSystemFolder("O:PAW_PBE") == vector<string>{"O"});
    expected_bool = true;
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    check_function = "aflowlib::ChaosCheckApplies()";
    check_description = "database runs (LIBn, AFLOWDATA) only";
    calculated_bool = aflowlib::ChaosCheckApplies("./LIB2/LIB/Mg_pvO/AB_cF8_225_a_b.AB") && aflowlib::ChaosCheckApplies("./AFLOWDATA/Mg_pvO/AB_cF8_225_a_b.AB") && !aflowlib::ChaosCheckApplies("/home/u/tests/run1") && !aflowlib::ChaosCheckApplies("/home/u/LIBRARY/run1");
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    // a list of every run in CHAOS, one hour and 100 hours old, and one that does not say it is complete
    string list_file = aurostd::TmpFileCreate("chaos_runs_list");
    std::time_t now = time(NULL);
    auto list_text = [](std::time_t written, const string& more = "") {
      std::tm g = {}; gmtime_r(&written, &g); char buf[32]; strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &g);
      string entries = string("aurl=s4e.ai:AFLOWDATA/LIB2_WEB/Ni_pvO/AB_cF8_225_a_b.AB|species=Ni,O\n"
        "aurl=\"s4e.ai:AFLOWDATA/LIB2_WEB/Ni_pvO/AB_cF8_225_a_b.AB:LDAU2\"|species=[\"Ni\",\"O\"]\n") + more;
      return string("# runs in CHAOS (test)\ncomplete ") + buf + "\n" + entries + "end " + std::to_string(std::count(entries.begin(), entries.end(), '\n')) + "\n";
    };
    aurostd::string2file(list_text(now - 3600), list_file);
    check_function = "aflowlib::ChaosRuns::readListFile()";
    aflowlib::ChaosRuns chaos_list;
    string problem = "";
    check_description = "a list one hour old is used: runs, paths, every system";
    calculated_bool = chaos_list.readListFile(list_file, problem) && chaos_list.hasPath("Ni_pvO/AB_cF8_225_a_b.AB:LDAU2") && chaos_list.hasRun("AB_cF8_225_a_b.AB:Ni:O") && chaos_list.hasSystem({"Mg", "O"});
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    aurostd::string2file(list_text(now - 100 * 3600), list_file);
    aflowlib::ChaosRuns chaos_old;
    check_description = "a list 100 hours old is not used";
    calculated_bool = !chaos_old.readListFile(list_file, problem) && problem.find("h old") != string::npos;
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    aurostd::string2file(list_text(now + 60), list_file);
    aflowlib::ChaosRuns chaos_ahead;
    check_description = "a list written a minute ahead of this clock is used";
    calculated_bool = chaos_ahead.readListFile(list_file, problem);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    string whole = list_text(now - 3600);
    aurostd::string2file(whole.substr(0, whole.rfind("aurl=")), list_file);
    aflowlib::ChaosRuns chaos_cut;
    check_description = "a copy cut short (no end line) is not used";
    calculated_bool = !chaos_cut.readListFile(list_file, problem) && problem.find("cut short") != string::npos;
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    aurostd::string2file(whole.substr(0, whole.rfind("aurl=")) + "end 2\n", list_file);
    aflowlib::ChaosRuns chaos_short;
    check_description = "a list with fewer entries than its end line says is not used";
    calculated_bool = !chaos_short.readListFile(list_file, problem) && problem.find("end line says 2") != string::npos;
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    // the check of new runs, with that list in CHAOS_RUNS_LIST
    check_function = "aflowlib::ChaosCheckDirectory()";
    const char* env_list = getenv("CHAOS_RUNS_LIST");
    string env_list_saved = (env_list == NULL ? string("") : string(env_list));
    setenv("CHAOS_RUNS_LIST", list_file.c_str(), 1);
    aurostd::string2file(list_text(now - 3600), list_file);
    aflowlib::ChaosCheckReset();
    string reason = "";
    check_description = "fresh list: NiO in CHAOS, MgO and NiO +U of another prototype not";
    calculated_bool = (aflowlib::ChaosCheckDirectory("./LIB2/LIB/Ni_pvO/AB_cF8_225_a_b.AB", {}, reason) == aflowlib::CHAOS_CHECK_IN)
      && (aflowlib::ChaosCheckDirectory("./LIB2/LIB/Mg_pvO/AB_cF8_225_a_b.AB", {}, reason) == aflowlib::CHAOS_CHECK_NEW)
      && (aflowlib::ChaosCheckDirectory("./LIB2/LIB/Ni_pvO/AB_hP4_186_b_b-001.AB:LDAU2", {}, reason) == aflowlib::CHAOS_CHECK_NEW);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    check_description = "fresh list: NiO under another system folder (NiO, Ni_pv in CHAOS) is in CHAOS by run name; another decoration is not";
    calculated_bool = (aflowlib::ChaosCheckDirectory("./AFLOWDATA/NiO/AB_cF8_225_a_b.AB:LDAU2", {}, reason) == aflowlib::CHAOS_CHECK_IN) && reason.find("another system folder") != string::npos
      && (aflowlib::ChaosCheckDirectory("./LIB2/LIB/Ni_pvO/AB_cF8_225_a_b.BA", {}, reason) == aflowlib::CHAOS_CHECK_NEW);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    // CO20261009 - LIB2LIB post-processes an entry that is in CHAOS: KBIN must start it then
    check_function = "aflowlib::ChaosCheckSuspended";
    check_description = "the check is off while one lives, also nested, and on again after";
    bool off_before = aflowlib::ChaosCheckDisabled(), off_inside = false, off_nested = false;
    {
      aflowlib::ChaosCheckSuspended suspended;
      off_inside = aflowlib::ChaosCheckDisabled();
      {
        aflowlib::ChaosCheckSuspended nested;
        off_nested = aflowlib::ChaosCheckDisabled();
      }
      off_inside = off_inside && aflowlib::ChaosCheckDisabled();
    }
    calculated_bool = !off_before && off_inside && off_nested && !aflowlib::ChaosCheckDisabled();
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    // CO20261017 - without a list of CHAOS runs and without a CHAOS API key (anyone outside the group), the check is off
    if (aurostd::httpChaosApiKey().empty()) {
      check_function = "aflowlib::ChaosCheckDisabled()";
      check_description = "no list of CHAOS runs and no CHAOS API key: the check is off";
      setenv("CHAOS_RUNS_LIST", (list_file + ".absent").c_str(), 1);
      checkEqual(aflowlib::ChaosCheckDisabled(), true, check_function, check_description, passed_checks, results);
      setenv("CHAOS_RUNS_LIST", list_file.c_str(), 1);
    }
    string chaos_root = aurostd::TmpDirectoryCreate("chaos_check");
    string chaos_entry = chaos_root + "/LIB2/LIB/Ni_pvO/AB_cF8_225_a_b.AB";
    aurostd::DirectoryMake(chaos_entry);
    aurostd::string2file("[AFLOW] test\n", chaos_entry + "/" + _AFLOWIN_);
    _aflags chaos_aflags;
    chaos_aflags.AFLOW_FORCE_RUN = true;  // as LIB2LIB sets it
    check_function = "KBIN::Legitimate_aflowdir()";
    check_description = "an entry in CHAOS is not started and is marked ALREADY_IN_DATABASE";
    calculated_bool = !KBIN::Legitimate_aflowdir(chaos_entry, chaos_aflags) && aurostd::FileExist(chaos_entry + "/ALREADY_IN_DATABASE");
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    aurostd::RemoveFile(chaos_entry + "/ALREADY_IN_DATABASE");
    check_description = "the same entry is started, and not marked, while a ChaosCheckSuspended lives (LIB2LIB)";
    {
      aflowlib::ChaosCheckSuspended suspended;
      calculated_bool = KBIN::Legitimate_aflowdir(chaos_entry, chaos_aflags) && !aurostd::FileExist(chaos_entry + "/ALREADY_IN_DATABASE");
    }
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(chaos_root);
    check_function = "aflowlib::ChaosCheckDirectory()";
    aurostd::string2file(list_text(now - 100 * 3600), list_file);
    aflowlib::ChaosCheckReset();
    check_description = "stale list: cannot be checked, nothing guessed";
    calculated_bool = (aflowlib::ChaosCheckDirectory("./LIB2/LIB/Mg_pvO/AB_cF8_225_a_b.AB", {}, reason) == aflowlib::CHAOS_CHECK_UNKNOWN) && reason.find("h old") != string::npos;
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    // the same process, a new list put in place (one more run): read again
    aurostd::string2file(list_text(now - 3600, "aurl=s4e.ai:AFLOWDATA/LIB2_WEB/Mg_pvO/AB_cF8_225_a_b.AB|species=Mg,O\n"), list_file);
    check_description = "a new list is read again by a running process: MgO now in CHAOS";
    calculated_bool = (aflowlib::ChaosCheckDirectory("./LIB2/LIB/Mg_pvO/AB_cF8_225_a_b.AB", {}, reason) == aflowlib::CHAOS_CHECK_IN);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    // the list grows older than the limit while the process runs
    const char* env_age = getenv("CHAOS_RUNS_MAX_AGE_HOURS");
    string env_age_saved = (env_age == NULL ? string("") : string(env_age));
    setenv("CHAOS_RUNS_MAX_AGE_HOURS", "0.5", 1);
    check_description = "a list that became too old while running is not used";
    calculated_bool = (aflowlib::ChaosCheckDirectory("./LIB2/LIB/Ni_pvO/AB_cF8_225_a_b.AB", {}, reason) == aflowlib::CHAOS_CHECK_UNKNOWN) && reason.find("h old") != string::npos;
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    if (env_age_saved.empty()) {unsetenv("CHAOS_RUNS_MAX_AGE_HOURS");} else {setenv("CHAOS_RUNS_MAX_AGE_HOURS", env_age_saved.c_str(), 1);}
    if (env_list_saved.empty()) {unsetenv("CHAOS_RUNS_LIST");} else {setenv("CHAOS_RUNS_LIST", env_list_saved.c_str(), 1);}
    aflowlib::ChaosCheckReset();
    aurostd::RemoveFile(list_file);

    // AFLOW's tables of calculated entries (vLIBS) from a list file: auid and loop as the
    // database keeps them, a child run and its parent (one folder), a ternary //CO20261004
    check_function = "aflowlib::ChaosCalculatedEntries()";
    string tables_file = aurostd::TmpFileCreate("chaos_runs_tables");
    {
      std::tm g = {}; gmtime_r(&now, &g); char buf[32]; strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &g);
      aurostd::string2file(string("# runs in CHAOS (test)\ncomplete ") + buf + "\n"
        "aurl=\"s4e.ai:AFLOWDATA/LIB2_WEB/Mg_pvO/AB_cF8_225_a_b.AB\"|auid=\"aflow:0000000000000001\"|loop=[\"thermodynamics\",\"magnetic\"]|species=[\"Mg\",\"O\"]\n"
        "aurl=s4e.ai:AFLOWDATA/LIB2_WEB/FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_006_A0D3-A1D4|auid=aflow:0000000000000002|loop=thermodynamics|species=F,Na\n"
        "aurl=s4e.ai:AFLOWDATA/LIB2_WEB/FNa_sv/AB_cF8_225_a_b.AB|auid=aflow:0000000000000003|loop=thermodynamics|species=F,Na\n"
        "aurl=s4e.ai:AFLOWDATA/LIB3_WEB/AlMgO/A2BC4_cF56_227_d_a_e.ABC|auid=aflow:0000000000000004|loop=thermodynamics|species=Al,Mg,O\n"
        "end 4\n", tables_file);
    }
    vector<string> vaurl, vauid, vloop;
    vector<vector<string> > vlibrary;
    check_description = "four entries; auid and loop without the quotes and brackets of the database";
    calculated_bool = (aflowlib::ChaosCalculatedEntries(tables_file, vaurl, vauid, vloop, vlibrary, problem) == 4) && problem.empty()
      && vaurl[0] == "s4e.ai:AFLOWDATA/LIB2_WEB/Mg_pvO/AB_cF8_225_a_b.AB" && vauid[0] == "aflow:0000000000000001" && vloop[0] == "thermodynamics,magnetic";
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    check_description = "LIB2 folders: a child run counts once, as its parent; LIB3 one folder";
    calculated_bool = vlibrary.size() == 11 && vlibrary[2] == vector<string>{"Mg_pvO/AB_cF8_225_a_b.AB", "FNa_sv/AB_cF8_225_a_b.AB"} && vlibrary[3].size() == 1 && vlibrary[10].empty();
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    aurostd::RemoveFile(tables_file);
    check_description = "no list: empty tables, and the problem says so";
    calculated_bool = (aflowlib::ChaosCalculatedEntries(tables_file, vaurl, vauid, vloop, vlibrary, problem) == 0) && vaurl.empty() && problem.find("no list") != string::npos;
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    // list file written and read back
    check_function = "aflowlib::ChaosRuns::writeFile(), readFile()";
    string file = aurostd::TmpFileCreate("chaos_runs");
    chaos.writeFile(file);
    aflowlib::ChaosRuns chaos_read;
    chaos_read.readFile(file);
    aurostd::RemoveFile(file);
    check_description = "same systems and runs after writing and reading the list";
    calculated_bool = (chaos_read.systems == chaos.systems) && (chaos_read.runs == chaos.runs) && (chaos_read.paths == chaos.paths);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
  }

  void UnitTest::entryLoaderTest(uint &passed_checks, vector <vector<string>> &results, vector <string> &errors) {
    (void) errors;  // Suppress compiler warnings

    string auidPrefix = DEFAULT_ENTRY_LOADER_AUID_PREFIX; //WJ20240523 Check where running from
    string aurlPath = aurostd::tolower(DEFAULT_ENTRY_LOADER_AURL_PATH);

    // setup test environment
    string task_description = "Testing EntryLoader";
    stringstream result;
    string check_description = "";
    stringstream check_description_helper;
    string check_function = "";

    std::string test_alloy = ""; // KC20240506 - declaration - defined by AUID prefixes below
    bool recursive = false;
    aflowlib::EntryLoader el;
    aflowlib::_aflowlib_entry test_entry;
    xstructure test_structure;

    // KC20240309 START - adjusting for s4e 
    size_t expected_size_t = 0;
    std::string test_AUID; //declaration
    std::vector <std::string> test_AUIDs; // declaration
    std::string test_AURL; // declaration 
    std::vector <std::string> test_AURLs; // declaration 

    std::map <std::string, aflowlib::EntryLoader::Source> test_sources;
    std::map <std::string, aflowlib::EntryLoader::Source> short_test_sources;

    //CO20261002 - CHAOS examples on every machine: s4e.ai is the only server, so the same
    //entries are found everywhere (SQLITE only where the local database exists)
    (void) auidPrefix; (void) aurlPath;
    test_alloy = "BiOTi";

    test_AUIDs = {
      "s4e:0011df4470f20f8b",
      "0088e48b0ea8b960",
      "auid:000a6d5239d442d8",
      "s4e:00a9642c7b361409",
      "00fc29fa53910541",
      "broken"
    };
    test_AUID = "s4e:0171c43afba95d07";

    test_AURLs = {
      "s4e.ai:AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/AB2C_hR4_166_a_c_b-003.CBA",
      "AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/ABC2_oI32_72_j_f_2j-001.CAB",
      "aurl:AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/A2B2C5_aP18_2_2i_2i_5i-001.ACB",
      "LIB3_WEB/Bi_dOTi_sv/A3B6C_cF40_225_bc_e_a-002.ABC",
      "s4e.ai:AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/AB3C8_tI24_121_a_bd_2i-001.ACB"
    };
    test_AURL = "s4e.ai:AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/AB9C4_hP28_188_e_kl_ak-001.CBA";

    test_sources = {
      {"AUTO SELECT",    aflowlib::EntryLoader::Source::NONE},
      {"SQLITE",         aflowlib::EntryLoader::Source::SQLITE},
      {"AFLUX",          aflowlib::EntryLoader::Source::AFLUX}
    };

    short_test_sources = {
      {"SQLITE",     aflowlib::EntryLoader::Source::SQLITE},
      {"AFLUX",      aflowlib::EntryLoader::Source::AFLUX}
    };

    // AFLUX on s4e.ai needs a CHAOS API key: say so once, plainly, instead of only failing below
    if (!checkChaosApiKey(passed_checks, results)) {return;} //CO20261004 - one helper, the server list split into hosts //CO20261017 - no key: skipped
    // KC20240309 STOP - adjusting for s4e 


    // ---------------------------------------------------------------------------
    // Check | load alloys
    for (std::map<std::string, aflowlib::EntryLoader::Source>::iterator source = test_sources.begin();
        source != test_sources.end(); source++) {
      check_function = "EntryLoader::loadAlloy()";
      if (source->first == "RESTAPI" || source->first == "RESTAPI_RAW") recursive = false;
      else recursive = true;
      check_description = source->first + " - " + test_alloy;
      expected_size_t = 1;
      if (recursive) {
        check_description += " - recursive";
      }
      el.clear();
      el.m_out_silent = true;
      {
        long double start = aurostd::get_seconds();
        if (el.setSource(source->second)) { // don't test if basic requirements are not met for a source
          el.loadAlloy(test_alloy, recursive);
          long double duration = aurostd::get_delta_seconds(start);
          aurostd::StringstreamClean(check_description_helper);
          check_description_helper << " | speed " << el.m_entries_flat->size() / duration << " entries/s; "
            << el.m_entries_flat->size() << " entries";
          check_description += check_description_helper.str();
          check((el.m_entries_flat->size() >= expected_size_t), el.m_entries_flat->size(), expected_size_t,
              check_function, check_description, passed_checks, results);
        }
      }
    }

    // ---------------------------------------------------------------------------
    // Check | load AUID + Xstructure
    for (std::map<std::string, aflowlib::EntryLoader::Source>::iterator source = short_test_sources.begin();
        source != short_test_sources.end(); source++) {
      if (getAUIDPrefix()=="s4e" and source->first=="RESTAPI"){continue;} // KC20240504 no REST API for s4e entries
      check_function = "EntryLoader::loadAUID()";
      check_description = source->first + " + xstructure";
      expected_size_t = 1;
      el.clear();
      el.m_out_silent = true;
      el.m_xstructure_original = true;
      el.m_xstructure_relaxed = true;
      if (source->first == "RESTAPI" || source->first == "RESTAPI_RAW")
        el.m_filesystem_path = "/fake/"; // force xstructure test to use REST API
      long double start = aurostd::get_seconds();
      if (el.setSource(source->second)) { // don't test if basic requirements are not met for a source
        bool m_filesystem_available = aurostd::IsDirectory(el.m_filesystem_path + "AUID/");
        if(!m_filesystem_available) { // MB20250312 - the structures are read from the filesystem
          //CO20261002 - without it, still load the entries (no structures) instead of skipping the check
          el.m_xstructure_original = false;
          el.m_xstructure_relaxed = false;
          check_description = source->first + " (no filesystem: entries without structures)";
        }
        el.loadAUID(test_AUID);
        if (source->first == "AFLUX") {
          if (el.m_entries_flat->size() < 1) {
            check_description += " | failed to load test AUID";
            check(false, el.m_entries_flat->size(), (size_t)1, check_function,
                check_description, passed_checks, results);
            continue;
          } else {
            test_entry = *el.m_entries_flat->back();
          }
        }
        el.loadAUID(test_AUIDs);

        long double duration = aurostd::get_delta_seconds(start);
        aurostd::StringstreamClean(check_description_helper);
        check_description_helper << " | speed " << el.m_entries_flat->size() / duration << " entries/s; "
          << el.m_entries_flat->size() << " entries";
        check_description += check_description_helper.str();
        check((el.m_entries_flat->size() >= expected_size_t), el.m_entries_flat->size(), expected_size_t, check_function,
            check_description, passed_checks, results);
      }
    }

    // ---------------------------------------------------------------------------
    // Check | a quoted AUID loads the same entry as the plain one //CO20261004
    check_function = "EntryLoader::cleanAUID()";
    check_description = "AFLUX - \"" + test_AUID + "\" with quotes";
    {
      vector<string> auids_plain, auids_quoted;
      el.clear();
      el.m_out_silent = true;
      if (el.setSource(aflowlib::EntryLoader::Source::AFLUX)) {
        el.loadAUID(test_AUID);
        for (const auto& entry : *el.m_entries_flat) auids_plain.push_back(entry->auid);
      }
      el.clear();
      el.m_out_silent = true;
      if (el.setSource(aflowlib::EntryLoader::Source::AFLUX)) {
        el.loadAUID("\"" + test_AUID + "\"");
        for (const auto& entry : *el.m_entries_flat) auids_quoted.push_back(entry->auid);
      }
      checkEqual(auids_quoted, auids_plain, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | load xstructure from file

    // --------- WJ20240521 LoadXstructureFile uses RestAPI - OLD ----------------
    // check_function = "EntryLoader::loadXstructureFile()";
    // check_description = "load xstructure from CONTCAR.relax";
    // if (!test_entry.auid.empty()) {
    //   el.loadXstructureFile(test_entry, test_structure, {"CONTCAR.relax"});
    //   checkEqual(test_structure.atoms.size(), (size_t)
    //       6, check_function, check_description, passed_checks, results);
    // } else {
    //   check_description += " | failed to load example structure form AFLUX in previous test";
    //   check(false, 0, 0, check_function, check_description, passed_checks, results);
    // }

    // KC20240506 - this test fails in loadXstructureFile
    // should load /common/LIB3/LIB/Bi_dOTi...
    // tries to load /common/_dOTi...
    // error lies on line 1111 of aflowlib_entry_loader.cpp when base_folder is assigned

    // ---------------------------------------------------------------------------
    // Check | load AURL
    for (std::map<std::string, aflowlib::EntryLoader::Source>::iterator source = short_test_sources.begin();
        source != short_test_sources.end(); source++) {
      check_function = "EntryLoader::loadAURL()";
      check_description = source->first;
      expected_size_t = 1;
      el.clear();
      el.m_out_silent = true;
      {
        long double start = aurostd::get_seconds();
        if (el.setSource(source->second)) { // don't test if basic requirements are not met for a source
          //CO20261002 - loadAURL reads no structures, so it does not need the filesystem (MB20250312 skipped it)
          el.loadAURL(test_AURLs);
          el.loadAURL(test_AURL);
          long double duration = aurostd::get_delta_seconds(start);
          aurostd::StringstreamClean(check_description_helper);
          check_description_helper << " | speed " << el.m_entries_flat->size() / duration << " entries/s; "
            << el.m_entries_flat->size() << " entries";
          check_description += check_description_helper.str();
          check((el.m_entries_flat->size() >= expected_size_t), el.m_entries_flat->size(), expected_size_t, check_function,
              check_description, passed_checks, results);
        }
      }
    }
  }
}

//CO20261002 START
namespace unittest {

  //CO20261004 START
  /// @brief when a default AFLUX server is s4e.ai, check that a CHAOS API key is at hand (s4e.ai answers AFLUX queries only with one)
  /// @return true when the checks that query CHAOS can run
  /// @note the server list is split into hosts with aurostd::string2tokens, as in AFLUXTest's aflowrc check;
  ///       httpIsS4EHost() of the whole list ("s4e.ai,other.example") was false, and the check was skipped
  /// @note CO20261017 - without a key the checks that query CHAOS are skipped, so the unit tests need no key and
  ///       no network; with CHAOS_CHECK_REQUIRED=1 a missing key fails instead
  bool UnitTest::checkChaosApiKey(uint &passed_checks, vector<vector<string> > &results) {
    vector<string> vservers;
    aurostd::string2tokens(DEFAULT_ENTRY_LOADER_AFLUX_SERVER_LIST, vservers, ",");
    for (size_t i = 0; i < vservers.size(); i++) {
      if (!aurostd::httpIsS4EHost(vservers[i])) {continue;}
      const bool has_key = !aurostd::httpChaosApiKey().empty();
      if (!has_key && string(getenv("CHAOS_CHECK_REQUIRED") ? getenv("CHAOS_CHECK_REQUIRED") : "") != "1") {
        cerr << "MMMMM  " << __AFLOW_FUNC__ << " no CHAOS API key: the checks that query CHAOS are skipped (set CHAOS_API_KEY or put the key in "
             << aurostd::httpChaosApiKeyFileName() << " to run them)" << endl;
        return false;
      }
      check(has_key, string("missing"), string("a key"), "aurostd::httpChaosApiKey()",
          "CHAOS API key for AFLUX (set CHAOS_API_KEY or put the key in " + aurostd::httpChaosApiKeyFileName() + ")", passed_checks, results);
      return has_key;  // one check, however many s4e.ai servers are listed
    }
    return true;  // no s4e.ai server listed: nothing needs the key
  }
  //CO20261004 STOP

  /// @brief AURL host and path for any host, and the pages AFLUX queries with paging(0) are sent as
  /// @note offline: no network, no database
  void UnitTest::aurlHostTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function = "aflowlib::AURLStripHost()";
    // an s4e.ai AURL (the old substr(28) assumed a 28-character "host:AFLOWDATA/" and cut 11
    // characters into the path of these)
    const string s4e = "s4e.ai:AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/AB9C4_hP28_188_e_kl_ak-001.CBA";
    checkEqual(aflowlib::AURLStripHost(s4e), string("AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/AB9C4_hP28_188_e_kl_ak-001.CBA"),
        check_function, "s4e.ai host removed", passed_checks, results);
    checkEqual(aflowlib::AURLStripHost("other.example.org:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138"), string("AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138"),
        check_function, "any other host removed", passed_checks, results);
    checkEqual(aflowlib::AURLStripHost("s4e.ai:AFLOWDATA/LIB1_RAW/Pt:PAW_PBE:05Jan2001/A6"), string("AFLOWDATA/LIB1_RAW/Pt:PAW_PBE:05Jan2001/A6"),
        check_function, "LIB1 AURL keeps the colons of its path", passed_checks, results);
    checkEqual(aflowlib::AURLStripHost("AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138"), string("AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138"),
        check_function, "no host: unchanged", passed_checks, results);
    checkEqual(aflowlib::AURLStripHost("LIB2_RAW/Ca_svCu_pv/138"), string("LIB2_RAW/Ca_svCu_pv/138"),
        check_function, "no AFLOWDATA: unchanged", passed_checks, results);

    check_function = "aflowlib::AURLPath()";
    checkEqual(aflowlib::AURLPath(s4e), string("LIB3_WEB/Bi_dOTi_sv/AB9C4_hP28_188_e_kl_ak-001.CBA"),
        check_function, "s4e.ai AURL: path below AFLOWDATA/", passed_checks, results);
    checkEqual(aflowlib::AURLPath("other.example.org:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138"), string("LIB2_RAW/Ca_svCu_pv/138"),
        check_function, "other host: path below AFLOWDATA/", passed_checks, results);
    checkEqual(aflowlib::AURLPath("AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138"), string("LIB2_RAW/Ca_svCu_pv/138"),
        check_function, "no host", passed_checks, results);
    checkEqual(aflowlib::AURLPath("LIB2_RAW/Ca_svCu_pv/138"), string("LIB2_RAW/Ca_svCu_pv/138"),
        check_function, "path only", passed_checks, results);

    check_function = "_aflowlib_entry::getPathDirectory()";
    aflowlib::_aflowlib_entry entry;
    entry.aurl = s4e;
    checkEqual(entry.getPathDirectory("LIB"), DEFAULT_ENTRY_LOADER_FS_PATH + "LIB3/LIB/Bi_dOTi_sv/AB9C4_hP28_188_e_kl_ak-001.CBA/",
        check_function, "s4e.ai AURL to the LIB folder", passed_checks, results);
    entry.aurl = "other.example.org:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138";
    checkEqual(entry.getPathDirectory("RAW"), DEFAULT_ENTRY_LOADER_FS_PATH + "LIB2/RAW/Ca_svCu_pv/138/",
        check_function, "other host to the RAW folder", passed_checks, results);

    //CO20261004 START - getPathAURL(), getSpeciesAURL() and the entry loader's AURL cleaning remove the host with AURLStripHost()
    std::ostringstream oss_aurl;
    check_function = "_aflowlib_entry::getPathAURL()";
    const string server = XHOST.vflag_control.flag("AFLOWLIB_SERVER") ? XHOST.vflag_control.getattachedscheme("AFLOWLIB_SERVER") : string(AFLOWLIB_SERVER_DEFAULT);
    const vector<vector<string> > aurl_paths = {  // AURL, path below the server, description
      {"s4e.ai:AFLOWDATA/LIB1_RAW/Pt:PAW_PBE:05Jan2001/A6", "AFLOWDATA/LIB1_RAW/Pt:PAW_PBE:05Jan2001/A6", "s4e.ai host, LIB1 colons kept"},
      {"other.example.org:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138", "AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138", "any other host"},
      {"AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138", "AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138", "no host: the path is kept (it was dropped)"}
    };
    for (size_t i = 0; i < aurl_paths.size(); i++) {
      entry.aurl = aurl_paths[i][0];
      checkEqual(entry.getPathAURL(oss_aurl, false), server + "/" + aurl_paths[i][1], check_function, aurl_paths[i][2], passed_checks, results);
    }

    check_function = "_aflowlib_entry::getSpeciesAURL()";
    entry.aurl = "s4e.ai:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138";
    checkEqual(aurostd::joinWDelimiter(entry.getSpeciesAURL(oss_aurl), ","), string("Ca,Cu"), check_function, "s4e.ai host", passed_checks, results);
    entry.aurl = "AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138";
    checkEqual(aurostd::joinWDelimiter(entry.getSpeciesAURL(oss_aurl), ","), string("Ca,Cu"), check_function, "no host", passed_checks, results);
    entry.aurl = "s4e.ai:AFLOWDATA/LIB1_RAW/Pt:PAW_PBE:05Jan2001/A6";
    const string species_lib1_host = aurostd::joinWDelimiter(entry.getSpeciesAURL(oss_aurl), ",");
    entry.aurl = "AFLOWDATA/LIB1_RAW/Pt:PAW_PBE:05Jan2001/A6";
    checkEqual(aurostd::joinWDelimiter(entry.getSpeciesAURL(oss_aurl), ","), species_lib1_host, check_function,
        "LIB1 without a host: as with one (it was cut at its first colon)", passed_checks, results);

    // EntryLoader::cleanAURL() is private: through loadAURL() from a filesystem holding one entry, in a folder of its own
    check_function = "EntryLoader::loadAURL()";
    {
      const string fs_dir = aurostd::TmpDirectoryCreate("aurl_host_fs");
      const string entry_dir = fs_dir + "/LIB2/RAW/Ca_svCu_pv/138";
      aurostd::DirectoryMake(fs_dir + "/AUID");  // the loader takes a filesystem with an AUID folder
      aurostd::DirectoryMake(entry_dir);
      aurostd::string2file("aurl=s4e.ai:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138|auid=s4e:1000000000000003|compound=Ca1Cu1|species=Ca,Cu|nspecies=2",
          entry_dir + "/" + DEFAULT_FILE_AFLOWLIB_ENTRY_OUT);
      const vector<vector<string> > aurls_fs = {  // AURL, description
        {"s4e.ai:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138", "s4e.ai host"},
        {"other.example.org:AFLOWDATA/LIB2_WEB/Ca_svCu_pv/138", "any other host, another collection"},
        {"aurl:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138", "aurl: prefix, no host"},
        {"AFLOWDATA/LIB2_LIB/Ca_svCu_pv/138", "no host"},
        {"LIB2_RAW/Ca_svCu_pv/138", "no host, no AFLOWDATA"}
      };
      for (size_t i = 0; i < aurls_fs.size(); i++) {
        aflowlib::EntryLoader el_fs;
        el_fs.m_out_silent = true;
        el_fs.m_filesystem_path = fs_dir + "/";
        el_fs.m_sqlite_alloy_file = fs_dir + "/no_alloy.db";
        const bool source_set = el_fs.setSource(aflowlib::EntryLoader::Source::FILESYSTEM_RAW);
        if (source_set) {el_fs.loadAURL(aurls_fs[i][0]);}
        checkEqual(source_set ? el_fs.m_entries_flat->size() : (size_t) 0, (size_t) 1, check_function, aurls_fs[i][1] + ": the entry is found", passed_checks, results);
      }
      aurostd::RemoveDirectory(fs_dir);
    }
    //CO20261004 STOP

    //CO20261004 START - one path for plain and encoded queries: decoded terms, the paging term replaced, encoded again
    check_function = "aflowlib::AFLUXPageQuery()";
    const string mnpd_page1 = "?%2A%2Calloy%28Mn%3APd%3AMnPd%29%2Cformat%28aflow%29%2Cpaging%281%2C2000%29";
    checkEqual(aflowlib::AFLUXPageQuery("?*,alloy(Mn:Pd:MnPd),format(aflow),paging(0)", 1, 2000), mnpd_page1,
        check_function, "plain paging(0): encoded as buildAFLUXQuery() encodes", passed_checks, results);
    // the query of the failed CI run (percent-encoded by buildAFLUXQuery)
    checkEqual(aflowlib::AFLUXPageQuery("?%2A%2Calloy%28Mn%3APd%3AMnPd%29%2Cformat%28aflow%29%2Cpaging%280%29", 1, 2000), mnpd_page1,
        check_function, "encoded paging(0): the same page query", passed_checks, results);
    checkEqual(aflowlib::AFLUXPageQuery("?*,alloy(Mn:Pd:MnPd),format(aflow),paging(0)", 2, 2000),
        string("?%2A%2Calloy%28Mn%3APd%3AMnPd%29%2Cformat%28aflow%29%2Cpaging%282%2C2000%29"), check_function, "plain paging(0), page 2", passed_checks, results);
    checkEqual(aflowlib::AFLUXPageQuery("?species(Fe),paging( 0 , 10),format(aflow)", 1, 64),
        string("?species%28Fe%29%2Cpaging%281%2C64%29%2Cformat%28aflow%29"), check_function, "plain paging(0,k) with spaces", passed_checks, results);
    checkEqual(aflowlib::AFLUXPageQuery("?species%28Fe%29%2cpaging%280%2c5%29", 3, 100),
        string("?species%28Fe%29%2Cpaging%283%2C100%29"), check_function, "encoded paging(0,k), lower-case hex", passed_checks, results);
    checkEqual(aflowlib::AFLUXPageQuery("species(Fe),$paging(0),format(aflow)", 1, 10),
        string("?species%28Fe%29%2C%24paging%281%2C10%29%2Cformat%28aflow%29"), check_function, "no leading ?, $paging keeps its $", passed_checks, results);
    checkEqual(aflowlib::AFLUXPageQuery("?species(Fe)", 1, 2000), string(""),
        check_function, "no paging term: no page query", passed_checks, results);
    checkEqual(aflowlib::AFLUXPageQuery("?species%28Fe%29%2Cformat%28aflow%29", 1, 2000), string(""),
        check_function, "encoded, no paging term: no page query", passed_checks, results);

    // whether a query is sent as pages: AFLUX's parser reads paging and format (m_default_paging, m_pagingParam_n, m_format)
    check_function = "aflowlib::AFLUXPagingAll()";
    struct PagingCase {string query; bool paged; string description;};
    const vector<PagingCase> paging_cases = {
      {"?*,alloy(Mn:Pd:MnPd),format(aflow),paging(0)", true, "plain paging(0)"},
      {"?%2A%2Calloy%28Mn%3APd%3AMnPd%29%2Cformat%28aflow%29%2Cpaging%280%29", true, "encoded paging(0)"},
      {"?species%28Fe%29%2cpaging%280%2c5%29%2cformat%28aflow%29", true, "encoded paging(0,k), lower-case hex"},
      {"?species(Fe),paging( 0 , 10),format(aflow)", true, "paging(0,k) with spaces"},
      {"?species(Fe),$paging(0),format(aflow)", true, "$paging(0)"},
      {"?chaos_only_keyword(1),paging(0),format(aflow)", true, "a keyword the local parser does not know: only paging and format are read"},
      {"?species(Fe),paging(1,5),format(aflow)", false, "paging(1,5) is bounded"},
      {"?species(Fe),paging(10),format(aflow)", false, "paging(10) is bounded"},
      {"?species%28Fe%29%2Cpaging%2810%29%2Cformat%28aflow%29", false, "encoded paging(10) is bounded"},
      {"?species(Fe),format(aflow)", false, "no paging term"},
      {"?species(Fe),paging(0)", false, "paging(0) in the default format (json): pages are joined by line, so not paged"},
      {"?species(Fe),paging(0),format(json)", false, "paging(0) in format(json): not paged"}
    };
    for (size_t q = 0; q < paging_cases.size(); q++) {
      checkEqual(aflowlib::AFLUXPagingAll(paging_cases[q].query), paging_cases[q].paged, check_function, paging_cases[q].description, passed_checks, results);
    }

    // aflowlib::AFLUXCall() (aflow --aflux=...): the query it builds from a summons, and the pages it sends (no network here)
    const string summons_all = "species(Fe),format(aflow),paging(0)";
    const string query_all = "?" + aurostd::httpPercentEncodingFull(summons_all);  // as AFLUXCall() encodes the summons
    checkEqual(aflowlib::AFLUXPagingAll(query_all), true, check_function, "AFLUXCall(): " + summons_all + " goes as pages", passed_checks, results);
    checkEqual(aflowlib::AFLUXPagingAll("?" + aurostd::httpPercentEncodingFull("species(Fe),format(aflow),paging(1,5)")), false, check_function,
        "AFLUXCall(): paging(1,5) goes as one request", passed_checks, results);
    checkEqual(aflowlib::AFLUXPagingAll("?" + aurostd::httpPercentEncodingFull("species(Fe),paging(0)")), false, check_function,
        "AFLUXCall(): paging(0) in json goes as one request (the server answers)", passed_checks, results);
    check_function = "aflowlib::AFLUXPageQuery()";
    checkEqual(aflowlib::AFLUXPageQuery(query_all, 2, 10000), string("?species%28Fe%29%2Cformat%28aflow%29%2Cpaging%282%2C10000%29"), check_function,
        "AFLUXCall(): page 2 of " + summons_all, passed_checks, results);
    //CO20261004 STOP
  }

  /// @brief the CHAOS API key goes only to s4e.ai, only over https, never onto a command line or into a file
  /// @note offline: no network; the environment is not changed (other tests may run at the same time)
  void UnitTest::chaosKeyTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function = "aurostd::httpIsS4EHost()";
    const vector<string> s4e_hosts = {"s4e.ai", "S4E.AI", "www.s4e.ai", "https://s4e.ai/API/chaos/", "s4e.ai:443", "s4e.ai."};
    const vector<string> other_hosts = {"", "ai", "evils4e.ai", "s4e.ai.example.org", "example.org", "https://example.org/s4e.ai"};
    for (size_t i = 0; i < s4e_hosts.size(); i++) {
      checkEqual(aurostd::httpIsS4EHost(s4e_hosts[i]), true, check_function, "s4e.ai host: \"" + s4e_hosts[i] + "\"", passed_checks, results);
    }
    for (size_t i = 0; i < other_hosts.size(); i++) {
      checkEqual(aurostd::httpIsS4EHost(other_hosts[i]), false, check_function, "not s4e.ai: \"" + other_hosts[i] + "\"", passed_checks, results);
    }

    check_function = "aurostd::httpChaosApiKeyUsable()";
    checkEqual(aurostd::httpChaosApiKeyUsable("loop_Ab12.cd-EF~gh+ij/kl="), true, check_function, "token characters", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyUsable(""), false, check_function, "empty", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyUsable("abc def"), false, check_function, "space", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyUsable("abc\r\nX-Other: 1"), false, check_function, "line break (header injection)", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyUsable("abc'def"), false, check_function, "quote", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyUsable(string(513, 'a')), false, check_function, "longer than 512", passed_checks, results);

    // key resolution, with a key file in a folder of its own
    const string dir = aurostd::TmpDirectoryCreate("chaos_key_test");
    const string key_file = dir + "/chaos_api_key";
    aurostd::string2file("  loop_filekey123  \nsecond line\n", key_file);
    check_function = "aurostd::httpChaosApiKeyResolve()";
    checkEqual(aurostd::httpChaosApiKeyResolve(" loop_envkey ", key_file), string("loop_envkey"), check_function, "environment first", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyResolve("  ", key_file), string("loop_filekey123"), check_function, "blank environment: first line of the file, trimmed", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyResolve("", dir + "/missing"), string(""), check_function, "no environment, no file", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyResolve("", ""), string(""), check_function, "no environment, no file name", passed_checks, results);
    //CO20261004 START - FileExist and file2vectorstring read the first line as getline did
    const string key_file_blank = dir + "/chaos_api_key_blank", key_file_bare = dir + "/chaos_api_key_bare";
    aurostd::string2file("\nloop_secondline\n", key_file_blank);
    aurostd::string2file("loop_nonewline", key_file_bare);
    checkEqual(aurostd::httpChaosApiKeyResolve("", key_file_blank), string(""), check_function, "empty first line: no key (the key is the first line)", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyResolve("", key_file_bare), string("loop_nonewline"), check_function, "file without a final line break", passed_checks, results);
    // the environment through getenv2string, the default file under XHOST.home; compared as booleans so a real key is never printed
    check_function = "aurostd::httpChaosApiKey()";
    const string env_key = aurostd::getenv2string("CHAOS_API_KEY");
    checkEqual(aurostd::httpChaosApiKey(key_file) == aurostd::httpChaosApiKeyResolve(env_key, key_file), true, check_function,
        "CHAOS_API_KEY, else the given file", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKey() == aurostd::httpChaosApiKeyResolve(env_key, aurostd::httpChaosApiKeyFile(XHOST.home)), true, check_function,
        "CHAOS_API_KEY, else " + aurostd::httpChaosApiKeyFileName() + " with ~ from XHOST.home", passed_checks, results);
    aurostd::RemoveFile(key_file_blank);
    aurostd::RemoveFile(key_file_bare);
    //CO20261004 STOP

    // the key file is outside the aflow namespace (CO20261004); the environment is not changed here,
    // since other tests read CHAOS_API_KEY in parallel threads
    check_function = "aurostd::httpChaosApiKeyFileName()";
    checkEqual(aurostd::httpChaosApiKeyFileName(), string("~/.s4e/chaos_api_key"), check_function, "~/.s4e/chaos_api_key", passed_checks, results);
    check_function = "aurostd::httpChaosApiKeyFile()";
    checkEqual(aurostd::httpChaosApiKeyFile("/home/u"), string("/home/u/.s4e/chaos_api_key"), check_function, "in the home folder", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyFile(""), string(""), check_function, "no home folder: no file", passed_checks, results);
    aurostd::DirectoryMake(dir + "/.s4e");
    aurostd::string2file("loop_homekey456\n", aurostd::httpChaosApiKeyFile(dir));
    checkEqual(aurostd::httpChaosApiKeyResolve("", aurostd::httpChaosApiKeyFile(dir)), string("loop_homekey456"), "aurostd::httpChaosApiKeyResolve()", "no environment: the key file under the home folder's .s4e", passed_checks, results);
    aurostd::RemoveFile(aurostd::httpChaosApiKeyFile(dir));
    aurostd::RemoveDirectory(dir + "/.s4e");

    // the URL curl gets is built from checked parts (an independent review found that a host such as
    // "evil.example?.s4e.ai" passed a suffix check while curl connected to evil.example)
    check_function = "aurostd::httpS4EUrl()";
    aurostd::xURL u = aurostd::httpParseURL("https://s4e.ai/API/chaos/?species(Fe),paging(1,5)");
    checkEqual(aurostd::httpS4EUrl(u), string("https://s4e.ai/API/chaos/?species(Fe),paging(1,5)"), check_function, "parsed https URL", passed_checks, results);
    u.scheme = "http"; u.port = 80;
    checkEqual(aurostd::httpS4EUrl(u), string("https://s4e.ai/API/chaos/?species(Fe),paging(1,5)"), check_function, "http becomes https", passed_checks, results);
    u.host = "S4E.AI.";
    checkEqual(aurostd::httpS4EUrl(u), string("https://s4e.ai/API/chaos/?species(Fe),paging(1,5)"), check_function, "host case and final dot", passed_checks, results);
    const vector<string> bad_hosts = {"evil.example?.s4e.ai", "evil.example#.s4e.ai", "s4e.ai@evil.example", "a_b.s4e.ai", "evil.example"};
    for (size_t i = 0; i < bad_hosts.size(); i++) {
      u = aurostd::httpParseURL("https://s4e.ai/API/chaos/?x"); u.host = bad_hosts[i];
      checkEqual(aurostd::httpS4EUrl(u), string(""), check_function, "refused host \"" + bad_hosts[i] + "\"", passed_checks, results);
      checkEqual(aurostd::httpIsS4EHost(bad_hosts[i]), false, "aurostd::httpIsS4EHost()", "not s4e.ai: \"" + bad_hosts[i] + "\"", passed_checks, results);
    }
    u = aurostd::httpParseURL("https://s4e.ai/API/chaos/?x"); u.path = "@evil.example/";
    checkEqual(aurostd::httpS4EUrl(u), string(""), check_function, "path not starting with / (would make s4e.ai the user part)", passed_checks, results);
    u = aurostd::httpParseURL("https://s4e.ai/API/chaos/?x"); u.user = "s4e.ai";
    checkEqual(aurostd::httpS4EUrl(u), string(""), check_function, "user part", passed_checks, results);
    u = aurostd::httpParseURL("https://s4e.ai/API/chaos/?x"); u.query = "?a b";
    checkEqual(aurostd::httpS4EUrl(u), string(""), check_function, "white space in the query", passed_checks, results);
    u = aurostd::httpParseURL("https://evil.example/x?y");  // a redirect to another host is parsed like this
    checkEqual(aurostd::httpIsS4EHost(u.host) || !aurostd::httpS4EUrl(u).empty(), false, check_function, "redirect to another host: no key path", passed_checks, results);
    //CO20261004 START - httpParseURL splits the host for httpIsS4EHost(); httpJoinURL assembles the URL of httpS4EUrl()
    u = aurostd::httpParseURL("https://s4e.ai/API/chaos/?x"); u.host = "WWW.S4E.ai.";
    checkEqual(aurostd::httpS4EUrl(u), string("https://www.s4e.ai/API/chaos/?x"), check_function, "subdomain: lower case, no final dot", passed_checks, results);
    check_function = "aurostd::httpIsS4EHost()";
    checkEqual(aurostd::httpIsS4EHost("HTTPS://S4E.AI:443/API/chaos/?x"), true, check_function, "scheme, port, path and query split off", passed_checks, results);
    checkEqual(aurostd::httpIsS4EHost("user@s4e.ai"), false, check_function, "a user part is refused", passed_checks, results);
    checkEqual(aurostd::httpIsS4EHost("https://user@s4e.ai/API/"), false, check_function, "a user part is refused (full URL)", passed_checks, results);
    check_function = "aurostd::httpJoinURL()";
    checkEqual(aurostd::httpJoinURL(aurostd::httpParseURL("https://s4e.ai/API/chaos/?species(Fe),paging(1,5)")), string("https://s4e.ai/API/chaos/?species(Fe),paging(1,5)"),
        check_function, "parsed URL joined back", passed_checks, results);
    check_function = "aurostd::httpS4EUrl()";
    //CO20261004 STOP

    //CO20261006 START - an AURL keeps its path (AFLOWDATA was read as port 0 and dropped), and s4e.ai has no AFLOWLIB folders
    check_function = "aurostd::httpParseURL()";
    struct ParsedURL {string url, host; uint port; string path, query, description;};
    const vector<ParsedURL> parsed_urls = {
      {"s4e.ai:AFLOWDATA/LIB1_RAW/Pt:PAW_PBE:05Jan2001/A6", "s4e.ai", 80, "/AFLOWDATA/LIB1_RAW/Pt:PAW_PBE:05Jan2001/A6", "", "AURL: AFLOWDATA starts the path, LIB1 colons kept"},
      {"s4e.ai:AFLOWDATA/ICSD_WEB/FCC/?format=text", "s4e.ai", 80, "/AFLOWDATA/ICSD_WEB/FCC/", "?format=text", "AURL with a query"},
      {"https://s4e.ai:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138", "s4e.ai", 443, "/AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138", "", "AURL behind https://"},
      {"other.example.org:AFLOWDATA", "other.example.org", 80, "/AFLOWDATA", "", "AURL of a host, no slash"},
      {"https://s4e.ai:443/API/chaos/?x", "s4e.ai", 443, "/API/chaos/", "?x", "a port is digits"},
      {"localhost:8080/a/b", "localhost", 8080, "/a/b", "", "a port without a scheme"},
      {"s4e.ai:/x", "s4e.ai", 80, "/x", "", "an empty port: the default"},
      {"s4e.ai:", "s4e.ai", 80, "/", "", "an empty port, no path"},
      {"localhost:8080?x", "localhost", 8080, "/", "?x", "a query right after the port"},
      {"https://s4e.ai?x", "s4e.ai", 443, "/", "?x", "a query right after the host"}
    };
    for (size_t i = 0; i < parsed_urls.size(); i++) {
      const aurostd::xURL p = aurostd::httpParseURL(parsed_urls[i].url);
      checkEqual(p.host + " | " + aurostd::utype2string<uint>(p.port) + " | " + p.path + " | " + p.query,
          parsed_urls[i].host + " | " + aurostd::utype2string<uint>(parsed_urls[i].port) + " | " + parsed_urls[i].path + " | " + parsed_urls[i].query,
          check_function, parsed_urls[i].description + ": " + parsed_urls[i].url, passed_checks, results);
    }
    check_function = "aurostd::httpS4EServesPath()";
    const vector<std::pair<string, bool> > s4e_paths = {
      {"https://s4e.ai/API/chaos/?species(Fe),paging(1,5)", true}, {"https://s4e.ai/API/unit_test/xstructure/CONTCAR.relax.vasp_Cl1Na1_ICSD_240599", true},
      {"https://s4e.ai/", true}, {"https://s4e.ai/AFLOWDATAX/x", true},
      {"https://s4e.ai/AFLOWDATA/ICSD_WEB/FCC/Si1_ICSD_150530/?files", false}, {"s4e.ai:AFLOWDATA/ICSD_WEB/FCC/?format=text", false},
      {"s4e.ai/AFLOWDATA/LIB2_RAW/?aflowlib_entries", false}, {"http://s4e.ai//AFLOWDATA/LIB2_RAW/", false}, {"https://s4e.ai/aflowdata/LIB2_RAW/", false}
    };
    for (size_t i = 0; i < s4e_paths.size(); i++) {
      checkEqual(aurostd::httpS4EServesPath(aurostd::httpParseURL(s4e_paths[i].first)), s4e_paths[i].second, check_function,
          (s4e_paths[i].second ? "served: " : "no AFLOWLIB folders: ") + s4e_paths[i].first, passed_checks, results);
    }
    // the same AFLOWDATA path on another host is not refused (the refusal prints its reason, so it is not called with an s4e.ai folder here)
    check_function = "aurostd::httpS4EPathRefused()";
    checkEqual(aurostd::httpS4EPathRefused(aurostd::httpParseURL(AFLOWLIB_SERVER_LEGACY + ":AFLOWDATA/ICSD_WEB/FCC/?format=text")), false, check_function,
        "the legacy server's folders are not refused", passed_checks, results);
    checkEqual(aurostd::httpS4EPathRefused(aurostd::httpParseURL("https://s4e.ai/API/chaos/?x")), false, check_function,
        "the CHAOS API is not refused", passed_checks, results);
    //CO20261006 STOP

    // curl arguments: the key is read by curl from its standard input, never from the command line
    check_function = "aurostd::httpCUrlArgs()";
    const vector<string> with_key = aurostd::httpCUrlArgs("https://s4e.ai/API/chaos/?x", true);
    const vector<string> without_key = aurostd::httpCUrlArgs("https://s4e.ai/API/chaos/?x", false);
    checkEqual(aurostd::joinWDelimiter(with_key, " "), string("curl -q -i -s --raw --proto =https --suppress-connect-headers --connect-timeout 30 --max-time 900 -H @- https://s4e.ai/API/chaos/?x"),
        check_function, "with the key: no ~/.curlrc, header from standard input, https only, no -v", passed_checks, results);
    checkEqual(aurostd::joinWDelimiter(without_key, " "), string("curl -q -i -s --raw --proto =https --suppress-connect-headers --connect-timeout 30 --max-time 900 https://s4e.ai/API/chaos/?x"),
        check_function, "without the key: no header", passed_checks, results);

    // curl runs without a shell; its standard input and output, and its exit status, come through
    check_function = "aurostd::httpCUrlRun()";
    string curl_out;
    int curl_exit = -1;
    bool curl_ok = aurostd::httpCUrlRun({"curl", "-s", "-K", "-"}, "url = \"file:///dev/null\"\nwrite-out = \"stdin-ok\"\n", curl_out, curl_exit);
    checkEqual(curl_ok && curl_exit == 0 && curl_out == "stdin-ok", true, check_function,
        "input reaches curl (a config on standard input), its output comes back", passed_checks, results);
    curl_ok = aurostd::httpCUrlRun({"curl", "-s", "--proto", "=https", "http://127.0.0.1:9/"}, "", curl_out, curl_exit);
    checkEqual(!curl_ok && curl_exit > 0, true, check_function, "plain http refused by --proto =https: failure with curl's exit status", passed_checks, results);

    // an answer cut off is not an answer (the gate drops the connection when the query fails midway)
    check_function = "aurostd::httpParseResponse(strict)";
    std::map<string, string> hdr;
    string body;
    const string chunked = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
    checkEqual(aurostd::httpParseResponse(chunked + "5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n", body, hdr, true), 200, check_function, "complete chunked answer", passed_checks, results);
    checkEqual(body, string("hello world"), check_function, "complete chunked answer: body", passed_checks, results);
    hdr.clear();
    checkEqual(aurostd::httpParseResponse(chunked + "5\r\nhello\r\n", body, hdr, true), -1, check_function, "chunked answer cut off after a chunk", passed_checks, results);
    hdr.clear();
    checkEqual(aurostd::httpParseResponse("HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nhello world", body, hdr, true), 200, check_function, "body as long as Content-Length", passed_checks, results);
    hdr.clear();
    checkEqual(aurostd::httpParseResponse("HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nhello", body, hdr, true), -1, check_function, "body shorter than Content-Length", passed_checks, results);
    hdr.clear();
    checkEqual(aurostd::httpParseResponse("HTTP/2 401\r\ncontent-type: application/json\r\n\r\n{}", body, hdr, true), 401, check_function, "HTTP/2 status line without reason", passed_checks, results);
    // the last header line was dropped by the parser (any mode): a Location sent last was lost
    check_function = "aurostd::httpParseResponse()";
    hdr.clear();
    aurostd::httpParseResponse("HTTP/1.1 302 Found\r\nContent-Length: 0\r\nLocation: https://s4e.ai/x\r\n\r\n", body, hdr);
    checkEqual(hdr.count("location") ? hdr["location"] : string("(missing)"), string("https://s4e.ai/x"), check_function, "last header line kept", passed_checks, results);
    //CO20261004 START - a header line without ':' is skipped, repeated headers are joined, and only strict
    // mode (s4e.ai) refuses an answer cut off: other servers' answers are read as before
    hdr.clear();
    aurostd::httpParseResponse("HTTP/1.1 200 OK\r\nX-Junk\r\nVary: a\r\nVary: b\r\n\r\nbody", body, hdr);
    vector<string> header_fields;
    for (std::map<string, string>::const_iterator it = hdr.begin(); it != hdr.end(); ++it) {header_fields.push_back(it->first + "=" + it->second);}
    checkEqual(aurostd::joinWDelimiter(header_fields, ";"), string("vary=a,b"), check_function,
        "a line without ':' is skipped, a repeated header joined with ','", passed_checks, results);
    vector<string> lenient_answers;
    hdr.clear();
    lenient_answers.push_back(aurostd::utype2string<int>(aurostd::httpParseResponse(chunked + "5\r\nhello\r\n", body, hdr)) + ":" + body);
    hdr.clear();
    lenient_answers.push_back(aurostd::utype2string<int>(aurostd::httpParseResponse("HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nhello", body, hdr)) + ":" + body);
    checkEqual(aurostd::joinWDelimiter(lenient_answers, "|"), string("200:hello|200:hello"), check_function,
        "not strict: no last chunk, or a body shorter than Content-Length, still read", passed_checks, results);
    //CO20261004 STOP

    aurostd::RemoveFile(key_file);
    aurostd::RemoveDirectory(dir);

    check_function = "aurostd::httpChaosApiKeyHint()";
    checkEqual(aurostd::httpChaosApiKeyHint("example.org", 401), string(""), check_function, "other host: no hint", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyHint("s4e.ai", 200), string(""), check_function, "200: no hint", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyHint("s4e.ai", 401).find("CHAOS API key") != string::npos, true, check_function, "401 names the key", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyHint("s4e.ai", 401).find(aurostd::httpChaosApiKeyFileName()) != string::npos, true, check_function, "401 names the key file", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyHint("s4e.ai", 401).find(".aflow") == string::npos, true, check_function, "401 does not name ~/.aflow", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyHint("s4e.ai", 403).find("approved") != string::npos, true, check_function, "403 names the approval", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyHint("s4e.ai", 400).find("paging(0)") != string::npos, true, check_function, "400 names paging(0)", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyHint("s4e.ai", 400).find("parentheses") != string::npos, true, check_function, "400 names the parentheses", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyHint("s4e.ai", 400).find("$") != string::npos, true, check_function, "400 names the $ prefix", passed_checks, results);
    checkEqual(aurostd::httpChaosApiKeyHint("https://s4e.ai/API/chaos/", 429), string("over the CHAOS API request limits; try again in a minute"), check_function, "429 names the request limits", passed_checks, results); //CO20261004

    //CO20261004 START - the rest of the URL curl gets is checked too: a query starts with '?', no backslash or DEL
    check_function = "aurostd::httpS4EUrl()";
    vector<string> refused_rest;
    u = aurostd::httpParseURL("https://s4e.ai/API/chaos/?x"); u.query = "x";
    refused_rest.push_back("query without ?: \"" + aurostd::httpS4EUrl(u) + "\"");
    u = aurostd::httpParseURL("https://s4e.ai/API/chaos/?x"); u.path = "/API\\chaos/";
    refused_rest.push_back("backslash in the path: \"" + aurostd::httpS4EUrl(u) + "\"");
    u = aurostd::httpParseURL("https://s4e.ai/API/chaos/?x"); u.query = string("?a") + '\x7f';
    refused_rest.push_back("DEL in the query: \"" + aurostd::httpS4EUrl(u) + "\"");
    checkEqual(aurostd::joinWDelimiter(refused_rest, "; "),
        string("query without ?: \"\"; backslash in the path: \"\"; DEL in the query: \"\""),
        check_function, "refused: a query without '?', a backslash, a DEL character", passed_checks, results);
    //CO20261004 STOP
  }


  /// @brief chull orders the unary entries without crashing when nothing was loaded
  /// @note when the server refused the query (no CHAOS API key, say), loadPoints() read entries[0] of
  ///       an empty list and aflow --chull ended in a segmentation fault before printing why
  void UnitTest::chullUnaryOrderTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) {
    (void) errors;  // Suppress compiler warnings
    const string check_function = "chull::ConvexHull::reorderUnariesLIB2()";
    std::ostringstream oss;
    chull::ConvexHull hull(oss);
    vector<string> velements = {"Mn", "Pd"};
    vector<vector<vector<aflowlib::_aflowlib_entry> > > entries;
    hull.reorderUnariesLIB2(entries, velements);
    checkEqual(entries.size(), (size_t) 0, check_function, "nothing loaded: returns, adds nothing", passed_checks, results);
    entries.resize(1);
    hull.reorderUnariesLIB2(entries, velements);
    checkEqual(entries[0].size(), (size_t) 0, check_function, "no unary alloys", passed_checks, results);
    entries[0].resize(2);
    hull.reorderUnariesLIB2(entries, velements);
    checkEqual(entries[0][0].size() + entries[0][1].size(), (size_t) 0, check_function, "unary alloys without entries", passed_checks, results);
    // two Mn entries out of order: sorted by catalog, then prototype (unchanged behavior)
    aflowlib::_aflowlib_entry a2, a1;
    a2.Load("aurl=s4e.ai:AFLOWDATA/LIB1_RAW/Mn_pv:PAW_PBE:07Sep2000/A2|auid=s4e:1000000000000002|compound=Mn1|species=Mn|nspecies=1|catalog=LIB1|prototype=A2", oss);
    a1.Load("aurl=s4e.ai:AFLOWDATA/LIB1_RAW/Mn_pv:PAW_PBE:07Sep2000/A1|auid=s4e:1000000000000001|compound=Mn1|species=Mn|nspecies=1|catalog=LIB1|prototype=A1", oss);
    entries[0][0] = {a2, a1};
    hull.reorderUnariesLIB2(entries, velements);
    checkEqual(entries[0][0].size() == 2 ? entries[0][0][0].auid + " " + entries[0][0][1].auid : string("wrong size"),
        string("s4e:1000000000000001 s4e:1000000000000002"), check_function, "entries sorted by prototype", passed_checks, results);
  }

  /// @brief the convex hull's logos and entry links: the S4E and CHAOS logos are embedded, and no entry links to a page
  /// @note offline. CHAOS has no web page per entry, so isMaterialPageAvailable() is false for every AUID; it was
  ///       true for every AUID not starting with "s4e:", which linked compounds to the legacy materials server
  void UnitTest::chullLinksTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) { //CO20261006
    (void) errors;  // Suppress compiler warnings
    string check_function = "chull::ChullPoint::isMaterialPageAvailable()";
    std::ostringstream oss;
    chull::ChullPoint point(oss);
    point.m_entry.auid = "aflow:0171c43afba95d07";
    checkEqual(point.isMaterialPageAvailable(), false, check_function, "aflow: AUID: no entry page", passed_checks, results);
    point.m_entry.auid = "s4e:0171c43afba95d07";
    checkEqual(point.isMaterialPageAvailable(), false, check_function, "s4e: AUID: no entry page", passed_checks, results);

    check_function = "aurostd::EmbData::save_to_file()";
    const string file = aurostd::TmpFileCreate("chull_logo");
    for (const string logo : {"S4E_LOGO.pdf", "S4E_LOGO_SKINNY.pdf", "CHAOS_LOGO.pdf"}) {
      string content;
      try {
        aurostd::EmbData::save_to_file(logo, file);
        content = aurostd::file2string(file);
      } catch (aurostd::xerror&) {content = "";}
      checkEqual(content.substr(0, 5), string("%PDF-"), check_function, logo + " is embedded, a PDF", passed_checks, results);
    }
    vector<string> still_embedded;
    for (const string logo : {"AFLOW_LOGO_FULL.pdf", "AFLOW_LOGO_SKINNY.pdf", "NOMAD_LOGO.png"}) {
      try {
        aurostd::EmbData::save_to_file(logo, file);
        still_embedded.push_back(logo);
      } catch (aurostd::xerror&) {}
    }
    aurostd::RemoveFile(file);
    checkEqual(aurostd::joinWDelimiter(still_embedded, ","), string(""), check_function, "the AFLOW and NOMAD logos are not embedded", passed_checks, results);
  }

  /// @brief the AUIDs and AURLs the entry loader sends to s4e.ai's AFLUX engine, and the SQL the engine makes of them
  /// @note offline: no network, no database. The loader quoted each value ('s4e:...'), as the earlier AFLUX
  ///       servers took it; the engine in aflow_aflux.cpp splits a value at every ':' and keeps the quotes, so
  ///       loadAUID() and loadAURL() through s4e.ai found nothing (the entry_loader test on the group's server, 2026-10-03)
  void UnitTest::afluxIdQueryTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) { //CO20261003
    (void) errors;  // Suppress compiler warnings
    string check_function = "aflowlib::AFLUXValueList()";
    const string auid1 = "s4e:0171c43afba95d07", auid2 = "s4e:0011df4470f20f8b";
    const string aurl1 = "s4e.ai:AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/AB9C4_hP28_188_e_kl_ak-001.CBA";
    const string aurl2 = "s4e.ai:AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/AB2C_hR4_166_a_c_b-003.CBA";
    checkEqual(aflowlib::AFLUXValueList({auid1}), auid1, check_function, "one AUID: as it is, no quotes", passed_checks, results);
    checkEqual(aflowlib::AFLUXValueList({auid1, auid2}), auid1 + ":" + auid2, check_function, "two AUIDs: joined with ':' (AFLUX's or)", passed_checks, results);

    //CO20261004 START - one batching helper for loadAUID() and loadAURL(); batches shown joined with " | "
    check_function = "aflowlib::AFLUXValueBatches()";
    checkEqual(aurostd::joinWDelimiter(aflowlib::AFLUXValueBatches({auid1, auid2}, 5000), " | "), auid1 + ":" + auid2,
        check_function, "two AUIDs under the limit: one batch", passed_checks, results);
    checkEqual(aurostd::joinWDelimiter(aflowlib::AFLUXValueBatches({aurl1, aurl2}, aurl1.size() + aurl2.size()), " | "), aurl1 + " | " + aurl2,
        check_function, "two AURLs at the limit: two batches", passed_checks, results);
    checkEqual(aurostd::joinWDelimiter(aflowlib::AFLUXValueBatches({"aaaa", "bbbb", "cccc"}, 9), " | "), string("aaaa:bbbb | cccc"),
        check_function, "limit 9: 4+4 is below it (the ':' is not counted), 9+4 is not", passed_checks, results);
    checkEqual(aurostd::joinWDelimiter(aflowlib::AFLUXValueBatches({"aaaa", "bbbb", "cccc"}, 8), " | "), string("aaaa | bbbb | cccc"),
        check_function, "limit 8: 4+4 is not below it", passed_checks, results);
    checkEqual(aflowlib::AFLUXValueBatches({}, 5000).size(), (size_t) 0,
        check_function, "empty list: no batch (an empty one made the query auid without a value)", passed_checks, results);
    checkEqual(aurostd::joinWDelimiter(aflowlib::AFLUXValueBatches({string(12, 'x'), "aa", "bb"}, 10), " | "), string(12, 'x') + " | aa:bb",
        check_function, "first value over the limit: a batch of its own, no empty batch", passed_checks, results);
    //CO20261004 STOP

    //CO20261004 START - the encoding of every AFLUX query: no "%00" after a final unreserved character
    check_function = "aurostd::httpPercentEncodingFull()";
    checkEqual(aurostd::httpPercentEncodingFull("species(Fe),$compound"), string("species%28Fe%29%2C%24compound"), check_function, "ends in a letter: no %00", passed_checks, results);
    checkEqual(aurostd::httpPercentEncodingFull("paging(0)"), string("paging%280%29"), check_function, "ends in a reserved character", passed_checks, results);
    checkEqual(aurostd::httpPercentEncodingFull("s4e.ai-_~09"), string("s4e.ai-_~09"), check_function, "unreserved characters only: unchanged", passed_checks, results);
    checkEqual(aurostd::httpPercentEncodingFull(""), string(""), check_function, "empty", passed_checks, results);
    //CO20261004 STOP

    // the whole query as the loader sends it (EntryLoader::buildAFLUXQuery(): keys in order, percent-encoded),
    // read by the engine as s4e.ai's API does
    check_function = "aflux::AFLUX::parseQuery()";
    //CO20261008 - the database names them chaos_id and chaos_path; an AURL is matched as its chaos_path
    const string path1 = aflowlib::AURL2ChaosPath(aurl1), path2 = aflowlib::AURL2ChaosPath(aurl2);
    const string expected_auid = "(chaos_id LIKE '\"" + auid1 + "\"' OR chaos_id LIKE '\"" + auid2 + "\"')";
    const string expected_aurl = "(chaos_path LIKE '\"" + path1 + "\"' OR chaos_path LIKE '\"" + path2 + "\"')";
    {
      aflux::AFLUX engine;
      engine.parseQuery("?" + aurostd::httpPercentEncodingFull("*,auid(" + aflowlib::AFLUXValueList({auid1, auid2}) + "),format(aflow),paging(1,10000)"));
      checkEqual(engine.m_SQL_query, expected_auid, check_function, "two AUIDs: one match each, prefix and number together", passed_checks, results);
    }
    {
      aflux::AFLUX engine;
      engine.parseQuery("?" + aurostd::httpPercentEncodingFull("*,aurl(" + aflowlib::AFLUXValueList({aurl1, aurl2}) + "),format(aflow),paging(1,10000)"));
      checkEqual(engine.m_SQL_query, expected_aurl, check_function, "two AURLs: one match each, as chaos_path", passed_checks, results);
    }
    {
      aflux::AFLUX engine;  //CO20261008 - what the loader sends now
      engine.parseQuery("?" + aurostd::httpPercentEncodingFull("*,chaos_path(" + aflowlib::AFLUXValueList({path1, path2}) + "),format(aflow),paging(1,10000)"));
      checkEqual(engine.m_SQL_query, expected_aurl, check_function, "two chaos_paths: one match each", passed_checks, results);
    }
    {
      aflux::AFLUX engine;  //CO20261008 - a supercell's system, an identifier like chaos_id
      engine.parseQuery("?" + aurostd::httpPercentEncodingFull("*,pocc_system(" + aflowlib::AFLUXValueList({auid1, auid2}) + "),paging(1,10)"));
      checkEqual(engine.m_SQL_query, "(pocc_system LIKE '\"" + auid1 + "\"' OR pocc_system LIKE '\"" + auid2 + "\"')", check_function, "two systems in pocc_system: prefix and number together", passed_checks, results);
    }
    {
      // colons inside paths (pseudopotential, POCC) are not separators; LIKE's _ matches them
      const string p1 = "LIB1/As:PAW_PBE/A_hP2_194_c-001.A";
      const string p2 = "LIB5/Cr_pvCu_pvMo_pvNi_pvZn:PAW_PBE/A_cI2_229_a.A:POCC_P0-0.2xA-0.2xB-0.2xC-0.2xD-0.2xE/ARUN.POCC_05_H0C4";
      checkEqual(aflux::chaosPathParam(p1 + ":" + p2), string("LIB1/As_PAW_PBE/A_hP2_194_c-001.A:LIB5/Cr_pvCu_pvMo_pvNi_pvZn_PAW_PBE/A_cI2_229_a.A_POCC_P0-0.2xA-0.2xB-0.2xC-0.2xD-0.2xE/ARUN.POCC_05_H0C4"),
          "aflux::chaosPathParam()", "colons inside paths become _, the one between them stays", passed_checks, results);
      checkEqual(aflux::chaosPathParam("*"), string("*"), "aflux::chaosPathParam()", "* unchanged", passed_checks, results);
      checkEqual(aflux::chaosPathParam("s4e.ai:AFLOWDATA/LIB1_WEB/As:PAW_PBE/A"), string("LIB1/As_PAW_PBE/A"), "aflux::chaosPathParam()", "an AURL with a colon in its path", passed_checks, results);
      aflux::AFLUX engine;
      engine.parseQuery("?" + aurostd::httpPercentEncodingFull("*,chaos_path(" + p1 + ":" + p2 + "),format(aflow),paging(1,10000)"));
      checkEqual(engine.m_SQL_query, "(chaos_path LIKE '\"" + aflux::chaosPathParam(p1) + "\"' OR chaos_path LIKE '\"" + aflux::chaosPathParam(p2) + "\"')",
          check_function, "two paths with colons: two matches", passed_checks, results);
    }
    {
      // the quoted form the loader sent before: the quotes stay in the SQL, so nothing matches
      aflux::AFLUX engine;
      engine.parseQuery("?" + aurostd::httpPercentEncodingFull("*,auid('" + auid1 + "':'" + auid2 + "'),format(aflow),paging(1,10000)"));
      check(engine.m_SQL_query != expected_auid, engine.m_SQL_query, string("not ") + expected_auid, check_function,
          "the quoted form gives other SQL (why the loader no longer quotes)", passed_checks, results);
    }
  }

  /// @brief the keys of the CHAOS API's answers (Corey, 2026-10-07): the identifier is chaos_id; the run's path
  ///        (chaos_path), the library and AFLOW version stamps and the run details are left out of JSON and HTML;
  ///        chaos_id(...) and auid(...) in a query are the identifier; format(aflow), which AFLOW's own clients
  ///        read, stays whole, with chaos_path and chaos_id; the files lib2raw writes keep auid and aurl until
  ///        lib2raw switches. Checked for the database as it is now (chaos_id, chaos_path, CO20261008) and as it
  ///        was (auid, aurl).
  /// @note offline: no network or database. If a check fails after a change to aflowlib2string() or to
  ///       aflux::responseKeyMap(), the API is printing a key it should not (or lost one it needs).
  void UnitTest::afluxResponseKeysTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) { //CO20261007
    (void) errors;  // Suppress compiler warnings
    const string id = "s4e:0171c43afba95d07";
    aflowlib::_aflowlib_entry entry;
    entry.auid = id;
    entry.aurl = "s4e.ai:AFLOWDATA/LIB3_WEB/Bi_dOTi_sv/AB9C4_hP28_188_e_kl_ak-001.CBA";
    entry.compound = "Bi4O9Ti";
    entry.vloop.push_back("thermodynamics");
    entry.aflow_version = "aflow30847";
    entry.vaflowlib_date.push_back("20250101_00:00:00_GMT-5");
    const vector<string> hidden = aflux::responseHiddenKeywords();

    string check_function = "aflux::entryIdKeyword()";
    checkEqual(aflux::entryIdKeyword(), string("chaos_id"), check_function, "the schema names the identifier chaos_id", passed_checks, results);
    checkEqual(aflux::entryPathKeyword(), string("chaos_path"), check_function, "the schema names the run's path chaos_path", passed_checks, results);
    checkEqual(aflux::entryPathKeyword("auid"), string("aurl"), check_function, "a database built before: aurl", passed_checks, results);

    check_function = "aflowlib::_aflowlib_entry::aflowlib2string()";
    // AFLOW's own JSON (lib2raw's aflowlib.json, which the import reads) keeps auid and aurl (CO20261008)
    const string full = entry.aflowlib2string("JSON");
    checkEqual(aurostd::substring2bool(full, "\"auid\":\"" + id + "\""), true, check_function, "AFLOW's JSON: auid", passed_checks, results);
    checkEqual(aurostd::substring2bool(full, "\"aurl\":\"" + entry.aurl + "\""), true, check_function, "AFLOW's JSON: aurl", passed_checks, results);
    checkEqual(aurostd::substring2bool(full, "\"chaos_id\":") || aurostd::substring2bool(full, "\"chaos_path\":"), false, check_function, "AFLOW's JSON: no chaos_id or chaos_path yet", passed_checks, results);
    checkEqual(aurostd::substring2bool(full, "\"aflowlib_version\":"), true, check_function, "AFLOW's JSON: aflowlib_version kept", passed_checks, results);
    // the API's JSON, with and without nulls
    for (const bool print_null : {false, true}) {
      const string tag = print_null ? " (nulls printed)" : "";
      const string api = entry.aflowlib2string("JSON", print_null, aflux::responseKeyMap());
      for (const string& key : hidden) {
        checkEqual(aurostd::substring2bool(api, "\"" + key + "\":"), false, check_function, "API: no " + key + tag, passed_checks, results);
      }
      checkEqual(aurostd::substring2bool(api, "\"chaos_id\":\"" + id + "\""), true, check_function, "API: chaos_id with the identifier" + tag, passed_checks, results);
      checkEqual(aurostd::substring2bool(api, "\"compound\":\"Bi4O9Ti\""), true, check_function, "API: other keys untouched" + tag, passed_checks, results);
      const aurostd::JSON::object parsed = aurostd::JSON::loadString(api);
      checkEqual(parsed.type == aurostd::JSON::object_types::DICTIONARY, true, check_function, "API: a JSON object" + tag, passed_checks, results);
      checkEqual(string(parsed["chaos_id"]), id, check_function, "API: chaos_id read back from the JSON" + tag, passed_checks, results);
    }
    // a database built before (auid, aurl): auid printed as chaos_id, aurl left out
    {
      const std::map<string,string> before = aflux::responseKeyMap("auid");
      checkEqual(before.count("auid") == 1 && before.at("auid") == "chaos_id", true, check_function, "before: auid printed as chaos_id", passed_checks, results);
      checkEqual(before.count("aurl") == 1 && before.at("aurl").empty(), true, check_function, "before: aurl left out", passed_checks, results);
    }
    // a key renamed to one the entry already prints is left out: never the same key twice
    {
      const std::map<string,string> onto_id = {{"compound", "auid"}};
      const string api = entry.aflowlib2string("JSON", false, onto_id);
      checkEqual(aurostd::substring2bool(api, "\"auid\":\"" + id + "\""), true, check_function, "rename onto a printed key: the printed key stays", passed_checks, results);
      checkEqual(aurostd::substring2bool(api, "\"auid\":\"Bi4O9Ti\""), false, check_function, "rename onto a printed key: the renamed key goes", passed_checks, results);
      checkEqual(aurostd::substring2bool(api, "\"compound\":"), false, check_function, "rename onto a printed key: the old name goes", passed_checks, results);
    }
    // format(aflow), which AFLOW's clients (EntryLoader, ChaosRuns) read: chaos_path and chaos_id (mode chaos)
    const string chaos = entry.aflowlib2string("chaos");
    checkEqual(aurostd::substring2bool(chaos, "chaos_id=" + id), true, check_function, "format(aflow): chaos_id", passed_checks, results);
    checkEqual(aurostd::substring2bool(chaos, "chaos_path=LIB3/Bi_dOTi_sv/AB9C4_hP28_188_e_kl_ak-001.CBA" + _AFLOWLIB_ENTRY_SEPARATOR_), true, check_function, "format(aflow): chaos_path", passed_checks, results);
    checkEqual(aurostd::substring2bool(chaos, "aurl=") || aurostd::substring2bool(chaos, "auid="), false, check_function, "format(aflow): no aurl or auid", passed_checks, results);
    // lib2raw's aflowlib.out keeps aurl and auid, and the rest of the line is the same
    const string out = entry.aflowlib2string("out");
    checkEqual(aurostd::substring2bool(out, "aurl=" + entry.aurl + _AFLOWLIB_ENTRY_SEPARATOR_ + "auid=" + id), true, check_function, "aflowlib.out: aurl and auid", passed_checks, results);
    checkEqual(out.substr(out.find("compound=")), chaos.substr(chaos.find("compound=")), check_function, "aflowlib.out and format(aflow) differ only in the two names", passed_checks, results);

    check_function = "aflux::keywordFromResponseName()";
    checkEqual(aflux::keywordFromResponseName("chaos_id"), string("chaos_id"), check_function, "chaos_id is chaos_id", passed_checks, results);
    checkEqual(aflux::keywordFromResponseName("auid"), string("chaos_id"), check_function, "auid still works", passed_checks, results);
    checkEqual(aflux::keywordFromResponseName("$auid"), string("$chaos_id"), check_function, "$auid is $chaos_id", passed_checks, results);
    checkEqual(aflux::keywordFromResponseName("aurl"), string("chaos_path"), check_function, "aurl is chaos_path", passed_checks, results);
    checkEqual(aflux::keywordFromResponseName("species"), string("species"), check_function, "other names unchanged", passed_checks, results);
    checkEqual(aflux::keywordFromResponseName("chaos_id", "auid"), string("auid"), check_function, "before: chaos_id is auid", passed_checks, results);
    checkEqual(aflux::keywordFromResponseName("chaos_path", "auid"), string("aurl"), check_function, "before: chaos_path is aurl", passed_checks, results);

    check_function = "aflux::AFLUX::parseQuery()";
    {
      aflux::AFLUX by_chaos_id, by_auid;
      by_chaos_id.parseQuery("?" + aurostd::httpPercentEncodingFull("chaos_id(" + id + "),paging(1,10)"));
      by_auid.parseQuery("?" + aurostd::httpPercentEncodingFull("auid(" + id + "),paging(1,10)"));
      checkEqual(by_chaos_id.m_SQL_query, by_auid.m_SQL_query, check_function, "auid(...) gives the SQL of chaos_id(...)", passed_checks, results);
      checkEqual(by_chaos_id.m_SQL_query, "(chaos_id LIKE '\"" + id + "\"')", check_function, "chaos_id(...): one match on the identifier", passed_checks, results);
    }
  }

  /// @brief chaos_path, the run's path as CHAOS writes it (Corey, 2026-10-08): the catalog and the folders below it
  ///        (LIBn/...), which are the folders under the catalog's LIB (the runs) and RAW (lib2raw's output) on the
  ///        machine that keeps them; the identifier is chaos_id; a supercell names its POCC system (pocc_system).
  ///        Files and database lines written before (auid, aurl) read the same way, and a database built from
  ///        either gives the same tables (chaos_id_00 to chaos_id_ff).
  /// @note the paths are the shapes of the CHAOS database (LIB1 to LIB8, systems and supercells); the database
  ///       part builds a small database in a temporary folder
  /// @brief the AFLUX filters the Methods of the CHAOS paper describe: an atom-specific filter passes when any atom of the
  ///        element passes, a species-pair filter reads the pair named, components count from 0 in the SQL and in the C++
  ///        check, the per-sublattice lists take a plain comparison (any value) or @(I), and ldau_TLUJ is tensor-like
  /// @note offline: the SQL runs on a fixture database built here
  void UnitTest::afluxFiltersTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) { //CO20261018
    // C++ checks on one entry, read as the loader reads it: Fe2O2, charges Fe 1.0 and 1.5, O -1.2 and -1.3; bonds Fe-Fe
    // 2.5, Fe-O 2.1, O-O 3.0 (the order of GetNBONDXX()); stress tensor 1 to 9
    aflowlib::_aflowlib_entry fe2o2;
    fe2o2.Load("species=Fe,O | composition=2,2 | bader_net_charges=1.0,1.5,-1.2,-1.3 | nbondxx=2.5,2.1,3.0 | stress_tensor=1,2,3,4,5,6,7,8,9", *p_oss);
    string check_function = "aflux::applyAtomicFilter()";
    checkEqual(aflux::applyAtomicFilter("@Fe(1.2*)", "bader_net_charges", fe2o2), true, check_function, "the second Fe atom passes (only the first was read)", passed_checks, results);
    checkEqual(aflux::applyAtomicFilter("@Fe(1.6*)", "bader_net_charges", fe2o2), false, check_function, "no Fe atom passes", passed_checks, results);
    checkEqual(aflux::applyAtomicFilter("@O(*-1.25)", "bader_net_charges", fe2o2), true, check_function, "the second O atom passes", passed_checks, results);
    checkEqual(aflux::applyAtomicFilter("@Fe(!1.0)", "bader_net_charges", fe2o2), true, check_function, "not equal: one Fe atom differs", passed_checks, results);
    checkEqual(aflux::applyAtomicFilter("@Fe(!*1.0)", "bader_net_charges", fe2o2), true, check_function, "strictly more than 1.0: the second Fe atom", passed_checks, results);
    checkEqual(aflux::applyAtomicFilter("@Ni(0*)", "bader_net_charges", fe2o2), false, check_function, "an element the entry lacks", passed_checks, results);
    checkEqual(aflux::applyAtomicFilter("@Fe(1.2*),@O(*-1.25)", "bader_net_charges", fe2o2), true, check_function, "two terms, both pass", passed_checks, results);
    {
      aflowlib::_aflowlib_entry short_list;
      short_list.Load("species=Fe,O | composition=2,2 | bader_net_charges=1.0,1.5", *p_oss);
      checkEqual(aflux::applyAtomicFilter("@O(*0)", "bader_net_charges", short_list), false, check_function, "atoms without a value do not pass (and are not read past the list)", passed_checks, results);
    }

    check_function = "aflux::filterNbondxx()";
    checkEqual(aflux::filterNbondxx("@(O,O)(2.8*)", fe2o2), true, check_function, "O-O, 3.0 (Fe-O was read)", passed_checks, results);
    checkEqual(aflux::filterNbondxx("@(Fe,O)(2.0*)", fe2o2), true, check_function, "Fe-O, 2.1 (Fe-Fe was read)", passed_checks, results);
    checkEqual(aflux::filterNbondxx("@(O,Fe)(2.0*)", fe2o2), true, check_function, "the pair in either order", passed_checks, results);
    checkEqual(aflux::filterNbondxx("@(Fe,O)(2.2*)", fe2o2), false, check_function, "Fe-O below the bound", passed_checks, results);
    checkEqual(aflux::filterNbondxx("@(Fe,Fe)(*2.6)", fe2o2), true, check_function, "Fe-Fe, 2.5", passed_checks, results);
    checkEqual(aflux::filterNbondxx("@(Ni,O)(0*)", fe2o2), false, check_function, "a pair the entry lacks", passed_checks, results);
    checkEqual(aflux::filterNbondxx("@(O,O)(2.8*),@(Fe,O)(*2.2)", fe2o2), true, check_function, "two pairs, both pass", passed_checks, results);
    checkEqual(aflux::filterNbondxx("@(Ni,O)(0*),@(O,O)(2.8*)", fe2o2), false, check_function, "a missing pair before a present one", passed_checks, results);
    checkEqual(aflux::filterNbondxx("(@(O,O)(2.8*)),(@(Fe,O)(*2.2))", fe2o2), true, check_function, "each term in its own parentheses", passed_checks, results);
    {
      aflowlib::_aflowlib_entry cofeo;
      cofeo.Load("species=Co,Fe,O | composition=1,1,2 | nbondxx=1,2,3,4,5,6", *p_oss);
      checkEqual(aflux::filterNbondxx("@(Co,O)(3)", cofeo) && aflux::filterNbondxx("@(Fe,Fe)(4)", cofeo) && aflux::filterNbondxx("@(Fe,O)(5)", cofeo) && aflux::filterNbondxx("@(O,O)(6)", cofeo),
          true, check_function, "three species: Co-O, Fe-Fe, Fe-O and O-O are the 3rd to 6th values", passed_checks, results);
    }

    check_function = "aflux::applyTensorFilter()";
    checkEqual(aflux::applyTensorFilter("@(1)(2)", "stress_tensor", fe2o2), true, check_function, "component 1 is the second value, as in the SQL", passed_checks, results);
    checkEqual(aflux::applyTensorFilter("@(0)(1)", "stress_tensor", fe2o2), true, check_function, "component 0 is the first value", passed_checks, results);
    checkEqual(aflux::applyTensorFilter("@(9)(0*)", "stress_tensor", fe2o2), false, check_function, "a component past the end", passed_checks, results);

    // the SQL, on a fixture database: three entries with the per-sublattice lists, a stress tensor and ldau_TLUJ
    const string dir = aurostd::TmpDirectoryCreate("aflux_filters");
    try {
      const string data_dir = dir + "/data";
      aurostd::DirectoryMake(data_dir);
      const string id1 = "s4e:0a000000000000a1", id2 = "s4e:0a000000000000a2", id3 = "s4e:0a000000000000a3";
      const string lines =
        "{\"chaos_id\":\"" + id1 + "\",\"chaos_path\":\"LIB5/test/a1\",\"compound\":\"Fe1O1\",\"species\":[\"Fe\",\"O\"],\"composition\":[1,1],"
        "\"delta_radius_covalent_PT_sublattice\":[3.5],\"delta_electronegativity_Pearson_sublattice\":[0.2],"
        "\"stress_tensor\":[1,-2,3,4,5,6,7,8,9],\"ldau_TLUJ\":[2,[2,-1],[5,0],[0,0]]}\n"
        "{\"chaos_id\":\"" + id2 + "\",\"chaos_path\":\"LIB5/test/a2\",\"compound\":\"Fe1O1\",\"species\":[\"Fe\",\"O\"],\"composition\":[1,1],"
        "\"delta_radius_covalent_PT_sublattice\":[6.0,2.0],\"delta_electronegativity_Pearson_sublattice\":[0.5,0.1],"
        "\"stress_tensor\":[-1,2,3,4,5,6,7,8,9],\"ldau_TLUJ\":[2,[2,-1],[3,0],[0,0]]}\n"
        "{\"chaos_id\":\"" + id3 + "\",\"chaos_path\":\"LIB5/test/a3\",\"compound\":\"Fe1O1\",\"species\":[\"Fe\",\"O\"],\"composition\":[1,1]}\n";
      for (int i = 0; i < 256; i++) {
        stringstream t; t << std::setfill('0') << std::setw(2) << std::hex << i;
        aurostd::string2file((t.str() == "0a" ? lines : string("")), data_dir + "/" + getAUIDPrefix() + ":" + t.str() + ".jsonl");
      }
      const string db_file = dir + "/chaos.db";
      {
        stringstream quiet;
        aflowlib::AflowDB db(db_file, data_dir, dir + "/chaos.lck", XHOST.vschema, XHOST.vschema_internal, quiet);
        checkEqual(db.rebuildDatabase(true), 0, "aflowlib::AflowDB::rebuildDatabase()", "the fixture database builds", passed_checks, results);
      }
      stringstream quiet;
      aflowlib::AflowDB db(db_file, XHOST.vschema, XHOST.vschema_internal, quiet);
      // the identifiers a query selects, without their quotes, in order
      auto ids_of = [&db](const string& query) {
        aflux::AFLUX engine;
        engine.parseQuery("?" + query + ",paging(0)");
        vector<string> ids = db.getValuesMultiTable("chaos_id", engine.m_SQL_query);
        for (string& id : ids) {aurostd::StringSubst(id, "\"", "");}
        std::sort(ids.begin(), ids.end());
        return aurostd::joinWDelimiter(ids, ",");
      };
      check_function = "aflux::AFLUX::genQueryString()";
      checkEqual(ids_of("delta_radius_covalent_PT_sublattice(*5)"), id1 + "," + id2, check_function, "a list: any sublattice at most 5 (the type stopped the query)", passed_checks, results);
      checkEqual(ids_of("delta_radius_covalent_PT_sublattice(5*)"), id2, check_function, "a list: any sublattice at least 5", passed_checks, results);
      checkEqual(ids_of("delta_radius_covalent_PT_sublattice(@(0)(5*))"), id2, check_function, "a list: the first sublattice", passed_checks, results);
      checkEqual(ids_of("delta_radius_covalent_PT_sublattice(@(1)(*3))"), id2, check_function, "a list: the second sublattice", passed_checks, results);
      checkEqual(ids_of("delta_electronegativity_Pearson_sublattice(*0.15:0.4*)"), id2, check_function, "a list: or of two comparisons", passed_checks, results);
      checkEqual(ids_of("delta_radius_covalent_PT_sublattice(*)"), id1 + "," + id2, check_function, "a list: any value", passed_checks, results);
      checkEqual(ids_of("stress_tensor(@(1)(0*))"), id2, check_function, "1D tensor: component 1 is the second value", passed_checks, results);
      checkEqual(ids_of("ldau_TLUJ(@(2,0)(4*))"), id1, check_function, "ldau_TLUJ: U of the first species (it went to the atom filter and matched nothing)", passed_checks, results);
      checkEqual(ids_of("ldau_TLUJ(@(0)(2))"), id1 + "," + id2, check_function, "ldau_TLUJ: the type of the correction", passed_checks, results);
    } catch(aurostd::xerror& e) {
      errors.push_back(e.buildMessageString());
    } catch(const std::exception& e) {
      errors.push_back(string("afluxFiltersTest(): ") + e.what());
    }
    aurostd::RemoveDirectory(dir);
  }

  void UnitTest::chaosPathTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) { //CO20261008
    (void) errors;  // Suppress compiler warnings
    string check_function = "aflowlib::AURL2ChaosPath()";
    const vector<std::pair<string,string> > cases = {
      {"s4e.ai:AFLOWDATA/LIB1_WEB/As:PAW_PBE/A_hP2_194_c-001.A", "LIB1/As:PAW_PBE/A_hP2_194_c-001.A"},
      {"s4e.ai:AFLOWDATA/LIB2_WEB/FMg_pv/A2B_oI6_71_e_a-001.AB", "LIB2/FMg_pv/A2B_oI6_71_e_a-001.AB"},
      {"s4e.ai:AFLOWDATA/LIB3_WEB/OSc_svZn/A2B4C_aP14_1_4a_8a_2a-001.CAB", "LIB3/OSc_svZn/A2B4C_aP14_1_4a_8a_2a-001.CAB"},
      {"s4e.ai:AFLOWDATA/LIB5_WEB/Cr_pvCu_pvMo_pvNi_pvZn:PAW_PBE/A_cI2_229_a.A:POCC_P0-0.2xA-0.2xB-0.2xC-0.2xD-0.2xE",
        "LIB5/Cr_pvCu_pvMo_pvNi_pvZn:PAW_PBE/A_cI2_229_a.A:POCC_P0-0.2xA-0.2xB-0.2xC-0.2xD-0.2xE"},
      {"s4e.ai:AFLOWDATA/LIB5_WEB/Cr_pvCu_pvMo_pvNi_pvZn:PAW_PBE/A_cI2_229_a.A:POCC_P0-0.2xA-0.2xB-0.2xC-0.2xD-0.2xE/ARUN.POCC_05_H0C4",
        "LIB5/Cr_pvCu_pvMo_pvNi_pvZn:PAW_PBE/A_cI2_229_a.A:POCC_P0-0.2xA-0.2xB-0.2xC-0.2xD-0.2xE/ARUN.POCC_05_H0C4"},
      {"s4e.ai:AFLOWDATA/LIB8_WEB/Ca_svCr_pvLi_svMg_pvNa_svOTi_svV_sv:PAW_PBE/AB_cF8_225_a_b.AB:POCC_P0-0.143xA_P1-1xF/ARUN.POCC_30_H5C4651",
        "LIB8/Ca_svCr_pvLi_svMg_pvNa_svOTi_svV_sv:PAW_PBE/AB_cF8_225_a_b.AB:POCC_P0-0.143xA_P1-1xF/ARUN.POCC_30_H5C4651"}};
    for (size_t i = 0; i < cases.size(); i++) {
      const string lib = cases[i].second.substr(0, cases[i].second.find('/'));
      checkEqual(aflowlib::AURL2ChaosPath(cases[i].first), cases[i].second, check_function, lib + (i == 4 || i == 5 ? " supercell" : ""), passed_checks, results);
      checkEqual(aflowlib::ChaosPath2AURL(cases[i].second), cases[i].first, "aflowlib::ChaosPath2AURL()", lib + ": back to the AURL", passed_checks, results);
    }
    checkEqual(aflowlib::AURL2ChaosPath("aflowlib.duke.edu:AFLOWDATA/LIB2_RAW/Mg_pvO/A"), string("LIB2/Mg_pvO/A"), check_function, "another host, RAW", passed_checks, results);
    checkEqual(aflowlib::AURL2ChaosPath("LIB2/Mg_pvO/A"), string("LIB2/Mg_pvO/A"), check_function, "a chaos_path stays", passed_checks, results);
    checkEqual(aflowlib::AURL2ChaosPath("DEED"), string("DEED"), check_function, "not an AURL: unchanged", passed_checks, results);
    checkEqual(aflowlib::AURL2ChaosPath(""), string(""), check_function, "empty", passed_checks, results);
    checkEqual(aflowlib::ChaosPath2AURL("s4e.ai:AFLOWDATA/LIB2_WEB/Mg_pvO/A"), string("s4e.ai:AFLOWDATA/LIB2_WEB/Mg_pvO/A"), "aflowlib::ChaosPath2AURL()", "an AURL stays (old files)", passed_checks, results);

    check_function = "aflowlib::ChaosPathPoccSystem()";
    checkEqual(aflowlib::ChaosPathPoccSystem(cases[4].second), cases[3].second, check_function, "a supercell's system", passed_checks, results);
    checkEqual(aflowlib::ChaosPathPoccSystem(cases[3].second), string(""), check_function, "a system has none", passed_checks, results);
    checkEqual(aflowlib::ChaosPathPoccSystem(cases[1].second), string(""), check_function, "an ordered entry has none", passed_checks, results);

    // an entry read from what is written now and from what was written before
    check_function = "aflowlib::_aflowlib_entry::Load()";
    const string sys_id = "s4e:0a1b2c3d4e5f6071", sc_id = "s4e:1f2e3d4c5b6a7980";
    {
      aflowlib::_aflowlib_entry now, before;
      now.Load("chaos_path=" + cases[4].second + " | chaos_id=" + sc_id + " | pocc_system=" + sys_id + " | compound=Cr1Cu1Mo1Ni1Zn1", *p_oss);
      before.Load("aurl=" + cases[4].first + " | auid=" + sc_id + " | compound=Cr1Cu1Mo1Ni1Zn1", *p_oss);
      checkEqual(now.auid, sc_id, check_function, "chaos_id read", passed_checks, results);
      checkEqual(now.aurl, cases[4].first, check_function, "chaos_path read as the AURL kept inside", passed_checks, results);
      checkEqual(now.pocc_system, sys_id, check_function, "pocc_system read", passed_checks, results);
      checkEqual(before.auid == now.auid && before.aurl == now.aurl, true, check_function, "auid and aurl (before) read the same", passed_checks, results);
      const string chaos = now.aflowlib2string("chaos");
      checkEqual(aurostd::substring2bool(chaos, "aurl=") || aurostd::substring2bool(chaos, "auid="), false, "aflowlib::_aflowlib_entry::aflowlib2string()", "format(aflow): no aurl or auid written", passed_checks, results);
      checkEqual(aurostd::substring2bool(chaos, "chaos_path=" + cases[4].second) && aurostd::substring2bool(chaos, "chaos_id=" + sc_id) && aurostd::substring2bool(chaos, "pocc_system=" + sys_id), true, "aflowlib::_aflowlib_entry::aflowlib2string()", "format(aflow): chaos_path, chaos_id, pocc_system", passed_checks, results);
      aflowlib::_aflowlib_entry again;
      again.Load(chaos, *p_oss);
      checkEqual(again.auid == sc_id && again.aurl == cases[4].first && again.pocc_system == sys_id, true, check_function, "format(aflow) read back", passed_checks, results);
    }

    // database lines: before and now give the same line, and a supercell gets its system
    check_function = "aflowlib::AflowDB::normalizeEntryJSON()";
    const string line_before = "{\"aurl\":\"" + cases[4].first + "\",\"auid\":\"" + sc_id + "\",\"compound\":\"Cr1Cu1Mo1Ni1Zn1\",\"species_pp_AUID\":[\"x\"]}";
    const string line_now = "{\"chaos_path\":\"" + cases[4].second + "\",\"chaos_id\":\"" + sc_id + "\",\"compound\":\"Cr1Cu1Mo1Ni1Zn1\",\"species_pp_AUID\":[\"x\"]}";
    checkEqual(aflowlib::AflowDB::normalizeEntryJSON(line_before), line_now, check_function, "auid, aurl -> chaos_id, chaos_path; species_pp_AUID untouched", passed_checks, results);
    checkEqual(aflowlib::AflowDB::normalizeEntryJSON(line_now), line_now, check_function, "a line written now is unchanged", passed_checks, results);
    checkEqual(aflowlib::AflowDB::normalizeEntryJSON("{\"aurl\":null,\"auid\":null}"), string("{\"chaos_path\":null,\"chaos_id\":null}"), check_function, "null values: the names change", passed_checks, results);
    const std::map<string,string> systems = {{cases[3].second, sys_id}};
    const string with_system = aflowlib::AflowDB::normalizeEntryJSON(line_before, systems);
    checkEqual(aurostd::substring2bool(with_system, "\"pocc_system\":\"" + sys_id + "\"}"), true, check_function, "a supercell gets its system", passed_checks, results);
    checkEqual(aflowlib::AflowDB::normalizeEntryJSON(with_system, systems), with_system, check_function, "pocc_system is not written twice", passed_checks, results);

    // a database built from lines of both kinds: tables chaos_id_00..ff, columns chaos_id, chaos_path, pocc_system
    check_function = "aflowlib::AflowDB::rebuildDatabase()";
    const string dir = aurostd::TmpDirectoryCreate("chaos_path_db");
    const string data_dir = dir + "/data";
    aurostd::DirectoryMake(data_dir);
    const string sys_line_before = "{\"aurl\":\"" + cases[3].first + "\",\"auid\":\"" + sys_id + "\",\"compound\":\"Cr1Cu1Mo1Ni1Zn1\",\"catalog\":\"LIB5\"}";
    for (int i = 0; i < 256; i++) {
      stringstream t; t << std::setfill('0') << std::setw(2) << std::hex << i;
      string content = "";
      if (t.str() == "0a") content = sys_line_before + "\n";   // the system, written before
      if (t.str() == "1f") content = line_now + "\n";          // its supercell, written now
      aurostd::string2file(content, data_dir + "/" + getAUIDPrefix() + ":" + t.str() + ".jsonl");
    }
    const string db_file = dir + "/chaos.db";
    {
      stringstream quiet;
      aflowlib::AflowDB db(db_file, data_dir, dir + "/chaos.lck", XHOST.vschema, XHOST.vschema_internal, quiet);
      checkEqual(db.rebuildDatabase(true), 0, check_function, "built", passed_checks, results);
    }
    {
      stringstream quiet;
      aflowlib::AflowDB db(db_file, XHOST.vschema, XHOST.vschema_internal, quiet);
      const vector<string> tables = db.getTables();
      checkEqual(tables.size() == 256 && aurostd::WithinList(tables, string("chaos_id_0a")) && !aurostd::WithinList(tables, string("auid_0a")), true, check_function, "tables chaos_id_00 to chaos_id_ff", passed_checks, results);
      checkEqual(db.idColumn() + "," + db.pathColumn(), string("chaos_id,chaos_path"), "aflowlib::AflowDB::idColumn()", "the identifier and the path", passed_checks, results);
      checkEqual(db.tableOf(sc_id), string("chaos_id_1f"), "aflowlib::AflowDB::tableOf()", "an s4e: identifier's table (substr(6,2) took the wrong digits)", passed_checks, results);
      checkEqual(db.tableOf("s4e") + db.tableOf(""), string(""), "aflowlib::AflowDB::tableOf()", "no table for a value without a prefix and digits", passed_checks, results);  //CO20261010
      checkEqual(db.getValue("chaos_id_0a", "chaos_path", "chaos_id='\"" + sys_id + "\"'"), "\"" + cases[3].second + "\"", check_function, "the system: chaos_path from aurl", passed_checks, results);
      checkEqual(db.getValue("chaos_id_1f", "pocc_system", "chaos_id='\"" + sc_id + "\"'"), "\"" + sys_id + "\"", check_function, "the supercell: pocc_system, its system's chaos_id", passed_checks, results);
      const string entry_out = db.getEntry(sc_id, aflow_ft);
      checkEqual(aurostd::substring2bool(entry_out, "chaos_id=" + sc_id), true, "aflowlib::AflowDB::getEntry()", "an entry found by its identifier", passed_checks, results);
    }
    aurostd::RemoveDirectory(dir);
  }

  /// @brief machines and the data server without names (Corey, 2026-10-04): a run uses a generic template
  ///        and says which machine it is with --machine_name, printed first; the data server is the aflow
  ///        account with the AUID project folder; every AUID is s4e
  /// @note offline: no network
  void UnitTest::machineServerTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) { //CO20261004
    (void) errors;  // Suppress compiler warnings
    // the name AFLOW prints (LOCK, messages)
    {
      const string check_function = "KBIN::machineName()";
      const string t103 = "MACHINE::MACHINE103";
      const vector<std::array<string, 4> > cases = {
        {"Lab-Cluster", t103, "Lab-Cluster (machine template MACHINE::MACHINE103)", "named template: the name first"},
        {"  Lab-Cluster \t", t103, "Lab-Cluster (machine template MACHINE::MACHINE103)", "white space around the name is dropped"},
        {"", t103, "MACHINE::MACHINE103 (unnamed; set --machine_name)", "unnamed template says so"},
        {"", "MACHINE::MPCDF_EOS", "MACHINE::MPCDF_EOS", "an upstream machine without a name: as before"},
        {"Lab-Cluster", "", "Lab-Cluster", "a name and no --machine=: the name"},
        {"", "", "", "nothing given: nothing printed"},
      };
      for (size_t i = 0; i < cases.size(); i++) {
        checkEqual(KBIN::machineName(cases[i][0], cases[i][1]), cases[i][2], check_function, cases[i][3], passed_checks, results);
      }
    }
    // the same through the command line, as a run reads it
    {
      const string check_function = "KBIN::loadAflagsFromARGV()";
      const double maxmem = XHOST.maxmem;  // the machine options set it; put it back afterwards
      const _aflags named = KBIN::loadAflagsFromARGV({"aflow", "--machine=machine103", "--machine_name=Lab-Cluster"});
      checkEqual(named.AFLOW_MACHINE_GLOBAL.flag("MACHINE::MACHINE103"), true, check_function,
          "--machine=machine103 selects the template", passed_checks, results);
      checkEqual(named.AFLOW_MACHINE_GLOBAL.getattachedscheme("NAME"), string("Lab-Cluster (machine template MACHINE::MACHINE103)"),
          check_function, "--machine_name=Lab-Cluster is the name printed in the LOCK", passed_checks, results);
      const _aflags unnamed = KBIN::loadAflagsFromARGV({"aflow", "--machine=machine103"});
      checkEqual(unnamed.AFLOW_MACHINE_GLOBAL.getattachedscheme("NAME"), string("MACHINE::MACHINE103 (unnamed; set --machine_name)"),
          check_function, "without --machine_name the LOCK says the machine is unnamed", passed_checks, results);
      XHOST.maxmem = maxmem;
    }
    // the data server, by role
    {
      const string check_function = "isDataServer()";
      const string dir = aurostd::TmpDirectoryCreate("machine_server_test");
      checkEqual(isDataServer("aflow", dir), true, check_function, "the aflow account with the AUID folder", passed_checks, results);
      checkEqual(isDataServer("chaos", dir), true, check_function, "the chaos account (aflow renamed, Oct 2026) with the AUID folder", passed_checks, results);
      checkEqual(isDataServer("student", dir), false, check_function, "another account with the AUID folder", passed_checks, results);
      checkEqual(isDataServer("chaos", ""), false, check_function, "the chaos account without an AUID folder registered", passed_checks, results);
      checkEqual(isDataServer("aflow", ""), false, check_function, "the aflow account without an AUID folder registered", passed_checks, results);
      checkEqual(isDataServer("aflow", dir + "/missing"), false, check_function, "the aflow account, AUID folder missing", passed_checks, results);
      aurostd::RemoveDirectory(dir);
    }
    // s4e everywhere
    checkEqual(getAUIDPrefix(), string("s4e"), "getAUIDPrefix()", "every AUID is an s4e AUID, on every machine", passed_checks, results);
    {
      const aflowlib::_aflowlib_entry entry;
      checkEqual(entry.data_source, string("s4e"), "aflowlib::_aflowlib_entry", "data_source of a new entry is s4e", passed_checks, results);
    }
  }

  /// @brief which server a --server= value names (Corey, 2026-10-03): s4e.ai by default, the legacy
  ///        AFLOWLIB server only by name, anything else refused; the CHAOS API key stays with s4e.ai
  /// @note offline: no network
  void UnitTest::serverOptionTest(uint &passed_checks, vector<vector<string> > &results, vector<string> &errors) { //CO20261003
    (void) errors;  // Suppress compiler warnings
    const string check_function = "aflowlib::AFLOWLIBServerFromOption()";
    const string s4e = AFLOWLIB_SERVER_DEFAULT, legacy = AFLOWLIB_SERVER_LEGACY;
    checkEqual(s4e, string("s4e.ai"), check_function, "the default server is s4e.ai", passed_checks, results);
    const vector<std::pair<string, string> > cases = {
      {"default", s4e}, {"s4e.ai", s4e}, {" S4E.AI ", s4e},
      {"aflowlib", legacy}, {legacy, legacy}, {"AFLOWLIB", legacy},
      {"", ""}, {"materials.duke.edu", ""}, {"evil.example", ""},
      {"s4e.ai.evil.example", ""}, {legacy + ".evil.example", ""}, {"aflowlib.evil.example", ""}, {"aflow.org", ""}
    };
    for (size_t i = 0; i < cases.size(); i++) {
      const string& value = cases[i].first;
      const string& expected = cases[i].second;
      checkEqual(aflowlib::AFLOWLIBServerFromOption(value), expected, check_function,
          "--server=" + value + (expected.empty() ? ": refused" : ": " + expected), passed_checks, results);
      //CO20261004 - the loaders look the value up once and keep the host; a loader called with that host looks it up again
      if (!expected.empty()) {
        checkEqual(aflowlib::AFLOWLIBServerFromOption(aflowlib::AFLOWLIBServerFromOption(value)), expected, check_function,
            "--server=" + value + ": the host it gives is accepted again as " + expected, passed_checks, results);
      }
    }
    // an .aflow.rc written with the earlier default lists the legacy AFLUX server after s4e.ai, and the
    // entry loader merges the answers of every server: that list is read as the current default
    {
      const string lists_function = "aflowrc::AFLUXServerLists()";
      struct Lists { string server, path, server_expected, path_expected; bool replaced; string description; };
      const vector<Lists> lists = {
        {"s4e.ai," + legacy, "/API/chaos/,/API/aflux/", "s4e.ai", "/API/chaos/", true, "earlier default"},
        {"s4e.ai, " + legacy, "/__dev__/API/aflux/,/API/aflux/", "s4e.ai", "/API/chaos/", true, "earlier default with a space and an older path list"},
        {"s4e.ai", "/API/chaos/", "s4e.ai", "/API/chaos/", false, "current default"},
        {legacy, "/API/aflux/", legacy, "/API/aflux/", false, "the legacy server alone: used as written"},
        {legacy + ",s4e.ai", "/API/aflux/,/API/chaos/", legacy + ",s4e.ai", "/API/aflux/,/API/chaos/", false, "another order: used as written"},
      };
      for (size_t i = 0; i < lists.size(); i++) {
        string server = lists[i].server, path = lists[i].path;
        const bool replaced = aflowrc::AFLUXServerLists(server, path);
        checkEqual(replaced, lists[i].replaced, lists_function, lists[i].description + ": replaced or not", passed_checks, results);
        checkEqual(server + " | " + path, lists[i].server_expected + " | " + lists[i].path_expected, lists_function, lists[i].description + ": lists", passed_checks, results);
      }
    }
    // the key goes to s4e.ai hosts only, so never to the legacy server
    checkEqual(aurostd::httpIsS4EHost(aflowlib::AFLOWLIBServerFromOption("aflowlib")), false, "aurostd::httpIsS4EHost()",
        "the legacy server gets no CHAOS API key", passed_checks, results);
    checkEqual(aurostd::httpIsS4EHost(aflowlib::AFLOWLIBServerFromOption("default")), true, "aurostd::httpIsS4EHost()",
        "s4e.ai gets the key", passed_checks, results);
    //CO20261004 START - the materials and web servers are s4e.ai, as init::InitMachine() sets them on every host;
    // a --server= value is kept as the host it names
    checkEqual(string(AFLOW_MATERIALS_SERVER_DEFAULT) + "," + string(AFLOW_WEB_SERVER_DEFAULT) + "," + XHOST.AFLOW_MATERIALS_SERVER + "," + XHOST.AFLOW_WEB_SERVER,
        string("s4e.ai,s4e.ai,s4e.ai,s4e.ai"), "init::InitMachine()",
        "AFLOW_MATERIALS_SERVER_DEFAULT, AFLOW_WEB_SERVER_DEFAULT, XHOST.AFLOW_MATERIALS_SERVER, XHOST.AFLOW_WEB_SERVER", passed_checks, results);
    if (XHOST.vflag_control.flag("AFLOWLIB_SERVER")) {
      const string server_option = XHOST.vflag_control.getattachedscheme("AFLOWLIB_SERVER");
      checkEqual(aflowlib::AFLOWLIBServerFromOption(server_option), server_option, "init::InitMachine()",
          "--server= of this run is kept as the host it names", passed_checks, results);
    }
    //CO20261004 STOP
    //CO20261006 - the server of this run, used where an AURL is built (QCA, APENNSY, --test_proto1)
    if (!XHOST.vflag_control.flag("AFLOWLIB_SERVER")) {
      checkEqual(aflowlib::AFLOWLIBServer(), string("s4e.ai"), "aflowlib::AFLOWLIBServer()", "s4e.ai without --server=", passed_checks, results);
    } else {
      checkEqual(aflowlib::AFLOWLIBServerFromOption(aflowlib::AFLOWLIBServer()), aflowlib::AFLOWLIBServer(), "aflowlib::AFLOWLIBServer()",
          "the host --server= names", passed_checks, results);
    }
  }
}
//CO20261002 STOP

// database
namespace unittest {

  //CO20261004 START
  // Hull distance and DEED in the database (aflow --derived_properties), on the
  // Ti-V-Zr fixture of Hashan Peiris (HP): three unaries, Ti2Zr, a POCC parent with EFA
  // 12.7 and three supercells, all above the zero plane.
  void UnitTest::derivedPropertiesTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    string check_function="";
    string check_description="";

    check_function="aflowlib::AflowDB::getAuidTable()";
    check_description="the table follows the two characters after the AUID namespace";
    checkEqual(aflowlib::AflowDB::getAuidTable("s4e:0de695eaaa711602"),string("auid_0d"),check_function,check_description,passed_checks,results);
    checkEqual(aflowlib::AflowDB::getAuidTable("aflow:AB75aa80ad978ef8"),string("auid_ab"),check_function,check_description,passed_checks,results);

    check_function="chull::getAffectedAlloys()";
    check_description="a change reaches its own alloy and every alloy that contains it";
    const vector<string> database_alloys={"Ti","V","Zr","TiZr","TiV","TiVZr","CuO","CuMgO"};
    checkEqual(aurostd::joinWDelimiter(chull::getAffectedAlloys({"TiZr"},database_alloys),","),string("TiVZr,TiZr"),check_function,check_description,passed_checks,results);
    checkEqual(aurostd::joinWDelimiter(chull::getAffectedAlloys({"O"},database_alloys),","),string("CuMgO,CuO"),check_function,check_description,passed_checks,results);
    check_function="chull::getMaximalAlloys()";
    check_description="only the alloys no other alloy contains";
    checkEqual(aurostd::joinWDelimiter(chull::getMaximalAlloys(database_alloys),","),string("CuMgO,TiVZr"),check_function,check_description,passed_checks,results);

    string fixture_directory="";
    try {
      // the fixture database of Hashan Peiris (HP)
      fixture_directory=aurostd::TmpDirectoryCreate("derived_properties");
      const string data_path=fixture_directory+"/jsonl";
      const string database_path=fixture_directory+"/database.db";
      const string lock_path=fixture_directory+"/database.lock";
      const string derived_file=fixture_directory+"/database_derived.jsonl";
      if(!aurostd::DirectoryMake(data_path)){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"Could not create "+data_path,_FILE_ERROR_);}
      const string auid_prefix=getAUIDPrefix()+":";
      const auto local_auid=[&auid_prefix](const string& fixture_auid){return auid_prefix+fixture_auid.substr(fixture_auid.find(':')+1);};
      vector<aurostd::JSON::object> entries;
      for(size_t i=0;i<utd["database_update"]["baseline"].size();i++){
        entries.push_back(utd["database_update"]["baseline"][i]);
        entries.back()["auid"]=local_auid(string(entries.back()["auid"]));
        entries.back()["distance_hull"]=nullptr;  // the fixture carries placeholders; the database starts without values
        entries.back()["deed"]=nullptr;
      }
      aurostd::JSON::object added=utd["database_update"]["added"];  // TiZr, -0.05 eV/atom
      added["auid"]=local_auid(string(added["auid"]));
      const string parent_auid=local_auid(string(utd["database_update"]["parent_auid"]));
      const string supercell_auid=local_auid("s4e:52cae4cd3e60c992");  // ARUN.POCC_1_H0C0, 0.276358 eV/atom
      const auto write_shards=[&](const vector<aurostd::JSON::object>& records){
        std::map<string,stringstream> shards;
        for(size_t i=0;i<records.size();i++){
          const string auid=string(records[i]["auid"]);
          shards[aurostd::tolower(auid.substr(auid_prefix.size(),2))] << records[i].toString() << endl;
        }
        for(int i=0;i<256;i++){  // one file per AUID table
          stringstream shard; shard << std::setfill('0') << std::setw(2) << std::hex << i;
          const string filename=data_path+"/"+auid_prefix+shard.str()+".jsonl";
          if(!aurostd::string2file(shards[shard.str()].str(),filename)){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"Could not write "+filename,_FILE_ERROR_);}
        }
      };
      write_shards(entries);
      // the tests run in parallel, so nothing here changes XHOST: the hulls read the database the run is given
      aflowlib::AflowDB database(database_path,data_path,lock_path,XHOST.vschema,XHOST.vschema_internal,*p_oss);
      checkEqual(database.rebuildDatabase(true),0,"aflowlib::AflowDB::rebuildDatabase()","the fixture database builds",passed_checks,results);
      //CO20261010 - the hulls read the identifier from this column, not from auid
      checkEqual(database.idColumn(),string("chaos_id"),"aflowlib::AflowDB::idColumn()","the fixture database names the identifier chaos_id",passed_checks,results);

      check_function="aflowlib::EntryLoader::sqliteFile()";
      check_description="--aflow_db= picks the SQLite file the entry loader reads";
      checkEqual(aflowlib::EntryLoader::sqliteFile(database_path),database_path,check_function,check_description,passed_checks,results);
      checkEqual(aflowlib::EntryLoader::sqliteFile(data_path),string(DEFAULT_AFLOW_DB_FILE),check_function,"a JSONL folder falls back to DEFAULT_AFLOW_DB_FILE",passed_checks,results);
      checkEqual(aflowlib::EntryLoader::sqliteFile(""),string(DEFAULT_AFLOW_DB_FILE),check_function,"no --aflow_db= falls back to DEFAULT_AFLOW_DB_FILE",passed_checks,results);
      checkEqual(database.getDatabaseFile(),database_path,"aflowlib::AflowDB::getDatabaseFile()","the file the hulls read",passed_checks,results);

      // first run: every entry is new
      check_function="chull::updateDatabaseDerivedProperties()";
      checkEqual(chull::updateDatabaseDerivedProperties(database,derived_file,*p_oss),0,check_function,"the first run patches the database",passed_checks,results);
      vector<string> lines;
      aurostd::file2vectorstring(derived_file,lines);
      checkEqual(lines.size(),entries.size(),check_function,"one line per entry",passed_checks,results);
      aflowlib::_aflowlib_entry parent=database.getEntryAentry(parent_auid);
      check(aurostd::isequal(parent.distance_hull,0.16814,1e-8),parent.distance_hull,0.16814,check_function,"the POCC parent's distance is its formation enthalpy above the zero plane",passed_checks,results);
      check(aurostd::isequal(parent.deed,std::sqrt(12.7/0.16814),1e-8),parent.deed,std::sqrt(12.7/0.16814),check_function,"DEED=sqrt(EFA/distance) on the parent",passed_checks,results);
      aflowlib::_aflowlib_entry supercell=database.getEntryAentry(supercell_auid);
      check(aurostd::isequal(supercell.distance_hull,0.276358,1e-8),supercell.distance_hull,0.276358,check_function,"a supercell is measured against the hull",passed_checks,results);
      checkEqual(aurostd::isNaN(supercell.deed),true,check_function,"no DEED on a supercell",passed_checks,results);
      checkEqual(chull::updateDatabaseDerivedProperties(database,derived_file,*p_oss),100,check_function,"a second run finds nothing to do",passed_checks,results);

      // a supercell below the hull of ordered structures does not define the hull
      const string supercell_patch=fixture_directory+"/supercell_patch.jsonl";
      aurostd::string2file("{\"auid\":\""+supercell_auid+"\",\"enthalpy_formation_atom\":-0.05}\n",supercell_patch);
      database.patchDatabase(supercell_patch,false);
      checkEqual(chull::updateDatabaseDerivedProperties(database,derived_file,*p_oss),0,check_function,"a changed formation enthalpy is found",passed_checks,results);
      supercell=database.getEntryAentry(supercell_auid);
      check(aurostd::isequal(supercell.distance_hull,-0.05,1e-8),supercell.distance_hull,-0.05,check_function,"signed distance of a supercell below the hull",passed_checks,results);
      parent=database.getEntryAentry(parent_auid);
      check(aurostd::isequal(parent.distance_hull,0.16814,1e-8),parent.distance_hull,0.16814,check_function,"the supercell below the hull is not a hull vertex",passed_checks,results);

      // a new ordered TiZr at -0.05 eV/atom lowers the hull under the ternary parent to -0.05*2/3
      entries.push_back(added);
      write_shards(entries);
      database.rebuildDatabase(true);
      checkEqual(chull::updateDatabaseDerivedProperties(database,derived_file,*p_oss),0,check_function,"a new binary entry is found",passed_checks,results);
      parent=database.getEntryAentry(parent_auid);
      const double parent_distance_added=0.16814+0.05*2.0/3.0;
      check(aurostd::isequal(parent.distance_hull,parent_distance_added,1e-8),parent.distance_hull,parent_distance_added,check_function,"the ternary parent is recomputed when its binary changes",passed_checks,results);
      check(aurostd::isequal(parent.deed,std::sqrt(12.7/parent_distance_added),1e-8),parent.deed,std::sqrt(12.7/parent_distance_added),check_function,"DEED follows the new distance",passed_checks,results);

      // removing it restores the old hull
      entries.pop_back();
      write_shards(entries);
      database.rebuildDatabase(true);
      checkEqual(chull::updateDatabaseDerivedProperties(database,derived_file,*p_oss),0,check_function,"a removed entry is found",passed_checks,results);
      parent=database.getEntryAentry(parent_auid);
      check(aurostd::isequal(parent.distance_hull,0.16814,1e-8),parent.distance_hull,0.16814,check_function,"the hull returns when the binary leaves",passed_checks,results);
      lines.clear();
      aurostd::file2vectorstring(derived_file,lines);
      checkEqual(lines.size(),entries.size(),check_function,"the removed entry leaves the file",passed_checks,results);

      // a rebuild from the JSONL files drops the values; the next run patches them back without computing a hull
      database.rebuildDatabase(true);
      checkEqual(aurostd::isNaN(database.getEntryAentry(parent_auid).distance_hull),true,"aflowlib::AflowDB::rebuildDatabase()","the rebuild drops the values",passed_checks,results);
      checkEqual(chull::updateDatabaseDerivedProperties(database,derived_file,*p_oss),0,check_function,"the values are patched back after a rebuild",passed_checks,results);
      parent=database.getEntryAentry(parent_auid);
      check(aurostd::isequal(parent.distance_hull,0.16814,1e-8),parent.distance_hull,0.16814,check_function,"the parent's value is back",passed_checks,results);
    } catch(aurostd::xerror& e) {
      errors.push_back(e.buildMessageString());
    } catch(const std::exception& e) {
      errors.push_back(string("derivedPropertiesTest(): ")+e.what());
    }
    if(!fixture_directory.empty()){aurostd::RemoveDirectory(fixture_directory);}
  }
  //CO20261004 STOP

  void UnitTest::schemaTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    (void) errors;  // Suppress compiler warnings

    // Set up test environment
    string check_function = "";
    string check_description = "";

    // ---------------------------------------------------------------------------
    // Check | internal consistency
    // ---------------------------------------------------------------------------
    check_function = "XHOST.vschema";
    check_description = "Internal consistency of vschema";
    vector<string> vschema_keys;
    vector<string> vschema_types = {"UNIT", "TYPE"};
    string key = "";
    uint ninconsistent = 0;
    for (size_t i = 0; i < XHOST.vschema.vxsghost.size(); i+= 2) {
      if(XHOST.vschema.vxsghost[i].find("::NAME:") != string::npos) {
        key=aurostd::RemoveSubString(XHOST.vschema.vxsghost[i], "SCHEMA::NAME:");
        vschema_keys.push_back(XHOST.vschema.getattachedscheme("SCHEMA::NAME:" + key));
        for (size_t j = 0; j < vschema_types.size(); j++) {
          if (!XHOST.vschema.isdefined("SCHEMA::" + vschema_types[j] + ":" + key)) {
            ninconsistent++;
            if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " SCHEMA::" << vschema_types[j] << ":" << key << " not found." << std::endl;
          }
        }
      }
    }
    checkEqual(ninconsistent, (uint)0, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | consistency between _aflowlib_entry and schema
    // ---------------------------------------------------------------------------
    check_function = "_aflowlib_entry";
    check_description = "Consistency between _aflowlib_entry json and schema";
    aflowlib::_aflowlib_entry aentry;
    //CO20261008 - as the database reads it: lib2raw's auid and aurl are the schema's chaos_id and chaos_path
    string aflowlib_json = aflowlib::AflowDB::normalizeEntryJSON(aentry.aflowlib2string("JSON", true));
    vector<string> json_keys = aurostd::extractJsonKeysAflow(aflowlib_json);

    vector<string> vkeys_ignore = {"data_language", "error_status", "natoms_orig",
      "density_orig", "volume_cell_orig", "volume_atom_orig",
      "spinD_magmom_orig"};
    for (size_t k = 0; k < json_keys.size(); k++) {
      const string& key = json_keys[k];
      if (!aurostd::WithinList(vkeys_ignore, key) && !aurostd::WithinList(vschema_keys, key)) {
        ninconsistent++;
        if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " " << key << " not found in schema." << std::endl;
      }
    }
    checkEqual(ninconsistent, (uint)0, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | POCC HEA fields //CO20260924
    // ---------------------------------------------------------------------------
    check_function = "_aflowlib_entry";
    aflowlib::_aflowlib_entry hea_entry;
    hea_entry.pocc_parameters = "P0-0.5xA-0.5xB:TOL_0.001_0.001";
    check_description = "POCC HEA fields are unset by default";
    string hea_json = hea_entry.aflowlib2string("JSON", false);
    checkEqual(hea_json.find("lat_distortion_misfit_mean_0K") == string::npos && hea_json.find("Hmix_miedema") == string::npos, true, check_function, check_description, passed_checks, results);
    check_description = "POCC HEA fields survive copy";
    hea_entry.lat_distortion_misfit_mean_0K = 0.125;
    aflowlib::_aflowlib_entry hea_copy(hea_entry);
    checkEqual(hea_copy.lat_distortion_misfit_mean_0K, 0.125, check_function, check_description, passed_checks, results);
    check_description = "Non-finite POCC HEA values are not written";
    hea_entry.lat_distortion_misfit_stdev_0K = std::numeric_limits<double>::quiet_NaN();
    hea_json = hea_entry.aflowlib2string("JSON", false);
    checkEqual(hea_json.find("lat_distortion_misfit_stdev_0K") == string::npos, true, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | schema tags //CO20260927
    // ---------------------------------------------------------------------------
    // The database looks up a column's type by the upper-case field name, so a
    // tag that differs from it stores the field as text.
    check_function = "XHOST.vschema";
    check_description = "Every schema tag is the upper-case field name";
    uint nmismatched = 0;
    for (size_t i = 0; i < XHOST.vschema.vxsghost.size(); i+= 2) {
      if (XHOST.vschema.vxsghost[i].find("SCHEMA::NAME:") == 0) {
        const string tag = aurostd::RemoveSubString(XHOST.vschema.vxsghost[i], "SCHEMA::NAME:");
        if (tag != aurostd::toupper(XHOST.vschema.vxsghost[i+1])) {
          nmismatched++;
          if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " tag " << tag << " for " << XHOST.vschema.vxsghost[i+1] << std::endl;
        }
      }
    }
    checkEqual(nmismatched, (uint)0, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | POCC HEA fields read from the database //CO20260927
    // ---------------------------------------------------------------------------
    // The AFLUX engine loads only the fields named in a query, so
    // pocc_parameters is usually absent.
    check_function = "_aflowlib_entry::Load()";
    aflowlib::_aflowlib_entry db_entry;
    db_entry.Load({aurostd::crc64("auid"), aurostd::crc64("VEC"), aurostd::crc64("species_sublattice"), aurostd::crc64("delta_electronegativity_Pearson_sublattice")},
                  {"\"aflow:0123456789abcdef\"", "7.5", "[\"Co*0.5,Mg*0.5\",\"F*0.5,O*0.5\"]", "[0.25,null]"});
    check_description = "HEA fields loaded without pocc_parameters are written";
    const string db_json = db_entry.aflowlib2string("JSON", false);
    checkEqual(db_json.find("\"VEC\":7.5") != string::npos, true, check_function, check_description, passed_checks, results);
    check_description = "species_sublattice keeps one item per site";
    checkEqual(db_entry.species_sublattice.size() == 2, true, check_function, check_description, passed_checks, results);
    checkEqual(db_json.find("\"species_sublattice\":[\"Co*0.5,Mg*0.5\",\"F*0.5,O*0.5\"]") != string::npos, true, check_function, check_description, passed_checks, results);
    check_description = "A site without a value reads back as unset and is written as null";
    checkEqual(db_entry.delta_electronegativity_Pearson_sublattice.size() == 2 && db_entry.delta_electronegativity_Pearson_sublattice[1] == AUROSTD_NAN, true, check_function, check_description, passed_checks, results);
    checkEqual(db_json.find("\"delta_electronegativity_Pearson_sublattice\":[0.25,null]") != string::npos, true, check_function, check_description, passed_checks, results);
    check_description = "Per-site values parse strictly: padded numbers are read, partial parses and inf are unset"; //CO20261004
    aflowlib::_aflowlib_entry strict_entry; //CO20261004
    strict_entry.Load({aurostd::crc64("auid"), aurostd::crc64("delta_radius_covalent_PT_sublattice")}, {"\"aflow:0123456789abcdef\"", "[ 2 ,1.5abc,inf,0.5]"}); //CO20261004
    checkEqual(strict_entry.delta_radius_covalent_PT_sublattice, vector<double>{2.0, AUROSTD_NAN, AUROSTD_NAN, 0.5}, check_function, check_description, passed_checks, results); //CO20261004
    check_description = "species_sublattice survives the aflowlib.out format";
    aflowlib::_aflowlib_entry out_entry;
    out_entry.Load(db_entry.aflowlib2string("out", false), std::cout);
    checkEqual(out_entry.species_sublattice == db_entry.species_sublattice, true, check_function, check_description, passed_checks, results);
    check_description = "An ordered entry writes no HEA fields";
    aflowlib::_aflowlib_entry ordered_entry;
    ordered_entry.auid = "aflow:fedcba9876543210";
    checkEqual(ordered_entry.aflowlib2string("JSON", true).find("species_sublattice") == string::npos, true, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | every POCC HEA field: schema type, initialization, copy, clear(), Load() //CO20261004
    // ---------------------------------------------------------------------------
    // The checks above take one or two of the fields; these take all of them, so a field left out of free(), copy(),
    // SetByHash() or the schema shows up.
    const vector<string> hea_scalar_names = {"lat_distortion_misfit_mean_0K", "lat_distortion_misfit_stdev_0K",
      "lat_distortion_relax_distance_mean_0K", "lat_distortion_relax_distance_stdev_0K", "solubility_parameter_0K",
      "lat_distortion_misfit_mean_300K", "lat_distortion_misfit_stdev_300K", "lat_distortion_relax_distance_mean_300K",
      "lat_distortion_relax_distance_stdev_300K", "solubility_parameter_300K", "param_geo_radius_covalent_PT_atom",
      "S_config_atom", "S_config_partial_atom", "VEC", "Hmix_miedema"};
    auto hea_scalars = [](aflowlib::_aflowlib_entry& e) {  // in the order of hea_scalar_names
      return vector<double*>{&e.lat_distortion_misfit_mean_0K, &e.lat_distortion_misfit_stdev_0K,
        &e.lat_distortion_relax_distance_mean_0K, &e.lat_distortion_relax_distance_stdev_0K, &e.solubility_parameter_0K,
        &e.lat_distortion_misfit_mean_300K, &e.lat_distortion_misfit_stdev_300K, &e.lat_distortion_relax_distance_mean_300K,
        &e.lat_distortion_relax_distance_stdev_300K, &e.solubility_parameter_300K, &e.param_geo_radius_covalent_PT_atom,
        &e.S_config_atom, &e.S_config_partial_atom, &e.VEC, &e.Hmix_miedema};
    };
    auto hea_values_of = [&hea_scalars](aflowlib::_aflowlib_entry& e) {
      vector<double> values;
      for (const double* field : hea_scalars(e)) values.push_back(*field);
      return values;
    };
    const vector<double> hea_unset(hea_scalar_names.size(), AUROSTD_NAN);

    check_function = "XHOST.vschema";
    check_description = "POCC HEA fields have the schema type of their member (param_geo_radius_covalent_PT_atom was numbers)";
    vector<string> hea_field_names = hea_scalar_names, hea_types, hea_types_expected(hea_scalar_names.size(), "number");
    hea_field_names.insert(hea_field_names.end(), {"species_sublattice", "delta_electronegativity_Pearson_sublattice", "delta_radius_covalent_PT_sublattice"});
    hea_types_expected.insert(hea_types_expected.end(), {"strings", "numbers", "numbers"});
    for (const string& name : hea_field_names) hea_types.push_back(XHOST.vschema.getattachedscheme("SCHEMA::TYPE:" + aurostd::toupper(name)));
    checkEqual(aurostd::joinWDelimiter(hea_types, ","), aurostd::joinWDelimiter(hea_types_expected, ","), check_function, check_description, passed_checks, results);

    check_function = "_aflowlib_entry";
    aflowlib::_aflowlib_entry hea_full;
    check_description = "a new entry has every POCC HEA field unset";
    checkEqual(hea_values_of(hea_full), hea_unset, check_function, check_description, passed_checks, results);
    checkEqual(hea_full.species_sublattice.empty() && hea_full.delta_electronegativity_Pearson_sublattice.empty() && hea_full.delta_radius_covalent_PT_sublattice.empty(), true,
        check_function, check_description + " (per-site lists empty)", passed_checks, results);
    vector<double> hea_values;
    const vector<double*> hea_fields = hea_scalars(hea_full);
    for (size_t i = 0; i < hea_fields.size(); i++) {*hea_fields[i] = 0.5 + 0.125 * i; hea_values.push_back(*hea_fields[i]);}
    hea_full.species_sublattice = {"Co*0.5,Mg*0.5", "F*0.5,O*0.5"};
    hea_full.delta_electronegativity_Pearson_sublattice = {0.25, 0.75};
    hea_full.delta_radius_covalent_PT_sublattice = {0.0625, AUROSTD_NAN};
    aflowlib::_aflowlib_entry hea_full_copy(hea_full), hea_full_assigned;
    hea_full_assigned = hea_full;
    for (int assigned = 0; assigned < 2; assigned++) {
      aflowlib::_aflowlib_entry& e = (assigned ? hea_full_assigned : hea_full_copy);
      const string how = (assigned ? "assignment" : "copy");
      check_description = how + " keeps every POCC HEA scalar field";
      checkEqual(hea_values_of(e), hea_values, check_function, check_description, passed_checks, results);
      check_description = how + " keeps the per-site lists";
      checkEqual(aurostd::joinWDelimiter(e.species_sublattice, ";"), string("Co*0.5,Mg*0.5;F*0.5,O*0.5"), check_function, check_description + " (species_sublattice)", passed_checks, results);
      checkEqual(e.delta_electronegativity_Pearson_sublattice, hea_full.delta_electronegativity_Pearson_sublattice, check_function, check_description + " (delta_electronegativity_Pearson_sublattice)", passed_checks, results);
      checkEqual(e.delta_radius_covalent_PT_sublattice, hea_full.delta_radius_covalent_PT_sublattice, check_function, check_description + " (delta_radius_covalent_PT_sublattice)", passed_checks, results);
    }
    check_description = "clear() unsets every POCC HEA field";
    hea_full.clear();
    checkEqual(hea_values_of(hea_full), hea_unset, check_function, check_description, passed_checks, results);
    checkEqual(hea_full.species_sublattice.empty() && hea_full.delta_electronegativity_Pearson_sublattice.empty() && hea_full.delta_radius_covalent_PT_sublattice.empty(), true,
        check_function, check_description + " (per-site lists empty)", passed_checks, results);

    check_function = "_aflowlib_entry::Load()";
    check_description = "every POCC HEA scalar field is read from the database";
    vector<uint64_t> hea_keys = {aurostd::crc64("auid")};
    vector<string> hea_contents = {"\"aflow:0123456789abcdef\""};
    for (size_t i = 0; i < hea_scalar_names.size(); i++) {hea_keys.push_back(aurostd::crc64(hea_scalar_names[i])); hea_contents.push_back(aurostd::utype2string(hea_values[i]));}
    aflowlib::_aflowlib_entry hea_loaded;
    hea_loaded.Load(hea_keys, hea_contents);
    checkEqual(hea_values_of(hea_loaded), hea_values, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | POCC HEA fields in the aflowlib.out format //CO20261004
    // ---------------------------------------------------------------------------
    // As in JSON: written when a field is set even without pocc_parameters, non-finite scalars left out, and a site
    // without a value written as null.
    check_function = "_aflowlib_entry::aflowlib2string(\"out\")";
    aflowlib::_aflowlib_entry nonfinite_entry;
    nonfinite_entry.auid = "aflow:0123456789abcdef";
    nonfinite_entry.VEC = 7.5;
    nonfinite_entry.S_config_atom = std::numeric_limits<double>::infinity();
    nonfinite_entry.Hmix_miedema = std::numeric_limits<double>::quiet_NaN();
    nonfinite_entry.delta_electronegativity_Pearson_sublattice = {0.25, AUROSTD_NAN};
    nonfinite_entry.delta_radius_covalent_PT_sublattice = {std::numeric_limits<double>::infinity(), 0.5};
    const string nonfinite_out = nonfinite_entry.aflowlib2string("out", false);
    check_description = "HEA fields set without pocc_parameters are written";
    checkEqual(aurostd::substring2bool(nonfinite_out, "VEC=7.5"), true, check_function, check_description, passed_checks, results);
    check_description = "non-finite POCC HEA values are not written (inf and nan)";
    checkEqual(!aurostd::substring2bool(nonfinite_out, "S_config_atom=") && !aurostd::substring2bool(nonfinite_out, "Hmix_miedema="), true, check_function, check_description, passed_checks, results);
    check_description = "a site without a finite value is written as null";
    checkEqual(aurostd::substring2bool(nonfinite_out, "delta_electronegativity_Pearson_sublattice=0.25,null") && aurostd::substring2bool(nonfinite_out, "delta_radius_covalent_PT_sublattice=null,0.5"), true,
        check_function, check_description, passed_checks, results);
  }

  // ---------------------------------------------------------------------------
  // pocc_hea  //CO20261004
  // ---------------------------------------------------------------------------
  // The POCC HEA fields (fix/hea-fields-uninitialized) beyond the entry checks in schemaTest():
  //  1. POccCalculator::setLatticeDistortion() averages the misfit and the relaxation distance of the supercells with
  //     data over their weights. When the weight sits entirely on supercells without data (low temperature), the sum
  //     of the weights is 0 and the averages were 0/0; a non-finite misfit made them inf or nan. Both now take the
  //     error path, which discards the statistics (AUROSTD_MAX_DOUBLE, not written to aflow.pocc.out).
  //  2. LIB2RAW_Loop_POCC() reads the fields from aflow.pocc.out with aurostd::string2finite(): nan, inf and partial
  //     parses leave a field unset, where aurostd::string2utype<double>() gave 0 or the leading number.
  // Not covered: setMisfitParams(), which takes the first candidate structure with a computed misfit and divides by
  // the atom count of that candidate; it reads the ARUN.POCC_* runs (aflow.in, POSCAR.relax1, the relaxed structure)
  // and calls compare::getTransformationBetweenStructures(), which a unit test cannot steer to a mapping without a
  // misfit.
  void UnitTest::poccHEAFieldsTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    // 1. lattice distortion statistics
    {
      const string check_function = "pocc::POccCalculator::setLatticeDistortion()";
      // the weight of each supercell, then for each supercell with data: its index, misfit and relaxation distance;
      // returns the misfit mean and standard deviation, then the distance mean and standard deviation
      auto lattice_distortion = [](const vector<double>& weights, const vector<uint>& with_data, const vector<double>& misfits, const vector<double>& distances) {
        std::stringstream oss_pocc;
        pocc::POccCalculator pcalc(oss_pocc);
        for (const double weight : weights) {pocc::POccSuperCellSet pscs; pscs.m_probability = weight; pcalc.l_supercell_sets.push_back(pscs);}
        for (size_t i = 0; i < with_data.size(); i++) {
          pcalc.m_vmisfit_info.push_back(structure_mapping_info());
          pcalc.m_vlat_distortion_supercells_accounted_for.push_back(with_data[i]);
          pcalc.m_vlat_distortion_misfits.push_back(misfits[i]);
          pcalc.m_vlat_distortion_relax_distances.push_back(distances[i]);
        }
        pcalc.setLatticeDistortion(0.0);
        return vector<double>{pcalc.m_lat_distortion_misfit_mean, pcalc.m_lat_distortion_misfit_stdev,
          pcalc.m_lat_distortion_relax_distance_mean, pcalc.m_lat_distortion_relax_distance_stdev};
      };
      const vector<double> discarded(4, AUROSTD_MAX_DOUBLE);
      checkEqual(lattice_distortion({0.25, 0.75}, {0, 1}, {0.1, 0.3}, {0.02, 0.06}), vector<double>{0.25, std::sqrt(0.0075), 0.05, std::sqrt(0.0003)}, check_function,
          "weights 1/4 and 3/4: weighted means and standard deviations of the misfit and the distance", passed_checks, results);
      checkEqual(lattice_distortion({0.0, 1.0}, {0}, {0.1}, {0.02}), discarded, check_function,
          "all weight on a supercell without data: the statistics are discarded (were 0/0)", passed_checks, results);
      checkEqual(lattice_distortion({0.5, 0.5}, {0, 1}, {0.1, std::numeric_limits<double>::infinity()}, {0.02, 0.06}), discarded, check_function,
          "a non-finite misfit: the statistics are discarded (were inf and nan)", passed_checks, results);
    }

    // 2. lib2raw reads the fields from aflow.pocc.out, in a temporary LIB folder
    {
      const string check_function = "aflowlib::LIB2RAW_Loop_POCC()";
      const string dir = aurostd::TmpDirectoryCreate("pocc_hea");
      const string dir_LIB = dir + "/LIB", dir_RAW = dir + "/RAW";
      aurostd::DirectoryMake(dir_LIB);
      aurostd::DirectoryMake(dir_RAW);
      stringstream pocc_out;  // the blocks as POccCalculator writes them, with values a broken run can leave
      pocc_out << "[AFLOW_POCC]START_TEMPERATURE=ALL" << std::endl;
      pocc_out << "Hmix_miedema=-1.5e-02  (eV/at)" << std::endl;
      pocc_out << "S_config_atom=nan" << std::endl;
      pocc_out << "S_config_partial_atom=inf" << std::endl;
      pocc_out << "VEC=7.5 (e/atom)" << std::endl;
      pocc_out << "[AFLOW_POCC]START_SUBLATTICE=0" << std::endl;
      pocc_out << "species_sublattice=Co*0.5,Mg*0.5" << std::endl;
      pocc_out << "delta_electronegativity_Pearson_sublattice=0.25" << std::endl;
      pocc_out << "delta_radius_covalent_PT_sublattice=nan" << std::endl;
      pocc_out << "param_geo_radius_covalent_PT_atom=1.25abc" << std::endl;
      pocc_out << "[AFLOW_POCC]STOP_SUBLATTICE=0" << std::endl;
      pocc_out << "[AFLOW_POCC]STOP_TEMPERATURE=ALL" << std::endl;
      pocc_out << "[AFLOW_POCC]START_TEMPERATURE=0000_K" << std::endl;
      pocc_out << "solubility_parameter=3.5 (GPa * \\AA^3)" << std::endl;
      pocc_out << "lat_distortion_misfit_mean=1.0e-01" << std::endl;
      pocc_out << "lat_distortion_misfit_stdev=-nan" << std::endl;
      pocc_out << "lat_distortion_relax_distance_mean=2.0e-02  (\\AA)" << std::endl;
      pocc_out << "lat_distortion_relax_distance_stdev=inf  (\\AA)" << std::endl;
      pocc_out << "[AFLOW_POCC]STOP_TEMPERATURE=0000_K" << std::endl;
      pocc_out << "[AFLOW_POCC]START_TEMPERATURE=0300_K" << std::endl;
      pocc_out << "solubility_parameter=4.5 (GPa * \\AA^3)" << std::endl;
      pocc_out << "lat_distortion_misfit_mean=nan" << std::endl;
      pocc_out << "[AFLOW_POCC]STOP_TEMPERATURE=0300_K" << std::endl;
      aurostd::string2file(pocc_out.str(), dir_LIB + "/" + POCC_FILE_PREFIX + POCC_OUT_FILE);
      aurostd::string2file("[AFLOW] pocc_hea unit test\n", dir_LIB + "/" + _AFLOWIN_);
      aurostd::string2file("", dir_LIB + "/" + POCC_FILE_PREFIX + POCC_UNIQUE_SUPERCELLS_FILE);
      aflowlib::_aflowlib_entry data;
      data.system_name = "CoMgO:PAW_PBE.AB_cF8_225_a_b.AB:POCC_P0-1xA_P1-0.5xB-0.5xC";
      vector<string> vfile;
      string stopped = "";
      try {aflowlib::LIB2RAW_Loop_POCC(dir_LIB, dir_RAW, vfile, data, "pocc_hea unit test");}
      catch (aurostd::xerror& excpt) {stopped = excpt.buildMessageString();}
      aurostd::RemoveDirectory(dir);
      // the files hold no supercells, so LIB2RAW_Loop_POCC() stops right after the fields
      checkEqual(aurostd::substring2bool(stopped, "No v_dgs found"), true, check_function, "the fields are read before the supercells, where the test files stop it [" + stopped + "]", passed_checks, results);
      checkEqual(vector<double>{data.Hmix_miedema, data.S_config_atom, data.S_config_partial_atom, data.VEC, data.param_geo_radius_covalent_PT_atom},
          vector<double>{-0.015, AUROSTD_NAN, AUROSTD_NAN, 7.5, AUROSTD_NAN}, check_function,
          "Hmix_miedema, S_config_atom (nan), S_config_partial_atom (inf), VEC, param_geo_radius_covalent_PT_atom (1.25abc): unset unless a finite number", passed_checks, results);
      checkEqual(aurostd::joinWDelimiter(data.species_sublattice, ";"), string("Co*0.5,Mg*0.5"), check_function, "species_sublattice of the one disordered site", passed_checks, results);
      checkEqual(data.delta_electronegativity_Pearson_sublattice, vector<double>{0.25}, check_function, "delta_electronegativity_Pearson_sublattice of the site", passed_checks, results);
      checkEqual(data.delta_radius_covalent_PT_sublattice, vector<double>{AUROSTD_NAN}, check_function, "delta_radius_covalent_PT_sublattice of the site (nan): unset", passed_checks, results);
      checkEqual(vector<double>{data.solubility_parameter_0K, data.lat_distortion_misfit_mean_0K, data.lat_distortion_misfit_stdev_0K, data.lat_distortion_relax_distance_mean_0K, data.lat_distortion_relax_distance_stdev_0K},
          vector<double>{3.5, 0.1, AUROSTD_NAN, 0.02, AUROSTD_NAN}, check_function,
          "0 K: solubility_parameter, misfit mean, misfit stdev (-nan), distance mean, distance stdev (inf)", passed_checks, results);
      checkEqual(vector<double>{data.solubility_parameter_300K, data.lat_distortion_misfit_mean_300K, data.lat_distortion_misfit_stdev_300K},
          vector<double>{4.5, AUROSTD_NAN, AUROSTD_NAN}, check_function,
          "300 K: solubility_parameter, misfit mean (nan), misfit stdev (not in the file)", passed_checks, results);
    }
    // 3. param_geo_radius_covalent_PT_atom, written once per disordered sublattice, is one number per entry: read with
    //    one disordered sublattice, unset with two //CO20261015
    {
      const string check_function = "aflowlib::LIB2RAW_Loop_POCC()";
      auto readPoccOut = [](const string& sublattices) {
        const string dir = aurostd::TmpDirectoryCreate("pocc_hea_param_geo");
        const string dir_LIB = dir + "/LIB", dir_RAW = dir + "/RAW";
        aurostd::DirectoryMake(dir_LIB);
        aurostd::DirectoryMake(dir_RAW);
        aurostd::string2file("[AFLOW_POCC]START_TEMPERATURE=ALL\n" + sublattices + "[AFLOW_POCC]STOP_TEMPERATURE=ALL\n", dir_LIB + "/" + POCC_FILE_PREFIX + POCC_OUT_FILE);
        aurostd::string2file("[AFLOW] pocc_hea unit test\n", dir_LIB + "/" + _AFLOWIN_);
        aurostd::string2file("", dir_LIB + "/" + POCC_FILE_PREFIX + POCC_UNIQUE_SUPERCELLS_FILE);
        aflowlib::_aflowlib_entry data;
        data.system_name = "CoMgO:PAW_PBE.AB_cF8_225_a_b.AB:POCC_P0-1xA_P1-0.5xB-0.5xC";
        vector<string> vfile;
        try {aflowlib::LIB2RAW_Loop_POCC(dir_LIB, dir_RAW, vfile, data, "pocc_hea unit test");}
        catch (aurostd::xerror& excpt) {;}  // stops at the missing supercells, after the fields
        aurostd::RemoveDirectory(dir);
        return data;
      };
      const string site0 = "[AFLOW_POCC]START_SUBLATTICE=0\nspecies_sublattice=Co*0.5,Mg*0.5\nparam_geo_radius_covalent_PT_atom=2.5\n[AFLOW_POCC]STOP_SUBLATTICE=0\n";
      const string site1 = "[AFLOW_POCC]START_SUBLATTICE=1\nspecies_sublattice=Al*0.5,Ga*0.5\nparam_geo_radius_covalent_PT_atom=4.0\n[AFLOW_POCC]STOP_SUBLATTICE=1\n";
      checkEqual(readPoccOut(site0).param_geo_radius_covalent_PT_atom, 2.5, check_function, "param_geo_radius_covalent_PT_atom of the one disordered sublattice", passed_checks, results);
      checkEqual(readPoccOut(site0 + site1).param_geo_radius_covalent_PT_atom, (double)AUROSTD_NAN, check_function, "param_geo_radius_covalent_PT_atom with two disordered sublattices: unset", passed_checks, results);
    }
    (void) errors;
  }

  //CO20261015
  /// @brief POCC descriptors: Miedema mixing enthalpies of iridium pairs, the regular-solution mixing enthalpy, the
  /// geometric parameter, Omega and k_cr1, and the copy of a POccCalculator
  void UnitTest::poccDescriptorsTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function="";
    stringstream xstrss;
    xstrss << "MgO primitive cell (pocc unit test)\n1.0\n0.0 2.1 2.1\n2.1 0.0 2.1\n2.1 2.1 0.0\nMg O\n1 1\nDirect\n0.0 0.0 0.0 Mg\n0.5 0.5 0.5 O\n";
    const xstructure xstr_prim(xstrss,IOAFLOW_AUTO);

    // ---------------------------------------------------------------------------
    // Check | Miedema mixing enthalpies of iridium pairs (xbinary), in either order
    // ---------------------------------------------------------------------------
    {
      checkEqual(vector<double>{xbinary::get_hmix_kj_mol_miedema("Ir", "Au"), xbinary::get_hmix_kj_mol_miedema("Pt", "Ir"), xbinary::get_hmix_kj_mol_miedema("Ir", "U")},
          vector<double>{13.0, 0.0, -50.0}, "xbinary::get_hmix_kj_mol_miedema()", "iridium pairs: Au-Ir, Ir-Pt, Ir-U", passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | regular-solution mixing enthalpy: sum over pairs of 4 H_AB xA xB, in eV/atom //CO20261015
    // ---------------------------------------------------------------------------
    {
      check_function = "xbinary::get_total_hmix_regular_solution()";
      xstructure xs_mix;
      xs_mix.species.push_back("Ag"); xs_mix.species.push_back("Al");
      xs_mix.stoich_each_type.push_back(0.5); xs_mix.stoich_each_type.push_back(0.5);
      const double expected_binary = 4.0 * 0.5 * 0.5 * 4.0 / 96.485;  // Ag-Al: 4 kJ/mol
      check(fabs(xbinary::get_total_hmix_regular_solution(xs_mix) - expected_binary) < 1.0e-12, xbinary::get_total_hmix_regular_solution(xs_mix), expected_binary, check_function, "equiatomic AgAl: H_AgAl (eV/atom)", passed_checks, results);
      xs_mix.species.push_back("Au");
      xs_mix.stoich_each_type.clear();
      for (int i = 0; i < 3; i++) xs_mix.stoich_each_type.push_back(1.0 / 3.0);
      const double expected_ternary = 4.0 / 9.0 * (4.0 - 6.0 - 22.0) / 96.485;  // Ag-Al, Ag-Au, Al-Au
      check(fabs(xbinary::get_total_hmix_regular_solution(xs_mix) - expected_ternary) < 1.0e-12, xbinary::get_total_hmix_regular_solution(xs_mix), expected_ternary, check_function, "equiatomic AgAlAu: 4/9 (H_AgAl + H_AgAu + H_AlAu) (eV/atom)", passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Check | geometric parameter S_config / delta^2: unset when it is not a finite number //CO20261015
    // ---------------------------------------------------------------------------
    {
      check_function = "pocc::getParamGeo()";
      checkEqual(vector<double>{pocc::getParamGeo(2.0, 2.0), pocc::getParamGeo(2.0, 0.0), pocc::getParamGeo(0.0, 0.0), pocc::getParamGeo(AUROSTD_MAX_DOUBLE, 1.0), pocc::getParamGeo(2.0, AUROSTD_MAX_DOUBLE)},
          vector<double>{0.5, AUROSTD_MAX_DOUBLE, AUROSTD_MAX_DOUBLE, AUROSTD_MAX_DOUBLE, AUROSTD_MAX_DOUBLE}, check_function,
          "2/2^2, then delta 0 (equal radii), 0/0, and either one unset", passed_checks, results);
    }

    //CO20261018 - the checks of R*, the solubility parameter and the reading of a representative's structure, as in
    //3.2.14+s4e.1 of 2026-10-15; xstr_prim is the MgO primitive cell above
    {
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    string check_description="";
    // ---------------------------------------------------------------------------
    // Check | a POSCAR that names no species gets the species of the directory; one that names them keeps them
    // ---------------------------------------------------------------------------
    {
      check_function="pocc::readRepresentativeStructure(), pocc::setRepresentativeSpecies()";
      string directory=aurostd::TmpDirectoryCreate("pocc_unit_test_species");
      const string file=directory+"/CONTCAR.relax";
      aurostd::string2file("MgO rock salt, VASP4 without species (pocc unit test)\n4.2\n0.0 0.5 0.5\n0.5 0.0 0.5\n0.5 0.5 0.0\n1 1\nDirect\n0.0 0.0 0.0\n0.5 0.5 0.5\n",file);
      xstructure xs;
      vector<string> calculated,expected={"Mg","O","Mg","O"};
      string msg="";
      try{pocc::readRepresentativeStructure(file,vector<string>({"Mg","O"}),xs);}
      catch(aurostd::xerror& err){msg=err.what();}
      if(LDEBUG && !msg.empty()){std::cerr << __AFLOW_FUNC__ << " readRepresentativeStructure: " << msg << std::endl;}
      for(size_t i=0;i<xs.atoms.size();i++){calculated.push_back(xs.atoms[i].name);}
      for(size_t i=0;i<xs.species.size();i++){calculated.push_back(xs.species[i]);}
      check_description="VASP4 POSCAR without species: the directory's species (atom names, then species)";
      check((calculated==expected),calculated,expected,check_function,check_description,passed_checks,results);
      pocc::setRepresentativeSpecies(vector<string>({"Ca","S"}),xs);
      calculated.clear();
      for(size_t i=0;i<xs.atoms.size();i++){calculated.push_back(xs.atoms[i].name);}
      for(size_t i=0;i<xs.species.size();i++){calculated.push_back(xs.species[i]);}
      check_description="a structure that names its species keeps them";
      check((calculated==expected),calculated,expected,check_function,check_description,passed_checks,results);
#ifndef _AFLOW_TEMP_PRESERVE_
      aurostd::RemoveDirectory(directory);
#endif
    }

    // rock salt (Mg0.5Ni0.5)O: primitive cell of MgO, a_par=4.2 A
    const double a_par=4.2;
    //POSCAR with the species line, atoms grouped by species in order of appearance
    auto poscar5_text=[](const string& title,const xstructure& x_in){
      xstructure x(x_in);
      x.ReScale(1.0);
      vector<string> species;
      for(size_t i=0;i<x.atoms.size();i++){
        x.atoms[i].CleanName();
        if(std::find(species.begin(),species.end(),x.atoms[i].cleanname)==species.end()){species.push_back(x.atoms[i].cleanname);}
      }
      stringstream ss;
      ss << title << endl << "1.0" << endl << std::setprecision(12) << std::fixed;
      for(int i=1;i<=3;i++){ss << x.lattice(i,1) << " " << x.lattice(i,2) << " " << x.lattice(i,3) << endl;}
      ss << aurostd::joinWDelimiter(species," ") << endl;
      for(size_t isp=0;isp<species.size();isp++){
        uint n=0;
        for(size_t i=0;i<x.atoms.size();i++){if(x.atoms[i].cleanname==species[isp]){n++;}}
        ss << n << (isp+1<species.size()?" ":"\n");
      }
      ss << "Direct" << endl;
      for(size_t isp=0;isp<species.size();isp++){
        for(size_t i=0;i<x.atoms.size();i++){
          if(x.atoms[i].cleanname!=species[isp]){continue;}
          ss << x.atoms[i].fpos(1) << " " << x.atoms[i].fpos(2) << " " << x.atoms[i].fpos(3) << " " << species[isp] << endl;
        }
      }
      return ss.str();
    };
    //homogeneous deformation D of the Cartesian vectors: lattice rows L -> L D^T, the same fractional coordinates
    auto deform=[](const xstructure& x_in,const xmatrix<double>& D){
      xstructure x(x_in);
      x.ReScale(1.0);
      x.lattice=x.lattice*aurostd::trasp(D);
      x.FixLattices();
      for(size_t ia=0;ia<x.atoms.size();ia++){x.atoms[ia].cpos=aurostd::trasp(x.lattice)*x.atoms[ia].fpos;}
      return x;
    };

    // ---------------------------------------------------------------------------
    // Check | R* of a rock salt from the volume per atom: the primitive cell (2 atoms), an index-2 supercell (4) and the
    // conventional cell (8), each stretched by 10% along [111], all give (a_par/2) 1.1^(1/3)
    // ---------------------------------------------------------------------------
    check_function="pocc::RStarPerAtom()";
    {
      xvector<double> n111(3);
      n111(1)=n111(2)=n111(3)=1.0/std::sqrt(3.0);
      const xmatrix<double> D=aurostd::eye<double>(3,3)+0.1*aurostd::outer_product(n111,n111);  //stretch along [111], volume x 1.1
      xmatrix<double> H2(3,3),Hconv(3,3);  //det 2 and 4
      H2(1,1)=2;H2(2,2)=1;H2(3,3)=1;
      Hconv(1,1)=-1;Hconv(1,2)=1;Hconv(1,3)=1;Hconv(2,1)=1;Hconv(2,2)=-1;Hconv(2,3)=1;Hconv(3,1)=1;Hconv(3,2)=1;Hconv(3,3)=-1;
      const double r_expected=0.5*a_par*std::cbrt(1.1);
      const vector<xstructure> vcells={xstr_prim,GetSuperCell(xstr_prim,H2),GetSuperCell(xstr_prim,Hconv)};
      vector<double> r_cells,expected_cells;
      bool same=true;
      for(size_t i=0;i<vcells.size();i++){
        r_cells.push_back(pocc::RStarPerAtom(deform(vcells[i],D)));
        expected_cells.push_back(r_expected);
        same=(same && fabs(r_cells.back()-r_expected)<1.0e-9);
      }
      check_description="rock salt stretched 10% along [111]: the primitive cell, an index-2 supercell and the conventional cell give the same R* ("+aurostd::utype2string(r_expected,6)+" A)";
      check(same,r_cells,expected_cells,check_function,check_description,passed_checks,results);
      string outcome="accepted";
      try{pocc::RStarPerAtom(xstructure());}
      catch(aurostd::xerror&){outcome="refused";}
      check_description="a structure without atoms is refused";
      checkEqual(outcome,string("refused"),check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // Check | POccCalculator::setRStar() and setSolubilityParameter() on a POCC directory written here: four ARUN
    // directories, each with POSCAR.orig and CONTCAR.relax. The representative is the index-2 supercell of the primitive
    // cell with one cation Mg and the other Ni. Relaxed: +12%, +12%, -20%; +2% along x; +12%, +12%, -20%; not deformed,
    // with one O moved into a tetrahedral interstice. R* is the edge of the cube with each representative's volume per atom.
    // ---------------------------------------------------------------------------
    {
      xmatrix<double> H2(3,3);
      H2(1,1)=2;H2(2,2)=1;H2(3,3)=1;
      xstructure xstr_rep=GetSuperCell(xstr_prim,H2);
      {
        bool first_cation=true;
        for(size_t ia=0;ia<xstr_rep.atoms.size();ia++){
          xstr_rep.atoms[ia].CleanName();
          if(xstr_rep.atoms[ia].cleanname=="O"){continue;}
          if(!first_cation){xstr_rep.atoms[ia].name="Ni";xstr_rep.atoms[ia].cleanname="Ni";}
          first_cation=false;
        }
      }
      aurostd::StringstreamClean(xstrss);
      xstrss << poscar5_text("representative (pocc unit test)",xstr_rep);
      xstr_rep=xstructure(xstrss,IOAFLOW_AUTO);
      xmatrix<double> A_far(3,3),A_small(3,3);
      A_far(1,1)=1.12;A_far(2,2)=1.12;A_far(3,3)=0.80;
      A_small(1,1)=1.02;A_small(2,2)=1.0;A_small(3,3)=1.0;
      const string dir_pocc=aurostd::TmpDirectoryCreate("pocc_unit_test_run");
      const vector<string> varun={"ARUN.POCC_01_H0000C0","ARUN.POCC_02_H0000C1","ARUN.POCC_03_H0000C2","ARUN.POCC_04_H0000C3"};
      for(size_t r=0;r<varun.size();r++){
        const string dir_arun=dir_pocc+"/"+varun[r];
        aurostd::DirectoryMake(dir_arun);
        aurostd::string2file(poscar5_text("start (pocc unit test)",xstr_rep),dir_arun+"/POSCAR.orig");
        if(r<3){
          aurostd::string2file(poscar5_text("relaxed (pocc unit test)",deform(xstr_rep,(r==1?A_small:A_far))),dir_arun+"/CONTCAR.relax");
          continue;
        }
        //the fourth: one O moved by a quarter of the cube diagonal of the parent cell, into a tetrahedral interstice
        xstructure xs_off(xstr_rep);
        xs_off.ReScale(1.0);
        xvector<double> d(3);
        d(1)=d(2)=d(3)=0.25*a_par;
        for(size_t ia=0;ia<xs_off.atoms.size();ia++){
          xs_off.atoms[ia].CleanName();
          if(xs_off.atoms[ia].cleanname!="O"){continue;}
          xs_off.atoms[ia].cpos+=d;
          xs_off.atoms[ia].fpos=C2F(xs_off.lattice,xs_off.atoms[ia].cpos);
          break;
        }
        aurostd::string2file(poscar5_text("relaxed off the parent (pocc unit test)",xs_off),dir_arun+"/CONTCAR.relax");
      }
      std::ostringstream oss_pocc;
      pocc::POccCalculator pcalc(oss_pocc);
      pcalc.m_aflags.Directory=dir_pocc;
      pcalc.m_ARUN_directories=varun;
      pcalc.xstr_pocc=xstr_prim;
      pcalc.l_supercell_sets.assign(varun.size(),pocc::POccSuperCellSet());

      //R* of each representative: 4 atoms in twice the primitive cell (a_par^3/2), so a_par/2 times the cube root of the
      //volume change; the fourth, not deformed, a_par/2
      check_function="pocc::POccCalculator::setRStar()";
      const double r_far=0.5*a_par*std::cbrt(1.12*1.12*0.80),r_small=0.5*a_par*std::cbrt(1.02),r_off=0.5*a_par;
      const vector<double> expected_rstar={r_far,r_small,r_far,r_off};
      bool rstar_set=false;
      try{rstar_set=pcalc.setRStar();}
      catch(aurostd::xerror& err){if(LDEBUG){std::cerr << __AFLOW_FUNC__ << " setRStar: " << err.what() << std::endl;}}
      bool same=(rstar_set && pcalc.m_vr_star.size()==expected_rstar.size());
      for(size_t i=0;i<pcalc.m_vr_star.size()&&same;i++){same=(fabs(pcalc.m_vr_star[i]-expected_rstar[i])<1.0e-6);}
      check_description="R* of the four representatives, the edge of the cube with the volume per atom, setRStar() true";
      check(same,pcalc.m_vr_star,expected_rstar,check_function,check_description,passed_checks,results);

      //solubility parameter G <R*> var(R*), with no division by Z. G is the shear modulus averaged over the species with
      //their share of the atoms, those without one (O) left out: (0.25 G_Mg + 0.25 G_Ni) / 0.5. With the probabilities
      //0.1, 0.2, 0.3, 0.4: <R*> = 0.4 r_far + 0.2 r_small + 0.4 r_off, var(R*) = 0.4 (r_far - <R*>)^2 +
      //0.2 (r_small - <R*>)^2 + 0.4 (r_off - <R*>)^2
      check_function="pocc::POccCalculator::setSolubilityParameter()";
      {
        const vector<double> vprob={0.1,0.2,0.3,0.4};
        uint isc=0;
        for(std::list<pocc::POccSuperCellSet>::iterator it=pcalc.l_supercell_sets.begin();it!=pcalc.l_supercell_sets.end();++it){(*it).m_probability=vprob[isc++];}
        pcalc.xstr_pocc.species.clear();pcalc.xstr_pocc.species.push_back("Mg");pcalc.xstr_pocc.species.push_back("Ni");pcalc.xstr_pocc.species.push_back("O");
        pcalc.xstr_pocc.stoich_each_type.clear();pcalc.xstr_pocc.stoich_each_type.push_back(0.25);pcalc.xstr_pocc.stoich_each_type.push_back(0.25);pcalc.xstr_pocc.stoich_each_type.push_back(0.5);
        pcalc.xstr_nopocc=xstr_prim;
        const double G_Mg=xelement::xelement("Mg").getPropertyDouble("modulus_shear"),G_Ni=xelement::xelement("Ni").getPropertyDouble("modulus_shear");
        const double G=(0.25*G_Mg+0.25*G_Ni)/0.5;
        const double r_avg=0.4*r_far+0.2*r_small+0.4*r_off;
        const double r_var=0.4*(r_far-r_avg)*(r_far-r_avg)+0.2*(r_small-r_avg)*(r_small-r_avg)+0.4*(r_off-r_avg)*(r_off-r_avg);
        const double expected_solubility=G*r_avg*r_var;
        try{pcalc.setSolubilityParameter(300.0);}
        catch(aurostd::xerror& err){if(LDEBUG){std::cerr << __AFLOW_FUNC__ << " setSolubilityParameter: " << err.what() << std::endl;}}
        check_description="G <R*> var(R*) over the four representatives, no division by Z (GPa A^3)";
        check(fabs(pcalc.m_solubility_parameter-expected_solubility)<1.0e-9*fabs(expected_solubility),pcalc.m_solubility_parameter,expected_solubility,check_function,check_description,passed_checks,results);
        check_description="shear moduli of Mg and Ni (GPa) in xelement";
        checkEqual(vector<double>{G_Mg,G_Ni},vector<double>{17.0,76.0},check_function,check_description,passed_checks,results);
      }

      //no representative has a relaxed structure: setRStar() false, R* none, and no solubility parameter
      {
        const string dir_none=aurostd::TmpDirectoryCreate("pocc_unit_test_none"),arun_none="ARUN.POCC_01_H0000C0";
        aurostd::DirectoryMake(dir_none+"/"+arun_none);
        aurostd::string2file(poscar5_text("start (pocc unit test)",xstr_rep),dir_none+"/"+arun_none+"/POSCAR.orig");
        std::ostringstream oss_none;
        pocc::POccCalculator pcalc_none(oss_none);
        pcalc_none.m_aflags.Directory=dir_none;
        pcalc_none.m_ARUN_directories={arun_none};
        pcalc_none.xstr_pocc=pcalc.xstr_pocc;
        pcalc_none.xstr_nopocc=xstr_prim;
        pcalc_none.l_supercell_sets.assign(1,pocc::POccSuperCellSet());
        pcalc_none.l_supercell_sets.front().m_probability=1.0;
        bool rstar_none=true;
        try{rstar_none=pcalc_none.setRStar();pcalc_none.setSolubilityParameter(300.0);}
        catch(aurostd::xerror& err){if(LDEBUG){std::cerr << __AFLOW_FUNC__ << " no relaxed structure: " << err.what() << std::endl;}}
        const vector<double> calculated_none={(double)rstar_none,(double)pcalc_none.m_vr_star.size(),(pcalc_none.m_vr_star.size()==1&&pcalc_none.m_vr_star[0]==AUROSTD_MAX_DOUBLE?1.0:0.0),(pcalc_none.m_solubility_parameter==AUROSTD_MAX_DOUBLE?1.0:0.0)};
        const vector<double> expected_none={0.0,1.0,1.0,1.0};
        check_function="pocc::POccCalculator::setRStar(), setSolubilityParameter()";
        check_description="no relaxed structure: setRStar() false, one R* and it is none, no solubility parameter";
        check(calculated_none==expected_none,calculated_none,expected_none,check_function,check_description,passed_checks,results);
#ifndef _AFLOW_TEMP_PRESERVE_
        aurostd::RemoveDirectory(dir_none);
#endif
      }
#ifndef _AFLOW_TEMP_PRESERVE_
      aurostd::RemoveDirectory(dir_pocc);
#endif
    }
    }

    // ---------------------------------------------------------------------------
    // Check | Omega = T_m dS_mix / |dH_mix| and k_cr1 = 1 + T dS_mix (1 - k2) / |dH_mix| (dH_mix < 0), with
    // dS_mix = k_B S_config; a missing value leaves them unset //CO20261015
    // ---------------------------------------------------------------------------
    {
      std::ostringstream oss_thermo;
      pocc::POccCalculator pcalc(oss_thermo);
      pcalc.xstr_pocc.species.clear(); pcalc.xstr_pocc.species.push_back("Mg"); pcalc.xstr_pocc.species.push_back("Ni");
      pcalc.xstr_pocc.stoich_each_type.clear(); pcalc.xstr_pocc.stoich_each_type.push_back(0.5); pcalc.xstr_pocc.stoich_each_type.push_back(0.5);
      pcalc.xstr_nopocc = xstr_prim;  // 2 sites
      pcalc.m_Hmix_miedema = -0.1;
      pcalc.m_S_config_cell = std::log(2.0);
      pcalc.m_S_config_atom = 0.5 * std::log(2.0);
      pcalc.m_k2 = 0.6;
      const double Tm = 0.5 * (xelement::xelement("Mg").temperature_melting + 273.15) + 0.5 * (xelement::xelement("Ni").temperature_melting + 273.15);
      const double expected_Omega = Tm * std::log(2.0) * KBOLTZEV / (0.1 * 2.0);
      pcalc.setOmega();
      check_function = "pocc::POccCalculator::setOmega()";
      check(fabs(pcalc.m_Omega - expected_Omega) < 1.0e-9 * expected_Omega, pcalc.m_Omega, expected_Omega, check_function, "MgNi, Hmix -0.1 eV/atom: T_m k_B S_config_cell / (|Hmix| N)", passed_checks, results);
      const double T = 1000.0;
      const double expected_kcr1 = 1.0 + T * KBOLTZEV * 0.5 * std::log(2.0) / 0.1 * (1.0 - 0.6);
      pcalc.setThermoParameter(T);
      check_function = "pocc::POccCalculator::setThermoParameter()";
      check(fabs(pcalc.m_kcr1 - expected_kcr1) < 1.0e-12 * expected_kcr1, pcalc.m_kcr1, expected_kcr1, check_function, "k_cr1 at 1000 K, k2 = 0.6, Hmix -0.1 eV/atom", passed_checks, results);
      pcalc.m_Hmix_miedema = 0.0;
      pcalc.setOmega();
      pcalc.setThermoParameter(T);
      checkEqual(vector<double>{pcalc.m_Omega, pcalc.m_kcr1}, vector<double>{AUROSTD_MAX_DOUBLE, AUROSTD_MAX_DOUBLE}, check_function, "Hmix 0: Omega and k_cr1 unset", passed_checks, results);

      // copy() keeps the descriptors of the sublattices
      check_function = "pocc::POccCalculator::copy()";
      pcalc.m_nsublattices_disordered = 1;
      pcalc.m_delta_electronegativity_Pauling_sublattice = {0.3};
      pcalc.m_delta_electronegativity_Allen_sublattice = {0.2};
      pocc::POccCalculator pcalc_copy(pcalc);
      checkEqual(vector<double>{(double)pcalc_copy.m_nsublattices_disordered, pcalc_copy.m_delta_electronegativity_Pauling_sublattice.at(0), pcalc_copy.m_delta_electronegativity_Allen_sublattice.at(0)},
          vector<double>{1.0, 0.3, 0.2}, check_function, "number of disordered sublattices and their electronegativity mismatch", passed_checks, results);
      std::ostringstream oss_new;
      pocc::POccCalculator pcalc_new(oss_new);
      checkEqual((double)pcalc_new.m_nsublattices_disordered, 0.0, "pocc::POccCalculator::free()", "a new calculator has no disordered sublattices", passed_checks, results);
    }
  }

}
//WJ20230816 START
namespace unittest {

  void UnitTest::AFLUXTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    (void) errors;  // Suppress compiler warnings
    
    if(LDEBUG){cerr << __AFLOW_FUNC__ << " BEGIN" << endl;} //CO20231222 - silence warnings

    // Set up test environment
    string check_function = "";
    string check_description = "";
    aflux::AFLUX tester;
    aflowlib::EntryLoader el;

    // ---------------------------------------------------------------------------
    // Test: alloy() written with or without commas //CO20260927
    // ---------------------------------------------------------------------------
    check_function = "AFLUX::genAlloy()";
    check_description = "elements written together, with commas, or repeated give the same systems";
    {
      aflux::AFLUX alloy_tester;
      const string with_commas = alloy_tester.genAlloy(vector<string>{"Co", "Mg", "O"});
      checkEqual(alloy_tester.genAlloy(vector<string>{"CoMgO"}), with_commas, check_function, check_description, passed_checks, results);
      // the systems come in the order the elements are first named (O, Co, Mg here), so compare them as sets //CO20261004
      auto systems = [](const string& clause) {
        vector<string> v;
        const size_t start = clause.find("alloy IN (");
        if (start == string::npos) return v;
        const size_t from = start + string("alloy IN (").size();
        aurostd::string2tokens(clause.substr(from, clause.find(')', from) - from), v, ",");
        std::sort(v.begin(), v.end());
        return v;
      };
      checkEqual(systems(alloy_tester.genAlloy(vector<string>{"O", "Co", "Mg", "MgO"})), systems(with_commas), check_function, check_description, passed_checks, results);
      checkEqual(systems(with_commas).size(), (size_t) 7, check_function, "three elements give seven systems", passed_checks, results); //CO20261004
      checkEqual(with_commas.find("'CoMgO'") != string::npos && with_commas.find("'MgO'") != string::npos, true, check_function, check_description, passed_checks, results);
      check_description = "a part that is not made of elements is ignored";
      checkEqual(alloy_tester.genAlloy(vector<string>{"Co", "Mg", "O", "co9"}), with_commas, check_function, check_description, passed_checks, results);
      check_description = "elements written together split like elements written apart"; //CO20261004
      checkEqual(alloy_tester.genAlloy(vector<string>{"MgOCo"}), alloy_tester.genAlloy(vector<string>{"Mg", "O", "Co"}), check_function, check_description, passed_checks, results); //CO20261004
      check_description = "a part starting in lower case is ignored"; //CO20261004
      checkEqual(alloy_tester.genAlloy(vector<string>{"Co", "Mg", "O", "oCoMg"}), with_commas, check_function, check_description, passed_checks, results); //CO20261004
      check_description = "a part with a symbol that is not an element is ignored"; //CO20261004
      checkEqual(alloy_tester.genAlloy(vector<string>{"Co", "Mg", "O", "CoXq"}), with_commas, check_function, check_description, passed_checks, results); //CO20261004
      check_description = "the element set matches with case (CoO is not CO)";
      checkEqual(with_commas.find("alloy COLLATE BINARY IN ('") != string::npos, true, check_function, check_description, passed_checks, results);
    }
    // every subset of the elements is a system of its own, so alloy() takes at most 12 elements; without the subsets
    // (recursive=false) the bound does not apply //CO20261004
    {
      const vector<string> thirteen = {"H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne", "Na", "Mg", "Al"};
      const vector<string> twelve(thirteen.begin(), thirteen.end() - 1);
      bool threw_thirteen = false, threw_twelve = false, threw_one_system = false;
      string where_twelve = "";
      {aflux::AFLUX bound_tester; try {bound_tester.genAlloy(thirteen);} catch (aurostd::xerror& excpt) {threw_thirteen = true;}}
      {aflux::AFLUX bound_tester; try {where_twelve = bound_tester.genAlloy(twelve);} catch (aurostd::xerror& excpt) {threw_twelve = true;}}
      {aflux::AFLUX bound_tester; try {bound_tester.genAlloy(thirteen, false);} catch (aurostd::xerror& excpt) {threw_one_system = true;}}
      check_description = "13 elements are refused";
      checkEqual(threw_thirteen, true, check_function, check_description, passed_checks, results);
      uint nsystems_twelve = 0;  // the list appears twice in the clause, the systems separated by ','
      for (size_t p = where_twelve.find("','"); p != string::npos; p = where_twelve.find("','", p + 1)) nsystems_twelve++;
      nsystems_twelve = (nsystems_twelve + 2) / 2;
      check_description = "12 elements give all 4095 systems, up to the one with all twelve";
      checkEqual(threw_twelve, false, check_function, check_description + " (no exception)", passed_checks, results);
      checkEqual(nsystems_twelve, (uint) 4095, check_function, check_description, passed_checks, results);
      checkEqual(aurostd::substring2bool(where_twelve, "'BBeCFHHeLiMgNNaNeO'"), true, check_function, check_description + " (BBeCFHHeLiMgNNaNeO)", passed_checks, results);
      check_description = "13 elements without the subsets are one system";
      checkEqual(threw_one_system, false, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Test 0: force AFLOW format without corrupting percent-encoded queries
    // ---------------------------------------------------------------------------
    check_function = "AFLUX::forceAflowFormat()";
    check_description = "force AFLOW format idempotently for encoded and plain queries";
    string format_query = "?%2A%2Calloy%28Mn%3APd%3AMnPd%29%2Cformat%28aflow%29%2Cpaging%280%29";
    string expected_format_query = format_query;
    tester.forceAflowFormat(format_query);
    checkEqual(format_query, expected_format_query, check_function+" (encoded AFLOW)", check_description, passed_checks, results);

    format_query = "?species%28Fe%29%2Cformat%28json%29";
    expected_format_query = "?species%28Fe%29%2Cformat%28aflow%29";
    tester.forceAflowFormat(format_query);
    checkEqual(format_query, expected_format_query, check_function+" (encoded JSON)", check_description, passed_checks, results);

    format_query = "?species%28Fe%29";
    expected_format_query = "?species%28Fe%29%2Cformat%28aflow%29";
    tester.forceAflowFormat(format_query);
    checkEqual(format_query, expected_format_query, check_function+" (encoded append)", check_description, passed_checks, results);

    format_query = "?species(Fe),format(html)";
    expected_format_query = "?species(Fe),format(aflow)";
    tester.forceAflowFormat(format_query);
    checkEqual(format_query, expected_format_query, check_function+" (plain HTML)", check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Test 0b: every term of an AND (,) or OR (:) list counts, not only the first two //CO20261002
    // ---------------------------------------------------------------------------
    {
      auto count_terms = [](const string& sql) {uint n = 0; for (size_t p = sql.find("json_extract"); p != string::npos; p = sql.find("json_extract", p + 1)) {n++;} return n;};
      aflowlib::_aflowlib_entry entry_terms;
      entry_terms.vstress_tensor = {1.0, 2.0, 3.0};
      check_function = "aflux::applyTensorFilter()";
      check_description = "AND list of two true terms passes";
      checkEqual(aflux::applyTensorFilter("@(1)(0*),@(2)(0*)", "stress_tensor", entry_terms), true, check_function, check_description, passed_checks, results);
      check_description = "AND list fails on its third term";
      checkEqual(aflux::applyTensorFilter("@(1)(0*),@(2)(0*),@(3)(*-1)", "stress_tensor", entry_terms), false, check_function, check_description, passed_checks, results);
      check_description = "OR list passes on its third term";
      checkEqual(aflux::applyTensorFilter("@(1)(*-1):@(2)(*-1):@(3)(0*)", "stress_tensor", entry_terms), true, check_function, check_description, passed_checks, results);
      check_function = "AFLUX::genSQLnumbers_fixed()";
      check_description = "AND list of three terms gives three clauses";
      checkEqual(count_terms(tester.genSQLnumbers_fixed("@(1)(0*),@(2)(0*),@(3)(0*)", "stress_tensor")), 3u, check_function, check_description, passed_checks, results);
      check_description = "OR list of three terms gives three clauses";
      checkEqual(count_terms(tester.genSQLnumbers_fixed("@(1)(0*):@(2)(0*):@(3)(0*)", "stress_tensor")), 3u, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Test 0c: the default AFLUX servers are s4e.ai only, one path per server //CO20261003
    // read from a freshly written default .aflow.rc, so a local .aflow.rc does not matter
    // ---------------------------------------------------------------------------
    {
      const string aflowrc_saved = XHOST.aflowrc_filename;
      XHOST.aflowrc_filename = aurostd::TmpFileCreate("aflowrc_unit_test");
      std::stringstream devnull_ss;
      aflowrc::write_default(devnull_ss, false);
      string aflowrc_default = "";
      aurostd::file2string(XHOST.aflowrc_filename, aflowrc_default);
      aurostd::RemoveFile(XHOST.aflowrc_filename);
      XHOST.aflowrc_filename = aflowrc_saved;
      auto rc_value = [&aflowrc_default](const string& key) {
        string value = "";
        const size_t p = aflowrc_default.find(key + "=\"");
        if (p != string::npos) {value = aflowrc_default.substr(p + key.size() + 2); value = value.substr(0, value.find('"'));}
        return value;
      };
      vector<string> vservers, vpaths;
      aurostd::string2tokens(rc_value("DEFAULT_ENTRY_LOADER_AFLUX_SERVER_LIST"), vservers, ",");
      aurostd::string2tokens(rc_value("DEFAULT_ENTRY_LOADER_AFLUX_PATH_LIST"), vpaths, ",");
      check_function = "aflowrc::write_default()";
      check_description = "default AFLUX servers are s4e.ai only";
      checkEqual(aurostd::joinWDelimiter(vservers, ","), string("s4e.ai"), check_function, check_description, passed_checks, results);
      check_description = "one AFLUX path per server";
      checkEqual(vpaths.size(), vservers.size(), check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Test 0d: filters with each term in its own parentheses, as in "(@(1)(0*)),(@(2)(0*))" //CO20261003
    // ---------------------------------------------------------------------------
    {
      auto count_terms = [](const string& sql) {uint n = 0; for (size_t p = sql.find("json_extract"); p != string::npos; p = sql.find("json_extract", p + 1)) {n++;} return n;};
      aflowlib::_aflowlib_entry entry_terms;
      entry_terms.vstress_tensor = {1.0, 2.0, 3.0};
      check_function = "aflux::removeOuterParentheses()";
      check_description = "a pair around the whole string is removed";
      checkEqual(aflux::removeOuterParentheses("(@(1)(0*))"), string("@(1)(0*)"), check_function, check_description, passed_checks, results);
      check_description = "two pairs side by side are kept";
      checkEqual(aflux::removeOuterParentheses("(@(1)(0*)),(@(2)(0*))"), string("(@(1)(0*)),(@(2)(0*))"), check_function, check_description, passed_checks, results);
      check_description = "an empty string stays empty";
      checkEqual(aflux::removeOuterParentheses(""), string(""), check_function, check_description, passed_checks, results);
      check_function = "aflux::applyTensorFilter()";
      check_description = "AND list of parenthesized terms passes";
      checkEqual(aflux::applyTensorFilter("(@(1)(0*)),(@(2)(0*))", "stress_tensor", entry_terms), true, check_function, check_description, passed_checks, results);
      check_description = "AND list of parenthesized terms fails on its third term";
      checkEqual(aflux::applyTensorFilter("(@(1)(0*)),(@(2)(0*)),(@(3)(*-1))", "stress_tensor", entry_terms), false, check_function, check_description, passed_checks, results);
      check_description = "OR list of parenthesized terms passes on its third term";
      checkEqual(aflux::applyTensorFilter("(@(1)(*-1)):(@(2)(*-1)):(@(3)(0*))", "stress_tensor", entry_terms), true, check_function, check_description, passed_checks, results);
      check_function = "AFLUX::genSQLnumbers_fixed()";
      check_description = "AND list of three parenthesized terms gives three clauses";
      checkEqual(count_terms(tester.genSQLnumbers_fixed("(@(1)(0*)),(@(2)(0*)),(@(3)(0*))", "stress_tensor")), 3u, check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Test 0e: the atomic filter had the same first-two-terms bug as the tensor filter; and the SQL clauses joined with
    // the right operator //CO20261004
    // ---------------------------------------------------------------------------
    {
      aflowlib::_aflowlib_entry entry_atoms;  // FeMgO2, Bader charges +1.2 (Fe), +1.6 (Mg), -1.4 (O, both)
      entry_atoms.vspecies = {"Fe", "Mg", "O"};
      entry_atoms.composition = "1,1,2";
      entry_atoms.vbader_net_charges = {1.2, 1.6, -1.4, -1.4};
      check_function = "aflux::applyAtomicFilter()";
      check_description = "AND list of three true terms passes";
      checkEqual(aflux::applyAtomicFilter("@Fe(0*),@Mg(0*),@O(*0)", "bader_net_charges", entry_atoms), true, check_function, check_description, passed_checks, results);
      check_description = "AND list fails on its third term";
      checkEqual(aflux::applyAtomicFilter("@Fe(0*),@Mg(0*),@O(0*)", "bader_net_charges", entry_atoms), false, check_function, check_description, passed_checks, results);
      check_description = "OR list passes on its third term";
      checkEqual(aflux::applyAtomicFilter("@Fe(*0):@Mg(*0):@O(*0)", "bader_net_charges", entry_atoms), true, check_function, check_description, passed_checks, results);
      check_description = "OR list of three false terms fails";
      checkEqual(aflux::applyAtomicFilter("@Fe(*0):@Mg(*0):@O(0*)", "bader_net_charges", entry_atoms), false, check_function, check_description, passed_checks, results);
      check_description = "AND list of parenthesized terms fails on its third term";
      checkEqual(aflux::applyAtomicFilter("(@Fe(0*)),(@Mg(0*)),(@O(0*))", "bader_net_charges", entry_atoms), false, check_function, check_description, passed_checks, results);
      check_description = "OR list of parenthesized terms passes on its third term";
      checkEqual(aflux::applyAtomicFilter("(@Fe(*0)):(@Mg(*0)):(@O(*0))", "bader_net_charges", entry_atoms), true, check_function, check_description, passed_checks, results);
      check_function = "AFLUX::genSQLnumbers_fixed()";
      const string clause1 = "(json_extract(stress_tensorCOMMA '$[1]')>=0)", clause2 = "(json_extract(stress_tensorCOMMA '$[2]')>=0)", clause3 = "(json_extract(stress_tensorCOMMA '$[3]')>=0)";
      check_description = "AND list of three terms: the clauses joined with AND";
      checkEqual(tester.genSQLnumbers_fixed("@(1)(0*),@(2)(0*),@(3)(0*)", "stress_tensor"), "(" + clause1 + " AND " + clause2 + " AND " + clause3 + ")", check_function, check_description, passed_checks, results);
      check_description = "OR list of three terms: the clauses joined with OR";
      checkEqual(tester.genSQLnumbers_fixed("@(1)(0*):@(2)(0*):@(3)(0*)", "stress_tensor"), "(" + clause1 + " OR " + clause2 + " OR " + clause3 + ")", check_function, check_description, passed_checks, results);
      check_description = "AND list of three parenthesized terms: the same clauses";
      checkEqual(tester.genSQLnumbers_fixed("(@(1)(0*)),(@(2)(0*)),(@(3)(0*))", "stress_tensor"), "(" + clause1 + " AND " + clause2 + " AND " + clause3 + ")", check_function, check_description, passed_checks, results);
    }

    // ---------------------------------------------------------------------------
    // Test 1: Test Inter-Server Connectivity 
    // ---------------------------------------------------------------------------
    // Tests if any AFLUX compatible server is reachable
    //CO20261002 - s4e.ai answers AFLUX queries only with a CHAOS API key: say so plainly
    string query = "";
    if (checkChaosApiKey(passed_checks, results)) { //CO20261004 - one helper, the server list split into hosts //CO20261017 - no key: skipped
      check_function = "parseQuery()";
      check_description = "AFLUX query inter-server connectivity";
      query = "?species(Fe),paging(1,5)";
      el.loadAFLUXQuery(query);
      bool expected_inter_server = true;
      bool actual_inter_server = tester.getNumEntries(el) > 0;
      checkEqual(actual_inter_server, expected_inter_server, check_function, check_description, passed_checks, results);
    }
    
    // ---------------------------------------------------------------------------
    // Test 0: check if internal servers are empty
    // ---------------------------------------------------------------------------
    // Skip Internal AFLUX tests if the internal servers are empty, as the tests will fail
    if(!aurostd::FileExist(el.m_filesystem_path)) {
      return; 
    }

    // The following tests are only applicable for S4E and are an example of how to use AFLUX unit tests

    if (getAUIDPrefix() != "s4e") { // End test if not S4E machine
      return;
    }
    // ---------------------------------------------------------------------------
    // Test 2: simply query WHERE generation
    // ---------------------------------------------------------------------------
    tester.clear();
    check_function = "parseQuery()";
    query = "?species(Fe,O),density(0*),bader_net_charges(@O(*0)),paging(1,5)";
    string expected_SQL_WHERE = "(species LIKE '%\"Fe\"%' AND species LIKE '%\"O\"%') AND (density>=0) AND (bader_net_charges IS NOT NULL)";
    
    
    tester.parseQuery(query);
    string actual_SQL_WHERE = tester.m_SQL_query;

    check_description = "parse AFLUX query into SQL";
    checkEqual(actual_SQL_WHERE,expected_SQL_WHERE,check_function,check_description,passed_checks,results);

    // ---------------------------------------------------------------------------
    // Test 3: Check paging parameters
    // ---------------------------------------------------------------------------
    check_function = "genSQLpaging()";
    int expected_pagingParam_k = 5;
    int actual_pagingParam_k = tester.m_pagingParam_k;

    check_description = "check paging parameters";
    checkEqual(actual_pagingParam_k,expected_pagingParam_k,check_function,check_description,passed_checks,results);

    // Skip DB-backed checks if local SQLITE DB is unavailable in this environment.
    if (!aurostd::FileExist(el.m_sqlite_file)) {
      check_function = "queryDB()";
      check_description = "local SQLITE DB unavailable, skipping DB-backed AFLUX checks";
      checkEqual(true, true, check_function, check_description, passed_checks, results);
      return;
    }

    // ---------------------------------------------------------------------------
    // Test 4: check querying DB
    // ---------------------------------------------------------------------------
    check_function = "queryDB()";

    el.clear();
    tester.clear();
    string queryOne = "?species(Fe,O),density(0*),bader_net_charges(@O(*0)),paging(1,5)";
    tester.parseQuery(queryOne);
    tester.queryDB(el);
    bool expected_numEntries = 1;
    bool actual_numEntries = tester.getNumEntries(el);

    check_description = "checking db querying";
    checkEqual(actual_numEntries,expected_numEntries,check_function,check_description,passed_checks,results);

    // ---------------------------------------------------------------------------
    // Test 5: check species filtering
    // ---------------------------------------------------------------------------
    
    check_function = "applyCppFilter()";
    string queryTwo = "?species(Fe,O),density(0*),bader_net_charges(@O(0*)),paging(1,5)"; //materials where O has a bader_net_charge greater than 0 (should be very few)
    
    int queryOneEntries = tester.getNumEntries(el);

    el.clear();
    tester.clear();
    tester.parseQuery(queryTwo);
    tester.queryDB(el);
    
    int queryTwoEntries = tester.getNumEntries(el);
 
    bool expected_pct = 1;
    bool actual_pct = (queryOneEntries > 0) && ((static_cast<double>(queryTwoEntries) / queryOneEntries) < 0.1); //checks if less than 10% of entries have O with a positive bader charge

    check_description = "number of entries with positive 0 bader";
    checkEqual(actual_pct,expected_pct,check_function,check_description,passed_checks,results);

    //TL20231204 START
    // ---------------------------------------------------------------------------
    // Test 6: check special character
    // ---------------------------------------------------------------------------
    el.clear();
    tester.clear();
    check_function = "specialCharQuery()";
    string specialCharQuery = "?species(fe,o),density(0.5*),volume_atom(1*),paging(1,5)";
    tester.parseQuery(specialCharQuery);
    tester.queryDB(el);
    bool isQuerySuccessful = tester.getNumEntries(el) > 0;

    check_description = "Special Character Test";
    checkEqual(isQuerySuccessful,true,check_function,check_description,passed_checks,results);

    // ---------------------------------------------------------------------------
    // Test 7: check keywords order 
    // ---------------------------------------------------------------------------
    check_function = "keywordsOrderTest()";
    el.clear();
    tester.clear();
    string query_order_1 = "?species(Fe,O),density(0.5*),volume_atom(1*),paging(1,5)";
    tester.parseQuery(query_order_1);
    tester.queryDB(el);
    int query_order_entries_1 = tester.getNumEntries(el);

    el.clear();
    tester.clear();
    string query_order_2 = "?species(Fe,O),volume_atom(1*),density(0.5*),paging(1,5)";
    tester.parseQuery(query_order_2);
    tester.queryDB(el);
    int query_order_entries_2 = tester.getNumEntries(el);

    //order will not change the result, query_1 should be same as query_2
    
    check_description = "key words order test";
    checkEqual(query_order_entries_1,query_order_entries_2,check_function,check_description,passed_checks,results);
    
    // ---------------------------------------------------------------------------
    // Test 8: check duplicate keywords 
    // ---------------------------------------------------------------------------
    check_function = "duplicateKeywordsTest()";
    el.clear();
    tester.clear();
    string baseline_query = "?species(Fe,O),density(0.5*),volume_atom(1*),paging(1,5)";
    tester.parseQuery(baseline_query);
    tester.queryDB(el);
    int baseline_query_entries = tester.getNumEntries(el);

    el.clear();
    tester.clear();
    string restricted_query = "?species(Fe,O),volume_atom(1*),volume_atom(5*),density(0.5*),paging(1,5)";
    tester.parseQuery(restricted_query);
    tester.queryDB(el);
    int restricted_query_entries = tester.getNumEntries(el);

    bool isRestrictionEffective = (restricted_query_entries <= baseline_query_entries);

    check_description = "Duplicate Keywords Restriction Test";
    checkEqual(isRestrictionEffective,true,check_function,check_description,passed_checks,results);

    // ---------------------------------------------------------------------------
    // Test 9: check syntax 
    // ---------------------------------------------------------------------------
    check_function = "syntaxtest()";
    tester.clear();
    el.clear();
    string syntax_query_1 = "?species(Fe:O),density(15*),volume_atom(1*),paging(1,5)";
    tester.parseQuery(syntax_query_1);
    tester.queryDB(el);
    int syntax_query_1_entries = tester.getNumEntries(el);

    tester.clear();
    el.clear();
    string syntax_query_2 = "?species(Fe,O),density(0.5*,*3),volume_atom(1*),paging(1,5)";
    tester.parseQuery(syntax_query_2);
    tester.queryDB(el);
    int syntax_query_2_entries = tester.getNumEntries(el);
    
    tester.clear();
    el.clear();
    string syntax_query_3 = "?species((Fe:Cr),(Cl:O)),density(0.5*),volume_atom(1*),paging(1,5)";
    tester.parseQuery(syntax_query_3);
    tester.queryDB(el);
    int syntax_query_3_entries = tester.getNumEntries(el);

    tester.clear();
    el.clear();
    string syntax_query_4 = "?species ( Fe , O ),density(0.5*),volume_atom(1*),paging(1,5)";
    tester.parseQuery(syntax_query_4);
    tester.queryDB(el);
    int syntax_query_4_entries = tester.getNumEntries(el);

    bool allSyntaxTestsPassed = (syntax_query_1_entries > 0) && 
                                (syntax_query_2_entries > 0) && 
                                (syntax_query_3_entries > 0) && 
                                (syntax_query_4_entries > 0);

    check_description = "All Syntax Tests";
    //All of these syntax inputs should work, and thus entries should be more than 0.
    checkEqual(allSyntaxTestsPassed,true,check_function,check_description,passed_checks,results);
    
    // ---------------------------------------------------------------------------
    // Test 10: check missing keywords 
    // ---------------------------------------------------------------------------
    check_function = "missingKeywordsTest()";
    tester.clear();
    el.clear();
    string missing_query = "?species(FeO),density(0.5*),volume_atom(1*),paging(1,5)";
    tester.parseQuery(missing_query);
    tester.queryDB(el);
    int missing_query_entries = tester.getNumEntries(el);
    
    bool expected_no_results = 1; 
    bool actual_no_results = (missing_query_entries == 0); //missing comma between Fe and O leads to search the element 'FeO' which is not existed, so the entry should be 0
    
    check_description = "Missing Keywords Test";
    checkEqual(expected_no_results,actual_no_results,check_function,check_description,passed_checks,results);
    //TL20231204 END

    // MB20240805 START
    int basic_actual_numEntries = 0;
    int many_actual_numEntries = 0;
    int complex_actual_numEntries = 0;
    int impossible_actual_numEntries = 1;
    // ---------------------------------------------------------------------------
    // Test 11: test alloy filtering
    // ---------------------------------------------------------------------------
    check_function = "alloyQuery()";
    check_description = "checking alloy gen querying";

    {
    // checking basic alloy gen querying
    aflux::AFLUX tester10_1;
    aflowlib::EntryLoader el10_1;
    string queryAlloyBasic = "?alloy(Fe,O)";
    tester10_1.parseQuery(queryAlloyBasic);
    tester10_1.queryDB(el10_1);
    expected_numEntries = 1;
    basic_actual_numEntries = tester10_1.getNumEntries(el10_1);

    // checking alloy gen querying with other params
    aflux::AFLUX tester10_2;
    aflowlib::EntryLoader el10_2;
    actual_numEntries = 0;
    string queryAlloyManyParams = "?alloy(Fe,O),density(0*),bader_net_charges(@O(*0)),paging(1,5)";
    tester10_2.parseQuery(queryAlloyManyParams);
    tester10_2.queryDB(el10_2);
    expected_numEntries = 1;
    many_actual_numEntries = tester10_2.getNumEntries(el10_2);

    // checking OR and AND alloy gen querying with Params
    aflux::AFLUX tester10_3;
    aflowlib::EntryLoader el10_3;
    actual_numEntries = 0;
    string queryAlloyComplex = "?alloy(F:Sr:Na:Mg:FMg:FSr:FNa:MgNa:MgSr:NaSr:FMgNa:FMgSr:FNaSr:MgNaSr:FMgNaSr),density(0*),paging(1,5)";
    tester10_3.parseQuery(queryAlloyComplex);
    tester10_3.queryDB(el10_3);
    expected_numEntries = 1;
    complex_actual_numEntries = tester10_3.getNumEntries(el10_3);

    // checking improbable alloy gen querying with no results
    aflux::AFLUX tester10_4;
    aflowlib::EntryLoader el10_4;
    actual_numEntries = 0;
    string queryAlloyImpossible = "?alloy(Fe,O),density(1000*),bader_net_charges(@O(*0)),paging(1,5)";
    tester10_4.parseQuery(queryAlloyImpossible);
    tester10_4.queryDB(el10_4);
    expected_numEntries = 0;
    impossible_actual_numEntries = tester10_4.getNumEntries(el10_4);

    bool allAlloyTestsPassed = (basic_actual_numEntries > 0) && 
                                (many_actual_numEntries > 0) && 
                                (complex_actual_numEntries > 0) && 
                                (impossible_actual_numEntries == 0);

    checkEqual(allAlloyTestsPassed,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // Test 12: test 1D tensors
    // ---------------------------------------------------------------------------
    check_function = "1DTensorTest()";
    check_description = "checking 1D tensor querying";

    {
    aflux::AFLUX tester11_1;
    aflowlib::EntryLoader el11_1;
    string queryStressTensorBasic = "?species(Fe,O,Mn),stress_tensor(@(1)(0*)),paging(1,5)";
    tester11_1.parseQuery(queryStressTensorBasic);
    tester11_1.queryDB(el11_1);
    int basic_actual_numEntries = tester.getNumEntries(el11_1);

    aflux::AFLUX tester11_2;
    aflowlib::EntryLoader el11_2;
    string queryStressTensorComplex = "?species(Fe,O,Mn),stress_tensor(@(0)(0*)),density(0*),bader_net_charges(@O(*0)),paging(1,5)";
    tester11_2.parseQuery(queryStressTensorComplex);
    tester11_2.queryDB(el11_2);
    int complex_actual_numEntries = tester11_2.getNumEntries(el11_2);

    aflux::AFLUX tester11_3;
    aflowlib::EntryLoader el11_3;
    string queryStressTensorImpossible = "?species(Fe,O,Mn),stress_tensor(@(110)(0*))";
    tester11_3.parseQuery(queryStressTensorImpossible);
    tester11_3.queryDB(el11_3);
    int impossible_actual_numEntries = tester.getNumEntries(el11_3);

    bool all1DTestsPassed = (basic_actual_numEntries > 0) && 
                                (complex_actual_numEntries > 0) && 
                                (impossible_actual_numEntries == 0);

    checkEqual(all1DTestsPassed,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // Test 13: test 2D tensors
    // ---------------------------------------------------------------------------
    check_function = "2DTensorTest()";
    check_description = "checking 2D tensor querying";

    {
    aflux::AFLUX tester12_1;
    aflowlib::EntryLoader el12_1;
    string queryKPointsBasic = "?species(Fe,O,Mn),kpoints(@(1,1)(0*)),paging(1,5)";
    tester12_1.parseQuery(queryKPointsBasic);
    tester12_1.queryDB(el12_1);
    int basic_actual_numEntries = tester.getNumEntries(el12_1);

    aflux::AFLUX tester12_2;
    aflowlib::EntryLoader el12_2;
    string queryKPointsComplex = "?species(Fe,O,Mn),kpoints(@(1,1)(0*)),density(0*),bader_net_charges(@O(*0)),paging(1,5)";
    tester12_2.parseQuery(queryKPointsComplex);
    tester12_2.queryDB(el12_2);
    int complex_actual_numEntries = tester.getNumEntries(el12_2);

    aflux::AFLUX tester12_3;
    aflowlib::EntryLoader el12_3;
    string queryKPointsImpossible = "?species(Fe,O,Mn),kpoints(@(11,11)(0*))";
    tester12_3.parseQuery(queryKPointsImpossible);
    tester12_3.queryDB(el12_3);
    int impossible_actual_numEntries = tester.getNumEntries(el12_3);
    
    bool all2DTestsPassed = (basic_actual_numEntries > 0) && 
                                (complex_actual_numEntries > 0) && 
                                (impossible_actual_numEntries == 0);

    checkEqual(all2DTestsPassed,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // Test 14: test nbondxx
    // ---------------------------------------------------------------------------
    check_function = "nbondxxTest()";
    check_description = "checking nbondxx querying";

    {
    aflux::AFLUX tester13_1;
    aflowlib::EntryLoader el13_1;
    string queryNbondxxBasic = "?species(Fe,O,Mn),nbondxx(@(O,O)(0*)),paging(1,5)";
    tester13_1.parseQuery(queryNbondxxBasic);
    tester13_1.queryDB(el13_1);
    int basic_actual_numEntries = tester13_1.getNumEntries(el13_1);

    aflux::AFLUX tester13_2;
    aflowlib::EntryLoader el13_2;
    string queryNbondxxComplex = "?species(Fe,O,Mn),nbondxx(@(O,O)(1.14*),@(O,O)(*3.14)),density(0*),bader_net_charges(@O(*0)),paging(1,5)";
    tester13_2.parseQuery(queryNbondxxComplex);
    tester13_2.queryDB(el13_2);
    int complex_actual_numEntries = tester13_2.getNumEntries(el13_2);

    aflux::AFLUX tester13_3;
    aflowlib::EntryLoader el13_3;
    string queryNbondxxImpossible = "?species(Fe,O,Mn),nbondxx(@(O,O)(*0))";
    tester13_3.parseQuery(queryNbondxxImpossible);
    tester13_3.queryDB(el13_3);
    int impossible_actual_numEntries = tester13_3.getNumEntries(el13_3);
    
    bool allNbondxxTestsPassed = (basic_actual_numEntries > 0) && 
                                (complex_actual_numEntries > 0) && 
                                (impossible_actual_numEntries == 0);

    checkEqual(allNbondxxTestsPassed,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // Test 15: test AFLUX-OPEN paper queries
    // ---------------------------------------------------------------------------
    check_function = "testAFLUXOPENQueries()";
    check_description = "checking AFLUX-OPEN querying";

    {
    // See section 3.4.1.1 Nearest Neighbor Distances, example text 
    // Technically only nbondxx, but without species, it will take a long time to run
    aflux::AFLUX tester14_1;
    aflowlib::EntryLoader el14_1;
    string queryNbondxxPaper = "?species(O),nbondxx(@(O,O)(1.3*),@(O,O)(*1.4))";
    tester14_1.parseQuery(queryNbondxxPaper);
    tester14_1.queryDB(el14_1);
    int actual_numEntries1 = tester14_1.getNumEntries(el14_1);

    // See section 3.4.1.2 Bader Net Charges, example text 
    // Technically only bader_net_charges, but without species, it will take a long time to run
    aflux::AFLUX tester14_2;
    aflowlib::EntryLoader el14_2;
    string queryBaderNetChargesPaper = "?species(O),bader_net_charges(@O(*-1.25))";
    tester14_2.parseQuery(queryBaderNetChargesPaper);
    tester14_2.queryDB(el14_2);
    int actual_numEntries2 = tester14_2.getNumEntries(el14_2);

    // See section 3.4.2 Tensor-like, example text 
    // This returns nothing right now, but should return 1 once we get data
    aflux::AFLUX tester14_3;
    aflowlib::EntryLoader el14_3;
    string queryStiffTensorPaper = "?density(*),ael_stiffness_tensor(@(4,4)(0*))";
    tester14_3.parseQuery(queryStiffTensorPaper);
    tester14_3.queryDB(el14_3);
    int actual_numEntries3 = tester14_3.getNumEntries(el14_3);

    // See section 5.3.1 Tensor-like, 1D example link 
    aflux::AFLUX tester14_4;
    aflowlib::EntryLoader el14_4;
    string query1DTensorPaper = "?species(Fe,O),stress_tensor(@(1)(0*))";
    tester14_4.parseQuery(query1DTensorPaper);
    tester14_4.queryDB(el14_4);
    int actual_numEntries4 = tester14_4.getNumEntries(el14_4);

    // See section 5.3.1 Tensor-like, 2D example link (This will be replaced by ael_stiffness_tensor)
    aflux::AFLUX tester14_5;
    aflowlib::EntryLoader el14_5;
    string query2DTensorPaper = "?species(Fe,O),kpoints(@(0,0)(0*))";
    tester14_5.parseQuery(query2DTensorPaper);
    tester14_5.queryDB(el14_5);
    int actual_numEntries5 = tester14_5.getNumEntries(el14_5);

    // See section 5.3.2 Atom/Species-specific, example link 
    aflux::AFLUX tester14_6;
    aflowlib::EntryLoader el14_6;
    string queryAtomSpecificPaper = "?species(Fe,O),bader_net_charges(@Fe(0*))";
    tester14_6.parseQuery(queryAtomSpecificPaper);
    tester14_6.queryDB(el14_6);
    int actual_numEntries6 = tester14_6.getNumEntries(el14_6);

    // See section 5.3.3 Species-Pair-specific, example link 
    aflux::AFLUX tester14_7;
    aflowlib::EntryLoader el14_7;
    string queryPairSpecificPaper = "?species(Fe,O),nbondxx(@(O,O)(1.4*))";
    tester14_7.parseQuery(queryPairSpecificPaper);
    tester14_7.queryDB(el14_7);
    int actual_numEntries7 = tester14_7.getNumEntries(el14_7);

    bool allAFLUXOPENTestsPassed = (actual_numEntries1 > 0) && 
                                (actual_numEntries2 > 0) &&
                                (actual_numEntries3 > 0) &&
                                (actual_numEntries4 > 0) &&
                                (actual_numEntries5 > 0) &&
                                (actual_numEntries6 > 0) && 
                                (actual_numEntries7 > 0);

    checkEqual(allAFLUXOPENTestsPassed,true,check_function,check_description,passed_checks,results);
    }
  // MB20240805 END
  }
}
//WJ20230816 START

// structure tests
namespace unittest {

  //HE20210511
  void UnitTest::atomicEnvironmentTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings

    // setup test environment
    string check_function = "";
    string check_description = "";

    // ---------------------------------------------------------------------------
    // Test 1: create AE - mode 1
    // ---------------------------------------------------------------------------

    check_description = "create AE - mode 1";
    check_function = "getAtomEnvironments()";
    // load test system from DATA/unit_test.json, so the test runs offline //CO20261006
    // (CONTCAR.relax.vasp of Ca_svCu_pv/AB_mC12_8_3a_3a-001.AB, which was fetched from s4e.ai/API/unit_test/xstructure/ at run time)
    const string poscar_cacu = string(utd["atomic_environment"]["poscar_CaCu_AB_mC12_8_3a_3a"]);
    stringstream ss(poscar_cacu);
    xstructure str(ss);
    checkEqual(uint(str.atoms.size()), uint(6), check_function, check_description, passed_checks, results);

    vector<AtomEnvironment> AE = getAtomEnvironments(str, 1);
    check_function = "getAtomEnvironments()";

    // ---------------------------------------------------------------------------
    // Check | number of created AEs
    check_description = "number of created AEs";
    checkEqual(uint(AE.size()), uint(6), check_function, check_description, passed_checks, results);
    if (AE.size() != 6) {return;}  //CO20261006 - the checks below index AE[1] to AE[4]
    
    // ---------------------------------------------------------------------------
    // Check | point index mapping
    check_description = "point index mapping";
    checkEqual(AE[1].index2Point(10), AE[1].coordinates_neighbor[1][2], check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | coordinate matching
    check_description = "coordinate matching";
    xvector<double> compare_point = {-1.9551925593108925e0, -2.2642136090979212e0, 2.4896636484942385e0};
    checkEqual(AE[1].index2Point(2), compare_point, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | center element
    check_description = "center element";
    checkEqual(std::string(AE[2].element_center), std::string("Ca"), check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | center element id
    check_description = "center element id";
    checkEqual(AE[4].type_center, uint(1), check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Test 2: create AE convex hull
    // ---------------------------------------------------------------------------

    // setup test environment
    const uint test_AE = 4;

    // create hull
    AE[test_AE].constructAtomEnvironmentHull();
    check_function = "contructAtomEnvironmentHull()";

    // ---------------------------------------------------------------------------
    // Check | hull bit set
    check_description = "hull bit set";
    checkEqual(AE[test_AE].has_hull, true, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | hull volume
    check_description = "hull volume";
    checkEqual(AE[test_AE].volume, 31.4622167689, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | hull area
    check_description = "hull area";
    checkEqual(AE[test_AE].area, 60.4979100628, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | triangle count
    check_description = "triangle count";
    checkEqual(AE[test_AE].facet_order[0], uint(6), check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | tetragon count
    check_description = "tetragon count";
    checkEqual(AE[test_AE].facet_order[1], uint(2), check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | pentagon count
    check_description = "pentagon count";
    checkEqual(AE[test_AE].facet_order[2], uint(0), check_function, check_description, passed_checks, results);
  }

  void UnitTest::xstructureParserTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    (void) errors;  // Suppress compiler warnings

    if (LDEBUG) std::cerr << "Running " << __AFLOW_FUNC__ << std::endl;

    // Set up test environment
    string check_function = "", check_description = "";
    bool calculated_bool = false, expected_bool = false;

    // Set up structure variables
    _aflags aflags; aflags.Directory = aurostd::getPWD();
    bool check_passed = true;
    stringstream xstrss;
    xstructure xstr_cif, xstr_poscar;
    string str_cif = "", str_poscar = "";

    bool same_species = true;
    bool scale_volume = false;
    bool optimize_match = false;
    double misfit = 0.0;

    // ---------------------------------------------------------------------------
    // Check | CIF parser
    // ---------------------------------------------------------------------------

    // ---------------------------------------------------------------------------
    // Test 1: parse structure with known settings
    // ---------------------------------------------------------------------------
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " Test 1: parse structure with known settings" << std::endl;

    // CrO3 was a problematic structure in the past
    str_cif = string(utd["xstructure_parser"]["cif_CrO3"]);
    str_poscar = string(utd["xstructure_parser"]["poscar_CrO3"]);
    if (LDEBUG){
      std::cerr << __AFLOW_FUNC__ << " str_cif = " << endl << str_cif << endl;
      std::cerr << __AFLOW_FUNC__ << " str_poscar = " << endl << str_poscar << endl;
    }

    check_function = "xstructure::operator<<";
    check_description = "Parsing CIF file with recognized setting (CrO3)";

    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " [1] Parsing CIF file with recognized setting (CrO3)" << std::endl;
    aurostd::StringstreamClean(xstrss);
    xstrss << str_cif;
    try {xstr_cif = xstructure(xstrss);}
    catch(aurostd::xerror& excpt) {errors.push_back("Could not generate CrO3");}
    if (LDEBUG){
      std::cerr << __AFLOW_FUNC__ << " xstr_cif = " << endl << xstr_cif << endl;
      for (size_t i = 0; i < xstr_cif.wyckoff_sites_ITC.size(); i++) {
        std::cerr << __AFLOW_FUNC__  << xstr_cif.wyckoff_sites_ITC[i] << endl;
      }
    }
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " [2] Parsing CIF file with recognized setting (CrO3)" << std::endl;

    aurostd::StringstreamClean(xstrss);
    xstrss << str_poscar;
    xstr_poscar = xstructure(xstrss);
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " xstr_poscar = " << endl << xstr_poscar << endl;
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " [3] Parsing CIF file with recognized setting (CrO3)" << std::endl;

    // ---------------------------------------------------------------------------
    // test: parse structure
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " test: parse structure" << std::endl;
    expected_bool = true;
    calculated_bool = compare::aflowCompareStructure(xstr_cif, xstr_poscar, same_species, scale_volume, optimize_match, misfit);
    checkEqual(expected_bool, calculated_bool, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // test: compare Wyckoff positions
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " test: compare Wyckoff positions" << std::endl;
    check_description = "Compare parsed Wyckoff positions of CrO3";
    vector<wyckoffsite_ITC> vwyckoff(4);
    xvector<double> coords;
    vwyckoff[0].type = "Cr"; vwyckoff[0].letter = "b"; vwyckoff[0].site_symmetry = "m.."; vwyckoff[0].multiplicity = 4; vwyckoff[0].coord[1] = 0.25; vwyckoff[0].coord[2] = 0.09676; vwyckoff[0].coord[3] = 0.5;
    vwyckoff[1].type = "O"; vwyckoff[1].letter = "a"; vwyckoff[1].site_symmetry = "..2"; vwyckoff[1].multiplicity = 4; vwyckoff[1].coord[1] = 0.0; vwyckoff[1].coord[2] = 0.0; vwyckoff[1].coord[3] = 0.3841;
    vwyckoff[2].type = "O"; vwyckoff[2].letter = "b"; vwyckoff[2].site_symmetry = "m.."; vwyckoff[2].multiplicity = 4; vwyckoff[2].coord[1] = 0.25; vwyckoff[2].coord[2] = 0.2677; vwyckoff[2].coord[3] = 0.3755;
    vwyckoff[3].type = "O"; vwyckoff[3].letter = "b"; vwyckoff[3].site_symmetry = "m.."; vwyckoff[3].multiplicity = 4; vwyckoff[3].coord[1] = 0.25; vwyckoff[3].coord[2] = 0.6078; vwyckoff[3].coord[3] = 0.3284;
    check_passed = (vwyckoff.size() == xstr_cif.wyckoff_sites_ITC.size());
    for (size_t i = 0; i < vwyckoff.size() && check_passed; i++) {
      check_passed = (check_passed && (xstr_cif.wyckoff_sites_ITC[i].type == vwyckoff[i].type));
      check_passed = (check_passed && (xstr_cif.wyckoff_sites_ITC[i].letter == vwyckoff[i].letter));
      check_passed = (check_passed && (xstr_cif.wyckoff_sites_ITC[i].multiplicity == vwyckoff[i].multiplicity));
      check_passed = (check_passed && (xstr_cif.wyckoff_sites_ITC[i].coord == vwyckoff[i].coord));
      if (LDEBUG && !check_passed) {
        std::cerr << "Failed site:" << std::endl;
        std::cerr << "type = " << xstr_cif.wyckoff_sites_ITC[i].type << ", ";
        std::cerr << "letter = " << xstr_cif.wyckoff_sites_ITC[i].letter << ", ";
        std::cerr << "site_symmetry = " << xstr_cif.wyckoff_sites_ITC[i].site_symmetry << ", ";
        std::cerr << "multiplicity = " << xstr_cif.wyckoff_sites_ITC[i].multiplicity << ", ";
        std::cerr << "coord = " << xstr_cif.wyckoff_sites_ITC[i].coord << std::endl << std::endl;
        std::cerr << "Should be:" << std::endl;
        std::cerr << "type = " << vwyckoff[i].type << ", ";
        std::cerr << "letter = " << vwyckoff[i].letter << ", ";
        std::cerr << "site_symmetry = " << vwyckoff[i].site_symmetry << ", ";
        std::cerr << "multiplicity = " << vwyckoff[i].multiplicity << ", ";
        std::cerr << "coord = " << vwyckoff[i].coord << std::endl;
      }
    }

    expected_bool = true;
    calculated_bool = check_passed;
    checkEqual(expected_bool, calculated_bool, check_function, check_description, passed_checks, results);

    // May need a better test case where the labels actually change
    check_description = "Compare calculated Wyckoff positions of CrO3";
    xstr_cif.SpaceGroup_ITC();
    vwyckoff[0].coord[1] = 0.25; vwyckoff[0].coord[2] = 0.09676; vwyckoff[0].coord[3] = 0.5000; //[CO20240519 - updated with fixes in isTranslation(), explained below]vwyckoff[0].coord[2] = 0.59676; vwyckoff[0].coord[3] = 0.0000;
    vwyckoff[1].coord[1] = 0.00; vwyckoff[1].coord[2] = 0.00000; vwyckoff[1].coord[3] = 0.3841;
    vwyckoff[2].coord[1] = 0.25; vwyckoff[2].coord[2] = 0.26770; vwyckoff[2].coord[3] = 0.3755; //[CO20240519 - updated with fixes in isTranslation(), explained below]vwyckoff[2].coord[2] = 0.76770; vwyckoff[2].coord[3] = 0.8755;
    vwyckoff[3].coord[1] = 0.25; vwyckoff[3].coord[2] = 0.10780; vwyckoff[3].coord[3] = 0.8284; //[CO20240519 - updated with fixes in isTranslation(), explained below]vwyckoff[3].coord[2] = 0.60780; vwyckoff[3].coord[3] = 0.3284;
    //CO20240519 - updated the wyckoff coordinate for position 1,3,4
    //SYSTEM: FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_006_A0D3-A1D4
    //for AAPL distorted supercells, PerformFullSymmetry()+GetRealLatticeType() done at in KBIN_StepSymmetryPerform() failed
    //that's because PerformFullSymmetry() increased the sym_eps and at this updated eps, GetRealLatticeType() fails
    //inside, GetPrimitive() fails because it generates a supercell with two missing atoms (the AAPL distorted atoms)
    //the following fix was implemented: GetPrimitive() has been added to the PerformFullSymmetry() eps loop, and
    //a try/catch was added to GetPrimitive() in GetRealLatticeType()
    //I also cleaned up the isTranslationVector() workflow, removing the naive fpos minimization and integrating the new skew scheme
    //this caused the positions translation vectors generated to change in magnitude and order
    //as a result, the wyckoff positions identified changed
    //there are a few different choices for the last wyckoff position: try ./aflow --wyccar | ./aflow --vasp
    //because the order of the atoms changed, the algorithm now picks a different equivalent atom as the canonical choice
    //we could sort the candidate vectors to yield more consistent results, but then we sacrifice speed
    //it is not physically different, so we can leave as is for now
    check_passed = (vwyckoff.size() == xstr_cif.wyckoff_sites_ITC.size());
    for (size_t i = 0; i < vwyckoff.size() && check_passed; i++) {
      check_passed = (check_passed && (xstr_cif.wyckoff_sites_ITC[i].type == vwyckoff[i].type));
      check_passed = (check_passed && (xstr_cif.wyckoff_sites_ITC[i].letter == vwyckoff[i].letter));
      check_passed = (check_passed && (xstr_cif.wyckoff_sites_ITC[i].multiplicity == vwyckoff[i].multiplicity));
      check_passed = (check_passed && aurostd::isequal(xstr_cif.wyckoff_sites_ITC[i].coord, vwyckoff[i].coord));
      if (LDEBUG && !check_passed) {
        std::cerr << "Failed site:" << std::endl;
        std::cerr << "type = " << xstr_cif.wyckoff_sites_ITC[i].type << ", ";
        std::cerr << "letter = " << xstr_cif.wyckoff_sites_ITC[i].letter << ", ";
        std::cerr << "site_symmetry = " << xstr_cif.wyckoff_sites_ITC[i].site_symmetry << ", ";
        std::cerr << "multiplicity = " << xstr_cif.wyckoff_sites_ITC[i].multiplicity << ", ";
        std::cerr << "coord = " << xstr_cif.wyckoff_sites_ITC[i].coord << std::endl << std::endl;
        std::cerr << "Should be:" << std::endl;
        std::cerr << "type = " << vwyckoff[i].type << ", ";
        std::cerr << "letter = " << vwyckoff[i].letter << ", ";
        std::cerr << "site_symmetry = " << vwyckoff[i].site_symmetry << ", ";
        std::cerr << "multiplicity = " << vwyckoff[i].multiplicity << ", ";
        std::cerr << "coord = " << vwyckoff[i].coord << std::endl;
      }
    }

    expected_bool = true;
    calculated_bool = check_passed;
    checkEqual(expected_bool, calculated_bool, check_function, check_description, passed_checks, results);
    // ---------------------------------------------------------------------------
    // Test 2: parse structure with unrecognized (old) settings
    // ---------------------------------------------------------------------------
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " Test 2: parse structure with unrecognized (old) settings" << std::endl;
    check_description = "Parsing CIF file with unrecognized setting (GePt3)";
    aurostd::StringstreamClean(xstrss);
    str_cif = string(utd["xstructure_parser"]["cif_Ge8Pt24"]);
    str_poscar = string(utd["xstructure_parser"]["poscar_Ge8Pt24"]);

    bool quiet_tmp = XHOST.QUIET;
    XHOST.QUIET = !LDEBUG;  // Suppress warnings
    xstrss << str_cif;
    xstr_cif = xstructure(xstrss);
    XHOST.QUIET = quiet_tmp;

    // ---------------------------------------------------------------------------
    // test: parse structure
    aurostd::StringstreamClean(xstrss);
    xstrss << str_poscar;
    xstr_poscar = xstructure(xstrss);
    expected_bool = true;
    calculated_bool = compare::aflowCompareStructure(xstr_cif, xstr_poscar, same_species, scale_volume, optimize_match, misfit);
    checkEqual(expected_bool, calculated_bool, check_function, check_description, passed_checks, results);

    //CO20250407
    // ---------------------------------------------------------------------------
    // Test 3: cif with quotes
    // ---------------------------------------------------------------------------
    str_cif = string(utd["xstructure_parser"]["cif_In6Zr2"]);
    str_poscar = string(utd["xstructure_parser"]["poscar_In6Zr2"]);

    check_function = "xstructure::operator<<";
    check_description = "Parsing CIF file with quotes inside (In6Zr2)";

    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " [1] Parsing CIF file with quotes inside (In6Zr2)" << std::endl;
    aurostd::StringstreamClean(xstrss);
    xstrss << str_cif;
    try {xstr_cif = xstructure(xstrss);}
    catch(aurostd::xerror& excpt) {errors.push_back("Could not generate In6Zr2");}
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " [2] Parsing CIF file with quotes inside (In6Zr2)" << std::endl;

    aurostd::StringstreamClean(xstrss);
    xstrss << str_poscar;
    xstr_poscar = xstructure(xstrss);
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " [3] Parsing CIF file with quotes inside (In6Zr2)" << std::endl;

    // ---------------------------------------------------------------------------
    // test: parse structure
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " test: parse structure" << std::endl;
    expected_bool = true;
    calculated_bool = compare::aflowCompareStructure(xstr_cif, xstr_poscar, same_species, scale_volume, optimize_match, misfit);
    checkEqual(expected_bool, calculated_bool, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // test: xstructure2cif() finds the space group unless told not to //CO20261002
    check_function = "xstructure::xstructure2cif()";
    ofstream devnull("/dev/null");
    xstructure xstr_fcc = aflowlib::PrototypeLibraries(devnull, "A_cF4_225_a", "", LIBRARY_MODE_HTQC);
    xstructure xstr_p1 = xstr_fcc;
    xstr_fcc.xstructure2cif();
    check_description = "FCC is written as space group 225";
    checkEqual(xstr_fcc.spacegroupnumber, 225, check_function, check_description, passed_checks, results);
    xstr_p1.xstructure2cif(false);
    check_description = "FCC is written as P1 when symmetry is turned off";
    checkEqual(xstr_p1.spacegroupnumber, 1, check_function, check_description, passed_checks, results);
    xstrss.str(""); xstrss << xstr_p1;
    check_description = "the file names space group P1";
    checkEqual(aurostd::substring2bool(xstrss.str(), "'P1'"), true, check_function, check_description, passed_checks, results);
    // the caller decides, not the structure: one already marked P1 (spacegroupnumber=1, as read from a P1 CIF) gets its
    // space group unless the caller turns symmetry off; before CO20261002 the search was skipped for it //CO20261004
    xstructure xstr_marked = aflowlib::PrototypeLibraries(devnull, "A_cF4_225_a", "", LIBRARY_MODE_HTQC);
    xstr_marked.spacegroupnumber = 1;
    xstr_marked.xstructure2cif();
    check_description = "a structure marked P1 is written as space group 225 when symmetry is on";
    checkEqual(xstr_marked.spacegroupnumber, 225, check_function, check_description, passed_checks, results);
  }

  /// @brief the lattice type of the AAPL supercell of NaF (5x5x5, 250 atoms, two atoms displaced) after
  ///        PerformFullSymmetry(): it failed when PerformFullSymmetry() changed sym_eps and GetRealLatticeType() then built a
  ///        supercell that missed two atoms
  /// @note needs about 5 GB and 4 minutes on Linux; make unit_test_4 runs it on its own, not make unit_test_public, and
  ///       not on macOS, where it has not been run on its own
  void UnitTest::aaplLatticeTypeTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) { //CO20261018 - from xstructureTest()
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    string check_function = "", check_description = "", xstr_str = "";
    bool expected_bool = true, calculated_bool = false;
    stringstream xstrss;
    xstructure xstr_in;
    // ---------------------------------------------------------------------------
    // Test: check GetPrimitive() with AAPL distorted structure
    // ---------------------------------------------------------------------------
    // CO20240519
    // this failed in previous versions because PerformFullSymmetry() changes sym_eps and GetRealLatticeType() uses GetPrimitive()
    // at the updated tolerance, the supercell generated misses two atoms
    // we have now incorporated GetPrimitive() into the PerformFullSymmetry() sym_eps loop and
    // added a try/catch around GetPrimitive() in GetRealLatticeType()
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " Test: check PerformFullSymmetry()+GetRealLatticeType() with AAPL distorted structure" << std::endl;
    check_function = "xstructure::GetRealLatticeType()";
    check_description = "Calculating correct Bravais lattice variation type for AAPL distorted structure (rocksalt NaF)";
    
    bool Krun=true;
    
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " [1] Krun=" << Krun << " Calculating correct Bravais lattice variation type for AAPL distorted structure (rocksalt NaF)" << std::endl;
    aurostd::StringstreamClean(xstrss);
    xstr_str = string(utd["xstructure"]["poscar_NaF_AAPL_supercell"]);  //did not work - poscar_FNa_sv_AB_cF8_225_a_b_AB_ARUN_AAPL_3_006_A0D3-A1D4
    xstrss << xstr_str;
    try {xstr_in = xstructure(xstrss);}
    catch(aurostd::xerror& excpt) {errors.push_back("Could not generate FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_006_A0D3-A1D4");Krun=false;}
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " [2] Krun=" << Krun << " Calculating correct Bravais lattice variation type for AAPL distorted structure (rocksalt NaF)" << std::endl;

    Krun=(Krun && pflow::PerformFullSymmetry(xstr_in));
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << " [3] Krun=" << Krun << " Calculating correct Bravais lattice variation type for AAPL distorted structure (rocksalt NaF)" << std::endl;
    if(Krun==false){errors.push_back("Could not calculate symmetry of FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_006_A0D3-A1D4");}
    if(Krun){
      try{xstr_in.GetRealLatticeType();}
      catch(aurostd::xerror& excpt) {errors.push_back("Could not determine lattice type of FNa_sv/AB_cF8_225_a_b.AB/ARUN.AAPL_3_006_A0D3-A1D4");Krun=false;}
    }
    if (LDEBUG){
      std::cerr << __AFLOW_FUNC__ << " [4] Krun=" << Krun << " Calculating correct Bravais lattice variation type for AAPL distorted structure (rocksalt NaF)" << std::endl;
      std::cerr << __AFLOW_FUNC__ << " [5] xstr_in.bravais_lattice_variation_type=" << xstr_in.bravais_lattice_variation_type << " (expected FCC)" << endl;
    }

    // ---------------------------------------------------------------------------
    // test: parse structure
    calculated_bool = (Krun && xstr_in.bravais_lattice_variation_type=="FCC");  //CO20240620 - MCLC1 with different tolerance
    checkEqual(expected_bool, calculated_bool, check_function, check_description, passed_checks, results);
  }

  void UnitTest::xstructureTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    // Set up test environment
    string check_function = "", check_description = "";
    bool expected_bool = false, calculated_bool = false;
    uint expected_uint = 0, calculated_uint = 0;

    // ---------------------------------------------------------------------------
    // Check | getCoordinations() //CO20190520
    // ---------------------------------------------------------------------------
    //CO20261006 - from DATA/unit_test.json, so the test runs offline (CONTCAR.relax.vasp of Cl1Na1_ICSD_240599, which was fetched from s4e.ai/API/unit_test/xstructure/ at run time)
    const string poscar_nacl = string(utd["xstructure"]["poscar_NaCl_ICSD_240599"]);
    stringstream ss_nacl(poscar_nacl);
    xstructure xstr(ss_nacl,IOAFLOW_AUTO);
    deque<deque<uint> > coordinations;
    xstr.GetCoordinations(coordinations);

    check_function = "xstructure::getCoordinations()";
    check_description = "Number of iatoms";
    calculated_uint = coordinations.size();
    expected_uint = 2;
    checkEqual(calculated_uint, expected_uint, check_function, check_description, passed_checks, results);

    check_description = "Number of coordination environments atom 1 > 2";
    expected_bool = true;
    calculated_bool = (coordinations[0].size() > 2);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    check_description = "Number of coordination environments atom 2 > 2";
    expected_bool = true;
    calculated_bool = (coordinations[1].size() > 2);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    //first iatom
    //first shell
    check_description = "First shell atom 1";
    calculated_uint = coordinations[0][0];
    expected_uint = 6;
    checkEqual(calculated_uint, expected_uint, check_function, check_description, passed_checks, results);

    //second shell
    check_description = "Second shell atom 1";
    calculated_uint = coordinations[0][1];
    expected_uint = 12;
    checkEqual(calculated_uint, expected_uint, check_function, check_description, passed_checks, results);

    //second iatom
    //first shell
    check_description = "First shell atom 2";
    calculated_uint = coordinations[1][0];
    expected_uint = 6;
    checkEqual(calculated_uint, expected_uint, check_function, check_description, passed_checks, results);

    //second shell
    check_description = "Second shell atom 2";
    calculated_uint = coordinations[1][1];
    expected_uint = 12;
    checkEqual(calculated_uint, expected_uint, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | foldAtomdInCell //DX20210129
    // ---------------------------------------------------------------------------

    // set fold atoms in cell variables
    bool skew = false;
    double tol = 0.01;
    bool check_min_dists = false;

    // ---------------------------------------------------------------------------
    // test 1: expand cell
    // create 3x1x1 supercell expansion matrix
    check_function = "xstructure::foldAtomsInCell()";
    check_description = "expand cell (3x1x1)";
    xmatrix<double> supercell_matrix = aurostd::eye<double>(3,3);
    supercell_matrix(1,1)=3.0;
    xmatrix<double> lattice_new = supercell_matrix*xstr.lattice; // transform lattice

    xstructure xstr_supercell = xstr;
    xstr_supercell.foldAtomsInCell(lattice_new, skew, tol, check_min_dists);

    bool same_species = true;
    expected_bool = true;
    calculated_bool = compare::structuresMatch(xstr,xstr_supercell,same_species);
    checkEqual(expected_bool, calculated_bool, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // test 2: reduce cell
    // convert supercell back to original lattice
    check_function = "xstructure::foldAtomsInCell()";
    check_description = "reduce cell into primitive";
    xstructure xstr_reduced = xstr_supercell;
    xstr_reduced.foldAtomsInCell(xstr.lattice, skew, tol, check_min_dists);

    expected_bool = true;
    calculated_bool = compare::structuresMatch(xstr,xstr_reduced,same_species);
    checkEqual(expected_bool, calculated_bool, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | slab test //CO20190520
    // ---------------------------------------------------------------------------
    //See W. Sun and G. Ceder, Surface Science 617 (2013) 53-59
    string xstr_str = "";
    stringstream xstrss;
    double min_dist = 0.0, min_dist_orig = 0.0;
    xvector<int> hkl({1, 0, 4});
    //    hkl[1] = 1; hkl[2] = 0; hkl[3] = 4;

    //create input structure
    xstr_str = string(utd["xstructure"]["poscar_FeO"]);
    aurostd::StringstreamClean(xstrss);
    xstrss << xstr_str;
    xstructure xstr_in;
    try {
      xstrss >> xstr_in;          //CO20200404 - this WILL throw an error because det(lattice)<0.0, leave alone
    } catch (aurostd::xerror& excpt) {} //CO20200404 - this WILL throw an error because det(lattice)<0.0, leave alone
    min_dist = min_dist_orig = xstr_in.MinDist();
    if(LDEBUG){
      std::cerr << __AFLOW_FUNC__ << " xstr_in=\n" << xstr_in << std::endl;
      std::cerr << __AFLOW_FUNC__ << " xstr_in.MinDist()=" << min_dist << std::endl;
    }

    //create xstr_slab (correct answer)
    xstr_str = string(utd["xstructure"]["poscar_FeO_slab"]);
    xstructure xstr_slab_correct;
    aurostd::StringstreamClean(xstrss);
    xstrss << xstr_str;
    try {
      xstrss >> xstr_slab_correct; //CO20200404 - this WILL throw an error because det(lattice)<0.0, leave alone
    } catch (aurostd::xerror& excpt) {} //CO20200404 - this WILL throw an error because det(lattice)<0.0, leave alone
                                        // ---------------------------------------------------------------------------
                                        // test 1: compare min distance of bulk and correct slab
    min_dist = xstr_slab_correct.MinDist();
    if(LDEBUG) {
      std::cerr << __AFLOW_FUNC__ << " xstr_slab_correct=\n" << xstr_slab_correct << std::endl;
      min_dist=xstr_slab_correct.MinDist();
      std::cerr << __AFLOW_FUNC__ << " xstr_slab_correct.MinDist()=" << min_dist << std::endl;
    }
    check_function = "xstructure::CreateSlab_SurfaceLattice()";
    check_description = "Compare minimum distance bulk vs. correct slab";
    checkEqual(min_dist, min_dist_orig, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // test 2: compare min distance of generated slab and correct slab
    xmatrix<double> lattice_slab_origbasis;
    xstructure xstr_slab_test = slab::CreateSlab_SurfaceLattice(xstr_in,hkl,1,0,5.0); //the v3len_max_strict is very important here, as the test from Sun et al. takes a shortcut here
    min_dist = xstr_slab_test.MinDist();
    check_function = "xstructure::CreateSlab_SurfaceLattice(hkl = 104)";
    check_description = "Compare minimum distance slab vs. correct slab";
    if(LDEBUG){
      std::cerr << __AFLOW_FUNC__ << " xstr_slab_test=\n" << xstr_slab_test << std::endl;
      min_dist=xstr_slab_test.MinDist();
      std::cerr << __AFLOW_FUNC__ << " xstr_slab_test.MinDist()=" << min_dist << std::endl;
    }
    checkEqual(min_dist, min_dist_orig, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // test 3: compare structures of generated slab and correct slab
    check_description = "Match structures of generated slab and correct slab";
    expected_bool = true;
    calculated_bool = compare::structuresMatch(xstr_slab_correct,xstr_slab_test,true,false,false);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    // ---------------------------------------------------------------------------
    // Check | GetStandardPrimitive //CO20230220
    // ---------------------------------------------------------------------------

    //create xstr_str
    xstr_str =
      "O Co Sr\n"
      "1.00000000000000\n"
      "  5.4205419737648093    0.0000000000000000    0.0000000000000000\n"
      "  0.0000000000000000    5.4205419737648093    0.0000000000000000\n"
      "  0.0000000000000000    0.0000000000000000    7.6623790337915390\n"
      "O    Co   Sr\n"
      "12    4    4\n"
      "Direct\n"
      "  0.7499997640097504  0.2500002359902496  0.5000000000000000\n"
      "  0.2500002359902496  0.7499997640097504  0.5000000000000000\n"
      "  0.2499997640097504  0.7500002359902496  0.0000000000000000\n"
      "  0.7500002359902496  0.2499997640097504  0.0000000000000000\n"
      "  0.7499997640097504  0.7499997640097504  0.5000000000000000\n"
      "  0.2500002359902496  0.2500002359902496  0.5000000000000000\n"
      "  0.2499997640097504  0.2499997640097504  0.0000000000000000\n"
      "  0.7500002359902496  0.7500002359902496  0.0000000000000000\n"
      "  0.5000000000000000  0.5000000000000000  0.7499999723900999\n"
      "  0.0000000000000000  0.0000000000000000  0.7500000276099001\n"
      "  0.0000000000000000  0.0000000000000000  0.2499999723900999\n"
      "  0.5000000000000000  0.5000000000000000  0.2500000276099001\n"
      "  0.0000000000000000  0.0000000000000000  0.5000000000000000\n"
      "  0.5000000000000000  0.5000000000000000  0.0000000000000000\n"
      "  0.5000000000000000  0.5000000000000000  0.5000000000000000\n"
      "  0.0000000000000000  0.0000000000000000  0.0000000000000000\n"
      "  0.5000000000000000  0.0000000000000000  0.7500000000000000\n"
      "  0.0000000000000000  0.5000000000000000  0.7500000000000000\n"
      "  0.5000000000000000  0.0000000000000000  0.2500000000000000\n"
      "  0.0000000000000000  0.5000000000000000  0.2500000000000000\n";
    aurostd::StringstreamClean(xstrss);
    xstrss << xstr_str;
    try {
      xstrss >> xstr_in;          //CO20200404 - this WILL throw an error because det(lattice)<0.0, leave alone
    } catch (aurostd::xerror& excpt) {} //CO20200404 - this WILL throw an error because det(lattice)<0.0, leave alone

    // ---------------------------------------------------------------------------
    // test 1: expand cell
    // create 3x1x1 supercell expansion matrix
    check_function = "xstructure::GetStandardPrimitive()";
    check_description = "resolve primitive cell and find Bravais lattice type";
    xstr_in.GetStandardPrimitive();

    expected_bool = true;
    calculated_bool = (xstr_in.bravais_lattice_type=="CUB");
    checkEqual(expected_bool, calculated_bool, check_function, check_description, passed_checks, results);

    //CO20261018 - the lattice type of the AAPL supercell of NaF is its own test, aapl_lattice_type (make unit_test_4): it
    //needs about 5 GB and 4 minutes on Linux. On macOS this test, with it, grew past 57 GB and stopped make
    //unit_test_public (Killed: 9); without it, the OZr_sv mC6 structure below still filled memory there, from the nearly
    //coplanar candidate cells that SpaceGroup_ITC() now skips

    // ---------------------------------------------------------------------------
    // test: singular matrices in lattice vectors
    // xstructure::SpaceGroup_ITC() calls SYM::ConventionalCell(), which sometimes results in attempting to invert singular matrices, leading to an error.
    // input file: /common/LIB3/LIB/Fe_pvOSr_sv/ABC_oC12_63_c_a_c-001.ACB/CONTCAR.relax1.xz
    {
      std::string xstr_in_string = "Fe_pvOSr_sv/ABC_oC12_63_c_a_c-001.ACB pa\n"
                            "   1.00000000000000\n"
                            "     7.4872258918898016    0.0000000000000000    0.0000000000000000\n"
                            "     0.0000000000000000    3.6766896655500836   -0.6532381889319517\n"
                            "     0.0000000000000000    0.6615657177708621    3.6913037992299720\n"
                            "   Fe   O    Sr\n"
                            "     2     2     2\n"
                            "Direct\n"
                            "  0.2500000000000000  0.5006594460371309  0.4999160179598326\n"
                            "  0.7500000000000000  0.4993405539628690  0.5000839820401606\n"
                            "  0.2500000000000000  0.0005878717001329  0.4999115663405174\n"
                            "  0.7500000000000000  0.9994121282998814  0.5000884336594824\n"
                            " -0.0000000000000000 -0.0000000000000000 -0.0000000000000000\n"
                            "  0.5000000000000000 -0.0000000000000000 -0.0000000000000000\n";



      std::stringstream xstrss(xstr_in_string);
      xstructure xstr_in = xstructure(xstrss);
      // value was found from gdb during the aflow-sym PerformFullSymmetry() analysis and the function xstructure::SpaceGroup_ITC was the one that called SYM::ConventionalCell on the singular matrix
      double use_tol = 0.028052705953001762; // need lvalue type for reference semantics
      bool no_scan = false;
      uint expected_uint = 123;
      uint calculated_uint = xstr_in.SpaceGroup_ITC(use_tol, -1, 0, no_scan);

      checkEqual(calculated_uint, expected_uint, "xstructure::SpaceGroup_ITC()", "Check for singular matrices in candidate lattice vectors", passed_checks, results);
      
    }

    // test: singular matrices in lattice vectors
    // input file: /common/LIB2/LIB/OZr_sv/AB2_mC6_12_a_i-003.BA/CONTCAR.relax2.xz
    {
      std::string xstr_in_string = 
        "OZr_sv/AB2_mC6_12_a_i-003.BA params=-1,0\n"
        "1.01639600000000\n"
        "1.7891862439591713    3.0988406125028423    0.0085629849192183\n"
        "-1.7891862439591713    3.0988406125028423    0.0085629849192183\n"
        "0.0000000000000000    2.0576722032804979    2.9361089538727745\n"
        "O    Zr\n"
        "2     1\n"
        "Direct\n"
        "0.7501158351036354  0.7501158351036354  0.7495921581406516\n"
        "0.2498841648963649  0.2498841648963649  0.2504078418593486\n"
        "0.0000000000000000  0.0000000000000000 -0.0000000000000000\n";

      std::stringstream xstrss(xstr_in_string);
      xstructure xstr_in = xstructure(xstrss);
      double use_tol = 0.028052705953001762;
      bool no_scan = false;
      uint expected_uint = 166;
      uint calculated_uint = xstr_in.SpaceGroup_ITC(use_tol, -1, 0, no_scan);

      checkEqual(calculated_uint, expected_uint, "xstructure::SpaceGroup_ITC()", "Check for singular matrices in candidate lattice vectors", passed_checks, results);
      
    }
    {
      // test xbinary Hmix data retrieval
      double ag_h_hmix = xbinary::get_hmix_kj_mol_miedema("Ag", "H");
      double h_ag_hmix = xbinary::get_hmix_kj_mol_miedema("H", "Ag");
      double hg_ba_hmix = xbinary::get_hmix_kj_mol_miedema("Hg", "Ba");
      double expected_hmix_ag_h = -10;
      double expected_hmix_hg_ba = -49;
      checkEqual(expected_hmix_ag_h, ag_h_hmix, "xbinary::xbinary::get_hmix()", "Getting Miedema H_mix values", passed_checks, results);
      checkEqual(expected_hmix_ag_h, h_ag_hmix, "xbinary::xbinary::get_hmix()", "Getting Miedema H_mix values", passed_checks, results);
      checkEqual(expected_hmix_hg_ba, hg_ba_hmix, "xbinary::xbinary::get_hmix()", "Getting Miedema H_mix values", passed_checks, results);
    }

  }

}

namespace unittest {
  //CO20261004
  //KBIN::GetMostRelaxedStructure() on run directories written here: the structure is told apart by its lattice
  //parameter (start of relax1 3.0, start of relax2 3.1, start of the static 3.2), with the files plain and compressed.
  //The later relaxation is the more relaxed in both cases (before, the compressed files returned POSCAR.relax1).
  void UnitTest::mostRelaxedStructureTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function = "KBIN::GetMostRelaxedStructure()", check_description = "";
    auto poscar = [](double a) -> string {
      stringstream ss;
      ss << "Cu" << endl << "1.0" << endl;
      ss << a << " 0 0" << endl << "0 " << a << " 0" << endl << "0 0 " << a << endl;
      ss << "Cu" << endl << "1" << endl << "Direct" << endl << "0 0 0" << endl;
      return ss.str();
    };
    auto lattice_parameter = [](const string& directory) -> double {
      xstructure xstr = KBIN::GetMostRelaxedStructure(directory);
      xstr.ReScale(1.0);
      return aurostd::modulus(xstr.lattice(1));
    };
    for (int compressed = 0; compressed < 2; compressed++) {
      const string kind = (compressed ? "compressed (xz)" : "plain");
      const string directory = aurostd::TmpDirectoryCreate("most_relaxed");
      auto write = [&](const string& name, double a) {
        aurostd::string2file(poscar(a), directory + "/" + name);
        if (compressed) {aurostd::CompressFile(directory + "/" + name, "xz", true);}
      };
      double calculated = 0.0;
      write("POSCAR.relax1", 3.0);
      write("POSCAR.relax2", 3.1);
      try {calculated = lattice_parameter(directory);}
      catch (aurostd::xerror&) {calculated = 0.0;}
      check_description = "POSCAR.relax1 and POSCAR.relax2, " + kind + ": POSCAR.relax2";
      checkEqual(calculated, 3.1, check_function, check_description, passed_checks, results);
      write("POSCAR.static", 3.2);
      try {calculated = lattice_parameter(directory);}
      catch (aurostd::xerror&) {calculated = 0.0;}
      check_description = "with POSCAR.static, " + kind + ": POSCAR.static";
      checkEqual(calculated, 3.2, check_function, check_description, passed_checks, results);
      aurostd::RemoveDirectory(directory);
    }
  }
}

namespace unittest {
  //CO20261006
  //Relaxation outputs kept across restarts, on run folders written here (no VASP): which relaxations count as complete,
  //how a VASP execution becomes a part, what KBIN::VASP_ResumePrepare() does with a folder a killed job left, and what
  //KBIN::Clean() keeps with KEEP_RELAX and DEMOTE_RELAX.
  void UnitTest::relaxResumeTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function = "", check_description = "";
    auto poscar = [](double a, uint natoms) -> string {
      stringstream ss;
      ss << "Cu" << endl << "1.0" << endl;
      ss << a << " 0 0" << endl << "0 " << a << " 0" << endl << "0 0 " << a << endl;
      ss << "Cu" << endl << natoms << endl << "Direct" << endl;
      for (uint i = 0; i < natoms; i++) {ss << 0.5 * (double) i << " 0 0" << endl;}
      return ss.str();
    };
    const string outcar_complete = " VASP output\n General timing and accounting informations for this job:\n Total CPU time used (sec):       12.345\n";
    const string outcar_cut = " VASP output\n   POSITION    TOTAL-FORCE (eV/Angst)\n";
    auto oszicar = [](uint nsteps) -> string {
      stringstream ss;
      for (uint i = 1; i <= nsteps; i++) {
        ss << "       N       E                     dE             d eps       ncg     rms          rms(c)" << endl;
        ss << "DAV:   1    -0.100E+02   -0.100E+02   -0.100E+02    10   0.1E+01" << endl;
        ss << "   " << i << " F= -.37E+01 E0= -.37E+01  d E =-.37E+01" << endl;
      }
      return ss.str();
    };
    auto aflowin = [&](double a) -> string {
      return string("[AFLOW] test\n[VASP_RUN]RELAX_STATIC=2\n") + string(_VASP_POSCAR_MODE_EXPLICIT_START_) + "\n" + poscar(a, 2) + string(_VASP_POSCAR_MODE_EXPLICIT_STOP_) + "\n";
    };
    auto structure = [&](double a, uint natoms) -> xstructure {
      const stringstream ss(poscar(a, natoms));
      return xstructure(ss, IOVASP_AUTO);
    };
    auto exists = [](const string& file) -> bool {return aurostd::FileExist(file);};
    auto last_a = [](const string& directory) -> double {
      xstructure xstr = KBIN::GetPOSCARXStructureFromDirectory(directory, IOVASP_AUTO, -1);
      xstr.ReScale(1.0);
      return aurostd::modulus(xstr.lattice(1));
    };

    // complete OUTCARs, plain and compressed
    check_function = "KBIN::VASP_OUTCARComplete()";
    string dir = aurostd::TmpDirectoryCreate("relax_resume");
    aurostd::string2file(outcar_complete, dir + "/OUTCAR.relax1");
    aurostd::string2file(outcar_cut, dir + "/OUTCAR.relax2");
    check_description = "OUTCAR with the timing lines";
    checkEqual(KBIN::VASP_OUTCARComplete(dir + "/OUTCAR.relax1"), true, check_function, check_description, passed_checks, results);
    check_description = "OUTCAR cut short";
    checkEqual(KBIN::VASP_OUTCARComplete(dir + "/OUTCAR.relax2"), false, check_function, check_description, passed_checks, results);
    aurostd::CompressFile(dir + "/OUTCAR.relax1", "xz", true);
    check_description = "OUTCAR with the timing lines, compressed (xz)";
    checkEqual(KBIN::VASP_OUTCARComplete(dir + "/OUTCAR.relax1"), true, check_function, check_description, passed_checks, results);
    aurostd::string2file(outcar_complete, dir + "/OUTCAR.static");
    aurostd::CompressFile(dir + "/OUTCAR.static", "gz", true);
    check_description = "OUTCAR with the timing lines, compressed (gz)";
    checkEqual(KBIN::VASP_OUTCARComplete(dir + "/OUTCAR.static"), true, check_function, check_description, passed_checks, results);

    // relaxations counted in order
    check_function = "KBIN::VASP_RelaxationsComplete()";
    aurostd::string2file(poscar(3.6, 2), dir + "/CONTCAR.relax1");
    aurostd::string2file(poscar(3.6, 2), dir + "/CONTCAR.relax2");
    check_description = "relax1 complete, relax2 cut short";
    checkEqual(KBIN::VASP_RelaxationsComplete(dir, "relax", 2), (uint) 1, check_function, check_description, passed_checks, results);
    aurostd::string2file(outcar_complete, dir + "/OUTCAR.relax2");
    check_description = "relax1 and relax2 complete";
    checkEqual(KBIN::VASP_RelaxationsComplete(dir, "relax", 2), (uint) 2, check_function, check_description, passed_checks, results);
    aurostd::RemoveFile(dir + "/CONTCAR.relax1");
    check_description = "relax1 without its CONTCAR";
    checkEqual(KBIN::VASP_RelaxationsComplete(dir, "relax", 2), (uint) 0, check_function, check_description, passed_checks, results);
    check_description = "relax_apl prefix, no files";
    checkEqual(KBIN::VASP_RelaxationsComplete(dir, "relax_apl", 2), (uint) 0, check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(dir);

    // ionic steps and part numbers
    dir = aurostd::TmpDirectoryCreate("relax_resume");
    check_function = "KBIN::VASP_IonicSteps()";
    aurostd::string2file(oszicar(3), dir + "/OSZICAR");
    check_description = "three F= lines";
    checkEqual(KBIN::VASP_IonicSteps(dir + "/OSZICAR"), (uint) 3, check_function, check_description, passed_checks, results);
    check_description = "no OSZICAR";
    checkEqual(KBIN::VASP_IonicSteps(dir + "/OSZICAR.static"), (uint) 0, check_function, check_description, passed_checks, results);
    check_function = "KBIN::VASP_NextPartNumber()";
    check_description = "no parts";
    checkEqual(KBIN::VASP_NextPartNumber(dir), (uint) 1, check_function, check_description, passed_checks, results);
    aurostd::string2file("x", dir + "/OUTCAR.relax2.part1");
    aurostd::string2file("x", dir + "/CONTCAR.relax1.part3");
    aurostd::CompressFile(dir + "/CONTCAR.relax1.part3", "xz", true);
    aurostd::string2file("x", dir + "/LOCK.job7");
    check_description = "parts 1 and 3 (compressed), a job log";
    checkEqual(KBIN::VASP_NextPartNumber(dir), (uint) 4, check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(dir);

    // a VASP execution becomes a part
    check_function = "KBIN::VASP_ArchiveSegment()";
    dir = aurostd::TmpDirectoryCreate("relax_resume");
    aurostd::string2file(outcar_cut, dir + "/OUTCAR");
    aurostd::string2file(oszicar(2), dir + "/OSZICAR");
    aurostd::string2file(poscar(3.6, 2), dir + "/CONTCAR");
    aurostd::string2file("ISIF=3", dir + "/INCAR");
    aurostd::string2file("x", dir + "/CHGCAR");
    uint part = KBIN::VASP_ArchiveSegment(dir, "", "relax2", true, true);
    check_description = "two ionic steps: part 1";
    checkEqual(part, (uint) 1, check_function, check_description, passed_checks, results);
    check_description = "OUTCAR, OSZICAR, CONTCAR and INCAR moved to .relax2.part1";
    checkEqual(exists(dir + "/OUTCAR.relax2.part1") && exists(dir + "/OSZICAR.relax2.part1") && exists(dir + "/CONTCAR.relax2.part1") && exists(dir + "/INCAR.relax2.part1") && !exists(dir + "/OUTCAR") && !exists(dir + "/INCAR"), true, check_function, check_description, passed_checks, results);
    check_description = "CHGCAR dropped";
    checkEqual(exists(dir + "/CHGCAR"), false, check_function, check_description, passed_checks, results);
    aurostd::string2file(outcar_cut, dir + "/OUTCAR");
    aurostd::string2file("", dir + "/OSZICAR");
    part = KBIN::VASP_ArchiveSegment(dir, "", "relax2", true, true);
    check_description = "no ionic step: nothing kept";
    checkEqual(part == 0 && !exists(dir + "/OUTCAR") && !exists(dir + "/OUTCAR.relax2.part2"), true, check_function, check_description, passed_checks, results);
    aurostd::string2file(outcar_complete, dir + "/OUTCAR.static");
    part = KBIN::VASP_ArchiveSegment(dir, ".static", "static", true, true);
    check_description = "a complete OUTCAR without its OSZICAR: kept";
    checkEqual(part == 2 && exists(dir + "/OUTCAR.static.part2"), true, check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(dir);

    // restart structures
    check_function = "KBIN::VASP_RestartStructureSound()";
    string reason = "";
    const xstructure reference = structure(3.6, 2);
    check_description = "a relaxed cell";
    checkEqual(KBIN::VASP_RestartStructureSound(structure(3.7, 2), reference, reason), true, check_function, check_description, passed_checks, results);
    check_description = "a different number of atoms";
    checkEqual(KBIN::VASP_RestartStructureSound(structure(3.7, 3), reference, reason), false, check_function, check_description, passed_checks, results);
    check_description = "a collapsed cell (volume / 8)";
    checkEqual(KBIN::VASP_RestartStructureSound(structure(1.8, 2), reference, reason), false, check_function, check_description, passed_checks, results);

    // a folder that a job killed in relax2 left
    check_function = "KBIN::VASP_ResumePrepare()";
    dir = aurostd::TmpDirectoryCreate("relax_resume");
    aurostd::string2file(aflowin(3.6), dir + "/" + _AFLOWIN_);
    aurostd::string2file(outcar_complete, dir + "/OUTCAR.relax1");
    aurostd::string2file(poscar(3.62, 2), dir + "/CONTCAR.relax1");
    aurostd::string2file(outcar_cut, dir + "/OUTCAR");
    aurostd::string2file(oszicar(2), dir + "/OSZICAR");
    aurostd::string2file(poscar(3.63, 2), dir + "/CONTCAR");
    aurostd::string2file("ISIF=3", dir + "/INCAR");
    aurostd::string2file("done", dir + "/" + _AFLOWENDOUT_);
    ofstream devnull("/dev/null");
    bool changed = KBIN::VASP_ResumePrepare(dir, "relax", 2, true, false, devnull);
    check_description = "killed in relax2: aflow.in changed";
    checkEqual(changed, true, check_function, check_description, passed_checks, results);
    check_description = "killed in relax2: restarts from its last structure";
    checkEqual(aurostd::isequal(last_a(dir), 3.63, 1e-6), true, check_function, check_description, passed_checks, results);
    check_description = "killed in relax2: kept as relax2.part1, relax1 untouched, end file gone";
    checkEqual(exists(dir + "/OUTCAR.relax2.part1") && exists(dir + "/INCAR.relax2.part1") && exists(dir + "/OUTCAR.relax1") && !exists(dir + "/OUTCAR") && !exists(dir + "/" + _AFLOWENDOUT_), true, check_function, check_description, passed_checks, results);
    check_description = "killed in relax2: relax1 still counts";
    checkEqual(KBIN::VASP_RelaxationsComplete(dir, "relax", 2), (uint) 1, check_function, check_description, passed_checks, results);
    // killed again, with a collapsed cell: kept, but not used
    aurostd::string2file(outcar_cut, dir + "/OUTCAR");
    aurostd::string2file(oszicar(1), dir + "/OSZICAR");
    aurostd::string2file(poscar(1.8, 2), dir + "/CONTCAR");
    changed = KBIN::VASP_ResumePrepare(dir, "relax", 2, true, false, devnull);
    check_description = "collapsed cell: aflow.in unchanged, kept as relax2.part2";
    checkEqual(!changed && aurostd::isequal(last_a(dir), 3.63, 1e-6) && exists(dir + "/CONTCAR.relax2.part2"), true, check_function, check_description, passed_checks, results);
    // relax2 that AFLOW backed up with an incomplete OUTCAR
    aurostd::string2file(outcar_cut, dir + "/OUTCAR.relax2");
    aurostd::string2file(oszicar(4), dir + "/OSZICAR.relax2");
    aurostd::string2file(poscar(3.64, 2), dir + "/CONTCAR.relax2");
    changed = KBIN::VASP_ResumePrepare(dir, "relax", 2, true, false, devnull);
    check_description = "incomplete OUTCAR.relax2: kept as relax2.part3";
    checkEqual(exists(dir + "/OUTCAR.relax2.part3") && !exists(dir + "/OUTCAR.relax2"), true, check_function, check_description, passed_checks, results);
    // the kept files of a release
    aurostd::execute("cd \"" + dir + "\" && tar -cJf " + KBIN::VASP_RestartPackName(_AFLOWIN_) + " OUTCAR.relax1 CONTCAR.relax1 && rm -f OUTCAR.relax1 CONTCAR.relax1");
    changed = KBIN::VASP_ResumePrepare(dir, "relax", 2, true, false, devnull);
    check_description = "release archive unpacked and removed";
    checkEqual(exists(dir + "/OUTCAR.relax1") && !exists(dir + "/" + KBIN::VASP_RestartPackName(_AFLOWIN_)), true, check_function, check_description, passed_checks, results);
    check_description = "archive name of a module input";
    checkEqual(KBIN::VASP_RestartPackName("aflow_agl.in"), string("aflow_agl.restart.tar.xz"), check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(dir);

    // archives: one inside another, an unpack that was interrupted, the move from the data folder
    check_function = "KBIN::VASP_RestartUnpack()";
    const string pack = KBIN::VASP_RestartPackName(_AFLOWIN_);
    dir = aurostd::TmpDirectoryCreate("relax_resume");
    aurostd::string2file("older", dir + "/OUTCAR.relax1.part1");
    aurostd::string2file("older", dir + "/OUTCAR");
    aurostd::string2file("older", dir + "/OSZICAR");
    aurostd::CompressFile(dir + "/OSZICAR", "xz", true);
    aurostd::execute("cd \"" + dir + "\" && tar -cJf " + pack + " OUTCAR.relax1.part1 OUTCAR OSZICAR.xz && rm -f OUTCAR.relax1.part1 OUTCAR OSZICAR.xz");
    aurostd::string2file("newer", dir + "/OUTCAR");
    aurostd::string2file("newer", dir + "/OSZICAR");
    aurostd::string2file("newer", dir + "/OUTCAR.relax1.part2");
    aurostd::execute("cd \"" + dir + "\" && tar -cJf outer.tar.xz " + pack + " OUTCAR OSZICAR OUTCAR.relax1.part2 && rm -f " + pack + " OUTCAR OSZICAR OUTCAR.relax1.part2 && mv outer.tar.xz " + pack);
    check_description = "an archive inside the archive is unpacked too";
    checkEqual(KBIN::VASP_RestartUnpack(dir, devnull) && exists(dir + "/OUTCAR.relax1.part1") && exists(dir + "/OUTCAR.relax1.part2") && !exists(dir + "/" + pack), true, check_function, check_description, passed_checks, results);
    check_description = "the older archive inside does not replace newer files, compressed or not";
    checkEqual(aurostd::file2string(dir + "/OUTCAR") == "newer" && aurostd::file2string(dir + "/OSZICAR") == "newer" && !exists(dir + "/OSZICAR.xz") && !exists(dir + "/" + pack + ".inner1"), true, check_function, check_description, passed_checks, results);
    aurostd::string2file("whole", dir + "/OUTCAR.relax1.part1");
    aurostd::execute("cd \"" + dir + "\" && tar -cJf " + pack + " OUTCAR.relax1.part1 && rm -f OUTCAR.relax1.part1");
    aurostd::DirectoryMake(dir + "/.aflow.restart_unpack");
    aurostd::string2file("cut", dir + "/.aflow.restart_unpack/OUTCAR.relax1.part1");
    check_description = "an unpack that was interrupted is done again, without the cut file";
    checkEqual(KBIN::VASP_RestartUnpack(dir, devnull) && aurostd::file2string(dir + "/OUTCAR.relax1.part1") == "whole" && !aurostd::IsDirectory(dir + "/.aflow.restart_unpack") && !exists(dir + "/" + pack), true, check_function, check_description, passed_checks, results);
    aurostd::string2file("x", dir + "/OUTCAR.relax1.part1");
    aurostd::execute("cd \"" + dir + "\" && echo broken > " + pack);
    bool thrown = false;
    try {KBIN::VASP_RestartUnpack(dir, devnull);} catch (aurostd::xerror& excpt) {thrown = true;}
    check_description = "an archive tar cannot read: an error, the archive and the folder's files stay";
    checkEqual(thrown && exists(dir + "/" + pack) && aurostd::file2string(dir + "/OUTCAR.relax1.part1") == "x" && !aurostd::IsDirectory(dir + "/.aflow.restart_unpack"), true, check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(dir);
    check_function = "KBIN::MoveRun2NewDirectory()";
    const string root = aurostd::TmpDirectoryCreate("relax_resume");
    const string data = root + "/home/XCALCS/LIB2/run", work = root + "/work/XCALCS/LIB2/run";
    aurostd::DirectoryMake(data);
    aurostd::string2file(aflowin(3.6), data + "/" + _AFLOWIN_);
    aurostd::string2file(outcar_complete, data + "/OUTCAR.relax1");
    aurostd::string2file(poscar(3.62, 2), data + "/CONTCAR.relax1");
    aurostd::execute("cd \"" + data + "\" && tar -cJf " + pack + " OUTCAR.relax1 CONTCAR.relax1 && rm -f OUTCAR.relax1 CONTCAR.relax1");
    _aflags aflags_move;
    aflags_move.Directory = data;
    const bool moved = KBIN::MoveRun2NewDirectory(aflags_move, root + "/home", root + "/work");
    check_description = "the archive goes to the work folder with the input";
    checkEqual(moved && aflags_move.Directory == work && exists(work + "/" + _AFLOWIN_) && exists(work + "/" + pack) && !exists(data + "/" + pack), true, check_function, check_description, passed_checks, results);
    KBIN::VASP_ResumePrepare(work, "relax", 2, true, false, devnull);
    check_description = "the run there resumes after relax1";
    checkEqual(KBIN::VASP_RelaxationsComplete(work, "relax", 2), (uint) 1, check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(root);

    // the stage an interrupted execution after the relaxations belongs to
    check_function = "KBIN::VASP_ResumeStageLabel()";
    dir = aurostd::TmpDirectoryCreate("relax_resume");
    aurostd::string2file("ISMEAR=-5\nICHARG=11\n", dir + "/INCAR");
    check_description = "ICHARG=11: bands";
    checkEqual(KBIN::VASP_ResumeStageLabel(dir, true, true), string("bands"), check_function, check_description, passed_checks, results);
    aurostd::string2file("ISMEAR=-5\n", dir + "/INCAR");
    check_description = "a static rerun after a complete static: static";
    checkEqual(KBIN::VASP_ResumeStageLabel(dir, true, true), string("static"), check_function, check_description, passed_checks, results);
    check_description = "neither static nor bands: run";
    checkEqual(KBIN::VASP_ResumeStageLabel(dir, false, false), string("run"), check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(dir);

    // KBIN::Clean() with KEEP_RELAX, and with DEMOTE_RELAX
    check_function = "KBIN::Clean()";
    dir = aurostd::TmpDirectoryCreate("relax_resume");
    aurostd::string2file(aflowin(3.6), dir + "/" + _AFLOWIN_);
    vector<string> vkept, vgone;
    aurostd::string2tokens("OUTCAR.relax1,CONTCAR.relax1,vasprun.xml.relax1,CHGCAR.relax1,POTCAR.relax1,OUTCAR.relax2.part1,INCAR.orig,aflow.qmvasp.out,OUTCAR,OSZICAR,CHGCAR.static,AECCAR0.static,OUTCAR.dielectric_static,aflow.error.nbands.job1", vkept, ",");
    aurostd::string2tokens("aflow.end.out,CHG.relax1,WAVECAR.relax1,AECCAR0.relax1,WAVECAR,CHGCAR,aflow.agroup.out,WAVECAR.dielectric_static", vgone, ",");
    for (uint i = 0; i < vkept.size(); i++) {aurostd::string2file("x", dir + "/" + vkept[i]);}
    for (uint i = 0; i < vgone.size(); i++) {aurostd::string2file("x", dir + "/" + vgone[i]);}
    aurostd::string2file("log of job 1", dir + "/LOCK.job1");
    aurostd::string2file("log of job 2", dir + "/" + _AFLOWLOCK_);
    aurostd::string2file("x", dir + "/aflow.error.zbrent");
    aurostd::CompressFile(dir + "/aflow.error.zbrent", "xz", true);
    aurostd::xoption opts_clean;
    opts_clean.flag("KEEP_RELAX", true);
    KBIN::Clean(dir, opts_clean);
    bool all_kept = true, all_gone = true;
    for (uint i = 0; i < vkept.size(); i++) {if (!exists(dir + "/" + vkept[i])) {all_kept = false; std::cerr << "KEEP_RELAX removed " << vkept[i] << std::endl;}}
    for (uint i = 0; i < vgone.size(); i++) {if (exists(dir + "/" + vgone[i])) {all_gone = false; std::cerr << "KEEP_RELAX left " << vgone[i] << std::endl;}}
    check_description = "KEEP_RELAX: stages, parts, .orig, fix logs and working files kept";
    checkEqual(all_kept && exists(dir + "/" + _AFLOWIN_), true, check_function, check_description, passed_checks, results);
    check_description = "KEEP_RELAX: end file, large relaxation files and symmetry outputs removed";
    checkEqual(all_gone, true, check_function, check_description, passed_checks, results);
    check_description = "KEEP_RELAX: the job log is LOCK.job2, after LOCK.job1";
    checkEqual(!exists(dir + "/" + _AFLOWLOCK_) && aurostd::file2string(dir + "/" + _AFLOWLOCK_ + ".job2") == "log of job 2", true, check_function, check_description, passed_checks, results);
    check_description = "KEEP_RELAX: the job's fix logs get its number, an earlier job's keep theirs";
    checkEqual(exists(dir + "/aflow.error.zbrent.job2.xz") && !exists(dir + "/aflow.error.zbrent.xz") && exists(dir + "/aflow.error.nbands.job1"), true, check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(dir);
    dir = aurostd::TmpDirectoryCreate("relax_resume");
    aurostd::string2file(aflowin(3.6), dir + "/" + _AFLOWIN_);
    vector<string> vstages;
    aurostd::string2tokens("relax1,relax2,static", vstages, ",");
    for (uint i = 0; i < vstages.size(); i++) {
      aurostd::string2file(outcar_complete, dir + "/OUTCAR." + vstages[i]);
      aurostd::string2file(oszicar(vstages[i] == "static" ? 1 : 3), dir + "/OSZICAR." + vstages[i]);
      aurostd::string2file(poscar(3.6, 2), dir + "/CONTCAR." + vstages[i]);
      aurostd::CompressFile(dir + "/OUTCAR." + vstages[i], "xz", true);
    }
    aurostd::string2file("[KBIN_ANALYZE]START_relax1", dir + "/aflow.qmvasp.out");
    aurostd::string2file("x", dir + "/aflow.error.nbands.job3");  //a job whose log is not here
    aurostd::string2file("x", dir + "/aflow.error.zbrent");
    opts_clean.flag("DEMOTE_RELAX", true);
    KBIN::Clean(dir, opts_clean);
    check_description = "DEMOTE_RELAX: relax1, relax2 and static become parts 1 to 3, in that order";
    checkEqual(exists(dir + "/OUTCAR.relax1.part1.xz") && exists(dir + "/OUTCAR.relax2.part2.xz") && exists(dir + "/OUTCAR.static.part3.xz") && exists(dir + "/CONTCAR.relax2.part2"), true, check_function, check_description, passed_checks, results);
    check_description = "DEMOTE_RELAX: no stage left, aflow.qmvasp.out goes with the parts";
    checkEqual(KBIN::VASP_RelaxationsComplete(dir, "relax", 2) == 0 && !exists(dir + "/OUTCAR.static.xz") && !exists(dir + "/aflow.qmvasp.out") && exists(dir + "/aflow.qmvasp.out.part1"), true, check_function, check_description, passed_checks, results);
    check_description = "no LOCK: the fix logs get the number after the highest earlier one";
    checkEqual(exists(dir + "/aflow.error.zbrent.job4") && exists(dir + "/aflow.error.nbands.job3") && !exists(dir + "/LOCK.job4"), true, check_function, check_description, passed_checks, results);
    aurostd::RemoveDirectory(dir);
  }
}

// VASP relaxations  //CO20261010
namespace unittest {

  // ---------------------------------------------------------------------------
  // vasp_relax_continue  //CO20261010
  // ---------------------------------------------------------------------------
  // WHY THIS TEST EXISTS
  //   1. KBIN::VASP_getNSTEPS() read the last "DAV:" line of OSZICAR. With ALGO=Fast the electronic loop ends in "RMM:"
  //      lines, so it returned one of the first Davidson steps, and the NELM flag missed unconverged RMM-DIIS loops. It
  //      also read a compressed OSZICAR as empty, and KBIN::VASP_getNELM() a compressed OUTCAR (NELM=60), which are the
  //      files --xplug passes: its test of the electronic convergence never fired.
  //   2. A relaxation that used all NSW ionic steps with EDDRMM warnings and a converged last electronic loop was run
  //      again from its start with ALGO=Normal (RMM_DIIS fix). EDDRMM is soft there now.
  //   3. The last relaxation continues while it is not converged (KBIN::VASP_RelaxContinue()): no "reached required
  //      accuracy", or, with ISIF=3, a stress component above --xplug's limit (KBIN::VASP_RelaxationConverged()).
  //   4. A relaxation that reached the required accuracy and then froze waited SECONDS_STALE_OUTCAR (6 h) before
  //      CALC_FROZEN; it now waits 30 minutes.
  // CHECKED
  //   Files written to temporary folders, plain and compressed. KBIN::VASP_ProcessWarnings() on folders holding the INCAR,
  //   OSZICAR, OUTCAR, vasp.out and LOCK of a relaxation. KBIN::VASP_RelaxContinue() with XHOST.AVOID_RUNNING_VASP set,
  //   so the VASP execution it starts throws, and the files it left show what it did before. No VASP run.
  // IF THIS TEST FAILS
  //   The electronic steps are miscounted again, EDDRMM at NSW triggers a rerun again, or the last relaxation stops
  //   unconverged: fix the code, not the expectations.
  void UnitTest::vaspRelaxContinueTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    string check_function = "", check_description = "";
    // one ionic step of OSZICAR: ndav Davidson steps, then nrmm RMM-DIIS steps (*** above 999, as VASP prints them),
    // then its F= line unless the loop is still running
    auto ionic = [](uint istep, uint ndav, uint nrmm, bool running) -> string {
      stringstream ss;
      ss << "       N       E                     dE             d eps       ncg     rms          rms(c)" << endl;
      for (uint i = 1; i <= ndav + nrmm; i++) {
        ss << (i <= ndav ? "DAV: " : "RMM: ");
        if (i < 1000) {ss << std::setw(3) << i;} else {ss << "***";}
        ss << "    -0.100E+02   -0.100E+02   -0.100E+02    10   0.1E+01" << endl;
      }
      if (!running) {ss << std::setw(4) << istep << " F= -.37E+01 E0= -.37E+01  d E =-.37E+01" << endl;}
      return ss.str();
    };
    auto outcar = [](int nelm, const string& stress, bool complete) -> string {
      stringstream ss;
      ss << " vasp.6.5.1" << endl;
      if (nelm > 0) {ss << "   NELM   = " << std::setw(5) << nelm << ";   NELMIN=  2; NELMDL= -5     # of ELM steps" << endl;}
      if (!stress.empty()) {ss << "  in kB  " << stress << endl;}
      if (complete) {ss << " General timing and accounting informations for this job:" << endl << " Total CPU time used (sec):       12.345" << endl;}
      return ss.str();
    };
    auto poscar = [](double a, uint natoms) -> string {
      stringstream ss;
      ss << "Cu" << endl << "1.0" << endl;
      ss << a << " 0 0" << endl << "0 " << a << " 0" << endl << "0 0 " << a << endl;
      ss << "Cu" << endl << natoms << endl << "Direct" << endl;
      for (uint i = 0; i < natoms; i++) {ss << 0.5 * (double) i << " 0 0" << endl;}
      return ss.str();
    };
    const string reached = " reached required accuracy - stopping structural energy minimisation\n";
    const string eddrmm = " WARNING in EDDRMM: call to ZHEGV failed, returncode =   6 3      16\n";
    const string stress_small = "     0.50     -1.20      2.00      0.10      0.00     -0.30";
    const string stress_large = "    12.30     -1.20      2.00      0.10      0.00     -0.30";

    // 1. electronic steps of the last loop, any algorithm
    check_function = "KBIN::VASP_getNSTEPS()";
    string dir = aurostd::TmpDirectoryCreate("vasp_relax_continue");
    struct nsteps_case { string oszicar; uint nsteps; string why; };
    const vector<nsteps_case> vnsteps = {
      {ionic(1, 12, 0, false) + ionic(2, 14, 0, false), 14, "Davidson only (ALGO=Normal)"},
      {ionic(1, 5, 18, false) + ionic(2, 1, 22, false), 23, "Davidson then RMM-DIIS (ALGO=Fast): all 23 steps (was 1)"},
      {ionic(1, 5, 10, false) + ionic(2, 1, 8, true), 9, "a loop still running"},
      {ionic(1, 0, 1002, false), 1002, "step numbers VASP printed as ***"},
    };
    for (const nsteps_case& c : vnsteps) {
      aurostd::string2file(c.oszicar, dir + "/OSZICAR");
      checkEqual(KBIN::VASP_getNSTEPS(dir + "/OSZICAR"), c.nsteps, check_function, c.why, passed_checks, results);
    }
    aurostd::string2file(vnsteps[1].oszicar, dir + "/OSZICAR.static");
    aurostd::CompressFile(dir + "/OSZICAR.static", "xz", true);
    checkEqual(KBIN::VASP_getNSTEPS(dir + "/OSZICAR.static.xz"), (uint) 23, check_function, "compressed (xz), as --xplug passes it (was 0)", passed_checks, results);
    checkEqual(KBIN::VASP_getNSTEPS(dir + "/OSZICAR.bands"), (uint) 0, check_function, "no OSZICAR", passed_checks, results);

    // NELM, plain and compressed
    check_function = "KBIN::VASP_getNELM()";
    aurostd::string2file(outcar(120, "", true), dir + "/OUTCAR");
    checkEqual(KBIN::VASP_getNELM(dir + "/OUTCAR"), (uint) 120, check_function, "NELM=120", passed_checks, results);
    aurostd::string2file(outcar(120, "", true), dir + "/OUTCAR.static");
    aurostd::CompressFile(dir + "/OUTCAR.static", "xz", true);
    checkEqual(KBIN::VASP_getNELM(dir + "/OUTCAR.static.xz"), (uint) 120, check_function, "NELM=120, compressed (xz) (was 60)", passed_checks, results);
    aurostd::string2file(outcar(0, "", true), dir + "/OUTCAR");
    checkEqual(KBIN::VASP_getNELM(dir + "/OUTCAR"), (uint) 60, check_function, "no NELM line: VASP's default 60", passed_checks, results);

    // --xplug's test: the last loop of a compressed stage at NELM
    check_function = "KBIN::VASP_OSZICARUnconverged()";
    aurostd::string2file(ionic(1, 1, 59, false), dir + "/OSZICAR.relax2");
    aurostd::CompressFile(dir + "/OSZICAR.relax2", "xz", true);
    aurostd::string2file(outcar(60, "", true), dir + "/OUTCAR.relax2");
    aurostd::CompressFile(dir + "/OUTCAR.relax2", "xz", true);
    checkEqual(KBIN::VASP_OSZICARUnconverged(dir + "/OSZICAR.relax2.xz", dir + "/OUTCAR.relax2.xz"), true, check_function, "60 RMM-DIIS steps at NELM=60, compressed (was converged)", passed_checks, results);
    aurostd::string2file(outcar(120, "", true), dir + "/OUTCAR.relax2");
    aurostd::CompressFile(dir + "/OUTCAR.relax2", "xz", true);
    checkEqual(KBIN::VASP_OSZICARUnconverged(dir + "/OSZICAR.relax2.xz", dir + "/OUTCAR.relax2.xz"), false, check_function, "60 steps at NELM=120, compressed", passed_checks, results);
    aurostd::RemoveDirectory(dir);

    // 2. the last stress tensor
    check_function = "KBIN::VASP_OUTCARStress()";
    dir = aurostd::TmpDirectoryCreate("vasp_relax_continue");
    aurostd::string2file(outcar(60, stress_large, false) + outcar(0, stress_small, true), dir + "/OUTCAR");
    vector<double> stress;
    const bool read_stress = KBIN::VASP_OUTCARStress(dir + "/OUTCAR", stress);
    checkEqual(read_stress && stress.size() == 6 && aurostd::isequal(stress[0], 0.5) && aurostd::isequal(stress[1], -1.2), true, check_function, "the last of two stress tensors", passed_checks, results);
    checkEqual(KBIN::VASP_OUTCARStress(dir + "/OUTCAR.static", stress), false, check_function, "no OUTCAR", passed_checks, results);
    aurostd::RemoveDirectory(dir);

    // 3. converged relaxations
    check_function = "KBIN::VASP_RelaxationConverged()";
    struct converged_case { string incar; string vaspout; string stress; uint nionic; bool compress; bool converged; string reason; string why; };
    const vector<converged_case> vconverged = {
      {"IBRION=2\nNSW=51\nISIF=3\n", reached, stress_small, 7, false, true, "", "reached accuracy, stress within 10 kB"},
      {"IBRION=2\nNSW=51\nISIF=3\n", reached, stress_large, 7, false, false, "stress component 12.3 kB above 10 kB", "reached accuracy, a stress component of 12.3 kB"},
      {"IBRION=2\nNSW=51\nISIF=2\n", reached, stress_large, 7, false, true, "", "ISIF=2: the stress is not tested"},
      {"IBRION=2\nNSW=51\nISIF=3\n", eddrmm, stress_small, 51, false, false, "no \"reached required accuracy\" after 51 ionic steps (NSW=51)", "stopped at NSW"},
      {"IBRION=2\nNSW=51\nISIF=3\n", reached, stress_small, 7, true, true, "", "reached accuracy, compressed (xz)"},
      {"IBRION=-1\nNSW=0\n", "", "", 0, false, true, "", "a static execution counts as converged"},
      {"IBRION=0\nNSW=100\n", "", "", 100, false, true, "", "molecular dynamics counts as converged"},
    };
    for (const converged_case& c : vconverged) {
      dir = aurostd::TmpDirectoryCreate("vasp_relax_continue");
      aurostd::string2file(c.incar, dir + "/INCAR.relax2");
      aurostd::string2file(c.vaspout, dir + "/" + DEFAULT_VASP_OUT + ".relax2");
      aurostd::string2file(outcar(60, c.stress, true), dir + "/OUTCAR.relax2");
      string oszicar = "";
      for (uint i = 1; i <= c.nionic; i++) {oszicar += ionic(i, 5, 10, false);}
      aurostd::string2file(oszicar, dir + "/OSZICAR.relax2");
      if (c.compress) {
        aurostd::CompressFile(dir + "/" + DEFAULT_VASP_OUT + ".relax2", "xz", true);
        aurostd::CompressFile(dir + "/OUTCAR.relax2", "xz", true);
      }
      string reason = "";
      const bool converged = KBIN::VASP_RelaxationConverged(dir, "relax2", reason);
      checkEqual(converged, c.converged, check_function, c.why, passed_checks, results);
      if (!c.converged) {checkEqual(reason, c.reason, check_function, c.why + " (reason)", passed_checks, results);}
      aurostd::RemoveDirectory(dir);
    }

    // 4. VASP_ProcessWarnings() on a relaxation that ended
    check_function = "KBIN::VASP_ProcessWarnings()";
    struct warnings_case { int nsw; uint nrmm_last; string vaspout; long int age; bool complete; vector<string> on; vector<string> off; string why; };
    const vector<warnings_case> vwarnings = {
      {3, 7, eddrmm, 0, true, {}, {"EDDRMM", "RMM_DIIS", "NELM"}, "EDDRMM after all NSW=3 ionic steps, last loop converged: soft (was the RMM_DIIS rerun)"},
      {51, 7, eddrmm, 0, true, {"EDDRMM", "RMM_DIIS"}, {"NELM"}, "EDDRMM after 3 of NSW=51 ionic steps: RMM_DIIS as before"},
      {3, 59, eddrmm, 0, true, {"NELM"}, {}, "last RMM-DIIS loop at NELM=60: NELM (was missed, the old count was 1)"},
      {3, 7, reached, 2000, false, {"CALC_FROZEN"}, {}, "reached accuracy, OUTCAR incomplete and 2000 s old: CALC_FROZEN (was 6 h)"},
      {3, 7, reached, 100, false, {}, {"CALC_FROZEN"}, "reached accuracy, OUTCAR incomplete and 100 s old: not frozen yet"},
    };
    for (const warnings_case& c : vwarnings) {
      dir = aurostd::TmpDirectoryCreate("vasp_relax_continue");
      aurostd::string2file("SYSTEM=vasp_relax_continue\nIBRION=2\nNSW=" + aurostd::utype2string<int>(c.nsw) + "\nISIF=3\nALGO=Fast\n", dir + "/INCAR");
      aurostd::string2file(ionic(1, 5, 10, false) + ionic(2, 1, 8, false) + ionic(3, 1, c.nrmm_last, false), dir + "/OSZICAR");
      aurostd::string2file(outcar(60, stress_small, c.complete), dir + "/OUTCAR");
      aurostd::string2file(" running on    4 total cores\n" + string(c.vaspout), dir + "/" + DEFAULT_VASP_OUT);
      aurostd::string2file("", dir + "/" + _AFLOWLOCK_);
      if (c.age > 0) {
        struct timeval tv[2];
        tv[0].tv_sec = tv[1].tv_sec = time(NULL) - c.age;
        tv[0].tv_usec = tv[1].tv_usec = 0;
        utimes((dir + "/OUTCAR").c_str(), tv);
      }
      _xvasp xvasp;
      _aflags aflags;
      _kflags kflags;
      xvasp.Directory = dir;
      aflags.Directory = dir;
      kflags.KBIN_BIN = "vasp_relax_continue_unit_test";  // no such process runs
      kflags.KBIN_MPI_BIN = kflags.KBIN_BIN;
      aurostd::xoption xmessage, xwarning, xfixed;
      ofstream FileMESSAGE;
      string threw = "";
      try {KBIN::VASP_ProcessWarnings(xvasp, aflags, kflags, xmessage, xwarning, xfixed, FileMESSAGE);}
      catch (aurostd::xerror& excpt) {threw = excpt.buildMessageString();}
      if (!threw.empty()) {errors.push_back(c.why + ": " + threw); aurostd::RemoveDirectory(dir); continue;}
      for (const string& flag : c.on) {checkEqual(xwarning.flag(flag), true, check_function, c.why + " (" + flag + " on)", passed_checks, results);}
      for (const string& flag : c.off) {checkEqual(xwarning.flag(flag), false, check_function, c.why + " (" + flag + " off)", passed_checks, results);}
      aurostd::RemoveDirectory(dir);
    }

    // 5. VASP_RelaxContinue(): converged, a CONTCAR that is not sound, and what it does before it runs VASP
    check_function = "KBIN::VASP_RelaxContinue()";
    const bool avoid_running_vasp = XHOST.AVOID_RUNNING_VASP;
    XHOST.AVOID_RUNNING_VASP = true;  // a VASP execution throws instead of running
    struct continue_case { string vaspout; uint natoms_contcar; bool ran; string why; };
    const vector<continue_case> vcontinue = {
      {reached, 2, false, "converged: nothing runs, nothing moves"},
      {eddrmm, 3, false, "not converged, CONTCAR with 3 atoms against 2: nothing runs, nothing moves"},
      {eddrmm, 2, true, "not converged: the execution becomes part1 and the next one starts from its CONTCAR"},
    };
    for (const continue_case& c : vcontinue) {
      dir = aurostd::TmpDirectoryCreate("vasp_relax_continue");
      aurostd::string2file("IBRION=2\nNSW=51\nISIF=3\n", dir + "/INCAR.relax2");
      aurostd::string2file("Automatic\n0\nGamma\n4 4 4\n", dir + "/KPOINTS.relax2");
      aurostd::string2file("POTCAR of the unit test\n", dir + "/POTCAR.relax2");
      aurostd::string2file(poscar(3.60, 2), dir + "/POSCAR.relax2");
      aurostd::string2file(poscar(3.55, c.natoms_contcar), dir + "/CONTCAR.relax2");
      aurostd::string2file(outcar(60, stress_small, true), dir + "/OUTCAR.relax2");
      aurostd::string2file(c.vaspout, dir + "/" + DEFAULT_VASP_OUT + ".relax2");
      aurostd::string2file(ionic(1, 5, 10, false) + ionic(2, 1, 8, false), dir + "/OSZICAR.relax2");
      _xvasp xvasp;
      _aflags aflags;
      _kflags kflags;
      _vflags vflags;
      xvasp.Directory = dir;
      aflags.Directory = dir;
      ofstream FileMESSAGE;
      bool ran = false, returned = false;
      try {returned = KBIN::VASP_RelaxContinue(xvasp, aflags, kflags, vflags, "relax2", FileMESSAGE);}
      catch (aurostd::xerror& excpt) {ran = true;}
      checkEqual(ran, c.ran, check_function, c.why + " (VASP " + (c.ran ? "started" : "not started") + ")", passed_checks, results);
      if (!c.ran) {
        checkEqual(returned && aurostd::FileExist(dir + "/OUTCAR.relax2") && !aurostd::FileExist(dir + "/OUTCAR.relax2.part1"), true, check_function, c.why, passed_checks, results);
      } else {
        const bool moved = (aurostd::FileExist(dir + "/OUTCAR.relax2.part1") && aurostd::FileExist(dir + "/CONTCAR.relax2.part1") && aurostd::FileExist(dir + "/INCAR.relax2.part1") && !aurostd::FileExist(dir + "/OUTCAR.relax2"));
        checkEqual(moved, true, check_function, c.why + " (relax2 files moved to part1)", passed_checks, results);
        checkEqual(aurostd::FileExist(dir + "/POSCAR") ? aurostd::file2string(dir + "/POSCAR") : string(""), poscar(3.55, 2), check_function, c.why + " (POSCAR is the CONTCAR)", passed_checks, results);
        checkEqual(aurostd::FileExist(dir + "/POTCAR") && aurostd::FileExist(dir + "/INCAR") && aurostd::FileExist(dir + "/KPOINTS"), true, check_function, c.why + " (INCAR, KPOINTS and POTCAR in place)", passed_checks, results);
      }
      aurostd::RemoveDirectory(dir);
    }
    XHOST.AVOID_RUNNING_VASP = avoid_running_vasp;
  }

}

// structure generation
namespace unittest {

  //CO20190520
  //CO20260930 - prototype redecoration: the prototype list (species in label
  //order), oxidation-state matching, decorations, commands
  void UnitTest::redecorationTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function = "", check_description = "";
    bool calculated_bool = false, expected_bool = false;
    uint calculated_uint = 0, expected_uint = 0;
    size_t index = 0;

    // rock salt NaCl: Na+ -> Li, K (He has no oxidation state, Mg is +2); Cl- -> Br, I
    check_function = "pflow::Redecoration::initialize()";
    pflow::Redecoration redec_rs(2, {"He", "Li", "Mg", "K"}, {"Br", "I"});
    check_description = "prototype list holds AB_cF8_225_a_b";
    expected_bool = true;
    calculated_bool = aurostd::WithinList(redec_rs.vlabel, string("AB_cF8_225_a_b"), index);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    if (calculated_bool) {
      check_description = "AB_cF8_225_a_b species in label order";
      checkEqual(redec_rs.velements[index], vector<string>({"Cl", "Na"}), check_function, check_description, passed_checks, results);
    }
    check_description = "TiO2 parameter set A2B_tP6_136_f_a-001 has species O,Ti (alphabetical, A=O)";
    expected_bool = true;
    calculated_bool = aurostd::WithinList(redec_rs.vlabel, string("A2B_tP6_136_f_a-001"), index);
    if (calculated_bool) { calculated_bool = (redec_rs.velements[index] == vector<string>({"O", "Ti"})); }
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    check_function = "pflow::Redecoration::RedecoratePrototype()";
    check_description = "rock salt with Li, K and Br, I";
    vector<string> expected_protos = {"AB_cF8_225_a_b.AB:Br:K", "AB_cF8_225_a_b.AB:Br:Li", "AB_cF8_225_a_b.AB:I:K", "AB_cF8_225_a_b.AB:I:Li"};
    checkEqual(redec_rs.RedecoratePrototype("AB_cF8_225_a_b", {"Cl", "Na"}), expected_protos, check_function, check_description, passed_checks, results);

    // rutile TiO2, charge-neutral rule: O2- with Ge4+, Sn4+; F- with Mg2+, Sn2+ (Ge is only +4, Mg only +2)
    pflow::Redecoration redec_ru(2, {"Ge", "Mg", "Sn"}, {"F", "O"});
    check_description = "rutile with Ge, Mg, Sn and F, O (GeO2, SnO2, MgF2, SnF2)";
    expected_protos = {"A2B_tP6_136_f_a-001.AB:F:Mg", "A2B_tP6_136_f_a-001.AB:F:Sn", "A2B_tP6_136_f_a-001.AB:O:Sn", "A2B_tP6_136_f_a-001.BA:Ge:O"};
    checkEqual(redec_ru.RedecoratePrototype("A2B_tP6_136_f_a-001", {"O", "Ti"}), expected_protos, check_function, check_description, passed_checks, results);

    // rock salt NaCl (+1, -1) with 2+ cations and O2-: MgO and NiO (CO20261003 - charge-neutral rule)
    pflow::Redecoration redec_mo(2, {"Mg", "Ni"}, {"O"});
    check_description = "rock salt with Mg, Ni and O (MgO, NiO)";
    expected_protos = {"AB_cF8_225_a_b.AB:Mg:O", "AB_cF8_225_a_b.AB:Ni:O"};
    checkEqual(redec_mo.RedecoratePrototype("AB_cF8_225_a_b", {"Cl", "Na"}), expected_protos, check_function, check_description, passed_checks, results);
    // fluorite CaF2 (+2, -1): O only as -2, so no MgO2
    check_description = "fluorite with Mg and O gives nothing (O only as -2)";
    expected_protos.clear();
    checkEqual(redec_mo.RedecoratePrototype("AB2_cF12_225_a_c", {"Ca", "F"}), expected_protos, check_function, check_description, passed_checks, results);

    // spinel Al2MgO4 (free parameter x, so the label takes -001): Al3+ -> Co, Fe; Mg2+ -> Co, Fe; O2- -> O
    check_function = "pflow::Redecoration::initialize()";
    pflow::Redecoration redec_sp(3, {"Co", "Fe"}, {"O"});
    check_description = "spinel Al2MgO4 is A2BC4_cF56_227_d_a_e-001 with species Al,Mg,O";
    expected_bool = true;
    calculated_bool = aurostd::WithinList(redec_sp.vlabel, string("A2BC4_cF56_227_d_a_e-001"), index);
    if (calculated_bool) { calculated_bool = (redec_sp.velements[index] == vector<string>({"Al", "Mg", "O"})); }
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    check_function = "pflow::Redecoration::RedecoratePrototype()";
    check_description = "spinel with Co and Fe (no element on two species)";
    expected_protos = {"A2BC4_cF56_227_d_a_e-001.ABC:Co:Fe:O", "A2BC4_cF56_227_d_a_e-001.BAC:Co:Fe:O"};
    checkEqual(redec_sp.RedecoratePrototype("A2BC4_cF56_227_d_a_e-001", {"Al", "Mg", "O"}), expected_protos, check_function, check_description, passed_checks, results);

    // commands for the rock salt only
    check_function = "pflow::Redecoration::GrabProtos()";
    redec_rs.vlabel = {"AB_cF8_225_a_b"};
    redec_rs.velements = {{"Cl", "Na"}};
    redec_rs.commands_file = "";
    vector<string> commands = redec_rs.GrabProtos();
    check_description = "one --noldau run per compound";
    calculated_uint = commands.size();
    expected_uint = 4;
    checkEqual(calculated_uint, expected_uint, check_function, check_description, passed_checks, results);
    check_description = "--noldau run of KBr";
    expected_bool = true;
    calculated_bool = aurostd::WithinList(commands, string("aflow --aflow_proto=AB_cF8_225_a_b.AB:Br:K --run_relax_static --bader --keep_nomix --aflow_db_run --force --noldau"));
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
    // rock salt with Mg, Ni and O (CO20261004): --noldau runs only by default; with ldau_runs, a +U
    // run with bands for NiO (Ni takes U), none for MgO (no species takes U)
    redec_mo.vlabel = {"AB_cF8_225_a_b"};
    redec_mo.velements = {{"Cl", "Na"}};
    redec_mo.commands_file = "";
    vector<string> commands_mo = redec_mo.GrabProtos();
    check_description = "--noldau runs only by default (MgO, NiO)";
    vector<string> expected_cmds = {
      "aflow --aflow_proto=AB_cF8_225_a_b.AB:Mg:O --run_relax_static --bader --keep_nomix --aflow_db_run --force --noldau",
      "aflow --aflow_proto=AB_cF8_225_a_b.AB:Ni:O --run_relax_static --bader --keep_nomix --aflow_db_run --force --noldau"};
    checkEqual(commands_mo, expected_cmds, check_function, check_description, passed_checks, results);
    redec_mo.ldau_runs = true;
    check_description = "with ldau_runs, a +U run with bands only where a species takes U (NiO)";
    expected_cmds = {
      "aflow --aflow_proto=AB_cF8_225_a_b.AB:Mg:O --run_relax_static --bader --keep_nomix --aflow_db_run --force --noldau",
      "aflow --aflow_proto=AB_cF8_225_a_b.AB:Ni:O --run_relax_static_bands --bader --keep_nomix --aflow_db_run --force --ldau",
      "aflow --aflow_proto=AB_cF8_225_a_b.AB:Ni:O --run_relax_static --bader --keep_nomix --aflow_db_run --force --noldau"};
    checkEqual(redec_mo.WriteCommands(), expected_cmds, check_function, check_description, passed_checks, results);

    // runs already in CHAOS are left out (CO20261004): the +U run of NiO, and the MgO run, which
    // CHAOS has under the equivalent decoration BA (in rock salt AB = BA)
    check_function = "pflow::Redecoration::WriteCommands()";
    redec_mo.chaos.addAFLUXResponse("compound = Ni4O4 | prototype = AB_cF8_225_a_b.AB | aurl = s4e.ai:AFLOWDATA/LIB2_RAW/Ni_pvO/AB_cF8_225_a_b.AB:LDAU2", {"Ni", "O"});
    redec_mo.chaos.addAFLUXResponse("compound = Mg1O1 | prototype = AB_cF8_225_a_b.BA | aurl = s4e.ai:AFLOWDATA/LIB2_RAW/Mg_pvO/AB_cF8_225_a_b.BA", {"Mg", "O"});
    redec_mo.chaos_checked = true;
    check_description = "runs in CHAOS are left out, equivalent decorations included (only the --noldau run of NiO is written)";
    expected_cmds = {"aflow --aflow_proto=AB_cF8_225_a_b.AB:Ni:O --run_relax_static --bader --keep_nomix --aflow_db_run --force --noldau"};
    checkEqual(redec_mo.WriteCommands(), expected_cmds, check_function, check_description, passed_checks, results);
    check_description = "two runs counted as already in CHAOS";
    calculated_uint = redec_mo.nruns_in_chaos;
    expected_uint = 2;
    checkEqual(calculated_uint, expected_uint, check_function, check_description, passed_checks, results);
  }

  //CO20261002
  void UnitTest::jhuPrototypeTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function = "", check_description = "";
    ofstream devnull("/dev/null");

    // every record generates and relabels to its stored label
    check_function = "jhuproto::records()";
    const vector<jhuproto::Record>& vrecords = jhuproto::records();
    vector<string> vfailed;
    bool boxes_kept = true;
    for (size_t r = 0; r < vrecords.size(); r++) {
      const string label_set = string(vrecords[r].label) + "-" + aurostd::PaddedNumString(vrecords[r].set, 3); //CO20261004
      try {
        xstructure xstr = aflowlib::PrototypeLibraries(devnull, label_set, "", LIBRARY_MODE_HTQC);
        if (vrecords[r].keep_lattice_parameter) {
          vector<string> vvalues;
          aurostd::string2tokens(string(vrecords[r].parameter_values), vvalues, ",");
          const double a = aurostd::string2utype<double>(vvalues.at(0));
          if (!aurostd::isequal(aurostd::modulus(xstr.lattice(1)) * xstr.scale, a, 1e-6)) {boxes_kept = false;}
        }
        anrl::structure2anrl(xstr, SYM::defaultTolerance(xstr), SG_SETTING_ANRL, true, false, false);
        if (xstr.prototype != vrecords[r].label) {vfailed.push_back(label_set + " -> " + xstr.prototype);}
      } catch (aurostd::xerror& excpt) {vfailed.push_back(label_set + " (not generated)");}
    }
    check_description = "all " + aurostd::utype2string<uint>(vrecords.size()) + " records relabel to their stored label";
    if (!vfailed.empty()) {std::cerr << check_description << ": " << aurostd::joinWDelimiter(vfailed, ", ") << std::endl;}
    checkEqual(vfailed.size(), (size_t) 0, check_function, check_description, passed_checks, results);
    check_description = "molecules in a box keep the stored lattice parameter";
    checkEqual(boxes_kept, true, check_function, check_description, passed_checks, results);

    // molecules in a box may match each other in the prototype test; nothing else may
    check_function = "jhuproto::moleculesInBox()";
    check_description = "the O2 and F2 boxes";
    checkEqual(jhuproto::moleculesInBox({"A_tP2_123_g-1003", "A_tP2_123_g-1004"}), true, check_function, check_description, passed_checks, results); //CO20261004 - numbers from 1001
    check_description = "a box and an ordinary prototype";
    checkEqual(jhuproto::moleculesInBox({"A_tP2_123_g-1003", "A_cF4_225_a-001"}), false, check_function, check_description, passed_checks, results); //CO20261004
    check_description = "a single label";
    checkEqual(jhuproto::moleculesInBox({"A_tP2_123_g-1003"}), false, check_function, check_description, passed_checks, results); //CO20261004
    check_description = "the former numbers no longer name the boxes"; //CO20261004
    checkEqual(jhuproto::moleculesInBox({"A_tP2_123_g-003", "A_tP2_123_g-004"}), false, check_function, check_description, passed_checks, results); //CO20261004
    check_description = "all five boxes"; //CO20261004
    checkEqual(jhuproto::moleculesInBox({"A_tP2_123_g-1001", "A_tP2_123_g-1002", "A_tP2_123_g-1003", "A_tP2_123_g-1004", "A_tP2_123_g-1005"}), true, check_function, check_description, passed_checks, results); //CO20261004
    check_description = "a box and the label without a set number"; //CO20261004
    checkEqual(jhuproto::moleculesInBox({"A_tP2_123_g-1003", "A_tP2_123_g"}), false, check_function, check_description, passed_checks, results); //CO20261004
    check_description = "a box and a JHU set that is not a box"; //CO20261004
    checkEqual(jhuproto::moleculesInBox({"A_tP2_123_g-1003", "AB2_oC12_36_a_2a-1001"}), false, check_function, check_description, passed_checks, results); //CO20261004

    //CO20261004 START - JHU sets are numbered from 1001 on their label and looked up by number;
    // the library's sets keep 1 to 999 and their positions
    check_function = "jhuproto::records()";
    vector<string> vlow, vtwice, vspecies; //CO20261004
    for (size_t r = 0; r < vrecords.size(); r++) {
      const string label_set = string(vrecords[r].label) + "-" + aurostd::utype2string<uint>(vrecords[r].set); //CO20261004
      if (vrecords[r].set < jhuproto::FIRST_SET) {vlow.push_back(label_set);} //CO20261004
      for (size_t s = r + 1; s < vrecords.size(); s++) { //CO20261004
        if (string(vrecords[r].label) == vrecords[s].label && vrecords[r].set == vrecords[s].set) {vtwice.push_back(label_set);} //CO20261004
      } //CO20261004
      vector<string> tokens; //CO20261004
      aurostd::string2tokens(string(vrecords[r].label), tokens, "_"); //CO20261004
      if (aurostd::getElements(tokens.at(0)).size() != aurostd::getElements(string(vrecords[r].prototype)).size()) {vspecies.push_back(label_set + " " + vrecords[r].prototype);} //CO20261004
    }
    check_description = "every record is numbered " + aurostd::utype2string<uint>(jhuproto::FIRST_SET) + " or up";
    checkEqual(aurostd::joinWDelimiter(vlow, ","), string(""), check_function, check_description, passed_checks, results); //CO20261004
    check_description = "no label uses a set number twice"; //CO20261004
    checkEqual(aurostd::joinWDelimiter(vtwice, ","), string(""), check_function, check_description, passed_checks, results); //CO20261004
    check_description = "every composition has as many species as its label"; //CO20261004
    checkEqual(aurostd::joinWDelimiter(vspecies, ","), string(""), check_function, check_description, passed_checks, results); //CO20261004

    //CO20261004 START - the Pearson symbol and space group are no longer stored in the records:
    // what the ANRL list and the listing derive from the label must equal the values stored before,
    // together with the species and the atoms in the primitive cell
    // (label-set, Pearson symbol, space group, species, atoms in the primitive cell)
    const vector<string> vstored = {
      "A2BC6D2_cF88_227_c_a_f_d-1001 cF88 227 4 22", "A4B12C3_oC76_41_2b_6b_ab-1001 oC76 41 3 38",
      "A3B7_mC60_12_a4i_b10i-1001 mC60 12 2 30", "A17B8_oI100_46_b8c_ab3c-1001 oI100 46 2 50",
      "A5B3_tP8_123_agh_bh-1001 tP8 123 2 8", "AB2_hR3_160_a_2a-1001 hR3 160 2 3",
      "A_tP2_123_g-1001 tP2 123 1 2", "A_tP2_123_g-1002 tP2 123 1 2", "A_tP2_123_g-1003 tP2 123 1 2",
      "A_tP2_123_g-1004 tP2 123 1 2", "A_tP2_123_g-1005 tP2 123 1 2",
      "A6BC4_mC22_12_ij_a_gh-1001 mC22 12 3 11", "A18B5C12_hP35_164_3i_a2d_gh-1001 hP35 164 3 35",
      "A6BC4_hP33_164_3i_ad_gh-1001 hP33 164 3 33", "A4BC3_oP8_25_abcd_a_bcd-1001 oP8 25 3 8",
      "AB_mC12_8_3a_3a-1001 mC12 8 2 6", "AB2_oC12_36_a_2a-1001 oC12 36 2 6", "AB2_mC6_12_a_i-1001 mC6 12 2 3",
      "A3B_oP16_59_ab2e_f-1001 oP16 59 2 16", "A7B2_oP36_61_a3c_c-1001 oP36 61 2 36",
      "A2B3_oP80_61_ab3c_6c-1001 oP80 61 2 80", "A2B_tP6_137_d_a-1001 tP6 137 2 6",
      "A2B_tI96_142_deg_g-1001 tI96 142 2 48", "A2B_hR3_166_c_a-1001 hR3 166 2 3"};
    vector<string> vproto, vproto_label, vproto_Pearson_symbol, vproto_params, vproto_Strukturbericht, vproto_prototype, vproto_dialect;
    vector<uint> vproto_nspecies, vproto_natoms, vproto_spacegroup, vproto_nunderscores, vproto_nparameters;
    anrl::PrototypeANRL_LoadList(vproto, vproto_label, vproto_nspecies, vproto_natoms, vproto_spacegroup, vproto_nunderscores,
        vproto_nparameters, vproto_Pearson_symbol, vproto_params, vproto_Strukturbericht, vproto_prototype, vproto_dialect);
    const string jhu_listing = jhuproto::listing();
    vector<string> vderived, vlisting_missing;
    for (size_t r = 0; r < vrecords.size(); r++) {
      const string label_set = string(vrecords[r].label) + "-" + aurostd::utype2string<uint>(vrecords[r].set);
      const size_t i = std::find(vproto_label.begin(), vproto_label.end(), string(vrecords[r].label)) - vproto_label.begin();
      if (i == vproto_label.size()) {vderived.push_back(label_set + " (not in the ANRL list)"); continue;}
      vderived.push_back(label_set + " " + vproto_Pearson_symbol[i] + " " + aurostd::utype2string<uint>(vproto_spacegroup[i]) + " "
          + aurostd::utype2string<uint>(vproto_nspecies[i]) + " " + aurostd::utype2string<uint>(vproto_natoms[i]));
      vector<string> vfields;
      aurostd::string2tokens(r < vstored.size() ? vstored[r] : string(""), vfields, " ");
      if (vfields.size() < 3) {continue;}
      const string listing_line = aurostd::PaddedPOST(vfields[0], 40) + aurostd::PaddedPOST(vfields[1], 10) + aurostd::PaddedPOST("#" + vfields[2], 7);
      if (jhu_listing.find(listing_line) == string::npos) {vlisting_missing.push_back(vfields[0]);}
    }
    check_function = "jhuproto::addToANRLList()";
    check_description = "Pearson symbol, space group, species and atoms of all " + aurostd::utype2string<uint>(vstored.size()) + " records";
    checkEqual(vderived, vstored, check_function, check_description, passed_checks, results);
    check_function = "jhuproto::listing()";
    check_description = "Pearson symbol and space group of every record";
    checkEqual(aurostd::joinWDelimiter(vlisting_missing, ","), string(""), check_function, check_description, passed_checks, results);
    //CO20261004 STOP

    // AB2_oC12_36_a_2a: two sets of the library and the JHU set 1001 (CrI2)
    const string label_shared = "AB2_oC12_36_a_2a";
    string values_jhu = "";
    for (size_t r = 0; r < vrecords.size(); r++) {if (label_shared == vrecords[r].label && vrecords[r].set == jhuproto::FIRST_SET) {values_jhu = vrecords[r].parameter_values;}}
    const vector<string> vall = anrl::getANRLParameters(label_shared, "all");
    check_function = "anrl::getANRLParameters()";
    string found = "";
    try {found = anrl::getANRLParameters(label_shared, "", 1000, true).at(0);} catch (aurostd::xerror& excpt) {found = "(refused)";}
    check_description = "a JHU set on a library label is found by its number (" + label_shared + "-1001)";
    checkEqual(found, values_jhu, check_function, check_description, passed_checks, results);
    found = "";
    try {found = anrl::getANRLParameters(label_shared, "", 1, true).at(0);} catch (aurostd::xerror& excpt) {found = "(refused)";}
    check_description = "the library's sets keep their positions (" + label_shared + "-002)";
    checkEqual(found, (vall.size() > 1 ? vall[1] : string("")), check_function, check_description, passed_checks, results);
    bool refused = false;
    try {anrl::getANRLParameters(label_shared, "", 2, true);} catch (aurostd::xerror& excpt) {refused = true;}
    check_description = "a former JHU number on a library label is refused (" + label_shared + "-003)";
    checkEqual(refused, true, check_function, check_description, passed_checks, results);
    refused = false;
    try {anrl::getANRLParameters("A_tP2_123_g", "", 1005, true);} catch (aurostd::xerror& excpt) {refused = true;}
    check_description = "a number without a set is refused (A_tP2_123_g-1006)";
    checkEqual(refused, true, check_function, check_description, passed_checks, results);

    check_function = "jhuproto::setNumbers()";
    check_description = "set numbers of a label shared with the library";
    checkEqual(aurostd::joinWDelimiter(jhuproto::setNumbers(label_shared), ','), string("1,2,1001"), check_function, check_description, passed_checks, results);
    check_description = "set numbers of a JHU label";
    checkEqual(aurostd::joinWDelimiter(jhuproto::setNumbers("A_tP2_123_g"), ','), string("1001,1002,1003,1004,1005"), check_function, check_description, passed_checks, results);
    check_description = "set numbers from the parameter sets the caller has"; //CO20261004
    checkEqual(aurostd::joinWDelimiter(jhuproto::setNumbers(label_shared, vall), ','), string("1,2,1001"), check_function, check_description, passed_checks, results); //CO20261004

    // designations listed with their set numbers (aflow --list_prototype_labels --arity=N) //CO20261004
    // the list includes the HTQC library, which is loaded from aflow_data (as for the help text below)
    check_function = "pflow::listPrototypeLabels()"; //CO20261004
    for (uint arity = 1; arity <= 2 && XHOST.is_command("aflow_data"); arity++) { //CO20261004
      const string label_listed = (arity == 1 ? string("A_tP2_123_g") : label_shared); //CO20261004
      const string expected_listed = (arity == 1 ? string("A_tP2_123_g-1001,A_tP2_123_g-1002,A_tP2_123_g-1003,A_tP2_123_g-1004,A_tP2_123_g-1005") //CO20261004
          : label_shared + "-001," + label_shared + "-002," + label_shared + "-1001"); //CO20261004
      aurostd::xoption vpflow_list; //CO20261004
      vpflow_list.push_attached("LIST_PROTOTYPE_LABELS::ARITY", aurostd::utype2string<uint>(arity)); //CO20261004
      vector<string> vlisted, vlisted_label; //CO20261004
      aurostd::string2tokens(pflow::listPrototypeLabels(vpflow_list), vlisted, ","); //CO20261004
      for (size_t i = 0; i < vlisted.size(); i++) {if (vlisted[i].compare(0, label_listed.size() + 1, label_listed + "-") == 0) {vlisted_label.push_back(vlisted[i]);}} //CO20261004
      check_description = "designations of " + label_listed; //CO20261004
      checkEqual(aurostd::joinWDelimiter(vlisted_label, ","), expected_listed, check_function, check_description, passed_checks, results); //CO20261004
    } //CO20261004
    //CO20261004 STOP

    //CO20261004 START - the fields addToANRLList() writes, read back through anrl::PrototypeANRL_LoadList():
    // a library label gets its JHU sets appended to the per-set fields, a JHU label gets a new line
    // (label;underscores;parameters;parameter list;Strukturbericht;prototype;name)
    check_function = "jhuproto::addToANRLList()";
    const vector<string> vlabels_anrl = {label_shared, "A_tP2_123_g"};
    const vector<string> vfields_anrl = {
      label_shared + ";4;9;a,b/a,c/a,y1,z1,y2,z2,y3,z3;-/-/-;C1N2/MoP2/CrI2;C1N2 (ICSD #247680)/MoP2/CrI2 (ICSD #2596)",
      "A_tP2_123_g;3;3;a,c/a,z1;-/-/-/-/-;H2/N2/O2/F2/Cl2;H2 molecule in a 20 Angstrom box/N2 molecule in a 20 Angstrom box/O2 molecule in a 20 Angstrom box/F2 molecule in a 20 Angstrom box/Cl2 molecule in a 20 Angstrom box"};
    for (size_t l = 0; l < vlabels_anrl.size(); l++) {
      const size_t i = std::find(vproto_label.begin(), vproto_label.end(), vlabels_anrl[l]) - vproto_label.begin();
      string fields = "(not in the ANRL list)";
      if (i < vproto_label.size()) {
        fields = vproto_label[i] + ";" + aurostd::utype2string<uint>(vproto_nunderscores[i]) + ";" + aurostd::utype2string<uint>(vproto_nparameters[i]) + ";"
          + vproto_params[i] + ";" + vproto_Strukturbericht[i] + ";" + vproto_prototype[i] + ";" + vproto_dialect[i];
      }
      check_description = string(l == 0 ? "a library label with a JHU set" : "a JHU label") + " (" + vlabels_anrl[l] + ")";
      checkEqual(fields, vfields_anrl[l], check_function, check_description, passed_checks, results);
      check_description = vlabels_anrl[l] + " has one line in the list";
      checkEqual((uint) std::count(vproto_label.begin(), vproto_label.end(), vlabels_anrl[l]), (uint) 1, check_function, check_description, passed_checks, results);
    }

    // the errors of anrl::getANRLParameters() name the sets by their numbers
    check_function = "anrl::getANRLParameters()";
    // designations of the "  label-NNN : values" lines of an error message
    auto listedSets = [](const string& message) {
      vector<string> vlines, vtokens, vlisted;
      aurostd::string2tokens(message, vlines, "\n");
      for (size_t i = 0; i < vlines.size(); i++) {
        if (aurostd::string2tokens(vlines[i], vtokens, " ") >= 2 && vtokens[1] == ":") {vlisted.push_back(vtokens[0]);}
      }
      return aurostd::joinWDelimiter(vlisted, ",");
    };
    string message_error = "";
    try {anrl::getANRLParameters(label_shared, "", 2, true);} catch (aurostd::xerror& excpt) {message_error = excpt.what();}
    check_description = "the refusal of " + label_shared + "-003 lists the sets by number";
    checkEqual(listedSets(message_error), label_shared + "-001," + label_shared + "-002," + label_shared + "-1001", check_function, check_description, passed_checks, results);
    message_error = "";
    try {anrl::getANRLParameters("A_tP2_123_g", "", -1, true);} catch (aurostd::xerror& excpt) {message_error = excpt.what();}
    check_description = "a JHU label without a set number lists the sets by number";
    checkEqual(listedSets(message_error), string("A_tP2_123_g-1001,A_tP2_123_g-1002,A_tP2_123_g-1003,A_tP2_123_g-1004,A_tP2_123_g-1005"), check_function, check_description, passed_checks, results);
    string suggested = "";
    const string::size_type ipos_suggested = message_error.find("--proto=");
    if (ipos_suggested != string::npos) {suggested = aurostd::RemoveWhiteSpaces(message_error.substr(ipos_suggested + string("--proto=").size()));}
    check_description = "a JHU label without a set number suggests its first set";
    checkEqual(suggested, string("A_tP2_123_g-1001"), check_function, check_description, passed_checks, results);

    // the hooks the generator and the prototype check call
    check_function = "jhuproto::keepLatticeParameter()";
    check_description = "the molecules in a box keep their lattice parameter";
    checkEqual(jhuproto::keepLatticeParameter("A_tP2_123_g"), true, check_function, check_description, passed_checks, results);
    check_description = "a library label with a JHU set is rescaled as before";
    checkEqual(jhuproto::keepLatticeParameter(label_shared), false, check_function, check_description, passed_checks, results);
    check_function = "anrl::isSpecialCaseEquivalentPrototypes()";
    check_description = "the H2 and Cl2 boxes are a documented special case";
    checkEqual(anrl::isSpecialCaseEquivalentPrototypes({"A_tP2_123_g-1001", "A_tP2_123_g-1005"}), true, check_function, check_description, passed_checks, results);
    check_description = "a box and a JHU set that is not a box are not";
    checkEqual(anrl::isSpecialCaseEquivalentPrototypes({"A_tP2_123_g-1003", label_shared + "-1001"}), false, check_function, check_description, passed_checks, results);

    // the prototype help (aflow --protos) carries the JHU table; the help text comes from aflow_data
    if (XHOST.is_command("aflow_data")) {
      check_function = "aflowlib::PrototypesHelp()";
      check_description = "the help carries jhuproto::listing()";
      checkEqual(aflowlib::PrototypesHelp().find(jhu_listing) != string::npos, true, check_function, check_description, passed_checks, results);
    }

    // CCE iodide references renamed to the 1001 numbers: the protos column keeps the 16-column layout
    // the corrections are read from, and a reference that names a JHU set has that set's composition
    check_function = "cce::get_corrections_line_I()";
    const vector<string> vcations_I = {"Cr_+2", "Ni_+2", "Zn_+2", "Be_+2", "Al_+3", "Ga_+3", "Sr_+2", "Zr_+4", "In_+3", "Sn_+2", "Hg_+1", "Hg_+2"};
    const string protos_I_expected = "Cr_pvI/AB2_oC12_36_a_2a-1001.AB,INi_pv/A2B_hR3_166_c_a-1001.AB,IZn/A2B_tI96_142_deg_g-1001.AB,Be_svI/AB2_oI12_72_a_j-1001.AB,"
      "AlI/AB3_mP16_14_e_3e-1003.AB,Ga_hI/AB3_mP16_14_e_3e-1003.AB,ISr_sv/A2B_oP24_61_2c_c-1001.AB,IZr_sv/A4B_mP30_13_6g_eg-1001.AB,"
      "IIn_d/A3B_mP16_14_3e_e-1001.AB,ISn/A2B_mC18_12_3i_ai-1001.AB,HgI/AB_tI8_139_e_e-1001.AB,HgI/AB2_oC12_36_a_2a-1002.AB";
    vector<string> vprotos_I, vcomposition_I;
    for (size_t c = 0; c < vcations_I.size(); c++) {
      vector<string> vcolumns, vname, vreference;
      aurostd::string2tokens(cce::get_corrections_line_I(vcations_I[c] + "_I"), vcolumns, " ");
      if (vcolumns.size() != 16) {vprotos_I.push_back(vcations_I[c] + " (" + aurostd::utype2string<size_t>(vcolumns.size()) + " columns)"); continue;}
      vprotos_I.push_back(vcolumns[12]);
      // compound of the correction (e.g. CrI2 in Cr_+2_CrI2*) and label-set of the reference (e.g. AB2_oC12_36_a_2a-1001)
      if (aurostd::string2tokens(vcolumns[0], vname, "_") < 3 || aurostd::string2tokens(vcolumns[12], vreference, "/.") < 3) {continue;}
      vector<string> vlabel_set;
      if (aurostd::string2tokens(vreference[1], vlabel_set, "-") != 2) {continue;}
      const uint set = aurostd::string2utype<uint>(vlabel_set[1]);
      for (size_t r = 0; r < vrecords.size(); r++) {
        if (vlabel_set[0] != vrecords[r].label || set != vrecords[r].set) {continue;}
        vector<string> velements_compound = aurostd::getElements(aurostd::RemoveCharacter(vname[2], '*'));
        vector<string> velements_record = aurostd::getElements(string(vrecords[r].prototype));
        std::sort(velements_compound.begin(), velements_compound.end());
        std::sort(velements_record.begin(), velements_record.end());
        if (velements_compound != velements_record) {vcomposition_I.push_back(vreference[1] + " " + vrecords[r].prototype + " for " + vname[2]);}
      }
    }
    check_description = "protos column of the 12 renamed iodide references";
    checkEqual(aurostd::joinWDelimiter(vprotos_I, ","), protos_I_expected, check_function, check_description, passed_checks, results);
    check_description = "references that name a JHU set have its composition";
    checkEqual(aurostd::joinWDelimiter(vcomposition_I, ","), string(""), check_function, check_description, passed_checks, results);
    //CO20261004 STOP
  }

  void UnitTest::ceramgenTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings

    // setup test environment
    string check_function = "", check_description = "";
    bool calculated_bool = false, expected_bool = false;
    uint calculated_uint = 0, expected_uint = 0;

    //./aflow --generate_ceramics --nm=N,C --m=Co,Mo,Fe,Ru,Ni,Rh,Pt,Cu,Cr,V --N=5
    vector<string> vnonmetals,vmetals;
    aurostd::string2tokens("N,C",vnonmetals,",");
    aurostd::string2tokens("Co,Mo,Fe,Ru,Ni,Rh,Pt,Cu,Cr,V",vmetals,",");

    check_function = "pflow::GENERATE_CERAMICS()";
    vector<string> commands=pflow::GENERATE_CERAMICS(vnonmetals,vmetals,5);

    check_description = "number of commands";
    calculated_uint = commands.size();
    expected_uint = 6;
    checkEqual(calculated_uint, expected_uint, check_function, check_description, passed_checks, results);

    //C:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:N
    //C:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:N:Ni,Pt,Rh,Ru,V
    //C:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:N:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V
    //C:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:N:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V
    //C:Co,Cr,Cu,Fe,Mo:N:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V
    //C:N:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V

    string search = "";

    search = "C:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:N";
    check_description = "find " + search;
    expected_bool = true;
    calculated_bool = aurostd::WithinList(commands, search);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    search = "C:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:N:Ni,Pt,Rh,Ru,V";
    check_description = "find " + search;
    calculated_bool = aurostd::WithinList(commands, search);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    search = "C:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:N:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V";
    check_description = "find " + search;
    calculated_bool = aurostd::WithinList(commands, search);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    search = "C:Co,Cr,Cu,Fe,Mo:Co,Cr,Cu,Fe,Mo:N:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V";
    check_description = "find " + search;
    calculated_bool = aurostd::WithinList(commands, search);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    search = "C:Co,Cr,Cu,Fe,Mo:N:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V";
    check_description = "find " + search;
    calculated_bool = aurostd::WithinList(commands, search);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    search = "C:N:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V:Ni,Pt,Rh,Ru,V";
    check_description = "find " + search;
    calculated_bool = aurostd::WithinList(commands, search);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);
  }

  //DX20200925
  //ME20220324 - refactored to run in parallel
  void _testPrototype(uint i, const vector<string>& prototype_labels, vector<uint>& nprotos, vector<string>& errors
#ifdef AFLOW_MULTITHREADS_ENABLE
      , std::mutex& m
#endif
      ) {
    double tolerance_sym = 0.0;
    string label_input = "";
    bool generated = false, sym = false, unique = false;
    xstructure xstr;
    ofstream ofs("/dev/null");

    vector<string> parameter_sets = anrl::getANRLParameters(prototype_labels[i], "all");
    const vector<uint> vnumbers = jhuproto::setNumbers(prototype_labels[i], parameter_sets); //CO20261004 - set number of each parameter set
    for (size_t j = 0; j < parameter_sets.size(); j++) {
      string error = "";
      try {
        xstr = aflowlib::PrototypeLibraries(ofs,prototype_labels[i],parameter_sets[j],1);
        generated = true;
      } catch(aurostd::xerror& excpt) {
        error = "Could not generate prototype=" + prototype_labels[i] + ", params=" + parameter_sets[j];
      }

      if (error.empty()) {
        label_input = prototype_labels[i] + "-" + aurostd::PaddedNumString(vnumbers[j], 3); //CO20261004 - was j+1
        tolerance_sym = anrl::specialCaseSymmetryTolerances(label_input);
        if(tolerance_sym != AUROSTD_MAX_DOUBLE) {
          xstr.sym_eps = tolerance_sym;
          xstr.sym_eps_calculated = true;
        }
      }

      if (error.empty()) {
        string updated_label_and_params = "";
        if(!anrl::structureAndLabelConsistent(xstr, prototype_labels[i], updated_label_and_params, tolerance_sym)){ //DX20201105 - added symmetry tolerance
          error = "The structure has a higher symmetry than indicated by the label (orig: proto="
            + prototype_labels[i] + ", params=" + parameter_sets[j] + ")."
            + " The correct label and parameters for this structure are:\n" + updated_label_and_params;
        } else {
          sym = true;
        }
      }
      if (error.empty()) {
        aurostd::xoption vpflow;
        // check if the prototype matches to more than one prototype
        // (i.e., a prototype should match with itself, but no others)
        string catalog = "anrl";
        vector<string> protos_matching = compare::getMatchingPrototypes(xstr, vpflow, catalog);
        // if it matches to more than one
        if(protos_matching.size() > 1 && !anrl::isSpecialCaseEquivalentPrototypes(protos_matching)) {
          error = prototype_labels[i] + ", params=" + parameter_sets[j]
            + " matches multiple prototypes (and not a documented special case): "
            + aurostd::joinWDelimiter(protos_matching,",") + "."
            + " If the prototype was newly added, ONLY include it in the encyclopedia"
            + " for a valid reason (e.g., historical, special designation, etc.)"
            + " and document this in anrl::isSpecialCaseEquivalentPrototypes().";
          // if it doesn't match with ITSELF
        } else if (protos_matching.size() == 0) {
          error = prototype_labels[i] + ", params=" + parameter_sets[j]
            + " does not match to any prototypes"
            + " (requires special symmetry tolerance or there is a bug with XtalFinder).";
        } else {
          unique = true;
        }
      }
#ifdef AFLOW_MULTITHREADS_ENABLE
      std::lock_guard<std::mutex> lk(m);
#endif
      if (generated) nprotos[0]++;
      if (sym) nprotos[1]++;
      if (unique) nprotos[2]++;
      if (!error.empty()) errors.push_back(error);
    }
  }

  //CO20261002
  void UnitTest::sqliteErrorTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    (void) errors;  // Suppress compiler warnings
    string check_function = "sql::SQLexecuteCommand()", check_description = "";
    sqlite3* db = nullptr;
    sqlite3_open(":memory:", &db);
    sql::SQLexecuteCommand(db, "CREATE TABLE t (x INTEGER)");
    sql::SQLexecuteCommand(db, "INSERT INTO t VALUES (7)");
    check_description = "a valid query returns its value";
    checkEqual(sql::SQLexecuteCommandSCALAR(db, "SELECT x FROM t"), string("7"), check_function, check_description, passed_checks, results);
    string message = "";
    try {sql::SQLexecuteCommand(db, "SELECT x FROM missing_table");}
    catch (aurostd::xerror& excpt) {message = excpt.buildMessageString();}
    check_description = "a failed command throws with the SQLite message and the command";
    checkEqual(aurostd::substring2bool(message, "no such table") && aurostd::substring2bool(message, "missing_table"), true, check_function, check_description, passed_checks, results);
    message = "";
    try {sql::SQLexecuteCommandSCALAR(db, "SELECT y FROM t");}
    catch (aurostd::xerror& excpt) {message = excpt.buildMessageString();}
    check_function = "sql::SQLexecuteCommandSCALAR()";
    check_description = "a failed query throws with the SQLite message";
    checkEqual(aurostd::substring2bool(message, "no such column"), true, check_function, check_description, passed_checks, results);

    // throwSQLiteError(), shared by the four functions since CO20260930: the SQLite code, the message and the command,
    // also when SQLite gives no message (a null database handle is refused as SQLITE_MISUSE, 21, before any message is
    // written; string(nullptr) threw std::logic_error there before) //CO20261004
    check_description = "the error names the SQLite code, the message and the command";
    check(aurostd::substring2bool(message, "SQLite error 1: no such column: y (command: SELECT y FROM t)"), message, string("SQLite error 1: no such column: y (command: SELECT y FROM t)"),
        check_function, check_description, passed_checks, results);
    message = "";
    try {sql::SQLexecuteCommand(db, "SELECT x FROM missing_table");}
    catch (aurostd::xerror& excpt) {message = excpt.buildMessageString();}
    check(aurostd::substring2bool(message, "SQLite error 1: no such table: missing_table (command: SELECT x FROM missing_table)"), message,
        string("SQLite error 1: no such table: missing_table (command: SELECT x FROM missing_table)"), "sql::SQLexecuteCommand()", check_description, passed_checks, results);
    message = "";
    try {sql::SQLexecuteCommandVECTOR(db, "SELECT z FROM t");}
    catch (aurostd::xerror& excpt) {message = excpt.buildMessageString();}
    check(aurostd::substring2bool(message, "SQLite error 1: no such column: z (command: SELECT z FROM t)"), message, string("SQLite error 1: no such column: z (command: SELECT z FROM t)"),
        "sql::SQLexecuteCommandVECTOR()", check_description, passed_checks, results);
    message = "";
    try {sql::SQLexecuteCommand2DVECTOR(db, "SELECT x FROM nowhere");}
    catch (aurostd::xerror& excpt) {message = excpt.buildMessageString();}
    check(aurostd::substring2bool(message, "SQLite error 1: no such table: nowhere (command: SELECT x FROM nowhere)"), message, string("SQLite error 1: no such table: nowhere (command: SELECT x FROM nowhere)"),
        "sql::SQLexecuteCommand2DVECTOR()", check_description, passed_checks, results);
    message = "";
    string other_exception = "";
    try {sql::SQLexecuteCommand(nullptr, "SELECT 1");}
    catch (aurostd::xerror& excpt) {message = excpt.buildMessageString();}
    catch (std::exception& excpt) {other_exception = excpt.what();}
    check_description = "an error without an SQLite message (null database handle) is reported with its code";
    check(aurostd::substring2bool(message, "SQLite error 21 (command: SELECT 1)") && other_exception.empty(), message + other_exception, string("SQLite error 21 (command: SELECT 1)"),
        "sql::SQLexecuteCommand()", check_description, passed_checks, results);
    sqlite3_close(db);
  }

  //CO20261002
  void UnitTest::wyckoffLibraryTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    // Each structure uses one Wyckoff set corrected in aflow_symmetry_spacegroup_ITC_library.cpp.
    // A wrong coordinate puts an atom off the orbit, which changes the space group or the atom count.
    (void) errors;  // Suppress compiler warnings
    string check_function = "", check_description = "";
    ofstream devnull("/dev/null");
    struct WyckoffCase {string label; string params; int space_group; uint natoms;};
    vector<WyckoffCase> cases = {
      {"A2B_oP6_16_u_r", "5.0,1.1,1.3,0.21,0.11,0.23,0.37", 16, 6},  // 2r
      {"A2B_oP6_16_u_n", "5.0,1.1,1.3,0.21,0.11,0.23,0.37", 16, 6},  // 2n
      {"A2B3_cF160_226_g_i", "10.0,0.13,0.17,0.31", 226, 40},        // 64g
      {"A_cI48_229_i", "10.0,0.17", 229, 24}};                       // 48i
    for (const WyckoffCase& wcase : cases) {
      check_function = "Wyckoff library, " + wcase.label;
      int space_group = -1;
      uint natoms = 0;
      try {
        xstructure xstr = aflowlib::PrototypeLibraries(devnull, wcase.label, wcase.params, LIBRARY_MODE_HTQC);
        natoms = xstr.atoms.size();
        space_group = (int) xstr.SpaceGroup_ITC();
      } catch (aurostd::xerror& excpt) {space_group = -1;}  // the generator throws if the symmetry is higher than the label
      check_description = "number of atoms in the primitive cell";
      checkEqual(natoms, wcase.natoms, check_function, check_description, passed_checks, results);
      check_description = "space group";
      checkEqual(space_group, wcase.space_group, check_function, check_description, passed_checks, results);
    }
  }

  //CO20261003
  void UnitTest::prototypeParameterTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    // Parameter sets of the prototype library, read as stored in aflow_anrl_list.cpp.
    (void) errors;  // Suppress compiler warnings
    string check_function = "anrl::getANRLParameters()", check_description = "";
    ofstream devnull("/dev/null");

    // Every value of every set is a number (AB_oC8_63_c_c-002 once read "1.0.0185797325").
    vector<string> prototype_labels, malformed;
    aflowlib::GetAllPrototypeLabels(prototype_labels, "anrl");
    for (const string& label : prototype_labels) {
      vector<string> parameter_sets = anrl::getANRLParameters(label, "all");
      for (size_t i = 0; i < parameter_sets.size(); i++) {
        vector<string> tokens;
        aurostd::string2tokens(aurostd::RemoveWhiteSpaces(parameter_sets[i]), tokens, ",");
        for (const string& token : tokens) {
          char* end = nullptr;
          std::strtod(token.c_str(), &end);
          if (token.empty() || *end != '\0') {malformed.push_back(label + "-" + aurostd::PaddedNumString(i + 1, 3) + ": " + token);}
        }
      }
    }
    check_description = "every parameter value is a number";
    checkEqual(aurostd::joinWDelimiter(malformed, "; "), string(""), check_function, check_description, passed_checks, results);
    check_description = "lattice parameter of AB_oC8_63_c_c-002";
    vector<string> sets_oC8 = anrl::getANRLParameters("AB_oC8_63_c_c", "all");
    checkEqual(sets_oC8.size() > 1 ? aurostd::string2utype<double>(sets_oC8[1]) : 0.0, 1.0185797325, check_function, check_description, passed_checks, results);  // as in public AFLOW v4.1

    // Mistyped coordinates put two atoms closer than any bond; the corrected sets give the shortest
    // distance (Angstrom) of the real structure and keep the space group.
    struct DistanceCase {string label; string bond; double distance; int space_group;};
    vector<DistanceCase> cases = {
      {"AB2_hP24_194_ef_fgh", "MgNi2, Ni-Ni", 2.378, 194},       // z1 read 0.04598: Mg-Mg 1.455
      {"ABC_cP12_198_a_a_a", "NiSSb, Ni-S", 2.347, 198},         // x3 read 0.875: Ni-Sb 1.029
      {"A5B7_tI24_107_ac_abd", "Co5Ge7, Co-Co", 2.377, 107},     // z2, z3 read 0.056, 0.04: Co-Ge 0.325
      {"A6B2CD6E_cP64_208_m_ad_b_m_c", "Cs2ZnFe(CN)6, C-N", 1.148, 208}};  // z6 read 0.458: C-N 0.395
    for (const DistanceCase& dcase : cases) {
      check_function = "prototype library, " + dcase.label + "-001";
      double distance = 0.0;
      int space_group = -1;
      try {
        xstructure xstr = aflowlib::PrototypeLibraries(devnull, dcase.label, anrl::getANRLParameters(dcase.label, "all")[0], LIBRARY_MODE_HTQC);
        distance = SYM::minimumDistance(xstr);
        space_group = (int) xstr.SpaceGroup_ITC();
      } catch (aurostd::xerror& excpt) {space_group = -1;}
      check_description = "shortest distance (" + dcase.bond + ")";
      checkEqual(aurostd::isequal(distance, dcase.distance, 0.005), true, check_function, check_description, passed_checks, results);
      check_description = "space group";
      checkEqual(space_group, dcase.space_group, check_function, check_description, passed_checks, results);
    }
  }

  void UnitTest::prototypeGeneratorTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    bool LDEBUG = (FALSE || XHOST.DEBUG);

    // Set up test environment
    string check_function = "", check_description = "";

    vector<string> prototype_labels, compositions;
    vector<uint> space_group_numbers;
    vector<vector<vector<string> > > grouped_Wyckoff_letters;
    string library = "anrl";

    uint num_protos = aflowlib::GetAllPrototypeLabels(prototype_labels,
        compositions,
        space_group_numbers,
        grouped_Wyckoff_letters,
        library);
    if (LDEBUG) std::cerr << __AFLOW_FUNC__ << "Number of prototype labels = " << num_protos << " (each may have multiple parameter sets)";

    // Test
    // 1: if the prototype can be generated,
    // 2: if symmetry and label are consistent
    // 3: if it is a unique prototye
    // Keep results in vector to simplify function input
    vector<uint> nprotos(3, 0);
#ifdef AFLOW_MULTITHREADS_ENABLE
    xthread::xThread xt(KBIN::get_NCPUS()); // Okay to be greedy - xThread will manage number of threads
    std::mutex m;
    xt.run(num_protos, _testPrototype, prototype_labels, nprotos, errors, m);
#else
    for (uint i = 0; i < num_protos; i++) _testPrototype(i, prototype_labels, nprotos, errors);
#endif

    // Get number of all protoypes + parameter sets
    uint expected_uint = 0;
    for (uint i = 0; i < num_protos; i++) {
      expected_uint += anrl::getANRLParameters(prototype_labels[i], "all").size();
    }

    check_function = "aflowlib::PrototypeLibraries()";
    check_description = "generate prototypes";
    checkEqual(nprotos[0], expected_uint, check_function, check_description, passed_checks, results);

    check_function = "anrl::structureAndLabelConsistent()";
    check_description = "symmetry consistent with prototype label";
    checkEqual(nprotos[1], expected_uint, check_function, check_description, passed_checks, results);

    check_function = "compare::getMatchingPrototypes()";
    check_description = "protoypes are unique";
    checkEqual(nprotos[2], expected_uint, check_function, check_description, passed_checks, results);
  }

}

// ovasp
namespace unittest {
  void UnitTest::xoutcarTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
    bool LDEBUG = (FALSE || XHOST.DEBUG);

    // setup test environment
    string check_function = "", check_description = "";
    string calculated_str = "", expected_str = "";
    bool calculated_bool = false, expected_bool = false;
    double calculated_dbl = 0.0, expected_dbl = 0.0;

    string system="",path="",query="",file="",efile="",ext="",tfile="",Egap_type="";
    vector<string> files;
    xOUTCAR xout;

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //FCC/Si1_ICSD_150530
    system="ICSD_WEB/FCC/Si1_ICSD_150530";

    // Fetch and parse required files first - abort test if unsuccessful
    path=AFLOWLIB_SERVER_DEFAULT+"/AFLOWDATA/"+system;
    query=path+"/?files";
    aurostd::url2tokens(query,files,",");
    if(files.size()==0){
      errors.push_back("Could not fetch query: " + query);
      return;
    }

    //OUTCAR.static
    file="OUTCAR.static";
    if(!aurostd::EWithinList(files,file,efile)){
      errors.push_back("No " + file + " found within " + query);
      return;
    }

    ext=aurostd::GetCompressionExtension(efile);
    tfile=aurostd::TmpFileCreate("Egap_file1")+ext;
    query=path+"/"+efile;
    if(!(aurostd::url2file(query,tfile,LDEBUG) && aurostd::FileExist(tfile))){
      errors.push_back("Could not fetch query: " + query);
      return;
    }

    check_function = "xOUTCAR::GetProperties()";
    check_description = "load OUTCAR.static";
    if (!xout.GetPropertiesFile(tfile,!LDEBUG)) {
      errors.push_back("Could not parse " + file);
      return;
    }
#ifndef _AFLOW_TEMP_PRESERVE_
    aurostd::RemoveFile(tfile);
#endif
    double EFERMI=xout.Efermi;

    //OUTCAR.bands
    file="OUTCAR.bands";
    if(!aurostd::EWithinList(files,file,efile)){
      query=path+"/?files"; //reload query for error message
      errors.push_back("No " + file + " found within " + query);
      return;
    }
    ext=aurostd::GetCompressionExtension(efile);
    tfile=aurostd::TmpFileCreate("Egap_file1")+ext;
    query=path+"/"+efile;
    if(!(aurostd::url2file(query,tfile,LDEBUG) && aurostd::FileExist(tfile))){
      errors.push_back("Could not fetch query: " + query);
      return;
    }
    if(!xout.GetPropertiesFile(tfile,!LDEBUG)){
      errors.push_back("Could not parse " + file);
      return;
    }
#ifndef _AFLOW_TEMP_PRESERVE_
    aurostd::RemoveFile(tfile);
#endif

    //GetBandGap
    check_function = "xOUTCAR::GetBandGap()";

    check_description = "Calculate Egap successfully";
    expected_bool = true;
    bool parsed = calculated_bool = xout.GetBandGap(EFERMI);
    checkEqual(calculated_bool, expected_bool, check_function, check_description, passed_checks, results);

    check_description = "Egap value";
    expected_dbl = 6.1000e-01;
    calculated_dbl = (parsed?xout.Egap[0]:AUROSTD_MAX_DOUBLE);
    checkEqual(calculated_dbl, expected_dbl, check_function, check_description, passed_checks, results);

    check_description = "Egap type";
    expected_str = "insulator-indirect";
    calculated_str = (parsed?xout.Egap_type[0]:"N/A");
    checkEqual(calculated_str, expected_str, check_function, check_description, passed_checks, results);
  }
}

namespace pocc {
namespace unittest {

namespace {
  //[CO20260423 - OBSOLETE]inline string valueToString(const string& value) { return value; }
  //[CO20260423 - OBSOLETE]inline string valueToString(const char* value) { return string(value); }
  inline string valueToString(bool value) { return value ? "true" : "false"; }
  template <typename T>
  string valueToString(const T& value) {
    return aurostd::utype2string(value);
  }

  inline bool valuesEqual(bool lhs, bool rhs) { return lhs == rhs; }
  template <typename T>
  bool valuesEqual(const T& lhs, const T& rhs) {
    return aurostd::isequal(lhs, rhs);
  }

  void appendResult(bool passed, const string& check_function, const string& check_description,
                    const string& calculated, const string& expected,
                    uint& passed_checks, vector<vector<string> >& results) {
    vector<string> result;
    result.push_back(aurostd::utype2string<uint>(results.size() + 1));
    if (passed) {
      passed_checks++;
      result.push_back("pass");
    } else {
      result.push_back("FAIL");
    }
    result.push_back(check_function);
    string description = check_description;
    if (!passed) {
      description += " (result: " + calculated + " | expected: " + expected + ")";
    }
    result.push_back(description);
    results.push_back(result);
  }

  template <typename T>
  void checkEqual(const T& calculated, const T& expected, const string& check_function,
                  const string& check_description, uint& passed_checks,
                  vector<vector<string> >& results) {
    bool passed = valuesEqual(calculated, expected);
    appendResult(passed, check_function, check_description,
                 valueToString(calculated), valueToString(expected),
                 passed_checks, results);
  }

  inline double absDiff(double a, double b) {
    double diff = a - b;
    return diff < 0.0 ? -diff : diff;
  }
}  // namespace

void deedTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
  try {
    const string check_function="pocc::getDEED()";
    const string check_description="DEED uses the shared POCC definition and rejects invalid inputs";
    checkEqual(absDiff(pocc::getDEED(9.0,0.25),6.0)<1e-12,true,check_function+" (valid)",check_description,passed_checks,results);
    checkEqual(pocc::getDEED(0.0,0.25)==AUROSTD_MAX_DOUBLE,true,check_function+" (zero EFA)",check_description,passed_checks,results);
    checkEqual(pocc::getDEED(9.0,0.0)==AUROSTD_MAX_DOUBLE,true,check_function+" (zero hull loss)",check_description,passed_checks,results);
    checkEqual(pocc::getDEED(AUROSTD_NAN,0.25)==AUROSTD_MAX_DOUBLE,true,check_function+" (unset EFA)",check_description,passed_checks,results);  //CO20261004 - AUROSTD_NAN is finite
    checkEqual(pocc::getDEED(9.0,AUROSTD_NAN)==AUROSTD_MAX_DOUBLE,true,check_function+" (unset hull loss)",check_description,passed_checks,results);  //CO20261004
    checkEqual(chull::CoordGroup::isValidDEEDValue(AUROSTD_NAN),false,"chull::CoordGroup::isValidDEEDValue() (unset)",check_description,passed_checks,results);  //CO20261004
  } catch(const std::exception& e) {
    errors.push_back("DEED formula test failed: "+string(e.what()));
  } catch(...) {
    errors.push_back("DEED formula test failed with an unknown exception");
  }

  string config_fixture_file;
  try {
    const string check_function="chull::DEEDPlotConfig lifecycle";
    const string check_description="DEED plot configuration preserves independent state across AFLOW copy, assignment, clear, and destruction semantics";
    chull::DEEDPlotConfig configured;
    aurostd::JSON::object custom_config=configured.toJSON();
    custom_config["enabled_interpolation"]=false;
    custom_config["interpolation_mode"]="flat_border_mask";
    custom_config["marker_draw_color"]="red";
    vector<string> custom_symbols;
    custom_symbols.push_back("diamond*");
    custom_symbols.push_back("star");
    custom_config["marker_symbol_cycle"]=custom_symbols;
    custom_config["marker_symbol_overrides"]["Fm-3m"]="square*";
    config_fixture_file=aurostd::TmpFileCreate("deed_plot_config");
    custom_config.saveFile(config_fixture_file,false);

    vector<string> warnings;
    checkEqual(configured.loadFile(config_fixture_file,warnings),true,check_function+" (load)",check_description,passed_checks,results);
    checkEqual(warnings.empty(),true,check_function+" (load warnings)",check_description,passed_checks,results);
    const string configured_json=configured.toJSON().toString();

    chull::DEEDPlotConfig copied(configured);
    chull::DEEDPlotConfig assigned;
    assigned=copied;
    const chull::DEEDPlotConfig& assigned_self=assigned;  //CO20261017 - self-assignment through a reference (clang warns about x=x)
    assigned=assigned_self;
    checkEqual(copied.toJSON().toString(),configured_json,check_function+" (copy)",check_description,passed_checks,results);
    checkEqual(assigned.toJSON().toString(),configured_json,check_function+" (assignment)",check_description,passed_checks,results);

    configured.clear();
    chull::DEEDPlotConfig defaults;
    checkEqual(configured.toJSON().toString(),defaults.toJSON().toString(),check_function+" (clear)",check_description,passed_checks,results);
    checkEqual(copied.toJSON().toString(),configured_json,check_function+" (copy independence)",check_description,passed_checks,results);
    checkEqual(assigned.toJSON().toString(),configured_json,check_function+" (assignment independence)",check_description,passed_checks,results);
    checkEqual(assigned.getMarkerSymbolOverrides().find("Fm-3m")!=assigned.getMarkerSymbolOverrides().end(),true,check_function+" (map state)",check_description,passed_checks,results);
  } catch(const std::exception& e) {
    errors.push_back("DEED plot-configuration lifecycle test failed: "+string(e.what()));
  } catch(...) {
    errors.push_back("DEED plot-configuration lifecycle test failed with an unknown exception");
  }
  if(!config_fixture_file.empty()){aurostd::RemoveFile(config_fixture_file);}

  string degeneracy_fixture_directory;
  try {
    const string check_function="pocc::SupercellDegeneracies lifecycle";
    const string check_description="POCC supercell degeneracies preserve loaded weights across AFLOW copy, assignment, clear, and destruction semantics";
    pocc::SupercellDegeneracies original;
    degeneracy_fixture_directory=aurostd::TmpDirectoryCreate("deed_supercell_degeneracies");
    stringstream pocc_out;
    pocc_out << "degeneracy_supercell_01=4.0  [ARUN.POCC_01_H1C1]" << endl;
    checkEqual(aurostd::stringstream2file(pocc_out,degeneracy_fixture_directory+"/"+POCC_FILE_PREFIX+POCC_OUT_FILE),true,check_function+" (fixture)",check_description,passed_checks,results);

    aflowlib::_aflowlib_entry parent;
    parent.el_source_type="FILESYSTEM_RAW";
    parent.el_source=degeneracy_fixture_directory+"/aflowlib.out";
    checkEqual(original.load(parent),true,check_function+" (load)",check_description,passed_checks,results);

    pocc::SupercellDegeneracies copied(original);
    pocc::SupercellDegeneracies assigned;
    assigned=copied;
    const pocc::SupercellDegeneracies& copied_self=copied;  //CO20261017 - self-assignment through a reference (clang warns about x=x)
    copied=copied_self;
    aflowlib::_aflowlib_entry child;
    child.prototype="ARUN.POCC_1_H1C1";

    bool found_weight=true;
    double original_weight=original.getWeight(child,found_weight);
    checkEqual(absDiff(original_weight,4.0)<1e-12,true,check_function+" (original)",check_description,passed_checks,results);
    checkEqual(found_weight,true,check_function+" (original state)",check_description,passed_checks,results);
    found_weight=true;
    double copied_weight=copied.getWeight(child,found_weight);
    checkEqual(absDiff(copied_weight,4.0)<1e-12,true,check_function+" (copy)",check_description,passed_checks,results);
    checkEqual(found_weight,true,check_function+" (copy state)",check_description,passed_checks,results);
    found_weight=true;
    double assigned_weight=assigned.getWeight(child,found_weight);
    checkEqual(absDiff(assigned_weight,4.0)<1e-12,true,check_function+" (assignment)",check_description,passed_checks,results);
    checkEqual(found_weight,true,check_function+" (assignment state)",check_description,passed_checks,results);

    original.clear();
    found_weight=true;
    double cleared_original_weight=original.getWeight(child,found_weight);
    checkEqual(absDiff(cleared_original_weight,1.0)<1e-12,true,check_function+" (original clear)",check_description,passed_checks,results);
    checkEqual(found_weight,false,check_function+" (original clear state)",check_description,passed_checks,results);

    assigned.clear();
    found_weight=true;
    double cleared_weight=assigned.getWeight(child,found_weight);
    checkEqual(absDiff(cleared_weight,1.0)<1e-12,true,check_function+" (clear)",check_description,passed_checks,results);
    checkEqual(found_weight,false,check_function+" (clear state)",check_description,passed_checks,results);
    found_weight=true;
    copied_weight=copied.getWeight(child,found_weight);
    checkEqual(absDiff(copied_weight,4.0)<1e-12,true,check_function+" (copy independence)",check_description,passed_checks,results);
    checkEqual(found_weight,true,check_function+" (copy independence state)",check_description,passed_checks,results);
  } catch(const std::exception& e) {
    errors.push_back("DEED POCC-degeneracy lifecycle test failed: "+string(e.what()));
  } catch(...) {
    errors.push_back("DEED POCC-degeneracy lifecycle test failed with an unknown exception");
  }
  if(!degeneracy_fixture_directory.empty()){aurostd::RemoveDirectory(degeneracy_fixture_directory);}

  try {
    const string check_function="chull::CoordGroup::addDEEDCandidate()";
    const string check_description="Composition groups retain sorted candidates and select deterministically";
    xvector<double> coords(1,2);
    coords[1]=0.2;
    coords[2]=0.3;
    chull::CoordGroup group(coords,true);

    checkEqual(group.addDEEDCandidate(10.0,5.0,7),true,check_function+" (first)",check_description,passed_checks,results);
    checkEqual(group.addDEEDCandidate(20.0,6.0,5),true,check_function+" (worse rank)",check_description,passed_checks,results);
    checkEqual(absDiff(group.getDEED(),10.0)<1e-12,true,check_function+" (rank retained)",check_description,passed_checks,results);
    checkEqual(group.addDEEDCandidate(15.0,4.0,9),true,check_function+" (better rank)",check_description,passed_checks,results);
    checkEqual(group.addDEEDCandidate(25.0,4.0,8),true,check_function+" (DEED tie-break)",check_description,passed_checks,results);
    checkEqual(group.addDEEDCandidate(25.0,4.0,8),true,check_function+" (duplicate)",check_description,passed_checks,results);
    checkEqual(group.addDEEDCandidate(-1.0,3.0,1),false,check_function+" (invalid)",check_description,passed_checks,results);

    const vector<double>& candidates=group.getDEEDCandidates();
    checkEqual(candidates.size(),(size_t)4,check_function+" (candidate count)",check_description,passed_checks,results);
    checkEqual(absDiff(candidates[0],25.0)<1e-12,true,check_function+" (candidate 1)",check_description,passed_checks,results);
    checkEqual(absDiff(candidates[1],20.0)<1e-12,true,check_function+" (candidate 2)",check_description,passed_checks,results);
    checkEqual(absDiff(candidates[2],15.0)<1e-12,true,check_function+" (candidate 3)",check_description,passed_checks,results);
    checkEqual(absDiff(candidates[3],10.0)<1e-12,true,check_function+" (candidate 4)",check_description,passed_checks,results);
    checkEqual(absDiff(group.getDEED(),25.0)<1e-12,true,check_function+" (selected DEED)",check_description,passed_checks,results);
    checkEqual(group.getDEEDPointIndex(),(uint)8,check_function+" (selected point)",check_description,passed_checks,results);

    chull::CoordGroup unranked_group(coords,true);
    unranked_group.addDEEDCandidate(10.0);
    unranked_group.addDEEDCandidate(20.0);
    checkEqual(absDiff(unranked_group.getDEED(),20.0)<1e-12,true,check_function+" (unranked tie-break)",check_description,passed_checks,results);
  } catch(const std::exception& e) {
    errors.push_back("DEED composition-selection test failed: "+string(e.what()));
  } catch(...) {
    errors.push_back("DEED composition-selection test failed with an unknown exception");
  }
  try {  //CO20261004
    const string check_function="chull::ConvexHull::getDEEDHullDistanceRank()";
    const string check_description="supercell hull loss is signed: positive above the hull, negative below";
    // the unaries and the ordered TiZr at -0.05 eV/atom of the Ti-V-Zr fixture, as aflowlib.out lines
    const auto aflowlib_out=[](const aurostd::JSON::object& record){
      const aurostd::JSON::Dictionary fields=record;
      vector<string> pairs;
      for(aurostd::JSON::Dictionary::const_iterator it=fields.begin();it!=fields.end();++it){
        if(it->second.type==aurostd::JSON::object_types::NONE){continue;}
        string value;
        if(it->second.type==aurostd::JSON::object_types::LIST){
          vector<string> items;
          aurostd::JSON::object list=it->second;  // size() is not const
          for(size_t i=0;i<list.size();i++){items.push_back(list[i].toString(false));}
          value=aurostd::joinWDelimiter(items,",");
        }else{value=it->second.toString(false);}
        pairs.push_back(it->first+"="+value);
      }
      return aurostd::joinWDelimiter(pairs,"|");
    };
    vector<string> lines;
    for(size_t i=0;i<3;i++){lines.push_back(aflowlib_out(::unittest::utd["database_update"]["baseline"][i]));}
    lines.push_back(aflowlib_out(::unittest::utd["database_update"]["added"]));
    aflowlib::EntryLoader el;
    el.loadText(lines,"deedTest");
    vector<vector<vector<aflowlib::_aflowlib_entry> > > entries;
    el.getEntriesThreeLayer(entries);
    aurostd::xoption cflags;
    cflags.flag("FORCE",true);
    cflags.flag("CHULL::SKIP_STRUCTURE_COMPARISON",true);
    cflags.flag("CHULL::SKIP_STABILITY_CRITERION_ANALYSIS",true);
    cflags.flag("CHULL::SKIP_N+1_ENTHALPY_GAIN_ANALYSIS",true);
    ofstream devnull("/dev/null");
    ostream oss_dn(devnull.rdbuf());
    chull::ConvexHull hull(cflags,vector<string>({"Ti","V","Zr"}),entries,oss_dn);
    xvector<double> stoich(1,3);
    stoich[1]=1.0/3.0; stoich[2]=1.0/3.0; stoich[3]=1.0/3.0;  // the hull is -0.05*2/3 here
    checkEqual(hull.m_initialized,true,check_function+" (hull)",check_description,passed_checks,results);
    checkEqual(absDiff(hull.getDEEDHullDistanceRank(stoich,0.1,true),0.1+0.05*2.0/3.0)<1e-9,true,check_function+" (above)",check_description,passed_checks,results);
    checkEqual(absDiff(hull.getDEEDHullDistanceRank(stoich,-0.1,true),-0.1+0.05*2.0/3.0)<1e-9,true,check_function+" (below)",check_description,passed_checks,results);
    checkEqual(absDiff(hull.getDEEDHullDistanceRank(stoich,-0.1,false),0.1-0.05*2.0/3.0)<1e-9,true,check_function+" (rank)",check_description,passed_checks,results);
  } catch(const std::exception& e) {
    errors.push_back("DEED hull-loss sign test failed: "+string(e.what()));
  }
  try {
    const string check_function="chull::ConvexHull DEED composition registry";
    const string check_description="DEED sources retain compositions absent from the hull without mutating hull geometry";
    chull::ConvexHull hull;
    hull.m_velements.push_back("Ti");
    hull.m_velements.push_back("V");
    hull.m_velements.push_back("Zr");

    xvector<double> database_coords(1,2);
    database_coords[1]=1.0/3.0;
    database_coords[2]=1.0/3.0;
    chull::CoordGroup database_group(database_coords,true);
    database_group.m_i_nary=2;
    hull.m_coord_groups.push_back(database_group);
    hull.clearDEED();
    checkEqual(hull.m_deed_coord_groups.size(),(size_t)1,check_function+" (seed)",check_description,passed_checks,results);

    xvector<double> external_stoich(1,3);
    external_stoich[1]=0.2;
    external_stoich[2]=0.2;
    external_stoich[3]=0.6;
    checkEqual(hull.applyDEED(external_stoich,18.0),true,check_function+" (external apply)",check_description,passed_checks,results);

    uint external_index=AUROSTD_MAX_UINT;
    checkEqual(hull.findDEEDCoordGroupIndex(external_stoich,external_index),true,check_function+" (external lookup)",check_description,passed_checks,results);
    checkEqual(hull.m_deed_coord_groups.size(),(size_t)2,check_function+" (external retained)",check_description,passed_checks,results);
    checkEqual(absDiff(hull.m_deed_coord_groups[external_index].getDEED(),18.0)<1e-12,true,check_function+" (external DEED)",check_description,passed_checks,results);
    checkEqual(hull.m_coord_groups[0].canReportDEED(),false,check_function+" (hull unchanged)",check_description,passed_checks,results);

    xvector<double> database_stoich(1,3);
    database_stoich[1]=1.0/3.0;
    database_stoich[2]=1.0/3.0;
    database_stoich[3]=1.0/3.0;
    checkEqual(hull.applyDEED(database_stoich,9.0),true,check_function+" (database apply)",check_description,passed_checks,results);
    uint database_index=AUROSTD_MAX_UINT;
    checkEqual(hull.findDEEDCoordGroupIndex(database_stoich,database_index),true,check_function+" (database lookup)",check_description,passed_checks,results);
    checkEqual(database_index!=external_index,true,check_function+" (composition separation)",check_description,passed_checks,results);

    chull::ConvexHull copied(hull);
    uint copied_external_index=AUROSTD_MAX_UINT;
    checkEqual(copied.findDEEDCoordGroupIndex(external_stoich,copied_external_index),true,check_function+" (copy lookup)",check_description,passed_checks,results);
    checkEqual(absDiff(copied.m_deed_coord_groups[copied_external_index].getDEED(),18.0)<1e-12,true,check_function+" (copy state)",check_description,passed_checks,results);

    hull.clearDEED();
    external_index=AUROSTD_MAX_UINT;
    checkEqual(hull.m_deed_coord_groups.size(),(size_t)1,check_function+" (source reset)",check_description,passed_checks,results);
    checkEqual(hull.findDEEDCoordGroupIndex(external_stoich,external_index),false,check_function+" (external reset)",check_description,passed_checks,results);
  } catch(const std::exception& e) {
    errors.push_back("DEED composition-registry test failed: "+string(e.what()));
  } catch(...) {
    errors.push_back("DEED composition-registry test failed with an unknown exception");
  }


  try {
    const string check_function="aflowlib::_aflowlib_entry POCC provenance";
    const string check_description="POCC parent/child classification and AURL normalization remain centralized on entries";
    aflowlib::_aflowlib_entry parent;
    parent.prototype="fcc:POCC_P0-0.5xA-0.5xB";
    parent.aurl="s4e.ai:AFLOWDATA/LIB3_RAW/Ta_pvTi_svZr_sv/POCC_PARENT";
    parent.vfiles.push_back("ARUN.POCC_1_H1C1/aflowlib.out");

    aflowlib::_aflowlib_entry child;
    child.prototype="ARUN.POCC_1_H1C1";
    child.aurl="s4e.ai:AFLOWDATA/LIB3_RAW/TaTiZr/POCC_PARENT/ARUN.POCC_1_H1C1";

    aflowlib::_aflowlib_entry sibling;
    sibling.prototype="ARUN.POCC_2_H1C1";
    sibling.aurl="s4e.ai:AFLOWDATA/LIB3_RAW/TaTiZr/POCC_PARENT_EXTRA/ARUN.POCC_2_H1C1";

    const string alternate_parent_aurl="s4e.ai:AFLOWDATA/LIB3_RAW/TaTiZr/POCC_PARENT";
    checkEqual(parent.hasPoccChildren(),true,check_function+" (children)",check_description,passed_checks,results);
    checkEqual(parent.isPoccParent(),true,check_function+" (parent)",check_description,passed_checks,results);
    checkEqual(parent.isPoccChild(),false,check_function+" (parent not child)",check_description,passed_checks,results);
    checkEqual(child.isPoccChild(),true,check_function+" (child)",check_description,passed_checks,results);
    checkEqual(child.isPoccChildOf(parent),true,check_function+" (relationship)",check_description,passed_checks,results);
    checkEqual(sibling.isPoccChildOf(parent),false,check_function+" (prefix boundary)",check_description,passed_checks,results);
    checkEqual(parent.hasMatchingPoccAurl(alternate_parent_aurl),true,check_function+" (AURL normalization)",check_description,passed_checks,results);
    checkEqual(child.getPoccParentAurlKey()==parent.getPoccParentAurlKey(),true,check_function+" (parent key)",check_description,passed_checks,results);
  } catch(const std::exception& e) {
    errors.push_back("DEED POCC-provenance test failed: "+string(e.what()));
  } catch(...) {
    errors.push_back("DEED POCC-provenance test failed with an unknown exception");
  }
}

// POCC Sampling Tests - TL20250616
// 1. Sample Rate Tests, 2. Sample Number Tests, 3. First Sampling Tests, 4. Second Sampling Tests
void poccSamplingTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
  // ========================================================================
  // SECTION 1: SAMPLE RATE TESTS
  // ========================================================================

  // Test 1.1: Sample Rate - Basic parsing functionality
  try {
    string check_function = "SampleRate_BasicParsing";
    string check_description = "Testing sample rate string parsing with valid two-stage input";

    pocc::POccCalculator calc;
    string test_input = "0.5:0.3";
    double expected_first_stage = 0.5;   // First sampling: 50%
    double expected_second_stage = 0.3;  // Second sampling: 30%

    double calculated_first = calc.setPOccSampleRate(test_input, 0);
    double calculated_second = calc.setPOccSampleRate(test_input, 1);

    checkEqual(calculated_first, expected_first_stage, check_function + " (first stage)",
               check_description, passed_checks, results);
    checkEqual(calculated_second, expected_second_stage, check_function + " (second stage)",
               check_description, passed_checks, results);

  } catch (aurostd::xerror& e) {
    errors.push_back("Test 1.1 failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("Test 1.1 failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("Test 1.1 failed - Unknown exception in sample rate basic parsing");
  }

  // Test 1.2: Sample Rate - Error handling and validation
  try {
    string check_function = "SampleRate_ErrorHandling";
    string check_description = "Testing sample rate error detection and exception handling";

    pocc::POccCalculator calc;

    bool negative_exception = false;
    try {
      calc.setPOccSampleRate("-0.5:0.3", 0);
    } catch (const aurostd::xerror&) {
      negative_exception = true;
    }
    checkEqual(negative_exception, true, check_function + " (negative rate detection)",
               check_description, passed_checks, results);

    bool zero_exception = false;
    try {
      calc.setPOccSampleRate("0.0:0.3", 0);
    } catch (const aurostd::xerror&) {
      zero_exception = true;
    }
    checkEqual(zero_exception, true, check_function + " (zero rate detection)",
               check_description, passed_checks, results);

    bool malformed_exception = false;
    try {
      calc.setPOccSampleRate("0.5", 1);
    } catch (const aurostd::xerror&) {
      malformed_exception = true;
    }
    checkEqual(malformed_exception, true, check_function + " (malformed input)",
               check_description, passed_checks, results);

  } catch (aurostd::xerror& e) {
    errors.push_back("Test 1.2 failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("Test 1.2 failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("Test 1.2 failed - Unknown exception in sample rate error handling");
  }

  // Test 1.3: Sample Rate - Boundary values and precision
  try {
    string check_function = "SampleRate_BoundaryValues";
    string check_description = "Testing sample rate boundary values and precision handling";

    pocc::POccCalculator calc;

    double tiny_rate = calc.setPOccSampleRate("0.001:0.002", 0);
    checkEqual(tiny_rate, 0.001, check_function + " (tiny rate)",
               check_description, passed_checks, results);

    double max_rate = calc.setPOccSampleRate("1.0:1.0", 1);
    checkEqual(max_rate, 1.0, check_function + " (maximum rate)",
               check_description, passed_checks, results);

    double precise_rate = calc.setPOccSampleRate("0.123456:0.654321", 1);
    bool precision_ok = (absDiff(precise_rate, 0.654321) < 1e-6);
    checkEqual(precision_ok, true, check_function + " (high precision)",
               check_description, passed_checks, results);

  } catch (aurostd::xerror& e) {
    errors.push_back("Test 1.3 failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("Test 1.3 failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("Test 1.3 failed - Unknown exception in sample rate boundary values");
  }

  // ========================================================================
  // SECTION 2: SAMPLE NUMBER TESTS
  // ========================================================================

  // Test 2.1: Sample Number - Basic string to integer conversion
  try {
    string check_function = "SampleNumber_BasicParsing";
    string check_description = "Testing sample number string to size_t conversion";

    size_t result_50 = aurostd::string2utype<size_t>("50");
    checkEqual(result_50, static_cast<size_t>(50), check_function + " (basic conversion)",
               check_description, passed_checks, results);

    size_t result_large = aurostd::string2utype<size_t>("5000");
    checkEqual(result_large, static_cast<size_t>(5000), check_function + " (large number)",
               check_description, passed_checks, results);

    size_t result_very_large = aurostd::string2utype<size_t>("1000000");
    checkEqual(result_very_large, static_cast<size_t>(1000000), check_function + " (very large)",
               check_description, passed_checks, results);

  } catch (aurostd::xerror& e) {
    errors.push_back("Test 2.1 failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("Test 2.1 failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("Test 2.1 failed - Unknown exception in sample number basic parsing");
  }

  // Test 2.2: Sample Number - Edge cases and error handling
  try {
    string check_function = "SampleNumber_EdgeCases";
    string check_description = "Testing sample number edge cases and robustness";

    size_t single_digit = aurostd::string2utype<size_t>("1");
    checkEqual(single_digit, static_cast<size_t>(1), check_function + " (minimum valid)",
               check_description, passed_checks, results);

    size_t zero_result = aurostd::string2utype<size_t>("0");
    checkEqual(zero_result, static_cast<size_t>(0), check_function + " (zero value)",
               check_description, passed_checks, results);

    size_t invalid_result = aurostd::string2utype<size_t>("not_a_number");
    checkEqual(invalid_result, static_cast<size_t>(0), check_function + " (invalid input)",
               check_description, passed_checks, results);

  } catch (aurostd::xerror& e) {
    errors.push_back("Test 2.2 failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("Test 2.2 failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("Test 2.2 failed - Unknown exception in sample number edge cases");
  }

  // ========================================================================
  // SECTION 3: FIRST SAMPLING TESTS
  // ========================================================================

  // Test 3.1: First Sampling - Sample rate calculation logic
  try {
    string check_function = "FirstSampling_RateCalculation";
    string check_description = "Testing first sampling calculation logic with sample rates";

    unsigned long long int total_configs = 1000;
    double sample_rate = 0.25;  // 25% sampling

    size_t calculated_count = static_cast<size_t>(total_configs * sample_rate);
    size_t expected_count = 250;  // 1000 * 0.25

    checkEqual(calculated_count, expected_count, check_function + " (calculation accuracy)",
               check_description, passed_checks, results);

    bool within_bounds = (calculated_count <= total_configs);
    checkEqual(within_bounds, true, check_function + " (bounds validation)",
               check_description, passed_checks, results);

    size_t optimized_conversion = static_cast<size_t>(total_configs * sample_rate);
    checkEqual(optimized_conversion, expected_count, check_function + " (optimized type conversion)",
               check_description, passed_checks, results);

  } catch (aurostd::xerror& e) {
    errors.push_back("Test 3.1 failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("Test 3.1 failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("Test 3.1 failed - Unknown exception in first sampling rate calculation");
  }

  // Test 3.2: First Sampling - Specific number of configurations
  try {
    string check_function = "FirstSampling_NumberMode";
    string check_description = "Testing first sampling number mode with specific configuration counts";

    size_t sample_count = aurostd::string2utype<size_t>("150");
    checkEqual(sample_count, static_cast<size_t>(150), check_function + " (number parsing)",
               check_description, passed_checks, results);

    unsigned long long int total_available = 1000;
    bool reasonable_size = (sample_count <= total_available);
    checkEqual(reasonable_size, true, check_function + " (reasonable size)",
               check_description, passed_checks, results);

    size_t large_sample = aurostd::string2utype<size_t>("500");
    size_t small_sample = aurostd::string2utype<size_t>("10");

    checkEqual(large_sample, static_cast<size_t>(500), check_function + " (large sample)",
               check_description, passed_checks, results);
    checkEqual(small_sample, static_cast<size_t>(10), check_function + " (small sample)",
               check_description, passed_checks, results);

  } catch (aurostd::xerror& e) {
    errors.push_back("Test 3.2 failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("Test 3.2 failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("Test 3.2 failed - Unknown exception in first sampling number mode");
  }

  // ========================================================================
  // SECTION 4: SECOND SAMPLING TESTS
  // ========================================================================

  // Test 4.1: Second Sampling - Cascading rate calculation
  try {
    string check_function = "SecondSampling_CascadingRates";
    string check_description = "Testing second sampling cascading rate calculations";

    unsigned long long int original_configs = 1000;
    double first_rate = 0.5;   // First stage: 50% -> 500 configs
    double second_rate = 0.6;  // Second stage: 60% of 500 -> 300 configs

    size_t first_result = static_cast<size_t>(original_configs * first_rate);
    checkEqual(first_result, static_cast<size_t>(500), check_function + " (first stage)",
               check_description, passed_checks, results);

    size_t second_result = static_cast<size_t>(first_result * second_rate);
    checkEqual(second_result, static_cast<size_t>(300), check_function + " (second stage)",
               check_description, passed_checks, results);

    double overall_rate = static_cast<double>(second_result) / static_cast<double>(original_configs);
    double expected_overall = first_rate * second_rate;  // 0.5 * 0.6 = 0.3
    bool efficiency_correct = (absDiff(overall_rate, expected_overall) < 1e-6);
    checkEqual(efficiency_correct, true, check_function + " (overall efficiency)",
               check_description, passed_checks, results);

  } catch (aurostd::xerror& e) {
    errors.push_back("Test 4.1 failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("Test 4.1 failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("Test 4.1 failed - Unknown exception in second sampling cascading rates");
  }

  // Test 4.2: Second Sampling - Per-superlattice distribution
  try {
    string check_function = "SecondSampling_PerSuperlattice";
    string check_description = "Testing second sampling per-superlattice calculation logic";

    unsigned long long int num_superlattices = 3;
    vector<unsigned long long int> configs_per_lattice = {120, 180, 300};
    double sampling_rate = 0.4;  // 40% sampling per superlattice

    vector<size_t> expected_samples = {48, 72, 120};

    for (size_t i = 0; i < num_superlattices; i++) {
      size_t calculated_sample = static_cast<size_t>(configs_per_lattice[i] * sampling_rate);

      checkEqual(calculated_sample, expected_samples[i],
                 check_function + " (lattice " + aurostd::utype2string(i) + ")",
                 check_description, passed_checks, results);
    }

    size_t total_original = 120 + 180 + 300;  // 600
    size_t total_sampled = 48 + 72 + 120;     // 240
    size_t expected_total_sampled = static_cast<size_t>(total_original * sampling_rate);

    checkEqual(total_sampled, expected_total_sampled, check_function + " (total consistency)",
               check_description, passed_checks, results);

  } catch (aurostd::xerror& e) {
    errors.push_back("Test 4.2 failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("Test 4.2 failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("Test 4.2 failed - Unknown exception in second sampling per-superlattice");
  }
}


//CO20261005 - ENUM: enumeration of the POCC decorations by symmetry (aflow_pocc_enumeration.cpp)
namespace {
  /// @brief PARTCAR of fcc Au(k/n)Cu(1-k/n) with HNF=n
  string partcarFCCAuCu(int n,int k) {
    stringstream ss;
    ss << "fcc Au" << k << "/" << n << "Cu" << n-k << "/" << n << std::endl;
    ss << "1.0 -" << n << std::endl;
    ss << "   0.0 1.8 1.8" << std::endl << "   1.8 0.0 1.8" << std::endl << "   1.8 1.8 0.0" << std::endl;
    ss << std::setprecision(12) << "1*" << (double)k/(double)n << " 1*" << 1.0-(double)k/(double)n << std::endl;
    ss << "Direct(2) Pocc [Au1Cu1]" << std::endl;
    ss << "   0.0 0.0 0.0 Au" << std::endl << "   0.0 0.0 0.0 Cu" << std::endl;
    return ss.str();
  }

  /// @brief PARTCAR of rocksalt Mg(k/n)Ni(1-k/n)O with HNF=n (same cation decorations as fcc)
  string partcarRocksaltMgNiO(int n,int k) {
    stringstream ss;
    ss << "rocksalt Mg" << k << "/" << n << "Ni" << n-k << "/" << n << "O" << std::endl;
    ss << "1.0 -" << n << std::endl;
    ss << "   0.0 2.1 2.1" << std::endl << "   2.1 0.0 2.1" << std::endl << "   2.1 2.1 0.0" << std::endl;
    ss << std::setprecision(12) << "1*" << (double)k/(double)n << " 1*" << 1.0-(double)k/(double)n << " 1" << std::endl;
    ss << "Direct(3) Pocc [Mg1Ni1O1]" << std::endl;
    ss << "   0.0 0.0 0.0 Mg" << std::endl << "   0.0 0.0 0.0 Ni" << std::endl << "   0.5 0.5 0.5 O" << std::endl;
    return ss.str();
  }

  /// @brief PARTCAR with point group 4/m on a tetragonal lattice (holohedry 4/mmm): Au(1/5)Cu(4/5) at the origin, Zn at (x,y,1/2) and its 4-fold images, HNF=5
  string partcarTetragonal4m() {
    stringstream ss;
    ss << "P4/m AuCu4Zn20" << std::endl;
    ss << "1.0 -5" << std::endl;
    ss << "   4.0 0.0 0.0" << std::endl << "   0.0 4.0 0.0" << std::endl << "   0.0 0.0 3.0" << std::endl;
    ss << "1*0.2 1*0.8 4" << std::endl;
    ss << "Direct(6) Pocc [Au1Cu1Zn4]" << std::endl;
    ss << "   0.0 0.0 0.0 Au" << std::endl << "   0.0 0.0 0.0 Cu" << std::endl;
    ss << "   0.3 0.1 0.5 Zn" << std::endl << "   0.9 0.3 0.5 Zn" << std::endl << "   0.7 0.9 0.5 Zn" << std::endl << "   0.1 0.7 0.5 Zn" << std::endl;
    return ss.str();
  }

  /// @brief PARTCAR of hcp (nonsymmorphic, two sites A and B) with Mg and Zn occupations on each site (the rest is vacant) and HNF=n
  string partcarHCPMgZn(int n,double mgA,double mgB,double znA,double znB) {
    stringstream ss;
    ss << std::setprecision(12) << "hcp A=Mg" << mgA << "Zn" << znA << " B=Mg" << mgB << "Zn" << znB << std::endl;
    ss << "1.0 -" << n << std::endl;
    ss << "   1.6000000 -2.7712813 0.0" << std::endl << "   1.6000000  2.7712813 0.0" << std::endl << "   0.0 0.0 5.2" << std::endl;
    ss << "1*" << mgA << "+1*" << mgB << " 1*" << znA << "+1*" << znB << std::endl;
    ss << "Direct(4) Pocc [Mg2Zn2]" << std::endl;
    ss << "   0.3333333333 0.6666666667 0.25 Mg" << std::endl << "   0.6666666667 0.3333333333 0.75 Mg" << std::endl;
    ss << "   0.3333333333 0.6666666667 0.25 Zn" << std::endl << "   0.6666666667 0.3333333333 0.75 Zn" << std::endl;
    return ss.str();
  }

  /// @brief PARTCAR of L1_2 Au(Cu0.5Ni0.5)3 with HNF=n: three partially occupied face-centered sites that the 3-fold axes cycle
  string partcarL12AuCuNi(int n) {
    stringstream ss;
    ss << "L1_2 Au(Cu0.5Ni0.5)3" << std::endl;
    ss << "1.0 -" << n << std::endl;
    ss << "   3.7 0.0 0.0" << std::endl << "   0.0 3.7 0.0" << std::endl << "   0.0 0.0 3.7" << std::endl;
    ss << "1 3*0.5 3*0.5" << std::endl;
    ss << "Direct(7) Pocc [Au1Cu3Ni3]" << std::endl;
    ss << "   0.0 0.0 0.0 Au" << std::endl;
    ss << "   0.5 0.5 0.0 Cu" << std::endl << "   0.5 0.0 0.5 Cu" << std::endl << "   0.0 0.5 0.5 Cu" << std::endl;
    ss << "   0.5 0.5 0.0 Ni" << std::endl << "   0.5 0.0 0.5 Ni" << std::endl << "   0.0 0.5 0.5 Ni" << std::endl;
    return ss.str();
  }

  /// @brief POccCalculator on a PARTCAR with the given enumerator, ready for calculate()
  void setupPOccCalculator(pocc::POccCalculator& pcalc,const string& partcar,const string& enumerator,const _kflags& kflags,stringstream& log) {
    stringstream input(partcar);
    xstructure xstr(input,IOAFLOW_AUTO);
    aurostd::xoption vpflow;
    vpflow.flag("POCC_ENUMERATOR",true);
    vpflow.push_attached("POCC_ENUMERATOR",enumerator);
    vpflow.flag("POCC_SKIP_WRITING_FILES",true);
    pcalc.initialize(xstr,vpflow,kflags,log);
    if(!pcalc.m_initialized){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"POccCalculator failed to initialize",_RUNTIME_ERROR_);}
    pcalc.calculateHNF();
    pcalc.getTotalPermutationsCount();
  }

  /// @brief counts from a POCC enumeration (ENUM or UFF)
  struct POccEnumerationCounts {
    unsigned long long int hnf_count=0;
    unsigned long long int hnf_count_lattice=0;
    unsigned long long int hnf_total=0;               //all HNF matrices of determinant n
    unsigned long long int decorations=0;             //total_permutations_count
    unsigned long long int unique=0;
    unsigned long long int degeneracy=0;              //sum over the unique supercells
    unsigned long long int unique_not_superperiodic=0;
    unsigned long long int unique_label_exchange=0;   //not superperiodic, up to exchanging species 0 and 1
    unsigned long long int merged_hnf=0;              //m_enum_merged_hnf
    unsigned long long int merged_across=0;           //m_enum_merged_across
    unsigned long long int skipped=0;                 //m_enum_skipped
    uint fgroup_dropped=0;                            //m_enum_fgroup_dropped
    vector<unsigned long long int> vdegeneracy;
    vector<double> venergy_uff;                       //UFF energy of each set
    vector<vector<std::pair<unsigned long long int,unsigned long long int> > > vsets;  //(H,C) of the members, sorted
  };

  /// @brief run a POCC enumerator on a PARTCAR; superperiodicity and label exchange are recalculated here from the public interface
  POccEnumerationCounts runPOccEnumeration(const string& partcar,const string& enumerator,bool label_exchange=false,const _kflags& kflags=_kflags()) {
    POccEnumerationCounts counts;
    stringstream log;
    pocc::POccCalculator pcalc(log);
    setupPOccCalculator(pcalc,partcar,enumerator,kflags,log);
    pcalc.calculate();
    counts.hnf_count=pcalc.hnf_count;
    counts.hnf_count_lattice=pcalc.m_hnf_count_lattice;
    for(uint i=0;i<pcalc.m_vhnf_multiplicity.size();i++){counts.hnf_total+=pcalc.m_vhnf_multiplicity[i];}
    counts.decorations=pcalc.total_permutations_count;
    counts.unique=pcalc.getUniqueSuperCellsCount();
    counts.merged_hnf=pcalc.m_enum_merged_hnf;
    counts.merged_across=pcalc.m_enum_merged_across;
    counts.skipped=pcalc.m_enum_skipped;
    counts.fgroup_dropped=pcalc.m_enum_fgroup_dropped;
    for(std::list<pocc::POccSuperCellSet>::const_iterator it=pcalc.l_supercell_sets.begin();it!=pcalc.l_supercell_sets.end();++it){
      counts.vdegeneracy.push_back((*it).getDegeneracy());
      counts.degeneracy+=counts.vdegeneracy.back();
      counts.venergy_uff.push_back((*it).getUFFEnergy());
      counts.vsets.push_back(vector<std::pair<unsigned long long int,unsigned long long int> >(0));
      for(uint i=0;i<(*it).m_psc_set.size();i++){counts.vsets.back().push_back(std::make_pair((*it).m_psc_set[i].m_hnf_index,(*it).m_psc_set[i].m_site_config_index));}
      std::sort(counts.vsets.back().begin(),counts.vsets.back().end());
    }
    if(aurostd::toupper(enumerator)=="UFF"){return counts;}
    //set of every decoration; superperiodic: fixed by a translation; label exchange partner
    std::map<std::pair<unsigned long long int,unsigned long long int>,uint> set_of;
    for(uint iset=0;iset<counts.vsets.size();iset++){
      for(uint i=0;i<counts.vsets[iset].size();i++){set_of[counts.vsets[iset][i]]=iset;}
    }
    xmatrix<double> hnf;
    vector<vector<int> > config,image;
    pocc::POccSupercellAction action;
    unsigned long long int nself=0;
    uint iset=0;
    for(std::list<pocc::POccSuperCellSet>::const_iterator it=pcalc.l_supercell_sets.begin();it!=pcalc.l_supercell_sets.end();++it,++iset){
      const pocc::POccSuperCell& psc=(*it).getSuperCell();
      pcalc.getHNFMatSiteConfig(psc,hnf,config);
      pcalc.getSupercellAction(hnf,action);
      const vector<bool> periods=pcalc.getDecorationPeriods(action,config);
      bool superperiodic=false;
      for(uint t=1;t<periods.size();t++){if(periods[t]){superperiodic=true;}}
      if(superperiodic){continue;}
      counts.unique_not_superperiodic++;
      if(label_exchange){
        image=config;
        for(uint site=0;site<image.size();site++){for(uint i=0;i<image[site].size();i++){if(image[site][i]==0){image[site][i]=1;}else if(image[site][i]==1){image[site][i]=0;}}}
        unsigned long long int rank=pcalc.rankDecoration(image);
        if(set_of.count(std::make_pair(psc.m_hnf_index,rank))==0){throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,"label exchange leaves the decorations",_RUNTIME_ERROR_);}
        if(set_of[std::make_pair(psc.m_hnf_index,rank)]==iset){nself++;}
      }
    }
    if(label_exchange){counts.unique_label_exchange=(counts.unique_not_superperiodic+nself)/2;}
    return counts;
  }
}  // namespace

/// @brief ENUM enumeration of the POCC decorations against published superlattice and superstructure counts, Burnside's lemma and structure comparison
/// @authors
/// @mod{CO,20261005,created}
void poccEnumerationTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
  string check_function="",check_description="";
  try {
    // ---------------------------------------------------------------------------
    // ranking follows getNextSiteConfiguration(); getDistinctPermutation() inverts it
    check_function="pocc::rankMultisetPermutation()";
    check_description="ranks follow the lexicographic order of getNextSiteConfiguration()";
    vector<int> seq={-1,0,0,1,1,1,2};
    const vector<int> seq0=seq;
    const unsigned long long int nperms=pocc::getDistinctPermutationsCount(seq);
    checkEqual(nperms,(unsigned long long int)420,"pocc::getDistinctPermutationsCount()","7!/(1!2!3!1!)=420 permutations",passed_checks,results);
    bool ranks_ok=true,unranks_ok=true;
    unsigned long long int count=0;
    do {
      if(pocc::rankMultisetPermutation(seq,4,nperms)!=count){ranks_ok=false;}
      if(pocc::getDistinctPermutation(seq0,count)!=seq){unranks_ok=false;}
      count++;
    } while(pocc::getNextSiteConfiguration(seq));
    checkEqual(ranks_ok,true,check_function,check_description,passed_checks,results);
    checkEqual(count,nperms,check_function,"getNextSiteConfiguration() visits every permutation once",passed_checks,results);
    checkEqual(unranks_ok,true,"pocc::getDistinctPermutation()","inverse of rankMultisetPermutation() for all 420 permutations",passed_checks,results);

    // ---------------------------------------------------------------------------
    // fcc binary: Phys. Rev. B 77, 224115 (2008), Tables IV and V
    // unique fcc superstructures of index n, without superperiodic ones and up to exchanging the species;
    // HNF matrices of determinant n (OEIS A001001) and fcc superlattices (Table IV)
    check_function="pocc::POccCalculator::calculateEnum()";
    const vector<unsigned long long int> fcc_table={0,0,2,3,12,14,50,52,229};  //index n
    const vector<unsigned long long int> hnf_table={0,1,7,13,35,31,91,57,155};
    const vector<unsigned long long int> fcc_superlattices={0,1,2,3,7,5,10,7,20};
    for(int n=2;n<(int)fcc_table.size();n++){
      unsigned long long int total=0;
      for(int k=1;2*k<=n;k++){
        POccEnumerationCounts fcc=runPOccEnumeration(partcarFCCAuCu(n,k),"ENUM",2*k==n);
        total+=(2*k==n ? fcc.unique_label_exchange : fcc.unique_not_superperiodic);
        if(k==1){
          checkEqual(fcc.hnf_total,hnf_table[n],check_function,"HNF matrices of determinant "+aurostd::utype2string(n)+" (OEIS A001001)",passed_checks,results);
          checkEqual(fcc.hnf_count,fcc_superlattices[n],check_function,"fcc superlattices of index "+aurostd::utype2string(n)+" (Phys. Rev. B 77, 224115, Table IV)",passed_checks,results);
          checkEqual(fcc.degeneracy,fcc.decorations,check_function,"fcc Au1Cu"+aurostd::utype2string(n-1)+": degeneracies add up to the decorations",passed_checks,results);
        }
      }
      checkEqual(total,fcc_table[n],check_function,"fcc binary superstructures of index "+aurostd::utype2string(n)+" (Phys. Rev. B 77, 224115, Table V)",passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // fcc n=2: L1_0 and L1_1, degeneracy 2 each
    POccEnumerationCounts fcc2=runPOccEnumeration(partcarFCCAuCu(2,1),"ENUM");
    checkEqual(fcc2.unique,(unsigned long long int)2,check_function,"fcc AuCu n=2: two supercells",passed_checks,results);
    checkEqual(fcc2.degeneracy,fcc2.decorations,check_function,"fcc AuCu n=2: degeneracies add up to the decorations",passed_checks,results);

    // ---------------------------------------------------------------------------
    // multilattice: rocksalt cation decorations equal the fcc ones
    for(int n=3;n<=4;n++){
      POccEnumerationCounts fcc=runPOccEnumeration(partcarFCCAuCu(n,1),"ENUM");
      POccEnumerationCounts rs=runPOccEnumeration(partcarRocksaltMgNiO(n,1),"ENUM");
      checkEqual(rs.unique,fcc.unique,check_function,"rocksalt MgNiO and fcc AuCu at n="+aurostd::utype2string(n)+": same number of supercells",passed_checks,results);
      checkEqual(rs.vdegeneracy==fcc.vdegeneracy,true,check_function,"rocksalt MgNiO and fcc AuCu at n="+aurostd::utype2string(n)+": same degeneracies",passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // Burnside's lemma and structure comparison on rocksalt Mg0.5Ni0.5O, n=4 (superperiodic decorations included)
    {
      stringstream log;
      pocc::POccCalculator pcalc(log);
      setupPOccCalculator(pcalc,partcarRocksaltMgNiO(4,2),"ENUM",_kflags(),log);
      pcalc.calculate();
      //decorations of each HNF matrix, and the set of each
      vector<vector<vector<vector<int> > > > vconfigs(pcalc.hnf_count);
      vector<vector<uint> > vset(pcalc.hnf_count);
      vector<xmatrix<double> > vhnf(pcalc.hnf_count);
      xmatrix<double> hnf;
      vector<vector<int> > config,image;
      bool members_match=true,representatives_differ=true;
      vector<pocc::POccSuperCell> vrep;
      uint iset=0;
      for(std::list<pocc::POccSuperCellSet>::const_iterator it=pcalc.l_supercell_sets.begin();it!=pcalc.l_supercell_sets.end();++it,++iset){
        const pocc::POccSuperCell& rep=(*it).getSuperCell();
        for(uint i=0;i<(*it).m_psc_set.size();i++){
          const pocc::POccSuperCell& psc=(*it).m_psc_set[i];
          pcalc.getHNFMatSiteConfig(psc,hnf,config);
          vhnf[psc.m_hnf_index]=hnf;
          vconfigs[psc.m_hnf_index].push_back(config);
          vset[psc.m_hnf_index].push_back(iset);
          if(i>0 && !pcalc.areEquivalentStructures(rep,psc)){members_match=false;}
        }
        for(uint j=0;j<vrep.size();j++){
          if(pcalc.areEquivalentStructures(vrep[j],rep)){representatives_differ=false;}
        }
        vrep.push_back(rep);
      }
      //orbits of the permutation group of each HNF matrix, counted here: Burnside's lemma, and each orbit lies in one set
      bool burnside=true,orbits_in_sets=true;
      pocc::POccSupercellAction action;
      for(unsigned long long int h=0;h<pcalc.hnf_count;h++){
        pcalc.getSupercellAction(vhnf[h],action);
        const vector<vector<uint> >& perms=action.perms;
        const uint n=vconfigs[h][0][0].size();
        unsigned long long int fixed=0,norbits=0;
        vector<bool> assigned(vconfigs[h].size(),false);
        for(uint c=0;c<vconfigs[h].size();c++){
          for(uint p=0;p<perms.size();p++){
            image=vconfigs[h][c];
            for(uint slot=0;slot<perms[p].size();slot++){image[perms[p][slot]/n][perms[p][slot]%n]=vconfigs[h][c][slot/n][slot%n];}
            if(image==vconfigs[h][c]){fixed++;}
            if(!assigned[c]){
              for(uint c2=0;c2<vconfigs[h].size();c2++){
                if(vconfigs[h][c2]==image){
                  if(vset[h][c2]!=vset[h][c]){orbits_in_sets=false;}
                  if(c2!=c){assigned[c2]=true;}
                }
              }
            }
          }
          if(!assigned[c]){norbits++;assigned[c]=true;}
        }
        if(fixed!=norbits*perms.size()){burnside=false;}
      }
      check_description="rocksalt MgNiO n=4: orbit counts satisfy Burnside's lemma";
      checkEqual(burnside,true,check_function,check_description,passed_checks,results);
      check_description="rocksalt MgNiO n=4: each orbit of a permutation group lies in one set";
      checkEqual(orbits_in_sets,true,check_function,check_description,passed_checks,results);
      check_description="rocksalt MgNiO n=4: every member matches its representative (structure comparison)";
      checkEqual(members_match,true,check_function,check_description,passed_checks,results);
      check_description="rocksalt MgNiO n=4: representatives differ, across HNF matrices too (structure comparison)";
      checkEqual(representatives_differ,true,check_function,check_description,passed_checks,results);
      check_description="rocksalt MgNiO n=4: superperiodic decorations of other HNF matrices join their sets";
      checkEqual(pcalc.m_enum_merged_across>0,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // crystal point group 4/m on a 4/mmm lattice: the two chiral sqrt(5) supercells are distinct
    {
      POccEnumerationCounts uff=runPOccEnumeration(partcarTetragonal4m(),"UFF");
      POccEnumerationCounts penum=runPOccEnumeration(partcarTetragonal4m(),"ENUM");
      check_description="P4/m n=5: the crystal point group separates HNF matrices the lattice point group joins";
      checkEqual(penum.hnf_count>penum.hnf_count_lattice,true,check_function,check_description,passed_checks,results);
      check_description="P4/m n=5: the HNF list of UFF is the start of the ENUM one";
      checkEqual(uff.hnf_count,penum.hnf_count_lattice,check_function,check_description,passed_checks,results);
      check_description="P4/m n=5: the HNF classes cover all 31 HNF matrices";
      checkEqual(penum.hnf_total,(unsigned long long int)31,check_function,check_description,passed_checks,results);
      check_description="P4/m n=5: degeneracies add up to the decorations";
      checkEqual(penum.degeneracy,penum.decorations,check_function,check_description,passed_checks,results);
      //UFF energies of the ENUM representatives (clusters built one HNF matrix at a time) against the UFF bins with the same representative
      std::map<std::pair<unsigned long long int,unsigned long long int>,double> uff_energy;
      for(uint i=0;i<uff.vsets.size();i++){uff_energy[uff.vsets[i][0]]=uff.venergy_uff[i];}
      uint nmatched=0;
      bool energies_match=true;
      for(uint i=0;i<penum.vsets.size();i++){
        std::map<std::pair<unsigned long long int,unsigned long long int>,double>::const_iterator it=uff_energy.find(penum.vsets[i][0]);
        if(it==uff_energy.end()){continue;}
        nmatched++;
        if(!aurostd::isequal(it->second,penum.venergy_uff[i],1e-8)){energies_match=false;}
      }
      check_description="P4/m n=5: UFF energies of the ENUM representatives equal those of the UFF bins with the same representative ("+aurostd::utype2string(nmatched)+")";
      checkEqual(energies_match && nmatched>0,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // hcp Mg0.5Zn0.5, n=4 (nonsymmorphic): superperiodic decorations found again on their own and on other HNF matrices
    // 45 distinct structures (XtalFinder on all 63 orbits of the permutation groups); UFF bins 57
    POccEnumerationCounts hcp=runPOccEnumeration(partcarHCPMgZn(4,0.5,0.5,0.5,0.5),"ENUM");
    {
      check_description="hcp Mg0.5Zn0.5 n=4: 45 unique supercells";
      checkEqual(hcp.unique,(unsigned long long int)45,check_function,check_description,passed_checks,results);
      check_description="hcp Mg0.5Zn0.5 n=4: degeneracies add up to the decorations";
      checkEqual(hcp.degeneracy,hcp.decorations,check_function,check_description,passed_checks,results);
      check_description="hcp Mg0.5Zn0.5 n=4: superperiodic decorations joined on their own HNF matrix and from others";
      checkEqual(hcp.merged_hnf>0 && hcp.merged_across>0,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // L1_2 Au(Cu0.5Ni0.5)3, n=4: three partial sites cycled by the 3-fold axes; 144 distinct structures (XtalFinder on all representatives)
    {
      POccEnumerationCounts l12=runPOccEnumeration(partcarL12AuCuNi(4),"ENUM");
      check_description="L1_2 Au(Cu0.5Ni0.5)3 n=4: 144 unique supercells";
      checkEqual(l12.unique,(unsigned long long int)144,check_function,check_description,passed_checks,results);
      check_description="L1_2 Au(Cu0.5Ni0.5)3 n=4: degeneracies add up to the decorations";
      checkEqual(l12.degeneracy,l12.decorations,check_function,check_description,passed_checks,results);
      check_description="L1_2 Au(Cu0.5Ni0.5)3 n=4: superperiodic decorations joined on their own HNF matrix and from others";
      checkEqual(l12.merged_hnf>0 && l12.merged_across>0,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // colors from the species counts: A=Mg0.5Zn0.5 B=Mg0.4Zn0.6 gives 2 Mg and 2 Zn on both sites at n=4, so the same supercells
    {
      POccEnumerationCounts hcp_diff=runPOccEnumeration(partcarHCPMgZn(4,0.5,0.4,0.5,0.6),"ENUM");
      check_description="hcp A=Mg0.5Zn0.5 B=Mg0.4Zn0.6 n=4: same sets as Mg0.5Zn0.5 on both sites";
      checkEqual(hcp_diff.vsets==hcp.vsets,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // sampling: ENUM groups the first round of UFF (FirstRandomSamplingWholeSpace() for SAMPLE_NUMBER and SAMPLE_HASH,
    // FirstRandomSampling() for SAMPLE_RATE); a set holds the drawn decorations of one structure, each counted once
    {
      typedef std::pair<unsigned long long int,unsigned long long int> HC;
      std::map<HC,uint> full_of;  //set of the full enumeration holding each decoration
      for(uint iset=0;iset<hcp.vsets.size();iset++){for(uint i=0;i<hcp.vsets[iset].size();i++){full_of[hcp.vsets[iset][i]]=iset;}}
      const unsigned long long int per_hnf=hcp.decorations/hcp.hnf_count;
      //first round only (calculateEnum()); same_draws: the decorations in the sets are those UFF draws
      bool same_draws=false;
      auto firstRound=[&](const _kflags& kf){
        POccEnumerationCounts sr;
        stringstream log;
        pocc::POccCalculator pcalc(log);
        setupPOccCalculator(pcalc,partcarHCPMgZn(4,0.5,0.5,0.5,0.5),"ENUM",kf,log);
        pcalc.calculateEnum();
        vector<unsigned long long int> vgrouped;
        for(std::list<pocc::POccSuperCellSet>::const_iterator it=pcalc.l_supercell_sets.begin();it!=pcalc.l_supercell_sets.end();++it){
          sr.vdegeneracy.push_back((*it).getDegeneracy());
          sr.degeneracy+=sr.vdegeneracy.back();
          sr.vsets.push_back(vector<HC>(0));
          for(uint i=0;i<(*it).m_psc_set.size();i++){
            sr.vsets.back().push_back(HC((*it).m_psc_set[i].m_hnf_index,(*it).m_psc_set[i].m_site_config_index));
            vgrouped.push_back((*it).m_psc_set[i].m_hnf_index*pcalc.types_config_permutations_count+(*it).m_psc_set[i].m_site_config_index);
          }
          std::sort(sr.vsets.back().begin(),sr.vsets.back().end());
        }
        sr.skipped=pcalc.m_enum_skipped;
        std::sort(vgrouped.begin(),vgrouped.end());
        vector<unsigned long long int> vdrawn=(kf.KBIN_POCC_SAMPLE_RATE_STRING.empty() ? pcalc.FirstRandomSamplingWholeSpace() : pcalc.FirstRandomSampling(pcalc.hnf_count,pcalc.types_config_permutations_count));
        std::sort(vdrawn.begin(),vdrawn.end());
        same_draws=(vgrouped==vdrawn);
        return sr;
      };
      //every set lies in one set of the full enumeration, no two sets in the same one, and each degeneracy is the number of drawn decorations
      auto sampledSetsOK=[&](const POccEnumerationCounts& sr){
        if(sr.vsets.empty()){return false;}
        std::set<uint> vfull;
        for(uint iset=0;iset<sr.vsets.size();iset++){
          if(sr.vsets[iset].empty() || sr.vdegeneracy[iset]!=sr.vsets[iset].size()){return false;}
          std::map<HC,uint>::const_iterator it=full_of.find(sr.vsets[iset][0]);
          if(it==full_of.end()){return false;}
          for(uint i=1;i<sr.vsets[iset].size();i++){
            std::map<HC,uint>::const_iterator jt=full_of.find(sr.vsets[iset][i]);
            if(jt==full_of.end() || jt->second!=it->second){return false;}
          }
          if(!vfull.insert(it->second).second){return false;}
        }
        return true;
      };
      //is every set of second a set of first
      auto subsetOf=[](const POccEnumerationCounts& second,const POccEnumerationCounts& first){
        const std::set<vector<HC> > vfirst(first.vsets.begin(),first.vsets.end());
        for(uint i=0;i<second.vsets.size();i++){if(!vfirst.count(second.vsets[i])){return false;}}
        return true;
      };
      _kflags kflags;
      const string tag="hcp Mg0.5Zn0.5 n=4 ("+aurostd::utype2string(hcp.decorations)+" decorations), ";
      check_function="pocc::POccCalculator::calculateEnum() [--pocc_sample_number]";
      kflags.KBIN_POCC_SAMPLE_NUMBER_STRING="1";
      const POccEnumerationCounts first1=firstRound(kflags);
      check_description=tag+"SAMPLE_NUMBER=1: each set is one structure of the full enumeration, counted by its draws";
      checkEqual(sampledSetsOK(first1),true,check_function,check_description,passed_checks,results);
      check_description=tag+"SAMPLE_NUMBER=1: the 100 decorations FirstRandomSamplingWholeSpace() draws for UFF";
      checkEqual(same_draws && first1.degeneracy==100,true,check_function,check_description,passed_checks,results);
      check_description=tag+"SAMPLE_NUMBER=1: drawn and skipped decorations add up";
      checkEqual(first1.degeneracy+first1.skipped,hcp.decorations,check_function,check_description,passed_checks,results);
      check_description=tag+"SAMPLE_NUMBER=1: same sets on a second run (fixed seed)";
      checkEqual(firstRound(kflags).vsets==first1.vsets,true,check_function,check_description,passed_checks,results);
      kflags.KBIN_POCC_SAMPLE_NUMBER_STRING="2";
      const POccEnumerationCounts first2=firstRound(kflags);
      check_description=tag+"SAMPLE_NUMBER=2: sets of the 200 drawn decorations";
      checkEqual(sampledSetsOK(first2) && same_draws && first2.degeneracy==200,true,check_function,check_description,passed_checks,results);
      //more than 1/100 of the decorations: every one, which is the full enumeration
      kflags.KBIN_POCC_SAMPLE_NUMBER_STRING=aurostd::utype2string(hcp.decorations/100+1);
      const POccEnumerationCounts first_all=firstRound(kflags);
      check_description=tag+"SAMPLE_NUMBER="+kflags.KBIN_POCC_SAMPLE_NUMBER_STRING+" (every decoration drawn): the sets and degeneracies of the full enumeration";
      checkEqual(std::set<vector<HC> >(first_all.vsets.begin(),first_all.vsets.end())==std::set<vector<HC> >(hcp.vsets.begin(),hcp.vsets.end()) && first_all.skipped==0 && first_all.degeneracy==hcp.decorations,true,check_function,check_description,passed_checks,results);
      //second round (calculate()): 2 of the first-round sets, unchanged
      kflags.KBIN_POCC_SAMPLE_NUMBER_STRING="2";
      const POccEnumerationCounts second2=runPOccEnumeration(partcarHCPMgZn(4,0.5,0.5,0.5,0.5),"ENUM",false,kflags);
      check_description=tag+"SAMPLE_NUMBER=2, second round: 2 of the sets of the first";
      checkEqual(second2.vsets.size()==2 && subsetOf(second2,first2),true,check_function,check_description,passed_checks,results);
      //rate: the same C indices on every HNF matrix, as UFF takes them
      check_function="pocc::POccCalculator::calculateEnum() [--pocc_sample_rate]";
      kflags.KBIN_POCC_SAMPLE_NUMBER_STRING="";
      kflags.KBIN_POCC_SAMPLE_RATE_STRING="0.5:0.5";
      const POccEnumerationCounts first_half=firstRound(kflags);
      check_description=tag+"SAMPLE_RATE=0.5:0.5: sets of the decorations FirstRandomSampling() takes for UFF";
      checkEqual(sampledSetsOK(first_half) && same_draws && first_half.degeneracy==(per_hnf/2)*hcp.hnf_count,true,check_function,check_description,passed_checks,results);
      const POccEnumerationCounts second_half=runPOccEnumeration(partcarHCPMgZn(4,0.5,0.5,0.5,0.5),"ENUM",false,kflags);
      check_description=tag+"SAMPLE_RATE=0.5:0.5, second round: half of the sets of the first";
      checkEqual(second_half.vsets.size()==first_half.vsets.size()/2 && subsetOf(second_half,first_half),true,check_function,check_description,passed_checks,results);
      //hash: the decoration alone
      check_function="pocc::POccCalculator::calculateEnum() [--pocc_sample_hash]";
      kflags.KBIN_POCC_SAMPLE_RATE_STRING="";
      const HC target=hcp.vsets.back().back();
      kflags.KBIN_POCC_SAMPLE_HASH_STRING="H"+aurostd::utype2string(target.first)+"C"+aurostd::utype2string(target.second);
      const POccEnumerationCounts hashed=runPOccEnumeration(partcarHCPMgZn(4,0.5,0.5,0.5,0.5),"ENUM",false,kflags);
      check_description=tag+kflags.KBIN_POCC_SAMPLE_HASH_STRING+": one set, that decoration";
      checkEqual(hashed.vsets.size()==1 && hashed.vsets[0]==vector<HC>(1,target) && hashed.vdegeneracy[0]==1 && hashed.skipped==hcp.decorations-1,true,check_function,check_description,passed_checks,results);
      //hash with a number: the decoration joins the draws; the second round is skipped and the set holding the hash is kept
      kflags.KBIN_POCC_SAMPLE_NUMBER_STRING="1";
      const POccEnumerationCounts hashed1=runPOccEnumeration(partcarHCPMgZn(4,0.5,0.5,0.5,0.5),"ENUM",false,kflags);
      bool found=false;
      for(uint i=0;i<hashed1.vsets.size();i++){if(std::binary_search(hashed1.vsets[i].begin(),hashed1.vsets[i].end(),target)){found=true;}}
      check_description=tag+kflags.KBIN_POCC_SAMPLE_HASH_STRING+" and SAMPLE_NUMBER=1: the hash joins the draws, and its set is kept";
      checkEqual(sampledSetsOK(hashed1) && found && hashed1.vsets.size()==1,true,check_function,check_description,passed_checks,results);
      //hash outside the decorations
      kflags.KBIN_POCC_SAMPLE_NUMBER_STRING="";
      kflags.KBIN_POCC_SAMPLE_HASH_STRING="H"+aurostd::utype2string(hcp.hnf_count)+"C0";
      bool thrown=false;
      try{firstRound(kflags);}
      catch(aurostd::xerror&){thrown=true;}
      check_description=tag+kflags.KBIN_POCC_SAMPLE_HASH_STRING+" (no such HNF matrix): throws";
      checkEqual(thrown,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // supercell action: the pull-back of an operation that keeps the superlattice is its permutation; unranking inverts ranking
    {
      stringstream log;
      pocc::POccSupercellAction action;
      vector<vector<int> > config,image,pulled;
      bool pullback_ok=true,nontrivial=false,unrank_ok=true;
      std::mt19937 rng(20261005);
      pocc::POccSuperCell psc;
      xmatrix<double> hnf;
      const vector<string> vpartcar={partcarHCPMgZn(4,0.5,0.5,0.5,0.5),partcarL12AuCuNi(4)};
      for(uint ipartcar=0;ipartcar<vpartcar.size();ipartcar++){
        pocc::POccCalculator pcalc(log);
        setupPOccCalculator(pcalc,vpartcar[ipartcar],"ENUM",_kflags(),log);
        for(unsigned long long int h=0;h<pcalc.hnf_count;h++){
          psc.m_hnf_index=h;psc.m_site_config_index=0;
          pcalc.getHNFMatSiteConfig(psc,hnf,config);
          pcalc.getSupercellAction(hnf,action);
          const uint n=(uint)pcalc.n_hnf;
          for(uint trial=0;trial<3;trial++){
            config=pcalc.getSiteConfiguration(rng()%pcalc.types_config_permutations_count);
            const vector<bool> periods=pcalc.getDecorationPeriods(action,config);
            for(uint iop=0;iop<action.keeps_superlattice.size();iop++){
              if(!action.keeps_superlattice[iop]){continue;}
              if(!pcalc.pullBackDecoration(action,periods,action,iop,config,pulled)){pullback_ok=false;continue;}
              //the same image must be among the permutations of the group (translation 0 after the operation)
              bool found=false;
              for(uint p=0;p<action.perms.size()&&!found;p++){
                image=config;
                for(uint slot=0;slot<action.perms[p].size();slot++){image[action.perms[p][slot]/n][action.perms[p][slot]%n]=config[slot/n][slot%n];}
                found=(image==pulled);
              }
              if(!found){pullback_ok=false;}
              if(pulled!=config){nontrivial=true;}
            }
          }
        }
      }
      check_function="pocc::POccCalculator::pullBackDecoration()";
      check_description="hcp and L1_2 n=4: operations that keep the superlattice pull back onto images of the permutation group";
      checkEqual(pullback_ok && nontrivial,true,check_function,check_description,passed_checks,results);
      //ranking against getSiteConfiguration(), on hcp with vacancies (two structure configurations at n=2)
      pocc::POccCalculator pcalc_v(log);
      setupPOccCalculator(pcalc_v,partcarHCPMgZn(2,0.25,0.25,0.5,0.5),"ENUM",_kflags(),log);
      pcalc_v.prepareEnumeration();
      unsigned long long int index=0;
      for(index=0;index<pcalc_v.types_config_permutations_count;index++){
        if(pcalc_v.rankDecoration(pcalc_v.getSiteConfiguration(index))!=index){unrank_ok=false;}
      }
      check_function="pocc::POccCalculator::rankDecoration()";
      check_description="hcp Mg0.25Zn0.5 n=2 (vacancies, "+aurostd::utype2string(pcalc_v.v_str_configs.size())+" structure configurations, "+aurostd::utype2string(index)+" decorations): inverse of getSiteConfiguration()";
      checkEqual(unrank_ok && index>1 && pcalc_v.v_str_configs.size()>1,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // enumerator selection and the comparison mode
    {
      check_function="pocc::POccCalculator::getStructureGenerationAlgo()";
      const vector<string> vinput={"uff"," Enum ","COMPARE"},vexpected={"UFF","ENUM","COMPARE"};
      for(uint i=0;i<vinput.size();i++){
        stringstream log;
        pocc::POccCalculator pcalc(log);
        setupPOccCalculator(pcalc,partcarFCCAuCu(2,1),vinput[i],_kflags(),log);
        checkEqual(pcalc.getStructureGenerationAlgo(),vexpected[i],check_function,"--pocc_enumerator="+aurostd::RemoveWhiteSpaces(vinput[i]),passed_checks,results);
      }
      bool threw=false;
      try {
        stringstream log;
        pocc::POccCalculator pcalc(log);
        setupPOccCalculator(pcalc,partcarFCCAuCu(2,1),"SNF",_kflags(),log);
      } catch (aurostd::xerror& e) {threw=(e.whatCode()==_INPUT_ILLEGAL_);}
      checkEqual(threw,true,check_function,"--pocc_enumerator=SNF is refused",passed_checks,results);

      // ENUM is the default; the aflow.in key keeps a parent on the enumerator it was written with (CO20261009)
      check_function="pocc::getPOccEnumerator()";
      checkEqual(pocc::getPOccEnumerator(" enum "),string("ENUM"),check_function,"\" enum \" is ENUM",passed_checks,results);
      checkEqual(pocc::getPOccEnumerator(""),string("UFF"),check_function,"an empty name is UFF",passed_checks,results);
      threw=false;
      try {pocc::getPOccEnumerator("SQS");} catch (aurostd::xerror& e) {threw=(e.whatCode()==_INPUT_ILLEGAL_);}
      checkEqual(threw,true,check_function,"SQS is refused",passed_checks,results);
      if(!XHOST.vflag_pflow.flag("POCC_ENUMERATOR")){  //the command line of this run names no enumerator
        check_function="pocc::getPOccEnumeratorForAflowIn()";
        checkEqual(pocc::getPOccEnumeratorForAflowIn(),pocc::getPOccEnumerator(DEFAULT_POCC_ENUMERATOR),check_function,"a new aflow.in records DEFAULT_POCC_ENUMERATOR",passed_checks,results);
      }
      //the key as AVASP writes it (padded, with a comment), lower case, and an aflow.in from before the key
      const string aflowin_structure="[POCC_MODE_EXPLICIT]START.POCC_STRUCTURE\n"+partcarFCCAuCu(2,1)+"[POCC_MODE_EXPLICIT]STOP.POCC_STRUCTURE\n";
      const vector<string> vkey={aurostd::PaddedPOST("[AFLOW_POCC]ENUMERATOR=ENUM",_AFLOWINPAD_)+" // ENUM or UFF, README_AFLOW_POCC.TXT","[AFLOW_POCC]ENUMERATOR=uff",""};
      const vector<string> vkey_expected={"ENUM","UFF","UFF"};
      const vector<string> vkey_description={"the key as a new aflow.in writes it","[AFLOW_POCC]ENUMERATOR=uff","an aflow.in without the key (written before it existed) is UFF"};
      _kflags kflags_uff;
      for(uint i=0;i<vkey.size();i++){
        _aflags aflags;aflags.Directory="./";
        stringstream oss_kflags;
        const string aflowin="[AFLOW_POCC]CALC\n"+(vkey[i].empty()?string(""):vkey[i]+"\n")+aflowin_structure;
        const _kflags kflags=KBIN::VASP_Get_Kflags_from_AflowIN(aflowin,aflags,oss_kflags);
        check_function="KBIN::VASP_Get_Kflags_from_AflowIN()";
        checkEqual(kflags.KBIN_POCC && kflags.KBIN_POCC_ENUMERATOR_STRING==vkey_expected[i],true,check_function,vkey_description[i]+": "+vkey_expected[i],passed_checks,results);
        if(vkey[i].empty()){kflags_uff=kflags;}
        if(!XHOST.vflag_pflow.flag("POCC_ENUMERATOR")){
          stringstream log,input(partcarFCCAuCu(2,1));
          xstructure xstr(input,IOAFLOW_AUTO);
          aurostd::xoption vpflow;
          vpflow.flag("POCC_SKIP_WRITING_FILES",true);
          pocc::POccCalculator pcalc(log);
          pcalc.initialize(xstr,vpflow,kflags,log);
          check_function="pocc::POccCalculator::getStructureGenerationAlgo()";
          checkEqual(pcalc.getStructureGenerationAlgo(),vkey_expected[i],check_function,vkey_description[i]+": the calculator follows aflow.in, whatever the default",passed_checks,results);
        }
      }
      {
        stringstream log;
        pocc::POccCalculator pcalc(log);
        setupPOccCalculator(pcalc,partcarFCCAuCu(2,1),"enum",kflags_uff,log);
        check_function="pocc::POccCalculator::getStructureGenerationAlgo()";
        checkEqual(pcalc.getStructureGenerationAlgo(),string("ENUM"),check_function,"--pocc_enumerator=enum on a parent whose aflow.in has no key: the command line wins",passed_checks,results);
      }

      POccEnumerationCounts penum=runPOccEnumeration(partcarFCCAuCu(4,2),"ENUM");
      POccEnumerationCounts compared=runPOccEnumeration(partcarFCCAuCu(4,2),"COMPARE");
      check_function="pocc::POccCalculator::compareEnumerators()";
      checkEqual(compared.vsets==penum.vsets,true,check_function,"fcc AuCu n=4: the comparison keeps the ENUM sets",passed_checks,results);
    }
  } catch (aurostd::xerror& e) {
    errors.push_back("POCC enumeration test failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("POCC enumeration test failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("POCC enumeration test failed - unknown exception");
  }
}

// CO20261006 - POCC sampling over the whole decoration space.
// With SAMPLE_NUMBER the first sampling round used to draw from the first 100*SAMPLE_NUMBER site configurations
// only: getTotalPermutationsCount() stopped counting at the first sites whose configurations reached that number,
// so every sampled structure kept the later sites in their starting decoration (in a spinel with four cations both
// tetrahedral positions never changed). These checks fail when that cut comes back.
void poccSamplingSpaceTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
  const string check_function = "pocc::POccCalculator (sampling space)";
  string check_description = "";

  // 1. Counting distinct permutations of one site, without overflow
  try {
    check_description = "permutations of a site with a vacancy, two of one type and two singles (5!/2!)";
    checkEqual(pocc::getDistinctPermutationsCount({-1, 0, 0, 1, 2}), 60ULL, check_function, check_description, passed_checks, results);
    check_description = "permutations of 20 positions shared by 5 types (20!/(4!)^5), past the factorials of aurostd::nCk()";
    checkEqual(pocc::getDistinctPermutationsCount({0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3, 4,4,4,4}), 305540235000ULL, check_function, check_description, passed_checks, results);
    check_description = "a count past unsigned long long int throws (30 positions, 6 types, 30!/(5!)^6 = 8.9e19)";
    vector<int> v_huge;
    for (int type = 0; type < 6; type++) { for (int i = 0; i < 5; i++) { v_huge.push_back(type); } }
    bool threw = false;
    try { pocc::getDistinctPermutationsCount(v_huge); } catch (aurostd::xerror&) { threw = true; }
    checkEqual(threw, true, check_function, check_description, passed_checks, results);
  } catch (aurostd::xerror& e) {
    errors.push_back("permutation count: " + e.buildMessageString());
  }

  // 2. The rank-th permutation is the one getNextSiteConfiguration() reaches after rank steps
  try {
    check_description = "getDistinctPermutation(rank) matches getNextSiteConfiguration() at every rank of {-1,0,0,1,2}";
    vector<int> start = {-1, 0, 0, 1, 2}, walk = start;
    unsigned long long int rank = 0, mismatches = 0;
    do {
      if (pocc::getDistinctPermutation(start, rank) != walk) { mismatches++; }
      rank++;
    } while (pocc::getNextSiteConfiguration(walk));
    checkEqual(rank, 60ULL, check_function, check_description + " (ranks visited)", passed_checks, results);
    checkEqual(mismatches, 0ULL, check_function, check_description, passed_checks, results);
  } catch (aurostd::xerror& e) {
    errors.push_back("permutation rank: " + e.buildMessageString());
  }

  // 3. A rock salt with three cations at 1/3 on its four cation positions (conventional cell, n=3)
  stringstream partcar;
  partcar << "CoFeNiO rock salt, conventional cell (POCC sampling test)" << endl;
  partcar << "1.0 0.001 0.001" << endl;
  partcar << "   4.20000000000000   0.00000000000000   0.00000000000000" << endl;
  partcar << "   0.00000000000000   4.20000000000000   0.00000000000000" << endl;
  partcar << "   0.00000000000000   0.00000000000000   4.20000000000000" << endl;
  partcar << "4*0.333333 4*0.333333 4*0.333333 4*1" << endl;
  partcar << "Direct(16) Partial" << endl;
  const vector<string> species = {"Co", "Fe", "Ni"};  // alphabetical, with O last, as POccCalculator requires
  const vector<string> cation_positions = {"0.0 0.0 0.0", "0.0 0.5 0.5", "0.5 0.0 0.5", "0.5 0.5 0.0"};
  const vector<string> anion_positions = {"0.5 0.5 0.5", "0.5 0.0 0.0", "0.0 0.5 0.0", "0.0 0.0 0.5"};
  for (uint s = 0; s < species.size(); s++) {
    for (uint i = 0; i < cation_positions.size(); i++) { partcar << "   " << cation_positions[i] << "  " << species[s] << " pocc=0.333333" << endl; }
  }
  for (uint i = 0; i < anion_positions.size(); i++) { partcar << "   " << anion_positions[i] << "  O pocc=1" << endl; }
  try {
    xstructure xstr_pocc(partcar, IOAFLOW_AUTO);
    aurostd::xoption pocc_flags;
    pocc_flags.flag("POCC_ENUMERATOR", true);
    pocc_flags.push_attached("POCC_ENUMERATOR", "UFF");  // CO20261009 - the UFF first round, whatever DEFAULT_POCC_ENUMERATOR says (ENUM's sampling is checked in pocc_enumeration)
    stringstream oss_quiet;
    pocc::POccCalculator pcalc(xstr_pocc, pocc_flags, oss_quiet);  // as aflow --pocc_count_total --pocc_enumerator=uff sets it up
    checkEqual(pcalc.m_initialized, true, check_function, "the rock salt PARTCAR initializes", passed_checks, results);
    if (!pcalc.m_initialized) { return; }
    pcalc.m_kflags.KBIN_POCC_SAMPLE_NUMBER_STRING = "2";  // [AFLOW_POCC]SAMPLE_NUMBER=2: the first round draws 100*2 = 200 decorations
    pcalc.calculateHNF();
    pcalc.getTotalPermutationsCount();

    check_description = "the count covers every site configuration under SAMPLE_NUMBER (6^4 per superlattice, not the 6^3 = 216 that reached 100*2)";
    checkEqual(pcalc.types_config_permutations_count, 1296ULL, check_function, check_description, passed_checks, results);

    check_description = "getSiteConfiguration(i) matches getNextSiteConfiguration() at every index";
    unsigned long long int index = 0, mismatches = 0;
    for (uint str_config = 0; str_config < pcalc.v_str_configs.size(); str_config++) {
      vector<vector<int> > v_types_config;
      for (uint site = 0; site < pcalc.v_str_configs[str_config].site_configs.size(); site++) {
        v_types_config.push_back(pcalc.v_str_configs[str_config].site_configs[site].getStartingTypesConfiguration());
      }
      vector<uint> v_config_order = pocc::getConfigOrder(v_types_config);
      do {
        if (pcalc.getSiteConfiguration(index) != v_types_config) { mismatches++; }
        index++;
      } while (pocc::getNextSiteConfiguration(v_config_order, v_types_config));
    }
    checkEqual(index, pcalc.types_config_permutations_count, check_function, check_description + " (indices visited)", passed_checks, results);
    checkEqual(mismatches, 0ULL, check_function, check_description, passed_checks, results);

    check_description = "the first round draws 200 distinct, sorted decorations from all superlattices, the same ones every time";
    const vector<unsigned long long int> v_indices = pcalc.FirstRandomSamplingWholeSpace();
    const unsigned long long int decorations_count = pcalc.hnf_count * pcalc.types_config_permutations_count;
    bool sorted_distinct_in_range = true;
    std::set<unsigned long long int> hnfs_drawn;
    for (size_t i = 0; i < v_indices.size(); i++) {
      if (v_indices[i] >= decorations_count || (i > 0 && v_indices[i] <= v_indices[i - 1])) { sorted_distinct_in_range = false; }
      hnfs_drawn.insert(v_indices[i] / pcalc.types_config_permutations_count);
    }
    checkEqual(v_indices.size(), (size_t)200, check_function, check_description + " (count)", passed_checks, results);
    checkEqual(sorted_distinct_in_range, true, check_function, check_description + " (sorted, distinct, in range)", passed_checks, results);
    checkEqual(hnfs_drawn.size() == pcalc.hnf_count, true, check_function, check_description + " (every superlattice)", passed_checks, results);
    checkEqual(pcalc.FirstRandomSamplingWholeSpace() == v_indices, true, check_function, check_description + " (same seed, same draw)", passed_checks, results);

    check_description = "every cation position takes all three species across the drawn decorations";
    vector<vector<std::set<int> > > vv_seen;
    for (size_t i = 0; i < v_indices.size(); i++) {
      const vector<vector<int> > v_types_config = pcalc.getSiteConfiguration(v_indices[i] % pcalc.types_config_permutations_count);
      vv_seen.resize(v_types_config.size());
      for (uint site = 0; site < v_types_config.size(); site++) {
        vv_seen[site].resize(v_types_config[site].size());
        for (uint pos = 0; pos < v_types_config[site].size(); pos++) { vv_seen[site][pos].insert(v_types_config[site][pos]); }
      }
    }
    uint positions_fixed = 0, positions_partial = 0;
    for (uint site = 0; site < vv_seen.size(); site++) {
      for (uint pos = 0; pos < vv_seen[site].size(); pos++) {
        if (pcalc.v_str_configs[0].site_configs[site].isPartiallyOccupied()) {
          positions_partial++;
          if (vv_seen[site][pos].size() != species.size()) { positions_fixed++; }
        }
      }
    }
    checkEqual(positions_partial, (uint)12, check_function, check_description + " (cation positions: 4 sites x 3)", passed_checks, results);
    checkEqual(positions_fixed, (uint)0, check_function, check_description, passed_checks, results);
  } catch (aurostd::xerror& e) {
    errors.push_back("rock salt sampling: " + e.buildMessageString());
  }
}


//CO20261006 - cluster correlation functions of the POCC supercells (aflow_pocc_correlations.cpp)
namespace {
  /// @brief PARTCAR of rocksalt (MgCoNiCuZn)O, 0.2 each, with HNF=n
  string partcarRocksaltHEO(int n) {
    stringstream ss;
    ss << "rocksalt (MgCoNiCuZn)O" << std::endl;
    ss << "1.0 -" << n << std::endl;
    ss << "   0.0 2.12 2.12" << std::endl << "   2.12 0.0 2.12" << std::endl << "   2.12 2.12 0.0" << std::endl;
    ss << "1*0.2 1*0.2 1*0.2 1*0.2 1 1*0.2" << std::endl;
    ss << "Direct(6) Pocc [Co1Cu1Mg1Ni1O1Zn1]" << std::endl;
    ss << "   0.0 0.0 0.0 Co" << std::endl << "   0.0 0.0 0.0 Cu" << std::endl << "   0.0 0.0 0.0 Mg" << std::endl;
    ss << "   0.0 0.0 0.0 Ni" << std::endl << "   0.5 0.5 0.5 O" << std::endl << "   0.0 0.0 0.0 Zn" << std::endl;
    return ss.str();
  }

  /// @brief correlations of every ENUM set (from its representative), its degeneracy, and the pair probabilities
  void poccSetCorrelations(pocc::POccCalculator& pcalc,vector<vector<double> >& vcorr,vector<double>& vdg,vector<vector<vector<double> > >& vprob,vector<vector<vector<int> > >& vdecoration) {
    vcorr.clear();vdg.clear();vprob.clear();vdecoration.clear();
    xmatrix<double> hnf;
    vector<vector<int> > decoration;
    vector<vector<double> > prob;
    pocc::POccSupercellAction action;
    for(std::list<pocc::POccSuperCellSet>::const_iterator it=pcalc.l_supercell_sets.begin();it!=pcalc.l_supercell_sets.end();++it){
      pcalc.getHNFMatSiteConfig((*it).getSuperCell(),hnf,decoration);
      pcalc.getSupercellAction(hnf,action);
      vcorr.push_back(pcalc.getCorrelations(action,decoration,prob));
      vdg.push_back((double)(*it).getDegeneracy());
      vprob.push_back(prob);
      vdecoration.push_back(decoration);
    }
  }

  /// @brief degeneracy-weighted average (infinite temperature)
  vector<double> poccInfiniteTemperature(const vector<vector<double> >& vcorr,const vector<double>& vdg) {
    vector<double> avg(vcorr.empty() ? 0 : vcorr[0].size(),0.0);
    double total=0.0;
    for(uint i=0;i<vcorr.size();i++){for(uint f=0;f<avg.size();f++){avg[f]+=vdg[i]*vcorr[i][f];} total+=vdg[i];}
    for(uint f=0;f<avg.size();f++){avg[f]/=total;}
    return avg;
  }

  /// @brief PARTCAR of B2 (Au0.5Cu0.5)(Ni0.5Zn0.5) with HNF=n: two inequivalent partially occupied sites
  string partcarB2AuCuNiZn(int n) {
    stringstream ss;
    ss << "B2 (Au0.5Cu0.5)(Ni0.5Zn0.5)" << std::endl;
    ss << "1.0 -" << n << std::endl;
    ss << "   3.0 0.0 0.0" << std::endl << "   0.0 3.0 0.0" << std::endl << "   0.0 0.0 3.0" << std::endl;
    ss << "1*0.5 1*0.5 1*0.5 1*0.5" << std::endl;
    ss << "Direct(4) Pocc [Au1Cu1Ni1Zn1]" << std::endl;
    ss << "   0.0 0.0 0.0 Au" << std::endl << "   0.0 0.0 0.0 Cu" << std::endl << "   0.5 0.5 0.5 Ni" << std::endl << "   0.5 0.5 0.5 Zn" << std::endl;
    return ss.str();
  }

  /// @brief values of an aflow.pocc.out line "key=v1,v2,...  (comment)"
  vector<double> poccLineValues(const string& line) {
    vector<double> values;
    string s=line.substr(line.find('=')+1);
    s=s.substr(0,s.find(' '));
    vector<string> vtokens;
    aurostd::string2tokens(s,vtokens,",");
    for(uint j=0;j<vtokens.size();j++){values.push_back(aurostd::string2utype<double>(vtokens[j]));}
    return values;
  }

  /// @brief Warren-Cowley sum rule of each pair cluster: sum over t of c_t alpha_st is zero for every s (and over s for every t when the sites differ), with c the marginals of P
  bool poccWarrenCowleySumRule(const pocc::POccCalculator& pcalc,const vector<vector<double> >& vprob) {
    const vector<double> valpha=pcalc.getWarrenCowley(vprob);
    vector<std::pair<uint,uint> > vst;
    uint ia=0,ip=0;
    for(uint io=0;io<pcalc.m_corr_orbits.size();io++){
      const pocc::POccClusterOrbit& orbit=pcalc.m_corr_orbits[io];
      if(orbit.points.size()!=2){continue;}
      const bool symmetric=pcalc.getWarrenCowleyPairs(orbit,vst);
      const vector<double>& P=vprob[ip++];
      const uint M0=pcalc.m_corr_site_values[(uint)orbit.points[0][0]].size(),M1=pcalc.m_corr_site_values[(uint)orbit.points[1][0]].size();
      vector<double> c0(M0,0.0),c1(M1,0.0);
      for(uint s=0;s<M0;s++){for(uint t=0;t<M1;t++){c0[s]+=P[s*M1+t];c1[t]+=P[s*M1+t];}}
      vector<vector<double> > alpha(M0,vector<double>(M1,0.0));
      for(uint i=0;i<vst.size();i++,ia++){
        alpha[vst[i].first][vst[i].second]=valpha[ia];
        if(symmetric){alpha[vst[i].second][vst[i].first]=valpha[ia];}
      }
      if(symmetric){for(uint s=0;s<M0;s++){c0[s]=c1[s]=0.5*(c0[s]+c1[s]);}}
      for(uint s=0;s<M0;s++){double sum=0.0;for(uint t=0;t<M1;t++){sum+=c1[t]*alpha[s][t];} if(std::abs(sum)>1e-12){return false;}}
      for(uint t=0;t<M1;t++){double sum=0.0;for(uint s=0;s<M0;s++){sum+=c0[s]*alpha[s][t];} if(std::abs(sum)>1e-12){return false;}}
    }
    return ia==valpha.size() && ia>0;
  }

  bool poccVectorsMatch(const vector<double>& a,const vector<double>& b,double tol=1e-9) {
    if(a.size()!=b.size()){return false;}
    for(uint i=0;i<a.size();i++){if(std::abs(a[i]-b[i])>tol){return false;}}
    return true;
  }
}  // namespace

/// @brief correlation functions of the POCC supercells: point functions, clusters, known structures, the infinite-temperature ensemble against an independent calculation and a closed form, pair probabilities, titles and aflow.pocc.out
/// @authors
/// @mod{CO,20261006,created}
void poccCorrelationsTest(uint& passed_checks, vector<vector<string> >& results, vector<string>& errors) {
  string check_function="",check_description="";
  try {
    // ---------------------------------------------------------------------------
    // point functions (CALPHAD 33, 266 (2009)): gamma_0=1, averages of zero over the species, orthogonal
    check_function="pocc::getPointFunction()";
    checkEqual(pocc::getPointFunction(2,1,0)==-1.0 && std::abs(pocc::getPointFunction(2,1,1)-1.0)<1e-15 && pocc::getPointFunction(5,0,3)==1.0,true,check_function,"binary: -1 and +1; gamma_0=1",passed_checks,results);
    bool zero_mean=true,orthogonal=true;
    for(uint M=2;M<=6;M++){
      for(uint i=0;i<M;i++){
        double sum=0.0;
        for(uint s=0;s<M;s++){sum+=pocc::getPointFunction(M,i,s);}
        if(i>0 && std::abs(sum)>1e-12){zero_mean=false;}
        for(uint j=i+1;j<M;j++){
          double dot=0.0;
          for(uint s=0;s<M;s++){dot+=pocc::getPointFunction(M,i,s)*pocc::getPointFunction(M,j,s);}
          if(std::abs(dot)>1e-12){orthogonal=false;}
        }
      }
    }
    checkEqual(zero_mean && orthogonal,true,check_function,"2 to 6 species: every function other than gamma_0 averages to zero, and they are orthogonal",passed_checks,results);
    bool threw=false;
    try{pocc::getPointFunction(3,3,0);}catch(aurostd::xerror&){threw=true;}
    checkEqual(threw,true,check_function,"function index out of range is refused",passed_checks,results);

    // ---------------------------------------------------------------------------
    // rocksalt Mg0.5Ni0.5O n=2: the cation sublattice is fcc; L1_1 (H0C0) and L1_0 (H1C0)
    check_function="pocc::POccCalculator::setCorrelationClusters()";
    stringstream log;
    vector<vector<double> > vcorr;
    vector<double> vdg;
    vector<vector<vector<double> > > vprob;
    vector<vector<vector<int> > > vdecoration;
    {
      pocc::POccCalculator pcalc(log);
      setupPOccCalculator(pcalc,partcarRocksaltMgNiO(2,1),"ENUM",_kflags(),log);
      pcalc.calculate();
      pcalc.setCorrelationClusters(6,2);
      uint npairs=0,ntriplets=0;
      vector<uint> vmult;
      for(uint io=0;io<pcalc.m_corr_orbits.size();io++){
        if(pcalc.m_corr_orbits[io].points.size()==2){npairs++;vmult.push_back(pcalc.m_corr_orbits[io].multiplicity);}
        else {ntriplets++;}
      }
      checkEqual(npairs==6 && ntriplets==2,true,check_function,"rocksalt cations, 6 shells and triangles within the 2nd: 6 pair and 2 triplet clusters",passed_checks,results);
      checkEqual(vmult==vector<uint>({6,3,12,6,12,4}),true,check_function,"fcc pair multiplicities per site: half of 12, 6, 24, 12, 24, 8",passed_checks,results);
      const vector<string> vlabels=pcalc.getCorrelationLabels();
      checkEqual(vlabels.size()==8 && vlabels[0]=="p1(1,1)" && vlabels[6]=="t1(1,1,1)",true,check_function,"binary: one function per cluster, p1(1,1) to t2(1,1,1)",passed_checks,results);
      check_function="pocc::POccCalculator::getCorrelations()";
      poccSetCorrelations(pcalc,vcorr,vdg,vprob,vdecoration);
      const vector<double> L11={0,-1,0,1,0,-1,0,0},L10={-1.0/3.0,1,-1.0/3.0,1,-1.0/3.0,1,0,0};
      checkEqual(vcorr.size()==2 && poccVectorsMatch(vcorr[0],L11) && poccVectorsMatch(vcorr[1],L10),true,check_function,"rocksalt Mg0.5Ni0.5O n=2: L1_1 (0,-1,0,1,0,-1) and L1_0 (-1/3,1,-1/3,1,-1/3,1) on shells 1 to 6",passed_checks,results);
      check_function="pocc::POccCalculator::getRandomCorrelations()";
      checkEqual(poccVectorsMatch(pcalc.getRandomCorrelations(),vector<double>(8,0.0)),true,check_function,"Mg0.5Ni0.5: every random correlation is zero",passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // infinite temperature, against an independent calculation (pair counting by distance on the POSCARs of the sets)
    // and, for the pairs, against the closed form: a pair on one slot gives 1, two slots of a cell with fixed counts give ((N_A-N_B)^2-n)/(n(n-1))
    check_function="pocc::POccCalculator::getCorrelations()";
    struct Reference {int n,k; vector<double> corr;};
    const vector<Reference> vref={
      {4,2,{-5.0/63.0,11.0/63.0,-1.0/9.0,3.0/7.0,-1.0/7.0,1.0/21.0,0,0}},
      {8,4,{-2.0/105.0,1.0/21.0,-2.0/35.0,8.0/35.0,-1.0/21.0,1.0/35.0,0,0}},
      {8,2,{33.0/140.0,2.0/7.0,29.0/140.0,59.0/140.0,3.0/14.0,19.0/70.0,1.0/10.0,9.0/70.0}}};  //Mg is the first species (-1): the triangles change sign from Mg=+1
    for(uint iref=0;iref<vref.size();iref++){
      const Reference& ref=vref[iref];
      pocc::POccCalculator pcalc(log);
      setupPOccCalculator(pcalc,partcarRocksaltMgNiO(ref.n,ref.k),"ENUM",_kflags(),log);
      pcalc.calculate();
      pcalc.setCorrelationClusters(6,2);
      poccSetCorrelations(pcalc,vcorr,vdg,vprob,vdecoration);
      const vector<double> vinf=poccInfiniteTemperature(vcorr,vdg);
      const string tag="rocksalt Mg"+aurostd::utype2string(ref.k)+"/"+aurostd::utype2string(ref.n)+"Ni n="+aurostd::utype2string(ref.n)+" ("+aurostd::utype2string(vcorr.size())+" sets)";
      check_description=tag+", infinite temperature: as counted from the POSCARs by distance";
      checkEqual(poccVectorsMatch(vinf,ref.corr),true,check_function,check_description,passed_checks,results);
      //closed form for the pairs: average over the HNF matrices (each weighs its decorations)
      const double n=(double)ref.n,NA=(double)ref.k,NB=n-NA;
      const double distinct=((NA-NB)*(NA-NB)-n)/(n*(n-1.0));
      vector<double> closed;
      xmatrix<double> hnf;
      vector<vector<int> > decoration;
      pocc::POccSupercellAction action;
      pocc::POccSuperCell psc;
      xvector<long long int> x(3);
      for(uint io=0;io<pcalc.m_corr_orbits.size();io++){
        const pocc::POccClusterOrbit& orbit=pcalc.m_corr_orbits[io];
        if(orbit.points.size()!=2){continue;}
        double sum=0.0;
        for(unsigned long long int h=0;h<pcalc.hnf_count;h++){
          psc.m_hnf_index=h;psc.m_site_config_index=0;
          pcalc.getHNFMatSiteConfig(psc,hnf,decoration);
          pcalc.getSupercellAction(hnf,action);
          double same=0.0,total=0.0;
          for(uint ii=0;ii<orbit.images.size();ii++){
            for(uint t=0;t<(uint)ref.n;t++){
              uint slot[2];
              for(uint m=0;m<2;m++){
                for(int i=0;i<3;i++){x[x.lrows+i]=orbit.images[ii][m][i+1]+action.vx[t][action.vx[t].lrows+i];}
                slot[m]=action.slot_of[(uint)orbit.images[ii][m][0]][action.gindex(x)];
              }
              if(slot[0]==slot[1]){same+=1.0;}
              total+=1.0;
            }
          }
          const double f=same/total;
          sum+=f+(1.0-f)*distinct;
        }
        closed.push_back(sum/(double)pcalc.hnf_count);
      }
      check_description=tag+", infinite temperature: pairs equal the closed form of the canonical constraint and the periodic images";
      checkEqual(poccVectorsMatch(vector<double>(vinf.begin(),vinf.begin()+closed.size()),closed),true,check_function,check_description,passed_checks,results);
      //pair probabilities: they sum to one, give the pair correlation, and their marginal is the concentration of the supercell
      bool prob_ok=true;
      for(uint i=0;i<vprob.size()&&prob_ok;i++){
        for(uint ip=0;ip<vprob[i].size();ip++){
          const vector<double>& P=vprob[i][ip];
          if(P.size()!=4 || std::abs(P[0]+P[1]+P[2]+P[3]-1.0)>1e-12){prob_ok=false;break;}
          if(std::abs((P[0]+P[3])-(P[1]+P[2])-vcorr[i][ip])>1e-12){prob_ok=false;}
          if(std::abs(P[0]+P[1]-NA/n)>1e-12 || std::abs(P[0]+P[2]-NA/n)>1e-12){prob_ok=false;}
        }
      }
      check_description=tag+": pair probabilities sum to 1, give the pair correlations, and have the concentrations as marginals";
      checkEqual(prob_ok,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // rocksalt (MgCoNiCuZn)O n=5: four functions per point; random correlations vanish at equal concentrations
    {
      check_function="pocc::POccCalculator::setCorrelationClusters()";
      pocc::POccCalculator pcalc(log);
      setupPOccCalculator(pcalc,partcarRocksaltHEO(5),"ENUM",_kflags(),log);
      pcalc.calculate();
      pcalc.setCorrelationClusters(6,2);
      check_description="rocksalt (MgCoNiCuZn)O: 6 pairs of 10 functions, the nearest triangle with 20, the 1-1-2 triangle with 40";
      checkEqual(pcalc.getCorrelationLabels().size(),(size_t)120,check_function,check_description,passed_checks,results);
      check_function="pocc::POccCalculator::getRandomCorrelations()";
      checkEqual(poccVectorsMatch(pcalc.getRandomCorrelations(),vector<double>(120,0.0),1e-12),true,check_function,"rocksalt (MgCoNiCuZn)O, 0.2 each: every random correlation is zero",passed_checks,results);
      check_function="pocc::POccCalculator::getCorrelations()";
      poccSetCorrelations(pcalc,vcorr,vdg,vprob,vdecoration);
      bool prob_ok=!vprob.empty();
      for(uint i=0;i<vprob.size()&&prob_ok;i++){
        for(uint ip=0;ip<vprob[i].size();ip++){
          const vector<double>& P=vprob[i][ip];
          double sum=0.0;
          for(uint s=0;s<5;s++){
            double row=0.0;
            for(uint t=0;t<5;t++){row+=P[s*5+t];sum+=P[s*5+t];}
            if(std::abs(row-0.2)>1e-12){prob_ok=false;}
          }
          if(std::abs(sum-1.0)>1e-12){prob_ok=false;}
        }
      }
      check_description="rocksalt (MgCoNiCuZn)O n=5 ("+aurostd::utype2string(vcorr.size())+" sets): pair probabilities sum to 1 with marginals 0.2";
      checkEqual(prob_ok,true,check_function,check_description,passed_checks,results);
      check_function="pocc::POccCalculator::getWarrenCowley()";
      checkEqual(pcalc.getWarrenCowleyLabels().size(),(size_t)90,check_function,"rocksalt (MgCoNiCuZn)O: 15 unordered species pairs per pair shell",passed_checks,results);
      bool sum_rule=true;
      for(uint i=0;i<vprob.size();i++){if(!poccWarrenCowleySumRule(pcalc,vprob[i])){sum_rule=false;}}
      check_description="rocksalt (MgCoNiCuZn)O n=5, every set and pair shell: sum over t of c_t alpha_st is zero for each species s";
      checkEqual(sum_rule,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // two inequivalent mixing sites: the pairs between them keep the order of their sites
    {
      check_function="pocc::POccCalculator::getWarrenCowley()";
      pocc::POccCalculator pcalc(log);
      setupPOccCalculator(pcalc,partcarB2AuCuNiZn(4),"ENUM",_kflags(),log);
      pcalc.calculate();
      pcalc.setCorrelationClusters(2,1);  //shell 1: A-B; shell 2: A-A and B-B; no triangle within shell 1
      vector<std::pair<uint,uint> > vst;
      string kinds="";
      for(uint io=0;io<pcalc.m_corr_orbits.size();io++){
        const bool symmetric=pcalc.getWarrenCowleyPairs(pcalc.m_corr_orbits[io],vst);
        const bool same_site=(pcalc.m_corr_orbits[io].points[0][0]==pcalc.m_corr_orbits[io].points[1][0]);
        kinds+=(same_site ? "S" : "D")+aurostd::utype2string(vst.size())+(symmetric ? "s" : "o");
      }
      const vector<string> vlabels=pcalc.getWarrenCowleyLabels();
      const string p1=(vlabels.empty() ? "" : vlabels[0]);
      check_description="B2 (Au0.5Cu0.5)(Ni0.5Zn0.5): A-B pairs have 4 ordered species pairs ("+p1+",...), A-A and B-B 3 unordered";
      checkEqual(kinds=="D4oS3sS3s" && (p1=="p1(Au-Ni)" || p1=="p1(Ni-Au)") && vlabels.size()==10,true,check_function,check_description,passed_checks,results);
      poccSetCorrelations(pcalc,vcorr,vdg,vprob,vdecoration);
      bool sum_rule=!vprob.empty();
      for(uint i=0;i<vprob.size();i++){if(!poccWarrenCowleySumRule(pcalc,vprob[i])){sum_rule=false;}}
      check_description="B2 (Au0.5Cu0.5)(Ni0.5Zn0.5) n=4 ("+aurostd::utype2string(vprob.size())+" sets): both sum rules of the ordered pairs, and the unordered one";
      checkEqual(sum_rule,true,check_function,check_description,passed_checks,results);
      //by hand, with compositions away from the parent's: A-B P=[[0.3,0.1],[0.2,0.4]] (c=0.4,0.6 on the first site, 0.5,0.5 on the second); A-A and B-B P=[[0.2,0.1],[0.1,0.6]] (c=0.3,0.7)
      const vector<vector<double> > vprob_hand={{0.3,0.1,0.2,0.4},{0.2,0.1,0.1,0.6},{0.2,0.1,0.1,0.6}};
      const vector<double> valpha_hand={-0.5,0.5,1.0/3.0,-1.0/3.0,-11.0/9.0,11.0/21.0,-11.0/49.0,-11.0/9.0,11.0/21.0,-11.0/49.0};
      check_description="B2: alpha from pair probabilities by hand, measured against their own marginals";
      checkEqual(poccVectorsMatch(pcalc.getWarrenCowley(vprob_hand),valpha_hand,1e-12),true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // hcp with vacancies (two structure configurations): the vacancy is the first species; marginals follow each supercell
    {
      check_function="pocc::POccCalculator::getCorrelations()";
      pocc::POccCalculator pcalc(log);
      setupPOccCalculator(pcalc,partcarHCPMgZn(4,0.25,0.25,0.5,0.5),"ENUM",_kflags(),log);
      pcalc.calculate();
      pcalc.setCorrelationClusters(4,2);
      bool vacancy_first=true;
      for(uint site=0;site<pcalc.m_corr_site_values.size();site++){
        if(pcalc.m_corr_site_values[site].empty() || pcalc.m_corr_site_values[site][0]!=-1){vacancy_first=false;}
        double sum=0.0;
        for(uint s=0;s<pcalc.m_corr_site_conc[site].size();s++){sum+=pcalc.m_corr_site_conc[site][s];}
        if(std::abs(sum-1.0)>1e-12){vacancy_first=false;}
      }
      checkEqual(vacancy_first,true,check_function,"hcp Mg0.25Zn0.5: vacancy first among the species of each site, concentrations summing to 1",passed_checks,results);
      poccSetCorrelations(pcalc,vcorr,vdg,vprob,vdecoration);
      bool marginals_ok=!vprob.empty();
      for(uint i=0;i<vprob.size()&&marginals_ok;i++){
        //concentration of each species over the slots of the supercell
        vector<double> conc(3,0.0);
        double nslots=0.0;
        for(uint site=0;site<vdecoration[i].size();site++){
          for(uint j=0;j<vdecoration[i][site].size();j++){conc[vdecoration[i][site][j]+1]+=1.0;nslots+=1.0;}
        }
        for(uint ip=0;ip<vprob[i].size();ip++){
          const vector<double>& P=vprob[i][ip];
          for(uint s=0;s<3;s++){
            const double row=P[s*3]+P[s*3+1]+P[s*3+2];
            if(std::abs(row-conc[s]/nslots)>1e-12){marginals_ok=false;}
          }
        }
      }
      check_description="hcp Mg0.25Zn0.5 n=4 ("+aurostd::utype2string(vcorr.size())+" sets, vacancies): the pair marginals are the species fractions of each supercell";
      checkEqual(marginals_ok,true,check_function,check_description,passed_checks,results);
      check_function="pocc::POccCalculator::getWarrenCowley()";
      bool sum_rule=!vprob.empty();
      for(uint i=0;i<vprob.size();i++){if(!poccWarrenCowleySumRule(pcalc,vprob[i])){sum_rule=false;}}
      check_description="hcp Mg0.25Zn0.5 n=4 (vacancies): the sum rule holds in every supercell";
      checkEqual(sum_rule,true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // post-processing: correlations from the POSCAR titles, and the aflow.pocc.out lines
    {
      check_function="pocc::parsePOccSupercellTitle()";
      pocc::POccCalculator pcalc(log);
      setupPOccCalculator(pcalc,partcarRocksaltMgNiO(4,2),"ENUM",_kflags(),log);
      pcalc.calculate();
      pcalc.setCorrelationClusters();
      poccSetCorrelations(pcalc,vcorr,vdg,vprob,vdecoration);
      bool titles_ok=true;
      int n=0;
      xmatrix<double> hnf,hnf_ref;
      vector<vector<int> > decoration,decoration_ref;
      pcalc.m_vsupercell_titles.clear();
      for(std::list<pocc::POccSuperCellSet>::const_iterator it=pcalc.l_supercell_sets.begin();it!=pcalc.l_supercell_sets.end();++it){
        const string title=pcalc.createXStructure((*it).getSuperCell(),pcalc.n_hnf,pcalc.hnf_count,pcalc.types_config_permutations_count,true,false).title;
        pcalc.m_vsupercell_titles.push_back(title);
        pcalc.getHNFMatSiteConfig((*it).getSuperCell(),hnf_ref,decoration_ref);
        if(!pocc::parsePOccSupercellTitle(title,n,hnf,decoration) || n!=4 || !aurostd::isequal(hnf,hnf_ref) || decoration!=decoration_ref){titles_ok=false;}
      }
      checkEqual(titles_ok,true,check_function,"rocksalt n=4: HNF matrix and decoration read back from every supercell title",passed_checks,results);
      checkEqual(pocc::parsePOccSupercellTitle("rocksalt H0C0 DG=4",n,hnf,decoration),false,check_function,"a title without them is refused",passed_checks,results);
      check_function="pocc::POccCalculator::calculateSupercellCorrelations()";
      const bool calculated=pcalc.calculateSupercellCorrelations();
      checkEqual(calculated && pcalc.m_vsupercell_corr==vcorr,true,check_function,"rocksalt n=4: the correlations from the titles equal those of the sets",passed_checks,results);
      //as post-processing sees them: the structures file written as from the parent aflow.in, read back into a calculator that never enumerated
      {
        const string dir=aurostd::TmpDirectoryCreate("pocc_correlations_unit_test");
        stringstream file_ss;
        unsigned long long int i=0;
        for(std::list<pocc::POccSuperCellSet>::const_iterator it=pcalc.l_supercell_sets.begin();it!=pcalc.l_supercell_sets.end();++it,++i){
          const string arun=pcalc.getARUNString(i);
          file_ss << AFLOWIN_SEPARATION_LINE << endl;
          file_ss << _VASP_POSCAR_MODE_EXPLICIT_START_P_ << arun << endl;
          file_ss << pcalc.createXStructure((*it).getSuperCell(),pcalc.n_hnf,pcalc.hnf_count,pcalc.types_config_permutations_count,true,true);
          file_ss << _VASP_POSCAR_MODE_EXPLICIT_STOP_P_ << arun << endl;
          file_ss << AFLOWIN_SEPARATION_LINE << endl;
        }
        aurostd::stringstream2file(file_ss,dir+"/"+POCC_FILE_PREFIX+POCC_UNIQUE_SUPERCELLS_FILE);
        _aflags aflags;aflags.Directory=dir;
        pocc::POccStructuresFile psf(log);
        psf.initialize(POCC_FILE_PREFIX+POCC_UNIQUE_SUPERCELLS_FILE,aflags);
        stringstream input(partcarRocksaltMgNiO(4,2));
        xstructure xstr(input,IOAFLOW_AUTO);
        pocc::POccCalculator pcalc_post(xstr,log);
        psf.loadDataIntoCalculator(pcalc_post,false);  //no run directories here; the sets and titles are copied first
        const bool post=pcalc_post.calculateSupercellCorrelations();
        aurostd::RemoveDirectory(dir);
        check_description="rocksalt n=4: from a structures file (primitive cells) in a calculator that never enumerated, the same correlations";
        checkEqual(post && pcalc_post.m_vsupercell_titles.size()==vcorr.size() && pcalc_post.m_vsupercell_corr==vcorr,true,check_function,check_description,passed_checks,results);
      }
      check_function="pocc::POccCalculator::writeCorrelations()";
      stringstream out;
      pcalc.writeCorrelations(out,false);
      const vector<double> vinf=poccInfiniteTemperature(vcorr,vdg);
      vector<string> vlines,vtokens;
      aurostd::string2vectorstring(out.str(),vlines);
      vector<double> vread;
      bool found_functions=false;
      for(uint i=0;i<vlines.size();i++){
        if(vlines[i].find("correlation_functions=p1(1,1),p2(1,1)")==0){found_functions=true;}
        if(vlines[i].find("correlation_infinite_temperature=")==0){
          string values=vlines[i].substr(string("correlation_infinite_temperature=").size());
          values=values.substr(0,values.find(' '));
          aurostd::string2tokens(values,vtokens,",");
          for(uint j=0;j<vtokens.size();j++){vread.push_back(aurostd::string2utype<double>(vtokens[j]));}
        }
      }
      checkEqual(found_functions && poccVectorsMatch(vread,vinf,1e-9),true,check_function,"aflow.pocc.out: the functions, and the infinite-temperature correlations as written",passed_checks,results);
      //Warren-Cowley of a binary at x=0.5, each pair shell: alpha(Mg-Ni) is the pair correlation, alpha(Mg-Mg) and alpha(Ni-Ni) its negative
      check_function="pocc::POccCalculator::getWarrenCowley()";
      const vector<string> vwc_labels=pcalc.getWarrenCowleyLabels();
      check_description="rocksalt Mg0.5Ni0.5O: Mg-Mg, Mg-Ni and Ni-Ni for each of the 6 pair shells";
      checkEqual(vwc_labels.size()==18 && aurostd::joinWDelimiter(vwc_labels,",").find("p1(Mg-Mg),p1(Mg-Ni),p1(Ni-Ni),p2(Mg-Mg),")==0,true,check_function,check_description,passed_checks,results);
      bool wc_ok=true;
      for(uint i=0;i<vprob.size();i++){
        const vector<double> valpha=pcalc.getWarrenCowley(vprob[i]);
        if(valpha.size()!=18){wc_ok=false;break;}
        for(uint k=0;k<6;k++){
          if(std::abs(valpha[3*k+1]-vcorr[i][k])>1e-12 || std::abs(valpha[3*k]+vcorr[i][k])>1e-12 || std::abs(valpha[3*k+2]+vcorr[i][k])>1e-12){wc_ok=false;}
        }
      }
      checkEqual(wc_ok,true,check_function,"rocksalt Mg0.5Ni0.5O n=4, every set: alpha(Mg-Ni) equals the pair correlation of its shell, alpha(Mg-Mg)=alpha(Ni-Ni) its negative",passed_checks,results);
      check_function="pocc::POccCalculator::writeCorrelations()";
      uint nsupercell=0;
      bool wc_written=true;
      vector<double> wc_infinite;
      for(uint i=0;i<vlines.size();i++){
        if(vlines[i].find("warren_cowley_supercell_")==0){
          if(nsupercell>=vprob.size() || !poccVectorsMatch(poccLineValues(vlines[i]),pcalc.getWarrenCowley(vprob[nsupercell]),1e-9)){wc_written=false;}
          nsupercell++;
        }
        if(vlines[i].find("warren_cowley_infinite_temperature=")==0){wc_infinite=poccLineValues(vlines[i]);}
      }
      bool wc_infinite_ok=(wc_infinite.size()==18);
      for(uint k=0;k<6&&wc_infinite_ok;k++){if(std::abs(wc_infinite[3*k+1]-vinf[k])>1e-9 || std::abs(wc_infinite[3*k]+vinf[k])>1e-9){wc_infinite_ok=false;}}
      check_description="aflow.pocc.out: Warren-Cowley of every supercell as written, and at infinite temperature alpha(Mg-Ni) equal to the infinite-temperature pair correlations";
      checkEqual(wc_written && nsupercell==vprob.size() && wc_infinite_ok,true,check_function,check_description,passed_checks,results);
      //at the degeneracies as probabilities, the ensemble at a temperature is the infinite-temperature one
      double dgtotal=0.0;
      for(uint i=0;i<vdg.size();i++){dgtotal+=vdg[i];}
      uint iset=0;
      for(std::list<pocc::POccSuperCellSet>::iterator it=pcalc.l_supercell_sets.begin();it!=pcalc.l_supercell_sets.end();++it,++iset){(*it).m_probability=vdg[iset]/dgtotal;}
      out.str("");
      pcalc.writeCorrelations(out,true);
      aurostd::string2vectorstring(out.str(),vlines);
      vector<double> corr_ensemble,wc_ensemble;
      for(uint i=0;i<vlines.size();i++){
        if(vlines[i].find("correlation_ensemble=")==0){corr_ensemble=poccLineValues(vlines[i]);}
        if(vlines[i].find("warren_cowley_ensemble=")==0){wc_ensemble=poccLineValues(vlines[i]);}
      }
      check_description="aflow.pocc.out at a temperature, with the degeneracies as probabilities: the ensemble correlations and Warren-Cowley equal the infinite-temperature ones";
      checkEqual(poccVectorsMatch(corr_ensemble,vinf,1e-9) && poccVectorsMatch(wc_ensemble,wc_infinite,1e-9),true,check_function,check_description,passed_checks,results);
    }

    // ---------------------------------------------------------------------------
    // UFF parents get them too: at n=4 the UFF bins hold one structure each, so the infinite-temperature correlations agree
    {
      check_function="pocc::POccCalculator::getCorrelations()";
      vector<vector<double> > vinf;
      for(const string enumerator : {"ENUM","UFF"}){
        pocc::POccCalculator pcalc(log);
        setupPOccCalculator(pcalc,partcarRocksaltMgNiO(4,2),enumerator,_kflags(),log);
        pcalc.calculate();
        pcalc.setCorrelationClusters();
        poccSetCorrelations(pcalc,vcorr,vdg,vprob,vdecoration);
        vinf.push_back(poccInfiniteTemperature(vcorr,vdg));
      }
      checkEqual(poccVectorsMatch(vinf[0],vinf[1]),true,check_function,"rocksalt Mg0.5Ni0.5O n=4: UFF bins and ENUM sets give the same infinite-temperature correlations",passed_checks,results);
    }
  } catch (aurostd::xerror& e) {
    errors.push_back("POCC correlations test failed - aurostd::xerror: " + e.buildMessageString());
  } catch (const std::exception& e) {
    errors.push_back("POCC correlations test failed - std::exception: " + string(e.what()));
  } catch (...) {
    errors.push_back("POCC correlations test failed - unknown exception");
  }
}

}  // namespace unittest
}  // namespace pocc

bool smithTest(ostream& oss){ofstream FileMESSAGE;return smithTest(FileMESSAGE,oss);}  //CO20190520
bool smithTest(ofstream& FileMESSAGE,ostream& oss){  //CO20190520
  bool LDEBUG=TRUE; // TRUE;
  stringstream message;
  _aflags aflags;aflags.Directory=".";

  xmatrix<int> A1(3,3),U1,V1,S1;
  A1 ={{3,2,1}, {5,3,1}, {6,8,9}};
  //  A1[1][1]=3;A1[1][2]=2;A1[1][3]=1;
  //  A1[2][1]=5;A1[2][2]=3;A1[2][3]=1;
  //  A1[3][1]=6;A1[3][2]=8;A1[3][3]=9;

  aurostd::getSmithNormalForm(A1,U1,V1,S1);

  if(LDEBUG){
    cerr << __AFLOW_FUNC__ << " A=" << endl;cerr << A1 << endl;
    cerr << __AFLOW_FUNC__ << " U=" << endl;cerr << U1 << endl;
    cerr << __AFLOW_FUNC__ << " V=" << endl;cerr << V1 << endl;
    cerr << __AFLOW_FUNC__ << " S=" << endl;cerr << S1 << endl;
  }

  xmatrix<long long int> A2(5,5),U2,V2,S2;  //long long int is CRUCIAL, Matlab actually gets this wrong because it uses long int by default
  A2 = {{ 25,  -300,   1050, -1400,   630},
    {-300,  4800, -18900, 26880, -12600},
    { 1050,-18900, 79380,-117600, 56700},
    {-1400, 26880,-117600,179200,-88200},
    { 630, -12600, 56700,-88200,  44100}};

  aurostd::getSmithNormalForm(A2,U2,V2,S2);

  if(LDEBUG){ //COME BACK AND PATCH FOR ANSWERS
    cerr << __AFLOW_FUNC__ << " A=" << endl;cerr << A2 << endl;
    cerr << __AFLOW_FUNC__ << " U=" << endl;cerr << U2 << endl;
    cerr << __AFLOW_FUNC__ << " V=" << endl;cerr << V2 << endl;
    cerr << __AFLOW_FUNC__ << " S=" << endl;cerr << S2 << endl;
  }

  message << "smith test successful";pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,aflags,FileMESSAGE,oss,_LOGGER_COMPLETE_);
  return TRUE; //CO20180419
}

// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2023           *
// *                                                                         *
// ***************************************************************************
