
#if !defined(TIME_HEAP_H)
#define TIME_HEAP_H
#include <iostream>
#include <netinet/in.h>
#include <time.h>

using std::exception;

#define BUFFER_SIZE 64
class heap_timer;

struct client_data
{
    struct sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    heap_timer *timer;
};

class heap_timer
{
public:
    heap_timer(int delay)
    {
        this->expire = time(NULL) + delay;
    }

public:
    time_t expire;
    void (*cb_func)(struct client_data *);
    struct client_data *user_data;
};

class time_heap
{
public:
    time_heap(int cap) throw(std::exception) : capacity(cap), cur_size(0)
    {
        array = new heap_timer *[capacity];
        // if the size is too large , throw execption
        if (!array)
        {
            throw std::execption();
        }
        for (int i = 0; i < cap; i++)
        {
            array[i] = NULL;
        }
    }

    time_heap(heap_timer **cur_array, int size, int capacity) throw(std::exception) : capacity(cap), cur_size(cap)
    {
        if (capacity < size)
        {
            throw std::exception();
        }
        array = new heap_timer *[capacity];
        if (!array)
        {
            throw std::exception();
        }

        for (int i = 0; i < cap; i++)
        {
            array[i] = NULL;
        }
        if (size != 0)
        {
            for (int i = 0; i < size; i++)
            {
                array[i] = cur_array[i];
            }
            for (int i = (cur_size - 1) / 2; i >= 0; i--)
            {
                precolate_down(i);
            }
        }
    }
    ~time_heap()
    {
        for (int i = 0; i < cur_size; i++)
        {
            delete array[i];
        }
        delete[] array;
    }

public:
    // Add timer
    void add_timer(heap_timer *timer) throw(std::exception)
    {
        if (!timer)
        {
            return;
        }
        // no left space for new timer
        if (cur_size >= capacity)
        {
            // double the capacity
            resize();
        }

        int hole = cur_size;
        cur_size += 1;
        int parent = 0;

        for (; hole > 0; hole = parent)
        {
            parent = (cur_size - 1) / 2;
            if (array[parent]->expire > timer->expire)
            {
                // exchange the pos
                array[hole] = array[parent];
            }
            else
            {
                break;
            }
        }
        array[hole] = timer;
    }

    // set cb_func to NULL, means no operation for this timer
    // delay reclaim
    void del_timer(heap_timer *timer)
    {
        if (!timer)
        {
            return;
        }
        timer->cb_func = NULL;
    }

    // Get the read-only timer, set the expire time as the heartbeat
    heap_timer *top() const
    {
        if (empty())
        {
            return NULL;
        }
        return array[0];
    }

    void pop_timer()
    {
        if (empty())
        {
            return;
        }

        if (array[0])
        {
            delete array[0];
            array[0] = array[--cur_size];
            precolate_down(0);
        }
    }

    void tick()
    {
        heap_timer *tmp = array[0];
        time_t cur = time(NULL);
        while (!empty())
        {
            if (!tmp)
            {
                break;
            }
            // top element's expire time is larger than current time
            // no need to delete timer
            if (tmp->expire > cur)
            {
                break;
            }

            if (array[0]->cb_func)
            {
                array[0]->cb_func(tmp->user_data);
            }
            pop_timer();
            tmp = array[0];
        }
    }
    bool empty() { return cur_size == 0; }

protected:
    // double the capacity
    void resize() throw(std::exception)
    {
        capacity *= 2;
        heap_timer **new_array = new heap_timer *[capacity];
        if (!new_array)
        {
            throw std::exception();
        }

        for (int i = 0; i < capacity; i++)
        {
            if (i < cur_size)
            {
                new_array[i] = array[i]
            }
            else
            {
                new_array[i] = NULL;
            }
            delete [] array;
            array = new_array;
        }
    }
    void precolate_down(int pos)
    {
        if(pos < 0 || pos >= cur_size){
            return;
        }
        heap_timer * tmp = array[pos];
        int child = 2 * pos + 1;
        for(;child < cur_size; pos = child ){
            child = 2 * pos + 1;
            if((child < cur_size - 1) && (array[child]->expire > array[child + 1]-> expire)){
                child = child + 1;     
            }
            if(array[child]->expire < array[pos]->expire){
                array[pos] = array[child];
            } else {
                break;
            }
        }
        array[pos] = tmp;
    }
    
    heap_timer **array; // a pointer list which point to a timer
    int capacity;
    int cur_size;
};

//--------------------------- Autres définitions dépendantes de <time_heap>

#endif // TIME_HEAP_H