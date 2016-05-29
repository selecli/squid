#include "preloader.h"

/* red-black tree:
* A special sorted binary tree.
* It is efficent for insert, find, delete.
* It tries to keep the completly balanced tree's attributes, but it's not a strictly completly balanced tree.
* Time Complexity: log(N)
*/

struct rbtree_node_st *rbtree_node_create(
    int color,
    long key,
    struct rbtree_node_st *parent,
    struct rbtree_node_st *left,
    struct rbtree_node_st *right)
{
    struct rbtree_node_st *node;

    node         = xmalloc(g_tsize.rbtree_node_st);
    node->color  = color;
    node->key    = key;
    node->parent = parent;
    node->left   = left;
    node->right  = right;

    return node;
}

struct rbtree_st *rbtree_create(IST_HDL *insert_handler)
{
    struct rbtree_st       *rbtree;
    struct rbtree_node_st  *sentinel;
    pthread_mutexattr_t     mutexattr;

    rbtree           = xmalloc(g_tsize.rbtree_st);
    sentinel         = rbtree_node_create(RBTREE_BLACK, 0, NULL, NULL, NULL);
    rbtree->root     = sentinel;
    rbtree->sentinel = sentinel;
    rbtree->insert   = insert_handler;
    rbtree->number   = 0;

    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&rbtree->mutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);

    return rbtree;
}

static struct rbtree_node_st *rbtree_find_min(struct rbtree_st *rbtree, struct rbtree_node_st *node)
{
    struct rbtree_node_st *min;

    pthread_mutex_lock(&rbtree->mutex);
    for (min = node; min->left != rbtree->sentinel; min = min->left) ;
    pthread_mutex_unlock(&rbtree->mutex);

    return min;
}

void rbtree_left_rotate(struct rbtree_st *rbtree, struct rbtree_node_st *node)
{
    struct rbtree_node_st *right_node;

    pthread_mutex_lock(&rbtree->mutex);

    right_node  = node->right;
    node->right = right_node->left;

    if (right_node->left != rbtree->sentinel)
    {
        right_node->left->parent = node;
    }

    right_node->parent = node->parent;

    if (node == rbtree->root)
    {
        rbtree->root = right_node;
    }
    else if (node == node->parent->left)
    {
        node->parent->left = right_node;
    }
    else
    {
        node->parent->right = right_node;
    }

    right_node->left = node;
    node->parent     = right_node;

    pthread_mutex_unlock(&rbtree->mutex);
}

void rbtree_right_rotate(struct rbtree_st *rbtree, struct rbtree_node_st *node)
{
    struct rbtree_node_st *left_node;

    pthread_mutex_lock(&rbtree->mutex);

    left_node  = node->left;
    node->left = left_node->right;

    if (left_node->right != rbtree->sentinel)
    {
        left_node->right->parent = node;
    }

    if (node == rbtree->root)
    {
        rbtree->root = left_node;
    }
    else if (left_node == node->parent->left)
    {
        node->parent->left = left_node;
    }
    else
    {
        node->parent->right = left_node;
    }

    left_node->right = node;
    node->parent     = left_node;

    pthread_mutex_unlock(&rbtree->mutex);
}

