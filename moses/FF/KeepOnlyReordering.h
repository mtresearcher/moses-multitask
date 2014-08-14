#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{
class PhraseDictionary;

class KeepOnlyReordering : public StatelessFeatureFunction
{
public:
  KeepOnlyReordering(const std::string &line);

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
  void EvaluateWithAllTransOpts(ChartTranslationOptionList &transOptList) const;

  void SetParameter(const std::string& key, const std::string& value);

  std::vector<float> DefaultWeights() const;

protected:
  const PhraseDictionary *m_pt;
  bool m_keepAll;

  bool ContainsReordering(const Phrase &sp, const TargetPhrase &tp) const;

};

}

