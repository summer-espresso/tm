#include "executor.hpp"
#include "globals.hpp"
#include "helpers.hpp"
#include "task_manager.hpp"

#include <crow/logging.h>
#include <json/json.h>

#include <fstream>
#include <iostream>
#include <vector>

#include <stdio.h>

void send_all(const std::string &, const char *);
void clear_all(const std::string &);
void broadcast_all(const char * buf);

const std::string execute_task(task_manager_t * task_manager, const std::string & task_path, int job_number)
{
	std::string result;
	int exit_code = 0;
	const int BUF_SIZE = 1024;
	static thread_local char buf[BUF_SIZE];
	FILE *fp = nullptr;
	const std::string cmd_path = escape_filename(glo::default_tasks + task_path + "/run.sh") + " 2>&1";
	const std::string log_path = glo::default_jobs + task_path + "/" + std::to_string(job_number) + "/output.log";

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
