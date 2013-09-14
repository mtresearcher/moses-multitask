#include "SearchGraph.h"
#include "moses/Manager.h"
#include "moses/Hypothesis.h"
#include "moses/FF/FeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"

#include <map>
#include <numeric>

namespace Moses
{


SearchGraph::SearchGraph(const Manager& manager)
{
  const HypothesisStack& lastStack = manager.GetLastSearchStack();
  for (HypothesisStack::const_iterator it = lastStack.begin();
      it != lastStack.end(); ++it)
  {

  }
}

void SearchGraph::Search(size_t pass)
{

}

SearchGraph::Edge::Edge()
{}

SearchGraph::Edge::Edge(const Edge& other)
{}

SearchGraph::Edge::~Edge()
{
}

} // namespace Moses

