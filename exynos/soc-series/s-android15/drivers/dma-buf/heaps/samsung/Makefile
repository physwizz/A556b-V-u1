# SPDX-License-Identifier: GPL-2.0
ccflags-y := -I$(srctree)/drivers/dma-buf/heaps
obj-$(CONFIG_DMABUF_CONTAINER)  += dma-buf-container.o
obj-$(CONFIG_DMABUF_SAMSUNG_HEAPS) += samsung_dma_heap.o
samsung_dma_heap-y += heap_dma_buf.o samsung_heap.o dma-buf-trace.o secure_buffer.o drm_sg.o samsung_page_pool.o
samsung_dma_heap-$(CONFIG_DMABUF_HEAPS_SAMSUNG_CMA) += cma_heap.o
samsung_dma_heap-$(CONFIG_DMABUF_HEAPS_SAMSUNG_CARVEOUT) += carveout_heap.o
samsung_dma_heap-$(CONFIG_DMABUF_HEAPS_GOOGLE_GCMA) += gcma_heap.o gcma_heap_sysfs.o
samsung_dma_heap-$(CONFIG_DMABUF_HEAPS_SAMSUNG_SYSTEM) += system_heap.o
samsung_dma_heap-$(CONFIG_DMABUF_HEAPS_SAMSUNG_SECURE_SYSTEM) += secure_system_heap.o
samsung_dma_heap-$(CONFIG_DMABUF_HEAPS_SAMSUNG_PREALLOCATED_CHUNK) += preallocated_chunk_heap.o
samsung_dma_heap-$(CONFIG_RBIN) += rbin_heap.o rbincache.o rbinregion.o
samsung_dma_heap-$(CONFIG_DMABUF_HEAPS_CAMERAPOOL) += camerapool.o
