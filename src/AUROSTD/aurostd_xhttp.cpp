// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2023           *
// *                                                                         *
// ***************************************************************************
// Written by Frisco Rose in 2018
// Complete rewrite by Hagen Eckert in 2022
// hagen.eckert@duke.edu

#ifndef _AUROSTD_XHTTP_CPP_
#define _AUROSTD_XHTTP_CPP_

#include "aurostd_xhttp.h"
#include <spawn.h> //CO20261002 - posix_spawn for curl

extern char **environ; //CO20261002 - passed to curl

#define _DEBUG_XHTTP_ false

namespace aurostd {
  /// If IPv6 is needed in the future it would be best to use
  /// inet_ntop from arpa/inet.h
  std::string httpGetIP4String(unsigned int ip)
  {
    char ip_str[20];
    unsigned char bytes[4];
    // move the 4 bytes into the lowest byte and mask it with &0xFF
    bytes[0] = ip & 0xFF; // technically (ip >> 0), but it's already the lowest byte
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    //HE20221102 switching from deprecated sprintf to snprintf (eliminates the chance of buffer overflows)
    std::snprintf(ip_str, 20, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
    return (std::string) ip_str;
  }

  xURL httpConstructURL(const std::string &host, const std::string &path="/", const std::string &query="", const unsigned int &port=80){
    xURL url;
    url.scheme = "http";
    url.port = port;
    url.host = host;
    url.path = path;
    url.query = query;
    return url;
  }

  /// @brief split a URL string into its parts
  /// @param url url string
  /// @param strict if set, a full URL needs a scheme (http://, https://), else it is interpreted as path
  /// @return xURL struct
  ///
  /// While a `user` can be extracted by this parser, `user:password` is not supported and should not be added!
  /// https://datatracker.ietf.org/doc/html/rfc3986#section-3.2.1
  xURL httpParseURL(const std::string &url, const bool strict) {

    bool LDEBUG = (false || XHOST.DEBUG || _DEBUG_XHTTP_);

    if (LDEBUG) {
      if (strict) cerr << __AFLOW_FUNC__ << " Parse '" << url << "' strict" << endl;
      else cerr << __AFLOW_FUNC__ << " Parse '" << url << "' lenient" << endl;
    }

    xURL result;
    std::string delimiter = "";
    size_t start = 0;
    size_t location = 0;

    // locate scheme
    delimiter = "://";
    location = url.find(delimiter);
    if (location != std::string::npos) {
      result.scheme = aurostd::tolower(url.substr(start, location - start));
      start = location + delimiter.length();
    }
    if (!strict || !result.scheme.empty()) {
      // locate username
      location = url.find('@', start);
      if (location != std::string::npos) {
        result.user = url.substr(start, location - start);
        start = location + 1;
      }

      // locate host
      location = url.find_first_of("/?", start); //CO20261006 - a query can follow the host directly (host:port?query)
      const bool has_path = (location != std::string::npos); //CO20261006
      if (location != std::string::npos) {
        result.host = url.substr(start, location - start);
        start = (url[location] == '/') ? location + 1 : location;
      } else {
        result.host = url.substr(start, url.length() - start);
        start = url.length();
      }

      // locate port (default to 80 or 443)
      //CO20261006 - only digits make a port; in an AURL (host:AFLOWDATA/...) the colon starts the path,
      //which was read as port 0 and dropped from the path
      std::string aurl_path = "";
      bool port_given = false;
      location = result.host.find(':');
      if (location != std::string::npos) {
        const std::string port = result.host.substr(location + 1);
        port_given = (!port.empty() && port.find_first_not_of("0123456789") == std::string::npos);
        if (port_given) result.port = aurostd::string2utype<uint>(port);
        else aurl_path = port;
        result.host.erase(location);
      }
      if (!port_given) {
        if (result.scheme == "https") result.port = 443;
        else if (result.scheme == "ftp") result.port = 21;
        else if (result.scheme == "sftp") result.port = 22;
        else result.port = 80;
      }

      // locate path
      result.path = "/" + url.substr(start);
      if (!aurl_path.empty()) result.path = "/" + aurl_path + (has_path ? result.path : ""); //CO20261006
    } else {
      result.path = url.substr(start);
      result.port = 0;
    }

    // split query from raw path
    location = result.path.find('?');
    if (location != std::string::npos) {
      result.query = result.path.substr(location);
      result.path = result.path.substr(0, location);
    } else {
      result.query = "";
    }

    if (LDEBUG) {
      cerr << __AFLOW_FUNC__ << " split up URL:" << "\n";
      cerr << "    " << "scheme: " << result.scheme << "\n";
      cerr << "    " << "user: " << result.user << "\n";
      cerr << "    " << "host: " << result.host << "\n";
      cerr << "    " << "port: " << result.port << "\n";
      cerr << "    " << "path: " << result.path << "\n";
      cerr << "    " << "query: " << result.query << std::endl;
    }

    return result;
  }
  
  string httpJoinURL(const xURL& url){  //CO20221209
    return url.scheme+(url.scheme.empty()?"":"://")+url.host+url.path+url.query;  //CO+WJ20231006 - removing strange characters in the path OR the query
    //deal with port later... need a bool to tell if a port was there originally (:)
  }

  /// @brief build a new xURL struct from a redirect location
  /// @param base_url url that initiated the redirection
  /// @param new_location new location - url or path (absolute or relative)
  /// @return constructed xURL struct
  ///
  /// @note https://en.wikipedia.org/wiki/HTTP_location
  xURL httpParseRedirect(const xURL &base_url, const std::string &new_location) {
    xURL new_url = httpParseURL(new_location, true);

    // redirect contains just a path
    // add information from the base_url
    if (new_url.port == 0) {
      new_url.port = base_url.port;
      new_url.scheme = base_url.scheme;
      new_url.host = base_url.host;
      new_url.user = base_url.user;
      // if relative path add new content behind the last '/'
      if (new_url.path[0] != '/') {
        int base_split = base_url.path.find_last_of('/');
        new_url.path = base_url.path.substr(0, base_split) + "/" + new_url.path;
      }
    }
    // some full redirects miss the original port, add it back in
    else if ((new_url.port != base_url.port) &&
             (base_url.port != 80 && base_url.port != 443) &&
             (new_url.host == base_url.host) &&
             (new_url.scheme == base_url.scheme)) {
      new_url.port = base_url.port;
    }
    return new_url;
  }

  /// @brief Parsing the output of a raw http response
  /// @param response raw http response
  /// @param output cleaned message body of the http response
  /// @param header map of header information
  /// @param strict an answer cut off (no last chunk, or a body shorter than Content-Length) gives -1
  /// @return http status code
  ///
  /// @note Message Header definition: https://datatracker.ietf.org/doc/html/rfc7230#section-3.2
  /// @note Chunked Transfer Coding definition: https://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.6.1
  int httpParseResponse(std::string response,
                        std::string &output, std::map <std::string, std::string> &header, bool strict) {

    bool LDEBUG = (false || XHOST.DEBUG || _DEBUG_XHTTP_);
    std::string header_raw = "";
    int status_code = -1;
    std::string status_line = "";
    std::string const delimiter = "\r\n";
    std::string chunk_length_octet = "";
    size_t pos_newline_border = 0;
    size_t split = 0;
    size_t full_request_length = 0;
    size_t offset = delimiter.length();

    // separate headers from message body
    pos_newline_border = response.find(delimiter + delimiter);
    header_raw = response.substr(0, pos_newline_border);
    response.erase(0, pos_newline_border + offset + offset);
    if(pos_newline_border==string::npos){response.clear();} //CO20221209 - this means the response is null

    if(LDEBUG){
      cerr << __AFLOW_FUNC__ << " pos_newline_border=" << pos_newline_border << endl;
      cerr << __AFLOW_FUNC__ << " header_raw=" << endl << "\"" << header_raw << "\"" << endl;
      cerr << __AFLOW_FUNC__ << " response=" << endl << "\"" << response << "\"" << endl;
    }

    // check if Transfer-Encoding is chunked
    bool isChunked = ( header_raw.find("chunked") != std::string::npos );

    // extract first line that contains the status
    pos_newline_border = header_raw.find(delimiter);
    status_line = header_raw.substr(0, pos_newline_border);
    header_raw.erase(0, pos_newline_border + offset);

    // save status code
    split = status_line.find(' ') + 1;
    pos_newline_border = status_line.find(' ', split + 1);
    status_code = aurostd::string2utype<uint>(status_line.substr(split, pos_newline_border - split));

    if(LDEBUG){cerr << __AFLOW_FUNC__ << " status_code=" << status_code << endl;}

    // extract the header data
    std::string key = "";
    std::string item = "";
    std::string header_line = "";
    while (!header_raw.empty()) {
      pos_newline_border = header_raw.find(delimiter);
      if (pos_newline_border != std::string::npos) {
        header_line = header_raw.substr(0, pos_newline_border);
        header_raw.erase(0, pos_newline_border + offset);
      } else { //CO20261002 - the last header line has no line break left here; it used to be dropped (a Location or Content-Length sent last was lost)
        header_line = header_raw;
        header_raw.clear();
      }
      split = header_line.find(':');
      if (split == std::string::npos) continue;
      // Header field name are case-insensitive
      key = aurostd::tolower(header_line.substr(0, split));
      item = aurostd::RemoveWhiteSpacesFromTheFrontAndBack(header_line.substr(split + 1));

      // Headers that appear multiple times are equal to a list separated by comma
      if (header.find(key) == header.end()) {
        header.emplace(key, item);
      } else {
        header[key] += "," + item;
      }
    }

    // in HTTP1.1 data is mostly sent in chunks
    if (isChunked) {
      output.clear(); //WJ20231025 Clear output var (otherwise response gets added twice later)
      size_t chuck_length = 0;
      while (!response.empty() && chunk_length_octet != "0") {
        pos_newline_border = response.find(delimiter);
        chunk_length_octet = response.substr(0, pos_newline_border);
        response.erase(0, pos_newline_border + offset);
        if (chunk_length_octet != "0") {
          chuck_length = aurostd::string2utype<uint>(chunk_length_octet, 16);
          full_request_length += chuck_length;
          // sanity check one - every chunk should end with \r\n
          if (response.substr(chuck_length, offset) != delimiter) {
            if (LDEBUG) cerr << __AFLOW_FUNC__ << " Chunk did not end in `\\r\\n`." << endl;
            return -1;
          }
          output += response.substr(0, chuck_length);
          response.erase(0, chuck_length + offset);
        }
      }
      // sanity check two - the constructed string should have the length reported by the server
      if (full_request_length != output.length()) {
        if (LDEBUG) cerr << __AFLOW_FUNC__ << " Final string size is different from the chunk size sum." << endl;
        return -1;
      }
      //CO20261002 - strict: an answer cut off between two chunks has no last (zero-length) chunk
      if (strict && chunk_length_octet != "0") {
        if (LDEBUG) cerr << __AFLOW_FUNC__ << " Chunked answer without its last chunk (cut off)." << endl;
        return -1;
      }
      return status_code;
    } else {
      output = response;
      //CO20261002 - strict: the body must have the length the header announced
      if (strict && header.find("content-length") != header.end() &&
          aurostd::string2utype<size_t>(header["content-length"]) != output.size()) {
        if (LDEBUG) cerr << __AFLOW_FUNC__ << " Body length differs from Content-Length (cut off)." << endl;
        return -1;
      }
      return status_code;
    }
  }

  /// @brief Fully percent encode a string
  /// @param work_str sting to escape
  /// @return escaped string
  /// @note https://www.rfc-editor.org/rfc/rfc3986#section-2.1
  /// @note just leave unreserved characters (ALPHA / DIGIT / "-" / "." / "_" / "~")
  string httpPercentEncodingFull(string work_str){

    bool LDEBUG = (false || XHOST.DEBUG || _DEBUG_XHTTP_);

    const char *allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                          "abcdefghijklmnopqrstuvwxyz"
                          "0123456789"
                          "-_.~";

    size_t pos=0;
    int to_replace=0;
    std::stringstream output;
    if (LDEBUG) cerr << __AFLOW_FUNC__ << " Escaping '" << work_str << "'" << std::endl;

    while (!work_str.empty()){
      pos = std::strspn(work_str.c_str(), allowed);
      if (pos == work_str.size()) {output << work_str; break;} //CO20261004 - the rest is unreserved; work_str[pos] is the terminating NUL, which was written as "%00"
      to_replace = work_str[pos];
      if (to_replace<0) to_replace+=256;
      output << work_str.substr(0, pos) << "%" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << to_replace;
      if (LDEBUG) cerr << " Match '" << work_str[pos] << "' (%" << std::uppercase << std::hex << std::setfill('0') << to_replace << std::dec << ")" << std::endl;
      work_str.erase(0,pos+1);
    }
    return output.str();
  }


  /// @brief Get a raw http response
  /// @param hostname hostname to establish a connection to
  /// @param query query to run on the server (like "/API/aflux/?nspecies(4),paging(1,300)")
  /// @param response raw http response
  /// @param success was the request successful
  ///
  /// This function is primitive and is designed to communicate with AFLOW's API servers.
  bool httpGetResponse(const xURL & url, std::string &response) {

    bool LDEBUG = (false || XHOST.DEBUG || _DEBUG_XHTTP_);

    response.clear();

    char buffer[BUFSIZ];
    char request_template[] = "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: s4e/%s (https://s4e.ai)\r\n\r\n";

    struct protoent *protocol_entry;
    struct hostent *host_entry;

    in_addr_t ip_address;
    struct sockaddr_in socket_entry{};
    std::string ip_address_str; // prepared for detailed debugging - see below
    char *request;
    int request_len;
    int socket_file_descriptor;
    ssize_t nbytes_total, nbytes_last;
    unsigned long loaded_bytes = 0;

    // build request and save its length
    request_len = asprintf(&request, request_template, (url.path + url.query).c_str(), url.host.c_str(), AFLOW_VERSION);  //CO20231228 - valgrind reports memory leak here because there is no free(request)

    if(LDEBUG){cerr << __AFLOW_FUNC__ << " request:" << endl << "---request begin---" << endl << request << endl << "---request end---" << endl;}

    // get the TCP protocol entry
    protocol_entry = getprotobyname("tcp");
    if (protocol_entry == nullptr) {
      if (LDEBUG) cerr << __AFLOW_FUNC__ << " Failed to get TCP protocol entry!" << endl;
      free(request);  //CO20231228 - freeing memory from asprintf() above
      return false;
    }

    // open the socket
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, protocol_entry->p_proto);
    if (socket_file_descriptor == -1) {
      if (LDEBUG) cerr << __AFLOW_FUNC__ << " Failed to open socket!" << endl;
      free(request);  //CO20231228 - freeing memory from asprintf() above
      return false;
    }

    // find the IP of the host
    host_entry = gethostbyname(url.host.c_str());
    if (host_entry == nullptr) {
      if (LDEBUG) cerr << __AFLOW_FUNC__ << " Failed to find information on '" << url.host << "'!" << endl;
      free(request);  //CO20231228 - freeing memory from asprintf() above
      return false;
    }

    ip_address = (*(struct in_addr *) *(host_entry->h_addr_list)).s_addr;
    ip_address_str = httpGetIP4String(ip_address);
    if(LDEBUG){cerr << __AFLOW_FUNC__ << " ip_address_str=" << ip_address_str << endl;}

    socket_entry.sin_addr.s_addr = ip_address;
    socket_entry.sin_family = AF_INET;
    if (url.port > 65535) {
      if (LDEBUG) cerr << __AFLOW_FUNC__ << " Failed to connect to " << url.host << " as port is out of range (" << url.port << ">65535)" << endl;
      free(request);  //CO20231228 - freeing memory from asprintf() above
      return false;
    }
    socket_entry.sin_port = htons((short) url.port);

    // start connection
    if (connect(socket_file_descriptor, (struct sockaddr *) &socket_entry, sizeof(socket_entry)) == -1) {
      if (LDEBUG) cerr << __AFLOW_FUNC__ << " Failed to connect to " << url.host << " (" << ip_address_str << ":" << url.port << ")" << endl;
      free(request);  //CO20231228 - freeing memory from asprintf() above
      return false;
    }
    if (LDEBUG)
      cerr << __AFLOW_FUNC__ << " Connected to " << url.host << " (" << ip_address_str << ":" << url.port << ")" << endl;

    // send the request
    nbytes_total = 0;
    while (nbytes_total < request_len) {
      nbytes_last = write(socket_file_descriptor, request + nbytes_total, request_len - nbytes_total);
      if (nbytes_last == -1) {
        if (LDEBUG) cerr << __AFLOW_FUNC__ << " Failed to send the request to " << url.host << "( " << ip_address_str << ":" << url.port << ")" << endl;
        free(request);  //CO20231228 - freeing memory from asprintf() above
        return false;
      }
      nbytes_total += nbytes_last;
    }
    if (LDEBUG) cerr << __AFLOW_FUNC__ << " Request written (" << nbytes_total << " bytes)" << endl;

    free(request);  //CO20231228 - freeing memory from asprintf() above, should be completely safe here

    // read the response
    if (LDEBUG) cerr << __AFLOW_FUNC__ << " Start reading response:";
    while ((nbytes_total = read(socket_file_descriptor, buffer, BUFSIZ)) > 0) {
      if (LDEBUG) {
        loaded_bytes += nbytes_total;
        cerr << "    read " << loaded_bytes << " bytes \r";
      }
      response += std::string(buffer, nbytes_total);
    }
    if (LDEBUG){
      cerr << __AFLOW_FUNC__ << " Finished reading response (" << loaded_bytes << " bytes)" << endl;
      cerr << __AFLOW_FUNC__ << " response:" << endl << "---response begin---" << endl << response << endl << "---response end---" << endl;
    }

    if (nbytes_total == -1) {
      if (LDEBUG) cerr << __AFLOW_FUNC__ << " Failed to read response from " << url.host << "( " << ip_address_str << ":" << url.port << ")" << endl;
      response.clear();
      return false;
    }

    // close the socket
    close(socket_file_descriptor);

    //PB20260803 - a zero-byte response is not a success: some TLS-only endpoints (a plaintext
    //request to an https port) simply close the connection. Reporting
    //success here hides the failure from httpGet() and suppresses the curl fallback.
    //NB: placed AFTER close() above, so this returns without double-closing the descriptor.
    if (response.empty()) {
      if (LDEBUG) cerr << __AFLOW_FUNC__ << " Empty response from " << url.host << " (" << ip_address_str << ":" << url.port << ")" << endl;
      return false;
    }

    return true;
  }

