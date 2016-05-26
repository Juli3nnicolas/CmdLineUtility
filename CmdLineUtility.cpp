#include "CmdLineUtility.h"
#include <set>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static std::set<CLU::ProcessID> s_processList;

static void SetProcessPriority(HANDLE _process, CLU::ProcessPriority _priority)
{
	using namespace CLU;

	switch ( _priority )
	{
	case LOW:
		SetPriorityClass( _process, BELOW_NORMAL_PRIORITY_CLASS );
		break;

	case NORMAL:
		SetPriorityClass( _process, NORMAL_PRIORITY_CLASS  );
		break;

	case HIGH:
		SetPriorityClass( _process, ABOVE_NORMAL_PRIORITY_CLASS );
		break;

	case VERY_HIGH:
		SetPriorityClass( _process, HIGH_PRIORITY_CLASS );
		break;

	default:
		SetPriorityClass( _process, NORMAL_PRIORITY_CLASS  );
		break;
	}
}

static bool Execute(const char* _cmd, const char* _workingDir, PROCESS_INFORMATION* _outInfo, CLU::OutputBuffer* _cmdOutputBuf, bool _showWindow)
{
	// Get the process' info (useful in case it is to be launched asynchronously)
	if ( _outInfo == nullptr )
		return false;

	// Initialize the STARTUPINFO struct
	STARTUPINFOA sinfo;
	ZeroMemory(&sinfo, sizeof(STARTUPINFOA));
	sinfo.cb = sizeof(STARTUPINFOA);
	sinfo.dwFlags = STARTF_USESHOWWINDOW;
	sinfo.wShowWindow = ( _showWindow ) ? SW_SHOWNORMAL : SW_HIDE;
	BOOL inherit_parent_streams = FALSE;

	if ( _cmdOutputBuf != nullptr )
	{
		// Inherit process' std streams
		sinfo.dwFlags   |= STARTF_USESTDHANDLES;
		sinfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
		sinfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		inherit_parent_streams = TRUE;
	}

	// Get a pipe from which we read output from the launched app
	HANDLE readfh;
	if ( _cmdOutputBuf != nullptr )
	{
		// Initialize security attributes to allow the launched app to
		// inherit the caller's STDOUT, STDIN, and STDERR
		SECURITY_ATTRIBUTES sattr;
		sattr.nLength = sizeof(SECURITY_ATTRIBUTES);
		sattr.lpSecurityDescriptor = 0;
		sattr.bInheritHandle = TRUE;

		if (!CreatePipe(&readfh, &sinfo.hStdOutput, &sattr, 0))
		{
			// Error opening the pipe
			return false;
		}

		// Set pipe's read-end handle (to read commands output)
		_cmdOutputBuf->SetHandle( (CLU::OutputBuffer::Handle) readfh );
	}

	// Launch the app. We should return immediately (while the app is running)
	if (!CreateProcessA(0, const_cast<char*>(_cmd), 0, 0, inherit_parent_streams, 0, 0, _workingDir, &sinfo, _outInfo))
	{
		if ( _cmdOutputBuf != nullptr )
		{
			CloseHandle(readfh);
			CloseHandle(sinfo.hStdOutput);
		}

		return false;
	}

	// We close the pipe's writing end (since we won't write anything) 
	if ( _cmdOutputBuf != nullptr )
		CloseHandle(sinfo.hStdOutput);

	return true;
}

bool CLU::ASyncExecute(const char* _cmd, const char* _workingDir /*= nullptr*/, CLU::OutputBuffer* _cmdOutputBuf /*= nullptr*/, bool _showWindow /*= false*/, ProcessPriority _prio /*= NORMAL*/)
{	
	PROCESS_INFORMATION pinfo;
	if ( Execute( _cmd, _workingDir, &pinfo, _cmdOutputBuf, _showWindow ) )
	{
		SetProcessPriority( pinfo.hProcess, _prio );
		return true;
	}

	return false;
}

