#pragma once

#include <string>
#include "StatelessFeatureFunction.h"
#include "moses/NonTerminal.h"

namespace Moses
{
// penalize rule if using X when syntax label is available for input range.
class SyntaxRHS : public StatelessFeatureFunction
{
public:
  SyntaxRHS(const std::string &line);

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
  std::vector<float> DefaultWeights() const;

protected:
  bool m_hardConstraint;

  bool IsValid(const Word &ruleNT, const NonTerminalSet &labels) const;
  bool IsGlueRule(const TargetPhrase &targetPhrase) const;

};

}

