#include "executor.hpp"
#include "globals.hpp"
#include "helpers.hpp"
#include "task_manager.hpp"

#include <crow.h>
#include <crow/middlewares/cors.h>
#include <curl/curl.h>

#include <algorithm>
#include <condition_variable>
#include <map>
#include <thread>

#include <json/json.h>
#include <unistd.h>

CURL * curl = nullptr;

using map_ws_t = std::map<std::string, std::map<crow::websocket::connection *, int>>;
using map_active_ws_t = std::map<crow::websocket::connection *, int>;
using map_ws_current_task_t = std::map<crow::websocket::connection *, size_t>;

map_active_ws_t map_active_ws;
map_ws_current_task_t map_ws_current_task;
map_ws_t map_ws;
std::mutex map_ws_mutex;

void broadcast_all(const char * buf)
{
	for (auto & v : map_active_ws)
	{
		v.first->send_text(buf);
	}
}

void broadcast_others(const char * buf, crow::websocket::connection * self_conn)
{
	for (auto & v : map_active_ws)
	{
		if (v.first != self_conn)
		{
			v.first->send_text(buf);
		}
	}
}

void send_output_stream(task_manager_t * task_manager, crow::websocket::connection * conn, const std::string & task_path)
{
	pthread_t tid = pthread_self();
	if (tid != map_ws_current_task[conn])
	{
		return;
	}

	auto log_cache = task_manager->get_log(task_path);
	auto mtx = task_manager->get_mtx(task_path);
	auto cov = task_manager->get_cov(task_path);
	auto cpt = task_manager->get_cpt(task_path);

	if (log_cache && mtx && cov && cpt)
	{
		int last_pos = 0;
		int max_pos = log_cache->size();
		std::string text;

		while (*cpt > -1)
		{
			if (tid != map_ws_current_task[conn])
			{
				return;
			}

			text = "data:";
			for (int i = last_pos ; i < max_pos ; i++)
			{
				text += (*log_cache)[i];
			}

			if (map_active_ws.find(conn) != map_active_ws.end())
			{
				if (map_ws[task_path].find(conn) != map_ws[task_path].end())
				{
					conn->send_text(text);
				}
			}
			else
			{
				std::lock_guard<std::mutex> lck(map_ws_mutex);
				map_ws[task_path].erase(conn);
				return;
			}

			last_pos = max_pos;
			std::unique_lock<std::mutex> lck(*mtx.get());
			cov->wait(lck);
			max_pos = *cpt;
		}
	}
}

void client_manager(task_manager_t * task_manager, crow::websocket::connection * conn, int read_fd)
{
	bool do_stop = false;
	static thread_local char buf[PIPE_BUF];
	ssize_t nb = 0;
	while (!do_stop)
	{
		if ((nb = read(read_fd, buf, PIPE_BUF)) > 0)
		{
			const std::string msg = std::string(buf, nb);
			const std::string prefix = msg.size() >= glo::PREFIX_LEN ? msg.substr(0, glo::PREFIX_LEN) : "";
			const std::string data = msg.size() > glo::PREFIX_LEN ? msg.substr(glo::PREFIX_LEN, msg.size() - glo::PREFIX_LEN) : "";

			if (prefix == glo::PREFIX_CLIENT_CLOSE)
			{
				do_stop = true;
				break;
			}
			else if (prefix == glo::PREFIX_SUBSCRIBE)
			{
				std::thread th = std::thread(send_output_stream, task_manager, conn, data);
				map_ws_current_task[conn] = th.native_handle();
				th.detach();
			}
			else if (prefix == glo::PREFIX_UNSUBSCRIBE)
			{
				map_ws_current_task.erase(conn);
			}
		}
	}
}

std::string get_job_log(Json::Value & log, const std::string & task_path, const int & job_number)
{
	std::string log_file = glo::default_jobs + task_path + "/" + std::to_string(job_number) + "/output.log";
	if (file_exists(log_file + ".html"))
	{
		log["html"] = load_content(log_file + ".html");
	}
	else
	{
		log["content"] = load_content(log_file);
	}
	std::ostringstream os;
	os << log;
	return os.str();
}

