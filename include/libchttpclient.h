#ifndef libasynchttpclient_H
#define libasynchttpclient_H

#include "type.h"

//============================================================================
//				常量
//============================================================================
#define SUPPORTED_ASYNC				1
#define NOT_SUPPORTED_ASYNC			0
#define DEFAULT_MAX_QUEUE_CAPACITY  32
#define DEFAULT_MAX_WORKERS 		4
#define DEFAULT_TIMEOUT 			3000

//============================================================================
//				错误码
//============================================================================
#define SUCCEED						 0			// 成功
#define ERROR_UNKNOWN 				-1			// 未知
#define ERROR_NO_MEMORY 			-2			// 分配内存失败
#define ERROR_BUFFER_TOO_SMALL		-3			//
#define ERROR_BASE64_INVALID_CHARACTER -4		//
#define ERROR_AES_INVALID_KEY_LENGTH -5			//
#define ERROR_AES_INVALID_INPUT_LENGTH -6		//

//============================================================================
//				数据结构
//============================================================================

typedef enum http_method {
	GET = 0,
	POST
} http_method;

// 配置数据结构
typedef struct config {

	int is_support_async;					// 是否支持异步

	int max_queue_capacity;					// 队列容量

	int max_workers;						// 工作者大小

	int timeout;							// 超时
} config;

typedef struct upload_memory_file {

	char* key;
	char* file_name;
	char* data;
	long   size;
} upload_memory_file;

//============================================================================
//				回调函数
//============================================================================
typedef struct request request;
typedef struct response response;

typedef void (*callback)(response*);

//============================================================================
//					查询参数
//============================================================================
typedef struct parameters parameters;

// 单个查询参数 key = value, 已经urlencode的
typedef struct parameter {
	char* key;
	char* value;

	struct parameter* next;
} parameter;

typedef void (*append)(struct parameters*, parameter*);
typedef char* (*to_buffer)(struct parameters*);

// 查询参数

typedef struct parameters {
	parameter* parameter;			// kv链表
	int size;						// 参数个数
	int memory_size;				// 需要的buffer size

	append append;					// 添加参数函数
	to_buffer to_buffer;			// 转换为char[]函数
} parameters;

//============================================================================
//					读入http响应
//============================================================================
typedef size_t (*reader)(void *contents, size_t size, size_t nmemb, void *userp);

// 请求数据结构
typedef struct request {

	int   id;													//

	char* url;													//
	http_method method;											//

	struct curl_slist* headers;									// http headers
	parameters* parameters;										// 查询参数

	upload_memory_file* upload_file;							// 上传文件, 目前只支持一个, 并且在内存中

	reader reader;												// 读入http响应
	callback callback;											// 回调

	void* attachment;											// 附件
} request;

// 请求数据结构
typedef struct response {

	request* request;											//

	int result_code;											//
	int http_response_code;

	int content_size;											//
	char*    content;											//

} response;

//============================================================================
//				初始化函数
//============================================================================
extern int initialize(config* config);

//============================================================================
//				创建一个请求, caller负责释放
//	@param url
//	@param method
//  @param attachment
//============================================================================
request* new_request(char* url, http_method method, void* attachment);

//============================================================================
//				创建一个请求, caller负责释放
//	@param url
//  @param attachment
//	@param method
//  @param reader
//============================================================================
request* new_request_r(char* url, http_method method, void* attachment, reader reader);

//============================================================================
//				设置上传文件
//	@param request
//  @param key
//  @param file_name
//	@param data
//  @param size
//============================================================================
int set_upload_file(request* request, char* key, char* file_name, char* data, int size);

//============================================================================
//				向请求中添加查询参数, 目前只支持字符串类参数
//	@param request
//  @param key
//  @param value
//============================================================================
int append_query(request* request, char* key, char* value);

//============================================================================
//				向请求中添加http header, 目前只支持字符串类参数
//	@param request
//  @param key
//  @param value
//============================================================================
int append_header(request* request, char* key, char* value);

//============================================================================
//				销毁由new_request或new_request_r创建的请求
//	@param request
//============================================================================
void destroy_request(request* request);

//============================================================================
//				销毁由sync_request或async_request或async_request_timeout创建的响应
//	@param response
//============================================================================
void destroy_response(response* response);

//============================================================================
//				同步发起http请求
//	@param request
//============================================================================
extern response* sync_request(request* request);

//============================================================================
//				异步发起http请求
//			目前是线程堆的, 排队
//	@param request
//	@param callback
//============================================================================
extern int async_request(request* request, callback callback);

//============================================================================
//				异步发起http请求
//			目前是线程堆的, 排队
//	@param request
//	@param callback
//	@param timeout
//============================================================================
extern int async_request_timeout(request* request, callback callback, int timeout);

//============================================================================
//				释放资源函数
//============================================================================
extern void release();

//============================================================================
//				工具函数
//============================================================================
extern int base64_encode( unsigned char* buffer, size_t* buffer_length, const unsigned char* src, size_t src_length );
extern int base64_decode( unsigned char* buffer, size_t* buffer_length, const unsigned char* src, size_t src_length );

#endif
