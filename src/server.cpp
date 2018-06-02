
#include "common.h"
#include "server.h"
#include <sys/time.h>
#include <unistd.h>

std::vector<HTTPPathHandler> pathHandlers;

HTTPRequest::HTTPRequest(struct evhttp_request* _req) : req(_req){}
HTTPRequest::~HTTPRequest()
{
    LOG(INFO) << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"  ;
}

HTTPRequest::RequestMethod HTTPRequest::GetRequestMethod()
{
    switch (evhttp_request_get_command(req)) {
    case EVHTTP_REQ_GET:
        return GET;
        break;
    case EVHTTP_REQ_POST:
        return POST;
        break;
    case EVHTTP_REQ_HEAD:
        return HEAD;
        break;
    case EVHTTP_REQ_PUT:
        return PUT;
        break;
	case EVHTTP_REQ_OPTIONS:
        return OPTIONS;
        break;
    default:
        return UNKNOWN;
        break;
    }
}

static std::string RequestMethodString(HTTPRequest::RequestMethod m)
{
    switch (m) {
    case HTTPRequest::GET:
        return "GET";
        break;
    case HTTPRequest::POST:
        return "POST";
        break;
    case HTTPRequest::HEAD:
        return "HEAD";
        break;
    case HTTPRequest::PUT:
        return "PUT";
        break;
    case HTTPRequest::OPTIONS:
        return "OPTIONS";
        break;
    default:
        return "unknown";
    }
}

void registerHTTPHandler(const std::string &prefix, const HTTPRequestHandler &handler)
{
    LOG(INFO) << "Registering HTTP handler for " << prefix;

    pathHandlers.push_back(HTTPPathHandler(prefix, handler));
}

std::string HTTPRequest::GetURI()
{
    return evhttp_request_get_uri(req);
}
std::string HTTPRequest::GetHeader()
{
    std::string urlheader;
    struct evkeyvalq *headers;
    struct evkeyval *header;
    headers = evhttp_request_get_input_headers(req);

    for (header = headers->tqh_first; header;header = header->next.tqe_next)
    {
        urlheader = urlheader + header->key + " : " + header->value + "\n";
    }

    return urlheader;
}


void HTTPRequest::GetPeer()
{
    evhttp_connection* con = evhttp_request_get_connection(req);
    if (con)
    {
        const char* address = "";
        uint16_t port = 0;
        evhttp_connection_get_peer(con, (char**)&address, &port);
        LOG(INFO) << address << " : " << port;
        return;
    }

    LOG(INFO) << "GET_PEER_ERROR";
    return;
}


void HTTPRequest::WriteHeader(const std::string& hdr, const std::string& value)
{
    struct evkeyvalq* headers = evhttp_request_get_output_headers(req);
    assert(headers);
    evhttp_add_header(headers, hdr.c_str(), value.c_str());
}

void HTTPRequest::WriteReply(int nStatus, const std::string& strReply)
{
    assert(req);
    struct evbuffer* evb = evhttp_request_get_output_buffer(req);
    assert(evb);
    evbuffer_add(evb, strReply.data(), strReply.size());
    auto req_copy = req;

    evhttp_send_reply(req_copy, nStatus, nullptr, nullptr);
    if (event_get_version_number() >= 0x02010600 && event_get_version_number() < 0x02020001)
    {
       evhttp_connection* conn = evhttp_request_get_connection(req_copy);
       if (conn)
       {
           bufferevent* bev = evhttp_connection_get_bufferevent(conn);
           if (bev)
           {
               bufferevent_enable(bev, EV_READ | EV_WRITE);
           }
       }
    }
}

std::string HTTPRequest::ReadBody()
{
    struct evbuffer* buf = evhttp_request_get_input_buffer(req);
    if (!buf)
    {
        LOG(INFO) << "READ_BODY ERROR 1";
        return "";
    }

    size_t size = evbuffer_get_length(buf);

    const char* data = (const char*)evbuffer_pullup(buf, size);
    if (!data)
    {
        LOG(INFO) << "READ_BODY ERROR 2   " << size;
        return "";
    }
    std::string rv(data, size);
    evbuffer_drain(buf, size);

    LOG(INFO) << "READ_BODY : " << rv;
    return rv;
}
bool checkHash(const std::string &txid)
{
    return isHex(txid) && HAHS_SIZE == txid.length();
}

