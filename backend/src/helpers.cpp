#include "helpers.hpp"
#include "globals.hpp"

#include <crow/logging.h>
#include <curl/curl.h>

#include <algorithm>
#include <fstream>
#include <mutex>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>

extern CURL * curl;
std::mutex curl_mutex;

using vec_string_t = std::vector<std::string>;
using vec_json_t = std::vector<Json::Value>;

namespace glo
{
vec_pid_t children_pid_list;
}; // namespace glo

#if (JSONCPP_VERSION_MAJOR==1 && JSONCPP_VERSION_MINOR<=7)

Json::Value decode_json(const std::string value)
{
	Json::Value root;

	try
	{
		Json::Reader reader;
		if (!reader.parse(value, root))
		{
			CROW_LOG_ERROR << "error parsing json " << value;
		}
	}
	catch (std::exception &e)
	{
		CROW_LOG_ERROR << "error reading json " << value;
	}

	return root;
}

Json::Value load_json(const std::string filename)
{
	std::string value = load_content(filename);
	Json::Value root;

	try
	{
		Json::Reader reader;
		if (!reader.parse(value, root))
		{
			CROW_LOG_ERROR << "error parsing json from file " << filename;
		}
	}
	catch (std::exception &e)
	{
		CROW_LOG_ERROR << "error reading json from file " << filename;
	}

	return root;
}

#elif (JSONCPP_VERSION_MAJOR==1 && JSONCPP_VERSION_MINOR>7)

Json::Value decode_json(const std::string value)
{
// 	https://github.com/open-source-parsers/jsoncpp/issues/815

// 	Json::Value root;
// 	JSONCPP_STRING errs;
// 	const auto len = static_cast<int>(value.length());
// 	Json::CharReaderBuilder builder;
// 	builder["collectComments"] = false;
// 	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
// 	if (!reader->parse(value.c_str(), value.c_str() + len, &root, &errs))
// 	{
// 		CROW_LOG_ERROR << "decode_json error";
// 	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	Json::Value root;

	try
	{
		Json::Reader reader;
		if (!reader.parse(value, root, false))
		{
			CROW_LOG_ERROR << "error parsing json " << value;
		}
	}
	catch (std::exception &e)
	{
		CROW_LOG_ERROR << "error reading json " << value;
	}

#pragma GCC diagnostic pop

	return root;
}

Json::Value load_json(const std::string filename)
{
	Json::Value root;

	try
	{
		std::ifstream ifs(filename);
		if (ifs.is_open())
		{
			Json::CharReaderBuilder builder;
			builder["collectComments"] = false;
			JSONCPP_STRING errs;
			if (!parseFromStream(builder, ifs, &root, &errs))
			{
				CROW_LOG_ERROR << "error parsing json from file " << filename;
			}
			ifs.close();
		}
	}
	catch (std::exception &e)
	{
		CROW_LOG_ERROR << "error reading json from file " << filename;
	}

	return root;
}

#else
#error "JSONCPP Version not supported"
#endif

void save_json(const std::string filename, Json::Value & node)
{
	std::ofstream ofs(filename);
	if (ofs.is_open())
	{
		ofs << node;
		ofs.close();
	}
}

// https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c

file_type_t load_tasks(Json::Value & node, const std::string storage_path, const std::string parent)
{
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir((glo::default_tasks + storage_path).c_str())) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if ((ent->d_type == DT_LNK) || (ent->d_type == DT_REG))
			{
				const std::string filename = ent->d_name;
				if (filename == "run.sh")
				{
					return file_type_t::TASK;
				}
			}
		}

		rewinddir(dir);

		vec_string_t element_list;

		while ((ent = readdir(dir)) != NULL)
		{
			if (ent->d_type == DT_DIR)
			{
				std::string dirname = ent->d_name;
				if ((dirname != ".") && (dirname != ".."))
				{
					element_list.push_back(dirname);
				}
			}
		}

		std::sort(element_list.begin(), element_list.end());

		vec_json_t dir_list;
		vec_json_t task_list;
        
		for (std::string & dirname : element_list)
		{
			Json::Value dir_path;
			dir_path["status"];
			dir_path["parent"] = parent.empty() ? "/" : parent;
			dir_path["name"] = dirname;
			file_type_t file_type = load_tasks(dir_path["children"], storage_path + "/" + dirname, parent + "/" + dirname);
			if (file_type == file_type_t::FOLDER)
			{
				dir_path["type"] = "folder";
                dir_list.push_back(dir_path);
			}
			if (file_type == file_type_t::TASK)
			{
				dir_path["type"] = "task";
				dir_path["status"] = load_json(glo::default_jobs + storage_path + "/" + dirname + STATUS_FILENAME);
                task_list.push_back(dir_path);
			}
		}
		
		for (Json::Value & elt : dir_list)
		{
			node.append(elt);
		}
        
		for (Json::Value & elt : task_list)
		{
			node.append(elt);
		}
        
		closedir(dir);
	}

	return file_type_t::FOLDER;
}

