// ***************************************************************************
// *                                                                         *
// *          Aflow COREY OSES - Johns Hopkins University 2022-2026          *
// *         Aflow WILL SHIBER - Johns Hopkins University 2023-2024          *
// *      Aflow MATTHEW BROWNRIGG - Johns Hopkins University 2024-2025       *
// *         Aflow KEITH CLARK - Johns Hopkins University 2023-2024          *
// *                                                                         *
// ***************************************************************************

#include "aflow.h"
#include "aflow_aflux.h"
#include "aflowlib.h"

namespace aflux{

  // ************************* HELPER FUNCTIONS *****************************

  /// @brief Find the next occurrence of a substring from a list
  /// @param str string to be searched for substring occurences
  /// @param stringList list of substrings that will be checked in str
  /// @param startPos starting index to begin checking in str
  /// @return the index of earliest occurence (from startPos) of any substring from stringList in str
  ///
  /// @authors
  /// @mod{WS,20231006, Created}
  /// @mod{MB,20240611, Add documentation}
  /// @mod{MB,20240611, Replace range-based for loop with indexed for-loop}
  size_t findNextOccurrence(const string& str, const vector<string>& stringList, size_t startPos) {
    size_t nextOccurrence = std::string::npos; // Initialize with a value indicating not found
    for (size_t i = 0; i < stringList.size(); ++i) { // Cycle through substrings 
      size_t foundPos = str.find(stringList.at(i), startPos); // check if substring in str
      if (foundPos != string::npos) { // Substring exists
        if (nextOccurrence == string::npos || foundPos < nextOccurrence) { 
          // get occurence with min distance from startPos.
          nextOccurrence = foundPos;
        }
      }
    }
    return nextOccurrence;
  }

  /// @brief Find the next occurrence of a char from a list
  /// @param str string to be searched for substring occurences
  /// @param charList list of chars that will be checked in str
  /// @param startPos starting index to begin checking in str
  /// @return the index of earliest occurence (from startPos) of any char from charList in str
  ///
  /// @authors
  /// @mod{WS,20231006, Created}
  /// @mod{MB,20240611, Add documentation}
  size_t findNextOccurrence(const string& str, const vector<char>& charList, size_t startPos){
    size_t strLength = str.length();
    size_t listSize = charList.size();
    for (size_t i = startPos; i < strLength; ++i){ // Iterate through str
      for (size_t j = 0; j < listSize; ++j){ // Cycle through substrings 
        if (str[i] == charList[j]){
          return i;
        }
      }
    }
    return std::string::npos; 
  }

  /// @brief Remove excess outer parentheses (container) from string
  /// @param str string with default container
  /// @return string without parentheses. Example: "(foo)" -> "foo"
  /// 
  /// @authors
  /// @mod{WS,20231006, Created}
  /// @mod{MB,20240611, Add documentation}
  /// 
  /// @note Default container is ()
  string removeOuterParentheses(const string& str){
    const vector<char>& container = {'(',')'};
    return removeOuterParentheses(str, container);
  }
  
  /// @brief Remove container from string
  /// @param str string with container
  /// @param container vector<char> with container symbols. Example: { "[" , "]" } or { "(" , ")" }
  /// @return string without parentheses. Example: "(foo)" -> "foo"
  /// 
  /// @authors
  /// @mod{WS,20231006, Created}
  /// @mod{MB,20240611, Add documentation}
  ///
  /// @note Default container is ()
  string removeOuterParentheses(const string& str, const vector<char>& container) {
    //CO20261003 - strip only a pair that encloses the whole string; "(a),(b)" starts and ends with
    //CO20261003 - container symbols, but they belong to two pairs, and stripping them gave "a),(b"
    if(str.size()<2 || str.front()!=container[0] || str.back()!=container[1]){return str;}
    int depth=0;
    for(size_t i=0;i<str.size()-1;i++){
      if(str[i]==container[0]){depth++;}
      else if(str[i]==container[1]){depth--; if(depth==0){return str;}}  // the first symbol closes before the end
    }
    return str.substr(1,str.size()-2);
  }

  /// @brief Split string by delimeters outside of given container 
  /// @param input string with multiple containers seperated by a delimeter
  /// @return vector<string> - list of only string content within containers.
  /// Example: "(foo),(fighters)" -> {"foo", "fighters"}
  ///
  /// @authors
  /// @mod{WS,20231006, Created}
  /// @mod{MB,20240611, Add documentation}
  ///
  /// @note default container is "()", default delimeter ","
  vector<string> splitStringByOuterDelimeter(const string& input){
  char delimeter = ',';
  vector<char> container = {'(',')'};
  return splitStringByOuterDelimeter(input, delimeter, container);
  }

  /// @brief Split string by delimeters outside of given container 
  /// @param input string with multiple containers seperated by a delimeter
  /// @param container vector<char> with container symbols. Example: { "[" , "]" } or { "(" , ")" }
  /// @return vector<string> - list of only string content within containers.
  /// Example: "(foo),(fighters)" -> {"foo", "fighters"}
  ///
  /// @authors
  /// @mod{WS,20231006, Created}
  /// @mod{MB,20240611, Add documentation}
  ///
  /// @note default container is "()", default delimeter ","
  vector<string> splitStringByOuterDelimeter(const string& input, const vector<char>& container){
    char delimeter = ',';
    return splitStringByOuterDelimeter(input, delimeter, container);
  }

  /// @brief Split string by delimeters outside of given container 
  /// @param input string with multiple containers seperated by a delimeter
  /// @param delimeter char that spilts the containers. Example: "," or ";"
  /// @return vector<string> - list of only string content within containers.
  /// Example: "(foo),(fighters)" -> {"foo", "fighters"}
  ///
  /// @authors
  /// @mod{WS,20231006, Created}
  /// @mod{MB,20240611, Add documentation}
  ///
  /// @note default container is "()", default delimeter ","
  vector<string> splitStringByOuterDelimeter(const string& input, const char& delimeter){
    vector<char> container = {'(',')'};
    return splitStringByOuterDelimeter(input, delimeter, container);
  }

  /// @brief Split string by delimeters outside of given container 
  /// @param input string with multiple containers seperated by a delimeter
  /// @param delimeter char that spilts the containers. Example: "," or ";"
  /// @param container vector<char> with container symbols. Example: { "[" , "]" } or { "(" , ")" }
  /// @return vector<string> - list of only string content within containers.
  /// Example: "(foo),(fighters)" -> {"foo", "fighters"}
  ///
  /// @authors
  /// @mod{WS,20231006, Created}
  /// @mod{MB,20240611, Add documentation}
  ///
  /// @note default container is "()", default delimeter ","
  vector<string> splitStringByOuterDelimeter(const string& input, const char& delimeter, const vector<char>& container) {
    vector<char> delimeterList = {delimeter};
    return splitStringByOuterDelimeter(input, delimeterList, container);
  }

  /// @brief Split string by delimeters outside of given container 
  /// @param input string with multiple containers seperated by a delimeter
  /// @param delimeterList list of char that spilts the containers. Example: { "," , ";" }
  /// @param container vector<char> with container symbols. Example: { "[" , "]" } or { "(" , ")" }
  /// @return vector<string> - list of only string content within containers.
  /// Example: "(foo),(fighters)" -> {"foo", "fighters"}
  ///
  /// @authors
  /// @mod{WS,20231006, Created}
  /// @mod{MB,20240611, Add documentation}
  /// @mod{MB,20240611, Replace range-based loop with indexed loop}
  /// @mod{MB,20240612, Replace stack with already imported deque}
  vector<string> splitStringByOuterDelimeter(const string& input, const vector<char>& delimeterList, const vector<char>& container){
    vector<string> result; 
    std::deque<char> parenthesesStack;
    string current = "";
    for (long unsigned int i = 0; i < input.size(); ++i) { // Index through every char in input 
        if (input.at(i) == container[0]) { 
            parenthesesStack.push_back(input.at(i)); 
        } else if (input.at(i) == container[1]) { 
            if (!parenthesesStack.empty()) {parenthesesStack.pop_back();}
        }
        if ((find(delimeterList.begin(), delimeterList.end(), input.at(i)) != delimeterList.end()) && parenthesesStack.empty()) {
            result.push_back(current);
            current.clear();
        } else {current += input.at(i);}
    }
    if (!current.empty()) {
        result.push_back(current);
    }
    return result;
  }

/// @brief Convert encoded URL to decoded URL string
/// @param encoded URL with HTTP encoding Ex."%42"
/// @return Decoded URL Ex."%42" -> "*"
/// @authors
/// @mod{MB,20240704,Created}
/// @note C++11 adaptation of pocode.tk/decode-url
string decodeURL(const std::string& encoded) {
  string edit_string = encoded; // Follow AFLUX const convention...
	string decoded;
	for (uint i = 0; i < edit_string.length(); i++) {
		if (edit_string.at(i) == '%') { // found econded char operator
      uint j = stoi(edit_string.substr(i+1,2), 0, 16); // Convert hex encoding to decimal
			char chr = static_cast<char>(j); // Convert decimal encoding to char
			decoded += chr; // Add decoded char to new string
			i += 2; // Jump to new char
		} else { 
			decoded += edit_string.at(i); 
		}
	}
	return decoded;
}

  /// @brief Checks if string has balanced parentheses
  /// @param input string with parentheses 
  /// @return True parentheses are balanced. False otherwise
  /// Example: "()" -> True, "())" -> False
  ///
  /// @authors
  /// @mod{MB,20240624, Created}
  bool hasBalancedParentheses(const string& query) {
    std::deque<char> parenthesesStack;

    for (uint i = 0; i < query.size(); i++) {
      if (query.at(i) == '(') {
        parenthesesStack.push_back(query.at(i)); // add "(" to stack
      } else if (query.at(i) == ')') { 
        if (parenthesesStack.empty()) { // Missing opening parenthesis
          return false;
        }
        parenthesesStack.pop_back(); // "(" matched with ")"
      } else { // This is a non-parenthesis character, skip
        continue;
      }
    }
    return parenthesesStack.empty(); // If not empty, then unbalanced.
  }
  
/// @brief Parses input and returns a list of functions and their params
/// @param input string containing commas at multiple depths (Ex. "species(Fe),paging(1,2)")
/// @return list of functions in input Ex. {"species(Fe)", "paging(1,2)"}
///
/// @authors
/// @mod{MB,06252024,Created}
vector<string> AFLUX::splitAtTopDepth(const string& input, const char& delim) const {
  string to_parse = input; 
  uint depth = 0; // Checks if comma is whithin parentheses
  vector<string> input_funcs = {}; // List of all funcs
  uint prev_pos = 0; // index

  for (uint pos = 0; pos < to_parse.size(); pos++) {
    if (to_parse.at(pos) == delim && depth == 0) {
      string func = to_parse.substr(prev_pos, pos - prev_pos); // Found query function
      prev_pos = pos + 1; // Skip over comma
      input_funcs.push_back(func); // Add to list of commands
    } else if (to_parse.at(pos) == '(') {
      depth++; // Went inside a layer of func
    } else if (to_parse.at(pos) == ')') {
      depth--; // Exited a layer of func
    }
  }
  input_funcs.push_back(to_parse.substr(prev_pos)); // Add whatever is left (no comma in the end)
  return input_funcs;
}

/// @brief Creates a list of function and parameter pairs
/// @param funcs list of function with parameters Ex. {"species(Fe,Mn)", "density(0.5)"}
/// @return list of function and parameter pairs Ex. { {"species", "(Fe,Mn)"}, {"density", "(0.5)"} }
///
/// @authors
/// @mod{MB,20240628,Creatd}
vector<std::pair<string, string>> pairParamsFromFuncs(const vector<string>& funcs) {
  vector<std::pair<string, string>> funcPairs;
  
  for (uint i = 0; i < funcs.size(); i++) {
    size_t delim_pos = funcs.at(i).find("("); // Split by starting parentheses
    string func;
    string param;
    if (delim_pos == string::npos) { // Function has no parameters
      func = funcs.at(i);
      param = "";
    } else { // function has parameters
      func = funcs.at(i).substr(0, delim_pos); // extract func name
      param = funcs.at(i).substr(delim_pos); // extract paramerters
    }
    funcPairs.push_back(std::make_pair(func, param)); // combine into pair 
  }
  return funcPairs;
}

