#include "libchttpclient.h"

#include "queue.h"

#include <pthread.h>
#include <curl/curl.h>

//=========================================================
//			内部的数据结构
//=========================================================

#include <curl/curlbuild-64.h>
#include <curl/easy.h>
#include <curl/typecheck-gcc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 工作者
typedef struct worker {

	char  name[36];						// 名称, 便于调试

	pthread_t pthread;					// pthread

	volatile int is_stoped;				// 是否停止
} worker;

//=========================================================
//			全局数据
//=========================================================

// 全局配置
static config* g_config = NULL;

// 工作队列, 目前单queue实现, 后面可以考虑多队列
block_queue* g_work_queue = NULL;

// 工作者
worker* g_workers = NULL;

//=========================================================
//			线程函数, 干活的
//=========================================================
static void* work(void* arg);

//=========================================================
//			将curl的错误码映射到本地的错误码
//=========================================================
static int curl_code_map_2_my_code(CURLcode code);

//=========================================================
//			发起http请求
//=========================================================
static response* do_request(request* request);

static size_t defult_get_data_size(request* request) {
	return 0;
}
static size_t default_writer(void* buffer, size_t size, size_t nmemb, void* userp) {
	// 默认没有数据需要写出
	printf("default_writer write none data\n");
	return 0;
}

static size_t default_reader(void* contents, size_t size, size_t nmemb, void* userp) {
	fprintf(stderr, "reader\n");
	response* r = (response*)userp;
	size_t realsize = size * nmemb;
	if(NULL == r->content) {
		r->content = malloc(realsize + 1);
	} else {
		r->content = realloc(r->content, r->content_size + realsize + 1);
	}

	memcpy(r->content + r->content_size, contents, realsize);
	r->content_size += realsize;
	r->content[r->content_size] = 0;
	return realsize;
}

void default_append(parameters* parameters, parameter* parameter) {
	parameter->next = NULL;
	if(NULL == parameters->parameter) {
		parameters->parameter = parameter;
	} else {
		parameter->next = parameters->parameter;
		parameters->parameter = parameter;
	}

	parameters->size++;
	parameters->memory_size += sizeof(char) * strlen(parameter->key);
	// one for = one for &
	parameters->memory_size += sizeof(char) * 2;
	parameters->memory_size += sizeof(char) * strlen(parameter->value);
}

char* default_to_buffer(parameters* parameters) {
	int size = parameters->memory_size + sizeof(char);
	char* buffer = malloc(size);
	if(NULL == buffer) {
		return NULL;
	}
	int position = 0;
	memset(buffer, 0x00, size);
	parameter* p = parameters->parameter;
	for(; NULL != p; p = p->next) {
		sprintf(buffer + position, "&%s=%s", p->key, p->value);
		position += sizeof(char) * strlen(p->key);
		position += sizeof(char) * 2;
		position += sizeof(char) * strlen(p->value);
	}
	return buffer;
}

//=========================================================
//			初始化工作队列
//=========================================================
static int initialize_work_queue() {
	g_work_queue = new_queue(g_config->max_queue_capacity);
	if(NULL == g_work_queue) {
		return ERROR_UNKNOWN;
	}
	return SUCCEED;
}

//=========================================================
//			初始化workers
//=========================================================
static int initialize_workers() {
	int ret = SUCCEED;
	// 分配内存
	g_workers =  (worker*)malloc(sizeof(worker) * g_config->max_workers);
	if(NULL == g_workers) {
		return ERROR_NO_MEMORY;
	}
	memset(g_workers, 0x00, sizeof(worker) * g_config->max_workers);

	// 创建threads
	int i = 0;
	int j = 0;
	for(; i < g_config->max_workers; i++) {
		ret = pthread_create(&(g_workers[i].pthread), NULL, work, &(g_workers[i]));
		if(SUCCEED != ret) {
			// 停止已经创建的线程
			j = 0;
			for(; j < i; j++) {
				g_workers[j].is_stoped = 1;
				pthread_cancel(g_workers[i].pthread);
			}

			free(g_workers);
			break;
		}
		sprintf(g_workers[i].name, "chttpclient.thread-%d", i);
	}

	return ret;
}

static int initialize_curl() {
	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	if(CURLE_OK == code) {
		return SUCCEED;
	}
	return curl_code_map_2_my_code(code);
}

//=========================================================
//			销毁工作者
//=========================================================
void destroy_workers() {
	int i = 0;
	for(; i < g_config->max_workers; i++) {
		g_workers[i].is_stoped = 1;
		pthread_cancel(g_workers[i].pthread);
	}
	free(g_workers);
}

//=========================================================
//			销毁工作队列
//=========================================================
void destroy_work_queue() {
	// 这里会free g_work_queue
	destroy_queue(g_work_queue);
	//free(g_work_queue);
}

void destroy_curl() {
	curl_global_cleanup();
}

