
#ifndef __MANUS_SERVER
#define __MANUS_SERVER

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <memory>

#include "mongoose.h"

#ifndef DEFAULT_LISTENING_PORT
#define DEFAULT_LISTENING_PORT 8080
#endif

using namespace std;

class Server;
class Handler;

class Request {
friend Server;
public:
    ~Request();

    void set_status(int status_code);
    void set_header(const string& name, const string& value);

    string get_header(const string& name) const;

    void send_data(const string& str);
    void send_data(const void *data, int data_len);

    string get_variable(const string& name, const string& def=string()) const;
    bool has_variable(const string& name) const;

    void finish();
    bool is_finished();

    string get_uri();

protected:
    Request(shared_ptr<Handler> handler, struct mg_connection* connection, const string& uri, map<string, string> matches);
    shared_ptr<Handler> handler;
private:

    map<string, string> variables;

    struct mg_connection* connection;
    bool finished;

    string uri;
};

class Matcher {
public:
    Matcher(string tpl);
    ~Matcher();

    virtual bool matches(const string& uri, map<string, string>& variables) = 0;

protected:
    string tpl;

};

class PrefixMatcher : public Matcher {
public:
    PrefixMatcher(string tpl, string varname = "suffix");
    ~PrefixMatcher();

    virtual bool matches(const string& uri, map<string, string>& variables);

private:

    string varname;
};

class Handler {
public:
    Handler();
    ~Handler();

    virtual bool authenticate(const Request& request);
    virtual void handle(Request& request);

    virtual void open(const Request& request);
    virtual void close(const Request& request);

};

class Server {
public:

    Server(int port = DEFAULT_LISTENING_PORT);
    ~Server();

    int get_port();

    void wait(int timeout);

    void append_handler(shared_ptr<Matcher> matcher, shared_ptr<Handler> handler);
    void prepend_handler(shared_ptr<Matcher> matcher, shared_ptr<Handler> handler);

    void set_default_handler(shared_ptr<Handler> handler);

private:

    static int master_event_handler(struct mg_connection *conn, enum mg_event ev);
    int event_handler(struct mg_connection *conn, enum mg_event ev);

    vector<pair<shared_ptr<Matcher>, shared_ptr<Handler> > > handlers;
    shared_ptr<Handler> default_handler;

    struct mg_server *server;

};


#endif