  // ************************* AFLUX CLASS FUNCTIONS ************************

  AFLUX::AFLUX(){init();}

  AFLUX::~AFLUX() { xStream::free(); }

  const AFLUX &AFLUX::operator=(const AFLUX &other) {
    if (this != &other) { copy(other); }
    return *this;
  }

  ///@mod{20240611, 20240611, Rename m_keywordDict to m_keywordMap to match C++ style guide}
  void AFLUX::init() {
    m_keywordMap = createKeywordMap();
		m_keywordList = getKeysFromMap(m_keywordMap); m_keywordList.push_back("paging"); m_keywordList.push_back("format");
		m_paging = 0;
		m_SQL_query = "";
		m_AFLUX_query = "";
		m_defaultKeywords = {"composition", "compound", entryIdKeyword(), entryPathKeyword(), "spacegroup_relax", "Pearson_symbol_relax"}; //CO20261007 - the identifier by its name in the database
		m_queryLogicChars = {'(', ')', ',', ':', ' '};
		m_searchKeywords = m_defaultKeywords; 
    initSearchMap();
	  m_order_by = "";
    m_pagingParam_k = 64;
    m_pagingParam_n = 0;
		m_format = "json";
		m_directiveList = {"catalog", "format", "help", "paging", "schema", "global", "local", "version"};
    m_queryForHelp = false;
    m_queryForSchema = false;
    m_numEntries = 0;
    m_printNull = false;
    m_useLoadAlloy = false;
    m_alloyParams = "";
    m_default_paging = true;
    m_isLocalQuery = true;
    m_version = v200;
    m_queryForVersion = false;
  }

  /// @brief creates a search map, index of all keywords and a bool to confirm if they will appear in query.
  void AFLUX::initSearchMap() {
    vector<string> keys = getKeysFromMap(m_keywordMap);
    for (uint i = 0; i < keys.size(); i++) {
      m_searchMap[keys[i]] = false;
    }
    for (uint i = 0; i< m_defaultKeywords.size(); i++) {
      m_searchMap[m_defaultKeywords[i]] = true;
    }
  }

  void AFLUX::clear() { free(); }
  void AFLUX::free() { *this = {}; } // calling the default constructor to ensure a clean instance of AFLUX
  void AFLUX::copy(const AFLUX &b) {  //copy PRIVATE
    m_keywordMap = b.m_keywordMap;
		m_keywordList = b.m_keywordList;
    m_paging = b.m_paging;
		m_SQL_query = b.m_SQL_query;
		m_AFLUX_query = b.m_AFLUX_query;
    m_schemaParams = b.m_schemaParams;
    m_helpParams = b.m_helpParams;
    m_AFLUXHelpText = b.m_AFLUXHelpText;
    m_AFLUXSchemaText = b.m_AFLUXSchemaText;
		m_atomicFilterParams = b.m_atomicFilterParams; 
		m_atomicFilterKeywords = b.m_atomicFilterKeywords; 
    m_tensorFilterKeywords = b.m_tensorFilterKeywords;
    m_tensorFilterParams = b.m_tensorFilterParams;
		m_defaultKeywords = b.m_defaultKeywords; 
		m_queryLogicChars = b.m_queryLogicChars;
		m_searchKeywords = b.m_searchKeywords; 
    m_searchMap = b.m_searchMap;
		m_pagingParam_k= b.m_pagingParam_k; 
		m_pagingParam_n= b.m_pagingParam_n; 
		m_format = b.m_format;
		m_order_by = b.m_order_by;
		m_directiveList = b.m_directiveList;
    m_queryForHelp = b.m_queryForHelp;
    m_queryForSchema = b.m_queryForSchema;
    m_numEntries = b.m_numEntries;
    m_printNull = b.m_printNull;
    m_default_paging = b.m_default_paging;
    m_isLocalQuery = b.m_isLocalQuery;
    m_version = b.m_version;
    m_queryForVersion = b.m_queryForVersion;
  }


/// @brief Creates a map of all keywords and their corresponding response type
/// @param void
/// @return keywordMap - map containing all map<keyword, response> pairs
///
/// @authors
/// @mod{WS, 20231006, Created}
/// @mod{MB, 20240611, Rename keywordDict to keywordMap to match C++ style guide}
std::map<string, string> AFLUX::createKeywordMap(){
  std::map<string, string> keywordMap;
  for (uint i = 0, n = XHOST.vschema.vxsghost.size(); i < n; i += 2) { 
    if(XHOST.vschema.vxsghost[i].find("SCHEMA::NAME:")!=string::npos) {
      const string& key=XHOST.vschema.vxsghost[i+1];
      const string& rUnit = XHOST.vschema.vxsghost[i+5];
      keywordMap[key] = rUnit;
    }    
  }
  keywordMap["alloy"] = "string";
  return keywordMap;
}

/// @brief Take everything after the question mark and replace aliases
/// @param input AFLUX URL containing a query
/// @return string with only query text (URL is removed)
///
/// @authors
/// @mod{WS, 20231006, Created}
string AFLUX::getQueryFromLink(const string& input) {
  size_t pos = input.find('?');
  string result = "";
  if (pos != string::npos && pos + 1 < input.length()) {
    result = input.substr(pos + 1);
  }else{
    result = input;
  }
  if(result[0] == '*'){ // Display all keywords
      std::map<string, bool>::iterator it;
      for (it = m_searchMap.begin(); it != m_searchMap.end(); it++) {
        it->second = true; // Set all searches to true;
      }
      result = result.substr(2);
    }
  return replaceAliases(result);
}

/// @brief Find and replace aliases
/// @param input SQL query text
/// @return string copy of SQL query text with replaced alias
///
/// @authors
/// @mod{WS, 20231006, Created}
/// @mod{MB, 20240611, Add documentation}
/// @mod{MB, 20240611, Rename m_aliasesDict to m_aliasesMap}
string AFLUX::replaceAliases(const string& input){
  loadAliasesMap();
  vector<string> aliases = getKeysFromMap(m_aliasesMap);
  string str = input;
  while(findNextOccurrence(str,aliases) != string::npos){
    for(uint i = 0; i < aliases.size(); i++){
      if(str.find(aliases[i]) != string::npos){
        if(str[str.find(aliases[i]) - 1] == '('){ // Don't want to replace if it is a subword of another alias (i.e. Metals in contained in NonMetals)
          str.replace(str.find(aliases[i]),aliases[i].size(), m_aliasesMap[aliases[i]]);
        }
      }
    }
  }
  return str;
}

/// @brief replaces all instances of SQL "COMMA" with "," within input string
/// @param input SQL query text
/// @return Edits the input string.
///
/// @authors
/// @mod{WS, 20231006, Created}
/// @mod{MB, 20240611, Add documentation}
///
/// @note Silly. This function has no unit tests.
void AFLUX::replaceCOMMAWithAnActualComma(string& input){
  while(input.find("COMMA") != string::npos) { 
    input.replace(input.find("COMMA"), 5, ",");
  }
}

/// @brief Returns a vector of the keys of a map
/// @param input any map<string, string> with at least one entry
/// @return vector<string> containing all keys from map
///
/// @authors
/// @mod{WS, 20231006, Created}
/// @mod{MB, 20240611, Add documentation}
/// @mod{MB, 20240611, Replace range-based for loop to iterated for loop}
vector<string> AFLUX::getKeysFromMap(const std::map<string, string>& input){
  vector<string> vkeys; //CO20260930
  vkeys.reserve(input.size());
  for (std::map<string, string>::const_iterator it = input.cbegin(); it != input.cend(); ++it) {vkeys.push_back(it->first);}
  return vkeys;
}

/// @brief Checks if given character is not a query keyword or query parameter character.
/// @brief Current query chars: [ '(' , ')' , ',' , ':' , ' ' ]
/// @param chr char that will be compared to query operators
/// @return True: input is a query keyword, False: input is not a query keyword
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB, 20240611, Add documentation}
bool AFLUX::isQueryLogicChar(const char& chr){
  if(find(m_queryLogicChars.begin(), m_queryLogicChars.end(), chr) != m_queryLogicChars.end()){
    return true;
  } else { 
    return false;
  }
}

/// @brief Check if a word is a directive
/// @param word string that will be compared to a directive list.
/// @return True: input is directive, False: input is not directive
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB, 20240611, Add documentation}
bool AFLUX::isDirective(const string& word){
  string temp = aurostd::RemoveCharacterFromTheFront(word, '$');
  if(find(m_directiveList.begin(), m_directiveList.end(), temp) != m_directiveList.end()){
    return true;
  } else {
    return false;
  }
}

/// @brief Check if a word is a keyword
/// @param word string that will be compared to a keyword map.
/// @return True: input is Keyword, False: otherwise
/// @authors
/// @mod{MB,20240625,Created}
bool AFLUX::isKeyword(const string& word){
  string temp = aurostd::RemoveCharacterFromTheFront(word, '$');
  if (m_keywordMap.count(temp)) {
    return true;
  } else {
    return false;
  }
}

/// @brief generates NumEntries from the EntryLoader 
/// @param el EntryLoader object
/// @return number of entries after the query
/// @authors
/// @mod{MB,20240805,Created}
/// @note function must be called after a query has been made
uint AFLUX::getNumEntries(const aflowlib::EntryLoader& el) {
  vector<aflowlib::_aflowlib_entry> entries;
  el.getEntriesFlat(entries);
  m_numEntries = entries.size();
  return m_numEntries;
}

bool AFLUX::isNonQuery() {
  if (m_queryForSchema || m_queryForHelp || m_queryForVersion) {
    return true;
  } else {
    return false;
  }
}

