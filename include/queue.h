/*
 * queue.h
 *
 *  Created on: Jan 6, 2014
 *      Author: sihai
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//============================================================================
//				常量
//============================================================================
#define MAX_TIMEOUT				1024000

//============================================================================
//				数据结构
//============================================================================

// 队列元素
typedef struct element {

	void*  value;						//

	struct element* next;		//

} element;

// 队列
typedef struct block_queue {

	int capacity;							// 队列最大容量
	int size;								// 队列当前大小
	element* head;							// 队列头
	element* tail;							// 队列尾

	pthread_mutex_t lock;					//
	pthread_cond_t  has_element_condition;	//
	pthread_cond_t  has_space_condition;	//
} block_queue;

//============================================================================
//				创建queue
//============================================================================
block_queue* new_queue(int capacity);

//============================================================================
//				销毁queue, 并释放queue的所有元素
//============================================================================
void destroy_queue(block_queue* queue);

//============================================================================
//				入队列
//============================================================================
int enqueue(block_queue* queue, element* element);

int enqueue_timeout(block_queue* queue, element* element, int timeout);

//============================================================================
//				出队列
//============================================================================
element* dequeue(block_queue* queue);

element* dequeue_timeout(block_queue* queue, int timeout);

#endif /* QUEUE_H_ */
