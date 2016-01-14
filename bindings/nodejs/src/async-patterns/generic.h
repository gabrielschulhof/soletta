#ifndef __SOLETTA_NODE_JS_ASYNC_PATTERN_GENERIC__
#define __SOLETTA_NODE_JS_ASYNC_PATTERN_GENERIC__

#include <queue>
#include <uv.h>
#include <nan.h>

extern uv_mutex_t big_giant_mutex;

#define DELIVER_ONE_ITEM(idle, destination, callback, queue_type) \
	do { \
		bool queue_empty = true; \
		uv_async_idle_t *async_idle = ((uv_async_idle_t *)(idle)); \
		queue_type the_queue = (queue_type)(async_idle->the_queue); \
		uv_mutex_lock(&big_giant_mutex); \
		queue_empty = the_queue->empty(); \
		if (!queue_empty) { \
			destination = the_queue->front(); \
			the_queue->pop(); \
		} else { \
			uv_idle_stop(idle); \
		} \
		uv_mutex_unlock(&big_giant_mutex); \
		if (queue_empty) { \
			return; \
		} \
		callback = async_idle->jsCallback; \
	} while(0)

#define SOLETTA_CALLBACK(data, the_entry, queue_type) \
	do { \
		uv_async_monitor_t *monitor = (uv_async_monitor_t *)(data); \
		uv_mutex_lock(&big_giant_mutex); \
		((queue_type)((monitor)->idle.the_queue))->push(the_entry); \
		uv_mutex_unlock(&big_giant_mutex); \
		uv_async_send((uv_async_t *)(monitor)); \
	} while(0)

typedef struct _uv_async_idle_t uv_async_idle_t;
typedef struct _uv_async_monitor_t uv_async_monitor_t;
typedef void generic_queue;

struct _uv_async_idle_t {
	uv_idle_t base;
	Nan::Callback *jsCallback;
	generic_queue *the_queue;
	void (*free_the_queue)(generic_queue *the_queue);
	void (*deliver_one_item)(uv_idle_t *idle);
	uv_async_monitor_t *monitor;
};

struct _uv_async_monitor_t {
	uv_async_t base;
	uv_async_idle_t idle;
	void (*soletta_callback)(void *data);
};

uv_async_monitor_t *uv_async_monitor_new(
	Nan::Callback *jsCallback,
	generic_queue *the_queue,
	void (*free_the_queue)(generic_queue *the_queue),
	void (*deliver_one_item)(uv_idle_t *idle),
	void (*soletta_callback)(void *data));
void uv_async_monitor_free(uv_async_monitor_t *monitor);

#define DECLARE_PUBLIC_API_C(name, data_type) \
	uv_async_##name##_monitor_t *uv_async_##name##_monitor_new(Nan::Callback *jsCallback) { \
		return (uv_async_##name##_monitor_t *)uv_async_monitor_new( \
			jsCallback, \
			new std::queue<data_type>, \
			(void(*)(generic_queue *))free_the_queue, \
			deliver_one_item, \
			(void(*)(void *))defaultMonitor_soletta); \
	} \
	void uv_async_##name##_monitor_free(uv_async_##name##_monitor_t *monitor) { \
		uv_async_monitor_free((uv_async_monitor_t *)monitor); \
	}

#define DECLARE_PUBLIC_API_H(name) \
	typedef uv_async_monitor_t uv_async_##name##_monitor_t; \
	uv_async_##name##_monitor_t *uv_async_##name##_monitor_new(Nan::Callback *jsCallback); \
	void uv_async_##name##_monitor_free(uv_async_##name##_monitor_t *monitor);

#endif /* ndef __SOLETTA_NODE_JS_ASYNC_PATTERN_GENERIC__ */
