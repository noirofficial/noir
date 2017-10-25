#include "HTTPGetRequest.h"

HTTPGetRequest::HTTPGetRequest(boost::asio::io_service& io_service, std::string host, std::string clipURL, HTTPRequestDataReceived receivedCB, HTTPRequestComplete completeCB) :
 m_host(host),
 m_relativeURL(clipURL),
 m_io_service(io_service),
 m_socket(io_service),
 m_resolver(m_io_service),
 m_receivedCB(receivedCB),
 m_completeCB(completeCB)
{

}

HTTPGetRequest::~HTTPGetRequest()
{

}

// host should be in format such as "www.google.co.nz"
// url should be in format such as "/index.html"
void HTTPGetRequest::sendRequest()
{
 tcp::resolver::query query(m_host, "http");

 m_resolver.async_resolve(query,
  [this](const boost::system::error_code& ec, tcp::resolver::iterator endpoint_iterator)
  {
   boost::asio::async_connect(m_socket, endpoint_iterator,
    [this](boost::system::error_code ec, tcp::resolver::iterator)
   {
    if (!ec)
    {
     std::ostream request_stream(&m_request);
     request_stream << "GET " << m_relativeURL << " HTTP/1.1\r\n";
     request_stream << "Host: " << m_host << "\r\n";
     request_stream << "Accept: */*\r\n";
     request_stream << "Connection: close\r\n\r\n";

     boost::asio::async_write(m_socket, m_request,
      [this](boost::system::error_code ec, std::size_t /*length*/)
     {
      boost::asio::async_read_until(m_socket, m_response, "\r\n\r\n",
       [this](boost::system::error_code ec, std::size_t length)
       {
        ReadData();
       });
     });
    }
   });
  });
}

void HTTPGetRequest::ReadData()
{
 boost::asio::async_read(m_socket, m_response, boost::asio::transfer_at_least(1),
  [this](boost::system::error_code ec, std::size_t /*length*/)
 {
  size_t size = m_response.size();
  if (size > 0)
  {
   std::unique_ptr<char> buf(new char[size]);
   m_response.sgetn(buf.get(), size);

   m_receivedCB(buf.get(), size);
  }

  if (ec != boost::asio::error::eof)
  {
   ReadData();
   return;
  }

  m_socket.close();

  m_completeCB();
 });
}