/// @brief Prints help text for a given keyword
/// @param keyword string that will be compared to a keyword map.
/// @return void
/// @authors
/// @mod{MB,20240625,Created}
void AFLUX::printNonQuery() {
  // Conditions that will end query early
  if (m_queryForSchema) { // Queried for schema
    if ( !m_schemaParams.empty() ) {
      printPropertyHelp(m_schemaParams); 
    } else {
      printPropertyHelp("*");
    }
    cout << 0 << endl;
    return;
  } else if (m_queryForHelp) { // Queried for help
    if ( !m_helpParams.empty() ) {
      loadHelpText(m_helpParams);
    } else {
      loadHelpText("general");
    }
    cout << m_AFLUXHelpText << endl;
    cout << 2 << endl;
    return;
  } else if (m_queryForVersion) {
    cout << "-" << "v" << m_version << "-" << endl;
    cout << 2 << endl;
    return;
  }
}

/// @brief Replaces commas (",") with "AND" and colons (":") with "OR"
/// @param str SQL query string
/// @return Edits the str string
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB, 20240611,Add documentation}
void AFLUX::replaceCommaAndColonWithAndOr(string& str){
  while(str.find(',') != string::npos){str.replace(str.find(','), 1, " AND ");}
  while(str.find(':') != string::npos){str.replace(str.find(':'), 1, " OR ");}
}

/// @brief the name the CHAOS API gives an entry's identifier
/// @authors
/// @mod{CO,20261007,Created}
string chaosIdName() {return "chaos_id";}

/// @brief the identifier's name in AFLOW's schema and in the database: chaos_id once the schema has it
///        (lib2raw writing chaos_id), auid until then
/// @note every place the engine names the identifier (a query term, the order of alloy(...), the
///       default fields, an answer's keys) goes through this, so the lib2raw change needs nothing else
///       here. The CHAOS gate's documentation check and system pages read the column by the same rule.
/// @authors
/// @mod{CO,20261007,Created}
string entryIdKeyword() {
  const vector<string>& vschema = XHOST.vschema.vxsghost;
  for (size_t i = 0; i + 1 < vschema.size(); i += 2) {
    if (vschema[i].find("SCHEMA::NAME:") != string::npos && vschema[i + 1] == chaosIdName()) {return chaosIdName();}
  }
  return "auid";
}

/// @brief the run's path in AFLOW's schema and in the database: chaos_path where the identifier is chaos_id
///        (the two came in together, CO20261008), aurl before
/// @param id_keyword the identifier's name in the database (entryIdKeyword())
/// @authors
/// @mod{CO,20261008,Created}
string entryPathKeyword(const string& id_keyword) {return id_keyword == chaosIdName() ? string("chaos_path") : string("aurl");}

/// @brief how the API's JSON and HTML answers name an entry's keys: the identifier is printed as chaos_id;
///        aurl (where the entry sits in the library), the library and AFLOW version stamps, and how the entry
///        ran are left out (an empty name)
/// @param id_keyword the identifier's name in the database (entryIdKeyword())
/// @return key -> printed name, "" for a key that is not printed
/// @note format(aflow) is written whole, as chaos_path= and chaos_id= (aflowlib2string("chaos"), CO20261008): AFLOW's
///       own clients (EntryLoader, ChaosRuns) read the identifier and the path from it. aurl(...) and auid(...)
///       still work as filters; only the printed key goes. An entry keeps its identifier in auid whatever the
///       database calls it, and the JSON writer names it auid (as lib2raw's files do), so auid is always printed
///       as chaos_id; the writer leaves out a renamed key that the entry prints already, so an answer never has
///       two identifiers.
/// @authors
/// @mod{CO,20261007,Created}
/// @mod{CO,20261008,auid is always printed as chaos_id; chaos_path is left out like aurl}
std::map<string,string> responseKeyMap(const string& id_keyword) {
  std::map<string,string> keys = {
    {"aurl", ""}, {"chaos_path", ""}, {"data_api", ""}, {"data_source", ""}, {"loop", ""}, {"calculation_cores", ""},
    {"aflow_version", ""}, {"aflowlib_version", ""}, {"aflowlib_date", ""}};
  keys["auid"] = chaosIdName();  //CO20261008 - the entry's identifier, whatever the database calls it
  if (id_keyword != chaosIdName()) {keys[id_keyword] = chaosIdName();}
  return keys;
}

/// @brief the keys responseKeyMap() leaves out
/// @authors
/// @mod{CO,20261007,Created}
vector<string> responseHiddenKeywords(const string& id_keyword) {
  vector<string> keys;
  for (const std::pair<const string, string>& k : responseKeyMap(id_keyword)) {
    if (k.second.empty()) {keys.push_back(k.first);}
  }
  return keys;
}

/// @brief the field a query term names: chaos_id, and auid as written before, are the identifier's name in
///        the database; a leading $ is kept
/// @param name a term's name as written in the query, e.g. "chaos_id" or "$chaos_id"
/// @param id_keyword the identifier's name in the database (entryIdKeyword())
/// @return e.g. "auid" or "$auid" while the database names it auid; any other name unchanged
/// @authors
/// @mod{CO,20261007,Created}
string keywordFromResponseName(const string& name, const string& id_keyword) {
  const bool suppress = !name.empty() && name[0] == '$';
  const string bare = suppress ? name.substr(1) : name;
  if (bare == chaosIdName() || bare == "auid") {return (suppress ? "$" : "") + id_keyword;}
  if (bare == "chaos_path" || bare == "aurl") {return (suppress ? "$" : "") + entryPathKeyword(id_keyword);}  //CO20261008
  return name;
}

/// @brief the values of a chaos_path(...) term as the database keeps the paths: an AURL (as AFLOW's clients built
///        before chaos_path send it, aurl(...)) becomes its chaos_path, "s4e.ai:AFLOWDATA/LIB5_WEB/X" -> "LIB5/X";
///        a colon inside a path (LIB1/As:PAW_PBE/..., .../A.A:POCC_P0-...) is written as LIKE's one-character
///        wildcard, so that only a colon before the next path (LIBn/, ICSD/) or an operator (*, !) separates
///        values, as ':' does for every other field
/// @param param the term's parameter as written in the query, e.g. "LIB1/As:PAW_PBE/A_hP2_194_c-001.A:LIB2/FMg_pv/A"
/// @return e.g. "LIB1/As_PAW_PBE/A_hP2_194_c-001.A:LIB2/FMg_pv/A"
/// @note '_' already stands for itself in a path and matches it as a wildcard, so the match is as exact as before
/// @authors
/// @mod{CO,20261008,Created}
string chaosPathParam(const string& param) {
  static const std::regex aurl("[A-Za-z0-9.-]+:AFLOWDATA/(LIB[0-9]+|ICSD)_(?:WEB|RAW|LIB)/");
  static const std::regex next_value("^((LIB[0-9]+|ICSD)/|[*!])");
  const string p = std::regex_replace(param, aurl, "$1/");
  string out = "";
  for (size_t i = 0; i < p.size(); i++) {
    if (p[i] == ':' && !std::regex_search(p.substr(i + 1), next_value)) {out += '_'; continue;}
    out += p[i];
  }
  return out;
}

/// @brief corrects the splitting of s4e prefix and auid.
/// Example: auid LIKE '"s4e"' OR auid LIKE '"0088e48b0ea8b960"'  --> auid LIKE '"s4e:0088e48b0ea8b960"'
/// @param str SQL query string
/// @return Edits str string.
/// 
/// @authors
/// @mod{KC,20240308,Created}
/// @mod{MB,20240611,Add documentation}
void AFLUX::correctSQLforAUIDs(string& str){ // KC20240308
  //CO20261007 - the identifier's name in the database; CO20261008 - pocc_system holds an identifier too
  for (const string& column : {entryIdKeyword(), string("pocc_system")}) {
    const string split_id = "\'\"s4e\"\' OR " + column + " LIKE \'\"";
    while(str.find(split_id) != string::npos){str.replace(str.find(split_id), split_id.size(), "\'\"s4e:");}
  }
  while(str.find("\'\"s4e.ai\"\' OR aurl LIKE \'\"") != string::npos){str.replace(str.find("\'\"s4e.ai\"\' OR aurl LIKE \'\""), 26, "\'\"s4e.ai:");}
}

/// @brief Convert AFLUX json response to corresponding html
/// @param input JSON-formatted string
/// @return HTML-formatted string
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240611,Add documentation}
string AFLUX::json2html(const string& input){
    vector<string> rows = splitStringByOuterDelimeter(input, ',',{'{','}'});
    vector<vector<string>> tokens = {};
    const vector<string> meta = responseHiddenKeywords(); //CO20261007 - was data_api, data_source, loop, calculation_cores, aflowlib_version
    vector<vector<string>> data = {};
    vector<string> headers = m_searchKeywords;
    vector<string> tempVec = {};
    vector<string> values = {};
    string row = "";
    string html = "";
    string key = "";
    string value = "";
    string toAdd = "";
    string index = "";
    for (uint i = 0; i < rows.size(); i++){
      row = aurostd::RemoveCharacterFromTheFront(rows[i],'{');
      row = row.substr(0,row.size()-2);
      row = aurostd::RemoveCharacterFromTheBack(row,'}');
      tokens.push_back(splitStringByOuterDelimeter(row, {'[',']'}));
    }

    html += "<!DOCTYPE html>\r";
    html += "<html lang=\"en\">\r";
    html +=   "\t<head>\r";
    html +=     "\t\t<meta charset=\"utf-8\">\r";
    html +=     "\t\t<title>s4e.ai</title>\r";
    html +=   "\t</head>\r";
    html +=   "\t<body>";
    html +=     "<table border =1>";
    html +=       "<thead>";
    html +=         "<tr>";


    for(uint i = 0; i < tokens.size(); i++){
      headers.clear();
      values.clear();
    
      if(m_paging){
        tempVec.clear();
        toAdd.clear();
        index = aurostd::RemoveCharacterFromTheFrontAndBack(splitStringByOuterDelimeter(tokens[i][0],':')[0], '"');
        tempVec = splitStringByOuterDelimeter(tokens[i][0],':');
        tempVec.erase(tempVec.begin());
        value = aurostd::RemoveCharacterFromTheFrontAndBack(aurostd::joinWDelimiter(tempVec,':'),'"');
        value = aurostd::RemoveCharacterFromTheFront(value, '{');
        tokens[i].erase(tokens[i].begin());
        toAdd += "\"index\":"; toAdd += index;
        tokens[i].insert(tokens[i].begin(), value);
        tokens[i].insert(tokens[i].begin(), toAdd);
      }
      for(uint j = 0; j < tokens[i].size(); j++){
        tempVec.clear();
        key = aurostd::RemoveCharacterFromTheFrontAndBack(splitStringByOuterDelimeter(tokens[i][j],':')[0], '"');
        tempVec = splitStringByOuterDelimeter(tokens[i][j],':');
        tempVec.erase(tempVec.begin());
        value = aurostd::RemoveCharacterFromTheFrontAndBack(aurostd::joinWDelimiter(tempVec,':'),'"');
        if(find(meta.begin(), meta.end(), key) == meta.end()){headers.push_back(key); values.push_back(value);}
      }
      data.push_back(values);
    }
    
    for(uint i = 0; i < headers.size(); i++){
      html += "<th>"; html += headers[i]; html += "</th>";
    }
      html +=         "</tr>";
      html +=       "<thead>";
      html +=       "<tbody>";
    for(uint i = 0; i < data.size(); i++){
      html += "<tr>";
      for(uint j = 0; j < data[i].size(); j++){
        html += "<td>"; html += data[i][j]; html += "</td>";
      }
      html += "</tr>";
    }
    html +=     "</tbody>";
    html +=     "</table>";
    html +=   "</body>\r";
    html += "</html>";

    return html;
  }
  
