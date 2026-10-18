// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2023           *
// *                                                                         *
// ***************************************************************************
// This EntryLoader class is the combination/evolution of different prior solutions to integrate with the AFLOW database.
// 2016 - Corey Oses (CHULL)
// 2018 - Marco Esters (database design/integration)
// 2018 - Frisco Rose (API requests)
// 2020 - David Hicks (structural comparisons)
// 2022 - Hagen Eckert (unified EntryLoader for all available database sources)
// hagen.eckert@duke.edu


#ifndef _AFLOWLIB_ENTRY_LOADER_CPP_
#define _AFLOWLIB_ENTRY_LOADER_CPP_

#include "aflow.h"
#include <algorithm> //CO20260930
#include "aflowlib.h"
#include "aflow_aflux.h" //WJ Include aflux for filtering functions

static const char* const ENTRY_LOADER_COLLECTION_WEB="WEB"; //CO20260930

#define _DEBUG_ENTRY_LOADER_ false


namespace aflowlib {
  //CO20261002 - entries per AFLUX page when paging(0) is sent as pages, and the most pages one query may take.
  //The AFLUX server runs the whole query for every page and returns one slice; the CHAOS API will take
  //pages of at most 500 (CO20261006), so a large query takes more requests.
  static const uint AFLUX_PAGE_SIZE = AFLUX_PAGE_SIZE_DEFAULT; //CO20261006 - the CHAOS API answers pages of at most 500 (was 10000)
  static const uint AFLUX_MAX_PAGES = 1000;
  //CO20261004 - AUIDs or AURLs per AFLUX request: a list joins a batch while the batch and the next value
  //(without the ':') are shorter than this, so the URL stays short (HE20221117)
  static const size_t AFLUX_VALUE_LIST_MAX = 5000;

  //CO20261004 START
  /// @brief the terms of an AFLUX query at the outer level, decoded: "?a(1),b%282%2C3%29" -> {"a(1)", "b(2,3)"}
  /// @param query AFLUX query, plain or percent-encoded, with or without the leading `?`
  static std::vector<std::string> AFLUXQueryTerms(const std::string &query) {
    std::string decoded = aflux::decodeURL(query);
    if (!decoded.empty() && decoded[0] == '?') {decoded.erase(0, 1);}
    return aflux::splitStringByOuterDelimeter(decoded, ',');
  }

  /// @brief the name of an AFLUX term without white space: " $paging( 0 )" -> "$paging"
  static std::string AFLUXTermName(const std::string &term) {
    return aurostd::RemoveWhiteSpaces(aflux::pairParamsFromFuncs({term})[0].first);
  }

  /// @brief does a query, as AFLUX's parser read it, ask for its whole result in one answer: paging(0) in format(aflow)
  /// @param parsed AFLUX object after AFLUX::parseQuery()
  /// @note the s4e.ai gate refuses paging(0); such a query is sent as pages (EntryLoader::getAFLUXPages()),
  ///       joined line by line, which needs format(aflow) (one entry per line)
  static bool AFLUXPagingAll(const aflux::AFLUX &parsed) {
    return !parsed.m_default_paging && parsed.m_pagingParam_n == 0 && parsed.m_format == "aflow";
  }
  //CO20261004 STOP

  /// @class EntryLoader
  /// @brief unified interface to gather AFLOW lib entries from different sources
  ///
  /// @authors
  /// @mod{CO,2016,CHULL}
  /// @mod{ME,2018,database design/integration}
  /// @mod{FR,2018,API requests}
  /// @mod{DX,2020,structural comparisons}
  /// @mod{HE,20220819,created EntryLoader}
  ///
  /// Basic usage
  /// @code
  /// std::vector <std::shared_ptr<aflowlib::_aflowlib_entry>> results;
  /// { // ensures that all resources are released as soon as possible
  ///   aflowlib::EntryLoader el;
  ///   el.m_out_debug = true; // change the behavior of the EntryLoader class
  ///   el.loadAlloy("NiCa"); // the load functions can be called multiple times
  ///   el.loadAUID("aflow:7dd846bc04c764e8"); // no duplicates will be stored
  ///   el.getEntriesViewFlat(results);
  /// }
  /// for (std::shared_ptr<aflowlib::_aflowlib_entry>> entry : results) std::cout << entry->auid << std::endl;
  /// @endcode

  // class constructor
  EntryLoader::EntryLoader(std::ostream &oss) : xStream(oss) { init(); }

  EntryLoader::EntryLoader(std::ofstream &FileMESSAGE, std::ostream &oss) : xStream(FileMESSAGE, oss) { init(); }

  /// @brief create a copy at initialization
  /// @note DB connections are reused and not recreated
  /// @note a new view on the data is created, but the same lib entries are reused
  EntryLoader::EntryLoader(const EntryLoader &b) : xStream(*b.getOFStream(), *b.getOSS()) { copy(b); } // copy PUBLIC

  // class de-constructor
  EntryLoader::~EntryLoader() { xStream::free(); }

  /// @brief create a copy through assigning
  /// @note DB connections are reused and not recreated
  /// @note a new view on the data is created, but the same lib entries are reused
  const EntryLoader &EntryLoader::operator=(const EntryLoader &other) {
    if (this != &other) { copy(other); }
    return *this;
  }

  /// @brief initialize the class (private)
  /// create shared pointer used to store the data views and read default values from flags
  /// @brief the SQLite file the entry loader reads for a given --aflow_db= value
  /// @param aflow_db_file value of --aflow_db= (XHOST.vflag_control "AFLOW_DB_FILE")
  /// @return the file itself; DEFAULT_AFLOW_DB_FILE when it is empty or a folder (a JSONL folder is not an SQLite file)
  /// @authors
  /// @mod{CO,20261007,created}
  std::string EntryLoader::sqliteFile(const std::string& aflow_db_file) {
    if (aflow_db_file.empty() || aurostd::IsDirectory(aflow_db_file)) return DEFAULT_AFLOW_DB_FILE;
    return aflow_db_file;
  }

  void EntryLoader::init() {
    m_out_silent = true;
    m_out_debug = (XHOST.DEBUG || _DEBUG_ENTRY_LOADER_);
    m_xstructure_relaxed = false;
    m_xstructure_original = false;
    m_xstructure_final_file_name = {"CONTCAR.relax2", "CONTCAR.relax", "POSCAR.static", "POSCAR.bands", "CONTCAR.static", "CONTCAR.bands"};

    m_sqlite_file = sqliteFile(XHOST.vflag_control.getattachedscheme("AFLOW_DB_FILE"));  //CO20261004 - --aflow_db= picks the database (defaults to DEFAULT_AFLOW_DB_FILE)
    m_sqlite_alloy_file = DEFAULT_ENTRY_LOADER_ALLOY_DB_FILE;
    m_sqlite_collection = std::string(ENTRY_LOADER_COLLECTION_WEB); //CO20260930

    m_auid_prefix = DEFAULT_ENTRY_LOADER_AUID_PREFIX;
    m_aurl_path = DEFAULT_ENTRY_LOADER_AURL_PATH;

    //WJ20231103 begin Create vector of servers and lists from string in rc
    m_aflux_server_list = DEFAULT_ENTRY_LOADER_AFLUX_SERVER_LIST;
    m_aflux_path_list = DEFAULT_ENTRY_LOADER_AFLUX_PATH_LIST;
    aurostd::string2tokens(m_aflux_server_list, m_aflux_server_vector, ",");
    aurostd::string2tokens(m_aflux_path_list, m_aflux_path_vector, ",");
    if(m_aflux_server_vector.size()!=m_aflux_path_vector.size()){
      m_logger_message << "Bad .aflow.rc specifications, DEFAULT_ENTRY_LOADER_AFLUX_SERVER_LIST.size() != DEFAULT_ENTRY_LOADER_AFLUX_PATH_LIST.size()";
      outHardError(__AFLOW_FUNC__, __LINE__,_INDEX_MISMATCH_);
      return;
    }
    //WJ end



    m_aflux_collection = "WEB"; //WJ20240523 - Changed RAW to WEB
    m_aflux_directives = {{"format", "aflow"}, {"paging", "0"}}; //WJ20231103 NOTE: Need to change paging parameter (AFLOW struggles to return all data with 0 paging (too many entries))

    m_restapi_server = DEFAULT_ENTRY_LOADER_RESTAPI_SERVER;
    m_restapi_path = DEFAULT_ENTRY_LOADER_RESTAPI_PATH;
    m_restapi_directives = DEFAULT_FILE_AFLOWLIB_ENTRY_OUT;
    m_restapi_listing_dirs = "?aflowlib_entries";
    m_restapi_listing_files = "?files";
    m_restapi_collection = "WEB";
    m_filesystem_outfile = DEFAULT_FILE_AFLOWLIB_ENTRY_OUT;
    m_filesystem_path = DEFAULT_ENTRY_LOADER_FS_PATH;
    m_filesystem_collection = "RAW";

    m_entries_flat = std::make_shared<std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>();
    m_entries_layered_map = std::make_shared<std::map<short,
                                             std::map<std::string,
                                             std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>>>();

    m_current_source = Source::NONE;
    m_filesystem_available = false;
    m_out_super_silent = false;
  }

  /// @brief clean a EntryLoader object
  void EntryLoader::clear() { free(); }

  /// @brief reset a EntryLoader object
  void EntryLoader::free() { *this = {}; } // calling the default constructor to ensure a clean instance of EntryLoader

  /// @brief create a copy (privat)
  void EntryLoader::copy(const EntryLoader &b) {  //copy PRIVATE
    m_out_debug = b.m_out_debug;
    m_out_silent = b.m_out_silent;
    m_xstructure_relaxed = b.m_xstructure_relaxed;
    m_xstructure_original = b.m_xstructure_original;
    m_xstructure_final_file_name = b.m_xstructure_final_file_name;
    m_sqlite_file = b.m_sqlite_file;
    m_sqlite_alloy_file = b.m_sqlite_alloy_file;
    m_sqlite_collection = b.m_sqlite_collection;
    m_aflux_server = b.m_aflux_server;
    m_aflux_path = b.m_aflux_path;
    m_aflux_collection = b.m_aflux_collection;
    m_aflux_directives = b.m_aflux_directives;
    m_restapi_server = b.m_restapi_server;
    m_restapi_path = b.m_restapi_path;
    m_restapi_directives = b.m_restapi_directives;
    m_restapi_listing_dirs = b.m_restapi_listing_dirs;
    m_restapi_listing_files = b.m_restapi_listing_files;
    m_restapi_collection = b.m_restapi_collection;
    m_filesystem_outfile = b.m_filesystem_outfile;
    m_filesystem_path = b.m_filesystem_path;
    m_filesystem_collection = b.m_filesystem_collection;
    m_current_source = b.m_current_source;
    m_filesystem_available = b.m_filesystem_available;
    m_auid_list = b.m_auid_list;
    aurostd::StringstreamClean(m_logger_message);

    m_sqlite_db_ptr = nullptr;
    m_sqlite_alloy_db_ptr = nullptr;

    m_entries_flat = std::make_shared < std::vector < std::shared_ptr < aflowlib::_aflowlib_entry>>>(*b.m_entries_flat);
    m_entries_layered_map = std::make_shared < std::map < short,
        std::map < std::string,
        std::vector < std::shared_ptr < aflowlib::_aflowlib_entry >>>>>(*b.m_entries_layered_map);
  }

