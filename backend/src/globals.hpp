#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <string>

extern const char * STATUS_FILENAME;

namespace glo {

extern int port;
extern int kill_signal;

extern std::string title;
extern std::string storage;
extern std::string default_tasks;
extern std::string default_jobs;

extern const size_t PREFIX_LEN;
extern const char * PREFIX_CLIENT_CLOSE;
extern const char * PREFIX_SUBSCRIBE;
extern const char * PREFIX_UNSUBSCRIBE;

}; // glo

#endif // GLOBALS_HPP
