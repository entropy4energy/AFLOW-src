// ***************************************************************************
// *                                                                         *
// *           Aflow STEFANO CURTAROLO - Duke University 2003-2022           *
// *                                                                         *
// ***************************************************************************
// Written by Frisco Rose in 2018
// Complete rewrite by Hagen Eckert in 2022
// hagen.eckert@duke.edu

#ifndef AFLOW_SRC_AUROSTD_XHTTP_H
#define AFLOW_SRC_AUROSTD_XHTTP_H

namespace aurostd{
  struct xURL {
    std::string scheme;
    std::string user;
    std::string host;
    unsigned int port;
    std::string path;
    std::string query;
  };

  int httpGetStatus(const std::string &url);
  int httpGetStatus(const std::string &url, std::string &output);
  int httpGetStatus(const std::string &url, std::string &output, std::map<std::string, std::string> &header);

  int httpGetStatus(const std::string &host, const std::string &path, const std::string &query, std::string &output);
  int httpGetStatus(const std::string &host, const std::string &path, const std::string &query, std::string &output, std::map<std::string, std::string> &header);

  bool foundFirewall(const string& response_url); //CO20231211

  std::string httpGet(const std::string &url);
  std::string httpGet(const std::string &url, int &status_code);
  std::string httpGet(const std::string &url, int &status_code, std::map<std::string, std::string> &header);

  std::string httpGet(const std::string &host, const std::string &path, const std::string &query);
  std::string httpGet(const std::string &host, const std::string &path, const std::string &query, int &status_code);
  std::string httpGet(const std::string &host, const std::string &path, const std::string &query, int &status_code, std::map<std::string, std::string> &header);

  std::string httpPercentEncodingFull(std::string work_str);

  xURL httpParseURL(const std::string &url, const bool strict = false);
  string httpJoinURL(const xURL& url);  //CO20221209 - get back URL //CO20261004 - declared again, used by httpS4EUrl() and the unit test chaos_key

  //CO20261002 - s4e.ai over https only, with the CHAOS API key ($CHAOS_API_KEY or ~/.s4e/chaos_api_key)
  bool httpIsS4EHost(const std::string &host);
  bool httpS4EServesPath(const xURL &url); //CO20261006 - s4e.ai has no AFLOWLIB folders (/AFLOWDATA/...)
  bool httpS4EPathRefused(const xURL &url); //CO20261006
  std::string httpS4EUrl(const xURL &url);
  std::string httpChaosApiKeyFileName();
  std::string httpChaosApiKeyFile(const std::string &home);
  std::string httpChaosApiKey(const std::string &key_file = "");
  std::string httpChaosApiKeyResolve(const std::string &env_value, const std::string &key_file);
  bool httpChaosApiKeyUsable(const std::string &key);
  std::vector<std::string> httpCUrlArgs(const std::string &url, bool with_key);
  bool httpCUrlRun(const std::vector<std::string> &args, const std::string &input, std::string &output, int &exit_code);
  std::string httpChaosApiKeyHint(const std::string &host, int status);
  int httpParseResponse(std::string response, std::string &output, std::map<std::string, std::string> &header, bool strict = false);
}

#endif //AFLOW_SRC_AUROSTD_XHTTP_H