void rbtree_remove(struct rbtree_st *rbtree, struct rbtree_node_st *node)
{
    unsigned int            color;
    struct rbtree_node_st  *temp;
    struct rbtree_node_st  *tnode;
    struct rbtree_node_st  *subst;

    pthread_mutex_lock(&rbtree->mutex);

    rbtree->number--;

    if (node->left == rbtree->sentinel)
    {
        temp  = node->right;
        subst = node;
    }
    else if (node->right == rbtree->sentinel)
    {
        temp  = node->left;
        subst = node;
    }
    else
    {
        subst = rbtree_find_min(rbtree, node->right);
        temp  = subst->left == rbtree->sentinel ? subst->right : subst->left;
    }

    if (subst == rbtree->root)
    {
        temp->color  = RBTREE_BLACK;
        rbtree->root = temp;
        pthread_mutex_unlock(&rbtree->mutex);
        return;
    }

    color = subst->color;

    if (subst == subst->parent->left)
    {
        subst->parent->left = temp;
    }
    else
    {
        subst->parent->right = temp;
    }

    if (subst == node)
    {
        temp->parent = subst->parent;
    }
    else
    {
        temp->parent  = subst->parent == node ? subst: subst->parent;
        subst->left   = node->left;
        subst->right  = node->right;
        subst->parent = node->parent;
        subst->color  = node->color;

        if (node == rbtree->root)
        {
            rbtree->root = subst;
        }
        else
        {
            if (node == node->parent->left)
            {
                node->parent->left = subst;
            }
            else
            {
                node->parent->right = subst;
            }
        }

        if (subst->left != rbtree->sentinel)
        {
            subst->left->parent = subst;
        }

        if (subst->right != rbtree->sentinel)
        {
            subst->right->parent = subst;
        }
    }

    if (RBTREE_RED == color)
    {
        pthread_mutex_unlock(&rbtree->mutex);
        return;
    }

    while (temp != rbtree->root && RBTREE_BLACK == temp->color)
    {
        if (temp == temp->parent->left)
        {
            tnode = temp->parent->right;

            if (RBTREE_RED == tnode->color)
            {
                tnode->color        = RBTREE_BLACK;
                temp->parent->color = RBTREE_RED;
                rbtree_left_rotate(rbtree, temp->parent);
                tnode               = temp->parent->right;
            }

            if (RBTREE_BLACK == tnode->left->color && RBTREE_BLACK == tnode->right->color)
            {
                tnode->color = RBTREE_RED;
                temp         = temp->parent;
            }
            else
            {
                if (RBTREE_BLACK == tnode->right->color)
                {
                    tnode->left->color = RBTREE_BLACK;
                    tnode->color       = RBTREE_RED;
                    rbtree_right_rotate(rbtree, tnode);
                    tnode              = temp->parent->right;
                }
                tnode->color        = temp->parent->color;
                temp->parent->color = RBTREE_BLACK;
                tnode->right->color = RBTREE_BLACK;
                rbtree_left_rotate(rbtree, temp->parent);
                temp                = rbtree->root;
            }
        }
        else
        {
            tnode = temp->parent->left;

            if (RBTREE_RED == tnode->color)
            {
                tnode->color        = RBTREE_BLACK;
                temp->parent->color = RBTREE_RED;
                rbtree_right_rotate(rbtree, temp->parent);
                tnode               = temp->parent->left;
            }

            if (RBTREE_BLACK == tnode->left->color && RBTREE_BLACK == tnode->right->color)
            {
                tnode->color = RBTREE_RED;
                temp         = temp->parent;
            }
            else
            {
                if (RBTREE_BLACK == tnode->left->color)
                {
                    tnode->right->color = RBTREE_BLACK;
                    tnode->color        = RBTREE_RED;
                    rbtree_left_rotate(rbtree, tnode);
                    tnode               = temp->parent->left;
                }
                tnode->color        = temp->parent->color;
                temp->parent->color = RBTREE_BLACK;
                tnode->left->color  = RBTREE_BLACK;
                rbtree_right_rotate(rbtree, temp->parent);
                temp                = rbtree->root;
            }
        }
    }

    temp->color = RBTREE_BLACK;

    pthread_mutex_unlock(&rbtree->mutex);
}

