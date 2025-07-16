/* * Ring buffer for BL30 log share to kernel
 * Copyright (C) 2021-2022 amlogic
 *
 */
#include <common.h>
#include <asm/amlogic/arch/cpu_config.h>
#include <asm/amlogic/arch/register.h>
#include "ring_buffer.h"

#define LOCK 1

static int blx_ready;
static struct ring_buffer *g_rb;

/*log message timer*/
#define LOG_MSG_TIMER	SYSCTRL_TIMERE

static void Lock_EnterCritical(void)
{
	//taskENTER_CRITICAL();
}

static void Lock_ExitCritical(void)
{
	//taskEXIT_CRITICAL();
}

static int amp_lock_init(amp_lock *lock)
{
	//memset(lock,0,sizeof(amp_lock));
	lock->turn = 0;
	lock->req[0] = 0;
	lock->req[1] = 0;
	return 0;
}

int amp_lock_obtain(amp_lock *lock, int self)
{
#if LOCK
	int other = 1 - self;

    // disable interrupt here
	Lock_EnterCritical();

	lock->req[self] = 1;
	if (lock->req[other]) {
		lock->req[self] = 0;
		while (lock->turn != self)
			;
		lock->req[self] = 1;
		while (lock->req[other])
			;
	}
#endif
	return 0;
}

int amp_lock_release(amp_lock *lock, int self)
{
#if LOCK
	int other = 1 - self;

	lock->turn = other;
	lock->req[self] = 0;

    // enable interrupt here
	Lock_ExitCritical();
#endif
	return 0;
}

//int lock(void)
//{
//	return 0;
//}

//int unlock(void)
//{
//	return 0;
//}

static struct ring_buffer *init_ring_buffer(unsigned char *buffer, unsigned int size)
{
	struct ring_buffer *b = (struct ring_buffer *)buffer;

	if (!b || size < sizeof(struct ring_buffer))
		return NULL;
	if (b->magic == MAGIC)
		return b;

	b->magic = MAGIC;
	b->inited = UBOOT_LOG_INITFLAG;
	b->ver = UBOOT_LOG_VER;
	b->size = size - sizeof(struct ring_buffer) + 4;
	b->head = 0;
	b->tail = 0;
	b->len  = 0;
	b->len_b  = 0;
	b->mode = 0;
	amp_lock_init(&b->lock);
	return b;
}

int check_and_get_ring_buffer(unsigned char *buffer, unsigned int size)
{
	struct ring_buffer *b = (struct ring_buffer *)buffer;

	if (!b || size < sizeof(struct ring_buffer))
		return -1;
	if (b->magic == MAGIC) {
		g_rb = b;
		return 0;
	}
	return -1;
}

/*
 * buffer: entire loop buffer start address
 * size  : loop buffer size
 * return: 0: successful, -1: fail
 */
int initial_ring_buffer(unsigned char *buffer, unsigned int size)
{
	g_rb = init_ring_buffer(buffer, size);

	if (g_rb)
		return 0;
	return -1;
}

/* __read_ring_buffer
 * timer: get time from loop buffer
 * msg_state: 0: read a message in progress,
 *            bit0 is 1: read a mesaage begin
 *            bit1 is 1: read a message end
 */
