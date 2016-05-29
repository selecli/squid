
#ifndef SQUID_ARRAY_H
#define SQUID_ARRAY_H

/* see Array.c for more documentation */

typedef struct {
    int capacity;
    int count;
    void **items;
} Array;


extern Array *arrayCreate(void);
extern void arrayInit(Array * s);
extern void arrayClean(Array * s);
extern void arrayDestroy(Array * s);
extern void arrayAppend(Array * s, void *obj);
extern void arrayInsert(Array * s, void *obj, int position);
extern void arrayPreAppend(Array * s, int app_count);
extern void arrayShrink(Array *a, int new_count);



#endif /* SQUID_ARRAY_H */
