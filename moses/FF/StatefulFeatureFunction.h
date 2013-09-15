#pragma once

#include "FeatureFunction.h"
#include "util/check.hh"
#include "moses/SearchGraph.h"

namespace Moses
{
class FFState;

/** base class for all stateful feature functions.
 * eg. LM, distortion penalty
 */
class StatefulFeatureFunction: public FeatureFunction
{
  //All statefull FFs
  static std::vector<const StatefulFeatureFunction*> m_statefulFFs;

public:
  static std::vector<std::vector<const StatefulFeatureFunction*> > m_passes;

  static const std::vector<const StatefulFeatureFunction*>& GetStatefulFeatureFunctions(size_t pass) {
	CHECK(pass < m_passes.size());
	return m_passes[pass];
  }
  static const std::vector<const StatefulFeatureFunction*>& GetAllStatefulFF() {
    return m_statefulFFs;
  }
  static size_t GetAllStatefulFFSize() {
    return m_statefulFFs.size();
  }

  StatefulFeatureFunction(const std::string& description, const std::string &line);
  StatefulFeatureFunction(const std::string& description, size_t numScoreComponents, const std::string &line);

  using FeatureFunction::Evaluate;
  /**
   * \brief This interface should be implemented.
   * Notes: When evaluating the value of this feature function, you should avoid
   * calling hypo.GetPrevHypo().  If you need something from the "previous"
   * hypothesis, you should store it in an FFState object which will be passed
   * in as prev_state.  If you don't do this, you will get in trouble.
   */
  virtual FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const = 0;

  virtual FFState* EvaluateChart(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const = 0;

  //! return the state associated with the empty hypothesis for a given sentence
  virtual const FFState* EmptyHypothesisState(const InputType &input) const = 0;

  bool IsStateless() const {
    return false;
  }

  // multipass
  virtual FFState* Evaluate(SearchGraph::Edge &edge ) const
  {}
};


}



