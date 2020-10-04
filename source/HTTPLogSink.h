#include <NFHTTP/Client.h>
#include <NFLogger/LogSink.h>

#include <iostream>

namespace nativeformat {

using logger::LogSink;
using logger::Severity;

class HTTPLogSink : public LogSink {
 public:
  HTTPLogSink(std::string url, std::string token_type = "",
              std::string token = "");
  virtual ~HTTPLogSink();

  virtual void write(const std::string &serialized_msg, Severity level);

 private:
  void write(const std::string &serialized_msg);
  static void callback(const std::shared_ptr<http::Response> &response);

  const std::string _url;
  std::string _token_type;
  std::string _token;
  std::shared_ptr<http::Client> _client;
};
}  // namespace nativeformat
