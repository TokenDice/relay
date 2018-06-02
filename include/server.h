#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/queue.h>
#include <string>
#include <vector>
#include <event.h>
#include <evhttp.h>
#include <event2/keyvalq_struct.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include "curl/curl.h"
#include "easylogging++.h"

#include <vector>

class HTTPRequest;
using HTTPRequestHandler = std::function<void(std::unique_ptr<HTTPRequest> req)>;

//extern std::unique_ptr<CDatabaseObject> dbptr;

static const std::string ERROR_REQUEST ="invalid request";

struct HTTPPathHandler
{
    HTTPPathHandler() {}
    HTTPPathHandler(std::string _prefix, HTTPRequestHandler _handler):prefix(_prefix), handler(_handler){}
    std::string prefix;
    HTTPRequestHandler handler;
};
//


class HTTPRequest
{
private:
    struct evhttp_request* req;
public:
    HTTPRequest(struct evhttp_request* req);
    ~HTTPRequest();

    enum RequestMethod
    {
        UNKNOWN,
        GET,
        POST,
        HEAD,
        PUT,
		OPTIONS
    };

    std::string GetURI();

    RequestMethod GetRequestMethod();

    std::string GetHeader();

    std::string ReadBody();

    void GetPeer();

    void WriteHeader(const std::string& hdr, const std::string& value);

    void WriteReply(int nStatus, const std::string& strReply = "");
};

void readconf();

int getListenPort();

std::string getBindAddr();

int getTimeOut();

bool isDaemon();

void httpRequestCb(struct evhttp_request *req, void *arg);

void registerHTTPHandler(const std::string &prefix,const HTTPRequestHandler &handler);

bool isHex(const std::string& str);

signed char hexDigit(char c);

bool checkHash(const std::string &txid);

void runDaemon(bool daemon);

void signalHandler(int sig);

bool contentToipfshash(const std::string &content, std::string &ipfsHash);

CURLcode curl_post_req(const std::string &url, const std::string &postParams, std::string &filepath, std::string &response);

size_t req_reply(void *ptr, size_t size, size_t nmemb, void *stream);


void encodeNumber(std::unique_ptr<HTTPRequest> req);

void getSecret(std::unique_ptr<HTTPRequest> req);

void createFundTx(std::unique_ptr<HTTPRequest> req);

void signFundTx(std::unique_ptr<HTTPRequest> req);

void getFundTx(std::unique_ptr<HTTPRequest> req);

void anounceSecret(std::unique_ptr<HTTPRequest> req);

void getNum(std::unique_ptr<HTTPRequest> req);

#endif //server.h