static unsigned int __read_ring_buffer(struct ring_buffer *rb, unsigned int *timer,
		unsigned char *buffer, unsigned int size, unsigned int *msg_state)
{
	unsigned int count, rlen;
	unsigned int total_len;
	unsigned int msg_header;
	unsigned char *p;
	static unsigned int *r_message_len;
	static unsigned int r_message_len_b;

	if (!rb || rb->magic != MAGIC || rb->len == 0)
		return 0;
	if (!buffer || size == 0 || !timer || !msg_state)
		return 0;

	*msg_state = READ_MSG_STATE_IN_PROGRESS;
	count = 0;
	amp_lock_obtain(&rb->lock, ARM_CPU);
	/*rb->mode LOG_MESSAGE_READ bit is 0 to read header*/
	if ((rb->mode & LOG_MESSAGE_READ) != LOG_MESSAGE_READ) {
		total_len = rb->len;
		if (total_len >= MESSAGE_MIN_SIZE) {
			/* a message format:
			 * LOG_MESSAGE_HEADER(4bytes) + time(4bytes) + message len(4bytes) + message
			 * note: message end with '\n'
			 */
			/*find out a message header*/
			msg_header = LOG_MESSAGE_HEADER;
			p = (unsigned char *)&msg_header;
			do {
				if (p[0] != rb->data[rb->head] ||
					p[1] != rb->data[rb->head + 1] ||
					p[2] != rb->data[rb->head + 2] ||
					p[3] != rb->data[rb->head + 3]) {
					rb->head++;
					rb->head %= rb->size;
					rb->len--;
					if (rb->len == 0)
						goto err;
				} else {
					rb->head += 4;
					rb->head %= rb->size;
					rb->len -= 4;
					break;
				}
			} while (1);

			/*get time of a message*/
			p = (unsigned char *)timer;
			rlen = 4;
			rb->len -= rlen;
			while (rlen--) {
				p[count++] = rb->data[rb->head++];
				rb->head %= rb->size;
			}
			/*get a message len */
			r_message_len = (unsigned int *)&rb->data[rb->head];
			rb->head += 4;
			rb->head %= rb->size;
			rb->len -= 4;
			r_message_len_b = *r_message_len;
			/*start to get a message */
			rb->mode |= LOG_MESSAGE_READ;
			*msg_state |= READ_MSG_STATE_BEGIN;
		} else if (total_len == 0) {
			goto err;
		} // else {
			/*it should never be here, if come here, there is an error*/
		//}
	}
	if (!r_message_len) {
		/*it should never be here, if come here, there is an error*/
		goto err;
	}
	/*read a message*/
	count = 0;
	rlen = size < rb->len ? size : rb->len;
	rlen = r_message_len_b < rlen ? r_message_len_b : rlen;
	rb->len -= rlen;
	r_message_len_b -= rlen;
	while (rlen--) {
		buffer[count++] = rb->data[rb->head++];
		rb->head %= rb->size;
	}
	/*read message finished*/
	if (r_message_len_b == 0) {
		unsigned int save_head;

		save_head = rb->head;
		rb->head += (MESSAGE_ALIGN_SIZE - 1);
		rb->head = (rb->head >> 2) << 2;
		save_head = rb->head - save_head;
		rb->head %= rb->size;
		rb->len -= save_head;
		rb->mode &= ~(LOG_MESSAGE_READ);
		r_message_len = NULL;
		*msg_state |= READ_MSG_STATE_END; //one message read finished
	}
	if (rb->len == 0) {
		//if read finished, clean LOG_MESSAGE_SECOND_READ flag
		rb->mode &= ~(LOG_MESSAGE_SECOND_READ);
	}
	amp_lock_release(&rb->lock, ARM_CPU);
	return count;
err:
	amp_lock_release(&rb->lock, ARM_CPU);
	return 0;
}

unsigned int read_ring_buffer(unsigned int *timer, unsigned char *buffer,
		unsigned int size, unsigned int *msg_state)
{
	return __read_ring_buffer(g_rb, timer, buffer, size, msg_state);
}

static void __second_read_ring_buffer_set(struct ring_buffer *rb)
{
	amp_lock_obtain(&rb->lock, ARM_CPU);

	if (rb->len == 0 && ((rb->mode & LOG_MESSAGE_WRITE) != LOG_MESSAGE_WRITE)) {
		rb->len = rb->len_b;
		rb->head = 0;
		rb->mode |= (LOG_MESSAGE_SECOND_READ);
	}

	amp_lock_release(&rb->lock, ARM_CPU);
}

void second_read_ring_buffer_set(void)
{
	__second_read_ring_buffer_set(g_rb);
}

