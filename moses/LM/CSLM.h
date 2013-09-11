


#ifndef moses_NBESTCSLM_H_
#define moses_NBESTCSLM_H_

#include "TrellisPath.h"
//#include "HypothesisStack"
#include "Mach.h" // from cslm toolkit
#include "TrainerNgramSlist.h"
#include "StaticData.h"
#include <boost/thread.hpp>

class Vocab;
class Ngram;

namespace Moses
{


class HypothesisStack;
class Hypothesis;
// class TrainerNgramSlist;
// class Mach;

class LanguageModelCSLM
{

protected:
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
  LanguageModelCSLM();
  virtual ~LanguageModelCSLM();
  bool Load(char * CSLM_filepath, char * Wordlist_filepath, char * backofflm_filepath);  // Load CSLM Language Model
  void SetInitialStatus() {busy =false;}
  void TakeMach(){ busy =true; }
  void SetId(size_t m) {Id = m ;}

  //virtual void RescorePath(TrellisPath *path ); // recalc LM score on hypothesis, returns ln probability
  //virtual void RescoreNbest(TrellisPathList &nBest );


  void RescoreLAT( std::vector < HypothesisStack* >& hypoStackColl);
  void RescoreLATV1( std::vector < HypothesisStack* >& hypoStackColl);
  void GetCSLMScore( Hypothesis* hypo);
  void GetCSLMScoreOpt( Hypothesis* hypo);
  void FinishPending();
  float UpdatePathTotalScore(TrellisPath * path );

  //Convert CSLM Score to natural log
  float lognat2ln(float prob) {  return prob * 2.30258509299405f ; }


   // Gets
   size_t GetId() {return Id; }
   Vocab * GetVocab(){ return m_srilmVocab; }
   Mach *  GetMach() { return m_mach; }
   TrainerNgramSlist * GetTrainer() { return m_trainer; }



};


}

#endif

