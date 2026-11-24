#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void* threadfunc(void* thread_param)
{
   struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    if (pthread_mutex_lock(thread_func_args->t_mux) != 0)
      thread_func_args->thread_complete_success = false;

    usleep(thread_func_args->wait_to_release_ms * 1000);

    if(pthread_mutex_unlock(thread_func_args->t_mux) != 0)
      thread_func_args->thread_complete_success = false;
    
    thread_func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{

    struct thread_data *t_data;
    t_data = malloc(sizeof(struct thread_data));

    t_data->t_mux = mutex;
    t_data->wait_to_obtain_ms = wait_to_obtain_ms;
    t_data->wait_to_release_ms = wait_to_release_ms;

    if (pthread_create(thread, NULL, threadfunc, t_data) != 0) {
      perror("error during pthread creation!");
      return false;
    }

    return true;
}

