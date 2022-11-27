#include "task_manager.hpp"
#include "executor.hpp"
#include "globals.hpp"
#include "helpers.hpp"

#include <iostream>
#include <mutex>
#include <thread>

#include <sys/types.h>
#include <signal.h>
#include <time.h>

std::mutex tm_mutex;

// https://programmer.group/c-std-condition_variable-wait-wait_for-is-different-from-how-to-use-instances.html

int task_manager_t::start(const std::string & task_path)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	if (running_tasks.find(task_path) != running_tasks.end())
	{
		return running_tasks[task_path]["job_number"].asInt();
	}

	const std::string data_path = glo::default_jobs + task_path;
	int res = ensure_folder(data_path);
	if (res != 0)
	{
		return -res;
	}
	const std::string data_json_status_path = data_path + STATUS_FILENAME;
	Json::Value last_status = load_json(data_json_status_path);
	int job_number = 0;
	if (last_status.isMember("job_number"))
	{
		job_number = last_status["job_number"].asInt() + 1;
	}
	last_status["job_number"] = job_number;

	const std::string data_jobs_path = data_path + "/" + std::to_string(job_number);
	res = ensure_folder(data_jobs_path);
	if (res != 0)
	{
		return -res;
	}

	Json::Value task;
	task["command"] = task_path;
	task["status"] = "running";
	task["start_date"] = (Json::Value::Int64)time(0);
	task["job_number"] = job_number;

	save_json(data_json_status_path, task);
	save_json(data_jobs_path + STATUS_FILENAME, task);

	running_tasks[task_path] = task;

	std::thread th = std::thread(execute_task, this, task_path, job_number);
	running_tasks[task_path]["thread_id"] = (Json::Value::UInt64)th.native_handle();
	th.detach();

	return job_number;
}

Json::Value task_manager_t::stop(const std::string & task_path)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	Json::Value task;
	if (running_tasks.find(task_path) == running_tasks.end())
	{
		return task;
	}

	task = running_tasks[task_path];

	int res = 0;
	for (pid_t pid : map_pid[task_path])
	{
		res += kill(pid, glo::kill_signal);
	}

	task["kill_date"] = (Json::Value::Int64)time(0);
	task["kill_signal"] = glo::kill_signal;
	task["kill_result"] = res;

	return task;
}

void task_manager_t::signal_end(const std::string & task_path, int exit_code)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	if (running_tasks.find(task_path) != running_tasks.end())
	{
		Json::Value task = running_tasks[task_path];
		task["status"] = exit_code == 0 ? "finished" : "failed";
		task["end_date"] = (Json::Value::Int64)time(0);
		task["exit_code"] = exit_code;

		const std::string data_jobs_path = glo::default_jobs + task_path;
		save_json(data_jobs_path + STATUS_FILENAME, task);
		save_json(data_jobs_path + "/" + std::to_string(task["job_number"].asInt()) + STATUS_FILENAME, task);

		running_tasks.erase(task_path);
	}
}

std::shared_ptr<vec_str_t> task_manager_t::set_log(const std::string task_path)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	auto log_cache = std::make_shared<vec_str_t>();
	map_log[task_path] = log_cache;
	auto cv = std::make_shared<std::condition_variable>();
	map_cov[task_path] = cv;
	auto mtx = std::make_shared<std::mutex>();
	map_mtx[task_path] = mtx;
	auto cpt = std::make_shared<int>(0);
	map_cpt[task_path] = cpt;
	return log_cache;
}

const std::shared_ptr<int> task_manager_t::get_cpt(const std::string task_path)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	return map_cpt.find(task_path) != map_cpt.end() ? map_cpt[task_path] : nullptr;
}

const std::shared_ptr<vec_str_t> task_manager_t::get_log(const std::string task_path)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	return map_log.find(task_path) != map_log.end() ? map_log[task_path] : nullptr;
}

const std::shared_ptr<std::condition_variable> task_manager_t::get_cov(const std::string task_path)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	return map_cov.find(task_path) != map_cov.end() ? map_cov[task_path] : nullptr;
}

const std::shared_ptr<std::mutex> task_manager_t::get_mtx(const std::string task_path)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	return map_mtx.find(task_path) != map_mtx.end() ? map_mtx[task_path] : nullptr;
}

void task_manager_t::del_log(const std::string task_path)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	map_cpt.erase(task_path);
	map_log.erase(task_path);
	map_cov.erase(task_path);
	map_mtx.erase(task_path);
}

void task_manager_t::set_pid(const std::string& task_path, vec_pid_t pid_list)
{
	const std::lock_guard<std::mutex> lock(tm_mutex);
	map_pid[task_path] = pid_list;
}
