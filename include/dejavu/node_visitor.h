#ifndef NODE_VISITOR_H
#define NODE_VISITOR_H

#include "node.h"

template <typename subclass, typename ret = void>
struct node_visitor {
	ret visit(node *n) {
		switch (n->type) {
		default: return ret();
#		define NODE(X) case X ## _node: \
			return static_cast<subclass*>(this)->visit_ ## X( \
				static_cast<X*>(n) \
			);
#		include "nodes.tbl"
		}
	}

#	define NODE(X) ret visit_ ## X(X*) { return ret(); }
#	include "nodes.tbl"
};

#endif