  // ***************************************************************************
  // Function foundFirewall
  // ***************************************************************************
  //CO20231211 - some institutions do not allow http queries, must be https
  //check response to see if it was encountered
  bool foundFirewall(const string& response_url){
    if(response_url.find("blocked due to malicious activity")!=string::npos){return true;}
    if(response_url.find("SSL-enabled server port")!=string::npos){return true;}  //apache mod_ssl wording
    if(response_url.find("plain HTTP request was sent to HTTPS port")!=string::npos){return true;}  //PB20260803 - cloudflare/nginx wording
    return false;
  }

  //CO20261002 START - s4e.ai over https only, with the CHAOS API key when there is one
  /// @brief the host name of s4e.ai or one of its subdomains in its normal form (lower case, no final dot)
  /// @param host host name; a scheme (`https://`), a port and a path are ignored, a user part is refused
  /// @return the normal form, "" when the host is not s4e.ai
  /// @note the host must be letters, digits, hyphens and dots only: "evil.example?.s4e.ai" ends in
  ///       ".s4e.ai" but curl would connect to evil.example (the rest is a query)
  /// @note one check for httpIsS4EHost() and httpS4EUrl() //CO20261004
  static std::string httpS4EHostName(const std::string &host) {
    const xURL url = httpParseURL(host); //CO20261004 - httpParseURL splits off scheme, user, port and path (they were stripped by hand)
    if (!url.user.empty()) return ""; // "user@s4e.ai" stays refused
    std::string h = aurostd::tolower(url.host);
    if (!h.empty() && h.back() == '.') h.pop_back(); // "s4e.ai." is the same host
    static const std::regex hostname("^[a-z0-9-]+(\\.[a-z0-9-]+)*$");
    if (!std::regex_match(h, hostname)) return "";
    const std::string base = "s4e.ai";
    if (h == base) return h;
    if (h.size() > base.size() + 1 && h.compare(h.size() - base.size() - 1, base.size() + 1, "." + base) == 0) return h;
    return "";
  }

