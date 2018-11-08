#include "cutils/heap.h"

void heap_insert(struct heap *h, struct heap_node *n) {
	h->size++;
	struct heap_node *p = h->head;
	if (!p || h->before(n, p)) {
		n->parent = NULL;
		n->left = p;
		n->right = NULL;
		h->head = n;
		if (n->left) {
			n->left->parent = n;
		}
	} else {
		n->parent = p;
		n->left = NULL;
		n->right = p->left;
		if (n->right) {
			n->right->parent = n;
		}
		p->left = n;
	}
}

static struct heap_node *merge2(struct heap_node *a, struct heap_node *b, heap_before before) {
	if (before(b, a)) {
		struct heap_node *tmp = a;
		a = b;
		b = tmp;
	}
	// a      + b      = a
	// achild   bchild   b achild
	//                   bchild
	b->right = a->left;
	if (b->right) {
		b->right->parent = b;
	}
	a->left = b;
	b->parent = a;
	return a;
}

static struct heap_node *merge_list(struct heap_node *r, heap_before before) {
	// Take the list of children and merge consecutive pairs.
	// This forms a new list of children that is again
	// merged as pairs. Keep on doing this until we have only
	// 0 or 1 children in the list. This method finds the replacement
	// in O(log(n)) of the number of new nodes. It also remembers
	// those checks to make the next round quicker.
	while (r && r->right) {
		// take the previous list from r
		// and start a new one from r
		struct heap_node *cnext = r;
		r = NULL;
		while (cnext) {
			struct heap_node *c1 = cnext;
			struct heap_node *c2 = c1->right;
			if (!c2) {
				r->parent = c1;
				c1->right = r;
				r = c1;
				break;
			}
			// copy out cnext as merge2 and the pair->right call
			// may modify c2->right
			cnext = c2->right;
			struct heap_node *pair = merge2(c1, c2, before);
			if (r) {
				r->parent = pair;
			}
			pair->right = r;
			r = pair;
		}
	}
	return r;
}

void heap_remove(struct heap *h, struct heap_node *n) {
	if (!n->parent && n != h->head) {
		return;
	}

	h->size--;

	struct heap_node *r = NULL;
	if (n->left) {
		r = merge_list(n->left, h->before);
		r->parent = n->parent;
		r->right = n->right;
		if (r->right) {
			r->right->parent = r;
		}
	} else if (n->right) {
		r = n->right;
		r->parent = n->parent;
	}

	struct heap_node *p = n->parent;
	if (!p) {
		h->head = r;
	} else if (p->left == n) {
		p->left = r;
	} else {
		p->right = r;
	}

	n->parent = NULL;
	n->left = NULL;
	n->right = NULL;
}

void heap_update(struct heap *h, struct heap_node *n) {
	struct heap_node *r = n->right;
	struct heap_node *p = n->parent;
	// the existing n->left->right->right-> ... etc list
	// is the list of nodes behind n
	// n has moved, so we remerge n with that list
	n->right = n->left;
	n->left = NULL;
	struct heap_node *o = merge_list(n, h->before);
	o->right = r;
	if (r) {
		r->parent = o;
	}

	o->parent = p;
	if (!p) {
		h->head = o;
	} else if (p->left == n) {
		p->left = o;
	} else {
		p->right = o;
	}
}
