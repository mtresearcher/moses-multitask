#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function, unicode_literals

import logging
import argparse
import subprocess
import sys
import os
import codecs
import copy

# ../bilingual-lm
sys.path.append(os.path.join(os.path.dirname(sys.path[0]), 'bilingual-lm'))
import train_nplm
import extract_vocab
import extract_syntactic_ngrams

logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s', datefmt='%Y-%m-%d %H:%M:%S', level=logging.DEBUG)
parser = argparse.ArgumentParser()
parser.add_argument("--working-dir", dest="working_dir", metavar="PATH")
parser.add_argument("--corpus", dest="corpus_stem", metavar="PATH", help="input file")
parser.add_argument("--nplm-home", dest="nplm_home", metavar="PATH", help="location of NPLM", required=True)
parser.add_argument("--epochs", dest="epochs", type=int, metavar="INT", help="number of training epochs (default: %(default)s)")
parser.add_argument("--up-context-size", dest="up_context_size", type=int, metavar="INT", help="size of ancestor context (default: %(default)s)")
parser.add_argument("--left-context-size", dest="left_context_size", type=int, metavar="INT", help="size of sibling context (left) (default: %(default)s)")
parser.add_argument("--right-context-size", dest="right_context_size", type=int, metavar="INT", help="size of sibling context (right) (default: %(default)s)")
parser.add_argument("--mode", dest="mode", choices=['head', 'label'], help="type of RDLM to train (both are required for decoding)", required=True)
parser.add_argument("--minibatch-size", dest="minibatch_size", type=int, metavar="INT", help="minibatch size (default: %(default)s)")
parser.add_argument("--noise", dest="noise", type=int, metavar="INT", help="number of noise samples for NCE (default: %(default)s)")
parser.add_argument("--hidden", dest="hidden", type=int, metavar="INT", help="size of hidden layer (0 for single hidden layer) (default: %(default)s)")
parser.add_argument("--input-embedding", dest="input_embedding", type=int, metavar="INT", help="size of input embedding layer (default: %(default)s)")
parser.add_argument("--output-embedding", dest="output_embedding", type=int, metavar="INT", help="size of output embedding layer (default: %(default)s)")
parser.add_argument("--threads", "-t", dest="threads", type=int, metavar="INT", help="number of threads (default: %(default)s)")
parser.add_argument("--output-model", dest="output_model", metavar="PATH", help="name of output model (default: %(default)s)")
parser.add_argument("--output-dir", dest="output_dir", metavar="PATH", help="output directory (default: same as working-dir)")
parser.add_argument("--config-options-file", dest="config_options_file", metavar="PATH")
parser.add_argument("--log-file", dest="log_file", metavar="PATH", help="log file to write to (default: %(default)s)")
parser.add_argument("--validation-corpus", dest="validation_corpus", metavar="PATH", help="validation file (default: %(default)s)")
parser.add_argument("--activation-function", dest="activation_fn", choices=['identity', 'rectifier', 'tanh', 'hardtanh'], help="activation function (default: %(default)s)")
parser.add_argument("--learning-rate", dest="learning_rate", type=float, metavar="FLOAT", help="learning rate (default: %(default)s)")
parser.add_argument("--input-words-file", dest="input_words_file", metavar="PATH", help="input vocabulary (default: %(default)s)")
parser.add_argument("--output-words-file", dest="output_words_file", metavar="PATH", help="output vocabulary (default: %(default)s)")
parser.add_argument("--input_vocab_size", dest="input_vocab_size", type=int, metavar="INT", help="input vocabulary size (default: %(default)s)")
parser.add_argument("--output_vocab_size", dest="output_vocab_size", type=int, metavar="INT", help="output vocabulary size (default: %(default)s)")


parser.set_defaults(
    working_dir = "working"
    ,corpus_stem = "train"
    ,nplm_home = "/home/bhaddow/tools/nplm"
    ,epochs = 2
    ,up_context_size = 2
    ,left_context_size = 3
    ,right_context_size = 0
    ,minibatch_size=1000
    ,noise=100
    ,hidden=0
    ,mode='head'
    ,input_embedding=150
    ,output_embedding=750
    ,threads=4
    ,output_model = "train"
    ,output_dir = None
    ,config_options_file = "config"
    ,log_file = "log"
    ,validation_corpus = None
    ,activation_fn = "rectifier"
    ,learning_rate = 1
    ,input_words_file = None
    ,output_words_file = None
    ,input_vocab_size = 500000
    ,output_vocab_size = 500000
    )