/// @brief Translate numerical inputs to SQL. 
/// @param input number with comparator operator.
/// @return string with SQL compators. Example: "0*" -> ">=0"
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240612,Add documentation}
///
/// @note Table 2 in aflux paper
string AFLUX::translateNumberComparator(const string& input){
  string str = input;
  string output = "";
  if(str.find('*') == string::npos && str.find('!') == string::npos){ // equals
      output = "=" + str;
    } else if(str[0] == '!' && str.find('*') == string::npos){ // not equal
      str.erase(str.begin());
      output = "!=" + str;
    } else if(str[0] == '*' && str.find('!') == string::npos){ // less than or equal
      str.erase(str.begin());
      output = "<=" + str;
    } else if(str[str.size()-1] == '*' && str.find('!') == string::npos){// greater than or eqaul
      str.erase(str.end()-1);
      output = ">=" + str;
    } else if(str[0] == '!' && str[1] =='*'){ // not less than (greater than) (strict)
      str.erase(str.begin());
      str.erase(str.begin());
      output = ">" + str;
    } else if(str[0] == '!' && str[str.size()-1] == '*'){ // not greater than (less than) (strict)
      str.erase(str.begin());
      str.erase(str.end()-1);
      output = "<" + str;
    }
    return output;
  }

/// @brief Translate string input to SQL. 
/// @param input string with a comparator operator
/// @return string with SQL operators. Example: "foo*" -> "LIKE ''foo'%'"
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240612,Add documentation}
///
/// @warning There are no unit tests for this function
/// @note Table 3 in aflux paper. 
string AFLUX::translateString(const string& input){
  string str = aurostd::RemoveWhiteSpacesFromTheFrontAndBack(input);
  string output = "";
  char pct = '%';
  string sub = "";
  if(str.find('*') == string::npos && str.find('!') == string::npos){ //equal
      // ('foo')
      // String is exactly 'foo' 
      output = " LIKE '"; output += "\""; output += str; output += "\""; output += "'";
    } else if(str[0] == '!' && str.find('*') == string::npos){ //not equal
      // (!'foo')
      // String is not 'foo'
      sub = aurostd::RemoveCharacterFromTheFront(str,'!');
      output = " NOT LIKE'"; output += sub; output += "'";
    } else if(str[0] == '*' && str[str.size()-1] != '*' && str.find('!') == string::npos){ //ends with
      // (*'foo')
      // String ends with 'foo'
      sub = aurostd::RemoveCharacterFromTheFront(str,'*');
      output = " LIKE '"; output += pct; output += sub; output += "'";
    } else if(str[0] == '!' && str[1] =='*' && str[str.size()-1] != '*'){ //does not end with
      // (!*'foo')
      // String does not end with 'foo'
      sub = aurostd::RemoveCharacterFromTheFront(aurostd::RemoveCharacterFromTheFront(str,'!'),'*');
      output = " NOT LIKE '"; output += pct; output += sub; output += "'";
    } else if(str[str.size()-1] == '*' && str[0] != '*' && str.find('!') == string::npos){ //starts with
      // ('foo'*)
      // String starts with 'foo'
      sub = aurostd::RemoveCharacterFromTheBack(str,'*');
      output = " LIKE '"; output += sub; output += pct; output += "'";
    } else if(str[0] == '!' && str[1] !='*' && str[str.size()-1] == '*'){ //does not start with
      // (!'foo'*)
      // String does not start with 'foo'
      sub = aurostd::RemoveCharacterFromTheBack(aurostd::RemoveCharacterFromTheFront(str,'!'),'*');
      output = " NOT LIKE '"; output += sub; output += pct; output += "'";
    } else if(str[str.size()-1] == '*' && str[0] == '*' && str.find('!') == string::npos){ //contains
      // (*'foo'*)
      // String contains substring 'foo'
      sub = aurostd::RemoveCharacterFromTheFrontAndBack(str, '*');
      output = " LIKE '"; output += pct; output += sub; output+= pct; output += "'";
    } else if(str[str.size()-1] == '*' && str[1] == '*' && str[0] == '!'){ //does not contain
    // (!*'foo'*)
    // String does not contain substring 'foo'
      sub = aurostd::RemoveCharacterFromTheFrontAndBack(aurostd::RemoveCharacterFromTheFront(str, '!'), '*');
      output = " NOT LIKE '"; output += pct; output += sub; output += pct; output += "'";
  }
  return output;
}

/// @brief Translate param for keyword that returns "strings"
/// @param param search term (ex. "O")
/// @param keyword keyword (ex. "species")
/// @return SQL query string. Example: ("Fe", "species") -> "species LIKE '%"Fe"%'"
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240612,Add documentation}
string AFLUX::genSQLstringS(const string& param, const string& keyword){
  string result = "";
  if(param[0] != '!'){
    result += keyword; result += " LIKE "; result += "'%\""; result += param; result += "\"%'";
  }else if(param[0] == '!'){
    if(param.size() > 1){
      result += keyword; result += " NOT LIKE "; result += "'%\""; result += param.substr(1, param.size()); result += "\"%'";
    }else{
      result += "NOT ";
    }
    
  }
  return result;
}

/// @brief Translate param for keyword that returns "string"
/// @param param search term (ex. "O")
/// @param keyword keyword (ex. "species")
/// @return SQL query string. Example: ("Fe", "species") -> "species LIKE '%"Fe"%'"
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240612,Add documentation}
///
/// @warning There are no unit tests calling this function
string AFLUX::genSQLstring(const string& param, const string& keyword){
  string result = "";
  result += keyword; result += translateString(param);
  return result;
}

/// @brief Translate param for keyword that returns "number"
/// @param param number with non-SQL comparison operator. (ex. "0.5*")
/// @param keyword keyword (ex. "density")
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240612,Add documentation}
///
/// @return SQL query string. Example: ("1*", "volume_atom") -> "volume_atom>=1"
string AFLUX::genSQLnumber(const string& param, const string& keyword){
  string result = "";
  result += keyword; result += translateNumberComparator(param);
  return result;
}

/// @brief Generate SQL that passes when any value of a JSON list passes the comparison
/// @param param comparison, as for genSQLnumber() (Ex. "*5")
/// @param keyword the list (Ex. "delta_radius_covalent_PT_sublattice")
/// @return Ex. "EXISTS (SELECT 1 FROM json_each(delta_radius_covalent_PT_sublattice) WHERE value<=5)"
/// @authors
/// @mod{CO,20261018,created}
string AFLUX::genSQLnumbersAny(const string& param, const string& keyword){
  return "EXISTS (SELECT 1 FROM json_each(" + keyword + ") WHERE value" + translateNumberComparator(param) + ")";
}

/// @brief Generate SQL for queries on vectors of fixed size
/// @param
/// @param
/// @return
/// @warning There are no unit tests calling this function
string AFLUX::genSQLnumbers_fixed(const string& param, const string& keyword){
  vector<string> tokens;
  vector<string> coords;
  string pos = "";
  string val = "";
  string result = "";
  string str = removeOuterParentheses(param);
  if (splitStringByOuterDelimeter(str,',').size()>1){
    tokens = splitStringByOuterDelimeter(str,',');
    //CO20260930 every term joins the clause; the loop returned after the first two
    string clause = "(";
    for (size_t i = 0; i < tokens.size(); i++) {clause += (i == 0 ? "" : " AND ") + genSQLnumbers_fixed(tokens[i],keyword);}
    return clause + ')';
  } else if (splitStringByOuterDelimeter(str,':').size() > 1){
    tokens = splitStringByOuterDelimeter(str,':');
    string clause = "("; //CO20260930
    for (size_t i = 0; i < tokens.size(); i++) {clause += (i == 0 ? "" : " OR ") + genSQLnumbers_fixed(tokens[i],keyword);}
    return clause + ')';
  }else{
    pos = str.substr(str.find('('), str.find(')'));
    val = str.substr(str.find(')')+1, str.size());
    result += "(json_extract("; result += keyword; result += "COMMA '$";
    coords = splitStringByOuterDelimeter(removeOuterParentheses(pos),',');
    for (uint j = 0; j < coords.size(); j++){
        result += '['; result += coords[j]; result += ']';
      }
    result += "')";
    result += translateNumberComparator(removeOuterParentheses(val)); result += ')';
  }
  return result;
}

/// @brief Generate SQL command corresponding to given paging parameter
/// @param param string denoting paging range (ex. "1,5")
/// @return Defines m_pagingParam_n and m_pagingParam_k.
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240612,Add documentation}
void AFLUX::genSQLpaging(const string& param) {
  string str = param;
  if(param.size()){
    if(str.find(',') != string::npos){
      m_pagingParam_n= aurostd::string2utype<int>(splitStringByOuterDelimeter(str)[0]);
      m_pagingParam_k= aurostd::string2utype<int>(splitStringByOuterDelimeter(str)[1]);
    } else { 
      m_pagingParam_n = aurostd::string2utype<int>(str); 
    }
  }
  if (m_pagingParam_n < 0) {
    m_order_by += " DESC";
  } else { 
    m_order_by += " ASC";
  }
}

  /// @brief forces query to have aflow format to work with getRawAFLUXQuery()
  /// @param query query string
  /// @authors
  /// @mod{MB,20240805,created}
  void AFLUX::forceAflowFormat(std::string &query) {
    const string aflow_format="format(aflow)";
    const string aflow_format_encoded="format%28aflow%29";
    const string json_format="format(json)";
    const string json_format_encoded="format%28json%29";
    const string html_format="format(html)";
    const string html_format_encoded="format%28html%29";
    const bool percent_encoded=query.find('%')!=string::npos;

    if(query.find(aflow_format)!=string::npos || query.find(aflow_format_encoded)!=string::npos){return;}

    string::size_type pos=query.find(json_format);
    if(pos!=string::npos){
      query.replace(pos,json_format.size(),percent_encoded?aflow_format_encoded:aflow_format);
      return;
    }
    pos=query.find(json_format_encoded);
    if(pos!=string::npos){
      query.replace(pos,json_format_encoded.size(),aflow_format_encoded);
      return;
    }
    pos=query.find(html_format);
    if(pos!=string::npos){
      query.replace(pos,html_format.size(),percent_encoded?aflow_format_encoded:aflow_format);
      return;
    }
    pos=query.find(html_format_encoded);
    if(pos!=string::npos){
      query.replace(pos,html_format_encoded.size(),aflow_format_encoded);
      return;
    }

    query+=(percent_encoded?"%2C"+aflow_format_encoded:","+aflow_format);
  }

  /// @brief removes the query scope directives from a query local or global
  /// @param query query string
  /// @authors
  /// @mod{MB,20240805,created}
  /// @return query string without the scope directives
  std::string AFLUX::removeQueryScopeDir(const std::string &query) {
    std::vector<std::string> directives = splitAtTopDepth(query);
    std::vector<std::string> new_directives;
    std::vector<std::string>::iterator it;
    for (it = directives.begin(); it != directives.end(); it++) {
      if (*it != "local" || *it != "global") {
        new_directives.push_back(*it);
      }
    }
    return aurostd::joinWDelimiter(new_directives, ",");
  }

  /// @brief checks the AFLUX version of a server
  /// @param server server address
  /// @param path path to the AFLUX API
  /// @return version of the AFLUX server
  aflux::AFLUX::Version AFLUX::checkAFLUXVersion(const std::string& server, const std::string &path) {
    std::string output = "";
    if (200 == aurostd::httpGetStatus(server, path, "?version", output)){
      if (output.find("-v200-") != std::string::npos) {
        m_version = aflux::AFLUX::Version::v200;
      } else { // This should return Unknown, but older AFLUX servers have no version query
        m_version = aflux::AFLUX::Version::v100;
      }
    } else { // This should return Unknown, but older AFLUX servers have no version query
      m_version = aflux::AFLUX::Version::v100;
    }
    return m_version;
  }

