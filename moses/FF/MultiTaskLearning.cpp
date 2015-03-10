/*
 * MultiTaskLearning.cpp
 *
 *  Created on: Mar 1, 2015
 *
 */
#pragma once
#include "MultiTaskLearning.h"
#include "moses/StaticData.h"

namespace Moses {
MultiTaskLearning *MultiTaskLearning::s_instance = NULL;

MultiTaskLearning::MultiTaskLearning(const std::string &line)
		: StatelessFeatureFunction(1, line){
	m_users = 1;
	m_learningrate=0.1;
	m_implementation = noupdate;
	m_multitask=true;
	m_currtask=0;
	s_instance=this;
	ReadParameters();
}
void MultiTaskLearning::chop(string &str) {
        int i = 0;
        while (isspace(str[i]) != 0) {
                str.replace(i, 1, "");
        }
        while (isspace(str[str.length() - 1]) != 0) {
                str.replace(str.length() - 1, 1, "");
        }
        return;
}

int MultiTaskLearning::split_marker_perl(std::string& str, std::string marker,
                std::vector<std::string> &array){
        int found = str.find(marker), prev = 0;
        while (found != string::npos) // warning!
        {
                array.push_back(str.substr(prev, found - prev));
                prev = found + marker.length();
                found = str.find(marker, found + marker.length());
        }
        array.push_back(str.substr(prev));
        return array.size();
}

void MultiTaskLearning::ReadMatrix(std::string filename){
    ifstream file;
    file.open(filename.c_str(), ios::in);
    std::string line;
    if(file.is_open())
    {
    	getline(file, line);
    	std::vector<string> splits;
    	int s = split_marker_perl(line, " ", splits);
    	if(s==1){
    		int users;
    		stringstream(line)>>users;
    		m_users=users;
    		m_intMatrix.resize(users, users);
    		for(int i=0; i< users; i++){
    			getline(file, line);
    			splits.clear();
    			s=split_marker_perl(line, " ", splits);
    			if(s==users)
    				for(int j=0; j< users; j++){
    					stringstream(splits[j]) >> m_intMatrix(i, j);
    				}
    		}
    	}
    }
    std::cerr << "Interaction Matrix = " << m_intMatrix <<endl;
}
void MultiTaskLearning::SetParameter(const std::string& key, const std::string& value)
{
//	VERBOSE(2, "OnlineLearningFeature::SetParameter key:|" << key << "| value:|" << value << "|" << std::endl);
	if(key == "updateStep") {
	    std::string val = Scan<std::string>(value);
	    if(val.compare("noupdate")==0){m_implementation = noupdate;}
	    if(val.compare("vonNeumann")==0){ m_implementation = vonNeumann; }
	    if(val.compare("logDet")==0){ m_implementation = logDet; }
	    if(val.compare("Burg")==0){ m_implementation = Burg; }
	} else if (key == "matrix-lr"){
	    m_learningrate = Scan<double>(value);
	} else if (key == "matrix"){
	    std::string matrix_file = Scan<std::string>(value);
	    ReadMatrix(matrix_file);
	} else {
	    StatelessFeatureFunction::SetParameter(key, value);
	}
}
void MultiTaskLearning::EvaluateInIsolation(const Moses::Phrase& sp, const Moses::TargetPhrase& tp,
		Moses::ScoreComponentCollection& out, Moses::ScoreComponentCollection& fs) const  {
	float score=1;
	out.Assign(this, score);
}
ScoreComponentCollection MultiTaskLearning::GetWeightsVector(uint8_t user) {
	if(m_user2weightvec.find(user) != m_user2weightvec.end()){
		return m_user2weightvec[user];
	}
	else{
		VERBOSE(1, "Requested weights from a wrong user");
		exit(1);
	}
}
Eigen::MatrixXd MultiTaskLearning::GetWeightsMatrix() {
	Eigen::MatrixXd A(m_user2weightvec.begin()->second.Size(), m_users);
	for(int i=0;i<m_users;i++){ // K columns - tasks
		FVector weightVector = m_user2weightvec[i].GetScoresVector();
		const std::valarray<float>& scoreVector = m_user2weightvec[i].GetScoresVector().getCoreFeatures();
		for(size_t j=0; j<scoreVector.size(); j++){ // d rows - features
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

void MultiTaskLearning::updateIntMatrix(){
	Eigen::MatrixXd W = GetWeightsMatrix();
	std::cerr << "\n\nWeight Matrix \n";
	std::cerr << W << endl;
	Eigen::MatrixXd updated = m_intMatrix;
	Eigen::MatrixXd ones = Eigen::MatrixXd::Ones(m_users, m_users);
	
	if(m_implementation == noupdate){ return; }
	else if(m_implementation == vonNeumann){
		// (A^{-1}_t) = exp(log (A^{-1}_{t-1}) - \frac{\eta} * (W^T_{t-1} \times W_{t-1} + W_{t-1} \times W^T_{t-1}))
		Eigen::MatrixXd temp = W.transpose() * W;
		temp-=ones;
		std::cerr << "W + W^T Matrix \n"<< temp << endl;
		Eigen::MatrixXd sub = (temp + temp.transpose())/2;
		Eigen::MatrixXd A = updated.log();
		std::cerr << "Sub Matrix \n"<< sub << endl;
		A -= m_learningrate * sub;
		updated = A.exp();
		SetInteractionMatrix(updated);
		std::cerr << "Interaction Matrix \n"<< updated << endl;
	}

	else if(m_implementation == logDet){
		// A^{-1}_t = A^{-1}_{t-1} + \frac{\eta}{2} * (W^T_{t-1} \times W_{t-1} + W_{t-1} \times W^T_{t-1})
		Eigen::MatrixXd temp = W.transpose() * W;
		temp-=ones;
		std::cerr << "W + W^T Matrix \n"<< temp << endl;
		Eigen::MatrixXd adding = (temp + temp.transpose())/2;
		std::cerr << "Add Matrix \n"<< adding << endl;
		updated += m_learningrate * adding;
		m_intMatrix = updated;
		std::cerr << "Interaction Matrix \n"<< updated << endl;
	}

	else if(m_implementation == Burg){

	}
	// update kd x kd matrix because it is used in weight update not the interaction matrix
	int size = StaticData::Instance().GetAllWeights().Size();
	uint8_t tasks= GetNumberOfTasks();
	Eigen::MatrixXd kdkdmatrix(tasks*size, tasks*size);
	Eigen::MatrixXd m = Eigen::MatrixXd::Identity(size, size);
	kdkdmatrix = Eigen::kroneckerProduct(m_intMatrix, m).eval();
	m_kdkdmatrix = kdkdmatrix;
}

void MultiTaskLearning::l1(float lambda){

}

void MultiTaskLearning::l2(float lambda){

}

void MultiTaskLearning::l1X(float lambda){

}

} /* namespace Moses */
