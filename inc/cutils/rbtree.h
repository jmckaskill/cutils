#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct rbnode rbnode;
typedef struct rbtree rbtree;

typedef enum rbdirection {
	RB_LEFT,
	RB_RIGHT,
} rbdirection;

struct rbnode {
	uintptr_t parent_color;
	rbnode *child[2];
};

struct rbtree {
	rbnode *root;
	size_t size;
};


#define RB_INIT {NULL, 0}
static inline rbnode *rb_parent(const rbnode *n) {return (rbnode *) (n->parent_color &~ (uintptr_t)1);}
static inline rbnode *rb_child(const rbnode *n, rbdirection dir) {return n->child[dir];}

void rb_insert(rbtree *tree, rbnode *parent, rbnode *node, rbdirection dir);
void rb_remove(rbtree *tree, rbnode *n);

rbnode *rb_begin(const rbtree *tree, rbdirection dir);
rbnode *rb_next(const rbnode *node, rbdirection dir);

#ifndef container_of
#define container_of(ptr, type, member) ((type*) ((char*) (ptr) - offsetof(type, member)))
#endif