  /// @brief is the host s4e.ai or one of its subdomains
  /// @param host host name; a scheme (`https://`), a port and a path are ignored
  bool httpIsS4EHost(const std::string &host) {
    return !httpS4EHostName(host).empty(); //CO20261004
  }

  //CO20261006 START - s4e.ai has no AFLOWLIB folders
  /// @brief does s4e.ai serve this path
  /// @param url a URL on s4e.ai
  /// @return false for a path under /AFLOWDATA: s4e.ai has no AFLOWLIB calculation folders (their
  ///         listings ?files and ?aflowlib_entries, ?format=text, the files of each run); CHAOS
  ///         records come from the CHAOS API (/API/chaos/)
  bool httpS4EServesPath(const xURL &url) {
    vector<string> segments;
    aurostd::string2tokens(url.path, segments, "/");
    return segments.empty() || aurostd::toupper(segments[0]) != "AFLOWDATA";
  }

  /// @brief refuse a request for a path s4e.ai does not serve, before anything is sent
  /// @param url URL of the request
  /// @return true when the request is refused; the reason is printed once per run
  bool httpS4EPathRefused(const xURL &url) {
    if (!httpIsS4EHost(url.host) || httpS4EServesPath(url)) return false;
    static bool told = false;
    if (!told) cerr << "ERROR - " << __AFLOW_FUNC__ << " not sent: s4e.ai has no calculation folders (" << url.path << "); CHAOS records come from the CHAOS API (https://s4e.ai/API/chaos/)" << endl;
    told = true;
    return true;
  }
  //CO20261006 STOP

