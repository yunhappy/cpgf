#ifndef __TESTSCRIPTBINDMETADATA5_H
#define __TESTSCRIPTBINDMETADATA5_H

#include "cpgf/gmetaclass.h"

#include "../testmetatraits.h"

#include "cpgf/scriptbind/gscriptbind.h"
#include "cpgf/scriptbind/gscriptbindapi.h"
#include "cpgf/scriptbind/gscriptbindutil.h"
#include "cpgf/scriptbind/gscriptwrapper.h"

#include "testscriptbind.h"

#include <string.h>

#include <string>


namespace testscript {

class ScriptOverrideBase
{
public:
	ScriptOverrideBase() : n(1) {
	}

	explicit ScriptOverrideBase(int n) : n(n) {
	}

	virtual int getValue() {
		return this->n;
	}

	int n;
};

class ScriptOverride : public ScriptOverrideBase, public cpgf::GScriptWrapper
{
private:
	typedef ScriptOverrideBase super;

public:
	ScriptOverride() : super() {}
	explicit ScriptOverride(int n) : super(n) {}

	virtual int getValue();
};


}


#endif