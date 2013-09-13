#include "SearchGraph.h"
#include "moses/Manager.h"

#include <map>

namespace Moses
{

class SearchGraphBuilder
{
  typedef SearchGraph::VertexId VertexId;

  SearchGraph& m_graph;
  typedef std::map<const Hypothesis*, VertexId> HypoMap;
  HypoMap m_hypoMap;
  VertexId m_vertexCount;
  const std::vector<FeatureFunction*>& m_featureFunctions;
public:
  SearchGraphBuilder(SearchGraph& graph) :
      m_graph(graph), m_hypoMap(), m_vertexCount(0),
      m_featureFunctions(FeatureFunction::GetFeatureFunctionsForNow())
  {
    m_graph.m_featureDescriptions.resize(m_featureFunctions.size());
    m_graph.m_featureWeights.resize(m_featureFunctions.size());
    for (size_t i = 0; i < m_featureFunctions.size(); ++i)
    {
      m_graph.m_featureDescriptions[i] = m_featureFunctions[i]->GetScoreProducerDescription();
      m_graph.m_featureWeights[i] = StaticData::Instance().GetWeights(m_featureFunctions[i]);
    }
  }

  VertexId DepthFirstSearch(const Hypothesis* hypo)
  {
    HypoMap::const_iterator hypoMapIter = m_hypoMap.find(hypo);
    if (hypoMapIter != m_hypoMap.end())
      return hypoMapIter->second;
    PreliminaryEdgeList edges;
    if (hypo->GetId() != 0)
    {
      MakePreliminaryEdgeList(hypo, &edges);
    }
    VertexId result = AddVertex(hypo);
    for (size_t i = 0; i < edges.size(); ++i)
    {
      AddEdge(edges[i].first, result, *edges[i].second);
    }
    return result;
  }
private:
  typedef std::vector<std::pair<VertexId, const Hypothesis*> > PreliminaryEdgeList;

  size_t AddVertex(const Hypothesis* hypo)
  {
    VertexId result = m_vertexCount;
    m_hypoMap.insert(HypoMap::value_type(hypo, result));
    ++m_vertexCount;
    m_graph.m_incomingEdges.resize(m_vertexCount);
    m_graph.m_outgoingEdges.resize(m_vertexCount);
    return result;
  }

  void AddEdge(VertexId from, VertexId to, const Hypothesis& hypo)
  {
    size_t edgeId = m_graph.m_allEdges.size();
    m_graph.m_allEdges.push_back(SearchGraph::Edge(from, to, hypo, m_featureFunctions));
    m_graph.m_incomingEdges[to].push_back(edgeId);
    m_graph.m_outgoingEdges[from].push_back(edgeId);
  }

  void MakePreliminaryEdgeList(const Hypothesis* hypo,
      PreliminaryEdgeList* output)
  {
    output->push_back(
        PreliminaryEdgeList::value_type(DepthFirstSearch(hypo->GetPrevHypo()),
            hypo));
    const ArcList* arcList = hypo->GetArcList();
    if (arcList == NULL)
      return;
    for (ArcList::const_iterator it = arcList->begin(); it != arcList->end();
        ++it)
    {
      MakePreliminaryEdgeList(*it, output);
    }
  }
};

SearchGraph::SearchGraph(const Manager& manager)
{
  const HypothesisStack& lastStack = manager.GetLastSearchStack();
  SearchGraphBuilder builder(*this);
  for (HypothesisStack::const_iterator it = lastStack.begin();
      it != lastStack.end(); ++it)
  {
    builder.DepthFirstSearch(*it);
  }
}

namespace
{
class IngoingOutgoingEdgesIter: public SearchGraph::EdgeIterator
{
  typedef SearchGraph::VertexId VertexId;
  const std::vector<SearchGraph::Edge>& m_edgeList;
  std::vector<VertexId>::const_iterator m_current, m_end;
public:
  IngoingOutgoingEdgesIter(const std::vector<SearchGraph::Edge>& edgeList,
      std::vector<VertexId>::const_iterator begin,
      std::vector<VertexId>::const_iterator end) :
        m_edgeList(edgeList), m_current(begin), m_end(end)
  {}

