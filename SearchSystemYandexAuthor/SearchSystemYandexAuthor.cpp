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
//using statused_i = static_cast<TaskStatus>(i);

class TeamTasks {
public:
    // Получить статистику по статусам задач конкретного разработчика
    const TasksInfo& GetPersonTasksInfo(const string& person) const {
        return programmers_tasks_.at(person);
    }

    // Добавить новую задачу (в статусе NEW) для конкретного разработчика
    void AddNewTask(const string& person) {
        ++programmers_tasks_[person][TaskStatus::NEW];
    }

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

// Принимаем словарь по значению, чтобы иметь возможность
// обращаться к отсутствующим ключам с помощью [] и получать 0,
// не меняя при этом исходный словарь.
void PrintTasksInfo(TasksInfo tasks_info) {
    cout << tasks_info[TaskStatus::NEW] << " new tasks"s
        << ", "s << tasks_info[TaskStatus::IN_PROGRESS] << " tasks in progress"s
        << ", "s << tasks_info[TaskStatus::TESTING] << " tasks are being tested"s
        << ", "s << tasks_info[TaskStatus::DONE] << " tasks are done"s << endl;
}

int main() {
    TeamTasks tasks;
    tasks.AddNewTask("Ilia");
    for (int i = 0; i < 3; ++i) {
        tasks.AddNewTask("Ivan");
    }
    cout << "Ilia's tasks: ";
    PrintTasksInfo(tasks.GetPersonTasksInfo("Ilia"));
    cout << "Ivan's tasks: ";
    PrintTasksInfo(tasks.GetPersonTasksInfo("Ivan"));

    TasksInfo updated_tasks, untouched_tasks;
    cout << "task_count = 4" << endl;
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Ivan", 4);
    cout << "Updated Ivan's tasks: ";
    PrintTasksInfo(updated_tasks);
    cout << "Untouched Ivan's tasks: ";
    PrintTasksInfo(untouched_tasks);

    cout << "task_count = 6" << endl;
    tie(updated_tasks, untouched_tasks) = tasks.PerformPersonTasks("Ivan", 6);
    cout << "Updated Ivan's tasks: ";
    PrintTasksInfo(updated_tasks);
    cout << "Untouched Ivan's tasks: ";
    PrintTasksInfo(untouched_tasks);
}