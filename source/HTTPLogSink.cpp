#include "HTTPLogSink.h"

#include <google/protobuf/util/time_util.h>

using nativeformat::logger::LogInfo;

nativeformat::HTTPLogSink::HTTPLogSink(std::string url, std::string token_type,
                                       std::string token)
    : LogSink(""),
      _url(url),
      _token_type(token_type),
      _token(token),
      _client(
          http::createClient(http::standardCacheLocation(), "NFHTTPLogSink")) {}

nativeformat::HTTPLogSink::~HTTPLogSink() {}

void nativeformat::HTTPLogSink::write(const std::string &serialized_msg,
                                      Severity level) {
  write(serialized_msg);
}

void nativeformat::HTTPLogSink::callback(
    const std::shared_ptr<http::Response> &response) {
  int code = response->statusCode();
  if (!(code >= 200 && code < 300))
    fprintf(stderr, "HTTPLogSink error: response code %d\n",
            response->statusCode());
}

void nativeformat::HTTPLogSink::write(const std::string &serialized_msg) {
  // Generate a POST with serialized proto message as payload
  std::unordered_map<std::string, std::string> header_map = {};
  std::shared_ptr<http::Request> req = http::createRequest(_url, header_map);
  req->setMethod(http::PostMethod);
  req->setData((const unsigned char *)serialized_msg.c_str(),
               serialized_msg.size());
  (*req)["Authorization"] = _token_type + " " + _token;
  _client->performRequest(req, callback);
}
