#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <pthread.h>
#include <stdio.h>

#include "type.h"
#include "libchttpclient.h"
#include "aes.h"
#include "md5.h"
#include "storage.h"

int total = 1024;
pthread_mutex_t g_lock;					//
pthread_cond_t  g_condition;			//

void test_callback(response* response) {
	//printf("async request:%s, result:%s\n", response->request->url, response->content);

	// 释放请求的资源
	destroy_request(response->request);
	destroy_response(response);
	// 通知完成了
	pthread_mutex_lock(&(g_lock));
	if(0 == --total) {
		pthread_cond_signal(&(g_condition));
	}
	printf("total:%d\n", total);
	pthread_mutex_unlock(&(g_lock));
}

void init_env() {
	if(pthread_mutex_init(&(g_lock), NULL)) {
		printf("init pthread_mutex_t g_lock failed, exit\n");
	}
	if(pthread_cond_init(&(g_condition), NULL)) {
		pthread_mutex_destroy(&(g_lock));
		printf("init pthread_cond_init g_condition failed, exit\n");
	}
}

void clear_env() {
	pthread_mutex_destroy(&(g_lock));
	pthread_cond_destroy(&(g_condition));
}

//=========================================================
//
//=========================================================
long current_time() {
   struct timeval tv;
   gettimeofday(&tv,NULL);
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

//=========================================================
//			同步测试
//=========================================================
void test_sync() {

	init_env();

	// config
	config config;
	config.is_support_async = NOT_SUPPORTED_ASYNC;
	config.max_queue_capacity = DEFAULT_MAX_QUEUE_CAPACITY;
	config.max_workers = DEFAULT_MAX_WORKERS;
	config.timeout = DEFAULT_TIMEOUT;

	// 初始化
	int ret = initialize(&config);
	if(SUCCEED != ret) {
		printf("init libchttpclient failed, exit\n");
		exit(ret);
	}

	request* request = new_request("http://www.baidu.com", GET, NULL);
	append_query(request, "ss", "sihai");
	append_query(request, "yy", "12345678");
	append_header(request, "User-Agent", "Mozilla/5.0 (X11; Linux i686) AppleWebKit/537.1 (KHTML, like Gecko) Chrome/21.0.1180.89 Safari/537.1");

	// sync
	response* response = sync_request(request);
	printf("sync request:%s, result:%s\n", request->url, response->content);
	// 释放请求的资源
	destroy_response(response);
	destroy_request(request);

	request = new_request("http://www.taobao.com", POST, NULL);
	append_query(request, "ss", "sihai");
	append_query(request, "yy", "12345678");
	append_header(request, "User-Agent", "Mozilla/5.0 (X11; Linux i686) AppleWebKit/537.1 (KHTML, like Gecko) Chrome/21.0.1180.89 Safari/537.1");

	// sync
	response = sync_request(request);
	//printf("sync request:%s, result:%s\n", request->url, response->content);
	// 释放请求的资源
	destroy_response(response);
	destroy_request(request);

	//async_request(request, &test_callback);

	//pthread_cond_wait(&(g_condition), &(g_lock));

	// 释放资源
	release();

	clear_env();
	printf("completed, exit\n");
}

//=========================================================
//			异步测试
//=========================================================
void test_async() {

	int ret = SUCCEED;
	init_env();

	// config
	config config;
	config.is_support_async = SUPPORTED_ASYNC;
	config.max_queue_capacity = DEFAULT_MAX_QUEUE_CAPACITY;
	config.max_workers = DEFAULT_MAX_WORKERS;
	config.timeout = DEFAULT_TIMEOUT;

	// 初始化
	ret = initialize(&config);
	if(SUCCEED != ret) {
		printf("init libchttpclient failed, exit\n");
		exit(ret);
	}

	int i = 0;
	int count = (total / 2);
	for(; i < count; i++) {
		request* request = new_request("http://www.baidu.com", GET, NULL);
		append_query(request, "ss", "sihai");
		append_query(request, "yy", "12345678");
		ret = async_request(request, &test_callback);
		if(SUCCEED != ret) {
			printf("async_request failed, code:%d\n", ret);
			break;
		}

		request = new_request("http://www.taobao.com", POST, NULL);
		append_query(request, "ss", "sihai");
		append_query(request, "yy", "12345678");
		ret = async_request(request, &test_callback);
		if(SUCCEED != ret) {
			printf("async_request failed, code:%d\n", ret);
			break;
		}
	}

	printf("async_request commited, i:%d\n", i);

	pthread_cond_wait(&(g_condition), &(g_lock));

	// 释放资源
	release();

	// 释放环境
	clear_env();
	printf("completed, exit\n");
}

//=========================================================
//			base64测试
//=========================================================
void test_base64() {
	size_t length = 128;
	int ret = SUCCEED;
	char buffer[128];
	char buf[128];
	char* text = "四海你好啊，我好，很好，特别的好啊！";

	// 编码
	ret = base64_encode((unsigned char*)buffer, &length, (const unsigned char*)text, sizeof(char) * strlen(text));
	if(SUCCEED != ret) {
		printf("base64_encode failed, code:%d\n", ret);
		return;
	}
	printf("base64_encode result:%s, length:%d\n", buffer, length);

	// 解码
	ret = base64_decode((unsigned char*)buf, &length, buffer, length);
	if(SUCCEED != ret) {
		printf("base64_decode failed, code:%d\n", ret);
		return;
	}
	printf("base64_decode result:%s, length:%d\n", buf, length);

	if(strcmp(text, buf)) {
		printf("base64 wrong\n");
	}
}

//=========================================================
//			aes测试
//=========================================================

static const unsigned char aes_test_ecb_dec[3][16] =
{
    { 0x44, 0x41, 0x6A, 0xC2, 0xD1, 0xF5, 0x3C, 0x58,
      0x33, 0x03, 0x91, 0x7E, 0x6B, 0xE9, 0xEB, 0xE0 },
    { 0x48, 0xE3, 0x1E, 0x9E, 0x25, 0x67, 0x18, 0xF2,
      0x92, 0x29, 0x31, 0x9C, 0x19, 0xF1, 0x5B, 0xA4 },
    { 0x05, 0x8C, 0xCF, 0xFD, 0xBB, 0xCB, 0x38, 0x2D,
      0x1F, 0x6F, 0x56, 0x58, 0x5D, 0x8A, 0x4A, 0xDE }
};

static const unsigned char aes_test_ecb_enc[3][16] =
{
    { 0xC3, 0x4C, 0x05, 0x2C, 0xC0, 0xDA, 0x8D, 0x73,
      0x45, 0x1A, 0xFE, 0x5F, 0x03, 0xBE, 0x29, 0x7F },
    { 0xF3, 0xF6, 0x75, 0x2A, 0xE8, 0xD7, 0x83, 0x11,
      0x38, 0xF0, 0x41, 0x56, 0x06, 0x31, 0xB1, 0x14 },
    { 0x8B, 0x79, 0xEE, 0xCC, 0x93, 0xA0, 0xEE, 0x5D,
      0xFF, 0x30, 0xB4, 0xEA, 0x21, 0x63, 0x6D, 0xA4 }
};

static const unsigned char aes_test_cbc_dec[3][16] =
{
    { 0xFA, 0xCA, 0x37, 0xE0, 0xB0, 0xC8, 0x53, 0x73,
      0xDF, 0x70, 0x6E, 0x73, 0xF7, 0xC9, 0xAF, 0x86 },
    { 0x5D, 0xF6, 0x78, 0xDD, 0x17, 0xBA, 0x4E, 0x75,
      0xB6, 0x17, 0x68, 0xC6, 0xAD, 0xEF, 0x7C, 0x7B },
    { 0x48, 0x04, 0xE1, 0x81, 0x8F, 0xE6, 0x29, 0x75,
      0x19, 0xA3, 0xE8, 0x8C, 0x57, 0x31, 0x04, 0x13 }
};

static const unsigned char aes_test_cbc_enc[3][16] =
{
    { 0x8A, 0x05, 0xFC, 0x5E, 0x09, 0x5A, 0xF4, 0x84,
      0x8A, 0x08, 0xD3, 0x28, 0xD3, 0x68, 0x8E, 0x3D },
    { 0x7B, 0xD9, 0x66, 0xD5, 0x3A, 0xD8, 0xC1, 0xBB,
      0x85, 0xD2, 0xAD, 0xFA, 0xE8, 0x7B, 0xB1, 0x04 },
    { 0xFE, 0x3C, 0x53, 0x65, 0x3E, 0x2F, 0x45, 0xB5,
      0x6F, 0xCD, 0x88, 0xB2, 0xCC, 0x89, 0x8F, 0xF0 }
};

/*
 * AES-CFB128 test vectors from:
 *
 * http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf
 */
static const unsigned char aes_test_cfb128_key[3][32] =
{
    { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
      0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C },
    { 0x8E, 0x73, 0xB0, 0xF7, 0xDA, 0x0E, 0x64, 0x52,
      0xC8, 0x10, 0xF3, 0x2B, 0x80, 0x90, 0x79, 0xE5,
      0x62, 0xF8, 0xEA, 0xD2, 0x52, 0x2C, 0x6B, 0x7B },
    { 0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE,
      0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
      0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7,
      0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4 }
};

static const unsigned char aes_test_cfb128_iv[16] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const unsigned char aes_test_cfb128_pt[64] =
{
    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
    0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A,
    0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03, 0xAC, 0x9C,
    0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51,
    0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11,
    0xE5, 0xFB, 0xC1, 0x19, 0x1A, 0x0A, 0x52, 0xEF,
    0xF6, 0x9F, 0x24, 0x45, 0xDF, 0x4F, 0x9B, 0x17,
    0xAD, 0x2B, 0x41, 0x7B, 0xE6, 0x6C, 0x37, 0x10
};

static const unsigned char aes_test_cfb128_ct[3][64] =
{
    { 0x3B, 0x3F, 0xD9, 0x2E, 0xB7, 0x2D, 0xAD, 0x20,
      0x33, 0x34, 0x49, 0xF8, 0xE8, 0x3C, 0xFB, 0x4A,
      0xC8, 0xA6, 0x45, 0x37, 0xA0, 0xB3, 0xA9, 0x3F,
      0xCD, 0xE3, 0xCD, 0xAD, 0x9F, 0x1C, 0xE5, 0x8B,
      0x26, 0x75, 0x1F, 0x67, 0xA3, 0xCB, 0xB1, 0x40,
      0xB1, 0x80, 0x8C, 0xF1, 0x87, 0xA4, 0xF4, 0xDF,
      0xC0, 0x4B, 0x05, 0x35, 0x7C, 0x5D, 0x1C, 0x0E,
      0xEA, 0xC4, 0xC6, 0x6F, 0x9F, 0xF7, 0xF2, 0xE6 },
    { 0xCD, 0xC8, 0x0D, 0x6F, 0xDD, 0xF1, 0x8C, 0xAB,
      0x34, 0xC2, 0x59, 0x09, 0xC9, 0x9A, 0x41, 0x74,
      0x67, 0xCE, 0x7F, 0x7F, 0x81, 0x17, 0x36, 0x21,
      0x96, 0x1A, 0x2B, 0x70, 0x17, 0x1D, 0x3D, 0x7A,
      0x2E, 0x1E, 0x8A, 0x1D, 0xD5, 0x9B, 0x88, 0xB1,
      0xC8, 0xE6, 0x0F, 0xED, 0x1E, 0xFA, 0xC4, 0xC9,
      0xC0, 0x5F, 0x9F, 0x9C, 0xA9, 0x83, 0x4F, 0xA0,
      0x42, 0xAE, 0x8F, 0xBA, 0x58, 0x4B, 0x09, 0xFF },
    { 0xDC, 0x7E, 0x84, 0xBF, 0xDA, 0x79, 0x16, 0x4B,
      0x7E, 0xCD, 0x84, 0x86, 0x98, 0x5D, 0x38, 0x60,
      0x39, 0xFF, 0xED, 0x14, 0x3B, 0x28, 0xB1, 0xC8,
      0x32, 0x11, 0x3C, 0x63, 0x31, 0xE5, 0x40, 0x7B,
      0xDF, 0x10, 0x13, 0x24, 0x15, 0xE5, 0x4B, 0x92,
      0xA1, 0x3E, 0xD0, 0xA8, 0x26, 0x7A, 0xE2, 0xF9,
      0x75, 0xA3, 0x85, 0x74, 0x1A, 0xB9, 0xCE, 0xF8,
      0x20, 0x31, 0x62, 0x3D, 0x55, 0xB1, 0xE4, 0x71 }
};

/*
 * AES-CTR test vectors from:
 *
 * http://www.faqs.org/rfcs/rfc3686.html
 */

static const unsigned char aes_test_ctr_key[3][16] =
{
    { 0xAE, 0x68, 0x52, 0xF8, 0x12, 0x10, 0x67, 0xCC,
      0x4B, 0xF7, 0xA5, 0x76, 0x55, 0x77, 0xF3, 0x9E },
    { 0x7E, 0x24, 0x06, 0x78, 0x17, 0xFA, 0xE0, 0xD7,
      0x43, 0xD6, 0xCE, 0x1F, 0x32, 0x53, 0x91, 0x63 },
    { 0x76, 0x91, 0xBE, 0x03, 0x5E, 0x50, 0x20, 0xA8,
      0xAC, 0x6E, 0x61, 0x85, 0x29, 0xF9, 0xA0, 0xDC }
};

static const unsigned char aes_test_ctr_nonce_counter[3][16] =
{
    { 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
    { 0x00, 0x6C, 0xB6, 0xDB, 0xC0, 0x54, 0x3B, 0x59,
      0xDA, 0x48, 0xD9, 0x0B, 0x00, 0x00, 0x00, 0x01 },
    { 0x00, 0xE0, 0x01, 0x7B, 0x27, 0x77, 0x7F, 0x3F,
      0x4A, 0x17, 0x86, 0xF0, 0x00, 0x00, 0x00, 0x01 }
};

static const unsigned char aes_test_ctr_pt[3][48] =
{
    { 0x53, 0x69, 0x6E, 0x67, 0x6C, 0x65, 0x20, 0x62,
      0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x6D, 0x73, 0x67 },

    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F },

    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
      0x20, 0x21, 0x22, 0x23 }
};