int initialize(config* config) {

	int ret = SUCCEED;

	printf("chttpclient initializing ...\n");

	//
	g_config = config;

	ret = initialize_curl();
	if(SUCCEED != ret) {
		return ret;
	}
	printf("chttpclient config.is_support_async:%s ...\n", config->is_support_async ? "true" : "false");
	if(g_config->is_support_async) {
		// 初始化 work queue
		printf("chttpclient config.max_queue_capacity:%d ...\n", config->max_queue_capacity);
		ret = initialize_work_queue();
		if(SUCCEED != ret) {
			// 初始化work queue失败
			// 销毁curl环境
			destroy_curl();
			return ret;
		}
		// 初始化 workers
		printf("chttpclient config.max_workers:%d ...\n", config->max_workers);
		ret = initialize_workers();
		if(SUCCEED != ret) {
			// 初始化 workers失败
			// 销毁curl环境
			destroy_curl();
			// 销毁已经初始化的队列
			destroy_work_queue();
			return ret;
		}
	}

	printf("chttpclient initialized\n");

	return SUCCEED;
}

void release(config* config) {

	printf("chttpclient releasing ...\n");

	if(g_config->is_support_async) {
		// 销毁 workers
		printf("chttpclient destroy workers ...\n");
		destroy_workers();
		// 销毁 work queue
		printf("chttpclient destroy work queue ...\n");
		destroy_work_queue();
	}

	// 销毁curl环境
	destroy_curl();

	g_config = NULL;
	printf("chttpclient released\n");
}

request* new_request(char* url, http_method method, void* attachment) {
	return new_request_r(url, method, attachment, &default_reader);
}

request* new_request_r(char* url, http_method method, void* attachment, reader reader) {
	request* r = (request*)malloc(sizeof(request));
	if(NULL == r) {
		return NULL;
	}
	memset(r, 0x00, sizeof(request));
	r->url = url;
	r->method = method;
	r->reader = reader;
	r->attachment = attachment;

	return r;
}

int set_upload_file(request* request, char* key, char* file_name, char* data, int size) {

	upload_memory_file* file = malloc(sizeof(upload_memory_file));
	if(NULL == file) {
		return ERROR_NO_MEMORY;
	}
	memset(file, 0x00, sizeof(upload_memory_file));
	file->key = key;
	file->file_name = file_name;
	file->data = data;
	file->size = size;
	// 先释放以前的资源
	if(NULL != request->upload_file) {
		free(request->upload_file);
	}
	request->upload_file = file;
	return SUCCEED;
}

int append_query(request* request, char* key, char* value) {
	if(NULL == request->parameters) {
		request->parameters = (parameters*)malloc(sizeof(parameters));
		if(NULL == request->parameters) {
			return ERROR_NO_MEMORY;
		}
		memset(request->parameters, 0x00, sizeof(parameters));
		request->parameters->append = &default_append;
		request->parameters->to_buffer = &default_to_buffer;
	}
	//
	parameter* p = (parameter*)malloc(sizeof(parameter));
	if(NULL == p) {
		// 这里没有必要销毁 parameters, 它会随着request的销毁而销毁
		return ERROR_NO_MEMORY;
	}
	memset(p, 0x00, sizeof(parameter));
	p->key = key;
	p->value = value;

	request->parameters->append(request->parameters, p);

	return SUCCEED;
}

int append_header(request* request, char* key, char* value) {
	char* buffer = malloc(sizeof(char) * (strlen(key) + strlen(value) + 2));
	if(NULL == buffer) {
		return ERROR_NO_MEMORY;
	}
	sprintf(buffer, "%s:%s", key, value);
	request->headers = curl_slist_append(request->headers, buffer);
	return SUCCEED;
}

void destroy_request(request* request) {

	if(NULL != request->headers) {
		curl_slist_free_all(request->headers);
	}

	if(NULL != request->parameters) {
		parameter* p = request->parameters->parameter;
		for(; NULL != p; p = p->next) {
			free(p);
		}
		free(request->parameters);
	}

	if(NULL != request->upload_file) {
		free(request->upload_file);
	}
	free(request);
}

void destroy_response(response* response) {

	// TODO
	if(NULL != response->content) {
		free(response->content);
	}
	free(response);
}

response* sync_request(request* request) {
	return do_request(request);
}

int async_request(request* request, callback callback) {
	return async_request_timeout(request, callback, MAX_TIMEOUT);
}

int async_request_timeout(request* request, callback callback, int timeout) {
	// 设置回调
	request->callback = callback;
	// 构建队列元素
	element* e = (element*)malloc(sizeof(element));
	if(NULL == e) {
		return ERROR_NO_MEMORY;
	}
	memset(e, 0x00, sizeof(element));
	e->value = request;
	return enqueue_timeout(g_work_queue, e, timeout);
}
//=========================================================
//			线程方法
//=========================================================

void* work(void* arg) {
	request* r = NULL;
	worker* me = (worker*)arg;
	while(!me->is_stoped) {
		element* e = dequeue(g_work_queue);
		if(NULL != e) {
			r = (request*)e->value;
			// 释放队列元素占用的内存, 但是不会释放 e->value占用的内存的
			free(e);
			// 请求, 并回调
			r->callback(do_request(r));
		}
	}
	printf("worker:%s stoped\n", me->name);
	return NULL;
}

