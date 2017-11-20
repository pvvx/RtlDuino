/*
  PadiWebServer.cpp - Dead simple web-server.
  Supports only one simultaneous client, knows how to handle GET and POST.

  Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  Modified 8 May 2015 by Hristo Gochkov (proper post and file upload handling)
*/


#include <Arduino.h>
#include <libb64/cencode.h>
//#include "WiFiServer.h"
//#include "WiFiClient.h"
#include "PadiWebServer.h"
//#include "FS.h"
#include "SdFatFs.h"
#include "detail/RequestHandlersImpl.h"
#include "rtl_crypto.h"


//#define DEBUG_ESP_HTTP_SERVER
#ifdef DEBUG_ESP_PORT
#define DEBUG_OUTPUT DEBUG_ESP_PORT
#else
#define DEBUG_OUTPUT Serial
#endif

const char * AUTHORIZATION_HEADER = "Authorization";
/*
PadiWebServer::PadiWebServer(IPAddress addr, int port)
: _server(addr, port)
, _currentMethod(HTTP_ANY)
, _currentVersion(0)
, _currentStatus(HC_NONE)
, _statusChange(0)
, _currentHandler(0)
, _firstHandler(0)
, _lastHandler(0)
, _currentArgCount(0)
, _currentArgs(0)
, _headerKeysCount(0)
, _currentHeaders(0)
, _contentLength(0)
, _chunked(false)
{
}
*/

PadiWebServer::PadiWebServer(int port)
: _server(port)
, _currentMethod(HTTP_ANY)
, _currentVersion(0)
, _currentStatus(HC_NONE)
, _statusChange(0)
, _currentHandler(0)
, _firstHandler(0)
, _lastHandler(0)
, _currentArgCount(0)
, _currentArgs(0)
, _headerKeysCount(0)
, _currentHeaders(0)
, _contentLength(0)
, _chunked(false)
{
}

PadiWebServer::~PadiWebServer() {
  if (_currentHeaders)
    delete[]_currentHeaders;
  _headerKeysCount = 0;
  RequestHandler* handler = _firstHandler;
  while (handler) {
    RequestHandler* next = handler->next();
    delete handler;
    handler = next;
  }
  close();
}

void PadiWebServer::begin() {
  _currentStatus = HC_NONE;
  _server.begin();
  if(!_headerKeysCount)
    collectHeaders(0, 0);
}

String PadiWebServer::_exractParam(String& authReq,const String& param,const char delimit){
  int _begin = authReq.indexOf(param);
  if (_begin==-1) return "";
  return authReq.substring(_begin+param.length(),authReq.indexOf(delimit,_begin+param.length()));
}

