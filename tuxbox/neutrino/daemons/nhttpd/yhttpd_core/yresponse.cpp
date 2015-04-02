//=============================================================================
// YHTTPD
// Response
//=============================================================================

// c
#include <cstdarg>
#include <cstdio>
#include <errno.h>
// c++
#include <string.h>
// system
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
// yhttpd
#include "yconfig.h"
#include "yhttpd.h"
#include "ytypes_globals.h"
#include "ylogging.h"
#include "ywebserver.h"
#include "yconnection.h"
#include "helper.h"
#include "yhook.h"

#ifdef Y_CONFIG_HAVE_SENDFILE
#include <sys/sendfile.h>
#endif

//=============================================================================
// Constructor & Destructor
//=============================================================================
CWebserverResponse::CWebserverResponse(CWebserver *pWebserver)
{
	Webserver = pWebserver;
	CWebserverResponse();
}
//-----------------------------------------------------------------------------
CWebserverResponse::CWebserverResponse()
{
}

//=============================================================================
// Main Dispacher for Response
// To understand HOOKS reade yhook.cpp Comments!!!
//-----------------------------------------------------------------------------
// RFC 2616 / 6 Response
//
//	After receiving and interpreting a request message, a server responds
//	with an HTTP response message.
//	Response       =Status-Line		; generated by SendHeader
//			*(( general-header	; generated by SendHeader
//			 | response-header	; generated by SendHeader
//			 | entity-header ) CRLF); generated by SendHeader
//			 CRLF			; generated by SendHeader
//			 [ message-body ]	; by HOOK Handling Loop or Sendfile
//=============================================================================
bool CWebserverResponse::SendResponse()
{
	// Init Hookhandler
	Connection->HookHandler.session_init(Connection->Request.ParameterList, Connection->Request.UrlData, 
		(Connection->Request.HeaderList), (Cyhttpd::ConfigList), Connection->Method, Connection->keep_alive);
	//--------------------------------------------------------------
	// HOOK Handling Loop [ PREPARE response hook ]
	// Checking and Preperation: Auth, static, cache, ...
	//--------------------------------------------------------------

// move to mod_sendfile ???
#ifdef Y_CONFIG_USE_HOSTEDWEB
	// for hosted webs: rewrite URL
	std::string _hosted="/hosted/";
	if((Connection->Request.UrlData["path"]).compare(0,_hosted.length(),"/hosted/") == 0)		// hosted Web ?
		Connection->Request.UrlData["path"]=Cyhttpd::ConfigList["WebsiteMain.hosted_directory"]
			+(Connection->Request.UrlData["path"]).substr(_hosted.length()-1);
#endif //Y_CONFIG_USE_HOSTEDWEB
	log_level_printf(5,"UrlPath:%s\n",(Connection->Request.UrlData["path"]).c_str());

	do
	{
		if(Connection->RequestCanceled)
			return false;

		Connection->HookHandler.Hooks_PrepareResponse();
		if(Connection->HookHandler.status == HANDLED_ERROR || Connection->HookHandler.status == HANDLED_ABORT)
		{
			log_level_printf(2,"Response Prepare Hook found but Error\n");
			Write(Connection->HookHandler.BuildHeader());
			Write(Connection->HookHandler.yresult);
			return false;
		}
		// URL has new value. Analyze new URL for SendFile
		else if(Connection->HookHandler.status == HANDLED_SENDFILE ||
			Connection->HookHandler.status == HANDLED_REWRITE)
		{
			Connection->Request.analyzeURL(Connection->HookHandler.NewURL);
			Connection->HookHandler.UrlData = Connection->Request.UrlData;
		}
		if(Connection->HookHandler.status == HANDLED_REDIRECTION)
		{
			Write(Connection->HookHandler.BuildHeader());
			return false;
		}
	}
	while(Connection->HookHandler.status == HANDLED_REWRITE);
	
	// Prepare = NOT_MODIFIED ?
	if(Connection->HookHandler.httpStatus == HTTP_NOT_MODIFIED)
	{
		Write(Connection->HookHandler.BuildHeader());
		return true;
	}
	
	//--------------------------------------------------------------
	// HOOK Handling Loop [ response hook ]
	// Production
	//--------------------------------------------------------------
	if(Connection->HookHandler.status != HANDLED_SENDFILE)
	do
	{
		if(Connection->RequestCanceled)
			return false;

		Connection->HookHandler.Hooks_SendResponse();
		if((Connection->HookHandler.status == HANDLED_READY)||(Connection->HookHandler.status == HANDLED_CONTINUE))
		{
			log_level_printf(2,"Response Hook Output. Status:%d\n", Connection->HookHandler.status);
			Write(Connection->HookHandler.BuildHeader());
			if(Connection->Method != M_HEAD)
				Write(Connection->HookHandler.yresult);
			if(Connection->HookHandler.status != HANDLED_CONTINUE)
				return true;
		}
		else if(Connection->HookHandler.status == HANDLED_ERROR)
		{
			log_level_printf(2,"Response Hook found but Error\n");
			Write(Connection->HookHandler.BuildHeader());
			if(Connection->Method != M_HEAD)
				Write(Connection->HookHandler.yresult);
			return false;
		}
		else if(Connection->HookHandler.status == HANDLED_ABORT)
			return false;
		// URL has new value. Analyze new URL for SendFile
		else if(Connection->HookHandler.status == HANDLED_SENDFILE ||
			Connection->HookHandler.status == HANDLED_REWRITE)
		{
			Connection->Request.analyzeURL(Connection->HookHandler.NewURL);
			Connection->HookHandler.UrlData = Connection->Request.UrlData;
		}
		if(Connection->HookHandler.status == HANDLED_REDIRECTION)
		{
			Write(Connection->HookHandler.BuildHeader());
			return false;
		}
	}
	while(Connection->HookHandler.status == HANDLED_REWRITE);

	// Send static file 
	if(Connection->HookHandler.status == HANDLED_SENDFILE && !Connection->RequestCanceled)
	{
		bool cache = true;
//		if(Connection->HookHandler.UrlData["path"] == "/tmp/")//TODO: un-cachable dirs
//			cache = false;
		Write(Connection->HookHandler.BuildHeader(cache));
		if(Connection->Method != M_HEAD)
			Sendfile(Connection->Request.UrlData["url"]);
		return true;
	}			

	// arrived here? = error!
	SendError(HTTP_NOT_FOUND);
	return false;
}

