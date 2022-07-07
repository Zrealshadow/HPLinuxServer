/*************************************************************************
                            time_wheel_timer  -  description
                              --------------------
   début                : 07/07/2022
   copyright            : (C) 2022 par name
*************************************************************************/

//---------- Interface de la classe <time_wheel_timer> (fichier <time_wheel_timer.h>) ------

#if !defined(TIME_WHEEL_TIMER_H)
#define TIME_WHEEL_TIMER_H
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 1024
class tw_timer;

struct client_data
{
    struct sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    tw_timer *timer;
};

class tw_timer
{
public:
    tw_timer(int rot, int ts) : next(NULL), prev(NULL), rotation(rot), time_slot(ts){};

public:
    int rotation;
    int time_slot;
    void (*cb_func)(struct client_data *);
    client_data *user_data;
    tw_timer *next;
    tw_timer *prev;
};
//
//------------------------------------------------------------------------
class time_wheel_timer
{
    //----------------------------------------------------------------- PUBLIC

public:
    //----------------------------------------------------- Méthodes publiques

    //------------------------------------------------- Surcharge d'opérateurs

    //-------------------------------------------- Constructeurs - destructeur
    time_wheel_timer() : cur_slot(0)
    {
        for (int i = 0; i < N; i++)
        {
            this->slots[i] = NULL;
        }
    }
    // Mode d'emploi :
    // Constructeur par défaut de la classe <time_wheel_timer>
    //
    // Contrat :
    //

    ~time_wheel_timer()
    {
        for (size_t i = 0; i < N; i++)
        {
            tw_timer *tmp = slots[i];
            while (tmp)
            {
                slots[i] = tmp->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }

    // add a timer into time wheel
    tw_timer *add_timer(int timeout)
    {
        if (timeout < 0)
        {
            return NULL;
        }
        // first we calculate the offset
        int offset;
        if (timeout / SI == 0)
        {
            offset = 1;
        }
        else
        {
            offset = timeout / SI;
        }
        int rotation = offset / N;
        int slot_pos = (cur_slot + offset) % N;
        tw_timer *timer = new tw_timer(rotation, slot_pos);
        if (!slots[slot_pos])
        {
            slots[slot_pos] = timer;
        }
        else
        {
            timer->next = slots[slot_pos];
            timer->next->prev = timer;
            slots[slot_pos] = timer;
        }
        return timer;
    }

    // delete the used timer
    void del_timer(tw_timer *timer)
    {
        if (!timer)
        {
            return;
        }

        int ts = timer->time_slot;
        if (timer == slots[ts])
        {
            slots[ts] = timer->next;
            if (slots[ts])
            {
                slots[ts]->prev = NULL;
            }
            delete timer;
        }
        else
        {
            timer->prev->next = timer->next;
            if (timer->next)
            {
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
    }

    // pass SI second, the cur_slot += 1, clean some outdated timer
    void tick()
    {
        tw_timer *tmp = slots[cur_slot];
        while (tmp)
        {
            if (tmp->rotation > 0)
            {
                tmp->rotation -= 1;
                tmp = tmp->next;
            }
            else
            {
                tmp->cb_func(tmp->user_data);
                if (tmp == slots[cur_slot])
                {
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if (slots[cur_slot])
                    {
                        slots[cur_slot]->prev = NULL;
                    }
                    tmp = slots[cur_slot];
                }
                else
                {
                    tmp->prev->next = tmp->next;
                    if (tmp->next)
                    {
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer *tmp2 = tmp->next;
                    delete (tmp);
                    tmp = tmp2;
                }
            }
        }
        cur_slot = ++cur_slot % N;
    }

protected:
    //----------------------------------------------------- Attributs protégés
    static const int N = 60;
    static const int SI = 1; // the interval time is 1s
    tw_timer *slots[N];
    int cur_slot;
};

//--------------------------- Autres définitions dépendantes de <time_wheel_timer>

#endif // TIME_WHEEL_TIMER_H