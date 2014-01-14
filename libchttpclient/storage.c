/*
 * storage.c
 *
 *  Created on: Jan 13, 2014
 *      Author: sihai
 */

#include "type.h"
#include "md5.h"
#include "aes.h"
#include "storage.h"

typedef struct attachment {

	token* token;
	char*  file_name;
	char*  file_data;
	int    file_size;
} attachment;

static char* g_media_center_gateway = "http://media.daily.taobao.net/upload/upload.htm";

static char g_token_seeds[16] = { 'Q', 'U', 'C', 'E', 'K', 'O', 'D', 'W', 'J', 'R', 'T', 'L', 'Z', 'S', 'H', 'I' };
static int g_token_seeds_size = 16;

static char* generate_meida_center_token(token* token) {

	int i = 0;
	int v = 0;
	// md5
	time_t rawtime;
	char buf[64];
	char buffer[16];
	time(&rawtime);
	sprintf(buf, "%ld", token->user_id + rawtime + 300);
	md5(buf, sizeof(char) * strlen(buf), buffer);
	char* t = malloc(sizeof(char) * 65);
	if(NULL == t) {
		// 内存分配失败
		return NULL;
	}
	memset(t, 0x00, sizeof(char) * 65);
	char* p = buffer;
	char* q = t;
	for(; i < 16; i++) {
		v = (*p < 0) ? -(*p) : *p;
		//printf("v=%d\n", v);
		printf("v=%d\n", *p);
		*q = g_token_seeds[v % g_token_seeds_size];
		p += 1;
		q += 1;
	}

	printf("t:%s\n", t);
	sprintf(t, "%s%ld", t, ((rawtime) + 300));
	printf("str_user_id=%s, md5_src=%s,  md5r=%s, token:%s\n", token->str_user_id, buf, buffer, t);
	return t;
}

static request* storage_new_request(token* token, char* file_name, char* file_data, int file_size) {

	// 创建一个 post 请求
	request* request = new_request(g_media_center_gateway, POST, NULL);

	// 设置要上传的文件
	set_upload_file(request, "file", file_name, file_data, file_size);

	int ret = SUCCEED;
	// append 查询参数
	ret = append_query(request, "token", generate_meida_center_token(token));
	ret = append_query(request, "bizCode", token->biz_code);
	ret = append_query(request, "userId", token->str_user_id);
	return request;
}

static uint16_t read_short(char** p) {
	uint16_t v = 0;
	v |= *(*p);
	v |= *(*p+1) << 8;
	*p += 2;
	return v;
}

static uint32_t read_int(char** p) {
	uint32_t v = 0;
	v |= *(*p);
	v |= *(*p + 1) << 8;
	v |= *(*p + 2) << 16;
	v |= *(*p + 3) << 24;
	*p += 4;
	return v;
}

static uint64_t read_long(char** p) {
	uint64_t v = 0;
	v |= (uint64_t)*(*p);
	v |= ((uint64_t)*(*p + 1) & 255) << 8;
	v |= ((uint64_t)*(*p + 2) & 255) << 16;
	v |= ((uint64_t)*(*p + 3) & 255) << 24;
	v |= ((uint64_t)*(*p + 4) & 255) << 32;
	v |= ((uint64_t)*(*p + 5) & 255) << 40;
	v |= ((uint64_t)*(*p + 6) & 255) << 48;
	v |= ((uint64_t)*(*p + 7) & 255) << 56;
	*p += 8;
	return v;
}

token* parse_token(char* buffer, int size) {

	char tlen = 0;
	size_t length = 128;
	char tmp_buffer[128];

	base64_decode(tmp_buffer, &length, (const unsigned char*)buffer, size);
	printf("decode:%s\n, length:%d\n", tmp_buffer, length);

	token* t = (token*)malloc(sizeof(token));
	if(NULL == t) {
		// NO Memory
		return NULL;
	}
	memset((void*)t, 0x00, sizeof(token));

	// aes 解密
	//aes_crypt_cbc();
	char* p = tmp_buffer + 1;

	t->verison = tmp_buffer[0];
	t->upload_type = *p;
	p++;
	t->max_retry_times = read_int(&p);
	t->validate_type = read_short(&p);
	t->size = read_int(&p);
	t->crc = read_int(&p);
	t->expire = read_long(&p);
	t->file_id = read_long(&p);
	t->user_id = read_long(&p);

	// 变长的了
	// client_net_type
	/*tlen = *p;
	p++;
	if(tlen > 0) {
		memcpy(t->client_net_type, p, tlen);
	}
	p += tlen;*/

	// biz_code
	tlen = *p;
	p++;
	if(tlen > 0) {
		memcpy(t->biz_code, p, tlen);
	}
	p += tlen;

	// private_data
	tlen = *p;
	p++;
	if(tlen > 0) {
		memcpy(t->private_data, p, tlen);
	}
	p += tlen;

	sprintf(t->str_user_id, "%ld", t->user_id);

	return t;
}

response* sync_store_2_meida_center(token* token, char* file_name, char* file_data, int file_size) {

	request* request = storage_new_request(token, file_name, file_data, file_size);
	response* response = sync_request(request);
	destroy_request(request);
	return response;
}

void async_store_2_meida_center(token* token, char* file_data, int file_size, storage_callback callback) {

}