  /// @brief load a single entry from a AUID
  /// @param AUID AFLOW unique ID
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note following AUIDs would be equivalent:
  /// @note `aflow:d912e209c81aeb94`
  /// @note `auid:d912e209c81aeb94`
  /// @note `d912e209c81aeb94`
  void EntryLoader::loadAUID(std::string AUID) { 
    selectSource();

    m_logger_message << "Try loading: " << AUID;
    outInfo(__AFLOW_FUNC__);
    if (!cleanAUID(AUID)) {
      m_logger_message << "AUID cleaning failed";
      outError(__AFLOW_FUNC__, __LINE__);
      return;
    }

    switch (m_current_source) {

      case Source::SQLITE: {
        std::string where = m_sqlite_db_ptr->idColumn() + "='\"" + AUID + "\"'";  //CO20261008 - chaos_id, or auid in a database built before
        loadSqliteWhere(where);
        break;
      }

      case Source::AFLUX: {
        std::map <std::string, std::string> matchbook{{"*",    ""},
                                                      {"chaos_id", AFLUXValueList({AUID})}};  //CO20261008 - was auid
        loadAFLUXMatchbook(matchbook);
        break;
      }

      case Source::RESTAPI:
      case Source::RESTAPI_RAW: {
        loadAUID(std::vector<std::string>({AUID}));
        break;
      }

      case Source::FILESYSTEM:
      case Source::FILESYSTEM_RAW: {
        loadAUID(std::vector<std::string>({AUID}));
        break;
      }

      default:
        break;
    }
  }

  /// @brief load multiple entries from a vector of AUIDs
  /// @param AUID list of AURLs
  /// @authors
  /// @mod{HE,20220216,created}
  /// @mod{HE,20221117,ensure that aflux URL is not longer than 8000 characters}
  void EntryLoader::loadAUID(const std::vector<std::string> &AUID) {
    selectSource();
    std::vector <std::string> clean_AUID;

    m_logger_message << "Try loading " << AUID.size() <<" AUIDs";
    outInfo(__AFLOW_FUNC__);

    for (std::vector<std::string>::const_iterator AUID_single = AUID.begin(); AUID_single != AUID.end(); AUID_single++) {
      std::string temp = *AUID_single;
      if (cleanAUID(temp)) clean_AUID.push_back(temp);
    }

    if (clean_AUID.size()!=AUID.size()) {
      m_logger_message << "cleaning of " << AUID.size() - clean_AUID.size()  << " AUIDs failed";
      outError(__AFLOW_FUNC__, __LINE__);
    }

    switch (m_current_source) {

      case Source::AFLUX: {
        //CO20261004 - batches from AFLUXValueBatches(), were joined here by hand with a first flag
        const std::vector<std::string> batches = AFLUXValueBatches(clean_AUID, AFLUX_VALUE_LIST_MAX);
        for (size_t i = 0; i < batches.size(); i++) {
          loadAFLUXMatchbook({{"*",    ""},
                              {"chaos_id", batches[i]}});  //CO20261008 - was auid
        }
        break;
      }

      case Source::SQLITE: {
        std::string where = aurostd::joinWDelimiter(clean_AUID, "\"','\"");
        where = m_sqlite_db_ptr->idColumn() + " IN ('\"" + where + "\"')";  //CO20261008 - chaos_id, or auid in a database built before
        loadSqliteWhere(where);
        break;
      }

      case Source::RESTAPI:
      case Source::RESTAPI_RAW: {
        std::vector <std::string> queries;
        const std::string auid_prefix = m_auid_prefix + ":";
        const size_t prefix_len = auid_prefix.size();
        for (std::vector<std::string>::const_iterator AUID_single = clean_AUID.begin(); AUID_single != clean_AUID.end(); AUID_single++) {
          if (AUID_single->size() <= prefix_len) continue;
          std::string rest_query = "AUID/" + auid_prefix;
          for (size_t part = prefix_len; part + 1 < AUID_single->size(); part += 2) {
            rest_query += AUID_single->substr(part, 2) + "/";
          }
          rest_query += m_restapi_collection + "/";
          queries.push_back(rest_query);
        }
        loadRestAPIQueries(queries);
        break;
      }

      case Source::FILESYSTEM:
      case Source::FILESYSTEM_RAW: {
        std::vector <std::string> files;
        const std::string auid_prefix = m_auid_prefix + ":";
        const size_t prefix_len = auid_prefix.size();
        for (std::vector<std::string>::const_iterator AUID_single = clean_AUID.begin(); AUID_single != clean_AUID.end(); AUID_single++) {
          if (AUID_single->size() <= prefix_len) continue;
          std::string file_path = m_filesystem_path + "AUID/" + auid_prefix;
          for (size_t part = prefix_len; part + 1 < AUID_single->size(); part += 2) {
            file_path += AUID_single->substr(part, 2) + "/";
          }
          file_path += m_filesystem_collection + "/" + m_filesystem_outfile;
          files.push_back(file_path);
        }
        loadFiles(files);
        break;
      }

      default:
        break;
    }
  }

  /// @brief load a single entry from an AURL
  /// @param AURL AFLOW unique URL
  /// @authors
  /// @mod{HE,20220217,created}
  /// @note following AURLs would be equivalent:
  /// @note `s4e.ai:AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138`
  /// @note `AFLOWDATA/LIB2_RAW/Ca_svCu_pv/138`
  /// @note `LIB2_RAW/Ca_svCu_pv/138`
  /// @note `WEB`, `RAW`, and `LIB` will be replaced by the selected collection
  ///       (#m_sqlite_collection, #m_aflux_collection, #m_restapi_collection, #m_filesystem_collection)
  void EntryLoader::loadAURL(std::string AURL) {
    selectSource();

    if (!cleanAURL(AURL)) return;

    m_logger_message << "Try loading " << AURL;
    outInfo(__AFLOW_FUNC__);
    if (!cleanAURL(AURL)) {
      m_logger_message << "AURL cleaning failed";
      outError(__AFLOW_FUNC__, __LINE__);
      return;
    }

    switch (m_current_source) {

      case Source::SQLITE: {
        std::string where = "";
        if (m_sqlite_db_ptr->pathColumn() == "chaos_path") {where = "chaos_path='\"" + AURL2ChaosPath(AURL) + "\"'";}  //CO20261008
        else {
          where = "aurl='\"" + AURL + "\"'";
          where = std::regex_replace(where, aurostd::regex_aurl2file, "$1_" + m_sqlite_collection + "/");
        }
        loadSqliteWhere(where);
        break;
      }

      case Source::AFLUX: {
        std::map <std::string, std::string> matchbook{{"*",    ""},
                                                      {"chaos_path", AFLUXValueList({AURL2ChaosPath(AURL)})}};  //CO20261008 - was aurl
        loadAFLUXMatchbook(matchbook);
        break;
      }

      case Source::RESTAPI:
      case Source::RESTAPI_RAW: {
        loadAURL(std::vector<std::string>({AURL}));
        break;
      }

      case Source::FILESYSTEM:
      case Source::FILESYSTEM_RAW: {
        loadAURL(std::vector<std::string>({AURL}));
        break;
      }

      default:
        break;
    }
  }

  /// @brief load multiple entries from a vector of AURLs
  /// @param AURL list of AURLs
  /// @authors
  /// @mod{HE,20220322,created}
  /// @mod{HE,20221117,ensure that aflux URL is not longer than 8000 characters}
  void EntryLoader::loadAURL(const std::vector <std::string> &AURL) {
    selectSource();

    m_logger_message << "Try loading " << AURL.size() <<" AURLs";
    outInfo(__AFLOW_FUNC__);

    std::vector <std::string> clean_AURL;
    for (std::vector<std::string>::const_iterator AURL_single = AURL.begin(); AURL_single != AURL.end(); AURL_single++) {
      std::string temp = *AURL_single;
      if (cleanAURL(temp)) clean_AURL.push_back(temp);
    }

    if (clean_AURL.size()!=AURL.size()) {
      m_logger_message << "cleaning of " << AURL.size() - clean_AURL.size()  << " AURLs failed";
      outError(__AFLOW_FUNC__, __LINE__);
    }

    switch (m_current_source) {

      case Source::AFLUX: {
        //CO20261004 - batches from AFLUXValueBatches(), were joined here by hand with a first flag
        std::vector<std::string> clean_paths;  //CO20261008 - chaos_path of each AURL
        for (size_t i = 0; i < clean_AURL.size(); i++) clean_paths.push_back(AURL2ChaosPath(clean_AURL[i]));
        const std::vector<std::string> batches = AFLUXValueBatches(clean_paths, AFLUX_VALUE_LIST_MAX);
        for (size_t i = 0; i < batches.size(); i++) {
          loadAFLUXMatchbook({{"*",    ""},
                              {"chaos_path", batches[i]}});  //CO20261008 - was aurl
        }
        break;
      }

      case Source::SQLITE: {
        std::string where = "";
        if (m_sqlite_db_ptr->pathColumn() == "chaos_path") {  //CO20261008
          std::vector<std::string> paths;
          for (size_t i = 0; i < clean_AURL.size(); i++) paths.push_back(AURL2ChaosPath(clean_AURL[i]));
          where = "chaos_path IN ('\"" + aurostd::joinWDelimiter(paths, "\"','\"") + "\"')";
        } else {
          where = aurostd::joinWDelimiter(clean_AURL, "\"','\"");
          where = std::regex_replace(where, aurostd::regex_aurl2file, "$1_" + m_sqlite_collection + "/");
          where = "aurl IN ('\"" + where + "\"')";
        }
        loadSqliteWhere(where);
        break;
      }

      case Source::RESTAPI:
      case Source::RESTAPI_RAW: {
        std::vector <std::string> queries;
        for (std::vector<std::string>::const_iterator AURL_single = clean_AURL.begin(); AURL_single != clean_AURL.end(); AURL_single++) {
          std::string rest_query = std::regex_replace(aflowlib::AURLPath(*AURL_single), aurostd::regex_aurl2file, "$1_" + m_sqlite_collection + "/") + "/";
          queries.push_back(rest_query);
        }
        loadRestAPIQueries(queries);
        break;
      }

      case Source::FILESYSTEM:
      case Source::FILESYSTEM_RAW: {
        std::vector <std::string> files;
        for (std::vector<std::string>::const_iterator AURL_single = clean_AURL.begin(); AURL_single != clean_AURL.end(); AURL_single++) {
          std::string file_path = m_filesystem_path + aflowlib::AURLPath(*AURL_single) + "/" + m_filesystem_outfile;
          file_path = std::regex_replace(file_path, aurostd::regex_aurl2file, "$1/" + m_filesystem_collection + "/");
          files.push_back(file_path);
        }
        loadFiles(files);
        break;
      }

      default:
        break;
    }
  }

