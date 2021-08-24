#ifndef INCLUDE_CCAPI_CPP_CCAPI_URL_H_
#define INCLUDE_CCAPI_CPP_CCAPI_URL_H_
#include <regex>
#include <string>
#include "ccapi_cpp/ccapi_macro.h"
#include "ccapi_cpp/ccapi_util_private.h"
namespace ccapi {
class Url CCAPI_FINAL {
 public:
  explicit Url(std::string urlStr) {
    std::regex ex("^(.*:)//([A-Za-z0-9\\-\\.]+)(:[0-9]+)?(.*)$");
    std::cmatch what;
    if (std::regex_match(urlStr.c_str(), what, ex)) {
      this->protocol = std::string(what[1].first, what[1].second);
      this->host = std::string(what[2].first, what[2].second);
      this->port = std::string(what[3].first, what[3].second);
      this->target = std::string(what[4].first, what[4].second);
    }
  }
  Url(std::string protocol, std::string host, std::string port, std::string target) : protocol(protocol), host(host), port(port), target(target) {}
  std::string toString() const {
    std::string output = "Url [protocol = " + protocol + ", host = " + host + ", port = " + port + ", target = " + target + "]";
    return output;
  }
  static std::string urlEncode(const std::string &value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
      std::string::value_type c = (*i);
      // Keep alphanumeric and other accepted characters intact
      if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
        escaped << c;
        continue;
      }
      // Any other characters are percent-encoded
      escaped << std::uppercase;
      escaped << '%' << std::setw(2) << int((unsigned char)c);
      escaped << std::nouppercase;
    }
    return escaped.str();
  }
  static std::string urlDecode(const std::string &value) {
    std::string ret;
    char ch;
    int i, ii;
    for (i = 0; i < value.length(); i++) {
      if (static_cast<int>(value[i]) == 37) {
        sscanf(value.substr(i + 1, 2).c_str(), "%x", &ii);
        ch = static_cast<char>(ii);
        ret += ch;
        i = i + 2;
      } else {
        ret += value[i];
      }
    }
    return (ret);
  }
  static std::map<std::string, std::string> convertQueryStringToMap(const std::string &input) {
    std::map<std::string, std::string> output;
    for (const auto &x : UtilString::split(input, "&")) {
      auto y = UtilString::split(x, "=");
      output.insert(std::make_pair(y.at(0), y.at(1)));
    }
    return output;
  }
  std::string protocol;
  std::string host;
  std::string port;
  std::string target;  // should be url-encoded
};
} /* namespace ccapi */
#endif  // INCLUDE_CCAPI_CPP_CCAPI_URL_H_
