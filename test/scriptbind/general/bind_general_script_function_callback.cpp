#include "../testscriptbind.h"

#include "cpgf/scriptbind/gscriptbindutil.h"


namespace {

string makeFunc(const string & name, TestScriptContext * context)
{
	if(context->isLua()) {
		return "function " + name + "(t, u) return t + u end";
	}
	if(context->isV8()) {
		return "function " + name + "(t, u) { return t + u; }";
	}

	return "";
}

template <typename T>
void doTestScriptFunctionCallback(T * binding, TestScriptContext * context)
{
	(void)binding;

	DO(makeFunc("fcallback", context))
	QDO(a = testAddCallback(fcallback))
	QASSERT(a == 11)
	
	DO(string("b = testAddCallback(") + makeFunc("", context) + ")")
	QASSERT(b == 11)
}

void testScriptFunctionCallback(TestScriptContext * context)
{
	GScriptObject * bindingLib = context->getBindingLib();
	IScriptObject * bindingApi = context->getBindingApi();

	if(bindingLib) {
		doTestScriptFunctionCallback(bindingLib, context);
	}
	
	if(bindingApi) {
		doTestScriptFunctionCallback(bindingApi, context);
	}
}

#define CASE testScriptFunctionCallback
#include "../bind_testcase.h"



template <typename T>
void doTestScriptFunctionProperty(T * binding, TestScriptContext * context)
{
	(void)binding;

	DO(makeFunc("fcallback", context))
	QDO(testScriptFunction = fcallback)
	QDO(a = testExecAddCallback())
	QASSERT(a == 11)

	DO(string("testScriptFunction = ") + makeFunc("", context) + "")
	QDO(b = testExecAddCallback())
	QASSERT(b == 11)
}

void testScriptFunctionProperty(TestScriptContext * context)
{
	GScriptObject * bindingLib = context->getBindingLib();
	IScriptObject * bindingApi = context->getBindingApi();

	if(bindingLib) {
		doTestScriptFunctionProperty(bindingLib, context);
	}
	
	if(bindingApi) {
		doTestScriptFunctionProperty(bindingApi, context);
	}
}

#define CASE testScriptFunctionProperty
#include "../bind_testcase.h"




}
