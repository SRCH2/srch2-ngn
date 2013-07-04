//$Id: Srch2Server.cpp 3456 2013-06-14 02:11:13Z jiaying $

#include <syslog.h>
#include "Srch2Server.h"

namespace srch2
{
namespace httpwrapper
{

const char *HTTPServerEndpoints::index_search_url = "/srch2/search";
const char *HTTPServerEndpoints::index_info_url = "/srch2/info";
const char *HTTPServerEndpoints::cache_clear_url = "/srch2/clear";
const char *HTTPServerEndpoints::index_insert_url = "/srch2/index/insert";
const char *HTTPServerEndpoints::index_delete_url = "/srch2/index/delete";
const char *HTTPServerEndpoints::index_save_url = "/srch2/index/save";
const char *HTTPServerEndpoints::index_merge_url = "/srch2/index/merge";
const char *HTTPServerEndpoints::index_stop_url = "/srch2/stop";

/*const char *ajax_reply_start =
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";*/

  // CHENLI
  /*const char *HTTPServerEndpoints::ajax_search_pass =
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length:%s\n"
		"\r\n";*/

  const char *HTTPServerEndpoints::ajax_search_fail =
		"HTTP/1.1 400 Bad Request\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

const char *HTTPServerEndpoints::ajax_insert_pass =
		"HTTP/1.1 201 Created\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

const char *HTTPServerEndpoints::ajax_insert_fail =
		"HTTP/1.1 400 Bad Request\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

const char *HTTPServerEndpoints::ajax_insert_fail_403 =
		"HTTP/1.1 403 Forbidden\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

const char *HTTPServerEndpoints::ajax_insert_fail_500 =
		"HTTP/1.1 500 Internal Server Error\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

const char *HTTPServerEndpoints::ajax_save_pass =
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

const char *HTTPServerEndpoints::ajax_merge_pass =
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

//TODO: NO way to tell if save failed on srch2 index
/*const char *ajax_save_fail =
		"HTTP/1.1 400 Bad Request\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";*/

const char *HTTPServerEndpoints::ajax_delete_pass =
		"HTTP/1.1 200 OK\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

const char *HTTPServerEndpoints::ajax_delete_fail =
		"HTTP/1.1 400 Bad Request\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

const char *HTTPServerEndpoints::ajax_delete_fail_500 =
		"HTTP/1.1 500 Internal Server Error\r\n"
		"Cache: no-cache\r\n"
		"Content-Type: application/x-javascript\r\n"
		"\r\n";

}
}
