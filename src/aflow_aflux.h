// ***************************************************************************
// *                                                                         *
// *          Aflow COREY OSES - Johns Hopkins University 2022-2026          *
// *         Aflow WILL SHIBER - Johns Hopkins University 2023-2024          *
// *      Aflow MATTHEW BROWNRIGG - Johns Hopkins University 2024-2025       *
// *         Aflow KEITH CLARK - Johns Hopkins University 2023-2024          *
// *                                                                         *
// ***************************************************************************
#ifndef _AFLOW_AFLUX_H_
#define _AFLOW_AFLUX_H_
namespace aflux{
void test(const string& query);
void parseAndQuery(const string& query);
// ******************** HELPER FUNCTIONS ********************
vector<aflowlib::_aflowlib_entry> filterNumbers_variable(const vector<aflowlib::_aflowlib_entry>& entryList);
vector<string> splitStringByOuterDelimeter(const string& input);
vector<string> splitStringByOuterDelimeter(const string& input, const char& delimeter);
vector<string> splitStringByOuterDelimeter(const string& input, const vector<char>& container);
vector<string> splitStringByOuterDelimeter(const string& input, const char& delimeter, const vector<char>& container);
vector<string> splitStringByOuterDelimeter(const string& input, const vector<char>& delimeterList, const vector<char>& container);
string removeOuterParentheses(const string& str);
string removeOuterParentheses(const string& str, const vector<char>& container);
size_t findNextOccurrence(const string& str, const vector<char>& charList, size_t startPos = 0);
size_t findNextOccurrence(const string& str, const vector<string>& stringList, size_t startPos = 0);
string decodeURL(const std::string& encoded); //MB20240702
bool hasBalancedParentheses(const string& query); //MB20240628
vector<string> spiltAtTopDepth(const string& input, const char& delim); //MB20240628
vector<std::pair<string, string>> pairParamsFromFuncs(const vector<string>& funcs); //MB20240628
bool applyAtomicFilter(const string& input, const string& keyword, const aflowlib::_aflowlib_entry& entry);
bool evaluateAtomicExpression(const string& expression, const string& keyword, const aflowlib::_aflowlib_entry& entry);
bool atomicFilterSingleEntry(const aflowlib::_aflowlib_entry entry, const vector<string> keywords, const vector<string> params);
bool evaluateTensorExpression(const string& expression, const string& keyword, const aflowlib::_aflowlib_entry& entry);
bool applyTensorFilter(const string& input, const string& keyword, const aflowlib::_aflowlib_entry& entry);
bool tensorFilterSingleEntry(const aflowlib::_aflowlib_entry entry, const vector<string> keywords, const vector<string> params);
bool filterNbondxx(const string& params, const aflowlib::_aflowlib_entry& entry);
bool filterSpecialCPP(const aflowlib::_aflowlib_entry& entry, const std::map<string,string>& paramMap);
string chaosIdName(); //CO20261007
string entryIdKeyword(); //CO20261007
string entryPathKeyword(const string& id_keyword=entryIdKeyword()); //CO20261008
std::map<string,string> responseKeyMap(const string& id_keyword=entryIdKeyword()); //CO20261007
vector<string> responseHiddenKeywords(const string& id_keyword=entryIdKeyword()); //CO20261007
string keywordFromResponseName(const string& name, const string& id_keyword=entryIdKeyword()); //CO20261007
string chaosPathParam(const string& param); //CO20261008
//******************** AFLUX (translator) class *************
class AFLUX : public xStream {
    public:
		AFLUX();

		~AFLUX();
		const AFLUX& operator=(const AFLUX& other);
		void clear();

