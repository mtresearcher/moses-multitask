/*
 * MultiTaskLearning.h
 *
 *  Created on: Mar 1, 2015
 *      Author: prashant
 */

#ifndef MULTITASKLEARNING_H_
#define MULTITASKLEARNING_H_

#include "ScoreComponentCollection.h"
#include "Util.h"
#include <map>
#include <vector>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/lu.hpp>

using namespace std;

namespace Moses {
// this is a stateless function because we need to add an additional bias feature
class MultiTaskLearning : public StatelessFeatureFunction {

	enum UpdateInteractionMatrixType {
		noupdate = 0,
		vonNeumann = 1,
		logDet = 2,
		burg = 3
	};

	int m_users;
	map<int, ScoreComponentCollection> m_user2weightvec;
	int m_currtask;
	float m_learningrate;
	bool m_learnmatrix;
	UpdateInteractionMatrixType m_implementation;
	boost::numeric::ublas::matrix<double> m_kdkdmatrix;
	boost::numeric::ublas::matrix<double> m_intMatrix;

public:
	const int GetNumberOfTasks() const {return m_users;};
	void SetCurrentTask(int taskid){m_currtask=taskid;};
	int GetCurrentTask() const {return m_currtask;};
	void SetInteractionMatrix(boost::numeric::ublas::matrix<double>& interactionMatrix);
	boost::numeric::ublas::matrix<double>& GetInteractionMatrix(){return m_intMatrix;};
	void SetKdKdMatrix(boost::numeric::ublas::matrix<double>&);
	boost::numeric::ublas::matrix<double>& GetKdKdMatrix(){return m_kdkdmatrix;};
	ScoreComponentCollection GetWeightsVector(int);
	void SetWeightsVector(int, ScoreComponentCollection);
	MultiTaskLearning(const std::string &line);
	inline std::string GetScoreProducerWeightShortName(unsigned) const { return "mtl"; };
	virtual ~MultiTaskLearning();

	bool IfUpdateIntMatrix(){return m_learnmatrix;};
	float GetLearningRateIntMatrix(){return m_learningrate;};
	boost::numeric::ublas::matrix<double> GetWeightsMatrix();

	virtual void EvaluateInIsolation(const Moses::Phrase&, const Moses::TargetPhrase&,
			Moses::ScoreComponentCollection&, Moses::ScoreComponentCollection&) const{}
	void EvaluateWithSourceContext(const InputType &input
			, const InputPath &inputPath
			, const TargetPhrase &targetPhrase
			, const StackVec *stackVec
			, ScoreComponentCollection &scoreBreakdown
			, ScoreComponentCollection *estimatedFutureScore = NULL) const ;
	void EvaluateTranslationOptionListWithSourceContext(const Moses::InputType&,
			const Moses::TranslationOptionList&) const {}
	void EvaluateWhenApplied(const Hypothesis& hypo,
			ScoreComponentCollection* accumulator) const {}
	void EvaluateWhenApplied(const ChartHypothesis &hypo,
			ScoreComponentCollection* accumulator) const {assert(false);}

	void EvaluateWhenApplied(const Syntax::SHyperedge &,
			ScoreComponentCollection*) const { assert(false); }

};

} /* namespace Moses */

#endif /* MULTITASKLEARNING_H_ */
