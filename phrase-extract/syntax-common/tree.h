#pragma once

#include <vector>

namespace MosesTraining {
namespace Syntax {

// A basic k-ary tree with node values of type T.  Each node has a vector of
// pointers to its children and a pointer to its parent (or 0 for the root).
//
// See the unit tests in tree_test.cc for examples of usage.
//
// Note: a Tree owns its children: it will delete them on destruction.
//
// Note: it's the user's responsibility to ensure that parent and child pointers
// are correctly set and maintained.  A convenient(-ish) way of building a
// properly-connected tree is to add all the nodes as children of their
// respective parents (using the children() accessor) and then call
// SetParents() on the root at the end.
//
template<typename T>
class Tree {
 public:
  // Constructors
  Tree()
      : value_()
      , children_()
      , parent_(0) {}

  Tree(const T &value)
      : value_(value)
      , children_()
      , parent_(0) {}

  // Destructor (deletes children)
  ~Tree();

  // Access tree's value.
  const T &value() const { return value_; }
  T &value() { return value_; }

  // Access tree's parent.
  const Tree *parent() const { return parent_; }
  Tree *&parent() { return parent_; }

  // Access tree's children.
  const std::vector<Tree *> &children() const { return children_; }
  std::vector<Tree *> &children() { return children_; }

  // Set the parent values for this subtree (excluding this node).
  void SetParents();

  // Leaf predicate.
  bool IsLeaf() const { return children_.empty(); }

  // Calculate the depth of this node within the tree (where the root has a
  // depth of 0, root's children have a depth 1, etc).
  std::size_t Depth() const;

  // Iterators
  //
  // All iterators are forward iterators.  Example use:
  //
  //  Tree<int> &root = GetMeATree();
  //  for (Tree<int>::PreOrderIterator p(root);
  //       p != Tree<int>::PreOrderIterator(); ++p) {
  //    std::cout << p->value() << " ";
  //  }

  // Pre-order iterators.
  class PreOrderIterator;
  // class ConstPreOrderIterator; TODO

  // Post-order iterators.
  // class PostOrderIterator; TODO
  // class ConstPostOrderIterator; TODO

  // Leaf iterators (left-to-right).
  // class LeafIterator; TODO
  // class ConstLeafIterator; TODO

 private:
  T value_;
  std::vector<Tree *> children_;
  Tree *parent_;
};

}  // namespace Syntax
}  // namespace MosesTraining

#include "tree-inl.h"