/// @brief Prints Help statement
/// @param params Directives (ex. "(foo,bar)")
/// @return Prints schema text of specific directive.
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240612,Add documentation}
void AFLUX::printPropertyHelp(const string& params) {
  vector<string> tokens = splitStringByOuterDelimeter(params);
  string path = XHOST.argv[0];
  path = path.substr(0, path.rfind('/'));
  path += "/README_AFLOW_CHAOS_SCHEMA.TXT";
  aurostd::JSON::object obj = aurostd::JSON::loadFile(path);
  std::regex esc_char("\\\\");

  // If the directive is not found, print the general help text
  if (find(tokens.begin(), tokens.end(), "*") != tokens.end()) {
    string fullText = obj.toString();
    std::regex_replace(std::ostreambuf_iterator<char>(std::cout), 
                        fullText.begin(), fullText.end(), esc_char, "");
    cout << endl;
    return;
  }

  // Print the help text for each directive
  for(uint i = 0; i < tokens.size(); i++){
    string partText = obj[tokens[i]].toString();
    std::regex_replace(std::ostreambuf_iterator<char>(std::cout), 
                        partText.begin(), partText.end(), esc_char, "");
    cout << endl;
    return;
  }
}

/// @brief adds species from atomic params
/// @param query SQL query string
/// @return Edits query string
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add documentation}
///
/// @warning There are no unit tests calling this function
/// @todo This needs more documentation information
void AFLUX::addSpeciesFromAtomicParams(string& query){
  string copy = "";
  vector<string> atoms;
  string atom = "";
  string speciesToAdd = "";
  if(m_atomicFilterParams.size()){
    speciesToAdd += " AND (";
    for(uint i = 0; i < m_atomicFilterParams.size(); i++){
      copy = m_atomicFilterParams[i];
      while(copy.find('@') != string::npos){
        //cout << copy << endl;
        while(copy[0] != '@'){
          copy = copy.substr(1, copy.size());
        }
        atom = copy.substr(copy.find('@')+1,copy.find('(')-1);
        if(count(atoms.begin(), atoms.end(), atom)  == 0){
           atoms.push_back(atom);
        }
        copy.replace(copy.begin(), copy.begin() + copy.find(')')+1, "");
      }
    }
    for(uint i = 0; i < atoms.size(); i++){
      if(i > 0){
        speciesToAdd += " AND ";
      }
      speciesToAdd += "species LIKE '%\""; speciesToAdd += atoms[i]; speciesToAdd += "\"%'";
    }
    speciesToAdd += ')';
  }
  query += speciesToAdd;
}

/// @brief Get result text from DB
/// @param out Determines if data query gets printed out. (help/schema always gets printed out)
/// @return Prints the result of the query to cout.
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation and formatting}
void AFLUX::queryDB(aflowlib::EntryLoader& el) {
  // Check if we are not querying (help, schema, version, etc.)
  if (isNonQuery()) {
    return;
  }

  el.m_out_debug = true;
  if (!el.setSource(aflowlib::EntryLoader::Source::SQLITE)) {
    return;
  }
  el.m_aflux_tensor_filter_keywords = m_tensorFilterKeywords;
  el.m_aflux_tensor_filter_params = m_tensorFilterParams;
  el.m_aflux_atomic_filter_keywords = m_atomicFilterKeywords; // Set filter params so as to not load unfiltered entries into entryloader (added for loadVec)
  el.m_aflux_atomic_filter_params = m_atomicFilterParams;
  el.m_aflux_special_filter_map = m_specialFilterMap;
  el.m_printNull = m_printNull;

  // All true searches will be added to searchKeywords for querying
  std::map<string, bool>::const_iterator it;
  for (it = m_searchMap.cbegin(); it != m_searchMap.cend(); it++) {
    if(it->second) {
      m_searchKeywords.push_back(it->first);
    }
  }

  if (m_useLoadAlloy) { // Edge Case, alloy generation
    vector<string> vec_alloyParams;
    aurostd::string2tokens(m_alloyParams, vec_alloyParams, ",");
    if (!m_SQL_query.empty()) { // Check if m_SQL_query is empty
      m_SQL_query += " AND " + genAlloy(vec_alloyParams);
    } else {
      m_SQL_query += genAlloy(vec_alloyParams);
    }
  }
  
  el.loadSqliteWhere(m_SQL_query, m_searchKeywords, m_order_by); // Load from our DB file
}

/// @brief Print entryloader entries
/// @param el EntryLoader object
/// @return Prints the result of the entries to cout in the specified format.
/// @note This function is called after queryDB() is called.
/// @authors
/// @mod{MB,20240613,Created}
void AFLUX::displayEntries(const aflowlib::EntryLoader& el) {
  aurostd::JSONwriter json;
  vector<aflowlib::_aflowlib_entry> entries;
  uint start = m_pagingParam_n;
  uint end = m_pagingParam_k;

  el.getEntriesFlat(entries);
  m_numEntries = entries.size();
  end = m_numEntries < end ? m_numEntries : end; // If end is greater than the number of entries, set end to the number of entries

  // Default paging is page 1, 64 entries (1,64)
  if ( !m_default_paging ) { // Client Queried with "paging"
    if (m_pagingParam_n > 0) { // Apply paging rules as given in the aflux paper
      start = ((m_pagingParam_n-1)*m_pagingParam_k);
      end = m_pagingParam_k + start;
      if (end > entries.size()) {
        end = entries.size();
      }
    } else if (m_pagingParam_n == 0) { // Client input "paging(0)"
      start = 0;
      end = entries.size();
    } else if (m_pagingParam_n < 0) { // Client is paging backwards
      start = (((m_pagingParam_n * (-1)) - 1) * m_pagingParam_k);
      end = m_pagingParam_k + start;
      if (end > entries.size()) {
        end = entries.size();
      }
    }
  }

  string jsonString = "";
  if( m_paging && m_format != "aflow") { // Display paging
    string key = "";
    for (uint i = start; i < end; i++) {
      key = "";
      key += aurostd::utype2string<uint>(i + 1) + " of " + aurostd::utype2string<uint>(entries.size());
      json.addRawJSON(key, entries.at(i).aflowlib2string("JSON", m_printNull, responseKeyMap())); //CO20261007 - chaos_id; no aurl or version stamps
    }
    jsonString = json.toString(true);
  } else { // Do not display paging info
    for (uint i = start; i < end; i++){
        json.mergeRawJSON(entries.at(i).aflowlib2string("JSON", m_printNull, responseKeyMap())); //CO20261007 - chaos_id; no aurl or version stamps
      }
      jsonString = '[' + json.toString(false) + ']';
  }

  // Print data
  if (m_format == "html") {
    jsonString = json.toString(false); 
    cout << json2html(jsonString) << endl;
    cout << 1 << endl; // Code for index.php to change to appropriate header
    return;
  } else if (m_format == "json") {
    cout << jsonString << endl;
    cout << 0 << endl; // Code for index.php to change to appropriate header
    return;
  } else if (m_format == "aflow") {
    for(uint i = start; i < end; i++) {
      cout << entries.at(i).aflowlib2string("chaos", m_printNull);  //CO20261008 - chaos_path= and chaos_id=, as CHAOS names them
    }
    cout << 2 << endl; // Code for index.php to change to appropriate header
    return;
  }
}

/// @brief Parses query string into generated SQL query for QueryDB() 
/// @param URL full link to API search result Ex. "www.<domain>/?<query>"
/// @return Generates m_SQL_query string and populates atomicFilter and tensorFilter params.
/// @authors
/// @mod{MB,20240628,Created}
void AFLUX::parseQuery(const string& URL) {
  string query = decodeURL(URL);
  query = getQueryFromLink(query); // Remove URL upto "?""

  if (!hasBalancedParentheses(query)) { 
    m_log_message << "Query has unbalanced parentheses";
    outError(__AFLOW_FUNC__, __LINE__, _INPUT_ILLEGAL_);
    return;
  }

  query = aurostd::RemoveWhiteSpaces(query); // Clean query from whitespace chars
  m_AFLUX_query = "?" + query; // Set the query for potential global queries
  vector<string> funcs = splitAtTopDepth(query); // Get list of funcs w params from query
  vector<std::pair<string,string>> funcPairs;
  funcPairs = pairParamsFromFuncs(funcs); // Split funcs and params
  for (std::pair<string,string>& fp : funcPairs) {
    fp.first = keywordFromResponseName(fp.first); //CO20261007 - chaos_id(...) is the identifier
    if (aurostd::RemoveCharacterFromTheFront(fp.first, '$') == "chaos_path") {fp.second = chaosPathParam(fp.second);} //CO20261008 - AURLs and colons in paths
  }

  // Find first ordering (first keyword is order)
  vector<std::pair<string,string>>::const_iterator it;
  for (it = funcPairs.begin(); it != funcPairs.end(); it++) {
    string func = it->first;
    string clean_func = aurostd::RemoveCharacterFromTheFront(func, '$'); // Remove $ for comparison
    if (isKeyword(clean_func)) {
      m_order_by = func;
      break;
    }
  }

  // Begin buidling SQL query
  for (it = funcPairs.begin(); it != funcPairs.end(); it++) {
    string func = it->first;
    string param = it->second;
    string clean_func = aurostd::RemoveCharacterFromTheFront(func, '$'); // Remove $ for comparison
    if (isDirective(clean_func)) {
      handleDirective(func, param);
    } else if (isKeyword(clean_func)) {
      handleKeyword(func, param);
    } else {
      // Error Handling
      m_log_message << "Invalid function in query: " << clean_func;
      outError(__AFLOW_FUNC__, __LINE__, _INPUT_ILLEGAL_);
    }
  }

  // MB20240628 This function does not work well, we can guarantee SQL query ends in ","
  // cleanUpEnd(m_SQL_query); 
  m_SQL_query = aurostd::RemoveCharacterFromTheBack(m_SQL_query, ',');
  replaceCommaAndColonWithAndOr(m_SQL_query);
  replaceCOMMAWithAnActualComma(m_SQL_query);
  std::regex pattern("&");
  m_SQL_query = std::regex_replace(m_SQL_query, pattern, ","); 
  correctSQLforAUIDs(m_SQL_query); // KC20240308
}