  /// @brief the file that holds the CHAOS API key when CHAOS_API_KEY is not set, as users write it
  /// @note outside ~/.aflow: the key belongs to s4e.ai, not to AFLOW (CO20261004)
  std::string httpChaosApiKeyFileName() {
    return "~/.s4e/chaos_api_key";
  }

  /// @brief the key file in a home folder
  /// @param home the home folder ($HOME)
  /// @return httpChaosApiKeyFileName() with ~ replaced by home, "" when home is empty
  std::string httpChaosApiKeyFile(const std::string &home) {
    if (home.empty()) return "";
    std::string file = httpChaosApiKeyFileName();
    aurostd::StringSubst(file, "~/", home + "/");
    return file;
  }

  /// @brief the CHAOS API key (a LOOP API key): $CHAOS_API_KEY, else the first line of key_file
  /// @param key_file file holding the key; empty means httpChaosApiKeyFile($HOME)
  /// @return the key without surrounding white space, "" when there is none
  std::string httpChaosApiKey(const std::string &key_file) {
    const std::string file = key_file.empty() ? httpChaosApiKeyFile(XHOST.home) : key_file; //CO20261004 - XHOST.home, was std::getenv("HOME") with a null check
    return httpChaosApiKeyResolve(aurostd::getenv2string("CHAOS_API_KEY"), file); //CO20261004 - getenv2string, was std::getenv with a null check
  }

