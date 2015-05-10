//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/date_time.hpp>
#include <boost/lexical_cast.hpp>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "html.h"

namespace http {
namespace server2 {

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
}

std::string request_handler::format_time(std::time_t t)
{
	using namespace boost::posix_time;
	static std::locale loc(std::cout.getloc(),
		new time_facet("%Y/%m/%d %H:%M:%S"));
	std::basic_stringstream<char> ss;
	ss.imbue(loc);

	ss << from_time_t(t);

	return ss.str();
}

std::string request_handler::size_string(boost::uint64_t bytes)
{
	static const char* c[] = { "B", "KB", "MB", "GB", "TB"};
	static const boost::uint64_t u[] = { 1, 1 << 10, 1 << 20, 1 << 30, 1 << 40 };
	int i;
	boost::uint64_t test = bytes;
	for (i = 0; i < 5; i++)
	{
		test = test >> 10;
		if (test == 0) {
			break;
		}
	}

	char buf[16];
	if (i > 0) 
	{
		sprintf(buf, "%.2f %s", bytes * 1.00 / u[i], c[i]);
	}
	else 
	{
		sprintf(buf, "%d B", bytes);
	}
	return std::string(buf);

}

#ifdef WIN32
std::string request_handler::ansi_to_utf8(const std::string& ansi)
{
	int len = ansi.length();
	if (len == 0) return std::string();

	wchar_t* unicode = new wchar_t[len + 1];
	len = ::MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), ansi.length(), unicode, len);
	unicode[len] = 0;

	int utf8_len = len * 4;
	char* utf8 = new char[utf8_len + 1];
	utf8_len = ::WideCharToMultiByte(CP_UTF8, 0, unicode, len, utf8, utf8_len, NULL, NULL);
	utf8[utf8_len] = 0;

	std::string ret(utf8, utf8_len);
	delete[] unicode;
	delete[] utf8;

	return ret;
}

std::string request_handler::utf8_to_ansi(const std::string& utf8)
{
	int len = utf8.length();
	if (len == 0) return std::string();

	wchar_t* unicode = new wchar_t[len + 1];
	len = ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.length(), unicode, len);
	unicode[len] = 0;

	int ansi_len = len * 4;
	char* ansi = new char[ansi_len + 1];
	ansi_len = ::WideCharToMultiByte(CP_ACP, 0, unicode, len, ansi, ansi_len, NULL, NULL);
	ansi[ansi_len] = 0;

	std::string ret(ansi, ansi_len);
	delete[] unicode;
	delete[] ansi;

	return ret;
}
#endif // WIN32


void request_handler::handle_request(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return ;
  }

#ifdef WIN32
  request_path = utf8_to_ansi(request_path);
#endif

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return ;
  }

  std::string full_path = doc_root_ + request_path;

  boost::filesystem::path p(full_path);

  if (boost::filesystem::is_directory(p))
  {
	  std::ostringstream stringStream;
	  stringStream << html_str << std::endl;
	  stringStream << "<script>start(\"" << request_path << "\");</script>" << std::endl;

	  if (request_path.length() > 1)
	  {
		  stringStream << "<script>addRow(\"..\", \"..\", 1, \"0 B\", \"\"); </script>" << std::endl;
	  }

	  std::vector<boost::filesystem::path> files;


	  std::copy(boost::filesystem::directory_iterator(p), boost::filesystem::directory_iterator(), std::back_inserter(files));
	  std::sort(files.begin(), files.end());
	  try
	  {
		  for (std::vector<boost::filesystem::path>::const_iterator it(files.begin()); it != files.end(); ++it)
		  {
			  if (boost::filesystem::is_directory(*it))
			  {
				  std::string name = it->filename().string();
#ifdef WIN32
				  name = ansi_to_utf8(name);
#endif // WIN32

				  stringStream << "<script>addRow(\"" << name << "\",\"" << name << "\",";
				  stringStream << "1, \"0 B\", ";
				  stringStream << "\"" << format_time(boost::filesystem::last_write_time(*it)) << "\"" << ");</script>" << std::endl;
			  }
		  }
	  }
	  catch (...) {}

	  try
	  {
		  for (std::vector<boost::filesystem::path>::const_iterator it(files.begin()); it != files.end(); ++it)
		  {
			  if (boost::filesystem::is_regular_file(*it))
			  {
				  std::string name = it->filename().string();
#ifdef WIN32
				  name = ansi_to_utf8(name);
#endif // WIN32
				  stringStream << "<script>addRow(\"" << name << "\",\"" << name << "\",";
				  stringStream << "0, \"" << size_string(boost::filesystem::file_size(*it)) << "\", ";
				  stringStream << "\"" << format_time(boost::filesystem::last_write_time(*it)) << "\"" << ");</script>" << std::endl;
			  }
		  }
	  }
	  catch (...) {}


	  rep.content.append(stringStream.str());

	  rep.status = reply::ok;
	  rep.headers.resize(2);
	  rep.headers[0].name = "Content-Length";
	  rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	  rep.headers[1].name = "Content-Type";
	  rep.headers[1].value = mime_types::extension_to_type("html");

	  return ;
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
  {
    extension = request_path.substr(last_dot_pos + 1);
  }

  // Open the file to send back.
  boost::shared_ptr<std::ifstream> is(new std::ifstream(full_path.c_str(), std::ios::in | std::ios::binary));
  if (!is->is_open())
  {
	  rep = reply::stock_reply(reply::not_found);
	  return ;
  }

  boost::uint64_t file_len = boost::filesystem::file_size(p);

  rep.status = reply::ok;
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = boost::lexical_cast<std::string>(file_len);
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = mime_types::extension_to_type(extension);

  char buf[1024];
  if (is->read(buf, sizeof(buf)).gcount() > 0) {
	  rep.content.append(buf, is->gcount());
  }

  rep.file_stream = is;

  return ;
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

} // namespace server2
} // namespace http