/// @brief Handles all directive calls  
/// @param function directive name 
/// @param parameters related parameters to directive if needed.
/// @return populates query attributes or signals for special queies.
/// @authors
/// @mod{MB,20240628,Created}
void AFLUX::handleDirective(const string& function, const string& parameters) {
  bool suppress;
  string func = function;
  string param = removeOuterParentheses(parameters);

  if (func.at(0) == '$') { // Check for suppression operator
    suppress = true;
    func = aurostd::RemoveCharacterFromTheFront(func, '$');
  }

  if (func == "version") {
    m_queryForVersion = true;
  // } else if (func == "local") { MB20240628 Hidden Feature for global queries
  //   m_isLocalQuery = true;
  // } else if (func == "global") {
  //   m_isLocalQuery = false;
  } else if (func == "paging") { 
    m_default_paging = false;
    if (suppress) {
      m_paging = false;
    } else {
      m_paging = true;
    }
    genSQLpaging(param);
  } else if (func == "catalog") {
    if (suppress) {
      m_searchMap["catalog"] = false;
    } else {
      m_searchMap["catalog"] = true;
    }
    genQueryString(func, param);
  } else if (func == "help") { 
    m_helpParams = param;
    m_queryForHelp = true; // Don't query if the AFLUX query is just asking for help/schema
    m_isLocalQuery = true;
  } else if (func == "format") {
    if (aurostd::tolower(param) == "json" // Check if format command is valid
    || aurostd::tolower(param) == "html" 
    || aurostd::tolower(param) == "aflow" ) {
      m_format = aurostd::tolower(param);
    } else {
      m_log_message << "Unknown format: " << param << "accepted formats: (json, html, aflow)";
      outError(__AFLOW_FUNC__, __LINE__, _INPUT_ILLEGAL_);
    }
  } else if (func == "schema") { 
    m_schemaParams = param;
    m_queryForSchema = true; // Don't query if the AFLUX query is just asking for help/schema 
    m_isLocalQuery = true;
  } else {
    m_log_message << "Invalid directive: " << func;
    outError(__AFLOW_FUNC__, __LINE__, _INPUT_ILLEGAL_);
  }
}

/// @brief Handles all keyword calls  
/// @param function keyword name 
/// @param parameters related parameters to keyword if needed
/// @return Generates SQL string from keywords to m_SQL_query
/// @authors
/// @mod{MB,20240628,Created}
void AFLUX::handleKeyword(const string& function, const string& parameters) {
  string func = function;
  string param = removeOuterParentheses(parameters);
  bool suppress = false;

  if (function.at(0) == '$') { // Check for suppression operator
    func = aurostd::RemoveCharacterFromTheFront(func, '$');
    suppress = true;
    if (param.empty()) { // Skip from keyword search if no parameters are given
      m_searchMap[func] = false; 
      return;
    }
  }

  if (func == "alloy") {
    m_useLoadAlloy = true;
    std::regex pattern(":");
    param = std::regex_replace(param, pattern, ",");
    m_alloyParams = param;
    m_searchMap[func] = true;
    m_order_by = entryIdKeyword(); // temp fix to have some default ordering in alloys //CO20261007 - the identifier by its name in the database
  } else { 
    m_searchMap[func] = !suppress;
    genQueryString(func, param);
  }
}

/// @brief Generates SQL query string from function parameters
/// @param function keyword name Ex."bader_net_charges"
/// @param parameters parameters for function Ex."(\@Fe(0.6*))"
/// @authors
/// @mod{MB,20240628,Created}
/// @warning This function should be subdivided further, implementation is also questionable.
void AFLUX::genQueryString(const string& function, const string& parameters) {
  string type = m_keywordMap.at(function);
  string read_params = parameters;
  string edit_params = parameters;

  vector<string> tokens;
  aurostd::string2tokens(read_params, tokens, "():,"); // tokenize parameters

  //CO20261018 - a list of numbers ("numbers", one value per disordered sublattice): @(I) compares value I (from 0, as the
  //tensor filters), and a plain comparison passes when any value does; the type had no branch, and a query stopped
  const bool numbers_component = (type == "numbers" && parameters.find('@') != string::npos);

  m_SQL_query += "("; // Start
  if (type == "strings" || type == "string" || type == "number" || (type == "numbers" && !numbers_component)) { // Simple Expressions (Ex. "species" || "auid" || "density")
    string query = read_params;
    for (uint i = 0; i < tokens.size(); i++) { 
      string gen;
      int token_loc = edit_params.find(tokens.at(i)); // Find the location of the token being replaced
      int token_size = tokens.at(i).size(); 
      query = edit_params.substr(0, token_loc + token_size); // Copy parameter chunk that token is in
      edit_params = edit_params.substr(token_loc + token_size); // Remove parameter chunk from token to avoid overlap
      if (tokens.at(i) == "*") { // Lone * expression is found 
        gen = function + " IS NOT NULL"; 
      } else { // Complex * expression is found or no * is found
        if (type == "strings") {
          gen = genSQLstringS(tokens.at(i), function); // Generate strings query string
        } else if (type == "string") {
          gen = genSQLstring(tokens.at(i), function); // Generate string query string
        } else if (type == "number") {
          gen = genSQLnumber(tokens.at(i), function); // Generate number query string
        } else if (type == "numbers") {
          gen = genSQLnumbersAny(tokens.at(i), function); // any value of the list //CO20261018
        }
      } 
      query.replace(token_loc, token_size, gen); // Replace token with generated query string
      m_SQL_query += query; // Add generated chunk to larger SQL query
    }
    m_SQL_query += edit_params; // Add left over query syntax.

  } else if (type == "numbers_variable") { // Complex number expression (Ex. "bader_net_charges")
    if (tokens.size() == 1 && tokens.at(0) == "*") { // Lone * expression is found
      m_SQL_query += function + " IS NOT NULL";
    } else { // Complex * expression is found or no * is found
      m_atomicFilterParams.push_back(edit_params);
      m_atomicFilterKeywords.push_back(function); 
      m_SQL_query += function + " IS NOT NULL"; // Makes sure that DB is queried for data to filter
    }
    
  } else if (type == "numbers_fixed" || numbers_component) { // Complex tensor expression (Ex. "ael_stiffness_tensor")
    if (tokens.size() == 1 && tokens.at(0) == "*") { // Lone * expression is found
      m_SQL_query += function + " IS NOT NULL";
    } else { // Complex * expression is found or no * is found
      if (!numbers_component) { //CO20261018 - the SQL is exact for a list; the C++ check knows only the tensors
        m_tensorFilterParams.push_back(edit_params);
        m_tensorFilterKeywords.push_back(function);
      }
      m_SQL_query += genSQLnumbers_fixed(edit_params, function); 
    }
    
  } else if (type == "numbers_variable_special") { // Complex calculation expression (Ex. only "nbondxx")
    if (tokens.size() == 1 && tokens.at(0) == "*") { // Lone * expression is found
      m_SQL_query += function + " IS NOT NULL";
    } else { // Complex * expression is found or no * is found
      m_specialFilterMap[function] = edit_params;
      m_SQL_query += function + " IS NOT NULL"; // Makes sure that DB is queried for data to filter
    }
  } else {
    m_log_message << "Unknown keyword type: " << function << " : " << type;
    outError(__AFLOW_FUNC__, __LINE__, _INPUT_ILLEGAL_);
  }
  m_SQL_query += ")";
  m_SQL_query += ","; // End
}

  /// @brief generate an alloy set
  /// @param alloy alloy descriptor
  /// @param recursive include sub alloys
  /// @authors
  /// @mod{MB,20240718,created}
  /// @note an alloy can be given as comma separated list `"Cu, Au, Fe"`
  ///       or as combined string `"CuAuFe"` (elements don't need to be sorted)
  /// @note `recursive=true` (default) will generate sub-alloys so `CuAuFe`
  ///       will load `AuCuFe`, `AuCu`, `AuFe`, `CuFe`, `Au`, `Cu`, and `Fe`
  /// @note this function is a rework of HE's loadAlloy function without loading the data
  std::string AFLUX::genAlloy(const std::string &alloy, bool recursive) {
    std::vector <std::string> alloy_elements;
    if (alloy.find(", ") != std::string::npos) aurostd::string2tokens(alloy, alloy_elements, ", ");
    else if (alloy.find(',') != std::string::npos) aurostd::string2tokens(alloy, alloy_elements, ",");
    else alloy_elements = aurostd::getElements(alloy);
    return genAlloy(alloy_elements, recursive);
  }

  /// @brief generates alloy string for later query use
  /// @param alloy split alloy elements
  /// @param recursive include sub alloys
  /// @authors
  /// @mod{MB,20240718,created}
  /// @note this function is a rework of HE's loadAlloy function without loading the data
  std::string AFLUX::genAlloy(const std::vector <std::string> &alloy, bool recursive) {
    std::vector <std::string> alloy_clean;
  
    // check that all parts of alloy are actually elements; a part written as
    // elements together (CoMgO) is split into them, and repeats are dropped //CO20260927
    for (std::vector<std::string>::const_iterator element = alloy.begin(); element != alloy.end(); element++) {
      std::vector<std::string> parts;
      if (xelement::xelement::isElement(*element) != 0) {
        parts.push_back(*element);
      } else {
        // getElements() needs a capital first letter (it appends to the last element found) //CO20261004
        const std::string& word = *element;
        bool letters = !word.empty() && std::isupper(static_cast<unsigned char>(word[0]));
        for (const char ch : word) {
          if (!std::isalpha(static_cast<unsigned char>(ch))) letters = false;
        }
        if (letters) parts = aurostd::getElements(word); //CO20261004
      }
      bool all_elements = !parts.empty();
      for (const std::string& part : parts) {
        if (xelement::xelement::isElement(part) == 0) all_elements = false;
      }
      if (!all_elements) {
        m_log_message << *element << " is not an element";
        // outError(__AFLOW_FUNC__, __LINE__); This stops the entire program
        continue;
      }
      for (const std::string& part : parts) {
        if (std::find(alloy_clean.begin(), alloy_clean.end(), part) == alloy_clean.end()) alloy_clean.emplace_back(part);
      }
    }
    // the systems in one order whatever the order of the input (each system was sorted, the list was not) //CO20261005
    std::sort(alloy_clean.begin(), alloy_clean.end());

    // every subset is generated, so bound the number of elements //CO20260927
    const size_t max_recursive_elements = 12;
    if (recursive && alloy_clean.size() > max_recursive_elements) {
      m_log_message << "alloy() takes at most " << max_recursive_elements << " elements";
      outError(__AFLOW_FUNC__, __LINE__, _INPUT_ILLEGAL_);
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
    m_log_message << "Generating " << final_alloy_list.size() << " systems: ";
    for (std::vector<std::string>::const_iterator i_alloy = final_alloy_list.begin(); i_alloy != final_alloy_list.end(); i_alloy++) {
      m_log_message << "\"" << *i_alloy << "\" ";
    }
    // outInfo(__AFLOW_FUNC__);

    std::string where = aurostd::joinWDelimiter(final_alloy_list, "','");
    // The alloy column compares without case (COLLATE NOCASE), so CoO would
    // also match CO (carbon and oxygen). The first test uses the index, the
    // second keeps only exact matches. //CO20260927
    const std::string list = "('" + where + "')";
    where = "(alloy IN " + list + " AND alloy COLLATE BINARY IN " + list + ")";
    return where;
  }

/// @brief Parses and Queries the query input. Runs the entire code.
/// @param query Full URL string (link to api containing the query)
/// @return Prints query result to cout
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation}
void parseAndQuery(const string& query) {
    aflux::AFLUX trans;
    trans.parseQuery(query);
    aflowlib::EntryLoader el;

    // Check if we are doing a global query
    if (trans.m_isLocalQuery) {
      trans.queryDB(el); // Local query (SQLite)
    } else {
      el.loadAFLUXQuery(trans.m_AFLUX_query); // Global query (AFLUX)
    }

    // Check if we are not querying
    if (trans.isNonQuery()) {
      trans.printNonQuery(); // Print non-query results (help, schema, etc.)
    } else {
      trans.displayEntries(el); // Display the entries from QueryDB or loadAFLUXQuery
    }
}