  /// @brief the key from an environment value, else from the first line of a file
  /// @param env_value value of CHAOS_API_KEY ("" when unset)
  /// @param key_file file holding the key ("" for none)
  /// @return the key without surrounding white space, "" when there is none
  /// @note separate from httpChaosApiKey() so the unit test does not have to change the environment
  std::string httpChaosApiKeyResolve(const std::string &env_value, const std::string &key_file) {
    const std::string key = aurostd::RemoveWhiteSpacesFromTheFrontAndBack(env_value);
    if (!key.empty()) return key;
    //CO20261004 - FileExist and file2vectorstring, were ifstream and getline; consecutive=true keeps an
    //empty first line as the first line (no key), as getline did
    if (!aurostd::FileExist(key_file)) return "";
    vector<string> lines;
    aurostd::file2vectorstring(key_file, lines, true);
    if (lines.empty()) return "";
    return aurostd::RemoveWhiteSpacesFromTheFrontAndBack(lines[0]);
  }

  /// @brief can the key be sent as one header line: 1 to 512 characters from [A-Za-z0-9._~+/=-]
  bool httpChaosApiKeyUsable(const std::string &key) {
    if (key.empty() || key.size() > 512) return false;
    const std::string allowed = "._~+/=-";
    for (size_t i = 0; i < key.size(); i++) {
      const unsigned char c = key[i];
      if (!std::isalnum(c) && allowed.find((char) c) == std::string::npos) return false;
    }
    return true;
  }

  /// @brief the https URL for an s4e.ai request, built from checked parts
  /// @param url parsed URL
  /// @return "https://host/path?query", or "" when the host is not s4e.ai, the URL has a user part,
  ///         the path does not start with "/" (a path "@evil.example/" would make the user part
  ///         "s4e.ai"), or path or query hold white space or control characters
  std::string httpS4EUrl(const xURL &url) {
    xURL https_url = url;
    https_url.host = httpS4EHostName(url.host); //CO20261004 - normal form from the same check as httpIsS4EHost(), was lower-cased and stripped again here
    if (https_url.host.empty() || !url.user.empty()) return "";
    if (url.path.empty() || url.path[0] != '/') return "";
    if (!url.query.empty() && url.query[0] != '?') return "";
    const std::string rest = url.path + url.query;
    for (size_t i = 0; i < rest.size(); i++) {
      const unsigned char c = rest[i];
      if (c <= 0x20 || c == 0x7f || c == '\\') return "";
    }
    https_url.scheme = "https";
    return httpJoinURL(https_url); //CO20261004 - httpJoinURL, was "https://"+host+path+query by hand
  }

  /// @brief the curl arguments for an https GET
  /// @param url full URL (from httpS4EUrl())
  /// @param with_key curl reads the key header from its standard input (-H @-), so the key is
  ///        never on a command line (ps shows those to every account) nor in a file
  /// @note --proto =https: curl refuses anything but https; no -v (it would print the request
  ///       headers); no -L (httpGet() follows redirects itself and checks the host again); 900 s at
  ///       most per request (the CHAOS API stops a query after 120 s)
  std::vector<std::string> httpCUrlArgs(const std::string &url, bool with_key) {
    // -q first: no ~/.curlrc (a "location" or "trace-ascii" line there would follow redirects with
    // the key to another host, or write the request to a file)
    std::vector<std::string> args = {"curl", "-q", "-i", "-s", "--raw", "--proto", "=https", "--suppress-connect-headers", "--connect-timeout", "30", "--max-time", "900"};
    if (with_key) {
      args.push_back("-H");
      args.push_back("@-");
    }
    args.push_back(url);
    return args;
  }

