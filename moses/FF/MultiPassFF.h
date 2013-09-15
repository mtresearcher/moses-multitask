#pragma once

#include <string>
#include "StatelessFeatureFunction.h"

namespace Moses
{

class MultiPassFF : public StatelessFeatureFunction
{
public:
	MultiPassFF(const std::string &line)
	:StatelessFeatureFunction("MultiPassFF", line)
	{}

	bool IsUseable(const FactorMask &mask) const
	{ return true; }

	void Evaluate(const Phrase &source
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown
	                        , ScoreComponentCollection &estimatedFutureScore) const
	{}
	void Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , ScoreComponentCollection &scoreBreakdown) const
	{}
	  virtual void Evaluate(const Hypothesis& hypo,
	                        ScoreComponentCollection* accumulator) const
	  {}
	  void EvaluateChart(const ChartHypothesis &hypo,
	                             ScoreComponentCollection* accumulator) const
	  {}

	  void Evaluate(Edge &edge ) const;


};

}

