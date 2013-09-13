


#ifndef moses_NBESTCSLM_H_
#define moses_NBESTCSLM_H_

#include "moses/TrellisPath.h"
//#include "HypothesisStack"
#include "moses/StaticData.h"
#include <boost/thread.hpp>
#include "SingleFactor.h"

class Vocab;
class Ngram;
class TrainerNgramSlist;
class Mach;

namespace Moses
{


class HypothesisStack;
class Hypothesis;

class LanguageModelCSLM : public LanguageModelSingleFactor
{

protected:
  std::string	m_srilmVocab_filePath;
  std::string	m_srilmNgram_filePath;
  Vocab *m_srilmVocab;
  Mach *m_mach;
  TrainerNgramSlist *m_trainer;
  size_t nGramOrder;
  static const int max_words=16384;
  boost::mutex cslm_mutex;
  boost::condition_variable cond;
  bool busy;
  size_t Id;
public:
  LanguageModelCSLM(const std::string &line);
  virtual ~LanguageModelCSLM();
  void Load();  // Load CSLM Language Model
  void SetInitialStatus() {busy =false;}
  void TakeMach(){ busy =true; }
  void SetId(size_t m) {Id = m ;}

  // Recoring functions
  void RescoreLattice( std::vector < HypothesisStack* >& hypoStackColl);
  void GetCSLMScore( Hypothesis* hypo); // Request probability of hypo to CSLM (asynchonous-> the result will not be available directly)
  void FinishPending(); // Ask the CSLM to compute and give the requested probabilities

  //Convert CSLM Score to natural log
  float lognat2ln(float prob) {  return prob * 2.30258509299405f ; }

  void SetParameter(const std::string& key, const std::string& value);

   // Gets
   size_t GetId() {return Id; }
   Vocab * GetVocab(){ return m_srilmVocab; }
   Mach *  GetMach() { return m_mach; }
   TrainerNgramSlist * GetTrainer() { return m_trainer; }

   virtual LMResult GetValue(const std::vector<const Word*> &contextFactor, State* finalState = NULL) const;


};


}

#endif
