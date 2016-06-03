#pragma once

#include <string>

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

	// Buffer used to retrieve one command's console output
	// Is to be used directly with asynchronous command executions
	class OutputBuffer final
	{
	public:
		typedef uintptr_t Handle;
		static const Handle INVALID_HANDLE = NULL;

		OutputBuffer(): 
			m_handle(INVALID_HANDLE),
			m_buffer("") {}
		~OutputBuffer() { ReleaseResources(); }

		const std::string& Get() const { return m_buffer;  }
		void Flush()                   { m_buffer.clear(); }

		// Fill the buffer with the content pointed to by m_handle
		bool Fill();

		// Don't use unless you really know what you're doing
		// It is used by the XXExecute commands to setup the output process
		// OS' resources won't be freed correctly if this has a wrong value
		void SetHandle(Handle _handle) { m_handle = _handle; }

	private:
		bool ReleaseResources();

		std::string m_buffer;
		Handle      m_handle;

		OutputBuffer(const OutputBuffer&) {}
		OutputBuffer& operator=(const OutputBuffer&) { return *this; }
	};

	// GUIDELINES
	// The following xxxExecute functions enable users to launch processes.
	// Use the FormatCmd function before calling xxxExecute to execute it
	// in a console terminal.
	//
	// USE EXAMPLE
	// std::string ping = "ping /4 /n 10 xxx.xxx.xxx.xxx";
	// FormatCmd(ping);
	// xxxExecute(ping.c_str());
	//
	// IMPORTANT
	// If you pass a buffer and choose to display a command window (_showWindow == true), nothing will be printed on it.
	// The reason is that its whole content is sucked up by the buffer.

	// Create a new process then DIRECTLY quits after creation completion.
	// Returns true on successful start
	// When _workingDir is null, the working directory is 
	bool ASyncExecute(const char* _cmd, const char* _workingDir = nullptr, OutputBuffer* _cmdOutputBuf = nullptr, bool _showWindow = false, ProcessPriority _prio = NORMAL);

	// Same as Execute but termination is user managed
	ProcessID PermanentExecute(const char* _cmd, const char* _workingDir = nullptr, OutputBuffer* _cmdOutputBuf = nullptr, bool _showWindow = false, ProcessPriority _prio = NORMAL);

	// Executes a new process then waits until termination
	// Returns true on successful execution.
	bool SyncExecute(const char* _cmd, const char* _workingDir = nullptr, std::string* _cmdOutputBuf = nullptr, bool _showWindow = false, ProcessPriority _prio = NORMAL);

	// Terminates a process started with PermanentExecute.
	// Returns false in case an error occurred or the process wasn't launched
	// with PermanentExecute
	bool KillPermanent(ProcessID _p);

	// Returns the last error code. If _message is not null,
	// an error message will be copied to _message.
	unsigned int GetLastError(char* _message, size_t _size);

	// Formats a cmd to be executed in a terminal
	//
	// Warning: Any cmd constructed with this function will return an empty _cmdOutputBuf when
	// run from an Execute function.
	void FormatCmd(std::string& _cmd);
}