void httpRequestCb(struct evhttp_request *req, void *arg)
{

    if (event_get_version_number() >= 0x02010600 && event_get_version_number() < 0x02020001)
    {
        evhttp_connection* conn = evhttp_request_get_connection(req);
        if (conn)
        {
            bufferevent* bev = evhttp_connection_get_bufferevent(conn);
            if (bev)
            {
                bufferevent_disable(bev, EV_READ);
            }
        }
    }

    std::unique_ptr<HTTPRequest> hreq(new HTTPRequest(req));

    hreq->GetPeer();
    LOG(INFO) << "Received a " <<  RequestMethodString(hreq->GetRequestMethod()) << " request for " <<  hreq->GetURI() << " from ";

    if (hreq->GetRequestMethod() == HTTPRequest::UNKNOWN)
    {
        hreq->WriteReply(HTTP_BADMETHOD);
        return;
    }

	if (hreq->GetRequestMethod() == HTTPRequest::OPTIONS)
    {

		hreq->WriteHeader("Access-Control-Allow-Origin", "*");
		hreq->WriteHeader("Access-Control-Allow-Credentials", "true");
		hreq->WriteHeader("Access-Control-Allow-Headers", "access-control-allow-origin,Origin, X-Requested-With, Content-Type, Accept, Authorization");
		hreq->WriteReply(HTTP_OK);
        return ;
	}

    hreq->WriteHeader("Access-Control-Allow-Origin", "*");
    hreq->WriteHeader("Access-Control-Allow-Credentials", "true");
    hreq->WriteHeader("Access-Control-Allow-Headers", "access-control-allow-origin,Origin, X-Requested-With, Content-Type, Accept, Authorization");

    if (hreq->GetRequestMethod() != HTTPRequest::POST)
    {
        hreq->WriteReply(HTTP_BADMETHOD);
        return;
    }

    std::string strURI = hreq->GetURI();
    std::string path;
    std::vector<HTTPPathHandler>::const_iterator i = pathHandlers.begin();
    std::vector<HTTPPathHandler>::const_iterator iend = pathHandlers.end();
    for (; i != iend; ++i)
    {
        bool match = (strURI == i->prefix);
        if (match)
        {
            path = strURI;
            break;
        }
    }

    if(i != iend)
    {
        LOG(INFO) << "FOUND_PATH : " << path;
        i->handler(std::move(hreq));
    }
    else
    {
        LOG(INFO) << "NOT_FOUND_PATH : " <<  strURI;
        hreq->WriteReply(HTTP_NOTFOUND);
    }
}


void signalHandler(int sig)
{
    switch (sig)
    {
        case SIGTERM:
        case SIGHUP:
        case SIGQUIT:
        case SIGINT:
        {
            event_loopbreak();
        }
        break;
    }
}

void runDaemon(bool daemon)
{
    if (daemon) {
        pid_t pid;
        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
    }
}


bool contentToipfshash(const std::string &content, std::string &ipfsHash)
{
    char path[2048] = {0};
    strncpy(path, getenv("HOME"), strlen(getenv("HOME")));
    strcat(path, "/msg.txt");
    LOG(INFO) << "path is : "<< path;
    std::ofstream examplefile(path);
    examplefile << content;
    examplefile.close();

    std::string postUrlStr = "http://localhost:5001/api/v0/add";
    std::string postParams = "";
    std::string filePath(path);
    std::string postResponseStr;
    auto res = curl_post_req(postUrlStr, postParams, filePath, postResponseStr);
    if (res != CURLE_OK)
    {

        LOG(ERROR) << "curl post failed: " + std::string(curl_easy_strerror(res)) ;
    }
    else
    {
        int pos = postResponseStr.find("Hash");
        ipfsHash = postResponseStr.substr(pos + 7, pos + 27);
        LOG(INFO) << "createIpfsMsg is : "<< ipfsHash;
        return true;
    }
    return false;

}