static const unsigned char aes_test_ctr_ct[3][48] =
{
    { 0xE4, 0x09, 0x5D, 0x4F, 0xB7, 0xA7, 0xB3, 0x79,
      0x2D, 0x61, 0x75, 0xA3, 0x26, 0x13, 0x11, 0xB8 },
    { 0x51, 0x04, 0xA1, 0x06, 0x16, 0x8A, 0x72, 0xD9,
      0x79, 0x0D, 0x41, 0xEE, 0x8E, 0xDA, 0xD3, 0x88,
      0xEB, 0x2E, 0x1E, 0xFC, 0x46, 0xDA, 0x57, 0xC8,
      0xFC, 0xE6, 0x30, 0xDF, 0x91, 0x41, 0xBE, 0x28 },
    { 0xC1, 0xCF, 0x48, 0xA8, 0x9F, 0x2F, 0xFD, 0xD9,
      0xCF, 0x46, 0x52, 0xE9, 0xEF, 0xDB, 0x72, 0xD7,
      0x45, 0x40, 0xA4, 0x2B, 0xDE, 0x6D, 0x78, 0x36,
      0xD5, 0x9A, 0x5C, 0xEA, 0xAE, 0xF3, 0x10, 0x53,
      0x25, 0xB2, 0x07, 0x2F }
};

