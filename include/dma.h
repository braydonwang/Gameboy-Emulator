#ifndef __DMA_H__
#define __DMA_H__

#include <common.h>

void dma_start(u8 start);
void dma_tick();

bool dma_transferring();

#endif /* __DMA_H__ */