//=============================================================================
// Output 
//=============================================================================
void CWebserverResponse::SendHeader(HttpResponseType responseType, bool cache, std::string ContentType)
{
	Connection->HookHandler.SetHeader(responseType, ContentType); 
	Write(Connection->HookHandler.BuildHeader(cache));
}

//-----------------------------------------------------------------------------
// BASIC Send over Socket for Strings (char*)
//-----------------------------------------------------------------------------
bool CWebserverResponse::WriteData(char const * data, long length)
{
	if(Connection->RequestCanceled)
		return false;
	if(Connection->sock->Send(data, length) == -1)
	{
		log_level_printf(1,"response canceled: %s\n", strerror(errno));
		Connection->RequestCanceled = true;
		return false;
	}
	else
		return true;
}
//-----------------------------------------------------------------------------
#define bufferlen 4*1024
void CWebserverResponse::printf ( const char *fmt, ... )
{
	char buffer[bufferlen];
	va_list arglist;
	va_start( arglist, fmt );
	vsnprintf( buffer, bufferlen, fmt, arglist );
	va_end(arglist);
	Write(buffer);
}
//-----------------------------------------------------------------------------
bool CWebserverResponse::Write(char const *text)
{
	return WriteData(text, strlen(text));
}
//-----------------------------------------------------------------------------
bool CWebserverResponse::WriteLn(char const *text)
{
	if(!WriteData(text, strlen(text)))
		return false;
	return WriteData("\r\n",2);
}

//-----------------------------------------------------------------------------
bool CWebserverResponse::Sendfile(std::string filename)
{
	if(Connection->RequestCanceled)
		return false;
	int filed = open( filename.c_str(), O_RDONLY ); 
	if(filed != -1 ) //can access file?
	{
		if(!Connection->sock->SendFile(filed))
			Connection->RequestCanceled = true;
		close(filed);
	}
	return (filed != -1 );
}

//-----------------------------------------------------------------------------
// Send File: Determine MIME-Type fro File-Extention
//-----------------------------------------------------------------------------
std::string CWebserverResponse::GetContentType(std::string ext)
{
	std::string ctype = "text/plain";
	ext = string_tolower(ext);
	for (unsigned int i = 0;i < (sizeof(MimeFileExtensions)/sizeof(MimeFileExtensions[0])); i++)
		if (MimeFileExtensions[i].fileext == ext)
		{
			ctype = MimeFileExtensions[i].mime;
			break;
		}	
	return ctype;
}
