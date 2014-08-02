/*
 * Parameter.cpp
 *
 *  Created on: 17 Feb 2014
 *      Author: hieu
 */
#include "Parameter.h"

Parameter::Parameter()
:maxSpan(10)
,minSpan(0)
,maxNonTerm(2)
,maxHieroNonTerm(999)
,maxSymbolsTarget(999)
,maxSymbolsSource(5)
,minHoleSource(2)
,sentenceOffset(0)
,nonTermConsecSource(false)
,requireAlignedWord(true)
,fractionalCounting(true)
,gzOutput(false)

,hieroNonTerm("[X]")
,sourceSyntax(false)
,targetSyntax(false)

,mixedSyntaxType(0)
,multiLabel(0)
,nonTermConsecSourceMixed(true)
,hieroSourceLHS(false)
,maxSpanFreeNonTermSource(0)
,nieceTerminal(true)
,maxScope(UNDEFINED)
,minScope(0)

,spanLength(false)
,nonTermContext(false)
,nonTermContextTarget(false)
,nonTermContextFactor(0)

,numSourceFactors(1)
,numTargetFactors(1)
{}

Parameter::~Parameter() {
	// TODO Auto-generated destructor stub
}

