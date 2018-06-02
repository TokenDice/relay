#include "server.h"
#include <vector>

INITIALIZE_EASYLOGGINGPP

void test()
{


}

int main(int argc, char *argv[])
{
    el::Configurations conf("./conf/server_log.conf");//log
    el::Loggers::reconfigureAllLoggers(conf);
    LOG(INFO) << "---  start server  ---";

    signal(SIGHUP, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);

    readconf();
    // read conf file ?
    LOG(INFO) << getListenPort();
    LOG(INFO) << getBindAddr();
    LOG(INFO) << getTimeOut();
    LOG(INFO) << isDaemon();
    std::string httpd_option_listen = getBindAddr();
    int httpd_option_port = getListenPort();
    int httpd_option_daemon = isDaemon();
    int httpd_option_timeout = getTimeOut();

    event_init();
    struct evhttp *httpd;
	registerHTTPHandler("/encodeNumber",encodeNumber);
    registerHTTPHandler("/getSecret",getSecret);
    registerHTTPHandler("/createFundTx",createFundTx);
    registerHTTPHandler("/getFundTx",getFundTx);
    registerHTTPHandler("/signFundTx",signFundTx);
    registerHTTPHandler("/anounceSecret",anounceSecret);
    registerHTTPHandler("/getNum",getNum);

    httpd = evhttp_start(httpd_option_listen.c_str(), httpd_option_port);
    if(!httpd)
    {
        LOG(ERROR) << "http start error";
        return -1;
    }

    evhttp_set_allowed_methods(httpd, EVHTTP_REQ_GET | EVHTTP_REQ_POST |  EVHTTP_REQ_HEAD | EVHTTP_REQ_PUT | EVHTTP_REQ_OPTIONS);
    evhttp_set_timeout(httpd, httpd_option_timeout);
    evhttp_set_gencb(httpd, httpRequestCb, nullptr);

    event_dispatch();
    evhttp_free(httpd);
    LOG(INFO)  << "---  stop server  ---";
    return 0;
}