size_t reqReply(void *ptr, size_t size, size_t nmemb, void *stream)
{
    std::string *str = (std::string*)stream;
    (*str).append((char*)ptr, size*nmemb);
    return size * nmemb;
}

bool curlBitcoinReq(const std::string &data,std::string &response)
{
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;
	CURLcode res;

	const std::string url = "http://127.0.0.1:8332";

    if (curl)
    {
		headers = curl_slist_append(headers, "content-type: text/plain;");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)data.size());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, reqReply);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

		curl_easy_setopt(curl, CURLOPT_USERPWD, "hello:helloworld");
		curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
		res = curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        LOG(ERROR) << "CURL_FAILED : " << curl_easy_strerror(res);
        return false;
    }
    LOG(INFO) << "CURL_RESULT : " << response;

    return true;
}

CURLcode curl_post_req(const std::string &url, const std::string &postParams, std::string &filepath, std::string &response)
{
    // init curl
    CURL *curl = curl_easy_init();
    // res code
    CURLcode res;
    if (curl)
    {
        // set params
        curl_easy_setopt(curl, CURLOPT_POST, 1); // post req
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // url
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postParams.c_str()); // params
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false); // if want to use https
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false); // set peer and host verify false
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);

        if (!filepath.empty()) {
            struct curl_httppost* post = NULL;
            struct curl_httppost* last = NULL;
            curl_formadd(&post, &last, CURLFORM_COPYNAME, "uploadfile", CURLFORM_FILE, filepath.c_str(), CURLFORM_END);
            curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
        }

        // start req
        res = curl_easy_perform(curl);
    }
    // release curl
    curl_easy_cleanup(curl);
    return res;
}

// reply of the requery
size_t req_reply(void *ptr, size_t size, size_t nmemb, void *stream)
{
    std::string *str = (std::string*)stream;
    (*str).append((char*)ptr, size*nmemb);
    return size * nmemb;
}


static int g_roomId =1;
struct UserInfo
{
	int uid;
	std::string secrect;
	std::string address;
    std::string txid;
    int vout;
    std::string amount;
	int num;
};
struct GameInfo
{	
    std::vector<UserInfo*>  user_group;
	int user_size;
    int vin_size;
    int anounce_size;
    std::string fund_tx;
	GameInfo()
	{
		user_size=0;
        vin_size=0;
        anounce_size=0;
	}
};

std::map<int ,GameInfo*>  g_mapGameInfo;

static  void setUserInfo(UserInfo*user_info,int uid,const std::string &secret,const std::string &address)
{
    user_info->address = address;
    user_info->secrect = secret;
    user_info->uid = uid;
}

static void createRoom(int&uid,int&roomid,const std::string &secret,const std::string &address)
{
    uid = 0;
    GameInfo* game_info = new GameInfo();
    UserInfo* user_info = new UserInfo();
    setUserInfo(user_info,uid,secret,address);
    game_info->user_group.push_back(user_info);
    game_info->user_size =1;
    g_mapGameInfo[g_roomId] = game_info;
    roomid = g_roomId;
    g_roomId++;
}

