/*
 * concurrent_hashmap.h
 *
 *
 *  Created on: Jan 10, 2014
 *      Author: sihai
 */

#ifndef CONCURRENT_HASHMAP_H_
#define CONCURRENT_HASHMAP_H_

#include <stdio.h>
#include <string.h>
#include <pthread.h>

typedef struct concurrent_hashmap_ops concurrent_hashmap_ops;
typedef struct segment_ops segment_ops;

/**
 * \brief          entry of hash map segment
 *
 * \member key     key
 * \member value   value
 * \member hash	   value of hash(key)
 * \member next	   list next
 */
typedef struct entry {

	char* key;
	void* value;
	int   hash;

	struct entry* next;
} entry;

typedef void* (*s_put)(segment* segment, char* key, void* data);
typedef void* (*s_put_if_absent)(segment* segment, char* key, void* data);
typedef void* (*s_remove)(segment* segment, char* key);
typedef void* (*s_clear)(segment* segment);
typedef unsigned int (*s_size)(segment* segment);
typedef void (*s_lock)(segment* segment);
typedef void (*s_unlock)(segment* segment);

/**
 * \brief          functions of segment
 *
 * \member put     	size of segment
 * \member put_if_absent   	value
 * \member remove	   		value of hash(key)
 * \member next	  	 	list next
 */
struct segment_ops {
	s_put put;
	s_put_if_absent put_if_absent;
	s_remove remove;
	s_clear clear;
	s_size size;
	s_lock  lock;
	s_unlock unlock;
};

/**
 * \brief          segment of hash map
 *
 * \member size     	size of segment
 * \member entries   	value
 * \member hash	   		value of hash(key)
 * \member next	  	 	list next
 */
typedef struct segment {
	int size;
	entry* entries;

	pthread_mutex_t lock;

	segment_ops* ops;
} segment;

/**
 * \brief          concurrent hash map
 *
 * \member size     	size of segment
 * \member entries   	value
 * \member hash	   		value of hash(key)
 * \member next	  	 	list next
 */
typedef struct concurrent_hashmap {
	int capacity;
	float load_factor;
	int concurrency_level;

	int segment_size;
	segment* segments;

	pthread_mutex_t lock;
	concurrent_hashmap_ops* ops;
} concurrent_hashmap;

typedef void* (*put)(concurrent_hashmap* hashmap, char* key, void* data);
typedef void* (*put_if_absent)(concurrent_hashmap* hashmap, char* key, void* data);
typedef void* (*remove)(concurrent_hashmap* hashmap, char* key);
typedef void (*clear)(concurrent_hashmap* hashmap);
typedef unsigned int (*size)(concurrent_hashmap* hashmap);
typedef void (*lock)(concurrent_hashmap* hashmap);
typedef void (*unlock)(concurrent_hashmap* hashmap);

/**
 * \brief          functions of concurrent hash map
 *
 * \member put     	size of segment
 * \member put_if_absent   	value
 * \member remove	   		value of hash(key)
 * \member next	  	 	list next
 */
struct concurrent_hashmap_ops {
	put put;
	put_if_absent put_if_absent;
	remove remove;
	clear clear;
	size size;
	lock lock;
	unlock unlock;
};


/**
 * \brief          Create one concurrent hash map
 *
 * \param initial_capacity
 * \param load_factor
 * \param concurrency_level
 *
 * \return         SUCCEED if successful, or ERROR_AES_INVALID_KEY_LENGTH
 */
concurrent_hashmap* new_concurrent_hashmap(int initial_capacity, float load_factor, int concurrency_level);

/**
 * \brief          Destroy one concurrent hash map, release resources
 *
 * \param hashmap
 *
 */
void destroy_concurrent_hashmap(concurrent_hashmap* hashmap);


#endif /* CONCURRENT_HASHMAP_H_ */