bool PadiWebServer::authenticate(const char * username, const char * password){

//#include "hal_crypto.h"
//if ( rtl_cryptoEngine_init() != 0 ) {
//                DiagPrintf("crypto engine init failed\r\n");
//        }
//  ret = rtl_crypto_md5(plaintext, strlen(plaintext), (unsigned char *)&digest); // the length of MD5's digest is 16 bytes.

  if(hasHeader(AUTHORIZATION_HEADER)){
    String authReq = header(AUTHORIZATION_HEADER);
    if(authReq.startsWith("Basic")){
      authReq = authReq.substring(6);
      authReq.trim();
      char toencodeLen = strlen(username)+strlen(password)+1;
      char *toencode = new char[toencodeLen + 1];
      if(toencode == NULL){
        authReq = String();
        return false;
      }
      char *encoded = new char[base64_encode_expected_len(toencodeLen)+1];
      if(encoded == NULL){
        authReq = String();
        delete[] toencode;
        return false;
      }
      sprintf(toencode, "%s:%s", username, password);
      if(base64_encode_chars(toencode, toencodeLen, encoded) > 0 && authReq.equals(encoded)){
        authReq = String();
        delete[] toencode;
        delete[] encoded;
        return true;
      }
      delete[] toencode;
      delete[] encoded;
    }else if(authReq.startsWith("Digest")){
      authReq = authReq.substring(7);
      #ifdef DEBUG_ESP_HTTP_SERVER
      DEBUG_OUTPUT.println(authReq);
      #endif
      String _username = _exractParam(authReq,"username=\"");
      if((!_username.length())||_username!=String(username)){
        authReq = String();
        return false;
      }
      // extracting required parameters for RFC 2069 simpler Digest
      String _realm    = _exractParam(authReq,"realm=\"");
      String _nonce    = _exractParam(authReq,"nonce=\"");
      String _uri      = _exractParam(authReq,"uri=\"");
      String _response = _exractParam(authReq,"response=\"");
      String _opaque   = _exractParam(authReq,"opaque=\"");

      if((!_realm.length())||(!_nonce.length())||(!_uri.length())||(!_response.length())||(!_opaque.length())){
        authReq = String();
        return false;
      }
      if((_opaque!=_sopaque)||(_nonce!=_snonce)||(_realm!=_srealm)){
        authReq = String();
        return false;
      }
      // parameters for the RFC 2617 newer Digest
      String _nc,_cnonce;
      if(authReq.indexOf("qop=auth") != -1){
        _nc = _exractParam(authReq,"nc=",',');
        _cnonce = _exractParam(authReq,"cnonce=\"");
      }
      String buff;

      buff += (String(username)+":"+_realm+":"+String(password));  // md5 of the user:realm:user

      String _H1 = computeMD5(buff.data(),buff.size()).toString();
      #ifdef DEBUG_ESP_HTTP_SERVER
      DEBUG_OUTPUT.println("Hash of user:realm:pass=" + _H1);
      #endif
      buff = "";
      if(_currentMethod == HTTP_GET){
        buff += ("GET:"+_uri);
      }else if(_currentMethod == HTTP_POST){
        buff += ("POST:"+_uri);
      }else if(_currentMethod == HTTP_PUT){
        buff += ("PUT:"+_uri);
      }else if(_currentMethod == HTTP_DELETE){
        buff += ("DELETE:"+_uri);
      }else{
        buff += ("GET:"+_uri);
      }

      String _H2 = computeMD5(buff.data(),buff.size()).toString();
      #ifdef DEBUG_ESP_HTTP_SERVER
      DEBUG_OUTPUT.println("Hash of GET:uri=" + _H2);
      #endif
      buff = "";
      if(authReq.indexOf("qop=auth") != -1){
        buff += (_H1+":"+_nonce+":"+_nc+":"+_cnonce+":auth:"+_H2);
      }else{
        buff += (_H1+":"+_nonce+":"+_H2);
      }

      String _responsecheck = computeMD5(buff.data(),buff.size()).toString();

      #ifdef DEBUG_ESP_HTTP_SERVER
      DEBUG_OUTPUT.println("The Proper response=" +_responsecheck);
      #endif
      if(_response==_responsecheck){
        authReq = String();
        return true;
      }
    }
    authReq = String();
  }
  return false;
}

String PadiWebServer::_getRandomHexString(){
  unsigned char rnd[16];
  char buffer[33];  // buffer to hold 32 Hex Digit + /0
  for (int i = 0; i< 16; ++i){
    rnd[i] = random(255);
  }

  renderHexdata(buffer,33,reinterpret_cast<uint8_t *>(rnd),16);
  return String(buffer);
}

void PadiWebServer::requestAuthentication(HTTPAuthMethod mode, const char* realm, const String& authFailMsg){
  if(realm==NULL){
    _srealm = "Login Required";
  }else{
    _srealm = String(realm);
  }
  if(mode==BASIC_AUTH){
    sendHeader("WWW-Authenticate", "Basic realm=\"" + _srealm + "\"");
  }else{
    _snonce=_getRandomHexString();
    _sopaque=_getRandomHexString();
    sendHeader("WWW-Authenticate", "Digest realm=\"" +_srealm + "\", qop=\"auth\", nonce=\""+_snonce+"\", opaque=\""+_sopaque+"\"");
  }
  send(401,"text/html",authFailMsg);
}

void PadiWebServer::on(const String &uri, PadiWebServer::THandlerFunction handler) {
  on(uri, HTTP_ANY, handler);
}

void PadiWebServer::on(const String &uri, HTTPMethod method, PadiWebServer::THandlerFunction fn) {
  on(uri, method, fn, _fileUploadHandler);
}

void PadiWebServer::on(const String &uri, HTTPMethod method, PadiWebServer::THandlerFunction fn, PadiWebServer::THandlerFunction ufn) {
  _addRequestHandler(new FunctionRequestHandler(fn, ufn, uri, method));
}

