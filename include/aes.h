/*
 * aes.h
 *
 *  Created on: Jan 7, 2014
 *      Author: sihai
 */

#ifndef AES_H_
#define AES_H_

#include <inttypes.h>

//============================================================================
//				常量
//============================================================================
#define AES_ENCRYPT     1
#define AES_DECRYPT     0

//============================================================================
//				数据结构
//============================================================================
typedef struct aes_context {
    int nr;                     /*!<  number of rounds  */
    uint32_t *rk;               /*!<  AES round keys    */
    uint32_t buf[68];           /*!<  unaligned data    */
} aes_context;

//============================================================================
//				设置
//============================================================================
/**
 * \brief          AES key schedule (encryption)
 *
 * \param ctx      AES context to be initialized
 * \param key      encryption key
 * \param keysize  must be 128, 192 or 256
 *
 * \return         SUCCEED if successful, or ERROR_AES_INVALID_KEY_LENGTH
 */
int aes_setkey_enc(aes_context* ctx, const unsigned char* key, unsigned int keysize );

/**
 * \brief          AES key schedule (decryption)
 *
 * \param ctx      AES context to be initialized
 * \param key      decryption key
 * \param keysize  must be 128, 192 or 256
 *
 * \return         SUCCEED if successful, or ERROR_AES_INVALID_KEY_LENGTH
 */
int aes_setkey_dec( aes_context* ctx, const unsigned char* key, unsigned int keysize );

/**
 * \brief          AES-ECB block encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 *
 * \return         SUCCEED if successful
 */
int aes_crypt_ecb(aes_context* ctx, int mode, const unsigned char input[16], unsigned char output[16] );

/**
 * \brief          AES-CBC buffer encryption/decryption
 *                 Length should be a multiple of the block
 *                 size (16 bytes)
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 *
 * \return         SUCCEED if successful, or ERROR_AES_INVALID_INPUT_LENGTH
 */
int aes_crypt_cbc(aes_context* ctx, int mode, size_t length, unsigned char iv[16], const unsigned char *input, unsigned char *output );

/**
 * \brief          AES-CFB128 buffer encryption/decryption.
 *
 * Note: Due to the nature of CFB you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * aes_setkey_enc() for both AES_ENCRYPT and AES_DECRYPT.
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param length   length of the input data
 * \param iv_off   offset in IV (updated after use)
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 *
 * \return         SUCCEED if successful
 */
int aes_crypt_cfb128(aes_context* ctx, int mode, size_t length, size_t* iv_off, unsigned char iv[16], const unsigned char* input, unsigned char* output );

/**
 * \brief               AES-CTR buffer encryption/decryption
 *
 * Warning: You have to keep the maximum use of your counter in mind!
 *
 * Note: Due to the nature of CTR you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * aes_setkey_enc() for both AES_ENCRYPT and AES_DECRYPT.
 *
 * \param ctx           AES context
 * \param length        The length of the data
 * \param nc_off        The offset in the current stream_block (for resuming
 *                      within current cipher stream). The offset pointer to
 *                      should be 0 at the start of a stream.
 * \param nonce_counter The 128-bit nonce and counter.
 * \param stream_block  The saved stream-block for resuming. Is overwritten
 *                      by the function.
 * \param input         The input data stream
 * \param output        The output data stream
 *
 * \return         SUCCEED if successful
 */
int aes_crypt_ctr(aes_context* ctx, size_t length, size_t* nc_off, unsigned char nonce_counter[16], unsigned char stream_block[16], const unsigned char* input, unsigned char* output );

#endif /* AES_H_ */
