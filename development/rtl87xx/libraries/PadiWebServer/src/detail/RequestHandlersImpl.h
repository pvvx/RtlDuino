#ifndef REQUESTHANDLERSIMPL_H
#define REQUESTHANDLERSIMPL_H

#include "RequestHandler.h"

// Table of extension->MIME strings stored in PROGMEM, needs to be global due to GCC section typing rules
static const struct {const char endsWith[16]; const char mimeType[32];} mimeTable[] = {
    { ".html", "text/html" },
    { ".htm", "text/html" },
    { ".css", "text/css" },
    { ".css.map", "application/json" },
    { ".txt", "text/plain" },
    { ".js", "application/javascript" },
    { ".js.map", "application/json" },
    { ".json", "application/json" },
    { ".png", "image/png" },
    { ".gif", "image/gif" },
    { ".jpg", "image/jpeg" },
    { ".ico", "image/x-icon" },
    { ".svg", "image/svg+xml" },
    { ".ttf", "application/x-font-ttf" },
    { ".otf", "application/x-font-opentype" },
    { ".woff", "application/font-woff" },
    { ".woff2", "application/font-woff2" },
    { ".eot", "application/vnd.ms-fontobject" },
    { ".sfnt", "application/font-sfnt" },
    { ".xml", "text/xml" },
    { ".pdf", "application/pdf" },
    { ".zip", "application/zip" },
    { ".gz", "application/x-gzip" },
    { ".appcache", "text/cache-manifest" },
    { "", "application/octet-stream" } };

class FunctionRequestHandler : public RequestHandler {
public:
    FunctionRequestHandler(PadiWebServer::THandlerFunction fn, PadiWebServer::THandlerFunction ufn, const String &uri, HTTPMethod method)
    : _fn(fn)
    , _ufn(ufn)
    , _uri(uri)
    , _method(method)
    {
    }

    bool canHandle(HTTPMethod requestMethod, String requestUri) override  {
        if (_method != HTTP_ANY && _method != requestMethod)
            return false;

        if (requestUri != _uri)
            return false;

        return true;
    }

    bool canUpload(String requestUri) override  {
        if (!_ufn || !canHandle(HTTP_POST, requestUri))
            return false;

        return true;
    }

    bool handle(PadiWebServer& server, HTTPMethod requestMethod, String requestUri) override {
        (void) server;
        if (!canHandle(requestMethod, requestUri))
            return false;

        _fn();
        return true;
    }

    void upload(PadiWebServer& server, String requestUri, HTTPUpload& upload) override {
        (void) server;
        (void) upload;
        if (canUpload(requestUri))
            _ufn();
    }

protected:
    PadiWebServer::THandlerFunction _fn;
    PadiWebServer::THandlerFunction _ufn;
    String _uri;
    HTTPMethod _method;
};

static String getContentType(const String& path) {
    // Check all entries but last one for match, return if found
    for (size_t i=0; i < sizeof(mimeTable)/sizeof(mimeTable[0])-1; i++) {
        if (path.endsWith(mimeTable[i].endsWith)) {
            return String(mimeTable[i].mimeType);
        }
    }

    return String(mimeTable[sizeof(mimeTable)/sizeof(mimeTable[0])-1].mimeType);
}





/*class StaticRequestHandler : public RequestHandler {
public:
    StaticRequestHandler(SdFatFs& fs, const char* path, const char* uri, const char* cache_header)
    : _fs(fs)
    , _uri(uri)
    , _path(path)
    , _cache_header(cache_header)
    {
        _isFile = fs.isFile(path);
        //DEBUGV("StaticRequestHandler: path=%s uri=%s isFile=%d, cache_header=%s\r\n", path, uri, _isFile, cache_header);
        _baseUriLength = _uri.length();
    }

    bool canHandle(HTTPMethod requestMethod, String requestUri) override  {
        if (requestMethod != HTTP_GET)
            return false;

        if ((_isFile == 1 && requestUri != _uri) || !requestUri.startsWith(_uri))
            return false;

        return true;
    }

    bool handle(PadiWebServer& server, HTTPMethod requestMethod, String requestUri) override {
        if (!canHandle(requestMethod, requestUri))
            return false;

      //  DEBUGV("StaticRequestHandler::handle: request=%s _uri=%s\r\n", requestUri.c_str(), _uri.c_str());

        String path(_path);

        if (_isFile != 1) {
            // Base URI doesn't point to a file.
            // If a directory is requested, look for index file.
            if (requestUri.endsWith("/")) requestUri += "index.htm";

            // Append whatever follows this URI in request to get the file path.
            path += requestUri.substring(_baseUriLength);
        }
      //  DEBUGV("StaticRequestHandler::handle: path=%s, isFile=%d\r\n", path.c_str(), _isFile);

        String contentType = getContentType(path);

        // look for gz file, only if the original specified path is not a gz.  So part only works to send gzip via content encoding when a non compressed is asked for
        // if you point the the path to gzip you will serve the gzip as content type "application/x-gzip", not text or javascript etc...
        if (!path.endsWith(".gz") && !_fs.isFile(path.c_str()))  {
            String pathWithGz = path + ".gz";
            if(_fs.isFile(pathWithGz.c_str()) == 1)
                path += ".gz";
        }

        SdFatFile f = _fs.open(path.c_str());
        if (!f)
            return false;

        if (_cache_header.length() != 0)
            server.sendHeader("Cache-Control", _cache_header);

        server.streamFile(f, contentType, path);
        return true;
    }

    static String getContentType(const String& path) {
        char buff[sizeof(mimeTable[0].mimeType)];
        // Check all entries but last one for match, return if found
        for (size_t i=0; i < sizeof(mimeTable)/sizeof(mimeTable[0])-1; i++) {
            if (path.endsWith(mimeTable[i].endsWith)) {
                return String(mimeTable[i].mimeType);
            }
        }

        return String(mimeTable[sizeof(mimeTable)/sizeof(mimeTable[0])-1].mimeType);
    }

protected:
    SdFatFs _fs;
    String _uri;
    String _path;
    String _cache_header;
    bool _isFile;
    size_t _baseUriLength;
};*/



#endif //REQUESTHANDLERSIMPL_H
