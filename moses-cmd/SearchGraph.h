#pragma once

#include <vector>
#include <memory>

namespace Moses
{

class TranslationOption;
class Hypothesis;
class Manager;
class FeatureFunction;

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

  void Search(size_t pass);
private:
  friend class SearchGraphBuilder;

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
  Edge(VertexId begin, VertexId end, const Hypothesis& hypothesis, const std::vector<FeatureFunction*>& allFFs);
public:
  Edge() :
    m_begin(0), m_end(0),
    m_featureScores(),
    m_totalScore(std::numeric_limits<double>::infinity()),
    m_translationOption(NULL)
  {}

  Edge(const Edge& other) :
    m_begin(other.m_begin), m_end(other.m_end),
    m_featureScores(other.m_featureScores),
    m_totalScore(other.m_totalScore),
    m_translationOption(other.m_translationOption)
  {}

  VertexId Begin() const
  {
    return m_begin;
  }
  VertexId End() const
  {
    return m_end;
  }
  const std::vector<float> FeatureScores(size_t featureId) const
  {
    return m_featureScores.at(featureId);
  }
  float TotalScore() const
  {
    return m_totalScore;
  }
  const TranslationOption& GetTranslationOption() const {
    return *m_translationOption;
  }
  std::string GetSourceText() const;
  std::string GetTargetText() const;
private:
  friend class SearchGraphBuilder;
  VertexId m_begin; //! origin of the edge
  VertexId m_end;   //! end of the edge
  std::vector< std::vector<float> > m_featureScores;
  float m_totalScore;
  const TranslationOption* m_translationOption;
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

