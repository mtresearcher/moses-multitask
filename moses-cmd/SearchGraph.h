#pragma once

#include <vector>
#include <memory>

namespace Moses
{

class Manager;
class WordsRange;

/** SearchGraph class provides an abstraction for the search graph
 * It includes information about all feature functions and their weights
 */
class SearchGraph
{
public:
  typedef size_t VertexId;
  class Edge;
  class EdgeIterator;

  explicit SearchGraph(const Manager &manager);

  // Edge iterating functions
  std::auto_ptr<EdgeIterator> GetOutgoingEdgesIter(VertexId v) const;
  std::auto_ptr<EdgeIterator> GetIncomingEdgesIter(VertexId v) const;
  std::auto_ptr<EdgeIterator> GetAllEdgesIter() const;

  size_t NumberOfVertices() const
  {
    return m_incomingEdges.size();
  }

  // Information about features
  size_t NumberOfFeatures() const
  {
    return m_featureWeights.size();
  }

  std::string FeatureDescription(size_t featureIndex) const;
  const std::vector<float>& FeatureWeights(size_t featureIndex) const;
private:
  // Helpers
  void UpdateEdgeScore(Edge* edge);

  class Builder;

  // TODO: encapsulate all fields (as in Edge class)
  std::vector< Edge > m_allEdges;
  std::vector< std::vector< VertexId > > m_outgoingEdges, m_incomingEdges;
  std::vector< std::vector<float> > m_featureWeights;
  std::vector< std::string > m_featureDescriptions;
};

/** SearchGraph::Edge class provides a view to edges of the search graph
 *  Each edge matches a single Hypothesis. However once edge is created,
 *  it only retains link to the TranslationOption, but not Hypothesis itself.
 */
class SearchGraph::Edge
{
public:
  Edge();
  Edge(const Edge& other);
  ~Edge();
  VertexId Begin() const;
  VertexId End() const;
  const std::vector<float>& FeatureScores(size_t featureId) const;
  float TotalScore() const;
  std::string GetSourceText() const;
  std::string GetTargetText() const;
  WordsRange SourceWordsRange() const;
private:
  friend class SearchGraph;
  class Impl;
  Impl *m_impl;
};

class SearchGraph::EdgeIterator
{
public:
  virtual const Edge& CurrentEdge() const = 0;
  virtual bool Valid() const = 0;
  virtual void Next() = 0;
  virtual ~EdgeIterator() {}
};

} //namespace Moses

