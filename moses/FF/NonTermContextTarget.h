#pragma once

#include <string>
#include "StatefulFeatureFunction.h"
#include "FFState.h"
#include "moses/Word.h"

namespace Moses
{
class ChartCellLabel;
class NonTermContextTargetProperty;

class NonTermContextTargetState : public FFState
{
public:
  NonTermContextTargetState()
  {}

  int Compare(const FFState& other) const;

  const std::vector<const Factor*> &GetWords() const
  { return m_leftRight; }

protected:
  std::vector<const Factor*> m_leftRight;
};

//////////////////////////////////////////////////////////////////

class NonTermContextTarget : public StatefulFeatureFunction
{
public:
  NonTermContextTarget(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void Evaluate(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const;
  void Evaluate(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;

  FFState* Evaluate(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;

  FFState* EvaluateChart(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  //! return the state associated with the empty hypothesis for a given sentence
  const FFState* EmptyHypothesisState(const InputType &input) const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  float m_smoothConst;

  void SetScores(size_t ntInd,
			const NonTermContextTargetProperty &ntContextProp,
			const std::vector<const Factor*> &ntContext,
			ScoreComponentCollection &scoreBreakdown) const;

};

}

