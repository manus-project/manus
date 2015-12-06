#undef UNICODE                    // Use ANSI WinAPI functions
#undef _UNICODE                   // Use multibyte encoding on Windows
#define _MBCS                     // Use multibyte encoding on Windows
#define _WIN32_WINNT 0x500        // Enable MIIM_BITMAP
#define _CRT_SECURE_NO_WARNINGS   // Disable deprecation warning in VS2005
#define _XOPEN_SOURCE 600         // For PATH_MAX on linux
#undef WIN32_LEAN_AND_MEAN        // Let windows.h always include winsock2.h

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include <iostream>

#include "server.h"

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>  // For chdir()
    #include <winsvc.h>
    #include <shlobj.h>

    #ifndef PATH_MAX
    #define PATH_MAX MAX_PATH
    #endif

    #ifndef S_ISDIR
    #define S_ISDIR(x) ((x) & _S_IFDIR)
    #endif

    #define DIRSEP '\\'
    #define snprintf _snprintf
    #define vsnprintf _vsnprintf
    #define sleep(x) Sleep((x) * 1000)
    #define abs_path(rel, abs, abs_size) _fullpath((abs), (rel), (abs_size))
    #define SIGCHLD 0
    typedef struct _stat file_stat_t;
    #define stat(x, y) _stat((x), (y))
#else
    typedef struct stat file_stat_t;
    #include <sys/wait.h>
    #include <unistd.h>

    #define DIRSEP '/'
    #define __cdecl
    #define abs_path(rel, abs, abs_size) realpath((rel), (abs))
#endif // _WIN32

static char *sdup(const char *str) {
  char *p;
  if ((p = (char *) malloc(strlen(str) + 1)) != NULL) {
    strcpy(p, str);
  }
  return p;
}

static int is_path_absolute(const char *path) {
#ifdef _WIN32
  return path != NULL &&
    ((path[0] == '\\' && path[1] == '\\') ||  // UNC path, e.g. \\server\dir
     (isalpha(path[0]) && path[1] == ':' && path[2] == '\\'));  // E.g. X:\dir
#else
  return path != NULL && path[0] == '/';
#endif
}

static int path_exists(const char *path, int is_dir) {
  file_stat_t st;
  return path == NULL || (stat(path, &st) == 0 &&
                          ((S_ISDIR(st.st_mode) ? 1 : 0) == is_dir));
}

Request::Request(shared_ptr<Handler> handler, struct mg_connection* connection, const string& uri, map<string, string> matches) : handler(handler), connection(connection), finished(false), uri(uri), variables(matches) {

}

Request::~Request() {

}

void Request::set_status(int status_code) {
    if (!finished)
        mg_send_status(connection, status_code);
}

void Request::set_header(const string& name, const string& value) {
    if (!finished)
        mg_send_header(connection, name.c_str(), value.c_str());
}

string Request::get_header(const string& name) const {

    return string(mg_get_header(connection, name.c_str()));

}

void Request::send_data(const void *data, int data_len) {
     if (!finished)
        mg_send_data(connection, data, data_len);
}

void Request::send_data(const string& str) {
     if (!finished)
        mg_send_data(connection, str.c_str(), str.size());
}

string Request::get_variable(const string& name) const {

    map<string, string>::const_iterator it;
    it = variables.find(name);
    if (it != variables.end())
        return variables.at(name);

    size_t buffer_length = 1024;
    char* buffer = (char*) malloc(buffer_length);

    int result = mg_get_var(connection, name.c_str(), buffer, buffer_length);

    if (result >= 0) {
        string value(buffer);
        free(buffer);
        return value;
    }

    free(buffer);
    return string();

}

bool Request::has_variable(const string& name) const {

    map<string, string>::const_iterator it;
    it = variables.find(name);
    if (it != variables.end())
        return true;

    char buffer[1];

    int result = mg_get_var(connection, name.c_str(), buffer, 0);

    return result != -1;
    
}

void Request::finish() {
    finished = true;
}

bool Request::is_finished() {
    return finished;
}

string Request::get_uri() {
    return uri;
}

Matcher::Matcher(string tpl) : tpl(tpl) {}
Matcher::~Matcher() {}

bool Matcher::matches(const string& uri, map<string, string>& variables) {

    return uri == tpl;
}

