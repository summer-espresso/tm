#ifndef STATES_HPP
#define STATES_HPP

#include <string>

namespace glo {
extern std::string default_states;
};

std::string store_state(const std::string & topic, const std::string & body);
std::string read_state(const std::string & topic);
std::string delete_state(const std::string & topic);
std::string list_state();

#endif // STATES_HPP
