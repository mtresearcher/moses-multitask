#pragma once

#include <vector>
#include <set>
#include <memory>

namespace Moses
{

class Manager;
class Hypothesis;
class SearchGraph;
class CollEdge;

/** SearchGraph::Edge class provides a view to edges of the search graph
 *  Each edge matches a single Hypothesis. However once edge is created,
 *  it only retains link to the TranslationOption, but not Hypothesis itself.
 */
class Edge
{
  friend std::ostream& operator<<(std::ostream &out, const Edge &obj);

public:
  Edge(Hypothesis &hypo, const Edge *next, CollEdge &edges);
  ~Edge();

  Hypothesis &GetHypothesis()
  { return m_hypo; }

  //! transitive comparison used for adding objects into FactorCollection
  inline bool operator<(const Edge &compare) const {
    return &this->m_hypo < &compare.m_hypo;
  }

private:
  Hypothesis &m_hypo;
  const Edge *m_next;
};

class EdgeOrderer
{
public:
  bool operator()(const Edge* edgeA, const Edge *edgeB) const {
	  return (*edgeA) < (*edgeB);
  }
};

class CollEdge : public std::set<Edge*, EdgeOrderer>
{

};

/** SearchGraph class provides an abstraction for the search graph
 * It includes information about all feature functions and their weights
 */
class SearchGraph
{
public:
  typedef size_t VertexId;

  explicit SearchGraph(const Manager &manager);
  ~SearchGraph();

  void Search(size_t pass);

private:
  CollEdge m_edges;
};



} //namespace Moses

