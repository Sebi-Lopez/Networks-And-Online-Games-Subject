// Students: Jose Antonio Prieto Garcia & Sebastià López Tenorio

#include "ModuleTaskManager.h"

void ModuleTaskManager::threadMain()
{
	while (true)
	{
		// TODO 3:
		// - Wait for new tasks to arrive
		// - Retrieve a task from scheduledTasks
		// - Execute it
		// - Insert it into finishedTasks


		// critical section, waiting scheduledtasks queue
		{
			std::unique_lock<std::mutex> lock(mtx);
			// wait for any scheduledTasks on queue
			while (scheduledTasks.empty()) {
				event.wait(lock);
				if (exitFlag)
					return;
			}
		}

		Task* pendingTask = nullptr;

		// critical section, accessing to scheduledTasks queue
		{
			std::unique_lock<std::mutex> lock(mtx);
			// retrieve pending task
			pendingTask = scheduledTasks.front();
			scheduledTasks.pop();
		}
		
		// and execute the task, not critical and could be a heavy task
		// therefore we not aim to add on a locked scope
		pendingTask->execute();

		// critical section, accessing and pushing new finishedTask
		{
			std::unique_lock<std::mutex> lock(mtx);
			// insert the pending task into finishedtasks
			finishedTasks.push(pendingTask);
		}
	}
}

bool ModuleTaskManager::init()
{
	// TODO 1: Create threads (they have to execute threadMain())
	for (int i = 0; i < MAX_THREADS; ++i)
	{
		threads[i] = std::thread(&ModuleTaskManager::threadMain, this);
	}

	return true;
}

bool ModuleTaskManager::update()
{
	// TODO 4: Dispatch all finished tasks to their owner module (use Module::onTaskFinished() callback)
	
	// critical section, accessing to finishedTasks
	std::unique_lock<std::mutex> lock(mtx);
	while (!finishedTasks.empty())
	{
		Task* finishedTask = finishedTasks.front();
		finishedTasks.pop();
		finishedTask->owner->onTaskFinished(finishedTask);
	}

	return true;
}

bool ModuleTaskManager::cleanUp()
{
	// TODO 5: Notify all threads to finish and join them

	// critical section, change global flag wich mean we are terminating the program
	{
		std::unique_lock<std::mutex> lock(mtx);
		exitFlag = true; // threads read from this var to known if they need to finish
		// notify all threads to end its own waiting condition
		event.notify_all();
	}
	
	for (int i = 0; i < MAX_THREADS; ++i)	{
		threads[i].join();
	}

	return true;
}

void ModuleTaskManager::scheduleTask(Task *task, Module *owner)
{
	task->owner = owner;

	// TODO 2: Insert the task into scheduledTasks so it is executed by some thread

	// critical section
	{
		std::unique_lock<std::mutex> lock(mtx);
		scheduledTasks.push(task);
		event.notify_one();
	}

}