		enum Version {UNKNOWN = 0, v100 = 100, v200 = 200}; // MB20240805 Add version control
		std::map<string, string> m_keywordMap; // MB20240611 Map of all keywords and their types
		vector<string> m_keywordList;
		bool m_paging;
		string m_SQL_query;
		string m_AFLUX_query;
		string m_schemaParams;
		string m_helpParams;
		string m_AFLUXHelpText;
		string m_AFLUXSchemaText;
		vector<string> m_atomicFilterParams; //params that need to be filtered with c++
		vector<string> m_atomicFilterKeywords; //keywords that need to be filtered with c++
		vector<string> m_tensorFilterParams;
		vector<string> m_tensorFilterKeywords;
		vector<string> m_defaultKeywords; //default keywords to always include
		vector<char> m_queryLogicChars; //characters in query that should not be translated
		vector<string> m_searchKeywords; //keywords to be included in SQlite search
		std::map<string,bool> m_searchMap; // MB20240712 O(1) hash keywords for SQlite search 
		uint m_pagingParam_k; //paging parameter
		int m_pagingParam_n; //paging parameter
		bool m_default_paging; //MB20240710 add default paging mode
		bool m_isLocalQuery; //MB20240729 flag to indicate if the query is local
		string m_format;
		string m_order_by;
		vector<string> m_directiveList;
		bool m_queryForHelp;
		bool m_queryForSchema;
		bool m_printNull;
		bool m_useLoadAlloy;
		std::map<string,string> m_aliasesMap; //MB20240611
		string m_alloyParams;
		uint m_numEntries;
		std::map<string,string> m_specialFilterMap;
		stringstream m_log_message; //MB20240709
		Version m_version; // MB20240805 Add version control
		bool m_queryForVersion; // MB20240805 Add version control
		void parser(const string& query);
		void queryDB(aflowlib::EntryLoader& el);
		void displayEntries(const aflowlib::EntryLoader& el);
		void genSQLpaging(const string& param);
		void forceAflowFormat(string &query);
		string removeQueryScopeDir(const string &query); // MB20240807
		Version checkAFLUXVersion(const string& server, const string &path); // MB20240807
		void printPropertyHelp(const string& param = "*"); // MB20240807
		string genSQLnumber(const string& param, const string& keyword);
		string genSQLstring(const string& param, const string& keyword);
		string genSQLstringS(const string& param, const string& keyword);
		string genSQLnumbers_fixed(const string& param, const string& keyword);
		string genSQLnumbersAny(const string& param, const string& keyword); //CO20261018
		string translateString(const string& input);
		string translateNumberComparator(const string& input);
		string json2html(const string& input);
		uint getNumEntries(const aflowlib::EntryLoader& el); // MB20240805
		bool isNonQuery(); // MB20240807
		void printNonQuery(); // MB20240807
		vector<string> splitAtTopDepth(const string& input, const char& delim = ',') const;
		int getEntryKeywordSize(const aflowlib::_aflowlib_entry& entry, const string& keyword);
		bool isDirective(const string& input);
		bool isKeyword(const string& input); //MB20240625
		bool isQueryLogicChar(const char& input);
		vector<string> getKeysFromMap(const std::map<string, string>& input);
		void replaceCOMMAWithAnActualComma(string& input);
		void replaceCommaAndColonWithAndOr(string& str);
		void genQueryString(const string& function, const std::string& parameters); //MB20240625
		string genAlloy(const string &alloy, bool recursive=true); //MB20240718
		string genAlloy(const vector<std::string> &alloy, bool recursive=true); //MB20240718
		void parseQuery(const string& URL); //MB20240625
		void handleDirective(const string& func, const string& param); //MB20240625
		void handleKeyword(const string& function, const string& parameters); //MB20240625
		void correctSQLforAUIDs(string& str); //KC20240308
		string getQueryFromLink(const string& input);
		string replaceAliases(const string& input);
		bool applyFilter(const string& input);
		std::map<string, string> createKeywordMap();
		void loadHelpText(const string& type = "general"); //MB20240722
		void loadAliasesMap(); // MB20240611
		void addSpeciesFromAtomicParams(string& query);
		void outError(const std::string &function_name, int line_number, int error_type); //MB20240709

	private:
      void init();
	  void initSearchMap();
      void free();
      void copy(const AFLUX& b);
	};



}
// namespace aflux

#endif  // _AFLOW_AFLUX_H_
// ***************************************************************************
// *                                                                         *
// *           COREY OSES - Johns Hopkins University 2022-2023               *
// *                                                                         *
// ***************************************************************************
