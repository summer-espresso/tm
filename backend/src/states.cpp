#include "states.hpp"
#include "globals.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <mutex>
#include <fstream>
#include <sstream>

namespace glo {
std::string default_states;
};

#define RETURN_RESULT \
	std::ostringstream txt; \
	txt << res; \
	return txt.str();

#define CHECK_TOPIC \
	std::string err_check = check_topic(topic); \
	if (!err_check.empty()) \
	{ \
		res["status"] = "error"; \
		res["error"] = err_check; \
		RETURN_RESULT; \
	}

std::mutex state_mutex;
void broadcast_all(const char * buf);

std::string store_state(const std::string & topic, const std::string & body)
{
	const std::lock_guard<std::mutex> lock(state_mutex);
	Json::Value res;
	Json::Value node = decode_json(body);

	CHECK_TOPIC;

	const std::string state_path = glo::default_states + "/" + topic;

	std::ofstream ofs (state_path);
	if (ofs.is_open())
	{
		ofs << body;
		ofs.close();
		res["status"] = "ok";
		broadcast_all((std::string("state:store|") + topic).c_str());
		RETURN_RESULT;
	}
	else
	{
		res["status"] = "error";
		res["error"] = "unable to open file " + state_path + " for writing";
		RETURN_RESULT;
	}
}

std::string read_state(const std::string & topic)
{
	const std::lock_guard<std::mutex> lock(state_mutex);
	Json::Value res;

	CHECK_TOPIC;

	return load_content(glo::default_states + "/" + topic);
}

std::string delete_state(const std::string & topic)
{
	const std::lock_guard<std::mutex> lock(state_mutex);
	Json::Value res;

	CHECK_TOPIC;

	unlink((glo::default_states + "/" + topic).c_str());
	broadcast_all((std::string("state:delete|") + topic).c_str());
	return "";
}

std::string list_state()
{
	Json::Value res;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(glo::default_states.c_str())) != NULL)
	{
		// Effectuer un tri naturel
		std::vector<std::string> dir_list;
		while ((ent = readdir(dir)) != NULL)
		{
			if (ent->d_type == DT_REG)
			{
				std::string dirname = ent->d_name;
				if ((dirname != ".") && (dirname != ".."))
				{
					dir_list.push_back(ent->d_name);
				}
			}
		}

		closedir(dir);

		std::sort(dir_list.begin(), dir_list.end());

		Json::Value value;

		for (std::string & d : dir_list)
		{
			value["name"] = d;
			res.append(value);
		}
	}

	RETURN_RESULT;
}