static const int aes_test_ctr_len[3] =
    { 16, 32, 36 };

void test_aes() {

	int i, j, u, v;
	unsigned char key[32];
	unsigned char buf[64];
	unsigned char iv[16];
	unsigned char prv[16];
	size_t offset;
	int len;
	unsigned char nonce_counter[16];
	unsigned char stream_block[16];
	aes_context ctx;

	memset( key, 0, 32 );

	/*
	 * ECB mode
	 */
	for (i = 0; i < 6; i++) {
		u = i >> 1;
		v = i & 1;

		printf("  AES-ECB-%3d (%s): ", 128 + u * 64, (v == AES_DECRYPT) ? "dec" : "enc");

		memset(buf, 0, 16);

		if (v == AES_DECRYPT) {
			aes_setkey_dec(&ctx, key, 128 + u * 64);

			for (j = 0; j < 10000; j++)
				aes_crypt_ecb(&ctx, v, buf, buf);

			if (memcmp(buf, aes_test_ecb_dec[u], 16) != 0) {
				printf("failed\n");
				return;
			}
		} else {
			aes_setkey_enc(&ctx, key, 128 + u * 64);

			for (j = 0; j < 10000; j++)
				aes_crypt_ecb(&ctx, v, buf, buf);

			if (memcmp(buf, aes_test_ecb_enc[u], 16) != 0) {
				printf("failed\n");
				return;
			}
		}

		printf("passed\n");
	}

	 /*
	 * CBC mode
	 */
	for (i = 0; i < 6; i++) {
		u = i >> 1;
		v = i & 1;

		printf("  AES-CBC-%3d (%s): ", 128 + u * 64,(v == AES_DECRYPT) ? "dec" : "enc");

		memset(iv, 0, 16);
		memset(prv, 0, 16);
		memset(buf, 0, 16);

		if (v == AES_DECRYPT) {
			aes_setkey_dec(&ctx, key, 128 + u * 64);

			for (j = 0; j < 10000; j++)
				aes_crypt_cbc(&ctx, v, 16, iv, buf, buf);

			if (memcmp(buf, aes_test_cbc_dec[u], 16) != 0) {
				printf("failed\n");
				return;
			}
		} else {
			aes_setkey_enc(&ctx, key, 128 + u * 64);

			for (j = 0; j < 10000; j++) {
				unsigned char tmp[16];

				aes_crypt_cbc(&ctx, v, 16, iv, buf, buf);

				memcpy(tmp, prv, 16);
				memcpy(prv, buf, 16);
				memcpy(buf, tmp, 16);
			}

			if (memcmp(prv, aes_test_cbc_enc[u], 16) != 0) {
				printf("failed\n");
				return;
			}
		}

		printf("passed\n");
	}

	/*
	 * CFB128 mode
	 */
	for (i = 0; i < 6; i++) {
		u = i >> 1;
		v = i & 1;

		printf("  AES-CFB128-%3d (%s): ", 128 + u * 64, (v == AES_DECRYPT) ? "dec" : "enc");

		memcpy(iv, aes_test_cfb128_iv, 16);
		memcpy(key, aes_test_cfb128_key[u], 16 + u * 8);

		offset = 0;
		aes_setkey_enc(&ctx, key, 128 + u * 64);

		if (v == AES_DECRYPT) {
			memcpy(buf, aes_test_cfb128_ct[u], 64);
			aes_crypt_cfb128(&ctx, v, 64, &offset, iv, buf, buf);

			if (memcmp(buf, aes_test_cfb128_pt, 64) != 0) {
				printf("failed\n");
				return;
			}
		} else {
			memcpy(buf, aes_test_cfb128_pt, 64);
			aes_crypt_cfb128(&ctx, v, 64, &offset, iv, buf, buf);

			if (memcmp(buf, aes_test_cfb128_ct[u], 64) != 0) {
				printf("failed\n");
				return;
			}
		}

		printf("passed\n");
	}

	/*
	 * CTR mode
	 */
	for (i = 0; i < 6; i++) {
		u = i >> 1;
		v = i & 1;

		printf("  AES-CTR-128 (%s): ", (v == AES_DECRYPT) ? "dec" : "enc");

		memcpy(nonce_counter, aes_test_ctr_nonce_counter[u], 16);
		memcpy(key, aes_test_ctr_key[u], 16);

		offset = 0;
		aes_setkey_enc(&ctx, key, 128);

		if (v == AES_DECRYPT) {
			len = aes_test_ctr_len[u];
			memcpy(buf, aes_test_ctr_ct[u], len);

			aes_crypt_ctr(&ctx, len, &offset, nonce_counter, stream_block, buf, buf);

			if (memcmp(buf, aes_test_ctr_pt[u], len) != 0) {
				printf("failed\n");
				return;
			}
		} else {
			len = aes_test_ctr_len[u];
			memcpy(buf, aes_test_ctr_pt[u], len);

			aes_crypt_ctr(&ctx, len, &offset, nonce_counter, stream_block, buf,buf);

			if (memcmp(buf, aes_test_ctr_ct[u], len) != 0) {
				printf("failed\n");
				return;
			}
		}

		printf("passed\n");
	}

	printf("\n");

}

