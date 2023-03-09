#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <string>
#include <vector>

#include <proc/readproc.h>

#include <json/json.h>

using vec_pid_t = std::vector<pid_t>;

namespace glo
{
extern vec_pid_t children_pid_list;
}; // namespace glo

enum class file_type_t
 {
    FOLDER,
    TASK,
    ERROR,
};

Json::Value     decode_json         (const std::string value);
Json::Value     load_json           (const std::string filename);
void            save_json           (const std::string filename, Json::Value & node);
file_type_t     load_tasks          (Json::Value & node, const std::string storage_path, const std::string parent);
Json::Value     load_jobs           (const std::string storage_path);
std::string     load_content        (const std::string filename);
bool            dir_exists          (const std::string & folder);
bool            file_exists         (const std::string & filename);
int             ensure_folder       (const std::string & folder);
int             get_next_job_number (const std::string & task_path);
std::string     decode_url          (const std::string & url);
std::string     decode_param        (const std::string & param);
std::string     enquote_filename     (const std::string & filename);

vec_pid_t       children            (pid_t pid);
void            children            (pid_t pid, vec_pid_t & vec);
vec_pid_t       diff_children       (const std::string & task_path);

#endif // HELPERS_HPP
