#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP

#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <json/json.h>

using vec_pid_t = std::vector<pid_t>;
using vec_str_t = std::vector<std::string>;
using map_ptr_t = std::map<std::string, std::shared_ptr<vec_str_t>>;
using map_int_t = std::map<std::string, std::shared_ptr<int>>;
using map_cov_t = std::map<std::string, std::shared_ptr<std::condition_variable>>;
using map_mtx_t = std::map<std::string, std::shared_ptr<std::mutex>>;
using map_pid_t = std::map<std::string, vec_pid_t>;

using task_list_t = std::map<std::string, Json::Value>;

class task_manager_t
{
private:
    task_list_t task_list;
    task_list_t running_tasks;
    map_ptr_t   map_log;
    map_int_t   map_cpt;
    map_cov_t   map_cov;
    map_mtx_t   map_mtx;
    map_pid_t   map_pid;

public:
    int         start       (const std::string & task_path);
    Json::Value stop        (const std::string & task_path);
    void        signal_end  (const std::string & task_path, int exit_code);
    std::shared_ptr<vec_str_t> set_log     (const std::string task_path);
    void        del_log     (const std::string task_path);
    void        set_pid     (const std::string & task_path, vec_pid_t pid_list);

    const   std::shared_ptr<int>                        get_cpt (const std::string task_path);
    const   std::shared_ptr<vec_str_t>                  get_log (const std::string task_path);
    const   std::shared_ptr<std::mutex>                 get_mtx (const std::string task_path);
    const   std::shared_ptr<std::condition_variable>    get_cov (const std::string task_path);
};

#endif // TASK_MANAGER_HPP
