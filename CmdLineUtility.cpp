#include "CmdLineUtility.h"
#include <set>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

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

static void SetShellCmd(std::string* _resultCmd, const char* _cmd)
{
	*_resultCmd = "/S /C "; *_resultCmd += "\""; *_resultCmd += _cmd; *_resultCmd += "\"";
}

static void InitShellExStruct(SHELLEXECUTEINFOA* _shellExInfo, unsigned int _mask, const char* _cmd, const char* _workingDir, bool _hideWindow)
{
	_shellExInfo->cbSize        = sizeof(SHELLEXECUTEINFO);
	_shellExInfo->fMask         = _mask;
	_shellExInfo->hwnd          = NULL;
	_shellExInfo->lpVerb        = "open";
	_shellExInfo->lpFile        = "cmd";
	_shellExInfo->lpParameters  = _cmd;
	_shellExInfo->lpDirectory   = _workingDir;
	_shellExInfo->nShow         = (_hideWindow) ? SW_HIDE : SW_SHOWNORMAL;
	_shellExInfo->hInstApp      = NULL;
}

bool CLU::ASyncExecute(const char* _cmd, const char* _workingDir /*= nullptr*/, bool _hideWindow /*= true*/, ProcessPriority _prio /*= NORMAL*/)
{
	std::string cmd;
	SetShellCmd( &cmd, _cmd );
	
	SHELLEXECUTEINFOA shellExInfo;
	InitShellExStruct( &shellExInfo, SEE_MASK_DEFAULT, cmd.c_str(), _workingDir, _hideWindow);

	if ( ShellExecuteExA(&shellExInfo) == TRUE )
	{
		SetProcessPriority( shellExInfo.hProcess, _prio );
		return true;
	}

	return false;
}

CLU::ProcessID CLU::PermanentExecute(const char* _cmd, const char* _workingDir /* = nullptr*/, bool _hideWindow /*= true*/, ProcessPriority _prio /*= NORMAL*/)
{
	std::string cmd;
	SetShellCmd( &cmd, _cmd );

	SHELLEXECUTEINFOA* shellExInfo = new SHELLEXECUTEINFOA;
	InitShellExStruct( shellExInfo, SEE_MASK_NOCLOSEPROCESS, cmd.c_str(), _workingDir, _hideWindow );
	s_processList.insert(shellExInfo);

	if ( ShellExecuteExA(shellExInfo) == TRUE )
	{
		SetProcessPriority( shellExInfo->hProcess, _prio );
		return shellExInfo;
	}

	return nullptr;
}

bool CLU::SyncExecute(const char* _cmd, const char* _workingDir /*= nullptr*/, bool _hideWindow /*= true*/, ProcessPriority _prio /*= NORMAL*/)
{
	bool status = false;
	
	std::string cmd;
	SetShellCmd( &cmd, _cmd );

	SHELLEXECUTEINFOA shellExInfo;
	InitShellExStruct( &shellExInfo, SEE_MASK_NOCLOSEPROCESS, cmd.c_str(), _workingDir, _hideWindow );

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

bool CLU::KillPermanent(CLU::ProcessID _p)
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