Json::Value load_jobs(const std::string storage_path)
{
	Json::Value node;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(storage_path.c_str())) != NULL)
	{
		// Effectuer un tri naturel
		std::vector<int> dir_list;
		while ((ent = readdir(dir)) != NULL)
		{
			if (ent->d_type == DT_DIR)
			{
				std::string dirname = ent->d_name;
				if ((dirname != ".") && (dirname != ".."))
				{
					dir_list.push_back(atoi(ent->d_name));
				}
			}
		}

		std::sort(dir_list.rbegin(), dir_list.rend());

		for (int num : dir_list)
		{
			Json::Value job = load_json(storage_path + "/" + std::to_string(num) + STATUS_FILENAME);
			if (!job.isNull())
			{
				node.append(job);
			}
		}

		closedir(dir);
	}

	return node;
}

std::string load_content(const std::string filename)
{
	// https://github.com/CrowCpp/Crow/blob/4a6d5fe74c05d87ba8742a90666dce565848fbe3/tests/template/mustachetest.cpp
	std::ifstream is(filename);
	return { std::istreambuf_iterator<char> (is), std::istreambuf_iterator<char>() };
}

bool dir_exists(const std::string & folder)
{
	struct stat sb;
	return (stat(folder.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}

bool file_exists(const std::string & filename)
{
	struct stat sb;
	return (stat(filename.c_str(), &sb) == 0 && S_ISREG(sb.st_mode));
}

int ensure_folder(const std::string & folder)
{
	if (!dir_exists(folder))
	{
		std::string cmd = "mkdir -p " + escape_filename(folder);
		return system(cmd.c_str());
	}
	return 0;
}

std::string decode_url(const std::string & url)
{
	// https://curl.se/libcurl/c/threadsafe.html
	const std::lock_guard<std::mutex> lock(curl_mutex);
	std::string decoded_string;
	int decodelen;
	char * decoded = curl_easy_unescape(curl, url.c_str(), url.length(), &decodelen);
	if (decoded)
	{
		decoded_string = decoded;
		curl_free(decoded);
	}
	return decoded_string;
}

std::string escape_filename(const std::string & filename)
{
	std::string str = "\"";
	str += filename;
	str += "\"";
	return str;
}

// https://stackoverflow.com/questions/62365978/get-a-list-of-child-processes-from-parent-process-in-c-and-c-cross-platform

vec_pid_t children(pid_t pid)
{
	vec_pid_t vec;
	PROCTAB *proc = openproc(PROC_FILLSTAT);
	while (proc_t *proc_info = readproc(proc, nullptr))
	{
		if (proc_info->ppid == pid)
		{
			vec.push_back(proc_info->tgid);
		}
		freeproc(proc_info);
	}
	return vec;
}

void children(pid_t pid, vec_pid_t & vec)
{
	PROCTAB *proc = openproc(PROC_FILLSTAT);
	while (proc_t *proc_info = readproc(proc, nullptr))
	{
		if (proc_info->ppid == pid)
		{
			vec.push_back(proc_info->tgid);
		}
		freeproc(proc_info);
	}
}

vec_pid_t diff_children(const std::string & task_path)
{
	vec_pid_t pid_list;
	vec_pid_t children_list = children(getpid());
	for (pid_t pid : children_list)
	{
		children(pid, pid_list);
	}

	std::sort(pid_list.begin(), pid_list.end());

	vec_pid_t diff_list;

	std::set_difference(
	    pid_list.begin(),
	    pid_list.end(),
	    glo::children_pid_list.begin(),
	    glo::children_pid_list.end(),
	    back_inserter(diff_list));

	glo::children_pid_list = pid_list;

	return diff_list;
}
