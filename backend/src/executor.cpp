#include "executor.hpp"
#include "globals.hpp"
#include "helpers.hpp"
#include "task_manager.hpp"

#include <crow/logging.h>
#include <json/json.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include <stdio.h>

void send_all(const std::string &, const char *);
void clear_all(const std::string &);
void broadcast_all(const char * buf);

void purge_log(const std::string log_base, int max_log_jobs)
{
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(log_base.c_str())) != NULL)
	{
		std::vector<int> element_list;

		while ((ent = readdir(dir)) != NULL)
		{
			if (ent->d_type == DT_DIR)
			{
				std::string dirname = ent->d_name;
				if ((dirname != ".") && (dirname != ".."))
				{
					element_list.push_back(atoi(dirname.c_str()));
				}
			}
		}

		std::sort(element_list.rbegin(), element_list.rend());

		std::string log_path;
		for (int num : element_list)
		{
			if (max_log_jobs > 0)
			{
				max_log_jobs--;
				continue;
			}
			log_path = log_base + std::to_string(num);
			const std::string cmd = "rm -fR " + escape_filename(log_path);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
			(void) system(cmd.c_str());
#pragma GCC diagnostic pop
		}

		closedir(dir);
	}
}

const std::string execute_task(task_manager_t * task_manager, const std::string & task_path, int job_number)
{
	std::string result;
	int exit_code = 0;
	const int BUF_SIZE = 1024;
	static thread_local char buf[BUF_SIZE];
	FILE *fp = nullptr;
	const std::string base_path = glo::default_tasks + task_path;
	const std::string cmd_path = escape_filename(base_path) + "/run.sh 2>&1";
	const std::string conf_path = base_path + "/task.json";
	const std::string log_base = glo::default_jobs + task_path + "/";
	const std::string log_path = log_base + std::to_string(job_number) + "/output.log";

	int max_log_jobs = -1;

	if (file_exists(conf_path))
	{
		Json::Value conf = load_json(conf_path);
		if (conf.isMember("max_log_jobs") && conf["max_log_jobs"].isInt())
		{
			max_log_jobs = conf["max_log_jobs"].asInt();
		}
	}

	auto log_cache = std::make_shared<vec_str_t>();
	task_manager->set_log(task_path, log_cache);

	int position = 0;

	std::ofstream ofs(log_path);
	if (ofs.is_open())
	{
		if ((fp = popen(cmd_path.c_str(), "r")) != nullptr)
		{
			task_manager->set_pid(task_path, diff_children(task_path));

			auto mtx = task_manager->get_mtx(task_path);
			auto cov = task_manager->get_cov(task_path);
			auto cpt = task_manager->get_cpt(task_path);
			auto stk = task_manager->get_stk(task_path);

			std::string text;

			while (fgets(buf, BUF_SIZE, fp) != nullptr)
			{
				// file
				ofs << buf << std::flush;
				// memory
				log_cache->push_back(buf);
				// stdout
				position++;
				std::unique_lock<std::mutex> lck(*mtx.get());
				(*cpt)++;
				cov->notify_all();
				text = "data:";
				text += buf;
			}
			exit_code = WEXITSTATUS(pclose(fp));
			std::unique_lock<std::mutex> lck(*mtx.get());
			// Stop Token
			*stk = true;
			cov->notify_all();
		}
		else
		{
			exit_code = errno;
			clear_all(task_path);
		}
		ofs.close();
		broadcast_all("stop:");

		const std::string cmd = "ansi2html -n < " + escape_filename(log_path) + " > " + escape_filename(log_path + ".html");
		int res = system(cmd.c_str());
		if (res != 0)
		{
			CROW_LOG_WARNING << "unable to execute ansi2html exit code " << res;
		}

		if (max_log_jobs > -1)
		{
			purge_log(log_base, max_log_jobs);
		}
	}
	else
	{
		CROW_LOG_ERROR << "error opening " << log_path;
		result = "error";
	}

	task_manager->del_log(task_path);
	task_manager->signal_end(task_path, exit_code);

	return result;
}
