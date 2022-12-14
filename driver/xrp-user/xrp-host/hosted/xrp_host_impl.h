/*
 * Copyright (c) 2016 - 2018 Cadence Design Systems Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _XRP_LINUX_NATIVE_H
#define _XRP_LINUX_NATIVE_H

#include <xrp_api.h>
#include "xrp_thread_impl.h"
#include "xrp_queue_impl.h"

struct xrp_device_impl {
	int fd;
};

struct xrp_buffer_impl {
};

struct xrp_queue_impl {
	struct xrp_request_queue queue;
};

void xrp_impl_release_device(struct xrp_device *device);

void xrp_impl_create_device_buffer(struct xrp_device *device,
				   struct xrp_buffer *buffer,
				   size_t size,
				   enum xrp_status *status);
void xrp_impl_release_device_buffer(struct xrp_buffer *buffer);

void xrp_impl_create_queue(struct xrp_queue *queue,
			   enum xrp_status *status);
void xrp_impl_release_queue(struct xrp_queue *queue);

void xrp_impl_create_report(struct xrp_device *device,
				struct xrp_report *report,
				size_t size,
			    enum xrp_status *status);
				
void xrp_impl_release_report(struct xrp_device *device,
				struct xrp_report *report,enum xrp_status *status);
int xrp_impl_add_report_item(struct xrp_report *report,
								int (*cb)(void*context,void*data),
								void* context,
								size_t data_size);
int xrp_impl_add_report_item_with_id(struct xrp_report *report,
								int (*cb)(void*context,void*data),
								int report_id,
								void* context,
								size_t data_size);								

void xrp_impl_remove_report_item(struct xrp_report *report,int report_id);
void xrp_impl_import_dma_buf(struct xrp_device *device, int fd,enum xrp_access_flags,uint64_t *phy_addr,
                                uint64_t *user_addr,size_t* size,enum xrp_status *status);

void xrp_impl_release_dma_buf(struct xrp_device *device, int fd,enum xrp_status *status);
#endif
