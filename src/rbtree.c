#include "cutils/rbtree.h"
#include <assert.h>

static inline void setblack(rbnode *n) {n->parent_color &= ~(uintptr_t)1;}
static inline void setred(rbnode *n) {n->parent_color |= (uintptr_t)1;}
static inline uintptr_t isred(rbnode *n) {return (n->parent_color & (uintptr_t)1);}
static inline void setparent(rbnode *n, rbnode *parent, uintptr_t red) {
	n->parent_color = (uintptr_t)parent | red;
}

static void rotate(rbtree *tree, rbnode *n, int dir) {
	// rotate left (if dir == 0)
	//   N    ->   b
	// a   b     N   z
	//     yz   ay
	rbnode *b = n->child[!dir];
	rbnode *y = b->child[dir];
	rbnode *p = rb_parent(n);
	if (!p) {
		tree->root = b;
	} else if (p->child[0] == n) {
		p->child[0] = b;
	} else {
		p->child[1] = b;
	}
	setparent(b, p, isred(b));
	b->child[dir] = n;
	setparent(n, b, isred(n));
	n->child[!dir] = y;
	if (y) {
		setparent(y, n, isred(y));
	}
}

static rbnode *end(rbnode *n, int dir) {
	while (n->child[dir]) {
		n = n->child[dir];
	}
	return n;
}

void rb_insert(rbtree *tree, rbnode *p, rbnode *n, rbdirection dir) {
	tree->size++;
	n->child[0] = NULL;
	n->child[1] = NULL;

	if (!p) {
		tree->root = n;
		n->parent_color = 0;
		return;
	}

	if (p->child[dir]) {
		p = end(p->child[dir], !dir);
		p->child[!dir] = n;
	} else {
		p->child[dir] = n;
	}
	setparent(n, p, 1);

	for (;;) {
		p = rb_parent(n);

		// case 2 - parent is black
		// black parents are allowed to have red children
		if (!isred(p)) {
			return;
		}
		// case 3 - parent and uncle are red
		// since the parent is red, it can't be the root
		// this allows the parent to have a red child
		// but now we have an extra black depth for anything going through g
		// loop around at g, as g now has a red child to deal with
		rbnode *g = rb_parent(p);
		int pdir = (p == g->child[0]) ? 0 : 1;
		rbnode *u = g->child[!pdir];
		if (u && isred(u)) {
			setblack(p);
			setblack(u);
			if (g->parent_color) {
				setred(g);
				n = g;
				continue;
			} else {
				// grandparent is root of the tree
				// we've added an extra black depth across the whole tree
				return;
			}
		}

		// case 4 - parent is red, but uncle is black
		// step 1 - rotate to be fully left or fully right
		int ndir = (n == p->child[0]) ? 0 : 1;
		if (ndir != pdir) {
			//    G            G
			//  P    U  TO  N    U
			// a  N        P y
			//    xy      ax
			rotate(tree, p, pdir);
			rbnode *tmp = p;
			p = n;
			n = tmp;
		}

		// case 4
		// step 2 - rotate grandparent
		//    G           P
		// P     U  TO N     G
		//N y               y  U
		rotate(tree, g, !pdir);
		setblack(p);
		setred(g);
	}
}

