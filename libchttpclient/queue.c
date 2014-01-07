/*
 * queue
 *
 *  Created on: Jan 6, 2014
 *      Author: sihai
 */
#include "queue.h"

#define OK 0

block_queue* new_queue(int capacity) {
	printf("new_queue, capacity:%d\n", capacity);
	block_queue* q = malloc(sizeof(block_queue));
	if(NULL != q) {
		memset(q, 0x00, sizeof(block_queue));
		q->capacity = capacity;
		// 初始化queue的lock
		if(pthread_mutex_init(&(q->lock), NULL)) {
			free(q);
			// TODO log
			return NULL;
		}
		// 初始化条件变量
		if(pthread_cond_init(&(q->has_element_condition), NULL)) {
			free(q);
			pthread_mutex_destroy(&(q->lock));
			// TODO log
			return NULL;
		}
		if(pthread_cond_init(&(q->has_space_condition), NULL)) {
			free(q);
			pthread_mutex_destroy(&(q->lock));
			pthread_cond_destroy(&(q->has_element_condition));
			// TODO log
			return NULL;
		}
	}

	return q;
}

void destroy_queue(block_queue* queue) {
	//XXX 这里要不要加锁呢
	if(OK == pthread_mutex_lock(&(queue->lock))) {
		element* e = queue->head;
		for(; NULL != e; e = e->next) {
			free(e);
		}
		free(queue);
	}
}

int enqueue(block_queue* queue, element* element) {
	return enqueue_timeout(queue, element, MAX_TIMEOUT);
}

int enqueue_timeout(block_queue* queue, element* element, int timeout) {
	int ret = 0;
	if(OK == pthread_mutex_lock(&(queue->lock))) {
		while(queue->size >= queue->capacity) {
			printf("queue is full, waiting space\n");
			// 等待有空间
			pthread_cond_wait(&(queue->has_space_condition), &(queue->lock));
		}

		if(NULL == queue->tail) {
			queue->tail = element;
			queue->head = element;
		} else {
			queue->tail->next = element;
			element->next = NULL;
			queue->tail = element;
		}
		queue->size++;
		// 通知有元素了
		pthread_cond_signal(&(queue->has_element_condition));
		// 释放锁
		pthread_mutex_unlock(&(queue->lock));
		ret = OK;
	} else {
		// TODO log
		ret = -1;
	}

	return ret;
}

element* dequeue(block_queue* queue) {
	return dequeue_timeout(queue, MAX_TIMEOUT);
}

element* dequeue_timeout(block_queue* queue, int timeout) {

	element* e = NULL;
	if(OK == pthread_mutex_lock(&(queue->lock))) {
		while(0 == queue->size) {
			printf("queue is empty, waiting element\n");
			// 等待有元素
			pthread_cond_wait(&(queue->has_element_condition), &(queue->lock));
		}
		e = queue->head;
		queue->head = e->next;
		if(NULL == queue->head) {
			// 空了
			queue->tail = NULL;
		}
		// 当前size 减1
		queue->size--;

		// 通知有元素了
		pthread_cond_signal(&(queue->has_space_condition));
		// 释放锁
		pthread_mutex_unlock(&(queue->lock));
	} else {
		// TODO log
	}

	return e;
}


