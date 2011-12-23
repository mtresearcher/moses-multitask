/*
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 University of Edinburgh
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 
 */
#pragma once

#ifndef FEATUREVECTOR_H
#define FEATUREVECTOR_H

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <valarray>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>

#ifdef MPI_ENABLE
#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/valarray.hpp>
#endif

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#endif

namespace Moses {
	
	typedef float FValue;
	
	/**
	 * Feature name
	 **/
	struct FName {
		
    static const std::string SEP;
    
    typedef boost::unordered_map<std::string,size_t> Name2Id;
    //typedef std::map<std::string, size_t> Name2Id;
    static Name2Id name2id;
    static std::vector<std::string> id2name;
    
    //A feature name can either be initialised as a pair of strings,
    //which will be concatenated with a SEP between them, or as
    //a single string, which will be used as-is.
    explicit FName(const std::string root, const std::string name)
		{init(root + SEP + name);}
    explicit FName(const std::string& name)
		{init(name);}
    
    const std::string& name() const;
    //const std::string& root() const {return m_root;}
    
    size_t hash() const;

    bool operator==(const FName& rhs) const ;
    bool operator!=(const FName& rhs) const ;
		
  private:
    void init(const std::string& name);
    size_t m_id;
#ifdef WITH_THREADS
    //reader-writer lock
    static boost::shared_mutex m_idLock;
#endif
	};
	
	std::ostream& operator<<(std::ostream& out,const FName& name);
	
	struct FNameEquals {
		inline bool operator() (const FName& lhs, const FName& rhs) const {
			return (lhs == rhs);
		}
	};
	
	struct FNameHash
  : std::unary_function<FName, std::size_t>
	{
		std::size_t operator()(const FName& x) const
		{
			return x.hash();
		}
	};
	
	class ProxyFVector;
	
	/**
	 * A sparse feature (or weight) vector.
	 **/
	class FVector
	{
  public:
    /** Empty feature vector */
    FVector(size_t coreFeatures = 0);

    /* 
     * Change the number of core features
    **/
    void resize(size_t newsize);
    
    typedef boost::unordered_map<FName,FValue,FNameHash, FNameEquals> FNVmap;
    /** Iterators */
    typedef FNVmap::iterator iterator;
    typedef FNVmap::const_iterator const_iterator;
    iterator begin() {return m_features.begin();}
    iterator end() {return m_features.end();}
    const_iterator cbegin() const {return m_features.cbegin();}
    const_iterator cend() const {return m_features.cend();}
		
	bool hasNonDefaultValue(FName name) const { return m_features.find(name) != m_features.end();}
    void clear();
    
    
    /** Load from file - each line should be 'root[_name] value' */
    bool load(const std::string& filename);
    void save(const std::string& filename) const;
    void write(std::ostream& out) const ;
    
    /** Element access */
    ProxyFVector operator[](const FName& name);
    FValue& operator[](size_t index);
    FValue operator[](const FName& name) const;
    FValue operator[](size_t index) const;

    /** Size */
    size_t size() const {
      return m_features.size() + m_coreFeatures.size();
    }

    size_t coreSize() const {
      return m_coreFeatures.size();
    }
    
    /** Equality */
    bool operator== (const FVector& rhs) const;
    bool operator!= (const FVector& rhs) const;

    FValue inner_product(const FVector& rhs) const;
    
    friend class ProxyFVector;
    
    /**arithmetic */
    //Element-wise
    //If one side has fewer core features, take the missing ones to be 0.
    FVector& operator+= (const FVector& rhs);
    FVector& operator-= (const FVector& rhs);
    FVector& operator*= (const FVector& rhs);
    FVector& operator/= (const FVector& rhs);
    //Scalar
    FVector& operator*= (const FValue& rhs);
    FVector& operator/= (const FValue& rhs);
    
    FVector& max_equals(const FVector& rhs);
    
    /** norms and sums */
    FValue l1norm() const;
    FValue l2norm() const;
    FValue linfnorm() const;
    FValue sum() const;
    
    /** pretty printing */
    std::ostream& print(std::ostream& out) const;

    void logCoreFeatures(size_t baseOfLog);
    //scale so that abs. value is less than maxvalue
    void thresholdScale(float maxValue );

#ifdef MPI_ENABLE
    friend class boost::serialization::access;
#endif  
    
  private:
    
    /** Internal get and set. */
    const FValue& get(const FName& name) const;
    void set(const FName& name, const FValue& value);
    
		
    FNVmap m_features;
    std::valarray<FValue> m_coreFeatures;
		
#ifdef MPI_ENABLE
    //serialization
    template<class Archive>
    void save(Archive &ar, const unsigned int version) const {
			std::vector<std::string> names;
			std::vector<FValue> values;
			for (const_iterator i = cbegin(); i != cend(); ++i) {
				std::ostringstream ostr;
				ostr << i->first;
				names.push_back(ostr.str());
				values.push_back(i->second);
			}
			ar << names;
			ar << values;
      ar << m_coreFeatures;
    }
		
    template<class Archive>
    void load(Archive &ar, const unsigned int version) {
			clear();
			std::vector<std::string> names;
			std::vector<FValue> values;
			ar >> names;
			ar >> values;
      ar >> m_coreFeatures;
			assert (names.size() == values.size());
			for (size_t i = 0; i < names.size(); ++i) {
				set(FName(names[i]), values[i]);
			}
    }
		
    BOOST_SERIALIZATION_SPLIT_MEMBER()
		
#endif
    
	};
	
	std::ostream& operator<<( std::ostream& out, const FVector& fv);
	//Element-wise operations
	const FVector operator+(const FVector& lhs, const FVector& rhs);
	const FVector operator-(const FVector& lhs, const FVector& rhs);
	const FVector operator*(const FVector& lhs, const FVector& rhs);
	const FVector operator/(const FVector& lhs, const FVector& rhs);
	
	//Scalar operations
	const FVector operator*(const FVector& lhs, const FValue& rhs);
	const FVector operator/(const FVector& lhs, const FValue& rhs);
	
	const FVector fvmax(const FVector& lhs, const FVector& rhs);
	
	FValue inner_product(const FVector& lhs, const FVector& rhs);
	
	struct FVectorPlus {
    FVector operator()(const FVector& lhs, const FVector& rhs) const {
			return lhs + rhs;
    }
	};
	
	/**
	 * Used to help with subscript operator overloading.
	 * See http://stackoverflow.com/questions/1386075/overloading-operator-for-a-sparse-vector
	 **/
	class ProxyFVector {
  public:
    ProxyFVector(FVector *fv, const FName& name ) : m_fv(fv), m_name(name) {}
    ProxyFVector &operator=(const FValue& value) {
      // If we get here, we know that operator[] was called to perform a write access,
      // so we can insert an item in the vector if needed
      //std::cerr << "Inserting " << value << " into " << m_name << std::endl;
      m_fv->set(m_name,value);
      return *this;
      
    }
		
    operator FValue() {
      // If we get here, we know that operator[] was called to perform a read access,
      // so we can simply return the value from the vector
      return m_fv->get(m_name);
    }
    
    /*operator FValue&() {
		 return m_fv->m_features[m_name];
		 }*/
    
    FValue operator++() {
      return ++m_fv->m_features[m_name];
    }
    
    FValue operator +=(FValue lhs) {
      return (m_fv->m_features[m_name] += lhs);
    }
    
    FValue operator -=(FValue lhs) {
      return (m_fv->m_features[m_name] -= lhs);
    }

  private:
    FValue m_tmp;
    
  private:
    FVector* m_fv;
    const FName& m_name;
    
	};
	
}

#endif
