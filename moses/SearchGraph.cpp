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
  for (HypothesisStack::iterator it = lastStack.begin();
      it != lastStack.end(); ++it)
  {
	  Hypothesis &hypo = **it;
	  Edge *edge = new Edge(hypo, NULL, m_edges);

  }
}

SearchGraph::~SearchGraph()
{
	RemoveAllInColl(m_edges);
}

void SearchGraph::Search(size_t pass)
{
	const std::vector<const StatelessFeatureFunction*> &slffs = StatelessFeatureFunction::GetStatelessFeatureFunctions(pass);
	std::vector<Edge*>::iterator iter;
	for (iter = m_edges.begin(); iter != m_edges.end(); ++iter) {
		Edge &edge = **iter;

		for (size_t i = 0; i < slffs.size(); ++i) {
			const StatelessFeatureFunction &slff = *slffs[i];
			slff.Evaluate(edge);
		}
	}
}

SearchGraph::Edge::Edge(Hypothesis &hypo, const Edge* next, std::vector<Edge*> &edges)
:m_hypo(hypo)
,m_next(next)
{
	edges.push_back(this);

	Hypothesis *prevHypo = const_cast<Hypothesis*>(hypo.GetPrevHypo());
	  if (prevHypo) {
		  Edge *edge = new Edge(*prevHypo, this, edges);
	  }
}

SearchGraph::Edge::~Edge()
{
}

} // namespace Moses