def prepare_vocabulary(options):
  vocab_prefix = os.path.join(options.working_dir, 'vocab')
  extract_vocab_options = extract_vocab.create_parser().parse_args(['--input', options.corpus_stem, '--output', vocab_prefix])
  extract_vocab.main(extract_vocab_options)

  if options.input_words_file is None:
    options.input_words_file = vocab_prefix + '.input'
    orig = vocab_prefix + '.all'
    filtered_vocab = open(orig).readlines()
    if options.input_vocab_size:
      filtered_vocab = filtered_vocab[:options.input_vocab_size]
    open(options.input_words_file,'w').writelines(filtered_vocab)

  if options.output_words_file is None:
    options.output_words_file = vocab_prefix + '.output'
    if options.mode == 'label':
      blacklist = ['<null', '<root', '<start_head', '<dummy', '<head_head', '<stop_head']
      orig = vocab_prefix + '.special'
      filtered_vocab = open(orig).readlines()
      orig = vocab_prefix + '.nonterminals'
      filtered_vocab += open(orig).readlines()
      filtered_vocab = [word for word in filtered_vocab if not word.startswith(prefix) for prefix in blacklist]
      if options.output_vocab_size:
        filtered_vocab = filtered_vocab[:options.output_vocab_size]
    else:
      orig = vocab_prefix + '.all'
      filtered_vocab = open(orig).readlines()[:options.output_vocab_size]
    open(options.output_words_file,'w').writelines(filtered_vocab)

def main(options):

  options.ngram_size = 2*options.up_context_size + 2*options.left_context_size + 2*options.right_context_size
  if options.mode == 'head':
    options.ngram_size += 2
  elif options.mode == 'label':
    options.ngram_size += 1

  if options.input_words_file is None or options.output_words_file is None:
    sys.stderr.write('either input vocabulary or output vocabulary not specified: extracting vocabulary from training text\n')
    prepare_vocabulary(options)

  extract_options = extract_syntactic_ngrams.create_parser().parse_args(['--input', options.corpus_stem,
                                                                         '--output', os.path.join(options.working_dir, os.path.basename(options.corpus_stem) + '.numberized'),
                                                                         '--vocab', options.input_words_file,
                                                                         '--output_vocab', options.output_words_file,
                                                                         '--right_context', str(options.right_context_size),
                                                                         '--left_context', str(options.left_context_size),
                                                                         '--up_context', str(options.up_context_size),
                                                                         '--mode', options.mode
                                                                         ])
  sys.stderr.write('extracting syntactic n-grams\n')
  extract_syntactic_ngrams.main(extract_options)

  if validation_corpus:
    extract_options.input = options.validation_corpus
    options.validation_file = os.path.join(options.working_dir, os.path.basename(options.validation_corpus) + '.numberized')
    extract_options.output = options.validation_file
    sys.stderr.write('extracting syntactic n-grams (validation file)\n')
    extract_syntactic_ngrams.main(extract_options)

  sys.stderr.write('training neural network\n')
  train_nplm.main(options)

  sys.stderr.write('averaging null words\n')
  ret = subprocess.call([os.path.join(sys.path[0], 'average_null_embedding.py'),
                   options.nplm_home,
                   os.path.join(options.output_dir, options.output_model + '.model.nplm.' + str(options.epochs)),
                   os.path.join(options.working_dir, options.corpus_stem + '.numberized'),
                   os.path.join(options.output_dir, options.output_model + '.model.nplm.')
                   ])
  if ret:
      raise Exception("averaging null words failed")

if __name__ == "__main__":
  if sys.version_info < (3, 0):
    sys.stderr = codecs.getwriter('UTF-8')(sys.stderr)
    sys.stdout = codecs.getwriter('UTF-8')(sys.stdout)
    sys.stdin = codecs.getreader('UTF-8')(sys.stdin)

  options = parser.parse_args()
  main(options)

