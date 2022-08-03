#include "GetProcessor.hpp"

GetProcessor::GetProcessor(Response & response, FdTable & fdTable,
                            Monitor & monitor) : Processor(fdTable, monitor),
                                                  _response(response)
{}

GetProcessor::~GetProcessor(void)
{}

/*
**  A hash table could be implemented to avoid some file I/O and
**  increase response efficiency.
**
**  If user sets a cgi file as default, the complete path,
**  including the cgi_dir, must be provided in property "default_file"
**  of config file.
*/

bool  GetProcessor::_getFilePath(ConnectionData & connData) const
{
  LocationConfig const *  loc = connData.getLocation();
  std::map<std::string, std::string>::iterator  fileName;
  std::ifstream           file;

  connData.filePath = loc->root + '/';
  fileName = connData.urlData.find("FileName");
  if (fileName != connData.urlData.end()) //FileName present in request uri
  { //Provisional. Need to check multiple cgi extensions if necessary (php, cgi, py, etc.)
    if (connData.urlData.find("FileType")->second != ".cgi") //Requested file is not cgi
      connData.filePath.append(fileName->second);
    else if (loc->cgi_dir != "") //Requested file is cgi, and cgi_dir exists
      connData.filePath = loc->cgi_dir + '/' + fileName->second;
    else //Requested file is cgi, but no cgi_dir exists in this location
      return (false);
  }
  else //FileName not present in request uri
    connData.filePath.append(loc->default_file);
  file.open(connData.filePath.c_str());
  if (file.is_open()) //Requested file exists
    file.close();
  else //Requested file does not exist
    return (false);
  return (true);
}

bool  GetProcessor::_launchCGI(ConnectionData & connData, int const sockFd,
                                std::size_t const monitorIndex) const
{
  CgiData * cgiData;
  /*
  **  ADD CGI write and read pipe fds to this->_monitor
  **  and to this->_cgiPipes (Future fd direct address table)
  */
  cgiData = new CgiData(sockFd, monitorIndex, connData.filePath);
  if (!this->_response.cgiHandler.initPipes(*cgiData,
      *this->_response.cgiHandler.getEnv(connData.req.getHeaders(),
                                          connData.urlData, connData.ip)))
  {
    delete cgiData;
    return (false);
  }
  //Set non-blocking pipe fds
  if (fcntl(cgiData->getWInPipe(), F_SETFL, O_NONBLOCK)
      || fcntl(cgiData->getROutPipe(), F_SETFL, O_NONBLOCK))
  {
    std::cerr << "Could not set non-blocking pipe fds" << std::endl;
    close(cgiData->getWInPipe());
    close(cgiData->getROutPipe());
    delete cgiData;
    return (false);
  }
  //Provisional, need to check writen bytes after each call, as it is non-blocking
  write(cgiData->getWInPipe(), "Hola Mundo!", 11);
  close(cgiData->getWInPipe());
  //Associate read pipe fd with cgi class instance
  this->_fdTable.add(cgiData->getROutPipe(), cgiData);
  //Check POLLIN event of read pipe fd with poll()
  /*
  **  Determine if it makes sense to check for POLLOUT if it is never
  **  going to be used for listening sockets.
  */
  this->_monitor.add(cgiData->getROutPipe(), POLLIN /*| POLLOUT*/);
  return (true);
}

bool  GetProcessor::_openFile(ConnectionData & connData, int const sockFd,
                              std::size_t const monitorIndex) const
{
  if (!this->_response.fileHandler.openFile(connData.filePath, connData.fileFd))
    return (false);
  this->_monitor.add(connData.fileFd, POLLIN | POLLOUT);
  this->_fdTable.add(connData.fileFd,
                      new std::pair<int,std::size_t>(sockFd, monitorIndex));
  return (true);
}

bool  GetProcessor::start(int const sockFd, std::size_t const monitorIndex,
                          int & error) const
{
  ConnectionData &        connData = this->_fdTable.getConnSock(sockFd);
  LocationConfig const *  loc = connData.getLocation();

  if (!this->_getFilePath(connData))
  {
    connData.filePath.clear();
    if (loc->dir_list == true && !connData.urlData.count("FileName"))
    {
      this->_response.buildDirList(
        connData, connData.urlData.find("Path")->second, loc->root);
    }
    else
    {
      error = 404; // Not Found
      return (false);
    }
  }
  else
  {
    if (connData.filePath.rfind(".cgi") != std::string::npos) //Provisional. TODO: substr and able to check multiple cgi extensions if necessary
    {
      if (!this->_launchCGI(connData, sockFd, monitorIndex))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
    else
    {
      if(!this->_openFile(connData, sockFd, monitorIndex))
      {
        error = 500; // Internal Server Error
        return (false);
      }
      connData.rspStatus = 200; //Provisional
    }
  }
  return (true);
}
