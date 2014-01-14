/*
 * storage.h
 *
 *  Created on: Jan 13, 2014
 *      Author: sihai
 */

#ifndef STORAGE_H_
#define STORAGE_H_

#include "type.h"
#include "libchttpclient.h"

//============================================================================
//				数据结构
//============================================================================

/**
 * \brief          token of upload, awarded by mtop
 *
 * \member verison     		version of upload service
 * \member upload_type   	type of upload, 0 - resumable, 1 - normal
 * \member max_retry_times	max retry times
 * \member validate_type	file type, 0 - jpg, 1 - jpeg
 * \member size	file type	size of file
 * \member crc				crc of file content
 * \member expire		    expire time of token
 * \member file_id			file id
 * \member user_id			user id
 * \member client_net_type	network type of client
 * \member biz_code			code of biz, alloc by media center
 * \member private_data		private data
 *
 */
typedef struct token {

	char  verison;
	char  upload_type;
	int   max_retry_times;
	short validate_type;
	int   size;
	int   crc;
	long  expire;
	long  file_id;
	long  user_id;
	char  str_user_id[64];
	char  client_net_type[128];
	char  biz_code[128];
	char  private_data[128];
} token;

//============================================================================
//				异步回调
//============================================================================

/**
 * \brief          callback of async store request
 *
 * \member token
 * \member response
 *
 */
typedef void(*storage_callback)(token* token, response* response);

/**
 * \brief          parse token
 *
 * \param buffer   data
 * \param size     buffer length
 *
 */
token* parse_token(char* buffer, int size);

/**
 * \brief          token of upload, awarded by mtop
 *
 * \member token
 * \param  file_name
 * \member file_data
 * \member file_size
 *
 */
response* sync_store_2_meida_center(token* token, char* file_name, char* file_data, int file_size);

/**
 * \brief          token of upload, awarded by mtop
 *
 * \member token
 * \member file_data
 * \member file_size
 * \member callback
 *
 */
void async_store_2_meida_center(token* token, char* file_data, int file_size, storage_callback callback);

#endif /* STORAGE_H_ */