void rb_remove(rbtree *tree, rbnode *n) {
	tree->size--;

	if (n->child[0] && n->child[1]) {
		// find the immediate predecessor, swap that into n's position and rebalance from that point
		rbnode *a = n->child[0];
		while (a->child[1]) {
			a = a->child[1];
		}

		uintptr_t nred = isred(n);
		rbnode *np = rb_parent(n);
		rbnode *nc0 = n->child[0];
		rbnode *nc1 = n->child[1];

		uintptr_t ared = isred(a);
		rbnode *ap = rb_parent(a);
		rbnode *ac0 = a->child[0];

		// copy first child from a to n
		n->child[0] = ac0;
		if (ac0) {
			setparent(ac0, n, isred(ac0));
		}

		// copy first child from n to a
		// and link n to a's parent
		if (ap == n) {
			setparent(n, a, ared);
			a->child[0] = n;
		} else {
			setparent(n, ap, ared);
			ap->child[1] = n;
			a->child[0] = nc0;
			setparent(nc0, a, isred(nc0));
		}

		// copy second child
		n->child[1] = NULL;
		a->child[1] = nc1;
		if (nc1) {
			setparent(nc1, a, isred(nc1));
		}

		// link a to n's parent
		setparent(a, np, nred);
		if (!np) {
			tree->root = a;
		} else if (np->child[0] == n) {
			np->child[0] = a;
		} else {
			np->child[1] = a;
		}
	}

	// at this point n has 0 or 1 children
	rbnode *c = n->child[0] ? n->child[0] : n->child[1];
	rbnode *p = rb_parent(n);
	uintptr_t nred = isred(n);
	n->parent_color = 0;
	n->child[0] = NULL;
	n->child[1] = NULL;

	if (c) {
		setparent(c, p, isred(c));
	}

	if (!p) {
		// We've removed the root. The only child is now either the new root
		// or there is no new root. Need to make sure the root is black.
		// Otherwise don't need to rebalance as we've effected the black depth
		// for the whole tree.
		tree->root = c;
		if (c) {
			setblack(c);
		}
		return;
	}

	int dir = (p->child[0] == n) ? 0 : 1;
	p->child[dir] = c;

	// Trivial cases
	if (nred) {
		// n -> C
		//C
		return;
	} else if (c && isred(c)) {
		//  N  -> C
		//c
		setblack(c);
		return;
	}

	// both n & c are black
	// c may or may not exist

	for (;;) {
		// main loop
		// we need to rebalance around p
		// its 'dir' side has one less black then the other side

		// s must exist in order for the dir side to have a lower black depth
		rbnode *s = p->child[!dir];
		if (isred(s)) {
			// case 2 - sibling is red
			// the sibling must have 2 children in order for the sibling side
			// of the tree to have a higher black depth
			// rotate around P and swap colors
			// then fall down on the conditions below to figure out what to do
			// with the red p
			//     P              S
			//  N     s     TO  p     R
			//      L   R     N   L   CD
			//     AB   CD        AB
			// upper = black
			// lower = red
			setblack(s);
			setred(p);
			rotate(tree, p, dir);
			s = p->child[!dir];
		}

		rbnode *snear = s->child[dir];
		rbnode *sfar = s->child[!dir];
		int near_red = snear && isred(snear);
		int far_red = sfar && isred(sfar);
		if (!isred(s) && !near_red && !far_red) {
			if (isred(p)) {
				//   p           P
				// N   S   TO  N   s
				//    L  R        L  R
				setblack(p);
				setred(s);
				return;
			} else {
				//   P            P
				// N    S   TO  N   s
				//     L  R        L R
				//
				// this will rebalance around P, but P has a lower depth
				// then P's sibling so loop around one layer up
				setred(s);
				rbnode *g = rb_parent(p);
				if (g) {
					dir = (g->child[0] == p) ? 0 : 1;
					p = g;
					continue;
				} else {
					return;
				}
			}
		}

		if (!far_red) {
			// rotate around s so that we get a far red
			//   S   ->   L
			// l   R       s
			//              R
			rotate(tree, s, !dir);
			sfar = s;
			s = snear;
		}

		//   P*         S*
		// N   S  TO  P   R
		//      r    N
		if (isred(p)) {
			setred(s);
		} else {
			setblack(s);
		}
		setblack(p);
		setblack(sfar);
		rotate(tree, p, dir);
		return;
	}
}

rbnode *rb_begin(const rbtree *tree, rbdirection dir) {
	return tree->root ? end(tree->root, dir) : NULL;
}

rbnode *rb_next(const rbnode *n, rbdirection dir) {
	rbnode *c = n->child[dir];
	if (c) {
		return end(c, !dir);
	}

	for (;;) {
		rbnode *p = rb_parent(n);
		if (!p || n == p->child[!dir]) {
			return p;
		}
		n = p;
	}
}