void PadiWebServer::addHandler(RequestHandler* handler) {
    _addRequestHandler(handler);
}

void PadiWebServer::_addRequestHandler(RequestHandler* handler) {
    if (!_lastHandler) {
      _firstHandler = handler;
      _lastHandler = handler;
    }
    else {
      _lastHandler->next(handler);
      _lastHandler = handler;
    }
}
/*
void PadiWebServer::serveStatic(const char* uri, SdFatFs& fs, const char* path, const char* cache_header) {
    _addRequestHandler(new StaticRequestHandler(fs, path, uri, cache_header));
}*/
int PadiWebServer::serveStatic(SdFatFs& fs, String path){

      if (this->method() !=  HTTP_GET){
            return -1;
      }

      path += this->uri();

      if (this->uri().endsWith("/")) path += "index.html";
      //check if file exists
      if (fs.isFile(path.c_str()) != 1){
        return -1;
      }
      String contentType(getContentType(path));
      //check if gz version exists
      if (fs.isFile(String(path + ".gz").c_str()) == 1){
        path += ".gz";
      }



      SdFatFile file = fs.open(path.c_str(),"r");
      int ret = this->streamFile(file,contentType,path);
      file.close();
      return ret;
}

size_t PadiWebServer::streamFile(SdFatFile &file, const String& contentType,  const String& fileName){

      char buffer[4096];
      size_t size = 0;
      int readLen;
      setContentLength(file.size());
      if (fileName.endsWith(".gz") &&
          contentType != "application/x-gzip" &&
          contentType != "application/octet-stream"){
        sendHeader("Content-Encoding", "gzip");
      }
      send(200, contentType, "");
      /*write file with 1024 byte buffer*/
      while(true){
        readLen = file.read(buffer,sizeof(buffer));
        if (readLen){
          size += readLen;
          _currentClient.write(buffer,readLen);
        }
        else{
          break;
        }
      }
      return size;
}

void PadiWebServer::handleClient() {
  if (_currentStatus == HC_NONE) {
    WiFiClient client = _server.available();
    if (!client) {
      return;
    }

#ifdef DEBUG_ESP_HTTP_SERVER
    DEBUG_OUTPUT.println("New client");
#endif

    _currentClient = client;
    _currentStatus = HC_WAIT_READ;
    _statusChange = millis();
  }

  bool keepCurrentClient = false;
  bool callYield = false;

  if (_currentClient.connected()) {
    switch (_currentStatus) {
    case HC_WAIT_READ:
      // Wait for data from client to become available
      if (_currentClient.available()) {
        if (_parseRequest(_currentClient)) {
          _currentClient.setTimeout(HTTP_MAX_SEND_WAIT);
          _contentLength = CONTENT_LENGTH_NOT_SET;
          _handleRequest();

          if (_currentClient.connected()) {
            _currentStatus = HC_WAIT_CLOSE;
            _statusChange = millis();
            keepCurrentClient = true;
          }
        }
      } else { // !_currentClient.available()
        if (millis() - _statusChange <= HTTP_MAX_DATA_WAIT) {
          keepCurrentClient = true;
        }
        callYield = true;
      }
      break;
    case HC_WAIT_CLOSE:
      // Wait for client to close the connection
      if (millis() - _statusChange <= HTTP_MAX_CLOSE_WAIT) {
        keepCurrentClient = true;
        callYield = true;
      }
    }
  }

  if (!keepCurrentClient) {
    _currentClient = WiFiClient();
    _currentStatus = HC_NONE;
    _currentUpload.reset();
  }

  if (callYield) {
    yield();
  }
}

void PadiWebServer::close() {
  _server.stop();
}

void PadiWebServer::stop() {
  close();
}

void PadiWebServer::sendHeader(const String& name, const String& value, bool first) {
  String headerLine = name;
  headerLine += ": ";
  headerLine += value;
  headerLine += "\r\n";

  if (first) {
    _responseHeaders = headerLine + _responseHeaders;
  }
  else {
    _responseHeaders += headerLine;
  }
}

void PadiWebServer::setContentLength(size_t contentLength) {
    _contentLength = contentLength;
}

