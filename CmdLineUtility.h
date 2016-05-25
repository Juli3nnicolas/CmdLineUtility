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

	// Executes cmd in a dos shell and quits WHEN the shell has been launched
	// Returns true on successful shell start
	// When _workingDir is null, the working directory is 
	bool ASyncExecute(const char* _cmd, const char* _workingDir = nullptr, OutputBuffer* _cmdOutputBuf = nullptr, ProcessPriority _prio = NORMAL);

	// Same as Execute but termination is user managed
	ProcessID PermanentExecute(const char* _cmd, const char* _workingDir = nullptr, OutputBuffer* _cmdOutputBuf = nullptr, ProcessPriority _prio = NORMAL);

	// Executes cmd in a dos shell then waits until termination
	// Returns true on successful command line execution.
	bool SyncExecute(const char* _cmd, const char* _workingDir = nullptr, std::string* _cmdOutputBuf = nullptr, ProcessPriority _prio = NORMAL);

	// Terminates a process started with PermanentExecute.
	// Returns false in case an error occurred or the process wasn't launched
	// with PermanentExecute
	bool KillPermanent(ProcessID _p);

	// Returns the last error code. If _message is not null,
	// an error message will be copied to _message.
	unsigned int GetLastError(char* _message, size_t _size);
}