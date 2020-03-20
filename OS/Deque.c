#include "Deque.h"
#include <stdlib.h>
#include <stdio.h>

void DequeInit(Deque * pdeq)
{
  pdeq->head = pdeq->tail = NULL;
}

int DQIsEmpty(Deque * pdeq)
{
  if(pdeq->head)
    return FALSE;
  return TRUE;
}

void DQAddFirst(Deque * pdeq, Data data)
{
  Node * newNode = (Node *)malloc(sizeof(Node));
  newNode->data = data;
  newNode->prev = NULL;

  newNode->next = pdeq->head;
  //pdeq->head = newNode; if will be true
  if(DQIsEmpty(pdeq))
    pdeq->tail = newNode;
  else
    newNode->next->prev = newNode;

  pdeq->head = newNode;
}

void DQAddLast(Deque * pdeq, Data data)
{
  Node * newNode = (Node *)malloc(sizeof(Node));
  newNode->data = data;
  newNode->next = NULL;

  newNode->prev = pdeq->tail;
  if(DQIsEmpty(pdeq))
    pdeq->head = newNode;
  else
    newNode->prev->next = newNode;
  pdeq->tail = newNode;
}

Data DQRemoveFirst(Deque * pdeq)
{
  Node * delnode = pdeq->head;
  Data rdata;

  if(DQIsEmpty(pdeq))
  {
    puts("rf1 Deque is empty");
    exit(-1);
  }

  rdata = delnode->data;
  pdeq->head = pdeq->head->next;
  if(!(pdeq->head))
    pdeq->tail = NULL;
  else
    pdeq->head->prev = NULL;

  free(delnode);
  return rdata;
}

Data DQRemoveLast(Deque * pdeq)
{
  Node * delnode = pdeq->tail;
  Data rdata;

  if(DQIsEmpty(pdeq))
  {
    puts("uDeque is empty");
    exit(-1);
  }

  rdata = delnode->data;
  pdeq->tail = pdeq->tail->prev;
  if(!(pdeq->tail))
    pdeq->head = NULL;
  else
    pdeq->tail->next = NULL;

  free(delnode);
  return rdata;
}

Data DQGetFirst(Deque * pdeq)
{
  if(DQIsEmpty(pdeq))
  {
    puts("gf1 Deque is empty");
    exit(-1);
  }

  return pdeq->head->data;
}

Data DQGetLast(Deque * pdeq)
{
  if(DQIsEmpty(pdeq))
  {
    puts("dDeque is empty");
    exit(-1);
  }

  return pdeq->tail->data;
}