void PadiWebServer::_prepareHeader(String& response, int code, const char* content_type, size_t contentLength) {
    response = "HTTP/1."+String(_currentVersion)+" ";
    response += String(code);
    response += " ";
    response += _responseCodeToString(code);
    response += "\r\n";

    if (!content_type)
        content_type = "text/html";

    sendHeader("Content-Type", content_type, true);
    if (_contentLength == CONTENT_LENGTH_NOT_SET) {
        sendHeader("Content-Length", String(contentLength));
    } else if (_contentLength != CONTENT_LENGTH_UNKNOWN) {
        sendHeader("Content-Length", String(_contentLength));
    } else if(_contentLength == CONTENT_LENGTH_UNKNOWN && _currentVersion){ //HTTP/1.1 or above client
      //let's do chunked
      _chunked = true;
      sendHeader("Accept-Ranges","none");
      sendHeader("Transfer-Encoding","chunked");
    }
    sendHeader("Connection", "close");

    response += _responseHeaders;
    response += "\r\n";
    _responseHeaders = String();
}

void PadiWebServer::send(int code, const char* content_type, const String& content) {
    String header;
    // Can we asume the following?
    //if(code == 200 && content.length() == 0 && _contentLength == CONTENT_LENGTH_NOT_SET)
    //  _contentLength = CONTENT_LENGTH_UNKNOWN;
    _prepareHeader(header, code, content_type, content.length());
    _currentClient.write(header.data(), header.size());
    if(content.size())
      sendContent(content);
}
/*
void PadiWebServer::send_P(int code, PGM_P content_type, PGM_P content) {
    size_t contentLength = 0;

    if (content != NULL) {
        contentLength = strlen_P(content);
    }

    String header;
    char type[64];
    memccpy_P((void*)type, (PGM_VOID_P)content_type, 0, sizeof(type));
    _prepareHeader(header, code, (const char* )type, contentLength);
    _currentClient.write(header.c_str(), header.length());
    sendContent_P(content);
}

void PadiWebServer::send_P(int code, PGM_P content_type, PGM_P content, size_t contentLength) {
    String header;
    char type[64];
    memccpy_P((void*)type, (PGM_VOID_P)content_type, 0, sizeof(type));
    _prepareHeader(header, code, (const char* )type, contentLength);
    sendContent(header);
    sendContent_P(content, contentLength);
}*/

void PadiWebServer::send(int code, char* content_type, const String& content) {
  send(code, (const char*)content_type, content);
}

void PadiWebServer::send(int code, const String& content_type, const String& content) {
  send(code, (const char*)content_type.c_str(), content);
}

void PadiWebServer::sendContent(const String& content) {
  const char * footer = "\r\n";
  size_t len = content.size();
  if(_chunked) {
    char * chunkSize = (char *)malloc(11);
    if(chunkSize){
      sprintf(chunkSize, "%x%s", len, footer);
      _currentClient.write(chunkSize, strlen(chunkSize));
      free(chunkSize);
    }
  }
  _currentClient.write(content.data(), len);
  if(_chunked){
    _currentClient.write(footer, 2);
  }
}
/*
void PadiWebServer::sendContent_P(PGM_P content) {
  sendContent_P(content, strlen_P(content));
}

void PadiWebServer::sendContent_P(PGM_P content, size_t size) {
  const char * footer = "\r\n";
  if(_chunked) {
    char * chunkSize = (char *)malloc(11);
    if(chunkSize){
      sprintf(chunkSize, "%x%s", size, footer);
      _currentClient.write(chunkSize, strlen(chunkSize));
      free(chunkSize);
    }
  }
  _currentClient.write(content, size);
  if(_chunked){
    _currentClient.write(footer, 2);
  }
}

*/
String PadiWebServer::arg(String name) {
  for (int i = 0; i < _currentArgCount; ++i) {
    if ( _currentArgs[i].key == name )
      return _currentArgs[i].value;
  }
  return String();
}

String PadiWebServer::arg(int i) {
  if (i < _currentArgCount)
    return _currentArgs[i].value;
  return String();
}

String PadiWebServer::argName(int i) {
  if (i < _currentArgCount)
    return _currentArgs[i].key;
  return String();
}

int PadiWebServer::args() {
  return _currentArgCount;
}

