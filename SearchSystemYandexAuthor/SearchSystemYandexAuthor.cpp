#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#define statused_i static_cast<TaskStatus>(i)
using namespace std;

// Перечислимый тип для статуса задачи
enum class TaskStatus {
    NEW,          // новая
    IN_PROGRESS,  // в разработке
    TESTING,      // на тестировании
    DONE          // завершена
};

// Объявляем тип-синоним для map<TaskStatus, int>,
// позволяющего хранить количество задач каждого статуса
using TasksInfo = map<TaskStatus, int>;


class TeamTasks {
    // Обновить статусы по данному количеству задач конкретного разработчика,
    // подробности см. ниже
    tuple<TasksInfo, TasksInfo> PerformPersonTasks(const string& person, int task_count) {
        TasksInfo performed_tasks, unperformed_tasks = programmers_tasks_.at(person);
        TasksInfo start = programmers_tasks_.at(person);

        if (programmers_tasks_.count(person) == 0) {
            return tuple(performed_tasks, unperformed_tasks);
        }

        for (int i = 0; i < 3; ++i) {
            if (task_count == 0) {
                break;
            }
            PerformTasks(person, task_count, statused_i,
                performed_tasks, unperformed_tasks);
        }

        unperformed_tasks.erase(TaskStatus::DONE);

        return tuple(performed_tasks, unperformed_tasks);
    }

private:
    map<string, TasksInfo> programmers_tasks_;

    void PerformTasks(const string& person, int& task_count, TaskStatus task_status,
        TasksInfo& performed_tasks, TasksInfo& unperformed_tasks) {
        int next_status = static_cast<int>(task_status) + 1;
        while (task_count != 0 && programmers_tasks_.at(person)[task_status] != 0) {
            ++performed_tasks[static_cast<TaskStatus>(next_status)];
            --programmers_tasks_[person][task_status];
            ++programmers_tasks_[person][static_cast<TaskStatus>(next_status)];

            //если задач в текущем статусе на начало выполнения не было
            if (unperformed_tasks[task_status] != 0) {
                --unperformed_tasks[task_status];
            }
            --task_count;
        }
    }
};