static unsigned int __write_ring_buffer(struct ring_buffer *rb,
		unsigned char *buffer, unsigned int size)
{
	unsigned int count, wlen;
	unsigned int timer;
	unsigned int msg_header;
	unsigned char *p;
	static unsigned int *message_len;

	if (!rb || rb->magic != MAGIC)
		return 0;
	if (size == 0)
		return 0;

	count = 0;
	amp_lock_obtain(&rb->lock, RISC_V_CPU);
	/*rb->mode LOG_MESSAGE_WRITE bit is 0 to write header*/
	if ((rb->mode & LOG_MESSAGE_WRITE) != LOG_MESSAGE_WRITE) {
		/* a message format:
		 * LOG_MESSAGE_HEADER(4bytes) + time(4bytes) + message len(4bytes) + message
		 * note: message end with '\n'
		 */
		/*write message header to loop buffer, it indicate a message start*/
		msg_header = LOG_MESSAGE_HEADER;
		p = (unsigned char *)&msg_header;
		wlen = 4;
		rb->len += wlen;
		rb->len_b += wlen;
		while (wlen--) {
			rb->data[rb->tail++] = p[count++];
			rb->tail %= rb->size;
		}

		/*write time of a message */
		/*if LOG_MSG_TIMER is optimized that can not get time, use volatile*/
		timer = *(unsigned int *)(unsigned long)LOG_MSG_TIMER;
		p = (unsigned char *)&timer;
		wlen = 4;
		rb->len += wlen;
		rb->len_b += wlen;
		count = 0;
		while (wlen--) {
			rb->data[rb->tail++] = p[count++];
			rb->tail %= rb->size;
		}

		/*reserver message len space*/
		message_len = (unsigned int *)&rb->data[rb->tail];
		*message_len = 0;
		rb->tail += 4;
		rb->tail %= rb->size;
		rb->len += 4;
		rb->len_b += 4;
		/*write a message until message ends with '\n' in buffer,
		 *the middle '\n' in the buffer does not act as an end marker
		 */
		rb->mode |= LOG_MESSAGE_WRITE;
	}
	if (!message_len) {
		/*it should never be here, if come here, it have an error*/
		goto err;
	}
	wlen = size;
	rb->len += wlen;
	rb->len_b += wlen;
	(*message_len) += wlen;
	count = 0;
	while (wlen--) {
		rb->data[rb->tail++] = buffer[count++];
		rb->tail %= rb->size;
	}
	/*'\n' is message end, message size MESSAGE_ALIGN_SIZE size align */
	if (buffer[size - 1] == '\n') {
		unsigned int save_tail;

		save_tail = rb->tail;
		rb->tail += (MESSAGE_ALIGN_SIZE - 1);
		rb->tail = (rb->tail >> 2) << 2;
		save_tail = rb->tail - save_tail;
		rb->tail %= rb->size;
		rb->len += save_tail;
		rb->len_b += save_tail;
		rb->mode &= ~(LOG_MESSAGE_WRITE);
		message_len = NULL;
	}

	/* New data will overwrite the old data */
	if (rb->len > rb->size) {
		rb->head = rb->tail;
		rb->len = rb->size;
		rb->len_b = rb->size;
	}
	amp_lock_release(&rb->lock, RISC_V_CPU);

	return count;
err:
	amp_lock_release(&rb->lock, RISC_V_CPU);
	return 0;
}

unsigned int write_ring_buffer(unsigned char *buffer, unsigned int size)
{
	if (blx_ready)
		return __write_ring_buffer(g_rb, buffer, size);
	return 0;
}

static void __reset_ring_buffer(struct ring_buffer *rb)
{
	if (!rb || rb->magic != MAGIC)
		return;

	rb->head = 0;
	rb->tail = 0;
	rb->len  = 0;
	rb->len_b  = 0;
	amp_lock_init(&rb->lock);
}

void reset_ring_buffer(void)
{
	__reset_ring_buffer(g_rb);
}

int blx_init_uboot_log(void)
{
	blx_ready = 1;
	initial_ring_buffer((unsigned char *)UBOOT_LOG_BUF_BASE, UBOOT_LOG_BUF_SIZE);
	return 0;
}

void uboot_log_use_new_addr(unsigned long addr, unsigned int size)
{
	if (size < UBOOT_LOG_BUF_SIZE) {
		/*address space too small*/
		return;
	}
	memcpy((void *)addr, (void *)UBOOT_LOG_BUF_BASE, UBOOT_LOG_BUF_SIZE);
	initial_ring_buffer((unsigned char *)addr, UBOOT_LOG_BUF_SIZE);
}

void blx_put_char(unsigned char c)
{
	if (blx_ready) {
		if (c == '\n') {
			unsigned char c_r = '\r';

			write_ring_buffer(&c_r, 1);
		}
		write_ring_buffer(&c, 1);
	}
}

//#define DUMP_SAVE_LOG_TEST

#ifdef DUMP_SAVE_LOG_TEST
#include <serial.h>
#include <string.h>
void dump_loop_buf_log(void)
{
	unsigned char r_buf[50];
	unsigned int timer = 0;
	int i;
	unsigned int status;
	unsigned int count;

	blx_ready = 0;
	printf("save log temporary addr: 0x%x\n", UBOOT_LOG_BUF_BASE);
	printf("save log g_rb addr: 0x%lx\n", (long)g_rb);
	printf("save log g_rb->len: 0x%x, g_rb->len_b:0x%x\n", g_rb->len, g_rb->len_b);
	printf("save log g_rb->mode: 0x%x,g_rb->head:0x%x,g_rb->tail:0x%x\n",
	       g_rb->mode, g_rb->head, g_rb->tail);
	printf("*dump start:\n");
	memset(r_buf, 0, sizeof(r_buf));
	for (i = 0; i < 1000; i++) {
		memset(r_buf, 0, sizeof(r_buf));
		timer = 0;
		status = 0;
		count = read_ring_buffer(&timer, r_buf, sizeof(r_buf), &status);
		printf("dump status: 0x%x, count: 0x%x\n", status, count);
		if (count > 0) {
			switch (status & 3) {
			case 1:
			case 3:
				printf("dump timer: 0x%x\n", timer);
				printf((const char *)r_buf);
			break;
			case 0:
			case 2:
				printf((const char *)r_buf);
			break;
			default:
			break;
			}
		}
	}
	printf("*dump end\n");
}
#endif

