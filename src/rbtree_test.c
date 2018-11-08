#include "cutils/rbtree.h"
#include "cutils/test.h"
#include "cutils/log.h"

struct int_node {
	struct rbnode rb;
	int value;
};

static const char spaces[] = "                        ";

static int node_id(const rbnode *n) {
	if (!n) {
		return 0;
	}
	struct int_node *in = container_of(n, struct int_node, rb);
	return in->value;
}

static inline uintptr_t isred(rbnode *n) {return (n->parent_color & 1U);}

static void log_node(log_t *log, rbnode *n, int depth) {
	rbnode *left = rb_child(n, RB_LEFT);
	rbnode *right = rb_child(n, RB_RIGHT);
	rbnode *parent = rb_parent(n);
	if (left) {
		log_node(log, left, depth+1);
	}
	struct int_node *in = container_of(n, struct int_node, rb);
	LOG(log, "%.*s%d - %d %s",
		depth*2, spaces,
		in->value,
		node_id(parent),
		isred(n) ? "RED" : "BLACK");

	if (right) {
		log_node(log, right, depth+1);
	}
}

static void log_tree(log_t *log, const rbtree *tree) {
	LOG(log, "log tree %d", tree->size);
	if (tree->root) {
		log_node(log, tree->root, 0);
	}
	LOG(log, "\n");
}

static int black_depth(rbnode *n) {
	int ret = isred(n) ? 0 : 1;
	rbnode *left = rb_child(n, RB_LEFT);
	return ret + (left ? black_depth(left) : 0);
}

static int check_node(rbnode *n, int prev) {
	struct int_node *in = container_of(n, struct int_node, rb);
	int left_black = 0, right_black = 0;
	rbnode *left = rb_child(n, RB_LEFT);
	rbnode *right = rb_child(n, RB_RIGHT);
	if (left) {
		prev = check_node(left, prev);
		left_black = black_depth(n->child[0]);
	}
	EXPECT_GT(in->value, prev);
	prev = in->value;
	if (right) {
		prev = check_node(right, prev);
		right_black = black_depth(right);
	}
	if (isred(n)) {
		EXPECT_TRUE(!left || !isred(left));
		EXPECT_TRUE(!right || !isred(right));
	}
	CHECK(left_black == right_black, "black depth mismatch at node %d, left %d, right %d",
		node_id(n), left_black, right_black);
	return prev;
}

static void check_tree(rbtree *tree) {
	if (tree->root) {
		check_node(tree->root, 0);
		EXPECT_TRUE(!isred(tree->root));
	}
	int prev = 0;
	int count = 0;
	for (rbnode *n = rb_begin(tree, RB_LEFT); n != NULL; n = rb_next(n, RB_RIGHT)) {
		struct int_node *in = container_of(n, struct int_node, rb);
		EXPECT_GT(in->value, prev);
		prev = in->value;
		count++;
	}
	EXPECT_EQ(tree->size, count);
	prev++;
	count = 0;
	for (rbnode *n = rb_begin(tree, RB_RIGHT); n != NULL; n = rb_next(n, RB_LEFT)) {
		struct int_node *in = container_of(n, struct int_node, rb);
		EXPECT_GT(prev, in->value);
		prev = in->value;
		count++;
	}
	EXPECT_EQ(tree->size, count);
}

static void insert_node(log_t *log, rbtree *tree, struct int_node *in) {
	rbnode *p = tree->root;
	rbdirection dir = RB_LEFT;
	while (p) {
		struct int_node *ip = container_of(p, struct int_node, rb);
		dir = (in->value < ip->value) ? RB_LEFT : RB_RIGHT;
		if (!rb_child(p, dir)) {
			break;
		}
		p = rb_child(p, dir);
	}
	LOG(log, "insert %d to %d %s", in->value, node_id(p), (dir == RB_LEFT) ? "LEFT" : "RIGHT");
	rb_insert(tree, p, &in->rb, dir);
	log_tree(log, tree);
	check_tree(tree);
}

static void remove_node(log_t *log, rbtree *tree, struct int_node *in) {
	LOG(log, "remove %d", in->value);
	rb_remove(tree, &in->rb);
	log_tree(log, tree);
	check_tree(tree);
}

int main(int argc, const char *argv[]) {
	log_t *log = start_test(argc, argv);
	struct int_node n[30];
	for (int i = 0; i < sizeof(n)/sizeof(n[0]); i++) {
		n[i].value = i;
	}
	struct rbtree tree = {NULL, 0};
	insert_node(log, &tree, n+6);
	insert_node(log, &tree, n+5);
	insert_node(log, &tree, n+3);
	insert_node(log, &tree, n+4);
	insert_node(log, &tree, n+1);
	insert_node(log, &tree, n+2);
	insert_node(log, &tree, n+18);
	insert_node(log, &tree, n+17);
	insert_node(log, &tree, n+19);
	insert_node(log, &tree, n+20);
	insert_node(log, &tree, n+21);
	insert_node(log, &tree, n+22);
	insert_node(log, &tree, n+23);
	insert_node(log, &tree, n+7);
	insert_node(log, &tree, n+11);
	insert_node(log, &tree, n+8);
	insert_node(log, &tree, n+10);
	insert_node(log, &tree, n+12);
	insert_node(log, &tree, n+13);
	insert_node(log, &tree, n+14);
	insert_node(log, &tree, n+15);
	insert_node(log, &tree, n+16);
	insert_node(log, &tree, n+9);

	remove_node(log, &tree, n+5);
	remove_node(log, &tree, n+3);
	remove_node(log, &tree, n+2);
	remove_node(log, &tree, n+1);
	remove_node(log, &tree, n+4);
	remove_node(log, &tree, n+6);
	remove_node(log, &tree, n+8);
	remove_node(log, &tree, n+21);
	remove_node(log, &tree, n+22);
	remove_node(log, &tree, n+23);
	remove_node(log, &tree, n+20);
	remove_node(log, &tree, n+15);
	remove_node(log, &tree, n+16);
	remove_node(log, &tree, n+17);
	remove_node(log, &tree, n+18);
	remove_node(log, &tree, n+19);
	remove_node(log, &tree, n+10);
	remove_node(log, &tree, n+11);
	remove_node(log, &tree, n+12);
	remove_node(log, &tree, n+7);
	remove_node(log, &tree, n+9);
	remove_node(log, &tree, n+13);
	remove_node(log, &tree, n+14);

	return finish_test();
}
