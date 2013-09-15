#pragma once

#include <vector>
#include <memory>

namespace Moses
{

class Manager;
class Hypothesis;

/** SearchGraph class provides an abstraction for the search graph
 * It includes information about all feature functions and their weights
 */
class SearchGraph
{
public:
  typedef size_t VertexId;
  class Edge;

  explicit SearchGraph(const Manager &manager);
  ~SearchGraph();

  void Search(size_t pass);

private:
  std::vector<Edge*> m_edges;
};

/** SearchGraph::Edge class provides a view to edges of the search graph
 *  Each edge matches a single Hypothesis. However once edge is created,
 *  it only retains link to the TranslationOption, but not Hypothesis itself.
 */
class SearchGraph::Edge
{
public:
  Edge(Hypothesis &hypo, const Edge *next, std::vector<Edge*> &edges);
  ~Edge();

  Hypothesis &GetHypothesis()
  { return m_hypo; }
private:
  Hypothesis &m_hypo;
  const Edge *m_next;
};


} //namespace Moses

