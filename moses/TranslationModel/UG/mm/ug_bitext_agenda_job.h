// -*- c++ -*-
// class declaration of template<typename Token> class Bitxt<Token>::agenda::job
// to be included by ug_bitext.h
// todo: add check to enforce this

template<typename Token>
class 
Bitext<Token>::agenda::
job 
{
#if UG_BITEXT_TRACK_ACTIVE_THREADS
  static ThreadSafeCounter active;
#endif
  Bitext<Token> const* const m_bitext;
  boost::mutex lock; 
  friend class agenda;
  boost::taus88 rnd;  // every job has its own pseudo random generator 
  double rnddenom;    // denominator for scaling random sampling
  size_t min_diverse; // minimum number of distinct translations

  bool flip_coin(uint64_t & sid, uint64_t & offset); 
  bool step(uint64_t & sid, uint64_t & offset); // proceed to next occurrence

public:
  size_t         workers; // how many workers are working on this job?
  sptr<TSA<Token> const> root; // root of the underlying suffix array
  char const*       next; // next position to read from 
  char const*       stop; // end of index range
  size_t     max_samples; // how many samples to extract at most
  size_t             ctr; /* # of phrase occurrences considered so far
			   * # of samples chosen is stored in stats->good 
			   */
  size_t             len; // phrase length
  bool               fwd; // if true, source phrase is L1 
  sptr<pstats>     stats; // stores statistics collected during sampling
  sptr<SamplingBias const> const m_bias; // sentence-level bias for sampling
  float bias_total;
  bool nextSample(uint64_t & sid, uint64_t & offset); // select next occurrence
  
  int 
  check_sample_distribution(uint64_t const& sid, uint64_t const& offset);
  // for biased sampling: ensure the distribution approximately matches 
  // the bias
  
  bool done() const;
  job(Bitext<Token> const* const theBitext, 
      typename TSA<Token>::tree_iterator const& m, 
      sptr<TSA<Token> > const& r, size_t maxsmpl, bool isfwd, 
      sptr<SamplingBias const> const& bias);
  ~job();
};

template<typename Token>
Bitext<Token>::agenda::job
::~job()
{
  if (stats) stats.reset();
#if UG_BITEXT_TRACK_ACTIVE_THREADS
  // counter may not exist any more at destruction time, hence try .. catch ...
  try { --active; } catch (...) {} 
#endif
}

template<typename Token>
Bitext<Token>::agenda::job
::job(Bitext<Token> const* const theBitext,
      typename TSA<Token>::tree_iterator const& m, 
      sptr<TSA<Token> > const& r, size_t maxsmpl, 
      bool isfwd, sptr<SamplingBias const> const& bias)
  : m_bitext(theBitext)
  , rnd(0)
  , rnddenom(rnd.max() + 1.)
  , min_diverse(1)
  , workers(0)
  , root(r)
  , next(m.lower_bound(-1))
  , stop(m.upper_bound(-1))
  , max_samples(maxsmpl)
  , ctr(0)
  , len(m.size())
  , fwd(isfwd)
  , m_bias(bias)
{
  stats.reset(new pstats());
  stats->raw_cnt = m.approxOccurrenceCount();
  bias_total = 0; 
  
  // we need to renormalize on the fly, as the summ of all sentence probs over 
  // all candidates (not all sentences in the corpus) needs to add to 1.
  // Profiling question: how much does that cost us?
  if (m_bias)
    {
      // int ctr = 0;
      stats->raw_cnt = 0;
      for (char const* x = m.lower_bound(-1); x < stop;)
	{
	  uint32_t sid; ushort offset;
	  x = root->readSid(x,stop,sid);
	  x = root->readOffset(x,stop,offset);
#if 0
	  cerr << ctr++ << " " << m.str(m_bitext->V1.get()) 
	       << " " << sid << "/" << root->getCorpusSize() 
	       << " " << offset << " " << stop-x << endl;
#endif
	  bias_total += (*m_bias)[sid];
	  ++stats->raw_cnt;
	}
    }
#if UG_BITEXT_TRACK_ACTIVE_THREADS
  ++active;
  // if (active%5 == 0) 
  // cerr << size_t(active) << " active jobs at " << __FILE__ << ":" << __LINE__ << endl;
#endif
}

template<typename Token>
bool Bitext<Token>::agenda::job
::done() const
{ 
  return (max_samples && stats->good >= max_samples) || next == stop; 
}

