# Differences with Workr

|                                 | tm (0.2.1)          | workr (0.3.4)       |
|---------------------------------|---------------------|---------------------|
| authentication                  | no                  | yes                 |
| tasks folder                    | `/var/lib/tm/tasks` | `./jobs`            |
| jobs folder                     | `/var/lib/tm/jobs`  | `./data`            |
| script name                     | `run.sh`            | `run`               |
| task name (folder)              | no quotation marks  | no space character  |
| automatic status refresh        | yes                 | no                  |
| can try to kill a task          | yes                 | no                  |
| can group tasks into subfolders | yes                 | no                  |
| can handle large log            | yes                 | yes                 |
