#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{
class ChartCellLabel;

class NonTermContext : public StatelessFeatureFunction
{
public:
  NonTermContext(const std::string &line);

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
  void Evaluate(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const;
  void EvaluateChart(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  float m_const;

  void SetScores(size_t ntInd, ScoreComponentCollection &scoreBreakdown,
			const ChartCellLabel &cell,
			const TargetPhrase &targetPhrase) const;

};

}

