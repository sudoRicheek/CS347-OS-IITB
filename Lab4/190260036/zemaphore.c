#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include "zemaphore.h"

void zem_init(zem_t *s, int value) {
    s->count = value;
    if (pthread_cond_init(&s->signal, NULL) != 0) {                                    
        perror("pthread_cond_init() error");                                        
        exit(1);                                                                    
    }   
    if (pthread_mutex_init(&s->lock, NULL) != 0) {
        perror("pthread_mutex_init() error");
        exit(1);
    }
}

void zem_down(zem_t *s) {
    pthread_mutex_lock(&s->lock);
    s->count--;
    if(s->count < 0)
        pthread_cond_wait(&s->signal, &s->lock);
    pthread_mutex_unlock(&s->lock);
}

void zem_up(zem_t *s) {
    pthread_mutex_lock(&s->lock);
    s->count++;
    pthread_cond_signal(&s->signal);
    pthread_mutex_unlock(&s->lock);
}