PrefixMatcher::PrefixMatcher(string tpl, string varname) : Matcher(tpl), varname(varname) {}
PrefixMatcher::~PrefixMatcher() {}

bool PrefixMatcher::matches(const string& uri, map<string, string>& variables) {

    if (tpl.size() > uri.size())
        return false;


    if (uri.compare(0, tpl.size(), tpl) == 0) {
        variables[varname] = uri.substr(tpl.size());
        return true;
    }

    return false;

}

Handler::Handler() {

};

Handler::~Handler() {

};

bool Handler::authenticate(const Request& request) {

    return true;

}

void Handler::handle(Request& request) {

}

void Handler::open(const Request& request) {

}

void Handler::close(const Request& request) {

}

int Server::master_event_handler(struct mg_connection *conn, enum mg_event ev) {

    if (!conn->server_param)
        return MG_FALSE;

    return ((Server*) conn->server_param)->event_handler(conn, ev);

}

int Server::event_handler(struct mg_connection *conn, enum mg_event ev) {

    Request* request = NULL;
//printf("'%s' %d %d %d\n", conn->uri, ev, conn->connection_param, conn->remote_port);
    if (conn->connection_param) {

        request = (Request*)conn->connection_param;

    } else {

        if (ev == MG_POLL || ev == MG_CLOSE) return MG_FALSE;

        string uri(conn->uri);

        shared_ptr<Handler> handler = default_handler;

        map<string, string> matches;

        for (std::vector<pair<shared_ptr<Matcher>, shared_ptr<Handler> > >::iterator it = handlers.begin() ; it != handlers.end(); ++it) {
            matches.clear();

            if (!(*it).first->matches(conn->uri, matches))
                continue;

            handler = (*it).second;

            break;
        }

        if (handler) {

            request = new Request(handler, conn, uri, matches);
            request->handler->open(*request);
            conn->connection_param = request;

        }

    }

    if (!request) {
        //mg_send_status(conn, 404);
        return MG_FALSE;
    }

    switch (ev) {
    case MG_AUTH: {
        if (request->handler->authenticate(*request))
            return MG_TRUE;
        else
            return MG_FALSE;
    }
    case MG_REQUEST: {
        request->handler->handle(*request);
        if (request->is_finished()) {
            request->handler->close(*request);
            conn->connection_param = NULL;
            delete request;
            return MG_TRUE;
        } else
            return MG_MORE;
    }
    case MG_POLL: {
        if (request->is_finished())
            return MG_FALSE;
        else
            return MG_TRUE;
    }
    case MG_CLOSE: {
        request->handler->close(*request);
        conn->connection_param = NULL;
        delete request;
        break;
    }
    default: {
        break;
    }
    }

    return MG_FALSE;
}

Server::Server(int port) {
    server = mg_create_server(this, master_event_handler);

    default_handler = NULL;

    assert(server != NULL);

    #ifdef SERVER_DOCUMENT_ROOT
        // Make sure we have absolute paths for files and directories
        // https://github.com/valenok/mongoose/issues/181
        //set_absolute_path(options, "document_root");

        assert(path_exists(SERVER_DOCUMENT_ROOT, 1));

        mg_set_option(server, "document_root", SERVER_DOCUMENT_ROOT);
        // Change current working directory to document root. This way,
        // scripts can use relative paths.
        chdir(mg_get_option(server, "document_root"));

    #endif

    char buffer[128];

    sprintf(buffer, "%d", port);

    mg_set_option(server, "listening_port", buffer);

}

Server::~Server() {
    if (server) mg_destroy_server(&server);
}

void Server::wait(int timeout) {
    mg_poll_server(server, timeout);
}

int Server::get_port() {
    return atoi(mg_get_option(server, "listening_port"));
}


void Server::append_handler(shared_ptr<Matcher> matcher, shared_ptr<Handler> handler) {

    handlers.push_back(std::make_pair(matcher, handler));
}

void Server::prepend_handler(shared_ptr<Matcher> matcher, shared_ptr<Handler> handler) {

    handlers.insert (handlers.begin(), std::make_pair(matcher, handler));

}

void Server::set_default_handler(shared_ptr<Handler> handler) {

    default_handler = handler;

}

