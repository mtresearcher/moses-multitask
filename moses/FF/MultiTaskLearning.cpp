/*
 * MultiTaskLearning.cpp
 *
 *  Created on: Mar 1, 2015
 *
 */

#include "MultiTaskLearning.h"

namespace Moses {
MultiTaskLearning *MultiTaskLearning::s_instance = NULL;

MultiTaskLearning::MultiTaskLearning(const std::string &line) :
		StatelessFeatureFunction(1, line){
	m_users = 1;
	m_learningrate=0.1;
	m_implementation = noupdate;
	m_multitask=true;
	m_currtask=0;
	ReadParameters();
}
void MultiTaskLearning::SetParameter(const std::string& key, const std::string& value)
{
	VERBOSE(2, "OnlineLearningFeature::SetParameter key:|" << key << "| value:|" << value << "|" << std::endl);
	if (key == "updateStep") {
		m_implementation = Scan<std::string>(value);
	} else if (key == "matrix-lr"){
		m_learningrate = Scan<double>(value);
	} else if (key=="matrix"){
	} else {
		StatelessFeatureFunction::SetParameter(key, value);
	}
}
void EvaluateInIsolation(const Moses::Phrase& sp, const Moses::TargetPhrase& tp,
		Moses::ScoreComponentCollection& out, Moses::ScoreComponentCollection& fs) const  {
	float score=1;
	out.PlusEquals(this, score);
}
void MultiTaskLearning::SetInteractionMatrix(boost::numeric::ublas::matrix<double>& interactionMatrix){
	m_intMatrix = interactionMatrix;
}
void MultiTaskLearning::SetKdKdMatrix(boost::numeric::ublas::matrix<double>& kdkdmatrix){
	m_kdkdmatrix=kdkdmatrix;
}
ScoreComponentCollection MultiTaskLearning::GetWeightsVector(int user) const {
	if(m_user2weightvec.find(user) != m_user2weightvec.end()){
		return m_user2weightvec[user];
	}
	else{
		VERBOSE(1, "Requested weights from a wrong user");
		exit(1);
	}
}
boost::numeric::ublas::matrix<double> MultiTaskLearning::GetWeightsMatrix() {
	boost::numeric::ublas::matrix<double> A(m_user2weightvec.begin()->second.Size(), m_users);
	for(int i=0;i<m_users;i++){
		FVector weightVector = m_user2weightvec[i].GetScoresVector();
		weightVector.printCoreFeatures();
		const std::valarray<float>& scoreVector = m_user2weightvec[i].GetScoresVector().getCoreFeatures();
		for(size_t j=0; j<scoreVector.size(); j++){
			A(j,i) = scoreVector[j];
		}
	}
	return A;
}
void MultiTaskLearning::SetWeightsVector(uint8_t user, ScoreComponentCollection weightVec){
	if(user < m_users){	// < because the indexing starts from 0
		m_user2weightvec[user] = weightVec;
	}
	return;
}
MultiTaskLearning::~MultiTaskLearning() {
}

} /* namespace Moses */