void test_really() {

	int ret = SUCCEED;
	int offset = 0;
	char* version = "1.0";
	aes_context ctx;
	size_t length = 512;
	char buffer[512];
	char buf[512];
	unsigned char iv[16] = "0102030405060708";
	char* key = "ghlbOVdKdJz(oOKd";
	char* token = "MS4wKzgranBnKzEwMjQrMTIzKzEzODkzMzcyNjY0OTArM2M2M2ViNTYtZjNjYS00NGM0LWFiN2ItZDQ4NWRhMjAwYzdlK210b3BfdXBsb2FkK251bGwrYXBwa2V5PTEyMw==";
	// base64 decode
	// 解码
	memset(buffer, 0x00, 512);
	memset(buf, 0x00, 512);
	ret = base64_decode((unsigned char*)buf, &length, token, sizeof(char) * strlen(token));
	if(SUCCEED != ret) {
		printf("base64_decode failed, code:%d\n", ret);
		return;
	}
	printf("base64_decode result:%s, length:%d\n", buf, length);

	// ase 解密
	printf("key length:%d\n", sizeof(char) * strlen(key));
	ret = aes_setkey_dec(&ctx, key, 128);
	if(SUCCEED != ret) {
		printf("aes_setkey_dec failed, errorCode:%d\n", ret);
		exit(ret);
	}
	offset += sizeof(char) * (strlen(version) + 1);
	printf("offset:%d\n", offset);
	char* p = buf + offset;
	int i = 0;
	for(; i < length - offset; i++, p++) {
		if(*p >= 1 && *p <= 16) {
			*p = 0;
		}
	}
	ret = aes_crypt_cbc(&ctx, AES_DECRYPT, length - offset, iv, buf + offset, buffer);
	if(SUCCEED != ret) {
		printf("decrypt failed, errorCode:%d\n", ret);
	} else {
		printf("decrypt result:%s\n", buffer);
	}

}

