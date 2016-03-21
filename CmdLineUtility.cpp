#include "CmdLineUtility.h"
#include <set>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

static std::set<Data::ProcessID> s_processList;

static void SetProcessPriority(HANDLE _process, Data::ProcessPriority _priority)
{
	using namespace Data;

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

bool Data::ASyncExecute(const char* _cmd, const char* _workingDir /*= nullptr*/, ProcessPriority _prio /*= NORMAL*/)
{
	std::string cmd = "/C "; cmd += _cmd;
	
	SHELLEXECUTEINFOA shellExInfo;

	shellExInfo.cbSize        = sizeof(SHELLEXECUTEINFO);
	shellExInfo.fMask         = SEE_MASK_DEFAULT;
	shellExInfo.hwnd          = NULL;
	shellExInfo.lpVerb        = "open";
	shellExInfo.lpFile        = "cmd";
	shellExInfo.lpParameters  = cmd.c_str();
	shellExInfo.lpDirectory   = _workingDir;
	shellExInfo.nShow         = SW_HIDE;
	shellExInfo.hInstApp      = NULL;

	// >= 32 implies success, there's no maccro for this value in microsoft's doc...
	if ( ShellExecuteExA(&shellExInfo) >= 32 )
	{
		SetProcessPriority( shellExInfo.hProcess, _prio );
		return true;
	}

	return false;
}

Data::ProcessID Data::PermanentExecute(const char* _cmd, const char* _workingDir /* = nullptr*/, ProcessPriority _prio /*= NORMAL*/)
{
	std::string cmd = "/C "; cmd += _cmd;

	SHELLEXECUTEINFOA* shellExInfo = new SHELLEXECUTEINFOA;

	shellExInfo->cbSize        = sizeof(SHELLEXECUTEINFO);
	shellExInfo->fMask         = SEE_MASK_NOCLOSEPROCESS;
	shellExInfo->hwnd          = NULL;
	shellExInfo->lpVerb        = "open";
	shellExInfo->lpFile        = "cmd";
	shellExInfo->lpParameters  = cmd.c_str();
	shellExInfo->lpDirectory   = _workingDir;
	shellExInfo->nShow         = SW_HIDE;
	shellExInfo->hInstApp      = NULL;
	
	s_processList.insert(shellExInfo);

	// >= 32 implies success, there's no maccro for this value in microsoft's doc...
	if ( ShellExecuteExA(shellExInfo) >= 32 )
	{
		SetProcessPriority( shellExInfo->hProcess, _prio );
		return shellExInfo;
	}

	return nullptr;
}

bool Data::SyncExecute(const char* _cmd, const char* _workingDir /*= nullptr*/, ProcessPriority _prio /*= NORMAL*/)
{
	bool status = false;
	
	std::string cmd = "/C "; cmd += _cmd;

	SHELLEXECUTEINFOA shellExInfo;

	shellExInfo.cbSize        = sizeof(SHELLEXECUTEINFO);
	shellExInfo.fMask         = SEE_MASK_NOCLOSEPROCESS;
	shellExInfo.hwnd          = NULL;
	shellExInfo.lpVerb        = "open";
	shellExInfo.lpFile        = "cmd";
	shellExInfo.lpParameters  = cmd.c_str();
	shellExInfo.lpDirectory   = _workingDir;
	shellExInfo.nShow         = SW_HIDE;
	shellExInfo.hInstApp      = NULL;

	if ( ShellExecuteExA(&shellExInfo) == TRUE )
	{
		SetProcessPriority( shellExInfo.hProcess, _prio );

		// Wait for process to terminate
		DWORD waiting_over = ::WaitForSingleObject( shellExInfo.hProcess, INFINITE );
		
		if ( waiting_over == WAIT_OBJECT_0 )
		{
			if ( ::GetExitCodeProcess( shellExInfo.hProcess, &waiting_over ) )
				status = true;
		}

		// Destruct process handle
		CloseHandle(shellExInfo.hProcess);
	}

	return status;
}

bool Data::KillPermanent(Data::ProcessID _p)
{
	SHELLEXECUTEINFOA* shellExInfo = static_cast<SHELLEXECUTEINFOA*>( _p );
	if ( TerminateProcess(shellExInfo->hProcess, 1) )
	{
		s_processList.erase( _p );
		CloseHandle(shellExInfo->hProcess);
		delete _p;
		return true;
	}

	return false;
}

unsigned int Data::GetLastError(char* _message, size_t _size)
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