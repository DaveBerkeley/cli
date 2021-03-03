
#if !defined(__LIST_H__)

#define __LIST_H__

#include <stdbool.h>

#include "mutex.h"

#if defined(__cplusplus)
extern "C" {
#endif 
    
struct ListItem;

typedef struct ListItem *pList;

typedef pList* (*pnext)(pList item);

void list_push(pList *head, pList w, pnext next_fn, struct Mutex *mutex);
void list_append(pList *head, pList w, pnext next_fn, struct Mutex *mutex);
bool list_remove(pList *head, pList w, pnext next_fn, struct Mutex *mutex);
int list_size(pList *head, pnext next_fn, struct Mutex *mutex);

pList list_pop(pList *head, pnext next_fn, struct Mutex *mutex);

typedef int (*cmp_fn)(const pList w1, const pList w2);

void list_add_sorted(pList *head, pList w, pnext next_fn, cmp_fn cmp, struct Mutex *mutex);

bool list_has(pList *head, pList w, pnext next_fn, struct Mutex *mutex);

typedef int (*visitor)(pList w, void *arg);

pList  list_find(pList *head, pnext next_fn, visitor fn, void *arg, struct Mutex *mutex);
void list_visit(pList *head, pnext next_fn, visitor fn, void *arg, struct Mutex *mutex);

#if defined(__cplusplus)
}
#endif 

#if defined(__cplusplus)

    /**
     *
     */

template <class T>
class List
{
public:
    typedef T* (*fn)(T item);

    T head;
    fn next_fn;

    List(fn _fn) : head(0), next_fn(_fn) { }

    void push(T w, Mutex *mutex)
    {
        list_push((pList*) & head, (pList) w, (pnext) next_fn, mutex);
    } 

    void append(T w, Mutex *mutex)
    {
        list_append((pList *) & head, (pList) w, (pnext) next_fn, mutex);
    }

    bool remove(T w, Mutex *mutex)
    {
        return list_remove((pList *) & head, (pList) w, (pnext) next_fn, mutex);
    }

    int size(Mutex *mutex)
    {
        return list_size((pList *) & head, (pnext) next_fn, mutex);
    }

    bool empty()
    {
        return !head;
    }

    T pop(Mutex *mutex)
    {
        return (T) list_pop((pList *) & head, (pnext) next_fn, mutex);
    }

    void add_sorted(T w, int (*cmp)(T a, T b), Mutex *mutex)
    {
        list_add_sorted((pList *) & head, (pList) w, (pnext) next_fn, (cmp_fn) cmp, mutex);
    }

    T find(int (*fn)(T a, void *arg), void *arg, Mutex *mutex)
    {
        return (T) list_find((pList *) & head, (pnext) next_fn, (visitor) fn, arg, mutex);
    }

    bool has(T w, Mutex *mutex)
    {
        return list_has((pList *) & head, (pList) w, (pnext) next_fn, mutex);
    }

    void visit(int (*fn)(T a, void *arg), void *arg, Mutex *mutex)
    {
        list_visit((pList *) & head, (pnext) next_fn, (visitor) fn, arg, mutex);
    }
};

#endif // __cplusplus

#endif // __LIST_H__

//  FIN
