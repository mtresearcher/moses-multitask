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
void MultiTaskLearning::SetInteractionMatrix(boost::numeric::ublas::matrix<double>& interactionMatrix){
	m_intMatrix = interactionMatrix;
}
void MultiTaskLearning::SetKdKdMatrix(boost::numeric::ublas::matrix<double>& kdkdmatrix){
	m_kdkdmatrix=kdkdmatrix;
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

void MultiTaskLearning::updateIntMatrix(){
	boost::numeric::ublas::matrix<double> W = MultiTaskLearning::InstanceNonConst().GetWeightsMatrix();
	std::cerr << "\n\nWeight Matrix = ";
	std::cerr << W << endl;
	boost::numeric::ublas::matrix<double> updated = MultiTaskLearning::Instance().GetInteractionMatrix();

	if(m_implementation == MultiTaskLearning::vonNeumann){
		// log (A^{-1}_t) = log (A^{-1}_{t-1}) - \frac{\eta} * (W^T_{t-1} \times W_{t-1} + W_{t-1} \times W^T_{t-1})
		float eta = MultiTaskLearning::Instance().GetLearningRateIntMatrix();
		boost::numeric::ublas::matrix<double> sub = prod(trans(W), W) + trans(prod(trans(W), W)) ;
		std::transform(updated.data().begin(), updated.data().end(), updated.data().begin(), ::log);
		updated -= eta * sub;
		std::transform(updated.data().begin(), updated.data().end(), updated.data().begin(), ::exp);
		MultiTaskLearning::InstanceNonConst().SetInteractionMatrix(updated);
		std::cerr << "Updated = ";
		std::cerr << updated << endl;
	}

	if(m_implementation == MultiTaskLearning::logDet){
		// A^{-1}_t = A^{-1}_{t-1} + \frac{\eta}{2} * (W^T_{t-1} \times W_{t-1} + W_{t-1} \times W^T_{t-1})
		float eta = MultiTaskLearning::Instance().GetLearningRateIntMatrix();
		boost::numeric::ublas::matrix<double> adding = prod(trans(W), W) + trans(prod(trans(W), W)) ;
		updated += eta * adding;
		MultiTaskLearning::InstanceNonConst().SetInteractionMatrix(updated);
		std::cerr << "Updated = ";
		std::cerr << updated << endl;

	}

	if(m_implementation == MultiTaskLearning::Burg){

	}
	// update kd x kd matrix because it is used in weight update not the interaction matrix
	int size = StaticData::Instance().GetAllWeights().Size();
	uint8_t tasks= MultiTaskLearning::Instance().GetNumberOfTasks();
	boost::numeric::ublas::matrix<double> kdkdmatrix (tasks*size, tasks*size);
	boost::numeric::ublas::identity_matrix<double> m (size);
	KroneckerProduct(updated, m, kdkdmatrix);
	MultiTaskLearning::InstanceNonConst().SetKdKdMatrix(kdkdmatrix);
}

//    ------------------- Kronecker Product code ---------------------------- //
bool MultiTaskLearning::KroneckerProduct (const boost::numeric::ublas::matrix<double>& A,
		const boost::numeric::ublas::matrix<double>& B, boost::numeric::ublas::matrix<double>& C) {
	size_t rowA=-1,colA=-1,rowB=0,colB=0,prowB=1,pcolB=1;
	for(size_t i=0; i<C.size1(); i++){
		for(size_t j=0; j<C.size2(); j++){
			rowB=i%B.size1();
			colB=j%B.size2();
			if(pcolB!=0 && colB == 0) colA++;
			if(prowB!=0 && rowB==0) rowA++;
			prowB=rowB;
			pcolB=colB;
			if(colA >= A.size2()){colA=0; colB=0;pcolB=1;}
			C(i, j) = A(rowA, colA) * B(rowB, colB) ;
		}
	}
	return true;
}
//    ------------------- matrix inversion code ---------------------------- //
template<class T>
bool MultiTaskLearning::InvertMatrix (const boost::numeric::ublas::matrix<T>& input, boost::numeric::ublas::matrix<T>& inverse) {
	typedef boost::numeric::ublas::permutation_matrix<std::size_t> pmatrix;

	// create a working copy of the input
	boost::numeric::ublas::matrix<T> A(input);

	// create a permutation matrix for the LU-factorization
	pmatrix pm(A.size1());

	// perform LU-factorization
	size_t res = boost::numeric::ublas::lu_factorize(A, pm);
	if (res != 0)
		return false;

	// create identity matrix of "inverse"
	inverse.assign(boost::numeric::ublas::identity_matrix<T> (A.size1()));

	// backsubstitute to get the inverse
	boost::numeric::ublas::lu_substitute(A, pm, inverse);

	return true;
}


} /* namespace Moses */
