# Differences with Workr

|                                 | tm                  | workr               |
|---------------------------------|---------------------|---------------------|
| authentication                  | no                  | yes                 |
| tasks folder                    | `/var/lib/tm/tasks` | `./jobs`            |
| jobs folder                     | `/var/lib/tm/jobs`  | `./data`            |
| script name                     | `run.sh`            | `run`               |
| task name (folder)              | no quotation marks  | no space character  |
| automatic status refresh        | yes                 | no                  |
| can try to kill a task          | yes                 | no                  |
| can group tasks into subfolders | yes                 | no                  |
| can handle big log              | no                  | yes                 |
