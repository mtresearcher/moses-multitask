#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{
class ChartCellLabel;
class NonTermContextProperty;

class NonTermContext : public StatelessFeatureFunction
{
public:
  NonTermContext(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void EvaluateInIsolation(const Phrase &source
                , const TargetPhrase &targetPhrase
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection &estimatedFutureScore) const;
  void EvaluateWithSourceContext(const InputType &input
                , const InputPath &inputPath
                , const TargetPhrase &targetPhrase
                , const StackVec *stackVec
                , ScoreComponentCollection &scoreBreakdown
                , ScoreComponentCollection *estimatedFutureScore = NULL) const;
  void EvaluateWhenApplied(const Hypothesis& hypo,
                ScoreComponentCollection* accumulator) const;
  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                     ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);

protected:
  float m_smoothConst;
  FactorType m_factor;

  void SetScores(size_t ntInd, const InputType &input,
		  	  const NonTermContextProperty &ntContextProp,
		  	  const ChartCellLabel &cell,
		  	  const TargetPhrase &targetPhrase,
		  	  ScoreComponentCollection &scoreBreakdown) const;

};

}

