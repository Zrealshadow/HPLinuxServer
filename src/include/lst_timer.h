#ifndef LST_TIMER_H
#define LST_TIMER_H

#pragma once

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64
class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer *timer;
};

class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL){};

public:
    time_t expire;
    void (*cb_func)(client_data *);
    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};

class sort_timer_lst
{
public:
    sort_timer_lst() : head(NULL), tail(NULL){};
    ~sort_timer_lst()
    {
        util_timer *tmp = head;
        while (tmp)
        {
            head = head->next;
            delete tmp;
            tmp = head;
        }
    }

    void add_timer(util_timer * timer){
        if(!timer){
            return;
        }

        if(!head){
            head = tail = timer;
            return;
        }

        if(timer->expire < head->expire){
            head->prev = timer;
            timer->next = head;
            head =timer;
            return;
        }

        // the timer insert behind head
        add_timer(timer, head);
    }

    void adjust_timer(util_timer* timer){
        if(!timer){
            return;
        }
        util_timer * tmp = timer->next;
        // if this timer is at the tail or the next timer's expire time is bigger
        // no adjustment is needed
        if(!tmp || (timer->expire < tmp->expire)){
            return;
        }

        if(timer == head){
            head = head->next;
            head->prev  =NULL;
            timer->next = NULL;
            add_timer(timer,head);
        } else {
            // if timer is between head and tail
            // pick up this timer and insert it into sorted queue again
            timer->next->prev = timer->prev;
            timer->prev->next = timer->next;
            add_timer(timer,timer->next);
        }
    }

    // delete the target timer from sorted list
    void del_timer(util_timer *timer){
        if(!timer){
            return;
        }
        
        if((timer == head) && (timer == tail)){
            delete timer;
            head =NULL;
            tail =NULL;
            return;
        }

        if(timer==head){
            head->next->prev = NULL;
            head = head->next;
            delete timer;
            return;
        }

        if(timer == tail){
            tail->prev->next = NULL;
            tail = tail->prev;
            delete timer;
            return;
        }

        // the timer is between head and tail
        // pick up this timer

        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }

    // update the expired util_timer
    void tick(){
        // no existed timer
        if(!head){
            return;
        }
        printf("timer tick\n");
        time_t cur = time(NULL);
        util_timer * tmp = head;
        while(tmp){
            if(cur < tmp->expire){
                break;
            }
            // this util_timer is expired
            // invoke callback function
            tmp->cb_func(tmp->user_data);
            head = tmp->next;
            if(head){
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    // insert timer behind start timer
    void add_timer(util_timer *timer, util_timer *start){
        util_timer* tmp = start->next;
        while(tmp){
            if(timer->expire < tmp->expire){
                timer->next = tmp;
                timer->prev = tmp->prev;
                tmp->prev->next = timer;
                tmp->prev = timer;
                break;
            }
            tmp = tmp->next;
        }

        if(!tmp){
            // tmp == NULL, means that timer should insert into tail
            tail->next = timer;
            timer->next = NULL;
            timer->prev = tail;
            tail = timer;
        }
    }
    util_timer *head;
    util_timer *tail;
};

#endif