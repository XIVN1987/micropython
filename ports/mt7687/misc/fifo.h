#ifndef __MISC_UTIL_FIFO_H__
#define __MISC_UTIL_FIFO_H__

typedef struct {
    void *pvElements;
    unsigned int uiElementCount;
    unsigned int uiElementsMax;
    unsigned int uiFirst;
    unsigned int uiLast;
    void (*pfElementPush)(void * const pvFifo, const void * const pvElement);
    void (*pfElementPop)(void * const pvFifo, void * const pvElement);
}FIFO_t;

extern void FIFO_Init (FIFO_t *fifo, unsigned int uiElementsMax,
void (*pfElmentPush)(void * const pvFifo, const void * const pvElement),
void (*pfElementPop)(void * const pvFifo, void * const pvElement));
extern bool FIFO_bPushElement (FIFO_t *fifo, const void * const pvElement);
extern bool FIFO_bPopElement (FIFO_t *fifo, void * const pvElement);
extern bool FIFO_bPeekElement (FIFO_t *fifo, void * const pvElement);
extern bool FIFO_IsEmpty (FIFO_t *fifo);
extern bool FIFO_IsFull (FIFO_t *fifo);
extern void FIFO_Flush (FIFO_t *fifo);

#endif
