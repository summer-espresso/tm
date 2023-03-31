#ifndef FIFOS_HPP
#define FIFOS_HPP

#include <string>

namespace glo {
extern std::string default_fifos;
};

std::string push_fifo(const std::string & topic, const std::string & body);
std::string pull_fifo(const std::string & topic);
std::string delete_fifo(const std::string & topic);
std::string list_fifo();

#endif // FIFOS_HPP