/// @brief Test EntryLoader on RawAFLUXQuery
/// @param query Raw AFLUX query (link to api containing the query)
/// @return Prints list of query result strings on cout.
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation}
void test(const string& query){
  aflowlib::EntryLoader el;
  vector<string> out = el.getRawAFLUXQuery(query);
  cout << out.size() << endl;
}

//******************************* FILTER FUNCTIONS BELOW **********************

//CO20261018 START - the list a one-index filter reads (atom-specific, 1D tensor, nbondxx) and the comparison of a term
namespace {
  /// @brief value pos (from 0) of the list keyword of an entry
  /// @return false when the entry has no such value, so a filter on it does not pass
  bool filterValue1D(const aflowlib::_aflowlib_entry& entry, const string& keyword, size_t pos, double& value) {
    const vector<double>* list = nullptr;
    if (keyword == "bader_net_charges") {list = &entry.vbader_net_charges;}
    else if (keyword == "bader_atomic_volumes") {list = &entry.vbader_atomic_volumes;}
    else if (keyword == "stress_tensor") {list = &entry.vstress_tensor;}
    else if (keyword == "nbondxx") {list = &entry.vnbondxx;}
    if (list == nullptr || pos >= list->size()) {return false;}
    value = (*list)[pos];
    return true;
  }

  /// @brief the comparison of an AFLUX number term (Table 2 of the AFLUX paper), as translateNumberComparator() writes it
  ///        in SQL: "5" equal, "!5" not equal, "*5" at most, "5*" at least, "!*5" more than, "!5*" less than
  bool compareNumberTerm(double value, const string& term) {
    if (term.empty()) {return false;}
    const bool star = (term.find('*') != string::npos), bang = (term.find('!') != string::npos);
    if (!star && !bang) {return value == aurostd::string2utype<double>(term);}
    if (term[0] == '!' && !star) {return value != aurostd::string2utype<double>(term.substr(1));}
    if (term[0] == '*' && !bang) {return value <= aurostd::string2utype<double>(term.substr(1));}
    if (term.back() == '*' && !bang) {return value >= aurostd::string2utype<double>(term.substr(0, term.size() - 1));}
    if (term.size() > 1 && term[0] == '!' && term[1] == '*') {return value > aurostd::string2utype<double>(term.substr(2));}
    if (term.size() > 1 && term[0] == '!' && term.back() == '*') {return value < aurostd::string2utype<double>(term.substr(1, term.size() - 2));}
    return false;
  }
}
//CO20261018 STOP

/// @brief Calls aflowlib_entry funtion to evaluate expression
/// @param expression Atom and its coresponding comparison operation. Ex. "\@O(*0)"
/// @param keyword Search term expression is compared against. Ex. "bader_net_charges"
/// @param entry Prototype entry from aflow library
/// @return True if expression meets the comparison operations in keyword 
/// for specified entry. False otherwise.
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation and formatting}
bool evaluateAtomicExpression(const string& expression, const string& keyword, const aflowlib::_aflowlib_entry& entry){
  string atom = expression.substr(1, expression.find('(') - 1);
  string val = removeOuterParentheses(expression.substr(expression.find('('), expression.size() - 1));
  int count = 0;
  uint atomIndex = 0;
  vector<string> compNums;
  vector<string>::const_iterator it = find(entry.vspecies.begin(), entry.vspecies.end(), atom);
  if (it == entry.vspecies.end()) {return false;} // Check if atom exists in entry
  atomIndex = distance(entry.vspecies.begin(), it);

  if (entry.composition.size() < 1) {return false;} // Check if entry composition exists
  compNums = splitStringByOuterDelimeter(entry.composition);
  if (compNums.size() <= atomIndex) {return false;}

  for (uint i = 0; i < atomIndex; i++){
    count += aurostd::string2utype<int>(compNums[i]);
  }
  const int natoms = aurostd::string2utype<int>(compNums[atomIndex]);

  //CO20261018 - the entry passes when any atom of the element passes (IF ANY, as the CHAOS paper describes the filter); the
  //loop returned at the first atom, and it read the comparison again at every atom after erasing its characters
  double value = 0.0;
  for (int j = count; j < count + natoms; j++){
    if (filterValue1D(entry, keyword, (size_t) j, value) && compareNumberTerm(value, val)) {return true;}
  }
  return false;
}

/// @brief Checks all specifications to determine if entry should be added to response data.
/// @param input String of multiple expresions of atoms and their coresponding 
/// comparison operations. Ex."\@O(*0),\@Fe(0.5*)"
/// @param keyword Search term expression is compared against. Ex. "bader_net_charges"
/// @param entry Prototype entry from aflow library
/// @return True if all expressions within input meet the comparison operations in keyword 
/// for specified entry. False otherwise.
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation and formatting}
bool applyAtomicFilter(const string& input, const string& keyword, const aflowlib::_aflowlib_entry& entry){
  vector<string> tokens;
  string str = removeOuterParentheses(input);
  if (splitStringByOuterDelimeter(str,',').size() > 1) {
    tokens = splitStringByOuterDelimeter(str, ',');
    //CO20260930 every term counts; the loop returned after the first two
    bool all_pass = true;
    for (size_t i = 0; i < tokens.size() && all_pass; i++) {all_pass = applyAtomicFilter(tokens[i], keyword, entry);}
    return all_pass;
  } else if (splitStringByOuterDelimeter(str, ':').size() > 1){
    tokens = splitStringByOuterDelimeter(str, ':');
    bool any_pass = false; //CO20260930
    for (size_t i = 0; i < tokens.size() && !any_pass; i++) {any_pass = applyAtomicFilter(tokens[i], keyword, entry);}
    return any_pass;
  } else {
    return evaluateAtomicExpression(str, keyword, entry);
  }
  return false;
}

/// @brief Filters a single entry based on list of properties (called in entry_loader)
/// @param entry Prototype entry from aflow library
/// @param keywords List of all search terms parmas is compared against. 
/// Ex."{"bader_net_charges", "density"}"
/// @param params List of multiple expresions of atoms and their coresponding 
/// comparison operations. Ex. "{ "\@O(*0)", "\@Fe(0.5*)" }"
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation and formatting}
bool atomicFilterSingleEntry(const aflowlib::_aflowlib_entry entry, const vector<string> keywords, const vector<string> params){
    if (keywords.size()) { // Non-zero number of keywords
      for (uint j = 0; j < params.size(); j++) { // Iterate through all parameters
        if(!applyAtomicFilter(params[j], keywords[j], entry)) { // Checks if filter was unable to apply
          return false; 
        }
      }
    }
    return true; // All filters were properly applied
  }

/// @brief Filters a single entry based on list of properties (called in entry_loader)
/// @param entry Prototype entry from aflow library
/// @param keywords List of all search terms parmas is compared against. 
/// Ex."{"stress_tensor", "ael_stiffness_tensor"}"
/// @param params List of multiple expresions of tensors and their coresponding 
/// comparison operations. {"\@(1)(0*)", "\@(0,0)(*0.5)"}
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation and formatting}
bool tensorFilterSingleEntry(const aflowlib::_aflowlib_entry entry, const vector<string> keywords, const vector<string> params) {
  if (keywords.size()) { // Non-zero number of keywords
    for (uint j = 0; j < params.size(); j++) { // Iterate through all parameters
      if(!applyTensorFilter(params[j], keywords[j], entry)) { // Checks if filter was unable to apply
        return false;
      }
    }
  }
  return true; // All filters were properly applied
}

/// @brief Checks all specifications to determine if entry should be added to response data.
/// @param input String of multiple expresions of tensors and their coresponding 
/// comparison operations. Ex."(\@(1,1)(0*)),(\@(0)(*0.5))"
/// @param keyword Search term expression is compared against. Ex. "stress_tensor"
/// @param entry Prototype entry from aflow library
/// @return True if all expressions within input meet the comparison operations in keyword 
/// for specified entry. False otherwise.
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation and formatting}
bool applyTensorFilter(const string& input, const string& keyword, const aflowlib::_aflowlib_entry& entry){
  vector<string> tokens;
  string str = removeOuterParentheses(input);
  if (splitStringByOuterDelimeter(str,',').size() > 1) {
    tokens = splitStringByOuterDelimeter(str, ',');
    //CO20260930 every term counts; the loop returned after the first two
    bool all_pass = true;
    for (size_t i = 0; i < tokens.size() && all_pass; i++) {all_pass = applyTensorFilter(tokens[i], keyword, entry);}
    return all_pass;
  } else if (splitStringByOuterDelimeter(str, ':').size() > 1) {
    tokens = splitStringByOuterDelimeter(str, ':');
    bool any_pass = false; //CO20260930
    for (size_t i = 0; i < tokens.size() && !any_pass; i++) {any_pass = applyTensorFilter(tokens[i], keyword, entry);}
    return any_pass;
  } else {
    return evaluateTensorExpression(str, keyword, entry);
  }
  return false;
}

