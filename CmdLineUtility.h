#pragma once

namespace CLU
{
	typedef void* ProcessID;

	enum ProcessPriority: unsigned int
	{
		LOW,
		NORMAL,
		HIGH,
		VERY_HIGH
	};

	// Executes cmd in a dos shell and quits WHEN the shell has been launched
	// Returns true on successful shell start
	// When _workingDir is null, the working directory is 
	bool ASyncExecute(const char* _cmd, const char* _workingDir = nullptr, ProcessPriority _prio = NORMAL);

	// Same as Execute but termination is user managed
	ProcessID PermanentExecute(const char* _cmd, const char* _workingDir = nullptr, ProcessPriority _prio = NORMAL);

	// Executes cmd in a dos shell then waits until termination
	// Returns true on successful command line execution.
	bool SyncExecute(const char* _cmd, const char* _workingDir = nullptr, ProcessPriority _prio = NORMAL);

	// Terminates a process started with PermanentExecute.
	// Returns false in case an error occurred or the process wasn't launched
	// with PermanentExecute
	bool KillPermanent(ProcessID _p);

	// Returns the last error code. If _message is not null,
	// an error message will be copied to _message.
	unsigned int GetLastError(char* _message, size_t _size);
}