void encodeNumber(std::unique_ptr<HTTPRequest> req)
{
    try
    {
        std::string post_data = req->ReadBody();
		std::cout << "encodeNumber receive:"  <<  post_data << std::endl;
        auto jsonData = json::parse(post_data);

        if(!jsonData.is_object())
        {
            LOG(ERROR) << " encodeNumber  params error\n ";
            throw;
        }
		
       	std::string secret = jsonData["secret"].get<std::string>();
        std::cout << " secret is:  " << secret  <<std::endl;
        std::string address = jsonData["address"].get<std::string>();
		std::cout << "address is: " << address << std::endl;
		
        int roomid =-1;
        int uid = -1;
        std::map<int ,GameInfo*>::iterator iter = g_mapGameInfo.begin();
        if(iter == g_mapGameInfo.end())
        {
            createRoom(uid,roomid,secret,address);
        }
        else
        {
            bool has_match = false;
            for(;iter != g_mapGameInfo.end();++iter)
            {
                if(iter->second->user_size == 1)
                {
                    uid =1 ;
                    UserInfo* user_info = new UserInfo();
                    setUserInfo(user_info,uid,secret,address);
                    iter->second->user_group.push_back(user_info);
                    iter->second->user_size =2;
                    roomid = iter->first;
                    has_match =true;
                    break;
                }
            }

            if (!has_match)
            {
                createRoom(uid,roomid,secret,address);
            }
        }

        json response = json::object();
        response["roomid"] = roomid;
        response["uid"] = uid;
		
        std::string result = response.dump() ;
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK,result);
        return;

    }
    catch(...)
    {
        LOG(ERROR) << "  encodeNunber error: \n ";
    }

    req->WriteReply(HTTP_INTERNAL,ERROR_REQUEST);
}



void getSecret(std::unique_ptr<HTTPRequest> req)
{
    try
    {
	int ret_code=0;
        std::string post_data = req->ReadBody();
        std::cout << "getSecret receive:"  <<  post_data << std::endl;
        auto jsonData = json::parse(post_data);

        if(!jsonData.is_object())
        {
            LOG(ERROR) << " getSecret  params error\n ";
            throw;
        }

        std::string strReply;
        int  roomid = jsonData["roomid"].get<int> ();
        std::map<int ,GameInfo*>::iterator iter = g_mapGameInfo.find(roomid);
        if(iter != g_mapGameInfo.end())
        {
            if( g_mapGameInfo[roomid]->user_size == 1)
            {
                strReply =  "Maybe no user player with you!";
		ret_code=1;
            }
            else
            {
                json response = json::object();
                std::string reply_secret="secret";
                std::string reply_addres="address";
                for(int i =0;i<g_mapGameInfo[roomid]->user_group.size();i++)
                {
                   response[reply_secret + std::to_string(i)] = g_mapGameInfo[roomid]->user_group[i]->secrect;
                   response[reply_addres + std::to_string(i)] = g_mapGameInfo[roomid]->user_group[i]->address;
                }
                strReply = response.dump();
            }
        }
        else
        {
	    ret_code=2;
            strReply =  "No init!";

        }

        std::string result = makeReplyMsg(ret_code,strReply);
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK,result);
        return;

    }
    catch(...)
    {
        LOG(ERROR) << "  getSecret error: \n ";
    }

    req->WriteReply(HTTP_INTERNAL,ERROR_REQUEST);
}

void createFundTx(std::unique_ptr<HTTPRequest> req)
{
    try
    {
        std::string post_data = req->ReadBody();
        std::cout << "createFundTx receive:"  <<  post_data << std::endl;
        auto jsonData = json::parse(post_data);

        if(!jsonData.is_object())
        {
            LOG(ERROR) << " createFundTx  params error\n ";
            throw;
        }
        int roomid = jsonData["roomid"].get<int>();
        int uid = jsonData["uid"].get<int>();
        std::string txid = jsonData["txid"].get<std::string>();
        std::string amount = jsonData["amount"].get<std::string>();
        int vout = jsonData["vout"].get<int>();

	int ret_code = 0;
        std::string strReply;
        std::map<int ,GameInfo*>::iterator iter = g_mapGameInfo.find(roomid);
        if ( iter != g_mapGameInfo.end())
        {
            if(iter->second->user_size != 2)
            {
		ret_code =1;
                strReply = "No one palys with you!";
            }
            else
            {
                strReply ="OK";
                if(g_mapGameInfo[roomid]->vin_size != 2)
                    g_mapGameInfo[roomid]->vin_size++;
                g_mapGameInfo[roomid]->user_group[uid]->txid = txid;
                g_mapGameInfo[roomid]->user_group[uid]->amount = amount;
                g_mapGameInfo[roomid]->user_group[uid]->vout = vout;
            }
        }
        else
        {
	    ret_code=2;
            strReply = "No such roomid!";
        }
        std::string result = makeReplyMsg(ret_code,strReply);
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK,result);
    }
    catch(...)
    {
        LOG(ERROR) << "  createFundTx error: \n ";
    }

    req->WriteReply(HTTP_INTERNAL,ERROR_REQUEST);
}