template<typename Token>
int Bitext<Token>::agenda::job
::check_sample_distribution(uint64_t const& sid, uint64_t const& offset)
{ // ensure that the sampled distribution approximately matches the bias
  // @return 0: SKIP this occurrence
  // @return 1: consider this occurrence for sampling 
  // @return 2: include this occurrence in the sample by all means

  if (!m_bias) return 1;
  
  using namespace boost::math;
  typedef boost::math::binomial_distribution<> binomial;
  
  ostream* log = m_bias->loglevel > 1 ? m_bias->log : NULL;
  
  float p = (*m_bias)[sid]; 
  id_type docid = m_bias->GetClass(sid); 
  uint32_t k = docid < stats->indoc.size() ? stats->indoc[docid] : 0; 

  // always consider candidates from dominating documents and
  // from documents that have not been considered at all yet
  bool ret =  (p > .5 || k == 0);
  
  if (ret && !log) return 1;
  
  uint32_t N = stats->good; // number of trials
  float d = cdf(complement(binomial(N, p), k)); 
  // d: probability that samples contains k or more instances from doc #docid
  ret = ret || d >= .05; 
  
  if (log)
    {
      Token const* t = root->getCorpus()->sntStart(sid)+offset;
      Token const* x = t - min(offset,uint64_t(3));
      Token const* e = t+4; 
      if (e > root->getCorpus()->sntEnd(sid))
	e = root->getCorpus()->sntEnd(sid);
      *log << docid << ":" << sid << " " << size_t(k) << "/" << N 
	   << " @" << p << " => " << d << " [";
      for (size_t i = 0; i < stats->indoc.size(); ++i)
	{
	  if (i) *log << " ";
	  *log << stats->indoc[i];
	}
      *log << "] ";
      for (; x < e; ++x) *log << (*m_bitext->V1)[x->id()] << " ";
      if (!ret) *log << "SKIP";
      else if (p < .5 && d > .9) *log << "FORCE";
      *log << endl;
    }
  
  return (ret ? (p < .5 && d > .9) ? 2 : 1 : 0); 
}

template<typename Token>
bool Bitext<Token>::agenda::job
::flip_coin(uint64_t & sid, uint64_t & offset)
{
  int no_maybe_yes = m_bias ? check_sample_distribution(sid, offset) : 1;
  if (no_maybe_yes == 0) return false; // no
  if (no_maybe_yes > 1)  return true;  // yes
  // ... maybe: flip a coin
  size_t options_chosen = stats->good;
  size_t options_total  = max(stats->raw_cnt, this->ctr);
  size_t options_left   = (options_total - this->ctr);
  size_t random_number  = options_left * (rnd()/(rnd.max()+1.));
  size_t threshold; 
  if (bias_total) // we have a bias and there are candidates with non-zero prob
    threshold = ((*m_bias)[sid]/bias_total * options_total * max_samples);
  else // no bias, or all have prob 0 (can happen with a very opinionated bias)
    threshold = max_samples;
  return random_number + options_chosen < threshold;
}

template<typename Token>
bool Bitext<Token>::agenda::job
::step(uint64_t & sid, uint64_t & offset)
{ // caller must lock!
  if (next == stop) return false;
  UTIL_THROW_IF2 
    ( next > stop, "Fatal error at " << HERE << ". How did that happen?" );
  // boost::lock_guard<boost::mutex> jguard(lock); // caller must lock!
  next = root->readSid(next, stop, sid);
  next = root->readOffset(next, stop, offset);
  ++ctr;
  return true;
}

template<typename Token>
bool Bitext<Token>::agenda::job
::nextSample(uint64_t & sid, uint64_t & offset)
{
  boost::lock_guard<boost::mutex> jguard(lock);
  if (max_samples == 0) // no sampling, consider all occurrences
    return step(sid, offset); 

  while (step(sid,offset)) 
    {
      size_t good      = stats->good;
      size_t diversity = stats->trg.size();
      if (good >= max_samples && diversity >= min_diverse) 
	return false; // done

      // flip_coin softly enforces approximation of the sampling to the 
      // bias (occurrences that would steer the sample too far from the bias
      // are ruled out), and flips a biased coin otherwise.
      if (!flip_coin(sid,offset)) continue;
      return true;
    } 
  return false;
}

#if UG_BITEXT_TRACK_ACTIVE_THREADS
template<typename TKN>
ThreadSafeCounter Bitext<TKN>::agenda
::job
::active;
#endif