  /// @brief load all entries that are available for a alloy set
  /// @param alloy alloy descriptor
  /// @param recursive include sub alloys
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note an alloy can be given as comma separated list `"Cu, Au, Fe"`
  ///       or as combined string `"CuAuFe"` (elements don't need to be sorted)
  /// @note `recursive=true` (default) will generate sub-alloys so `CuAuFe`
  ///       will load `AuCuFe`, `AuCu`, `AuFe`, `CuFe`, `Au`, `Cu`, and `Fe`
  void EntryLoader::loadAlloy(const std::string &alloy, bool recursive) {
    std::vector <std::string> alloy_elements;
    if (alloy.find(", ") != std::string::npos) aurostd::string2tokens(alloy, alloy_elements, ", ");
    else if (alloy.find(',') != std::string::npos) aurostd::string2tokens(alloy, alloy_elements, ",");
    else alloy_elements = aurostd::getElements(alloy);
    loadAlloy(alloy_elements, recursive);
  }

  /// @brief load all entries that are available for a set alloy
  /// @param alloy split alloy elements
  /// @param recursive include sub alloys
  /// @authors
  /// @mod{HE,20220216,created}
  void EntryLoader::loadAlloy(const std::vector <std::string> &alloy, bool recursive) {
    selectSource();
    std::vector <std::string> alloy_clean;
  
    // check that all parts of alloy are actually elements
    for (std::vector<std::string>::const_iterator element = alloy.begin(); element != alloy.end(); element++) {
      if (xelement::xelement::isElement(*element) == 0) {
        m_logger_message << *element << " is not an element";
        outError(__AFLOW_FUNC__, __LINE__);
      } else alloy_clean.emplace_back(*element);
    }

    // build list of all needed sub-alloys
    std::vector<std::vector<std::string>> alloy_sub_list;
    if (recursive) {
      aurostd::xcombos xc;
      for (size_t i = 1; i <= alloy_clean.size(); i++) {
        xc.reset(alloy_clean.size(), i);
        std::vector<int> choice;
        while (xc.increment()) {
          std::vector <std::string> alloy_tmp;
          choice = xc.getCombo();
          for (size_t idx = 0; idx < choice.size(); ++idx) {
            if (choice[idx]) alloy_tmp.push_back(alloy_clean[idx]);
          }
          alloy_sub_list.push_back(alloy_tmp);
        }
      }
    } else {
      alloy_sub_list.push_back(alloy_clean);
    }

    // build sorted alloy strings
    std::vector <std::string> final_alloy_list;
    for (std::vector<std::vector<std::string>>::iterator a = alloy_sub_list.begin(); a != alloy_sub_list.end(); a++) {
      std::string alloy_tmp = "";
      std::sort(a->begin(), a->end());
      for (std::vector<std::string>::iterator e = a->begin(); e != a->end(); e++) alloy_tmp += *e;
      final_alloy_list.push_back(alloy_tmp);
    }
    m_logger_message << "Try loading " << final_alloy_list.size() << " systems: ";
    for (std::vector<std::string>::const_iterator i_alloy = final_alloy_list.begin(); i_alloy != final_alloy_list.end(); i_alloy++) {
      m_logger_message << "\"" << *i_alloy << "\" ";
    }
    outInfo(__AFLOW_FUNC__);

    // load the alloys
    switch (m_current_source) {
      case Source::SQLITE: {
        std::string where = aurostd::joinWDelimiter(final_alloy_list, "','");
        where = "alloy IN ('" + where + "')";
        loadSqliteWhere(where);
        break;
      }

      case Source::AFLUX: {
        std::string alloy_match = aurostd::joinWDelimiter(final_alloy_list, ":");
        std::map <std::string, std::string> matchbook{{"*",     ""},
                                                      {"alloy", alloy_match}}; 
        loadAFLUXMatchbook(matchbook);
        break;
      }

      case Source::RESTAPI: {
        std::vector <std::string> auid_list;
        getAlloyAUIDList(final_alloy_list, auid_list);
        loadAUID(auid_list);
        break;
      }

      case Source::RESTAPI_RAW: {
        loadAlloySearchRR(final_alloy_list, alloy_clean.size());
        break;
      }

      case Source::FILESYSTEM: {
        std::vector <std::string> auid_list;
        getAlloyAUIDList(final_alloy_list, auid_list);
        loadAUID(auid_list);
        break;
      }

      case Source::FILESYSTEM_RAW: {
        loadAlloySearchFSR(final_alloy_list, alloy_clean.size());
        break;
      }

      default:
        break;
    }

  }

  /// @brief load entries based on a custom query against the internal AFLUX SQLITE DB
  /// @param where part of the SQLITE query after a `WHERE` keyword
  /// @authors
  /// @mod{HE,20220422,created}
  /// @note use setSource() to change the current source to Source::SQLITE to ensure that the database is connected
  void EntryLoader::loadSqliteWhere(const string& where){
    vector<string> keywords = {};
    string order_by = "";
    return loadSqliteWhere(where, keywords, order_by);
  }



  void EntryLoader::loadSqliteWhere(const string &where, const vector<string>& keywords, const string& order_by) {
    //WJ20230807 START
    string ob = order_by;
    if (ob == "") ob = m_sqlite_db_ptr->idColumn(); // MB20240711 Needs default ordering or non-AFLUX-OPEN queries will not gen data. //CO20261008 - chaos_id, or auid before
    const vector<string> tables_db = m_sqlite_db_ptr->getTables();  //CO20261008 - was auid_00
    vector<string> keys = tables_db.empty() ? vector<string>() : m_sqlite_db_ptr->getColumnNames(tables_db[0]);
    string keywordString = "";
    m_aflux_version = aflux::AFLUX().m_version; // MB20240814

    // MB20240711
    // [OBSOLETE] bool m_printNull = true;//KC20240309 - tmp fix - avoids seg fault when running entry_loader unit tests
    // [OBSOLETE] if(m_printNull){ //WJ20230720 Changed to only return properties that were queried for
    
    if (keywords.size() > 0) { // Used for custom keyword query, default is *.
      keys.clear();
      for (uint i = 0; i < keywords.size(); i++){
        keys.push_back(keywords[i]);
      }

      if(where.find("alloy") != string::npos){ // FIXME: queries without species might not work
        keys.push_back("species");
      }

      keywordString += aurostd::joinWDelimiter(keys,", ");
    } else { // default query
      keywordString = "*";
    }
    
    vector<vector<string>> content;
    bool LDEBUG=(FALSE || XHOST.DEBUG);
    if(ob.size()){
      content = m_sqlite_db_ptr->getRowsMultiTables(where, keywordString, ob); //WJ20230720 Added ability to change order of results
        if(LDEBUG){cerr << __AFLOW_FUNC__ << " CONTENT " << content.size() << endl;}
    } else{
      content = m_sqlite_db_ptr->getRowsMultiTables(where);
    }
    //WJ20230807 STOP
    m_logger_message << content.size() << " entries found in DB for " << where;
    outDebug(__AFLOW_FUNC__);

    size_t start_size = m_entries_flat->size();
    loadVector(keys, content, where);
    m_logger_message << "Loaded " << m_entries_flat->size() - start_size << " new entries";
    outInfo(__AFLOW_FUNC__);

  }

  /// @brief load entries from a costum AFLUX query
  /// @param query AFLUX query
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note #m_aflux_server and #m_aflux_path will be added
  void EntryLoader::loadAFLUXQuery(const std::string &query) {
    size_t start_size = m_entries_flat->size();
    loadText(getRawAFLUXQuery(query), query);
    m_logger_message << "Loaded " << m_entries_flat->size() - start_size << " new entries";
    outInfo(__AFLOW_FUNC__);
  }

  /// @brief load entries from an AFLUX matchbook
  /// @param matchbook map with keyword and modifier
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note a pair creates `<keyword>(<modifier>)`
  /// @note if modifier is empty just the keyword is used
  void EntryLoader::loadAFLUXMatchbook(const std::map <std::string, std::string> &matchbook) {
      loadAFLUXQuery(buildAFLUXQuery(matchbook));
  }
//WJ20231103 BEGIN - clean AFLUX query back to basic (no new operator "@")
  string EntryLoader::cleanAFLUXquery(const string& query){
      vector<string> parts = aflux::splitStringByOuterDelimeter(query, ',');
      string cleanedQuery = "";
      string rem = "";
      for(uint i = 0; i < parts.size(); i++){
        if(parts[i].find('@') != string::npos){
          rem = parts[i].substr(parts[i].find('('));
          rem = rem.substr(1, rem.size()-2);
          parts[i].replace(parts[i].find(rem),rem.size(),"*");
        }
      }
      cleanedQuery = aurostd::joinWDelimiter(parts,",");
      return cleanedQuery;
    } 
  // WJ END

  

  /// @brief load entries from a list of REST API queries
  /// @authors
  /// @mod{HE,20220216,created}
  /// @param queries vector of queries
  /// @param full_url if `true` don't add #m_restapi_server, #m_restapi_path, and #m_restapi_directives
  void EntryLoader::loadRestAPIQueries(const std::vector <std::string> &queries, bool full_url) {
    size_t start_size = m_entries_flat->size();
    size_t done_downloads = 0;
    for (std::vector<std::string>::const_iterator query = queries.begin(); query != queries.end(); query++) {
      loadText({getRawRestAPIQuery(*query, full_url)}, *query);
      done_downloads++;
      if (done_downloads%100 == 0){
        m_logger_message << "Loaded " << done_downloads << " of " << queries.size();
        outInfo(__AFLOW_FUNC__);
      }
    }
    m_logger_message << "Loaded " << m_entries_flat->size() - start_size << " new entries";
    outInfo(__AFLOW_FUNC__);
  }

  /// @brief load entries from a list of file paths
  /// @param files list of file paths
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note doesn't add #m_filesystem_path or #m_filesystem_collection
  void EntryLoader::loadFiles(const std::vector <std::string> &files) {
    size_t start_size = m_entries_flat->size();
    Source last_source = m_current_source;
    m_current_source = Source::FILESYSTEM_RAW;
    for (std::vector<std::string>::const_iterator file_path = files.begin(); file_path != files.end(); file_path++) {
       std::string file_content;
       if (aurostd::file2string(*file_path, file_content) > 0) {
         loadText({file_content}, *file_path);
       }
    }
    m_logger_message << "Loaded " << m_entries_flat->size() - start_size << " new entries";
    outInfo(__AFLOW_FUNC__);
    m_current_source = last_source;
  }

  /// @brief load a entry from a file path
  /// @param file_path file paths
  /// @authors
  /// @mod{HE,20220913,created}
  /// @note doesn't add #m_filesystem_path or #m_filesystem_collection
  void EntryLoader::loadFiles(const std::string &file_path) {
    loadFiles((vector<std::string>){file_path});
  }

  /// @brief load all entries in the given folders and their sub-folders
  /// @param folders list of folder paths
  /// @authors
  /// @mod{HE,20220913,created}
  /// @note EntryLoader::m_filesystem_outfile (default aflowlib.out) is used to identify entries
  void EntryLoader::loadFolders(const std::vector<std::string> &folders) {
    std::vector <std::string> found_entries;

    char **paths = new char*[folders.size()+1](); // initialize the array with nullptr, +1 ensures it is null terminated
    for (size_t list_idx = 0; list_idx < folders.size(); list_idx++) {
      paths[list_idx] = (char *) folders[list_idx].c_str();
    }

    size_t scanned = 0;
    size_t found = 0;
    struct stat file_stats{};

    // initialize a file tree scan
    // https://man7.org/linux/man-pages/man3/fts.3.html
    // FTS_PHYSICAL - don't follow symlinks
    // FTS_NOCHDIR - don't change the workdir of the program
    // FTS_XDEV - don't descend into folders that are on a different device
    FTS *tree = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR | FTS_XDEV, nullptr);
    FTSENT *node;
    if (tree == nullptr) {
      m_logger_message << "Failed to initialize the file tree used to search for alloys!";
      outHardError(__AFLOW_FUNC__, __LINE__,_FILE_ERROR_);
      return;
    }

