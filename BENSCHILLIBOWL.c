#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#define MAX 99

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};
int BENSCHILLIBOWLMenuLength = 10;

/* Select a random item from the Menu and return it */
MenuItem PickRandomMenuItem() {

    return BENSCHILLIBOWLMenu[ (random()%BENSCHILLIBOWLMenuLength)];
}

/* Allocate memory for the Restaurant, then create the mutex and condition variables needed to instantiate the Restaurant */

BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {
    printf("Restaurant is open!\n");
    BENSCHILLIBOWL *bcb = (BENSCHILLIBOWL*) malloc(sizeof(BENSCHILLIBOWL));
    bcb->orders = NULL;
    bcb->current_size = 0;
    bcb->max_size = max_size;
    bcb->expected_num_orders = expected_num_orders;
    bcb->next_order_number = 1;
    bcb->orders_handled = 0;
    int err = pthread_mutex_init(&bcb->mutex, NULL);
    if (err != 0){
        perror("Failed to create mutex");
        exit(1);
    }
    err = pthread_cond_init(&bcb->can_add_orders, NULL);
    if (err != 0){
        perror("Failed to create mutex");
        exit(1);
    }
    err = pthread_cond_init(&bcb->can_get_orders, NULL);
    if (err != 0){
        perror("Failed to create mutex");
        exit(1);
    }
    return bcb;
}


/* check that the number of orders received is equal to the number handled (ie.fullfilled). Remember to deallocate your resources */
void recurse_free(Order *o){
  if (o) {
    recurse_free(o->next);
    free(o);
  }
}
void CloseRestaurant(BENSCHILLIBOWL * bcb) {
  printf("Restaurant is closed!\n");
  recurse_free(bcb->orders);
  pthread_mutex_destroy(&bcb->mutex);
  pthread_cond_destroy(&bcb->can_add_orders);
  pthread_cond_destroy(&bcb->can_get_orders);
}

/* add an order to the back of queue */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {
  pthread_mutex_lock(&bcb->mutex);
  int ret = bcb->next_order_number;
  if (!IsFull(bcb)){
    order->order_number = bcb->next_order_number;
    AddOrderToBack(&bcb->orders, order);
    bcb->next_order_number++;
    bcb->current_size++;
    
  }else{
    while (IsFull(bcb)){
      int e = pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }
    order->order_number = bcb->next_order_number;
    bcb->next_order_number++;
    bcb->current_size++;
    AddOrderToBack(&bcb->orders, order);
  }
  pthread_cond_signal(&bcb->can_get_orders);
  pthread_mutex_unlock(&bcb->mutex);
  return ret;
}

/* remove an order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {
  pthread_mutex_lock(&bcb->mutex);
  Order *ret;
  struct timespec timeToWait;
  struct timeval now;
  int rt;
  
  if (!IsEmpty(bcb)){
    ret = bcb->orders;
    bcb->orders = bcb->orders->next;
    ret->next=NULL;
    bcb->current_size--;
    bcb->orders_handled++;
  }else{
    while (IsEmpty(bcb)){
      gettimeofday(&now,NULL);
      timeToWait.tv_sec = now.tv_sec+1;
      int e = pthread_cond_timedwait(&bcb->can_get_orders, &bcb->mutex, &timeToWait);
      if (e == ETIMEDOUT){
        pthread_mutex_unlock(&bcb->mutex);
        return NULL;
      }
    }
    ret = bcb->orders;
    bcb->orders = bcb->orders->next;
    ret->next=NULL;
    bcb->current_size--;
    bcb->orders_handled++;
  }
  pthread_cond_signal(&bcb->can_add_orders);
  pthread_mutex_unlock(&bcb->mutex);
  return ret;
}

// Optional helper functions (you can implement if you think they would be useful)
bool IsEmpty(BENSCHILLIBOWL* bcb) {
  return bcb -> current_size == 0;
}

bool IsFull(BENSCHILLIBOWL* bcb) {
  return bcb -> current_size == bcb -> max_size;
}

/* this methods adds order to rear of queue */
void AddOrderToBack(Order **orders, Order * order) {
  Order *curr = *orders;
  if (curr == NULL){
    *orders = order;
    return;
  }
  while(curr->next != NULL){
    curr = curr->next;
  }
  curr->next = order;
  order->next = NULL;
}