void getFundTx(std::unique_ptr<HTTPRequest> req)
{
    try
    {
        std::string post_data = req->ReadBody();
        std::cout << "getFundTx receive:"  <<  post_data << std::endl;
        auto jsonData = json::parse(post_data);

        if(!jsonData.is_object())
        {
            LOG(ERROR) << " getFundTx  params error\n ";
            throw;
        }
	int ret_code=0;
        int roomid = jsonData["roomid"].get<int>();
        std::map<int ,GameInfo*>::iterator iter = g_mapGameInfo.find(roomid);
        std::string strReply;
        if ( iter != g_mapGameInfo.end())
        {
            if(iter->second->vin_size != 2)
            {
		ret_code=1;
                strReply = "No one palys agree you!";
            }
            else
            {
                json response = json::object();
                std::string txid="txid";
                std::string vout="vout";
                std::string amount = "amount";
		
                for(int i =0;i<g_mapGameInfo[roomid]->user_group.size();i++)
                {
                   response[txid + std::to_string(i)] = g_mapGameInfo[roomid]->user_group[i]->txid;
                   response[amount + std::to_string(i)] = g_mapGameInfo[roomid]->user_group[i]->amount;
                   response[vout + std::to_string(i)] = g_mapGameInfo[roomid]->user_group[i]->vout;
                }
		double amount0 = atof(g_mapGameInfo[roomid]->user_group[0]->amount.c_str());
		double amount1 = atof(g_mapGameInfo[roomid]->user_group[1]->amount.c_str());
                double  changle = amount0 - amount1;
                if( changle == 0.0)
                {
                    response["changeAddress"] = "";
                    response["change"] = "";
            response["scriptAmount"] = std::to_string(amount0*2 - 0.01);
		    
                }
                else if( changle > 0.0 )
                {
                    response["changeAddress"] = g_mapGameInfo[roomid]->user_group[0]->address;
                    response["change"] = std::to_string(changle);
            response["scriptAmount"] = std::to_string(amount1*2 - 0.01);
                }
                else
                {
                    changle = -changle;
                    response["changeAddress"] = g_mapGameInfo[roomid]->user_group[0]->address;
                    response["change"] = std::to_string(changle);
            response["scriptAmount"] = std::to_string(amount0*2 - 0.01);
                }
		
		

                response["hexTx"] = g_mapGameInfo[roomid]->fund_tx;
                strReply = response.dump();
            }
        }
        else
        {
	    ret_code=2;
            strReply = "No such roomid!";
        }

        std::string result = makeReplyMsg(ret_code,strReply);
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK,result);

    }
    catch(...)
    {
        LOG(ERROR) << "  getFundTx error: \n ";
    }

    req->WriteReply(HTTP_INTERNAL,ERROR_REQUEST);

}

void anounceSecret(std::unique_ptr<HTTPRequest> req)
{
    try
    {
        std::string post_data = req->ReadBody();
        std::cout << "anounceSecret receive:"  <<  post_data << std::endl;
        auto jsonData = json::parse(post_data);

        if(!jsonData.is_object())
        {
            LOG(ERROR) << " anounceSecret  params error\n ";
            throw;
        }

        int roomid = jsonData["roomid"].get<int>();
        int num = jsonData["num"].get<int>();
        int uid = jsonData["uid"].get<int>();
        std::map<int ,GameInfo*>::iterator iter = g_mapGameInfo.find(roomid);
        std::string strReply;
	int ret_code =0;
        if ( iter != g_mapGameInfo.end())
        {
            if(iter->second->user_size != 2)
            {
		ret_code=1;
                strReply = "No one palys with you!";
            }
            else
            {
                strReply = "OK!";
                if(g_mapGameInfo[roomid]->anounce_size !=2)
                    g_mapGameInfo[roomid]->anounce_size++;
                g_mapGameInfo[roomid]->user_group[uid]->num = num;
            }
        }
        else
        {	
            ret_code=2;
            strReply = "No such roomid!";
        }
        std::string result = makeReplyMsg(ret_code,strReply);
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK,result);
        return;
    }
    catch(...)
    {
        LOG(ERROR) << "  anounceSecret error: \n ";
    }

    req->WriteReply(HTTP_INTERNAL,ERROR_REQUEST);
}


