/*
 * concurrent_hashmap.c
 *
 *  Created on: Jan 10, 2014
 *      Author: sihai
 */


#include "concurrent_hashmap.h"

static void* s_put(segment* segment, char* key, void* data) {
	return NULL;
}

static void* s_put_if_absent(segment* segment, char* key, void* data) {
	return NULL;
}

static void* s_remove(segment* segment, char* key) {
	return NULL;
}

static void s_clear(segment* segment) {

}

static unsigned int s_size(segment* segment) {
	return 0;
}

static void s_lock(segment* segment) {

}

typedef void s_unlock(segment* segment) {

}

static void* put(concurrent_hashmap* hashmap, char* key, void* data) {
	int sindex = s_hash(key, hashmap->segment_size);
	return NULL;
}

static void* put_if_absent(concurrent_hashmap* hashmap, char* key, void* data) {
	return NULL;
}

static void* remove(concurrent_hashmap* hashmap, char* key) {

	return NULL;
}

static void clear(concurrent_hashmap* hashmap) {

}

static unsigned int size(concurrent_hashmap* hashmap) {

	return 0;
}

static void lock(concurrent_hashmap* hashmap) {

}

static void unlock(concurrent_hashmap* hashmap) {

}

//===============================================================
//				hash 函数
//===============================================================

unsigned int SDBMHash(char *str) {
	unsigned int hash = 0;

	while (*str) {
		// equivalent to: hash = 65599*hash + (*str++);
		hash = (*str++) + (hash << 6) + (hash << 16) - hash;
	}

	return (hash & 0x7FFFFFFF);
}

// RS Hash Function
unsigned int RSHash(char *str) {
	unsigned int b = 378551;
	unsigned int a = 63689;
	unsigned int hash = 0;

	while (*str) {
		hash = hash * a + (*str++);
		a *= b;
	}

	return (hash & 0x7FFFFFFF);
}

// JS Hash Function
unsigned int JSHash(char *str) {
	unsigned int hash = 1315423911;

	while (*str) {
		hash ^= ((hash << 5) + (*str++) + (hash >> 2));
	}

	return (hash & 0x7FFFFFFF);
}

// P. J. Weinberger Hash Function
unsigned int PJWHash(char *str) {
	unsigned int BitsInUnignedInt = (unsigned int) (sizeof(unsigned int) * 8);
	unsigned int ThreeQuarters = (unsigned int) ((BitsInUnignedInt * 3) / 4);
	unsigned int OneEighth = (unsigned int) (BitsInUnignedInt / 8);
	unsigned int HighBits = (unsigned int) (0xFFFFFFFF)
			<< (BitsInUnignedInt - OneEighth);
	unsigned int hash = 0;
	unsigned int test = 0;

	while (*str) {
		hash = (hash << OneEighth) + (*str++);
		if ((test = hash & HighBits) != 0) {
			hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
		}
	}

	return (hash & 0x7FFFFFFF);
}

// ELF Hash Function
unsigned int ELFHash(char *str) {
	unsigned int hash = 0;
	unsigned int x = 0;

	while (*str) {
		hash = (hash << 4) + (*str++);
		if ((x = hash & 0xF0000000L) != 0) {
			hash ^= (x >> 24);
			hash &= ~x;
		}
	}

	return (hash & 0x7FFFFFFF);
}

// BKDR Hash Function
unsigned int BKDRHash(char *str) {
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str) {
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

// DJB Hash Function
unsigned int DJBHash(char *str) {
	unsigned int hash = 5381;

	while (*str) {
		hash += (hash << 5) + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

// AP Hash Function
unsigned int APHash(char *str) {
	unsigned int hash = 0;
	int i;

	for (i = 0; *str; i++) {
		if ((i & 1) == 0) {
			hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
		} else {
			hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
		}
	}

	return (hash & 0x7FFFFFFF);
}

unsigned int s_hash(char* key, int bucket_size) {
	return BKDRHash(key) % bucket_size;
}

unsigned int hash(char* key, int segment_size) {
	return BKDRHash(key) % segment_size;
}

concurrent_hashmap* new_concurrent_hashmap(int initial_capacity, float load_factor, int concurrency_level) {

	int i = 0;
	segment* p = NULL;

	// alloc ops
	concurrent_hashmap_ops* ops = malloc(sizeof(concurrent_hashmap_ops));
	if(NULL == ops) {
		// no memory
		return NULL;
	}
	memset((void*)ops, 0x00, sizeof(concurrent_hashmap_ops));
	ops->put = &put;
	ops->put_if_absent = &put_if_absent;
	ops->remove = &remove;
	ops->clear = &clear;

	// alloc hash map
	concurrent_hashmap* hashmap = malloc(sizeof(concurrent_hashmap));
	if(NULL == hashmap) {
		// no memory
		free(ops);
		return NULL;
	}
	memset((void*)hashmap, 0x00, sizeof(concurrent_hashmap));
	hashmap->capacity = initial_capacity;
	hashmap->load_factor = load_factor;
	hashmap->concurrency_level = concurrency_level;
	hashmap->ops = ops;
	hashmap->segment_size = initial_capacity / concurrency_level;
	if(0 != initial_capacity % concurrency_level) {
		hashmap->segment_size++;
	}

	// alloc segments

	// alloc segment_ops
	segment_ops* sops = malloc(sizeof(segment_ops));
	if(NULL == sops) {
		// no memory
		free(ops);
		free(hashmap);
		return NULL;
	}
	memset((void*)sops, 0x00, sizeof(segment_ops));
	sops->put = &s_put;
	sops->put_if_absent = &s_put_if_absent;
	sops->remove = &s_remove;
	sops->clear = &s_clear;

	//
	segment* sgs = malloc(sizeof(segment) * hashmap->segment_size);
	memset((void*)sgs, 0x00, sizeof(segment) * hashmap->segment_size);

	p = sgs;
	for(; i < hashmap->segment_size; i++,p++) {
		p->size = 0;
		p->ops = sops;
	}

	return hashmap;
}

void destroy_concurrent_hashmap(concurrent_hashmap* hashmap) {

	int i = 0;
	// clear segments
	segment* s = hashmap->segments;
	for(; i < hashmap->segment_size; i++,s++) {
		s->ops->clear(s);
	}

	free(hashmap->segments);

	free(hashmap);
}