char* get_data_from_file(char* file_name) {
	int size = 0;
	int readed = 0;
	FILE *fp = NULL;

	fp = fopen(file_name, "r");
	if(NULL == fp) {
		printf("open file:%s failed", file_name);
		exit(-1);
	}

	if(fseek(fp, 0, SEEK_END)) {
		printf("fseek failed\n");
		exit(-1);
	}
	size = ftell(fp);
	if(fseek(fp, 0, SEEK_SET)) {
		printf("fseek failed\n");
		exit(-1);
	}
	char* data = malloc(4 + size);
	if(NULL == data) {
		printf("alloc memory failed\n");
		exit(-1);
	}
	memset(data, 0x00, 4 + size);

	*((int*) data) = size;

	fread(data + 4, 1, size, fp);
	fclose(fp);
	return data;
}

void test_storage() {

	token token;
	sprintf(token.biz_code, "mtopupload");
	token.crc = 123;
	token.expire = 1000;
	token.file_id = 123;
	token.max_retry_times = 2;
	sprintf(token.private_data, "appkey=1234");
	token.size = 1024;
	sprintf(token.str_user_id, "2026322467");
	token.user_id = 2026322467;
	token.upload_type = 0;
	token.validate_type = 0;
	token.verison = 1;

	char* data = get_data_from_file("/home/sihai/test.jpg");
	printf("try to sync request storage\n");
	long start = current_time();
	response* response = sync_store_2_meida_center(&token, "test.jpg", data + 4, *((int*)data));
	printf("consume: %d ms\n", current_time() - start);
	printf("response:%s\n", response->content);
	destroy_response(response);
	printf("sync request storage ok\n");
	free(data);
}