int main(int argc, char* argv[])
{
	char list_opt[] = "p:s:t:k:l:";
	int opt = 0;
	std::string log_level;

	while ((opt = getopt(argc, argv, list_opt)) != -1)
	{
		switch (opt)
		{
		case 'k':
			glo::kill_signal = atoi(optarg);
			break;
		case 'l':
			log_level = optarg;
			break;
		case 'p':
		{
			int port = atoi(optarg);
			if (port > 0)
			{
				glo::port = port;
			}
			break;
		}
		case 's':
			glo::storage = optarg;
			break;
		case 't':
			glo::title = optarg;
			break;
		}
	}

	if (glo::title.empty())
	{
		glo::title = "tm";
	}
	else
	{
		glo::title += " (tm)";
	}

	if (glo::storage.empty())
	{
		glo::storage = "/var/lib/tm";
	}

	crow::LogLevel crow_log_level = crow::LogLevel::Warning;

	if (log_level == "debug")
	{
		crow_log_level = crow::LogLevel::Warning;
	}
	else if (log_level == "info")
	{
		crow_log_level = crow::LogLevel::Info;
	}
	else if (log_level == "warning")
	{
		crow_log_level = crow::LogLevel::Warning;
	}
	else if (log_level == "error")
	{
		crow_log_level = crow::LogLevel::Error;
	}
	else if (log_level == "critical")
	{
		crow_log_level = crow::LogLevel::Critical;
	}

	if (!dir_exists(glo::storage))
	{
		CROW_LOG_ERROR << "Storage path " << glo::storage << " does not exist.";
		return EXIT_FAILURE;
	}

	glo::default_jobs = glo::storage + "/jobs";
	glo::default_tasks = glo::storage + "/tasks";

	// https://curl.se/libcurl/c/threadsafe.html
	curl = curl_easy_init();

	crow::App<crow::CORSHandler> app;
	app.loglevel(crow_log_level);

	auto& cors = app.get_middleware<crow::CORSHandler>();
	cors
	.global()
	.headers("Access-Control-Allow-Origin", "*")
	;

	ensure_folder(glo::default_tasks);
	ensure_folder(glo::default_jobs);

	task_manager_t task_manager;

	CROW_ROUTE(app, "/web/")
	([]()
	{
		return load_content("web/index.html");
	});

	CROW_ROUTE(app, "/web/<string>")
	([](const std::string & path)
	{
		return load_content(std::string("web/") + path);
	});

	CROW_ROUTE(app, "/web/<string>/<string>")
	([](const std::string & path, const std::string & filename)
	{
		return load_content("web/" + path + "/" + filename);
	});

	CROW_ROUTE(app, "/api/task/by_path/<string>/jobs")
	([](const std::string & task_path_encoded)
	{
		std::string result = "{}";
		std::string task_path = decode_url(task_path_encoded);
		if (!task_path.empty())
		{
			std::ostringstream os;
			os << load_jobs(glo::default_jobs + task_path);
			result = os.str();
		}
		return result;
	});

	CROW_ROUTE(app, "/api/task/by_path/<string>/execute")
	([&task_manager](const std::string & task_path_encoded)
	{
		std::string response = "{}";
		std::string task_path = decode_url(task_path_encoded);
		if (!task_path.empty())
		{
			int res = task_manager.start(task_path);
			if (res < 0)
			{
				response = "{ \"status\": \"error\", \"errnum\": " + std::to_string(-res) +  "}";
			}
			else
			{
				broadcast_all((std::string("strt:") + task_path).c_str());
				response = "{ \"status\": \"ok\", \"job\": " + std::to_string(res) +  "}";
			}
		}
		return response;
	});

	CROW_ROUTE(app, "/api/task/by_path/<string>/status")
	([](const std::string & task_path_encoded)
	{
		std::string task_path = decode_url(task_path_encoded);
		return load_content(glo::default_jobs + task_path + "/" + STATUS_FILENAME);
	});

	CROW_ROUTE(app, "/api/task/by_path/<string>/job/<int>/status")
	([](const std::string & task_path_encoded, const int & job_number)
	{
		std::string task_path = decode_url(task_path_encoded);
		return load_content(glo::default_jobs + task_path + "/" + std::to_string(job_number) + STATUS_FILENAME);
	});

	CROW_ROUTE(app, "/api/task/by_path/<string>/job/<int>/status_log")
	([](const std::string & task_path_encoded, const int & job_number)
	{
		std::string response;
		std::string task_path = decode_url(task_path_encoded);
		Json::Value log = load_json(glo::default_jobs + task_path + "/" + std::to_string(job_number) + STATUS_FILENAME);
		if (!task_path.empty() && (task_path[0] == '/'))
		{
			response = get_job_log(log, task_path, job_number);
		}
		return response;
	});

	CROW_ROUTE(app, "/api/task/by_path/<string>/job/<int>/log")
	([](const std::string & task_path_encoded, const int & job_number)
	{
		std::string response = "{ \"content\" : \"\" }";
		std::string task_path = decode_url(task_path_encoded);
		if (!task_path.empty() && (task_path[0] == '/'))
		{
			Json::Value log;
			response = get_job_log(log, task_path, job_number);
		}
		return response;
	});

	CROW_ROUTE(app, "/api/task/by_path/<string>/stop")
	([&task_manager](const std::string & task_path_encoded)
	{
		std::string task_path = decode_url(task_path_encoded);
		Json::Value status = task_manager.stop(task_path);
		map_ws.erase(task_path);
		broadcast_all("stop:");
		std::ostringstream txt;
		txt << status;
		return txt.str();
	});

	CROW_ROUTE(app, "/api/tasks")
	([]
	{
		Json::Value root;
		load_tasks(root, "", "");
		std::ostringstream txt;
		txt << root;
		return txt.str();
	});

	CROW_WEBSOCKET_ROUTE(app, "/ws")
	.onopen([&task_manager](crow::websocket::connection & conn)
	{
		std::lock_guard<std::mutex> _(map_ws_mutex);
		CROW_LOG_INFO << "websocket open " << &conn;
		conn.send_text("titl:" + glo::title);
		int pipe_fd[2];
		if (pipe(pipe_fd) == 0)
		{
			map_active_ws[&conn] = pipe_fd[1];
			std::thread th = std::thread(client_manager, &task_manager, &conn, pipe_fd[0]);
			th.detach();
		}
		else
		{
			CROW_LOG_ERROR << "Error creating pipe";
		}
	})
	.onclose([](crow::websocket::connection & conn, const std::string & reason)
	{
		std::lock_guard<std::mutex> _(map_ws_mutex);
		CROW_LOG_INFO << "websocket close " << &conn;
		if (map_active_ws.find(&conn) != map_active_ws.end())
		{
			if (write(map_active_ws[&conn], glo::PREFIX_CLIENT_CLOSE, glo::PREFIX_LEN) < 0)
			{
				CROW_LOG_WARNING << "error (internal) send closing message";
			}
			map_active_ws.erase(&conn);
		}
	})
	.onmessage([&task_manager](crow::websocket::connection & conn, const std::string & data, bool is_binary)
	{
		std::lock_guard<std::mutex> _(map_ws_mutex);
		if (!is_binary)
		{
			Json::Value msg = decode_json(data);
			if (msg["type"].asString() == "subscribe")
			{
				const std::string task_path = msg["task"].asString().c_str();
				const std::string msg = glo::PREFIX_SUBSCRIBE + task_path;
				map_ws[task_path][&conn] = 1;
				if (write(map_active_ws[&conn], msg.c_str(), msg.size()) < 0)
				{
					CROW_LOG_ERROR << "error (internal) send subscribe message";
				}
			}
			else if (msg["type"].asString() == "unsubscribe")
			{
				const std::string task_path = msg["task"].asString().c_str();
				const std::string msg = glo::PREFIX_UNSUBSCRIBE + task_path;
				map_ws[task_path].erase(&conn);
				if (write(map_active_ws[&conn], msg.c_str(), msg.size()) < 0)
				{
					CROW_LOG_ERROR << "error (internal) send unsubscribe message";
				}
			}
		}
	});

	app.port(glo::port).multithreaded().run();

	if (curl)
	{
		curl_easy_cleanup(curl);
	}
	return 0;
}

