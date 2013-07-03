//$Id: BimapleServer.cpp 2647 2012-07-03 17:02:15Z oliverax $

#include <syslog.h>
#include "BimapleServer.h"

namespace bimaple
{
namespace httpwrapper
{

const char *HTTPServerEndpoints::index_search_url = "/bimaple/search";
const char *HTTPServerEndpoints::index_info_url = "/bimaple/info";
const char *HTTPServerEndpoints::cache_clear_url = "/bimaple/clear";
const char *HTTPServerEndpoints::index_insert_url = "/bimaple/index/insert";
const char *HTTPServerEndpoints::index_delete_url = "/bimaple/index/delete";
const char *HTTPServerEndpoints::index_save_url = "/bimaple/index/save";
const char *HTTPServerEndpoints::index_merge_url = "/bimaple/index/merge";
const char *HTTPServerEndpoints::index_stop_url = "/bimaple/stop";

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

//TODO: NO way to tell if save failed on bimaple index
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
