#ifndef HTTPGETREQUEST_H
#define HTTPGETREQUEST_H

#pragma once

#include <string>

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

typedef void(*HTTPRequestDataReceived)(char*, size_t);
typedef void(*HTTPRequestComplete)();

class HTTPGetRequest
{
public:
 HTTPGetRequest(
  boost::asio::io_service& io_service,
  std::string host,
  std::string relativeURL,
  HTTPRequestDataReceived receivedCB,
  HTTPRequestComplete completeCB);

 ~HTTPGetRequest();

public:
 void sendRequest();

private:
 HTTPRequestDataReceived m_receivedCB;
 HTTPRequestComplete m_completeCB;

 std::string m_host;
 std::string m_relativeURL;

 tcp::socket m_socket;
 boost::asio::io_service &m_io_service;
 tcp::resolver m_resolver;

 boost::asio::streambuf m_request;
 boost::asio::streambuf m_response;

 void ReadData();
};


#endif // HTTPGETREQUEST_H