CLU::ProcessID CLU::PermanentExecute(const char* _cmd, const char* _workingDir /* = nullptr*/, OutputBuffer* _cmdOutputBuf /*= nullptr*/, bool _showWindow /*= false*/, ProcessPriority _prio /*= NORMAL*/)
{
	PROCESS_INFORMATION* pinfo = new PROCESS_INFORMATION;
	s_processList.insert(pinfo);

	if ( Execute( _cmd, _workingDir, pinfo, _cmdOutputBuf, _showWindow ) )
	{
		SetProcessPriority( pinfo->hProcess, _prio );
		return pinfo;
	}

	return nullptr;
}

bool CLU::SyncExecute(const char* _cmd, const char* _workingDir /*= nullptr*/, std::string* _cmdOutputBuf /*= nullptr*/, bool _showWindow /*= false*/, ProcessPriority _prio /*= NORMAL*/)
{
	bool status = false;

	OutputBuffer buffer; // Must remain until the command line output has fully been read
	PROCESS_INFORMATION pinfo;
	status = Execute( _cmd, _workingDir, &pinfo, (_cmdOutputBuf != nullptr) ? &buffer : nullptr, _showWindow );
	if ( status )
	{
		SetProcessPriority( pinfo.hProcess, _prio );

		// Wait for process to terminate
		DWORD waiting_over = ::WaitForSingleObject( pinfo.hProcess, INFINITE );
		
		if ( waiting_over == WAIT_OBJECT_0 )
		{
			if ( ::GetExitCodeProcess( pinfo.hProcess, &waiting_over ) )
				status = true;
		}

		// Destruct process handle
		CloseHandle(pinfo.hProcess);
	}

	// Fill output buffer with command's messages
	if ( _cmdOutputBuf != nullptr )
	{
		status = buffer.Fill();
		*_cmdOutputBuf = buffer.Get();
	}

	return status;
}

bool CLU::KillPermanent(CLU::ProcessID _p)
{
	PROCESS_INFORMATION* pinfo = static_cast<PROCESS_INFORMATION*>( _p );
	if ( TerminateProcess(pinfo->hProcess, 1) )
	{
		s_processList.erase( _p );
		CloseHandle(pinfo->hProcess);
		delete _p;
		return true;
	}

	return false;
}

unsigned int CLU::GetLastError(char* _message, size_t _size)
{
	DWORD error = ::GetLastError();
	if ( _message != nullptr )
	{
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,          // It´s a system error
			NULL,                                          // No string to be formatted needed
			error,                                        // Hey Windows: Please explain this error!
			MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),    // Do it in the standard language
			_message,                                   // Put the message here
			_size,                                     // Number of bytes to store the message
			NULL);
	}

	return error;
}

void CLU::FormatComplexCommand(std::string& _cmd)
{
	std::string body = "cmd /S /C \"";
	_cmd.insert(0, body);
	_cmd += "\"";
}

bool CLU::OutputBuffer::ReleaseResources()
{
	bool status = true;

	if ( m_handle != INVALID_HANDLE )
	{
		status = CloseHandle( (HANDLE) m_handle ) == TRUE ? true : false;
		m_handle = INVALID_HANDLE;
	}

	return status;
}

bool CLU::OutputBuffer::Fill()
{
	if ( m_handle == INVALID_HANDLE )
		return false;

	const size_t BUFFER_SIZE = 100;
	char buffer[BUFFER_SIZE] = "";

	// while still possible to read
	DWORD bytes_read = 0;
	do
	{
		if (!ReadFile( (HANDLE) m_handle, buffer, BUFFER_SIZE-1, &bytes_read, 0) || bytes_read == 0 )
		{
			// If we aborted for any reason other than that the
			// app has closed that pipe, it's an
			// error. Otherwise, the program has finished its
			// output apparently
			if (::GetLastError() != ERROR_BROKEN_PIPE && bytes_read)
				return false;
		}

		buffer[bytes_read] = '\0';
		m_buffer += buffer;
	} while ( bytes_read != 0 );

	return true;
}