/// @brief Calls aflowlib_entry funtion to evaluate expression
/// @param expression Tensor and its coresponding comparison operation. Ex. "(\@(0)(*1))" or "(\@(1,1)(0*))"
/// @param keyword Search term expression is compared against. Ex. "stress_tensor"
/// @param entry Prototype entry from aflow library
/// @return True if expression meets the comparison operations in keyword 
/// for specified entry. False otherwise.
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation and formatting}
bool evaluateTensorExpression(const string& expression, const string& keyword, const aflowlib::_aflowlib_entry& entry){
  string position = expression.substr(2, expression.find(')') - 2); // "(@" are the first two chars. Yields "(#)" or "(#,#)"

  if (aurostd::string2utype<int>(position) < 0) { // MB20240701 If Bond does not exist
    return false;
  }

  string val = removeOuterParentheses(expression.substr(expression.find('(',expression.find(position)),expression.size() - 1));
  uint pos1 = 0;
  uint pos2 = 0;
  aflowlib::_aflowlib_entry ent = entry; // Can't call the .comparision functions on const? 

  if (splitStringByOuterDelimeter(position,',').size() > 1){ // FOR 2D TENSOR
    pos1 = aurostd::string2utype<int>(splitStringByOuterDelimeter(position,',')[0]);
    pos2 = aurostd::string2utype<int>(splitStringByOuterDelimeter(position,',')[1]);
    if (val.find('*') == string::npos && val.find('!') == string::npos) { // equals
      return ent.equalTo(keyword, pos1, pos2, aurostd::string2utype<double>(val));
    } else if (val[0] == '!' && val.find('*') == string::npos) { // not equal
      val.erase(val.begin()); // Removes '!'
      return !ent.equalTo(keyword, pos1, pos2, aurostd::string2utype<double>(val));
    } else if (val[0] == '*' && val.find('!') == string::npos) { // less than
      val.erase(val.begin()); // Removes '*'
      return ent.lessThanOrEqual(keyword, pos1, pos2, aurostd::string2utype<double>(val));
    } else if (val[val.size()-1] == '*' && val.find('!') == string::npos) {// greater than
      val.erase(val.end()-1); // Removes '*'
      return ent.greaterThanOrEqual(keyword, pos1, pos2, aurostd::string2utype<double>(val));
    } else if (val[0] == '!' && val[1] =='*') { // not less than (greater than) (strict)
      val.erase(val.begin()); // Removes '!'
      val.erase(val.begin()); // Removes '*'
      return ent.greaterThan(keyword, pos1, pos2, aurostd::string2utype<double>(val));
    } else if (val[0] == '!' && val[val.size()-1] == '*') { // not greater than (less than) (strict)
      val.erase(val.begin()); // Removes '!'
      val.erase(val.end()-1); // Removes '*'
      return ent.lessThan(keyword, pos1, pos2, aurostd::string2utype<double>(val));
    }
  } else { // FOR 1D TENSOR
    //CO20261018 - components count from 0, as in the SQL (json_extract()) of the same filter, in the 2D tensors and in the
    //pairs filterNbondxx() gives; from 1 here, the check read the component before the one named
    double value = 0.0;
    return filterValue1D(entry, keyword, (size_t) aurostd::string2utype<int>(position), value) && compareNumberTerm(value, val);
  }
  return false;
}

/// @brief Filters entry by whether entry passes all nbonxx parameter comparisons.
/// @param entry Prototype entry from aflow library
/// @param paramMap Map of all parameters in query.
/// @return True if entry passed all nbondxx parameters. False otherwise.
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation and formatting}
bool filterSpecialCPP(const aflowlib::_aflowlib_entry& entry, const std::map<string,string>& paramMap){
  for (auto it = paramMap.begin(); it != paramMap.end(); it++) {
    if (aurostd::tolower(it->first) == "nbondxx") {
      if (!filterNbondxx(it->second, entry)) {
        return false;
      }
    }
  }
  return true;
}

/// @brief Filters entries by nearest neighboring bond lengths nbondxx.
/// @param params Expressions containing Nbondxx parameters Ex. "(\@(O,O)(1.3*))" or "(@(O)(*1.4))"
/// @param entry Prototype entry from aflow library
/// @return True if within Angstrom distance set by params. False otherwise.
///
/// @authors
/// @mod{WS,20231006,Created}
/// @mod{MB,20240613,Add Documentation and formatting}
bool filterNbondxx(const string& params, const aflowlib::_aflowlib_entry& entry){
  //CO20261018 - each @(El1,El2) becomes the index of the pair in nbondxx, whose pairs run (0,0),(0,1),...,(0,n-1),(1,1),...
  //over the species in order (GetNBONDXX()), or -1 when the entry lacks either species. The index came from the pairs
  //sorted by name and was then read from 1 (the pair before), a pair the entry lacks kept the previous term's index, and
  //a term in its own parentheses lost its species
  const vector<string> species = entry.getSpecies();
  const int nspecies = (int) species.size();
  string toUse = params;
  size_t at = 0;
  while ((at = toUse.find("@(", at)) != string::npos) {
    const size_t close = toUse.find(')', at);
    if (close == string::npos) {break;}
    const string pair_text = toUse.substr(at + 2, close - at - 2);
    const vector<string> pair = splitStringByOuterDelimeter(pair_text);
    int index = -1;
    if (pair.size() == 2) {
      int ia = (int) (find(species.begin(), species.end(), aurostd::RemoveWhiteSpaces(pair[0])) - species.begin());
      int ib = (int) (find(species.begin(), species.end(), aurostd::RemoveWhiteSpaces(pair[1])) - species.begin());
      if (ia < nspecies && ib < nspecies) {
        if (ia > ib) {std::swap(ia, ib);}
        index = ia * nspecies - ia * (ia - 1) / 2 + (ib - ia);
      }
    }
    const string index_text = aurostd::utype2string<int>(index);
    toUse.replace(at + 2, pair_text.size(), index_text);
    at += 2 + index_text.size();
  }
  return applyTensorFilter(toUse, "nbondxx", entry);
}

/// @brief Report an Error but do not throw exception to display onto Website and API requests.
/// @param function_name Function where error occcured
/// @param line_number Line where error occured
/// @param error_type Corresponding error code
/// @authors
/// @mod{MB,20240613,Created}
void AFLUX::outError(const std::string &function_name, int line_number, int error_type) {
  cout << "ERROR " << error_type << endl;
  cout << m_log_message.str() << endl;
  cout << (std::string) __FILE__ + ":" + aurostd::utype2string<int>(line_number) << " @ " << function_name << endl;
  throw aurostd::xerror((std::string) __FILE__ + ":" + std::to_string(line_number), function_name, m_log_message, error_type);
  aurostd::StringstreamClean(m_log_message);
}

//************************* AFLUX HELP/SCHEMA ***************************************

// @mod{MB, 20240611, Rename from m_aliasesDict to m_aliasesMap}
void AFLUX::loadAliasesMap(){
  m_aliasesMap = {{"AlkaliMetals","(Li:Na:K:Rb:Cs)"},
                   {"TransitionMetals","(Sc:Ti:V:Cr:Mn:Fe:Co:Ni:Cu:Zn:Y:Zr:Nb:Mo:Tc:Ru:Rh:Pd:Ag:Cd:Hf:Ta:W:Re:Os:Ir:Pt:Au:Hg)"},
                   {"OtherMetals","(Al:Ga:Ge:In:Sn:Sb:Tl:Pb:Bi)"},
                   {"NonMetals","(He:B:C:N:O:F:Ne:Si:P:S:Cl:Ar:As:Se:Br:Kr:Te:I:Xe)"},
                   {"Metals","(Li:Be:Na:Mg:Al:K:Ca:Sc:Ti:V:Cr:Mn:Fe:Co:Ni:Cu:Zn:Ga:Rb:Sr:Y:Zr:Nb:Mo:Tc:Ru:Rh:Pd:Ag:Cd:In:Sn:Sb:Cs:Ba:La:Ce:Pr:Nd:Pm:Sm:Eu:Gd:Tb:Dy:Ho:Er:Tm:Yb:Lu:Hf:Ta:W:Re:Os:Ir:Pt:Au:Hg:Tl:Pb:Bi)"},
                   {"AlkaliEarths","(Be:Mg:Ca:Sr:Ba)"},
                   {"Lanthanides","(La:Ce:Pr:Nd:Pm:Sm:Eu:Gd:Tb:Dy:Ho:Er:Tm:Yb:Lu)"},
                   {"BoronGroup","(B:Al:Ga:In:Tl)"},
                   {"CarbonGroup","(C:Si:Ge:Sn:Pb)"},
                   {"Pnictogens","(N:P:As:Sb:Bi)"},
                   {"Chalcogens","(O:S:Se:Te:Po)"},
                   {"Halogens","(F:Cl:Br:I:At)"},
                   {"aflow_prototype_parameters_orig","(aflow_prototype_params_list_orig,aflow_prototype_params_values_orig)"},
                   {"aflow_prototype_parameters_relax","(aflow_prototype_params_list_relax,aflow_prototype_params_values_relax)"},
                   {"Wyckoff_positions_orig","(Wyckoff_letters_orig,Wyckoff_multiplicities_orig,Wyckoff_site_symmetries_orig)"},
                   {"Wyckoff_positions","(Wyckoff_letters,Wyckoff_multiplicities,Wyckoff_site_symmetries)"}};
}

/// @brief Load all help text from README_AFLOW_AFLUX_HELP_TEXT
/// @param type Type of help text to load
/// @authors
/// @mod{MB, 20240611, Created}
void AFLUX::loadHelpText(const string& type) {
  
  if (type == "properties" || type == "schema"){
    printPropertyHelp("*");
    return;
  } else if (isKeyword(type)) {
    printPropertyHelp(type);
    return;
  }
  string path = XHOST.argv[0];
  path = path.substr(0, path.rfind('/'));
  path += "/README_AFLOW_CHAOS_HELP.TXT";

  ifstream file(path);
  string section = "-"+type;
  transform(section.begin(), section.end(), section.begin(), ::toupper);
  string line;
  bool section_body = false;
  while(getline(file, line)) {
    if (line == section + " START-") {
      	section_body = true;
    } else if (line == section + " END-") {
      	section_body = false;
    } else if (line == "-PRE START-") {
      	section_body = true;
    } else if (line == "-PRE END-") {
      	section_body = false;
    } else if (line == "-POST START-") {
      	section_body = true;
    } else if (line == "-POST END-") {
      	section_body = false;
    } else if (section_body == true) {
      	m_AFLUXHelpText += line + "\n";
    }
  }
  file.close();
}

}// namespace aflux

// ***************************************************************************
// *                                                                         * 
// *           COREY OSES - Johns Hopkins University 2022-2023               *
// *                                                                         *
// ***************************************************************************