  /// @brief run curl directly (no shell), with input on its standard input and its output read back
  /// @param args curl and its arguments (args[0] is replaced by curl's path)
  /// @param input text for curl's standard input ("" for none)
  /// @param output what curl wrote to its standard output, byte for byte
  /// @param exit_code curl's exit status (-1 when it did not run or did not exit normally)
  /// @return true when curl exited with 0
  /// @note no temporary file: execute2string() would pass the answer through a file in XHOST.tmpfs
  ///       that other accounts can read
  bool httpCUrlRun(const std::vector<std::string> &args, const std::string &input, std::string &output, int &exit_code) {
    output.clear();
    exit_code = -1;
    // an absolute path only (IsCommandAvailable() falls back to "./curl", a file anyone could put in the working folder)
    static const std::string curl_path = [](){ std::string path; return (aurostd::IsCommandAvailable("curl", path) && !path.empty() && path[0] == '/') ? path : std::string(""); }();
    if (curl_path.empty() || args.empty()) return false;
    // close-on-exec, so curl processes started by other threads do not keep these pipes open
    int in[2], out[2];
#if defined(__linux__)
    if (pipe2(in, O_CLOEXEC) != 0) return false;
    if (pipe2(out, O_CLOEXEC) != 0) {close(in[0]); close(in[1]); return false;}
#else
    if (pipe(in) != 0) return false;
    if (pipe(out) != 0) {close(in[0]); close(in[1]); return false;}
    for (int fd : {in[0], in[1], out[0], out[1]}) fcntl(fd, F_SETFD, FD_CLOEXEC); // not atomic here (no pipe2)
#endif
    // the input (one header line) is far smaller than a pipe's buffer: written before curl starts,
    // it cannot block, and no SIGPIPE can arrive (the read end is still ours)
    bool ok = true;
    if (!input.empty()) ok = (write(in[1], input.data(), input.size()) == (ssize_t) input.size());
    close(in[1]);
    if (!ok) {close(in[0]); close(out[0]); close(out[1]); return false;}
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, in[0], 0);
    posix_spawn_file_actions_adddup2(&actions, out[1], 1);
    posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_WRONLY, 0);
    std::vector<std::string> argv_strings = args;
    argv_strings[0] = curl_path;
    std::vector<char*> argv;
    for (size_t i = 0; i < argv_strings.size(); i++) argv.push_back(&argv_strings[i][0]);
    argv.push_back(nullptr);
    pid_t pid = 0;
    const int spawned = posix_spawn(&pid, curl_path.c_str(), &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(in[0]);
    close(out[1]);
    if (spawned != 0) {close(out[0]); return false;}
    char buffer[65536];
    while (true) {
      const ssize_t n = read(out[0], buffer, sizeof(buffer));
      if (n > 0) {output.append(buffer, n); continue;}
      if (n < 0 && errno == EINTR) continue;
      break;
    }
    close(out[0]);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
      if (errno != EINTR) return false;
    }
    exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return exit_code == 0;
  }

  /// @brief GET an s4e.ai URL over https with curl, sending the CHAOS API key when there is one
  /// @param url URL (host s4e.ai)
  /// @param response raw response (status line, header, body)
  /// @return true when curl answered completely (exit 0)
  static bool httpGetS4E(const xURL &url, std::string &response) {
    bool LDEBUG = (false || XHOST.DEBUG || _DEBUG_XHTTP_);
    response.clear();
    const std::string full_url = httpS4EUrl(url);
    if (full_url.empty()) {
      cerr << "ERROR - " << __AFLOW_FUNC__ << " refused to send a request to a malformed s4e.ai address" << endl;
      return false;
    }
    std::string key = httpChaosApiKey();
    if (!key.empty() && !httpChaosApiKeyUsable(key)) {
      static bool warned = false;
      if (!warned) cerr << "WARNING - " << __AFLOW_FUNC__ << " CHAOS API key ignored: it has characters a key does not have" << endl;
      warned = true;
      key.clear();
    }
    if (LDEBUG) cerr << __AFLOW_FUNC__ << " GET " << full_url << (key.empty() ? " without" : " with") << " a CHAOS API key" << endl;
    int exit_code = -1;
    const bool ok = httpCUrlRun(httpCUrlArgs(full_url, !key.empty()), key.empty() ? std::string("") : "X-API-Key: " + key + "\n", response, exit_code);
    if (!ok && LDEBUG) cerr << __AFLOW_FUNC__ << " curl exit status " << exit_code << " (an incomplete answer counts as no answer)" << endl;
    if (!ok) response.clear();
    return ok;
  }

  /// @brief what to tell the user when s4e.ai refuses a request
  /// @param host host of the request
  /// @param status HTTP status
  /// @return a hint, "" when there is nothing to add
  std::string httpChaosApiKeyHint(const std::string &host, int status) {
    if (!httpIsS4EHost(host)) return "";
    switch (status) {
      case 401:
        if (httpChaosApiKey().empty()) return "no CHAOS API key: set CHAOS_API_KEY or put a LOOP API key in " + httpChaosApiKeyFileName() + " (https://s4e.ai/chaos)";
        return "the CHAOS API key (CHAOS_API_KEY or " + httpChaosApiKeyFileName() + ") was not accepted";
      case 403: return "this account is not approved for CHAOS yet, or its profile is incomplete (https://s4e.ai/chaos)";
      case 429: return "over the CHAOS API request limits; try again in a minute";
      case 400: return "the CHAOS API could not read the query: name each field with parentheses (field(*) for any value) and without a $ in front, and use paging(n,k), since paging(0) is refused (see ?help(queries))"; //CO20261004 - the rules the API enforces
      default: return "";
    }
  }
  //CO20261002 STOP

  /// @brief get the content of a URL
  /// @param url copied so that a redirect can change it
  /// @param output
  /// @param status_code
  /// @param header
  /// @note this function is the 'private' implementation of httpGet()
  void httpGet(xURL url,
               std::string &output, int &status_code, std::map <std::string, std::string> &header) {

    bool LDEBUG = (false || XHOST.DEBUG || _DEBUG_XHTTP_);

    if (LDEBUG) cerr << __AFLOW_FUNC__ << " GET '" << url.path << "' from " << url.host << endl;

    output.clear();
    header.clear();
    int redirect_codes[] = {301, 302, 307, 308};
    std::string response = "";
    const int max_number_of_tries = 3;
    int number_of_tries = 0;
    bool success = false;

    while (status_code != 200 && number_of_tries < max_number_of_tries) {
      // read server response
      //PB20260803 - httpGetResponse() is a plaintext socket with no TLS support, so an https/443
      //URL can never succeed there. Skip it and go straight to the curl path, which handles
      //certificates. Without this, a TLS-terminating proxy answers the cleartext request with a
      //400 whose body is then parsed as if it were data.
      //CO20261002 - s4e.ai only over https (curl), with the CHAOS API key when there is one; a
      //redirect can change the host, so this is decided on every round
      const bool s4e = httpIsS4EHost(url.host);
      if (httpS4EPathRefused(url)) {status_code = -1; output.clear(); header.clear(); return;} //CO20261006 - nothing sent, no retries; after a redirect no earlier answer is kept
      if (s4e) {
        url.scheme = "https";
        if (url.port == 80 || url.port == 0) url.port = 443;
      }
      const bool needs_tls = (url.scheme == "https" || url.port == 443);
      if (s4e) {
        success = httpGetS4E(url, response);
      } else if (needs_tls) {
        response.clear();
        success = false;
      } else {
        success = httpGetResponse(url, response);
      }
      //CO20240527 START
      if(!s4e && (!success||foundFirewall(response))){ //CO20221209 - curl both leverages certificates and gives raw output
        success=aurostd::url2stringCUrl(aurostd::httpJoinURL(url),response,LDEBUG);
        if(LDEBUG){cerr << __AFLOW_FUNC__ << " response from CUrl:" << endl << "---response begin---" << endl << response << endl << "---response end---" << endl;}
        if(!success){
          if (LDEBUG) cerr << __AFLOW_FUNC__ << " Failed to get response from CUrl" << endl;
        }
      }
      //CO20240527 STOP
      number_of_tries += 1;
      if (success) {
        // parse the response to extract header and status code
        //CO20261002 - each answer is parsed on its own: headers of an earlier answer (a 429 or a redirect)
        //were joined to the next one's ("content-length: 61,5000000"), and s4e.ai answers are checked for completeness
        header.clear();
        output.clear();
        status_code = httpParseResponse(response, output, header, s4e);
        if (LDEBUG) cerr << __AFLOW_FUNC__ << " Received status code " << status_code << endl;
        if (LDEBUG && status_code!=-1) {
          cerr << __AFLOW_FUNC__ << " Header content: " << endl;
          for (std::map<std::string, std::string>::iterator it = header.begin(); it != header.end(); it++)
            cerr << "    " << it->first << ": " << it->second << std::endl;
        }
        //CO20261002 START - wait before asking again when the server is busy; a refusal does not change on retry
        if ((status_code == 429 || status_code == 503) && number_of_tries < max_number_of_tries) {
          int wait = 10;
          if (header.find("retry-after") != header.end()) wait = aurostd::string2utype<int>(header["retry-after"]);
          if (wait < 1) wait = 1;
          if (wait > 60) wait = 60;
          if (LDEBUG) cerr << __AFLOW_FUNC__ << " Status " << status_code << ", retry in " << wait << "s (" << number_of_tries << " of " << max_number_of_tries << ")" << endl;
          aurostd::Sleep(wait);
          continue;
        }
        if (status_code >= 400 && status_code < 500 && status_code != 408 && status_code != 429) return;
        //CO20261002 STOP
        if (std::find(std::begin(redirect_codes), std::end(redirect_codes), status_code) != std::end(redirect_codes)) {
          if (header.find("location")!=header.end()) {
            url = httpParseRedirect(url, header["location"]);
            if (LDEBUG) cerr << __AFLOW_FUNC__ << " Redirected to '" << header["location"] << "'" << endl;
          }
          else {
            if (LDEBUG) cerr << __AFLOW_FUNC__ << " GET request failed due to a redirect without location." << endl;
            return;
          }
        }
      } else if (number_of_tries < max_number_of_tries) {
        if (LDEBUG) cerr << __AFLOW_FUNC__ << " Retry failed GET in 5s (" << number_of_tries << " of " << max_number_of_tries << ")" << endl;
        aurostd::Sleep(5);
        continue;
      } else {
        if (LDEBUG) cerr << __AFLOW_FUNC__ << " GET request failed after " << number_of_tries << " tries." << endl;
        return;
      }
    }
    if (LDEBUG) {
      if (number_of_tries > 1) cerr << __AFLOW_FUNC__ << " GET request done after " << number_of_tries << " tries." << endl;
      else cerr << __AFLOW_FUNC__ << " GET request done" << endl;
    }
  }

  // Start of external callable functions


  /// @brief Retrieve data from an url string
  /// @param url_str content url
  /// @return HTTP status code (-1 on failure)
  int httpGetStatus(const std::string &url_str) {
    xURL url = httpParseURL(url_str);
    std::string output = "";
    int status_code = -1;
    std::map <std::string, std::string> header;
    httpGet(url, output, status_code, header);
    return status_code;
  }

  /// @brief Retrieve data from an url string
  /// @param url_str content url
  /// @param output message body
  /// @return HTTP status code (-1 on failure)
  int httpGetStatus(const std::string &url_str, std::string &output) {
    xURL url = httpParseURL(url_str);
    int status_code = -1;
    std::map <std::string, std::string> header;
    httpGet(url, output, status_code, header);
    return status_code;
  }

  /// @brief Retrieve data from an url string
  /// @param url_str content url
  /// @param output message body
  /// @param header response header
  /// @return HTTP status code (-1 on failure)
  int httpGetStatus(const std::string &url_str, std::string &output, std::map <std::string, std::string> &header) {
    xURL url = httpParseURL(url_str);
    int status_code = -1;
    httpGet(url, output, status_code, header);
    return status_code;
  }

  /// @brief Retrieve data from an url string
  /// @param host server name or IP to contact
  /// @param query GET query
  /// @param output message body
  /// @return HTTP status code (-1 on failure)
  int httpGetStatus(const std::string &host, const std::string &path, const std::string &query, std::string &output) {
    xURL url = httpConstructURL(host, path, query);
    int status_code = -1;
    std::map <std::string, std::string> header;
    httpGet(url, output, status_code, header);
    return status_code;
  }

  /// @brief Retrieve data from an url string
  /// @param host server name or IP to contact
  /// @param query GET query
  /// @param output message body
  /// @param header response header
  /// @return HTTP status code (-1 on failure)
  int httpGetStatus(const std::string &host, const std::string &path, const std::string &query, std::string &output, std::map <std::string, std::string> &header) {
    xURL url = httpConstructURL(host, path, query);
    int status_code = -1;
    httpGet(url, output, status_code, header);
    return status_code;
  }

  /// @brief Retrieve data from an url string
  /// @param url_str content url
  /// @return message body
  std::string httpGet(const std::string &url_str) {
    xURL url = httpParseURL(url_str);
    std::string output = "";
    int status_code = -1;
    std::map <std::string, std::string> header;

    httpGet(url, output, status_code, header);
    return output;
  }

  /// @brief Retrieve data from an url string
  /// @param url_str content url
  /// @param status_code HTTP status code (-1 on failure)
  /// @return message body
  std::string httpGet(const std::string &url_str, int &status_code) {
    xURL url = httpParseURL(url_str);
    std::string output = "";
    std::map <std::string, std::string> header;

    status_code = -1;
    httpGet(url, output, status_code, header);
    return output;
  }

  /// @brief Retrieve data from an url string
  /// @param url_str content url
  /// @param status_code HTTP status code (-1 on failure)
  /// @param header response header
  /// @return message body
  std::string httpGet(const std::string &url_str, int &status_code, std::map <std::string, std::string> &header) {
    xURL url = httpParseURL(url_str);

    std::string output="";
    status_code = -1;
    httpGet(url, output, status_code, header);
    return output;
  }

  /// @brief Retrieve data for host + query
  /// @param host server name or IP to contact
  /// @param query GET query
  /// @return message body
  std::string httpGet(const std::string &host, const std::string &path, const std::string &query) {
    xURL url = httpConstructURL(host, path, query);
    std::string output="";
    int status_code = -1;
    std::map <std::string, std::string> header;

    httpGet(url, output, status_code, header);
    return output;
  }

  /// @brief Retrieve data for host + query
  /// @param host server name or IP to contact
  /// @param query GET query
  /// @param status_code HTTP status code (-1 on failure)
  /// @return message body
  std::string httpGet(const std::string &host, const std::string &path, const std::string &query, int &status_code) {
    xURL url = httpConstructURL(host, path, query);
    std::string output="";
    std::map <std::string, std::string> header;

    status_code = -1;
    httpGet(url, output, status_code, header);
    return output;
  }

  /// @brief Retrieve data for host + query
  /// @param host server name or IP to contact
  /// @param query GET query
  /// @param status_code HTTP status code (-1 on failure)
  /// @param header response header
  /// @return message body
  std::string httpGet(const std::string &host, const std::string &path, const std::string &query, int &status_code, std::map <std::string, std::string> &header) {
    xURL url = httpConstructURL(host, path, query);
    std::string output="";

    status_code = -1;
    httpGet(url, output, status_code, header);
    return output;
  }

}

#endif  // _AUROSTD_XHTTP_CPP_