bool PadiWebServer::hasArg(String  name) {
  for (int i = 0; i < _currentArgCount; ++i) {
    if (_currentArgs[i].key == name)
      return true;
  }
  return false;
}


String PadiWebServer::header(String name) {
  for (int i = 0; i < _headerKeysCount; ++i) {
    if (_currentHeaders[i].key.equalsIgnoreCase(name))
      return _currentHeaders[i].value;
  }
  return String();
}

void PadiWebServer::collectHeaders(const char* headerKeys[], const size_t headerKeysCount) {
  _headerKeysCount = headerKeysCount + 1;
  if (_currentHeaders)
     delete[]_currentHeaders;
  _currentHeaders = new RequestArgument[_headerKeysCount];
  _currentHeaders[0].key = AUTHORIZATION_HEADER;
  for (int i = 1; i < _headerKeysCount; i++){
    _currentHeaders[i].key = headerKeys[i-1];
  }
}

String PadiWebServer::header(int i) {
  if (i < _headerKeysCount)
    return _currentHeaders[i].value;
  return String();
}

String PadiWebServer::headerName(int i) {
  if (i < _headerKeysCount)
    return _currentHeaders[i].key;
  return String();
}

int PadiWebServer::headers() {
  return _headerKeysCount;
}

bool PadiWebServer::hasHeader(String name) {
  for (int i = 0; i < _headerKeysCount; ++i) {
    if ((_currentHeaders[i].key.equalsIgnoreCase(name)) &&  (_currentHeaders[i].value.length() > 0))
      return true;
  }
  return false;
}

String PadiWebServer::hostHeader() {
  return _hostHeader;
}

void PadiWebServer::onFileUpload(THandlerFunction fn) {
  _fileUploadHandler = fn;
}

void PadiWebServer::onNotFound(THandlerFunction fn) {
  _notFoundHandler = fn;
}

void PadiWebServer::_handleRequest() {
  bool handled = false;
  if (!_currentHandler){
#ifdef DEBUG_ESP_HTTP_SERVER
    DEBUG_OUTPUT.println("request handler not found");
#endif
  }
  else {
    handled = _currentHandler->handle(*this, _currentMethod, _currentUri);
#ifdef DEBUG_ESP_HTTP_SERVER
    if (!handled) {
      DEBUG_OUTPUT.println("request handler failed to handle request");
    }
#endif
  }

  if (!handled) {
    if(_notFoundHandler) {
      _notFoundHandler();
    }
    else {
      send(404, "text/plain", String("Not found: ") + _currentUri);
    }
  }

  _currentUri = String();
}

String PadiWebServer::_responseCodeToString(int code) {
  switch (code) {
    case 100: return F("Continue");
    case 101: return F("Switching Protocols");
    case 200: return F("OK");
    case 201: return F("Created");
    case 202: return F("Accepted");
    case 203: return F("Non-Authoritative Information");
    case 204: return F("No Content");
    case 205: return F("Reset Content");
    case 206: return F("Partial Content");
    case 300: return F("Multiple Choices");
    case 301: return F("Moved Permanently");
    case 302: return F("Found");
    case 303: return F("See Other");
    case 304: return F("Not Modified");
    case 305: return F("Use Proxy");
    case 307: return F("Temporary Redirect");
    case 400: return F("Bad Request");
    case 401: return F("Unauthorized");
    case 402: return F("Payment Required");
    case 403: return F("Forbidden");
    case 404: return F("Not Found");
    case 405: return F("Method Not Allowed");
    case 406: return F("Not Acceptable");
    case 407: return F("Proxy Authentication Required");
    case 408: return F("Request Time-out");
    case 409: return F("Conflict");
    case 410: return F("Gone");
    case 411: return F("Length Required");
    case 412: return F("Precondition Failed");
    case 413: return F("Request Entity Too Large");
    case 414: return F("Request-URI Too Large");
    case 415: return F("Unsupported Media Type");
    case 416: return F("Requested range not satisfiable");
    case 417: return F("Expectation Failed");
    case 500: return F("Internal Server Error");
    case 501: return F("Not Implemented");
    case 502: return F("Bad Gateway");
    case 503: return F("Service Unavailable");
    case 504: return F("Gateway Time-out");
    case 505: return F("HTTP Version not supported");
    default:  return "";
  }
}