    // Iterate over all entries found in the folders listed in paths
    while ((node = fts_read(tree))) {
      scanned += 1;
      if (m_out_debug && ((scanned-1) % 1000 == 0)) {
        m_logger_message << (scanned-1) << " objects scanned; " << found << " entries found; next scan: " << node->fts_path;
        outDebug(__AFLOW_FUNC__);
      }
      if ((node->fts_info & FTS_D)) {
        std::string base_path = node->fts_path;
        std::string full_path = base_path + "/" + m_filesystem_outfile;
        if (stat(full_path.c_str(), &file_stats) == 0) {
          found += 1;
          found_entries.emplace_back(full_path);
        }
      }
    }
    fts_close(tree);
    delete[] paths;

    m_logger_message << "Finishing search in the filesystem after scanning " << scanned << " objects";
    outInfo(__AFLOW_FUNC__);
    Source last_source = m_current_source;
    m_current_source = Source::FILESYSTEM_RAW;
    loadFiles(found_entries);
    m_current_source = last_source;
  }

  /// @brief load all entries recursive in the given folder and its sub-folders
  /// @param folder_path folder path
  /// @authors
  /// @mod{HE,20220913,created}
  /// @note EntryLoader::m_filesystem_outfile (default aflowlib.out) is used to identify entries
  void EntryLoader::loadFolders(const std::string &folder_path) {
    loadFolders((vector<std::string>){folder_path});
  }

  /// @brief load entries from a list of strings
  /// @param raw_data_lines list of data strings
  /// @param source string describing the source, like a SQL query or a path
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note each entry in raw_data_lines should correspond to one AFLOW lib entry
  void EntryLoader::loadText(const std::vector <std::string> &raw_data_lines, const std::string &source) {
    for (std::vector<std::string>::const_iterator line = raw_data_lines.begin(); line != raw_data_lines.end(); line++) {
      std::shared_ptr <aflowlib::_aflowlib_entry> entry = std::make_shared<aflowlib::_aflowlib_entry>();
      entry->Load(*line, *p_oss);
      entry->el_source_type = getSourceString();
      entry->el_source = source;
      if (!entry->auid.empty() && (std::find(m_auid_list.begin(),m_auid_list.end(), entry->auid) == m_auid_list.end())) {
      //WJ20231103 BEGIN added filtering on atomic and tensor properties
        if(applyCPPFiltersToSingleEntry(*entry)){
          m_entries_flat->push_back(entry);
            auto& vlayer=(*m_entries_layered_map)[entry->nspecies][entry->species_pp]; //CO20260930
            vlayer.push_back(entry);
            m_auid_list.emplace_back(entry->auid);
            if (m_xstructure_original) addXstructure(*entry, true);
            if (m_xstructure_relaxed) addXstructure(*entry);
        }
      }
      //WJ END
    }
  }

  bool EntryLoader::applyCPPFiltersToSingleEntry(const aflowlib::_aflowlib_entry& entry){
    // MB20240814 This may not distinguish between versions correctly but it is better than just checking for "s4e.ai"
    if(m_aflux_version >= 200) { 
      return aflux::atomicFilterSingleEntry(entry, m_aflux_atomic_filter_keywords, m_aflux_atomic_filter_params) && aflux::filterSpecialCPP(entry, m_aflux_special_filter_map);
    } else{
      return aflux::atomicFilterSingleEntry(entry, m_aflux_atomic_filter_keywords, m_aflux_atomic_filter_params) && aflux::tensorFilterSingleEntry(entry, m_aflux_tensor_filter_keywords, m_aflux_tensor_filter_params) && aflux::filterSpecialCPP(entry, m_aflux_special_filter_map);
    }
  }

  /// @brief load entries from vectors
  /// @param keys list of keys
  /// @param content list of keys
  /// @param source string describing the source, like a SQL query or a path
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note each entry in content should correspond to one AFLOW lib entry
  void EntryLoader::loadVector(const std::vector<std::string> &keys, const std::vector<std::vector<std::string>> & content, const std::string & source) {
    std::vector<uint64_t> hash_list;
    for (std::vector<std::string>::const_iterator key = keys.begin(); key != keys.end(); key++) hash_list.emplace_back(aurostd::crc64(*key));
    for (std::vector<std::vector<std::string>>::const_iterator row=content.begin(); row!=content.end(); row++) {
      std::shared_ptr <aflowlib::_aflowlib_entry> entry = std::make_shared<aflowlib::_aflowlib_entry>();
      entry->Load(hash_list, *row);
      entry->el_source_type = getSourceString();
      entry->el_source = source;
      if (!entry->auid.empty() && (std::find(m_auid_list.begin(),m_auid_list.end(), entry->auid) == m_auid_list.end())) {
      //WJ20231103 BEGIN added filtering on atomic and tensor properties
        if(applyCPPFiltersToSingleEntry(*entry)){
          m_entries_flat->push_back(entry);
            auto& vlayer=(*m_entries_layered_map)[entry->nspecies][entry->species_pp]; //CO20260930
            vlayer.push_back(entry);
            m_auid_list.emplace_back(entry->auid);
            if (m_xstructure_original) addXstructure(*entry, true);
            if (m_xstructure_relaxed) addXstructure(*entry);
        }
      }
      //WJ END
    }
  }

  /// @brief returns the active data source
  EntryLoader::Source EntryLoader::getSource() const{
    return m_current_source;
  }

  /// @brief returns the active data source as string
  std::string EntryLoader::getSourceString() const {
    switch(m_current_source) {
      case Source::SQLITE: return "SQLITE";
      case Source::AFLUX: return "AFLUX";
      case Source::FILESYSTEM: return "FILESYSTEM";
      case Source::FILESYSTEM_RAW: return "FILESYSTEM_RAW";
      case Source::RESTAPI: return "RESTAPI";
      case Source::RESTAPI_RAW: return "RESTAPI_RAW";
      case Source::NONE: return "NONE";
      case Source::FAILED: return "FAILED";
      default: return "NONE";
    }
  }

  /// @brief change the currently used source and prepares them
  /// @param new_source source to change to
  /// @param silent silent all logging when called from selectSource() (default false)
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note if the public alloy SQLITE DB is not found Source::FILESYSTEM and Source::RESTAPI
  ///       are mapped to Source::FILESYSTEM_RAW and Source::RESTAPI_RAW respectively
  bool EntryLoader::setSource(EntryLoader::Source new_source) {
    if (new_source == m_current_source) return true;
    m_current_source = Source::FAILED;
    m_filesystem_available = aurostd::IsDirectory(m_filesystem_path + "AUID/");

    switch (new_source) {
      case Source::SQLITE: {
        if (aurostd::FileExist(m_sqlite_file)) {
          if (m_sqlite_db_ptr == nullptr) {
            std::unique_ptr<aflowlib::AflowDB> tmpPtr(new aflowlib::AflowDB(m_sqlite_file));
            m_sqlite_db_ptr = std::move(tmpPtr);
          }
          m_current_source = Source::SQLITE;
          return true;
        } else {
          m_logger_message << "Internal AFLUX SQLITE DB not found at " << m_sqlite_file;
          outError(__AFLOW_FUNC__, __LINE__);
        }
        break;
      }

      case Source::AFLUX: { //NOTE: WJ20231103: We should add a similar test page to s4e
        m_current_source = Source::AFLUX; 
        return true;
        if ("AFLUXtest" == aurostd::httpGet(m_aflux_server+"/test/?echo=AFLUXtest")) { //WJ comented out test 
          m_current_source = Source::AFLUX;
          cout << "TEST PASSED " << endl;
          return true;
        } else {
          m_logger_message << "AFLUX API could not be reached at " << m_aflux_server;
          outError(__AFLOW_FUNC__, __LINE__);
        }
        break;
      }

      case Source::FILESYSTEM: {
        if (m_filesystem_available) {
          if (aurostd::FileExist(m_sqlite_alloy_file)) { //
            if (m_sqlite_alloy_db_ptr == nullptr) {
              std::unique_ptr<aflowlib::AflowDB> tmpPtr(new aflowlib::AflowDB(m_sqlite_alloy_file));
              m_sqlite_alloy_db_ptr = std::move(tmpPtr);
            }
            m_current_source = Source::FILESYSTEM;
            return true;
          } else {
            m_logger_message << "Could not find public alloy SQLITE DB at " << m_sqlite_alloy_file << "; switching to Source::FILESYSTEM_RAW";
            outInfo(__AFLOW_FUNC__);
            return setSource(Source::FILESYSTEM_RAW);
          }
        } else {
          m_logger_message << "Could not find AFLOW entries in the filesystem at " << m_filesystem_path;
          outError(__AFLOW_FUNC__, __LINE__);
        }
        break;
      }

      case Source::FILESYSTEM_RAW: {
        if (m_filesystem_available) { //
          if (aurostd::FileExist(m_sqlite_alloy_file)) { // use the alloy DB to optimize the RAW results
            if (m_sqlite_alloy_db_ptr == nullptr) {
              std::unique_ptr<aflowlib::AflowDB> tmpPtr(new aflowlib::AflowDB(m_sqlite_alloy_file));
              m_sqlite_alloy_db_ptr = std::move(tmpPtr);
            }
          }
          m_current_source = Source::FILESYSTEM_RAW;
          return true;
        } else {
          m_logger_message << "Could not find AFLOW entries in the filesystem at " << m_filesystem_path;
          outError(__AFLOW_FUNC__, __LINE__);
        }
        break;
      }

      case Source::RESTAPI: {
        if (200 == aurostd::httpGetStatus(m_restapi_server+m_restapi_path+"ICSD_WEB/")) {
          if (aurostd::FileExist(m_sqlite_alloy_file)) {
            if (m_sqlite_alloy_db_ptr == nullptr) {
              std::unique_ptr<aflowlib::AflowDB> tmpPtr(new aflowlib::AflowDB(m_sqlite_alloy_file));
              m_sqlite_alloy_db_ptr = std::move(tmpPtr);
            }
            m_current_source = Source::RESTAPI;
            return true;
          } else {
            m_logger_message << "Could not find public alloy SQLITE DB at " << m_sqlite_alloy_file << "; switching to Source::RESTAPI_RAW";
            outInfo(__AFLOW_FUNC__);
            return setSource(Source::RESTAPI_RAW);
          }
        } else {
          m_logger_message << "AFLOW REST API could not be reached at " << m_aflux_server+m_restapi_path;
          outError(__AFLOW_FUNC__, __LINE__);
        }
        break;
      }

      case Source::RESTAPI_RAW: {
        if (200 == aurostd::httpGetStatus(m_restapi_server+m_restapi_path+"ICSD_WEB/")) {
          if (aurostd::FileExist(m_sqlite_alloy_file)) {
            if (m_sqlite_alloy_db_ptr == nullptr) {
              std::unique_ptr<aflowlib::AflowDB> tmpPtr(new aflowlib::AflowDB(m_sqlite_alloy_file));
              m_sqlite_alloy_db_ptr = std::move(tmpPtr);
            }
          }
          m_current_source = Source::RESTAPI_RAW;
          return true;
        }
        else {
          m_logger_message << "AFLOW REST API could not be reached at " << m_aflux_server+m_restapi_path;
          outError(__AFLOW_FUNC__, __LINE__);
        }
        break;
      }

      case Source::NONE: {
        m_current_source = Source::NONE;
        return true;
      }

      case Source::FAILED: {
        m_current_source = Source::FAILED;
        return true;
      }

    }
    return false;
  }

  /// @brief query the internal AFLUX SQLITE DB
  /// @param where part of the SQLITE query after a `WHERE` keyword
  /// @authors
  /// @mod{HE,20220216,created}
  /// @return list of result query strings
  std::vector <std::string> EntryLoader::getRawSqliteWhere(const std::string &where) const{
    if (m_sqlite_db_ptr != nullptr) return m_sqlite_db_ptr->getEntrySet(where, aflow_ft);
    else return {};
  }

  /// @brief query AFLUX using a matchbook
  /// @param matchbook map with keyword and modifier
  /// @authors
  /// @mod{HE,20220216,created}
  /// @return list of query result strings
  std::vector <std::string> EntryLoader::getRawAFLUXMatchbook(const std::map <std::string, std::string> &matchbook){
    return getRawAFLUXQuery(buildAFLUXQuery(matchbook));
  }

  /// @brief query AFLUX using a raw query
  /// @param matchbook map with keyword and modifier
  /// @authors
  /// @mod{HE,20220216,created}
  /// @return list of query result strings
  /// @note #m_aflux_server and #m_aflux_path will be added
  std::vector <std::string> EntryLoader::getRawAFLUXQuery(const std::string &orignal_query){
    std::string output = "";
    std::vector <std::string> raw_lines;
    std::string query = orignal_query;
    aflux::AFLUX trans;
    trans.forceAflowFormat(query);
    //WJ20231103 Begin
    string cleanedQuery = cleanAFLUXquery(query) += ",composition(*)"; //WJ20231103 Must include composition in order to do atomic filtering later
    vector<string> outVec;
    
    trans.parseQuery(query);
    const bool page_all = AFLUXPagingAll(trans); //CO20261004 - paging(0) as AFLUX's parser read the query (AFLUXPageQuery() looked for it with two regexes)
    m_aflux_atomic_filter_keywords = trans.m_atomicFilterKeywords;
    m_aflux_atomic_filter_params = trans.m_atomicFilterParams;
    m_aflux_tensor_filter_keywords = trans.m_tensorFilterKeywords;
    m_aflux_tensor_filter_params = trans.m_tensorFilterParams;
    m_aflux_special_filter_map = trans.m_specialFilterMap;

    for (uint i = 0; i < m_aflux_server_vector.size(); i++){ //WJ20231103 Iterate through list of servers to query
      m_aflux_server = m_aflux_server_vector[i];
      m_aflux_path = m_aflux_path_vector[i];


      // MB20240805 Checking Aflux version is more formal than check if server is s4e.ai
      aflux::AFLUX::Version ver = trans.checkAFLUXVersion(m_aflux_server, m_aflux_path);
      m_aflux_version = ver;

      //CO20261002 START - one fetch path for both versions; paging(0) is sent as pages (AFLUXPageQuery) //CO20261004 - when page_all
      std::string send_query;
      switch (ver) {
        case(aflux::AFLUX::Version::v200): send_query = query; break; //WJ20231103 No need to clean query when querying latest ver
        case(aflux::AFLUX::Version::v100): send_query = cleanedQuery; break; // Older version of AFLUX
        default:
          m_logger_message << "Unknown AFLUX version: ";
          m_logger_message << trans.m_version << " ";
          m_logger_message << "(" << m_aflux_server << " | " << m_aflux_path << " | " << query << ")";
          outError(__AFLOW_FUNC__, __LINE__);
          continue;
      }
      int status = -1;
      if (getAFLUXPages(send_query, page_all, raw_lines, status)) { //CO20261004 - page_all
        outVec.insert(outVec.begin(), raw_lines.begin(), raw_lines.end()); //WJ append to total list
      } else {
        m_logger_message << "Failed to get AFLUX query ";
        m_logger_message << "(" << m_aflux_server << " | " << m_aflux_path << " | " << query << ")";
        if (status < 0) m_logger_message << " - no complete answer (network error, or cut off in transit)";
        else m_logger_message << " - HTTP status " << status;
        const std::string hint = aurostd::httpChaosApiKeyHint(m_aflux_server, status);
        if (!hint.empty()) m_logger_message << " - " << hint;
        outError(__AFLOW_FUNC__, __LINE__);
      }
      //CO20261002 STOP
    }
    return outVec;
  }

  //CO20261002 START
  /// @brief run an AFLUX query on #m_aflux_server / #m_aflux_path, page by page when it asks for paging(0)
  /// @param query AFLUX query (plain or percent-encoded, with the leading `?`)
  /// @param page_all the query asks for paging(0) in format(aflow), as AFLUX's parser reads it (AFLUXPagingAll()) //CO20261004
  /// @param lines result lines (one entry per line in format(aflow))
  /// @param status HTTP status of the last request
  /// @return `true` when every request answered 200
  /// @note paging(0) returns the whole result set in one answer; the s4e.ai gate refuses it, so it
  ///       is sent as paging(1,k), paging(2,k), ... until a page has fewer than k entries. Pages
  ///       follow the server's order (by auid for alloy queries), which is the same in every request.
  ///       A page cut off in transit is a failed request (curl's exit status, Content-Length and the
  ///       last chunk are checked for s4e.ai), so a short page is the last page.
  bool EntryLoader::getAFLUXPages(const std::string &query, bool page_all, std::vector<std::string> &lines, int &status) {
    lines.clear();
    std::string output;
    const uint page_size = AFLUX_PAGE_SIZE;
    if (!page_all || AFLUXPageQuery(query, 1, page_size).empty()) { // bounded query: one request //CO20261004 - the caller decides (page_all)
      status = aurostd::httpGetStatus(m_aflux_server, m_aflux_path, query, output);
      if (status != 200) return false;
      aurostd::string2vectorstring(output, lines);
      return true;
    }
    std::string first_line_previous;
    for (uint page = 1; page <= AFLUX_MAX_PAGES; page++) {
      status = aurostd::httpGetStatus(m_aflux_server, m_aflux_path, AFLUXPageQuery(query, page, page_size), output);
      if (status != 200) return false;
      std::vector<std::string> page_lines, raw;
      aurostd::string2vectorstring(output, raw);
      for (size_t l = 0; l < raw.size(); l++) {
        if (!aurostd::RemoveWhiteSpaces(raw[l]).empty()) page_lines.push_back(raw[l]);
      }
      if (page_lines.empty()) return true;
      if (page > 1 && page_lines[0] == first_line_previous) { // the server ignored paging: keep one copy
        m_logger_message << "AFLUX server ignored paging(" << page << "," << page_size << ")";
        outError(__AFLOW_FUNC__, __LINE__);
        return true;
      }
      first_line_previous = page_lines[0];
      lines.insert(lines.end(), page_lines.begin(), page_lines.end());
      if (page_lines.size() < page_size) return true;
    }
    m_logger_message << "AFLUX query stopped after " << AFLUX_MAX_PAGES << " pages of " << page_size;
    outError(__AFLOW_FUNC__, __LINE__);
    status = -1;
    return false;
  }

  /// @brief AUIDs or AURLs as one AFLUX value: joined with ':' (AFLUX's OR), without quotes
  /// @param values AUIDs ("s4e:0171c43afba95d07") or AURLs ("s4e.ai:AFLOWDATA/LIB3_WEB/...")
  /// @return the value to put between the parentheses of auid(...) or aurl(...)
  /// @note s4e.ai's AFLUX engine (aflow_aflux.cpp) does not read quoted values: it splits a value
  ///       at every ':', then joins "s4e" with the AUID and "s4e.ai" with the AURL path again
  ///       (AFLUX::correctSQLforAUIDs()). A quoted value ('s4e:...', as the earlier AFLUX servers
  ///       took it) kept its quotes in the SQL and matched nothing. An AURL with a ':' after the
  ///       host (POCC or LIB1 run names) is still split there; the engine has no way to escape it.
  std::string AFLUXValueList(const std::vector<std::string> &values) { //CO20261003
    return aurostd::joinWDelimiter(values, ":");
  }

  //CO20261004 START
  /// @brief AUIDs or AURLs in batches, each batch one AFLUX value from AFLUXValueList()
  /// @param values AUIDs or AURLs
  /// @param max_length a value joins the current batch while the batch and the value together
  ///        (without the ':') are shorter than this
  /// @return one value per batch, in order; none for an empty list
  /// @note one helper for loadAUID() and loadAURL() (with #AFLUX_VALUE_LIST_MAX); a value too long on
  ///       its own gets a batch of its own. The loops this replaces sent an empty batch for an empty
  ///       list or a first value that was too long, which made the query auid or aurl without a value
  std::vector<std::string> AFLUXValueBatches(const std::vector<std::string> &values, size_t max_length) {
    std::vector<std::string> batches, batch;
    size_t batch_length = 0;  // length of AFLUXValueList(batch)
    for (size_t i = 0; i < values.size(); i++) {
      if (!batch.empty() && batch_length + values[i].size() >= max_length) {
        batches.push_back(AFLUXValueList(batch));
        batch.clear();
      }
      batch_length = (batch.empty() ? 0 : batch_length + 1) + values[i].size();
      batch.push_back(values[i]);
    }
    if (!batch.empty()) {batches.push_back(AFLUXValueList(batch));}
    return batches;
  }
  //CO20261004 STOP

  /// @brief the query for one page of an AFLUX query: its paging term set to paging(page,page_size)
  /// @param query AFLUX query, plain or percent-encoded, with or without the leading `?`
  /// @param page page number (from 1)
  /// @param page_size entries per page
  /// @return "?" and the query, percent-encoded as buildAFLUXQuery() encodes it, or "" when the query
  ///         has no paging term
  /// @note CO20261004 - the terms are decoded (aflux::decodeURL), split (aflux::splitStringByOuterDelimeter),
  ///       joined (aurostd::joinWDelimiter) and encoded again (aurostd::httpPercentEncodingFull), so plain and
  ///       encoded queries take one path (there were two regexes, one per form); whether a query is to be paged
  ///       is decided by AFLUX's parser (AFLUXPagingAll()), not here
  std::string AFLUXPageQuery(const std::string &query, uint page, uint page_size) {
    std::vector<std::string> terms = AFLUXQueryTerms(query);
    bool found = false;
    for (size_t i = 0; i < terms.size(); i++) {
      const std::string name = AFLUXTermName(terms[i]);
      if (aurostd::RemoveCharacterFromTheFront(name, '$') != "paging") {continue;}
      terms[i] = name + "(" + aurostd::utype2string<uint>(page) + "," + aurostd::utype2string<uint>(page_size) + ")";  // a '$' stays
      found = true;
    }
    if (!found) {return "";}
    return "?" + aurostd::httpPercentEncodingFull(aurostd::joinWDelimiter(terms, ","));
  }

  /// @brief does an AFLUX query ask for its whole result in one answer: paging(0) in format(aflow)
  /// @param query AFLUX query, plain or percent-encoded, with or without the leading `?`
  /// @return `true` when it should be sent as pages (EntryLoader::getAFLUXPages())
  /// @note for a query AFLUX's parser has not read yet (AFLUXCall()); only its paging and format terms go to
  ///       the parser, which does not know every keyword a server knows and would stop at one
  bool AFLUXPagingAll(const std::string &query) { //CO20261004
    try {
      std::vector<std::string> directives;
      const std::vector<std::string> terms = AFLUXQueryTerms(query);
      for (size_t i = 0; i < terms.size(); i++) {
        const std::string name = aurostd::RemoveCharacterFromTheFront(AFLUXTermName(terms[i]), '$');
        if (name == "paging" || name == "format") {directives.push_back(terms[i]);}
      }
      if (directives.empty()) {return false;}
      aflux::AFLUX parsed;
      parsed.parseQuery("?" + aurostd::joinWDelimiter(directives, ","));
      return AFLUXPagingAll(parsed);
    }
    catch (aurostd::xerror &) {return false;}   // an unknown format: sent as it is, the server answers
    catch (std::exception &) {return false;}    // a broken percent sign (decodeURL)
  }
  //CO20261002 STOP

  /// @brief query the REST API
  /// @param query REST API query
  /// @param full_url if `true` don't add #m_restapi_server, #m_restapi_path, and #m_restapi_directives
  /// @authors
  /// @mod{HE,20220216,created}
  /// @return query result string
  std::string EntryLoader::getRawRestAPIQuery(const std::string &query, bool full_url) {
    std::string output = "";
    std::string url;
    if (full_url) url = query;
    else url = m_restapi_server + m_restapi_path + query + m_restapi_directives;
    if (200 == aurostd::httpGetStatus(url, output)){
      return output;
    } else {
      m_logger_message << "Failed to get REST API query " << "(" << url << ")";
      outError(__AFLOW_FUNC__, __LINE__);
      return "";
    }
  }

  /// @brief add an xstructure to an AFLOW lib entry
  /// @param entry AFLOW lib entry
  /// @param orig if `true` load the original structure otherwise the final structure (default)
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note use this function only if you want the xstructure for a subset of entries;
  ///       if you want xstructures for all entries set #m_xstructure_relaxed or #m_xstructure_original to `true`
  /// @note adds the structure to entry.vstr
  void EntryLoader::addXstructure(aflowlib::_aflowlib_entry &entry, bool orig) {
    if (orig) {
      xstructure new_structure;
      if (loadXstructureAflowIn(entry, new_structure, 1)) { // load from aflow.in
        entry.vstr.push_back(new_structure);
      } else {
        m_logger_message << "Failed to add original structure to " << entry.auid << " (" << entry.aurl << ")";
        outError(__AFLOW_FUNC__, __LINE__);
      }
    } else {
      xstructure new_structure;
      if (pflow::loadXstructureLibEntry(entry, new_structure)) { // load directly from the entry (AFLUX, SQLITE)
        entry.vstr.push_back(new_structure);
      } else if (loadXstructureFile(entry, new_structure)){
        entry.vstr.push_back(new_structure);
      } else if (loadXstructureAflowIn(entry, new_structure, -1)) { // load from aflow.in
        entry.vstr.push_back(new_structure);
      } else {
        m_logger_message << "Failed to add relaxed structure to " << entry.auid << " (" << entry.aurl << ")";
        outError(__AFLOW_FUNC__, __LINE__);
      }
    }
  }

  /// @brief load a structure from a structure file
  /// @param entry AFLOW lib entry
  /// @param new_structure save-to structure
  /// @param possible_files load the structure from the first exiting sources given (if list is empty use m_xstructure_final_file_name)
  /// @return xstructure
  /// @note does not add the structure to entry.vstr
  bool EntryLoader::loadXstructureFile(const aflowlib::_aflowlib_entry &entry, xstructure &new_structure, std::vector <std::string> possible_files) {
    std::string base_url = m_restapi_server + m_restapi_path + aflowlib::AURLPath(entry.aurl) + "/";
    std::string base_folder;
    
    // MB20240711 START Add S4E File loading path for XStructure Unit Tests
    std::string aurl = entry.aurl;
    std::string org = aurl.substr(0, aurl.find(':'));

    std::string proto_end = aurl.substr(aurl.rfind('.')+1); 
    uint proto_len = proto_end.size();

    std::deque<string> tokens;
    aurostd::string2tokens(aurl, tokens, "/");
    std::string decoration_path = *(tokens.end() - 2);
    std::string proto_path = *(tokens.end() - 1);

    bool direct_fs = entry.el_source_type.substr(0, 10) == "FILESYSTEM"; // catch FILESYSTEM and FILESYSTEM_RAW

    if (org == "s4e.ai") { // S4E FILESYS
      base_folder = m_filesystem_path;
      base_folder += "LIB" + aurostd::utype2string<uint>(proto_len);
      base_folder += "/LIB/" + decoration_path + "/" + proto_path;
      // MB20240711 STOP
    } else { // AFLOWDATA layout (any other host)
      if (direct_fs){
        base_folder = entry.el_source.substr(0, entry.el_source.size()-m_filesystem_outfile.size()); // KC 20240503 - loading Xtstructure unit test fails here. Looks like we structure our filesystem differently than aflowOrg
      } else {
        base_folder = m_filesystem_path + aflowlib::AURLPath(entry.aurl) + "/";
        base_folder = std::regex_replace(base_folder, aurostd::regex_aurl2file, "$1/" + m_filesystem_collection + "/");
      } 
    }
    std::string poscar;
    if (entry.catalog =="LIB0" && !m_filesystem_available && entry.aurl.substr(entry.aurl.size() - 2)=="/0"){
      return false; // no entries in RESTAPI for WEB0 /0
    }

    // if no file names given use the class defaults
    if (possible_files.empty()) possible_files = m_xstructure_final_file_name;

    std::vector <std::string> available_files;
    if (m_filesystem_available || direct_fs) {
      aurostd::DirectoryLS(base_folder, available_files);
    } else {
      listRestAPI(base_url, available_files, false);
    }

    std::string selected_file;
    for (std::vector<std::string>::const_iterator file_name = possible_files.begin(); file_name != possible_files.end(); file_name++) {
      if (aurostd::EWithinList(available_files, *file_name, selected_file)) {
        if (m_filesystem_available || direct_fs) aurostd::efile2string(base_folder + selected_file, poscar);
        else poscar = getRawRestAPIQuery(base_url + selected_file, true);
        if (!poscar.empty()) { // load from aflow.in
          new_structure = xstructure((std::stringstream) poscar, IOVASP_AUTO);
          return true;
        }
      }
    }
    return false;
  }

  /// @brief load the first structure in an `aflow.in` file
  /// @param entry AFLOW lib entry
  /// @param new_structure save-to structure
  /// @authors
  /// @mod{HE,20220216,created}
  /// @return xstructure
  /// @note this is always the original structure
  /// @note does not add the structure to entry.vstr
  bool EntryLoader::loadXstructureAflowIn(const aflowlib::_aflowlib_entry &entry, xstructure &new_structure, const int index) {
    std::string base_folder;
    bool direct_fs = entry.el_source_type.substr(0, 10) == "FILESYSTEM"; // catch FILESYSTEM and FILESYSTEM_RAW
    if (direct_fs){
      base_folder = entry.el_source.substr(0, entry.el_source.size()-m_filesystem_outfile.size());
    } else {
      base_folder = m_filesystem_path + aflowlib::AURLPath(entry.aurl) + "/";
      base_folder = std::regex_replace(base_folder, aurostd::regex_aurl2file, "$1/" + m_filesystem_collection + "/");
    }
    std::string base_url = m_restapi_server + m_restapi_path + aflowlib::AURLPath(entry.aurl) + "/";
    std::string aflowin_content;

    if (m_filesystem_available || direct_fs) {
      std::stringstream buffer;
      std::ifstream open_file(base_folder + "aflow.in");
      buffer << open_file.rdbuf();
      aflowin_content = buffer.str();
    } else { // load form REST API
      m_out_super_silent = true;
      aflowin_content = getRawRestAPIQuery(base_url + "aflow.in", true);
      m_out_super_silent = false;
    }
    std::stringstream poscar;
    bool poscar_loaded =  KBIN::ExtractPOSCARStringStreamFromAFLOWIN(aflowin_content, poscar, index);
    if (poscar_loaded) { // load from aflow.in
      new_structure = xstructure((std::stringstream) poscar.str(), IOVASP_AUTO);
      return true;
    }
    return false;
  }

  /// @brief save the shared pointers to the AFLOW lib entries into an external vector
  /// @param result save-to vector
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note the underlying entries will not be copied and are likely not in a continuous chunk of memory
  /// @note creating a copy the smart pointer #m_entries_flat is a bit more efficient way to use
  ///       the loaded entries after the EntryLoader class goes out of scope
  void EntryLoader::getEntriesViewFlat(std::vector <std::shared_ptr<aflowlib::_aflowlib_entry>> &result) const {
    result = *m_entries_flat;
  }

  /// @brief save the shared pointers to the AFLOW lib entries into a two layer vector
  /// @param result save-to vector
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note the underlying entries will not be copied and are likely not in a continuous chunk of memory
  /// @note the entries are grouped by number of elements (smallest to largest)
  void EntryLoader::getEntriesViewTwoLayer(vector<vector<std::shared_ptr < aflowlib::_aflowlib_entry>> > &result) const{
  for (std::map<short, std::map<std::string, std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>>::iterator layer1 = m_entries_layered_map->begin(); layer1 != m_entries_layered_map->end(); layer1++) {
  std::vector <std::shared_ptr<aflowlib::_aflowlib_entry>> collected_entries;
    for (std::map<std::string, std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>::iterator layer2 = layer1->second.begin(); layer2 != layer1->second.end(); layer2++) {
      for (std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>::iterator entry = layer2->second.begin(); entry != layer2->second.end(); entry++) collected_entries.push_back(*entry);
      }
      result.push_back(collected_entries);
    }
  }

  /// @brief save the shared pointers to the AFLOW lib entries into a three layer vector
  /// @param result save-to vector
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note the underlying entries will not be copied and are likely not in a continuous chunk of memory
  /// @note the entries are grouped first by number of elements (smallest to largest) and then by alloy
  void EntryLoader::getEntriesViewThreeLayer(std::vector<std::vector<std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>> &result) const{
    for (std::map<short, std::map<std::string, std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>>::iterator layer1 = m_entries_layered_map->begin(); layer1 != m_entries_layered_map->end(); layer1++) {
      std::vector < std::vector < std::shared_ptr < aflowlib::_aflowlib_entry>>> collected_entries_l1;
      for (std::map<std::string, std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>::iterator layer2 = layer1->second.begin(); layer2 != layer1->second.end(); layer2++) {
        std::vector <std::shared_ptr<aflowlib::_aflowlib_entry>> collected_entries_l2;
        for (std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>::iterator entry = layer2->second.begin(); entry != layer2->second.end(); entry++) collected_entries_l2.push_back(*entry);
        collected_entries_l1.push_back(collected_entries_l2);
      }
      result.push_back(collected_entries_l1);
    }
  }

  /// @brief save the shared pointers to the AFLOW lib entries into a three layer map
  /// @param result save-to map
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note the underlying entries will not be copied and are likely not in a continuous chunk of memory
  /// @note the entries are grouped first by number of elements and then by alloy
  /// @note creating a copy of the smart pointer #m_entries_layered_map is a bit more efficient way to use
  ///       the loaded entries after the EntryLoader class goes out of scope
  void EntryLoader::getEntriesViewMap(std::map < short, std::map < std::string,
                                      std::vector < std::shared_ptr < aflowlib::_aflowlib_entry >> >> &result) const {
    result = *m_entries_layered_map;
  }

  /// @brief copy the AFLOW lib entries into a vector
  /// @param result save-to vector
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note the underlying entries are copied into a continuous chunk of memory
  void EntryLoader::getEntriesFlat(std::vector <aflowlib::_aflowlib_entry> &result) const {
    for (std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>::iterator entry = m_entries_flat->begin(); entry != m_entries_flat->end(); entry++){
      result.emplace_back(*(*entry));
    }
  }

  /// @brief copy the AFLOW lib entries into a two layer vector
  /// @param result save-to vector
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note the underlying entries are copied into a continuous chunk of memory
  /// @note the entries are grouped by number of elements (smallest to largest)
  void EntryLoader::getEntriesTwoLayer(std::vector <std::vector<aflowlib::_aflowlib_entry>> &result) const {
    for (std::map<short, std::map<std::string, std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>>::iterator layer1 = m_entries_layered_map->begin(); layer1 != m_entries_layered_map->end(); layer1++) {
      std::vector <aflowlib::_aflowlib_entry> collected_entries;
      for (std::map<std::string, std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>::iterator layer2 = layer1->second.begin(); layer2 != layer1->second.end(); layer2++) {
        for (std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>::iterator entry = layer2->second.begin(); entry != layer2->second.end(); entry++) collected_entries.push_back(*(*entry));
      }
      result.push_back(collected_entries);
    }
  }


  /// @brief copy the AFLOW lib entries into a three layer vector
  /// @param result save-to vector
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note the underlying entries are copied into a continuous chunk of memory
  /// @note the entries are grouped first by number of elements (smallest to largest) and then by alloy
  void EntryLoader::getEntriesThreeLayer(std::vector<std::vector<vector<aflowlib::_aflowlib_entry>>> & result) const {
    for (std::map<short, std::map<std::string, std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>>::iterator layer1 = m_entries_layered_map->begin(); layer1 != m_entries_layered_map->end(); layer1++) {
      std::vector <std::vector<aflowlib::_aflowlib_entry>> collected_entries_l1;
      for (std::map<std::string, std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>>::iterator layer2 = layer1->second.begin(); layer2 != layer1->second.end(); layer2++) {
        std::vector <aflowlib::_aflowlib_entry> collected_entries_l2;
        for (std::vector<std::shared_ptr<aflowlib::_aflowlib_entry>>::iterator entry = layer2->second.begin(); entry != layer2->second.end(); entry++) collected_entries_l2.push_back(*(*entry));
        collected_entries_l1.push_back(collected_entries_l2);
      }
      result.push_back(collected_entries_l1);
    }
  }

  // private functions
  /// @brief find the best available source
  /// @authors
  /// @mod{HE,20220216,created}
  void EntryLoader::selectSource() {
    if (m_current_source == Source::NONE){
      EntryLoader::getCommandlineSource();
    }

    if (m_current_source == Source::NONE || m_current_source == Source::FAILED) {
      m_out_super_silent = true;
      if (EntryLoader::setSource(Source::SQLITE)) {
        m_out_super_silent = false;
        m_logger_message << "Automatically selected Source::SQLITE";
        outInfo(__AFLOW_FUNC__);
        return;
      }
      if (EntryLoader::setSource(Source::AFLUX)) {
        m_out_super_silent = false;
        m_logger_message << "Automatically selected Source::AFLUX";
        outInfo(__AFLOW_FUNC__);
        return;
      }
      if (EntryLoader::setSource(Source::FILESYSTEM)) {
        m_out_super_silent = false;
        m_logger_message << "Automatically selected";
        if (m_current_source==Source::FILESYSTEM) m_logger_message << " Source::FILESYSTEM";
        else if (m_current_source==Source::FILESYSTEM_RAW) m_logger_message << " Source::FILESYSTEM_RAW";
        outInfo(__AFLOW_FUNC__);
        return;
      }
      if (EntryLoader::setSource(Source::RESTAPI)) {
        m_out_super_silent = false;
        m_logger_message << "Automatically selected";
        if (m_current_source==Source::RESTAPI) m_logger_message << " Source::RESTAPI";
        else if (m_current_source==Source::RESTAPI_RAW) m_logger_message << " Source::RESTAPI_RAW";
        outInfo(__AFLOW_FUNC__);
        return;
      }
      EntryLoader::setSource(Source::FAILED);
      m_out_super_silent = false;
      m_logger_message << "Failed to find a working source!";
      outHardError(__AFLOW_FUNC__, __LINE__, _GENERIC_ERROR_);
    }
  }

  bool EntryLoader::getCommandlineSource(){
    string raw_source = XHOST.vflag_control.getattachedscheme("ENTRY_LOADER::SOURCE");
    raw_source = aurostd::tolower(raw_source);
    if (raw_source == "aflux") return EntryLoader::setSource(Source::AFLUX);
    else if (raw_source == "sqlite") return EntryLoader::setSource(Source::SQLITE);
    else if (raw_source == "file" || raw_source == "filesystem") return EntryLoader::setSource(Source::FILESYSTEM);
    else if (raw_source == "rest" || raw_source == "restapi") return EntryLoader::setSource(Source::RESTAPI);
    else return false;
  }

  /// @brief list directories or files for the REST API
  /// @authors
  /// @mod{HE,20220216,created}
  /// @param url entry url
  /// @param result save-to vector
  /// @param directories list directories if `true` (default)
  void EntryLoader::listRestAPI(std::string url, std::vector<std::string> &result, bool directories) {
    result.clear();
    if (directories) url += m_restapi_listing_dirs;
    else url += m_restapi_listing_files;
    std::string output;
    if (200 == aurostd::httpGetStatus(url, output)) {
      if (output[output.size()-1]=='\n') output.erase(output.size()-1);
      aurostd::string2tokens(output, result, ",");
    } else {
      m_logger_message << "Could not list content for " << url;
      outInfo(__AFLOW_FUNC__);
    };
  }

  /// @brief generate a list of AUID for a given list of alloys by querying the public alloy SQLITE DB
  /// @param alloy_list list of cleaned and sorted alloys
  /// @param auid_list resulting AUIDs
  /// @authors
  /// @mod{HE,20220216,created}
  void EntryLoader::getAlloyAUIDList(const std::vector<std::string> & alloy_list, std::vector<std::string> &auid_list) {
    if (m_sqlite_alloy_db_ptr == nullptr) {
      m_logger_message << "Public alloy SQLITE DB is not ready";
      outHardError(__AFLOW_FUNC__, __LINE__, _RUNTIME_ERROR_);
      return;
    }
    auid_list.clear();
    std::string where = aurostd::joinWDelimiter(alloy_list, "','");
    where = "alloy IN ('" + where + "')";
    auid_list = m_sqlite_alloy_db_ptr->getValuesMultiTable(m_sqlite_alloy_db_ptr->idColumn(), where);  //CO20261008 - chaos_id, or auid before
  }

  /// @brief load AFLOW lib entries for a given alloy directly from the filesystem (source FILESYSTEM_RAW)
  /// @param alloy_list list of cleaned and sorted alloys
  /// @param lib_max number of elements in the largest alloy
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note if the public alloy SQLITE DB is available the results are expanded with missed entries
  void EntryLoader::loadAlloySearchFSR(const std::vector <std::string> &alloy_list, uint lib_max) {
    std::vector <std::string> found_entries;
    std::vector <std::string> search_path_list;
    search_path_list.emplace_back(m_filesystem_path + "ICSD/" + m_filesystem_collection + "/");
    if (alloy_list.size()>1) {
      for (uint lib_idx = 1; lib_idx <= lib_max; lib_idx++) {
        search_path_list.emplace_back(
            m_filesystem_path + "LIB" + std::to_string(lib_idx) + "/" + m_filesystem_collection + "/");
      }
    } else if (alloy_list.size()==1){
      search_path_list.emplace_back(
          m_filesystem_path + "LIB" + std::to_string(lib_max) + "/" + m_filesystem_collection + "/");
    } else {
      return;
    }

    char **paths = new char*[search_path_list.size()+1](); // initialize the array with nullptr, +1 ensures it is null terminated
    for (size_t list_idx = 0; list_idx < search_path_list.size(); list_idx++) {
      paths[list_idx] = (char *) search_path_list[list_idx].c_str();
    }

    size_t scanned = 0;
    size_t found = 0;
    size_t check_idx = m_filesystem_path.size();
    struct stat file_stats{};

    // initialize a file tree scan
    // https://man7.org/linux/man-pages/man3/fts.3.html
    // FTS_PHYSICAL - don't follow symlinks
    // FTS_NOCHDIR - don't change the workdir of the program
    // FTS_XDEV - don't descend into folders that are on a different device
    FTS *tree = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR | FTS_XDEV, nullptr);
    FTSENT *node;
    if (tree == nullptr) {
      m_logger_message << "Failed to initialize the file tree used to search for alloys!";
      outHardError(__AFLOW_FUNC__, __LINE__,_FILE_ERROR_);
      return;
    }

    // Iterate over all entries found in the folders listed in paths
    while ((node = fts_read(tree))) {
      scanned += 1;
      if ((scanned-1) % 10000 == 0) {
        m_logger_message << (scanned-1) << " objects scanned; " << found << " entries found; next scan: " << node->fts_path;
        outDebug(__AFLOW_FUNC__);
      }
      if ((node->fts_info & FTS_D)) {
        if (node->fts_path[check_idx] == 'I') { // ICSD
          if (node->fts_level == 2) {
            if (find(alloy_list.begin(), alloy_list.end(), extractAlloy(node->fts_name, 'I')) == alloy_list.end()) {
              fts_set(tree, node, FTS_SKIP);
              continue;
            } else {
              std::string base_path = node->fts_path;
              std::string full_path = base_path + "/" + m_filesystem_outfile;
              if (stat(full_path.c_str(), &file_stats) == 0) {
                found += 1;
                found_entries.emplace_back(full_path);
                if (base_path.find("POCC") == std::string::npos) fts_set(tree, node, FTS_SKIP);
              }
            }
          }
        } else if (node->fts_path[check_idx] == 'L') { // LIBX
          if (node->fts_level == 1) {
            if (find(alloy_list.begin(), alloy_list.end(), extractAlloy(node->fts_name, 'L')) == alloy_list.end()) {
              fts_set(tree, node, FTS_SKIP);
              continue;
            } else {
              std::string base_path = node->fts_path;
              std::string full_path = base_path + "/" + m_filesystem_outfile;
              if (stat(full_path.c_str(), &file_stats) == 0) {
                found += 1;
                found_entries.emplace_back(full_path);
                if (base_path.find("POCC") == std::string::npos) fts_set(tree, node, FTS_SKIP);
              }
            }
          } else if (node->fts_level > 1) {
            std::string base_path = node->fts_path;
            std::string full_path = base_path + "/" + m_filesystem_outfile;
            if (stat(full_path.c_str(), &file_stats) == 0) {
              found += 1;
              found_entries.emplace_back(full_path);
              if (base_path.find("POCC") == std::string::npos) fts_set(tree, node, FTS_SKIP);
            }
          }
        }
      }
    }
    fts_close(tree);
    delete[] paths;


    m_logger_message << "Finishing search in the filesystem after scanning " << scanned << " objects";
    outInfo(__AFLOW_FUNC__);
    loadFiles(found_entries);
    if (m_sqlite_alloy_db_ptr != nullptr) {
      std::vector <std::string> known_AUID_list;
      getAlloyAUIDList(alloy_list, known_AUID_list);
      std::vector <std::string> missing_AUID;
      for (std::vector<std::string>::const_iterator AUID = known_AUID_list.begin(); AUID != known_AUID_list.end(); AUID++) {
        if (std::find(m_auid_list.begin(), m_auid_list.end(), *AUID) == m_auid_list.end()) missing_AUID.emplace_back(*AUID);
      }
      m_logger_message << "Found " << missing_AUID.size() << " entries in the public alloy SQLITE DB that where not found in the filesystem search.";
      outDebug(__AFLOW_FUNC__);
      loadAUID(missing_AUID);
    }
  }

  /// @brief load AFLOW lib entries for a given alloy set directly from the AFLOW REST API (source RESTAPI_RAW)
  /// @param alloy_list list of cleaned and sorted alloys
  /// @param lib_max number of elements in the largest alloy
  /// @authors
  /// @mod{HE,20220216,created}
  void EntryLoader::loadAlloySearchRR(const std::vector <std::string> & alloy_list, uint lib_max) {
    std::vector<std::string> rest_api_queries;
    std::vector<std::string> icsd_search_path_list;
    std::vector<std::string> libx_search_path_list;
    std::vector<std::string> libx_crawl_list;
    for (std::vector<std::string>::const_iterator sym = BRAVAIS_LATTICES.begin(); sym != BRAVAIS_LATTICES.end(); sym++) {
      icsd_search_path_list.emplace_back(m_restapi_server + m_restapi_path + "ICSD" + "_" + m_restapi_collection + "/" + *sym + "/");
    }
    if (alloy_list.size()>1) {
      for (uint lib_idx = 1; lib_idx <= lib_max; lib_idx++) {
        libx_search_path_list.emplace_back(m_restapi_server + m_restapi_path + "LIB" + std::to_string(lib_idx) + "_" + m_restapi_collection + "/" );
      }
    } else if (alloy_list.size()==1){
      libx_search_path_list.emplace_back(m_restapi_server + m_restapi_path + "LIB" + std::to_string(lib_max) + "_" + m_restapi_collection + "/" );
    } else {
      return;
    }
    std::vector<std::string> listing;
    for (std::vector<std::string>::const_iterator url = icsd_search_path_list.begin(); url != icsd_search_path_list.end(); url++) {
      listRestAPI(*url, listing);
      for (std::vector<std::string>::const_iterator name = listing.begin(); name != listing.end(); name++) {
        if (find(alloy_list.begin(), alloy_list.end(), extractAlloy(*name, 'I')) != alloy_list.end()){
          rest_api_queries.emplace_back(*url + *name + "/" + m_restapi_directives);
        }
      }
    }

    m_logger_message << "Found " << rest_api_queries.size() << " ICSD entries in REST API search";
    outDebug(__AFLOW_FUNC__);
    for (std::vector<std::string>::const_iterator url = libx_search_path_list.begin(); url != libx_search_path_list.end(); url++) {
      listRestAPI(*url, listing);
      for (std::vector<std::string>::const_iterator name = listing.begin(); name != listing.end(); name++) {
        if (find(alloy_list.begin(), alloy_list.end(), extractAlloy(*name, 'L')) != alloy_list.end()){
          libx_crawl_list.emplace_back(*url + *name + "/" );
        }
      }
    }

    m_logger_message << "Found " << libx_crawl_list.size() << " LIBX base folders in the REST API search";
    outDebug(__AFLOW_FUNC__);

    uint scanned=0;
    uint found=0;
    while(!libx_crawl_list.empty()){
      scanned += 1;
      std::string url = libx_crawl_list.back();
      libx_crawl_list.pop_back();
      if ((scanned-1)%100 == 0) {
        m_logger_message << (scanned-1) << " folders scanned; " << found << " entries found; next scan: " << url;
        outDebug(__AFLOW_FUNC__);
      }
      listRestAPI(url, listing);
      if (listing.empty()) {
        rest_api_queries.emplace_back(url + m_restapi_directives);
        found += 1;
      } else {
        for (std::vector<std::string>::const_iterator name = listing.begin(); name != listing.end(); name++) {
          libx_crawl_list.emplace_back(url + *name + "/");
        }
      }
    }


    loadRestAPIQueries(rest_api_queries, true);
    if (m_sqlite_alloy_db_ptr != nullptr) {
      std::vector <std::string> known_AUID_list;
      getAlloyAUIDList(alloy_list, known_AUID_list);
      std::vector <std::string> missing_AUID;
      for (std::vector<std::string>::const_iterator AUID = known_AUID_list.begin(); AUID != known_AUID_list.end(); AUID++) {
        if (std::find(m_auid_list.begin(), m_auid_list.end(), *AUID)== m_auid_list.end()) missing_AUID.emplace_back(*AUID);
      }
      m_logger_message << "Found " << missing_AUID.size() << " entries in the public alloy SQLITE DB that where not found in the REST API search.";
      outDebug(__AFLOW_FUNC__);
      loadAUID(missing_AUID);
    }

  }

  /// @brief map different AUID styles to a default
  /// @param AUID AUID to clean
  /// @return `false` if AUID is not valid
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note the AUID is cleaned in place
  bool EntryLoader::cleanAUID(std::string & AUID) {
    aurostd::RemoveCharacterInPlace(AUID, '"'); //CO20260930 - strip quotes //CO20261004

    std::string prefix = m_auid_prefix;
    std::string id = AUID;
    if (id.substr(0, 5) == "auid:") {
      id = id.substr(5);
    } else {
      size_t prefix_pos = id.find(':');
      if (prefix_pos != std::string::npos) {
        prefix = id.substr(0, prefix_pos);
        id = id.substr(prefix_pos + 1);
      }
    }

    if (!aurostd::_ishex(id) || id.size() != 16) return false;

    AUID = prefix + ":" + id;
    return true;
  }

  /// @brief map different AURL styles to a default
  /// @param AUID AURL to clean
  /// @return `false` if AURL is not valid
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note AURL is not validated
  /// @note the AURL is cleaned in place
  bool EntryLoader::cleanAURL(std::string & AURL) {
    string path = m_aurl_path + ":";
    if (AURL.substr(0, 5) == "aurl:")  AURL = AURL.substr(5);
    // Normalize explicit host paths (e.g., s4e.ai:AFLOWDATA/...)
    //CO20261004 - AURLStripHost removes any host, was find(":AFLOWDATA/") by hand; the AFLOWDATA branch below
    //adds #m_aurl_path back (the separate check for path+"AFLOWDATA/" could not be reached: the find caught it first)
    AURL = aflowlib::AURLStripHost(AURL);
    if (AURL.substr(0, 9) == "AFLOWDATA") {
      AURL = path + AURL;
      return true;
    }
    if ((AURL.substr(0,3) == "LIB" || AURL.substr(0,4) == "ICSD")) {
      AURL = path + "AFLOWDATA/" + AURL;
      return true;
    }
    cout << "FAILED " << AURL << endl;
    return false;
  }

  /// @brief create a AFLUX query string from a matchbook
  /// @param matchbook map with keyword and modifier
  /// @returns AFLUX query
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note does not include #m_aflux_server or #m_aflux_path
  /// @note the string is percent encoded
  std::string EntryLoader::buildAFLUXQuery(const std::map<std::string, std::string> & matchbook) const {
    std::string query = "";
    std::vector<std::map<std::string, std::string>> joined_directives = {matchbook, m_aflux_directives};
    for (std::vector<std::map<std::string, std::string>>::const_iterator part = joined_directives.begin(); part != joined_directives.end(); part++) {
      for (std::map<std::string, std::string>::const_iterator it = part->begin(); it != part->end(); it++) {
        query += "," + it->first;
        if (!it->second.empty()) query += "(" + it->second + ")";
      }
    }
    query.erase(0,1); 
    return "?" + aurostd::httpPercentEncodingFull(query);
  }

  /// @brief extract alloy string from AFLOW run name
  /// @param name AFLOW run name (from path structure)
  /// @param lib_type indicate the LIB type | `I` for `ICSD` or `L` for 'LIBx'
  /// @return alloy string
  /// @authors
  /// @mod{HE,20220216,created}
  /// @note the alloy sting is sorted
  std::string EntryLoader::extractAlloy(std::string name, char lib_type) const {
    if (lib_type == 'I') { //ICSD
      name.erase(std::min(name.find("_ICSD_"), name.size()));
    } else if (lib_type == 'L') { //LIBX
      name.erase(std::min(name.find(':'), name.size()));
      name = std::regex_replace(name, aurostd::regex_ppclean,"");
    } else {
      return "";
    }
    std::vector <std::string> element_match(std::sregex_token_iterator(name.begin(), name.end(), aurostd::regex_elements),
                                            std::sregex_token_iterator());
    std::sort(element_match.begin(), element_match.end());
    return aurostd::joinWDelimiter(element_match, "");
  }

  void EntryLoader::outInfo(const std::string & function_name) {
    if ((!m_out_silent || m_out_debug) && !m_out_super_silent){
      pflow::logger(__FILE__, function_name, m_logger_message, *p_FileMESSAGE, *p_oss, _LOGGER_NOTICE_);
    }
    aurostd::StringstreamClean(m_logger_message);
  }

  void EntryLoader::outDebug(const std::string & function_name) {
    if (m_out_debug && !m_out_super_silent){
      pflow::logger(__FILE__, function_name, m_logger_message, *p_FileMESSAGE, *p_oss, _LOGGER_MESSAGE_);
    }
    aurostd::StringstreamClean(m_logger_message);
  }

  void EntryLoader::outError(const std::string &function_name, int line_number) {
    if (!m_out_super_silent) {
      pflow::logger((std::string) __FILE__ + ":" + std::to_string(line_number),
                    function_name, m_logger_message, *p_FileMESSAGE, *p_oss, _LOGGER_ERROR_);
    }
    aurostd::StringstreamClean(m_logger_message);
  }

  void EntryLoader::outHardError(const std::string &function_name, int line_number, int error_type) {
    throw aurostd::xerror((std::string) __FILE__ + ":" + std::to_string(line_number), function_name, m_logger_message, error_type);
    aurostd::StringstreamClean(m_logger_message);
  }


}

#endif  // _AFLOWLIB_ENTRY_LOADER_CPP_

// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2023           *
// *                                                                         *
// ***************************************************************************