  virtual const SearchGraph::Edge& CurrentEdge() const
  {
    if (!Valid())
      abort();
    return m_edgeList[*m_current];
  }
  virtual void Next()
  {
    if (!Valid())
      abort();
    ++m_current;
  }
  virtual bool Valid() const
  {
    return (m_current != m_end);
  }
};

class AllEdgesIter : public SearchGraph::EdgeIterator
{
  typedef std::vector<SearchGraph::Edge>::const_iterator Iter;
  Iter m_current, m_end;
public:
  AllEdgesIter(const Iter& begin, const Iter& end):
    m_current(begin),
    m_end(end)
  {}

  virtual const SearchGraph::Edge& CurrentEdge() const
  {
    if (!Valid())
      abort();
    return *m_current;
  }
  virtual void Next()
  {
    if (!Valid())
      abort();
    ++m_current;
  }
  virtual bool Valid() const
  {
    return (m_current != m_end);
  }
};

} // anonymous namespace

std::auto_ptr<SearchGraph::EdgeIterator> SearchGraph::GetOutgoingEdgesIter(
  VertexId v) const
{
  return std::auto_ptr<SearchGraph::EdgeIterator>(new IngoingOutgoingEdgesIter(
      m_allEdges,
      m_outgoingEdges[v].begin(),
      m_outgoingEdges[v].end()));
}

std::auto_ptr<SearchGraph::EdgeIterator> SearchGraph::GetIncomingEdgesIter(
  VertexId v) const
{
  return std::auto_ptr<SearchGraph::EdgeIterator>(new IngoingOutgoingEdgesIter(
        m_allEdges,
        m_incomingEdges[v].begin(),
        m_incomingEdges[v].end()));
}

std::auto_ptr<SearchGraph::EdgeIterator> SearchGraph::GetAllEdgesIter() const
{
  return std::auto_ptr<SearchGraph::EdgeIterator>(new AllEdgesIter(
      m_allEdges.begin(), m_allEdges.end()));
}

std::string SearchGraph::FeatureDescription(size_t featureIndex) const
{
  return "";
}

const std::vector<float>& SearchGraph::FeatureWeights(size_t featureIndex) const
{
  return m_featureWeights[featureIndex];
}

SearchGraph::Edge::Edge(VertexId begin, VertexId end,
  const Hypothesis& hypothesis, const std::vector<FeatureFunction*>& allFFs) :
  m_begin(begin), m_end(end),
  m_featureScores(allFFs.size()), m_totalScore(0.0),
  m_translationOption(&hypothesis.GetTranslationOption())
{
  const Hypothesis& prevHypoth = *hypothesis.GetPrevHypo();
  m_totalScore = hypothesis.GetTotalScore() - prevHypoth.GetTotalScore();
  const ScoreComponentCollection& currScores = hypothesis.GetScoreBreakdown();
  const ScoreComponentCollection& prevScores = prevHypoth.GetScoreBreakdown();
  for (size_t i = 0; i < allFFs.size(); ++i) {
    std::vector<float> vec1 = currScores.GetScoresForProducer(allFFs[i]);
    std::vector<float> vec2 = prevScores.GetScoresForProducer(allFFs[i]);
    m_featureScores[i].resize(allFFs[i]->GetNumScoreComponents());
    for (size_t j = 0; j < m_featureScores[i].size(); ++j)
    {
      m_featureScores[i][j] = vec1[j] - vec2[j];
    }
  }
}

static std::string FlattenPhrase(const Phrase& phrase)
{
  size_t size = phrase.GetSize();
  std::ostringstream oss;
  for (size_t i = 0; i < size; ++i)
  {
    const Word& w = phrase.GetWord(i);
    if (i != 0)
      oss << " ";
    oss << w.GetString(0);
  }
  return oss.str();
}

std::string SearchGraph::Edge::GetSourceText() const
{
  const InputPath& path = m_translationOption->GetInputPath();
  return FlattenPhrase(path.GetPhrase());
}

std::string SearchGraph::Edge::GetTargetText() const
{
  return FlattenPhrase(m_translationOption->GetTargetPhrase());
}

} // namespace Moses
