#include "cpgf/scriptbind/gscriptwrapper.h"
#include "cpgf/scriptbind/gscriptbindapi.h"
#include "cpgf/gclassutil.h"
#include "cpgf/gapiutil.h"


namespace cpgf {


GScriptWrapper::GScriptWrapper()
	: scriptDataStorage(NULL)
{
}

IScriptFunction * GScriptWrapper::getScriptFunction(const char * name)
{
	if(this->scriptDataStorage) {
		return this->scriptDataStorage->getScriptFunction(name);
	}
	else {
		return NULL;
	}
}

void GScriptWrapper::setScriptDataStorage(IScriptDataStorage * scriptDataStorage)
{
	this->scriptDataStorage = scriptDataStorage;
}


namespace scriptbind_internal {

class GMetaScriptWrapper : public IMetaScriptWrapper
{
	G_INTERFACE_IMPL_OBJECT

public:
	explicit GMetaScriptWrapper(CasterType caster) : caster(caster) {
	}

	virtual void G_API_CC setScriptDataStorage(void * instance, IScriptDataStorage * scriptDataStorage) {
		static_cast<GScriptWrapper *>(this->caster(instance))->setScriptDataStorage(scriptDataStorage);
	}

private:
	CasterType caster;

	GMAKE_NONCOPYABLE(GMetaScriptWrapper);
};


IMetaScriptWrapper * doCreateScriptWrapper(CasterType caster)
{
	return new GMetaScriptWrapper(caster);
}

} // namespace scriptbind_internal



} // namespace cpgf