void rbtree_process_event_timeout(struct rbtree_st *rbtree)
{
    ssize_t                 offset;
    time_t                  curtime;
    struct io_event_st     *ev;
    struct rbtree_node_st  *node;

    time(&curtime);

    for ( ; ; )
    {
        pthread_mutex_lock(&rbtree->mutex);

        if (rbtree->root == rbtree->sentinel)
        {
            pthread_mutex_unlock(&rbtree->mutex);
            return;
        }

        node = rbtree_find_min(rbtree, rbtree->root);

        if (node->key > curtime)
        {
            /* no timeout */
            pthread_mutex_unlock(&rbtree->mutex);
            return;
        }

        offset = offsetof(struct io_event_st, timer);
        ev = (struct io_event_st *)((char *)node - offset);
        if (pthread_mutex_trylock(&ev->mutex))
        {
            pthread_mutex_unlock(&rbtree->mutex);
            return;
        }
        rbtree_remove(rbtree, node);
        pthread_mutex_unlock(&rbtree->mutex);
        ev->timer_set       = 0;
        ev->posted_timedout = 1;
        ev->timer.key       = 0;
        ev->timer.left      = NULL;
        ev->timer.right     = NULL;
        ev->timer.parent    = NULL;
        ev->timer.color     = RBTREE_RED;
        pthread_mutex_unlock(&ev->mutex);
    }
}

void rbtree_insert_event_timer(struct rbtree_st *rbtree, struct rbtree_node_st *node)
{
    struct rbtree_node_st   *parent;
    struct rbtree_node_st  **child;

    pthread_mutex_lock(&rbtree->mutex);

    parent  = rbtree->root;

    for ( ; ; )
    {
        child = node->key < parent->key ? &parent->left : &parent->right;
        if ((*child) == rbtree->sentinel)
        {
            break;
        }
        parent = (*child);
    }

    *child       = node;
    node->parent = parent;
    node->color  = RBTREE_RED;
    node->left   = rbtree->sentinel;
    node->right  = rbtree->sentinel;

    pthread_mutex_unlock(&rbtree->mutex);
}

void rbtree_insert(struct rbtree_st *rbtree, struct rbtree_node_st *node)
{
    struct rbtree_node_st *uncle;
    struct rbtree_node_st *grandparent;

    pthread_mutex_lock(&g_rbtree->mutex);

    rbtree->number++;

    if (rbtree->root == rbtree->sentinel)
    {
        node->color  = RBTREE_BLACK;
        node->parent = NULL;
        node->left   = rbtree->sentinel;
        node->right  = rbtree->sentinel;
        rbtree->root = node;
        pthread_mutex_unlock(&g_rbtree->mutex);
        return;
    }

    rbtree->insert(rbtree, node);

    /* rebalance the rbtree. */

    while (node != rbtree->root && RBTREE_RED == node->parent->color)
    {
        grandparent = node->parent->parent;

        if (node->parent == grandparent->left)
        {
            uncle = grandparent->right;

            if (RBTREE_RED == uncle->color)
            {
                node->parent->color = RBTREE_BLACK;
                uncle->color        = RBTREE_BLACK;
                grandparent->color  = RBTREE_RED;
                node                = grandparent;
            }
            else
            {
                if (node == node->parent->right)
                {
                    node = node->parent;
                    rbtree_left_rotate(rbtree, node);
                }
                grandparent         = node->parent->parent;
                node->parent->color = RBTREE_BLACK;
                grandparent->color  = RBTREE_RED;
                rbtree_right_rotate(rbtree, grandparent);
            }
        }
        else
        {
            uncle = grandparent->left;

            if (RBTREE_RED == uncle->color)
            {
                node->parent->color = RBTREE_BLACK;
                uncle->color        = RBTREE_BLACK;
                grandparent->color  = RBTREE_RED;
                node                = grandparent;
            }
            else
            {
                if (node == node->parent->left)
                {
                    node = node->parent;
                    rbtree_right_rotate(rbtree, node);
                }
                grandparent         = node->parent->parent;
                node->parent->color = RBTREE_BLACK;
                grandparent->color  = RBTREE_RED;
                rbtree_left_rotate(rbtree, grandparent);
            }
        }
    }

    rbtree->root->color = RBTREE_BLACK;

    pthread_mutex_unlock(&g_rbtree->mutex);
}

void rbtree_node_destroy(struct rbtree_node_st *node)
{
    safe_process(free, node);
}

void rbtree_destroy(struct rbtree_st *rbtree)
{
    /* FIXME: maybe have node here. */
    pthread_mutex_destroy(&rbtree->mutex);
    safe_process(rbtree_node_destroy, rbtree->sentinel);
    safe_process(free, rbtree);
}