void test_parse_token() {
	char* txt = "AQAIAAAAAAAABAAAAAQAAOHsro9DAQAAC33GyP////8jOsd4AAAAAAQzZ3BwC210b3BfdXBsb2FkEGFwcGtleT0xMjMmaz0zMjE=";
	token* t = parse_token(txt, sizeof(char) * strlen(txt));
	printf("==================token=====================\n");
	printf("token->verison: %d\n", t->verison);
	printf("token->upload_type: %d\n", t->upload_type);
	printf("token->max_retry_times: %d\n", t->max_retry_times);
	printf("token->validate_type: %d\n", t->validate_type);
	printf("token->size: %d\n", t->size);
	printf("token->crc: %d\n", t->crc);
	printf("token->expire: %ld\n", t->expire);
	printf("token->file_id: %ld\n", t->file_id);
	printf("token->user_id: %ld\n", t->user_id);
	printf("token->str_user_id: %s\n", t->str_user_id);
	printf("token->biz_code: %s\n", t->biz_code);
	printf("token->private_data: %s\n", t->private_data);

	printf("==================token=====================\n");
	free(t);
}

void main(char** args) {
	// test sync
	//test_sync();
	// test_async
	//test_async();
	// test base64
	//test_base64();
	// test aes
	//test_aes();

	// test really
	//test_really();

	// test storage
	test_storage();

	//test_parse_token();
}

