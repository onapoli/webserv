#include "DeleteProcessor.hpp"

DeleteProcessor::DeleteProcessor(Response & response, FdTable & fdTable,
                                  Monitor & monitor) : Processor(fdTable,
                                                                  monitor),
                                                        _response(response)
{}

DeleteProcessor::~DeleteProcessor(void) {}

bool  DeleteProcessor::_getFilePath(ConnectionData & connData,
                                    std::string & filePath) const
{
  LocationConfig const *                        loc = connData.getLocation();
  std::map<std::string, std::string>::iterator  fileName;
  std::ifstream                                 file;

  fileName = connData.urlData.find("FILENAME");
  if (fileName != connData.urlData.end()) //FileName present in request uri
  {
    if (this->_getCgiInterpreter(connData,
        connData.urlData.find("FILETYPE")->second) == "")
    { //Requested file is not cgi
      if (loc->upload_dir == "")
        return (false);
      filePath = loc->upload_dir + '/' + fileName->second;
    }
    else if (loc->cgi_dir != "") //Requested file is cgi, and cgi_dir exists
      filePath = loc->cgi_dir + '/' + fileName->second;
    else //Requested file is cgi, but no cgi_dir exists in this location
      return (false);
  }
  else
    return (false); //Maybe 400 Bad Request instead of 404 is better in this case?
  file.open(filePath.c_str());
  if (file.is_open()) //Requested file exists
    file.close();
  else //Requested file does not exist
    return (false);
  return (true);
}

bool  DeleteProcessor::_launchCGI(ConnectionData & connData, pollfd & socket,
                                  std::string const & interpreterPath,
                                  std::string const & scriptPath) const
{
  CgiData *                                           cgiData;
  std::map<std::string, std::string>::const_iterator  bodyPair;

  cgiData = new CgiData(socket, interpreterPath, scriptPath);
  if (!this->_response.cgiHandler.initPipes(*cgiData,
      *this->_response.cgiHandler.getEnv(connData.req.getHeaders(),
                                          connData.urlData,
                                          connData.getLocation()->root,
                                          connData.ip)))
  {
    delete cgiData;
    return (false);
  }
  //Set non-blocking pipe fds
  if (fcntl(cgiData->getWInPipe(), F_SETFL, O_NONBLOCK)
      || fcntl(cgiData->getROutPipe(), F_SETFL, O_NONBLOCK))
  {
    std::cerr << "Could not set non-blocking pipe fds" << std::endl;
    cgiData->closeWInPipe();
    cgiData->closeROutPipe();
    delete cgiData;
    return (false);
  }
  /*
  **  cgiData's addition to connData must be done before adding its associated
  **  fds to monitor, otherwise, if a reallocation is done by monitor,
  **  cgiData's associated pollfd will not be updated, and will point to
  **  a previous allocation of monitor fds, which has been deleted.
  **  The way in which monitor reallocates can be changed in the future to
  **  prevent this condition. Currently, monitor stores all connection fds, and
  **  iterates through them to update its associated cgiData or fileData when
  **  reallocating. An alternative could be to store all cgiData and fileData
  **  instead and update them directly without knowing its associated connection
  **  fd, but that requires to distinguish its type at the moment of updating.
  */
  connData.cgiData = cgiData;
  bodyPair = connData.req.getHeaders().find("BODY");
  if (bodyPair != connData.req.getHeaders().end())
  {
    //Associate write pipe fd with cgi class instance
    this->_fdTable.add(cgiData->getWInPipe(), cgiData, false);
    this->_monitor.add(cgiData->getWInPipe(), POLLOUT, false);
    connData.io.setPayloadSize(bodyPair->second.length());
  }
  else
    cgiData->closeWInPipe();
  //Associate read pipe fd with cgi class instance
  this->_fdTable.add(cgiData->getROutPipe(), cgiData, true);
  //Check POLLIN event of read pipe fd with poll()
  this->_monitor.add(cgiData->getROutPipe(), POLLIN, false);
  return (true);
}

bool  DeleteProcessor::_removeFile(pollfd & socket, ConnectionData & connData,
                                    std::string const & filePath) const
{
  std::string content;

  if (!this->_response.fileHandler.removeFile(filePath))
    return (false);
  //Build success response directly
  this->_response.buildDeleted(socket, connData);
  return (true);
}

bool  DeleteProcessor::start(pollfd & socket, int & error) const
{
  ConnectionData &  connData = this->_fdTable.getConnSock(socket.fd);
  std::string       filePath;
  std::string       cgiInterpreterPath;

  if (!this->_getFilePath(connData, filePath))
  {
    filePath.clear();
    error = 404; // Not Found
    return (false);
  }
  else
  {
    cgiInterpreterPath = this->_getCgiInterpreter(connData,
      filePath.substr(filePath.rfind('.') + 1));
    if (cgiInterpreterPath != "")
    {
      if (!this->_launchCGI(connData, socket, cgiInterpreterPath, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
    else
    {
      if(!this->_removeFile(socket, connData, filePath))
      {
        error = 500; // Internal Server Error
        return (false);
      }
    }
  }
  return (true);
}
