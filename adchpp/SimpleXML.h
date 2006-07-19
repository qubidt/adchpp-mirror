/* 
 * Copyright (C) 2006 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef SIMPLEXML_H
#define SIMPLEXML_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Exception.h"
#include "Util.h"

STANDARD_EXCEPTION(SimpleXMLException);

/**
 * A simple XML class that loads an XML-ish structure into an internal tree
 * and allows easy access to each element through a "current location".
 */
class SimpleXML  
{
public:
	DLL SimpleXML(int numAttribs = 0);
	DLL ~SimpleXML();
	
	DLL void addTag(const string& aName, const string& aData = Util::emptyString) throw(SimpleXMLException);
	void addTag(const string& aName, int aData) throw(SimpleXMLException) {
		addTag(aName, Util::toString(aData));
	}
	void addTag(const string& aName, int64_t aData) throw(SimpleXMLException) {
		addTag(aName, Util::toString(aData));
	}

	template<typename T>
	void addAttrib(const string& aName, const T& aData) throw(SimpleXMLException) {
		addAttrib(aName, Util::toString(aData));
	}

	DLL void addAttrib(const string& aName, const string& aData) throw(SimpleXMLException);
	void addAttrib(const string& aName, bool aData) throw(SimpleXMLException) {	
		addAttrib(aName, string(aData ? "1" : "0"));
	}
	
	template <typename T>
    void addChildAttrib(const string& aName, const T& aData) throw(SimpleXMLException) {	
		addChildAttrib(aName, Util::toString(aData));
	}
	DLL void addChildAttrib(const string& aName, const string& aData) throw(SimpleXMLException);
	void addChildAttrib(const string& aName, bool aData) throw(SimpleXMLException) {	
		addChildAttrib(aName, string(aData ? "1" : "0"));
	}
	
	const string& getData() const {
		dcassert(current != NULL);
		return current->data;
	}
	
	DLL void stepIn() const throw(SimpleXMLException);
	DLL void stepOut() const throw(SimpleXMLException);
	
	void resetCurrentChild() const throw() {
		found = false;
		dcassert(current != NULL);
		currentChild = current->children.begin();
	}

	DLL bool findChild(const string& aName) const throw();

	const string& getChildData() const throw(SimpleXMLException) {
		checkChildSelected();
		return (*currentChild)->data;
	}

	const string& getChildAttrib(const string& aName, const string& aDefault = Util::emptyString) const throw(SimpleXMLException) {
		checkChildSelected();
		return (*currentChild)->getAttrib(aName, aDefault);
	}

	int getIntChildAttrib(const string& aName) throw(SimpleXMLException) {
		checkChildSelected();
		return Util::toInt(getChildAttrib(aName));
	}
	int64_t getLongLongChildAttrib(const string& aName) throw(SimpleXMLException) {
		checkChildSelected();
		return Util::toInt64(getChildAttrib(aName));
	}
	bool getBoolChildAttrib(const string& aName) throw(SimpleXMLException) {
		checkChildSelected();
		const string& tmp = getChildAttrib(aName);

		return (tmp.size() > 0) && tmp[0] == '1';
	}
	
	DLL void fromXML(const string& aXML) throw(SimpleXMLException);
	string toXML() { return (!root->children.empty()) ? root->children[0]->toXML(0) : Util::emptyString; }
	
	DLL static void escape(string& aString, bool aAttrib, bool aLoading = false);
	/** 
	 * This is a heurestic for whether escape needs to be called or not. The results are
 	 * only guaranteed for false, i e sometimes true might be returned even though escape
	 * was not needed...
	 */
	static bool needsEscape(const string& aString, bool aAttrib, bool aLoading = false) {
		return ((aLoading) ? aString.find('&') : aString.find_first_of(aAttrib ? "<&>'\"" : "<&>")) != string::npos;
	}
private:
	class Tag {
	public:
		typedef Tag* Ptr;
		typedef vector<Ptr> List;
		typedef List::iterator Iter;
		typedef pair<string,string> StringPair;
		typedef vector<StringPair> AttribMap;
		typedef AttribMap::iterator AttribIter;

		/**
		 * A simple list of children. To find a tag, one must search the entire list.
		 */ 
		List children;
		/**
		 * Attributes of this tag. According to the XML standard the names
		 * must be unique (case-sensitive). (Assuming that we have few attributes here,
		 * we use a vector instead of a (hash)map to save a few bytes of memory and unnecessary
		 * calls to the memory allocator...)
		 */
		AttribMap attribs;
		
		/** Tag name */
		string name;

		/** Tag data, may be empty. */
		string data;
				
		/** Parent tag, for easy traversal */
		Ptr parent;

		Tag(const string& aName, const string& aData, Ptr aParent, int numAttribs = 0) : name(aName), data(aData), parent(aParent) { 
			if(numAttribs > 0) 
				attribs.reserve(numAttribs);
		}
		
		const string& getAttrib(const string& aName, const string& aDefault = Util::emptyString) {
			AttribIter i = find_if(attribs.begin(), attribs.end(), CompareFirst<string,string>(aName));
			return (i == attribs.end()) ? aDefault : i->second; 
		}
		DLL string toXML(int indent);
		
		string::size_type fromXML(const string& tmp, string::size_type start, int aa, bool isRoot = false) throw(SimpleXMLException);
		string::size_type loadAttribs(const string& tmp, string::size_type start) throw(SimpleXMLException);

		void appendAttribString(string& tmp);
		/** Delete all children! */
		~Tag() {
			for(Iter i = children.begin(); i != children.end(); ++i) {
				delete *i;
			}
		}
	};

	/** Bogus root tag, should be only one child! */
	Tag::Ptr root;

	/** Current position */
	mutable Tag::Ptr current;

	mutable Tag::Iter currentChild;

	void checkChildSelected() const throw() {
		dcassert(current != NULL);
		dcassert(currentChild != current->children.end());
	}

	int attribs;
	mutable bool found;
};

#endif // SIMPLEXML_H