void getNum(std::unique_ptr<HTTPRequest> req)
{
    try
    {
        std::string post_data = req->ReadBody();
        std::cout << "getNum receive:"  <<  post_data << std::endl;
        auto jsonData = json::parse(post_data);
        if(!jsonData.is_object())
        {
            LOG(ERROR) << " getNum  params error\n ";
            throw;
        }
        int roomid = jsonData["roomid"].get<int>();
        std::map<int ,GameInfo*>::iterator iter = g_mapGameInfo.find(roomid);
        std::string strReply;
	int ret_code = 0;
        if ( iter != g_mapGameInfo.end())
        {
            if(iter->second->anounce_size != 2)
            {
		ret_code = 1;
                strReply = "No one palys with you!";
            }
            else
            {
                strReply = "OK!";
                json response = json::object();
                std::string secret="secret";
                for(int i =0;i<g_mapGameInfo[roomid]->user_group.size();i++)
                {
                   response[secret + std::to_string(i)] = g_mapGameInfo[roomid]->user_group[i]->num;
                }

                strReply = response.dump();
            }
        }
        else
        {
	    ret_code = 2;
            strReply = "No such roomid!";
        }
        std::string result = makeReplyMsg(ret_code,strReply);
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK,result);
        return;
    }
    catch(...)
    {
        LOG(ERROR) << "  getNum error: \n ";
    }

    req->WriteReply(HTTP_INTERNAL,ERROR_REQUEST);
}

void signFundTx(std::unique_ptr<HTTPRequest> req)
{
    try
        {
            std::string post_data = req->ReadBody();
            std::cout << "signFundTx receive:"  <<  post_data << std::endl;
            auto jsonData = json::parse(post_data);

            if(!jsonData.is_object())
            {
                LOG(ERROR) << " signFundTx  params error\n ";
                throw;
            }

            int roomid = jsonData["roomid"].get<int>();
            std::string hexTx = jsonData["hex"].get<std::string>();
            std::map<int ,GameInfo*>::iterator iter = g_mapGameInfo.find(roomid);
            std::string strReply;
            int ret_code =0 ;
            if ( iter != g_mapGameInfo.end())
            {
                if(iter->second->user_size != 2)
                {
                    ret_code =1;
                    strReply = "No one palys with you!";
                }
                else
                {
                    strReply = "OK!";
                    g_mapGameInfo[roomid]->fund_tx = hexTx;
                }
            }
            else
            {
		ret_code = 2;
                strReply = "No such roomid!";
            }
            std::string result = makeReplyMsg(ret_code,strReply);
            req->WriteHeader("Content-Type", "application/json");
            req->WriteReply(HTTP_OK,result);
            return;
        }
        catch(...)
        {
            LOG(ERROR) << "  signFundTx error: \n ";
        }
        req->WriteReply(HTTP_INTERNAL,ERROR_REQUEST);
}

static void releaseRoom(int room_id)
{
     std::map<int ,GameInfo*>::iterator iter = g_mapGameInfo.find(room_id);

     if ( iter != g_mapGameInfo.end() )
     {
         for ( int i =0 ; i<g_mapGameInfo[room_id]->user_group.size() ;++i )
         {
             delete g_mapGameInfo[room_id]->user_group[i];
         }

         delete g_mapGameInfo[room_id];
         g_mapGameInfo.erase(room_id);
     }

}