int curl_code_map_2_my_code(CURLcode code) {
	// TODO
	return code;
}

//====================================================================================
//					FOR DEBUG
//====================================================================================

struct data {
  char trace_ascii; /* 1 or 0 */
};

static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;

  unsigned int width=0x10;

  if(nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;

  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);

  for(i=0; i<size; i+= width) {

    fprintf(stream, "%4.4lx: ", (long)i);

    if(!nohex) {
      /* hex not disabled, show it */
      for(c = 0; c < width; c++)
        if(i+c < size)
          fprintf(stream, "%02x ", ptr[i+c]);
        else
          fputs("   ", stream);
    }

    for(c = 0; (c < width) && (i+c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if (nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
        i+=(c+2-width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if (nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
        i+=(c+3-width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}

static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  struct data *config = (struct data *)userp;
  const char *text;
  (void)handle; /* prevent compiler warning */

  switch (type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;

  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
  return 0;
}

response* do_request(request* request) {
	response* r = (response*)malloc(sizeof(response));
	if(NULL == r) {
		// 米有办法了
		return NULL;
	}
	memset(r, 0x00, sizeof(response));
	r->request = request;

	// http request
	CURL *curl = curl_easy_init();
	if(NULL == curl) {
		r->result_code = ERROR_NO_MEMORY;
	} else {
		// DEBUG
		struct data config;
		config.trace_ascii = 1; /* enable ascii tracing */
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
		curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);

		/* the DEBUGFUNCTION has no effect until we enable VERBOSE */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		curl_easy_setopt(curl, CURLOPT_URL, request->url);
		if(POST == request->method) {
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
		}

		// http header
		if(NULL != request->headers) {
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request->headers);
		}

		// set 查询参数
		char* url_buffer = NULL;
		char* query_buffer = NULL;
		struct curl_httppost* formpost = NULL;
		struct curl_httppost* lastptr  = NULL;

		if(POST == request->method || NULL != request->upload_file) {
			// query string
			/*if(NULL != request->parameters) {
				query_buffer = request->parameters->to_buffer(request->parameters);
				// GET 将参数拼接到url
				url_buffer = malloc(sizeof(char) * (strlen(query_buffer) + 2) + request->parameters->memory_size );
				if(NULL == url_buffer) {
					r->result_code = ERROR_NO_MEMORY;
				}
				if(strrchr(request->url, '?')) {
					sprintf(url_buffer, "%s%s", request->url, query_buffer);
				} else {
					sprintf(url_buffer, "%s?%s", request->url, query_buffer);
				}
				// set new url
				curl_easy_setopt(curl, CURLOPT_URL, url_buffer);
				printf("new url:%s\n", url_buffer);
			}*/
			if(NULL != request->parameters) {
				printf("try add parameters \n");
				parameter* p = request->parameters->parameter;
				for(; NULL != p; p = p->next) {
					printf("query string: %s=%s\n", p->key, p->value);
					curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, p->key, CURLFORM_COPYCONTENTS, p->value, CURLFORM_END);
				}
				printf("add parameters ok \n");
			}
			// upload file
			if(NULL != request->upload_file) {
				curl_formadd(&formpost,
				          &lastptr,
				          CURLFORM_COPYNAME, request->upload_file->key,
				          CURLFORM_BUFFER, request->upload_file->file_name,
				          CURLFORM_BUFFERPTR, request->upload_file->data,
				          CURLFORM_BUFFERLENGTH, request->upload_file->size,
				          CURLFORM_END);

				printf("set upload file ok\n");
			}
			if(NULL != formpost) {
				curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
			}
		} else {
			if(NULL != request->parameters) {
				// GET 将参数拼接到url
				url_buffer = malloc(sizeof(char) * (strlen(query_buffer) + 2) + request->parameters->memory_size );
				if(NULL == url_buffer) {
					r->result_code = ERROR_NO_MEMORY;
				}
				if(strrchr(request->url, '?')) {
					sprintf(url_buffer, "%s%s", request->url, query_buffer);
				} else {
					sprintf(url_buffer, "%s?%s", request->url, query_buffer);
				}
				// set new url
				curl_easy_setopt(curl, CURLOPT_URL, url_buffer);
				printf("new url:%s\n", url_buffer);
			}
		}

		/* send all data to this function  */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, request->reader);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, r);
		printf("try to do curl_easy_perform \n");
		CURLcode code = curl_easy_perform(curl);
		 if(CURLE_OK == code) {
			 r->result_code = SUCCEED;
			 code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &(r->http_response_code));
		 } else {
			 r->result_code = curl_code_map_2_my_code(code);
			 r->http_response_code = -1;
			 fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(code));
		 }

		 // 释放 query的 buffer
		 if(NULL != query_buffer) {
			 free(query_buffer);
		 }
		 if(NULL != url_buffer) {
			 free(url_buffer);
		 }
		 if(NULL != formpost) {
			 curl_formfree(formpost);
		 }
		/* if(NULL != lastptr) {
			 curl_formfree(lastptr);
		 }*/
		// clean
		curl_easy_cleanup(curl);
		printf("http request ok\n");
	}

	return r;
}
