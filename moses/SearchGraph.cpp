#include "SearchGraph.h"
#include "moses/Manager.h"
#include "moses/Hypothesis.h"
#include "moses/FF/FeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"

#include <map>
#include <numeric>

using namespace std;

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
	CollEdge::iterator iter;
	for (iter = m_edges.begin(); iter != m_edges.end(); ++iter) {
		Edge &edge = **iter;

		for (size_t i = 0; i < slffs.size(); ++i) {
			const StatelessFeatureFunction &slff = *slffs[i];
			slff.Evaluate(edge);
		}
	}
}

Edge::Edge(Hypothesis &hypo, const Edge* next, CollEdge &edges)
:m_hypo(hypo)
,m_next(next)
{
  pair<CollEdge::iterator,bool> ret = edges.insert(this);
  if (!ret.second) {
	 delete this;
	 return;
  }

	Hypothesis *prevHypo = const_cast<Hypothesis*>(hypo.GetPrevHypo());
	if (prevHypo) {
		  Edge *edge = new Edge(*prevHypo, this, edges);
	}
}

Edge::~Edge()
{
}

std::ostream& operator<<(std::ostream &out, const Edge &obj)
{
	out << "m_hypo=" << &obj.m_hypo;
	return out;
}

} // namespace Moses

