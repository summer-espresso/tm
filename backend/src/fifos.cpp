#include "fifos.hpp"
#include "globals.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <mutex>
#include <fstream>
#include <sstream>

namespace glo {
std::string default_fifos;
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

std::mutex fifo_mutex;
void broadcast_all(const char * buf);

std::string push_fifo(const std::string & topic, const std::string & body)
{
	const std::lock_guard<std::mutex> lock(fifo_mutex);
	Json::Value res;
	Json::Value node = decode_json(body);

	if (!topic.empty())
	{
		CHECK_TOPIC;

		const std::string topic_path = glo::default_fifos + "/" + topic;
		ensure_folder(topic_path);
		const std::string topic_path_index = glo::default_fifos + "/" + topic + "/next";
		if (!file_exists(topic_path_index))
		{
			std::ofstream ofs (topic_path_index);
			if (ofs.is_open())
			{
				ofs << "0";
				ofs.close();
			}
			else
			{
				res["status"] = "error";
				res["error"] = "unable to open file " + topic_path_index + " for writing";
				RETURN_RESULT;
			}
		}
		const std::string next = load_content(topic_path_index);
		int num = atoi(next.c_str());
		std::string fifo_path;

		for (fifo_path = topic_path + "/" + std::to_string(num) + ".json" ; file_exists(fifo_path) ; num++)
			;

		std::ofstream ofs (fifo_path);
		if (ofs.is_open())
		{
			ofs << body;
			ofs.close();
			res["status"] = "ok";
			res["num"] = std::to_string(num);

			std::ofstream ofs_next (topic_path_index);
			if (ofs_next.is_open())
			{
				ofs_next << (num + 1);
				ofs_next.close();
			}
			else
			{
				res["error"] = "unable to open file " + topic_path_index + " for writing";
			}

			broadcast_all((std::string("fifo:push|") + topic).c_str());
			RETURN_RESULT;
		}
		else
		{
			res["status"] = "error";
			res["error"] = "unable to open file " + fifo_path + " for writing";
			RETURN_RESULT;
		}
	}
	else
	{
		res["status"] = "error";
		res["error"] = "no topic found";
		RETURN_RESULT;
	}
}

std::string pull_fifo(const std::string & topic)
{
	const std::lock_guard<std::mutex> lock(fifo_mutex);
	Json::Value res;

	CHECK_TOPIC;

	const std::string topic_path = glo::default_fifos + "/" + topic;
	ensure_folder(topic_path);

	const std::string topic_path_next = glo::default_fifos + "/" + topic + "/next";
	const std::string next = load_content(topic_path_next);
	int num_next = atoi(next.c_str());

	const std::string topic_path_last = glo::default_fifos + "/" + topic + "/last";
	if (!file_exists(topic_path_last))
	{
		std::ofstream ofs (topic_path_last);
		if (ofs.is_open())
		{
			ofs << "-1";
			ofs.close();
		}
		else
		{
			res["status"] = "error";
			res["error"] = "unable to open file " + topic_path_last + " for writing";
			RETURN_RESULT;
		}
	}
	const std::string last = load_content(topic_path_last);
	if (last.empty())
	{
		res["status"] = "none";
		RETURN_RESULT;
	}

	int num_last = atoi(last.c_str());

	if ((num_last + 1) >= num_next)
	{
		res["status"] = "none";
		res["num"] = "";
		RETURN_RESULT;
	}

	if ((num_next - num_last + 1) > MAX_NB_FILE)
	{
		res["status"] = "error";
		res["error"] = "maximum number of files reached (" + std::to_string(MAX_NB_FILE) + ")";
		RETURN_RESULT;
	}

	num_last++;

	std::string fifo_path;

	for (fifo_path = topic_path + "/" + std::to_string(num_last) + ".json" ; !file_exists(fifo_path) && (num_last < num_next); num_last++)
		;

	if ((num_last + 1) >= num_next)
	{
		num_last = -1;
		num_next = 0;
		std::ofstream ofs (topic_path_next);
		if (ofs.is_open())
		{
			ofs << num_next;
			ofs.close();
		}
		else
		{
			res["status"] = "error";
			res["error"] = "unable to open file " + topic_path_next + " for writing";
		}
	}

	std::ofstream ofs (topic_path_last);
	if (ofs.is_open())
	{
		ofs << num_last;
		ofs.close();

		std::string content = load_content(fifo_path);
		unlink(fifo_path.c_str());
		broadcast_all((std::string("fifo:pull|") + topic).c_str());
		return content;
	}
	else
	{
		res["status"] = "error";
		res["error"] = "unable to open file " + topic_path_last + " for writing";
	}

	RETURN_RESULT;
}

std::string delete_fifo(const std::string & topic)
{
	const std::lock_guard<std::mutex> lock(fifo_mutex);
	Json::Value res;

	CHECK_TOPIC;

	const std::string cmd = "rm -fR " + enquote_filename(glo::default_fifos + "/" + topic);
	int ret_val = system(cmd.c_str());

	broadcast_all((std::string("fifo:delete|") + topic).c_str());
	return "{ \"result\": \"" + std::to_string(ret_val) + "\" }";
}

std::string list_fifo()
{
	Json::Value res;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(glo::default_fifos.c_str())) != NULL)
	{
		// Effectuer un tri naturel
		std::vector<std::string> dir_list;
		while ((ent = readdir(dir)) != NULL)
		{
			if (ent->d_type == DT_DIR)
			{
				std::string dirname = ent->d_name;
				if ((dirname != ".") && (dirname != ".."))
				{
					dir_list.push_back(ent->d_name);
				}
			}
		}

		std::sort(dir_list.begin(), dir_list.end());

		Json::Value value;

		for (std::string & d : dir_list)
		{
			value["last"] = load_content(glo::default_fifos + "/" + d + "/last");
			value["next"] = load_content(glo::default_fifos + "/" + d + "/next");
			value["name"] = d;
			res.append(value);
		}

		closedir(dir);
	}

	RETURN_RESULT;
}

