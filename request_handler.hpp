//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_REQUEST_HANDLER_HPP
#define HTTP_SERVER2_REQUEST_HANDLER_HPP

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem/path.hpp>

namespace http {
namespace server2 {

#define MEM_CACHE_SIZE (1024 * 1024)
struct reply;
struct request;

/// The common handler for all incoming requests.
class request_handler
  : private boost::noncopyable
{
public:
  /// Construct with a directory containing files to be served.
  explicit request_handler(const std::string& doc_root);

  /// Handle a request and produce a reply.
  void handle_request(const request& req, reply& rep);

private:
  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
  static std::string url_encode(const std::string &value);
  static std::string format_time(time_t t);
  static std::string size_string(boost::uint64_t bytes);
  static bool compare_nocase (const boost::filesystem::path& first, const boost::filesystem::path& second);

#ifdef WIN32
  static std::string ansi_to_utf8(const std::string& ansi);
  static std::string utf8_to_ansi(const std::string& utf8);
#endif // WIN32
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_REQUEST_HANDLER_HPP
