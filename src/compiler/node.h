#ifndef DUB_COMPILER_NODE_H_
#define DUB_COMPILER_NODE_H_

#include <cstdint>

#include "src/compiler/expression.h"

namespace dub::compiler {

// Two problems:
//  1. Decoupling "metadata" from actual form/expression
//  2. Recursive definitions of structure.
//
// What is the metadata?
//  Source location(filename, line, column etc.), Type information etc.
//
// Why does the metadata need to be decoupled?
//  1. Metadata is not part of the form/expression value identity (not used in `==` and `hashcode`.
//  2. Metadata is not known until appropriate passes are done (type checking for type info).
//  3. Because of point (2), the "const-ness" of the Form/Expression is not true.
//
// What is the major challenge?
//  Wrapping form/expression with `Node<Form>` does not work! Because a `Form` can also be
//  a `List<Form>. We want it to be a List<Node>. Similar with `Expression`, an `Expression`
//  contains multiple subexpressions.
//
//
// Solution #1: introduce a template parameter for `Expression`.
//  Problem: Node<Expression<Node<...>>>, this is infinite.
//   Expression<Expression<Expression<...>, this is also infinite.
//
// Solution #2:
//  On top of solution #1, do the following:
//  class ExprNode : Node<ExprNode> {
//    Expression<ExprNode> value_;
//  }
//
// Solution #3:
//  Wrap Expressions into Nodes in the Module.
//  Expressions when standalone do are just values, and cannot have metadata associated.
//  When Expressions are in a Module, they have IDs associated with them.
//
//  During parsing, the Module should be used to create new Nodes which are in a linear
//  memory. The position in the linear memory is also the ID for the Node. During semantic
//  passes, the type information will be stored in another linear memory.
//  The ID associates the type information with the node.
//
//  Problem: how do I traverse sub-nodes correctly? This seems overcomplicated.

template <typename T>
class Node {
 public:
 private:
  T* value_;
  std::uint64_t id_;
};

class ExprNode final : public Node<ExprNode> {
 private:
  Expression value_;
};

}  // namespace dub::compiler

#endif /* DUB_COMPILER_NODE_H_ */
