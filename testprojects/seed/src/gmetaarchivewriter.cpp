#include "gmetaarchivecommon.h"

#include "cpgf/gmetaapiutil.h"
#include "cpgf/gapiutil.h"

#include <map>
#include <string>

#include <iostream>

using namespace std;


namespace cpgf {


enum GMetaArchivePointerType {
	aptByValue, aptByPointer, aptIgnore
};

class GMetaArchiveWriterPointerTracker;
class GMetaArchiveWriterClassTypeTracker;

class GMetaArchiveWriter : public IMetaArchiveWriter
{
	G_INTERFACE_IMPL_OBJECT
	G_INTERFACE_IMPL_EXTENDOBJECT

public:
	GMetaArchiveWriter(const GMetaArchiveConfig & config, IMetaService * service, IMetaWriter * writer);
	~GMetaArchiveWriter();

	// take care of customized serializer, take care of pointer tracking.
	virtual void G_API_CC writeObject(const char * name, const void * instance, const GMetaTypeData * metaType, IMetaSerializer * serializer);

	virtual void G_API_CC beginWriteObject(const char * name, uint32_t archiveID, const void * instance, IMetaClass * metaClass, uint32_t classTypeID);
	virtual void G_API_CC endWriteObject(const char * name, uint32_t archiveID, const void * instance, IMetaClass * metaClass, uint32_t classTypeID);

protected:
	void writeObjectHelper(const char * name, const void * instance, IMetaClass * metaClass, IMetaSerializer * serializer, GMetaArchivePointerType pointerType);
	
	void doWriteObject(uint32_t archiveID, const void * instance, IMetaClass * metaClass, IMetaSerializer * serializer, GMetaArchivePointerType pointerType, GBaseClassMap * baseClassMap);
	void doDirectWriteObject(uint32_t archiveID, const void * instance, IMetaClass * metaClass, GBaseClassMap * baseClassMap);
	void doDirectWriteObjectWithoutBase(uint32_t archiveID, const void * instance, IMetaClass * metaClass);
	
	void doWriteField(const char * name, const void * instance, IMetaAccessible * accessible);
	void doWriteProperty(const char * name, const void * instance, IMetaAccessible * accessible);
	
	void doWriteValue(const char * name, const void * instance, const GMetaType & metaType, IMetaSerializer * serializer, GMetaArchivePointerType pointerType);
	
	uint32_t getClassTypeID(const void * instance, IMetaClass * metaClass, GMetaArchivePointerType pointerType, IMetaClass ** outCastedMetaClass);

	uint32_t getNextArchiveID();

	GMetaArchiveWriterPointerTracker * getPointerTracker();
	GMetaArchiveWriterClassTypeTracker * getClassTypeTracker();
	
	bool trackPointer(uint32_t archiveID, const void * instance, GMetaArchivePointerType pointerType);

private:
	GMetaArchiveConfig config;
	GScopedInterface<IMetaService> service;
	GScopedInterface<IMetaWriter> writer;
	uint32_t currentArchiveID;
	GScopedPointer<GMetaArchiveWriterPointerTracker> pointerSolver;
	GScopedPointer<GMetaArchiveWriterClassTypeTracker> classTypeTracker;
};


class GMetaArchiveWriterPointerTracker
{
private:
	typedef std::map<const void *, uint32_t> MapType;

public:
	bool hasPointer(const void * p) const;
	uint32_t getArchiveID(const void * p) const;
	void addPointer(const void * p, uint32_t archiveID);

private:
	MapType pointerMap;
};

class GMetaArchiveWriterClassTypeTracker
{
private:
	typedef std::map<string, uint32_t> MapType;

public:
	bool hasClassType(const string & classType) const;
	uint32_t getArchiveID(const string & classType) const;
	void addClassType(const string & classType, uint32_t archiveID);

private:
	MapType classTypeMap;
};


bool GMetaArchiveWriterPointerTracker::hasPointer(const void * p) const
{
	return this->pointerMap.find(p) != this->pointerMap.end();
}

uint32_t GMetaArchiveWriterPointerTracker::getArchiveID(const void * p) const
{
	MapType::const_iterator it = this->pointerMap.find(p);

	if(it == this->pointerMap.end()) {
		return archiveIDNone;
	}
	else {
		return it->second;
	}
}

void GMetaArchiveWriterPointerTracker::addPointer(const void * p, uint32_t archiveID)
{
	GASSERT(! this->hasPointer(p));

	this->pointerMap.insert(pair<const void *, uint32_t>(p, archiveID));
}


bool GMetaArchiveWriterClassTypeTracker::hasClassType(const string & classType) const
{
	return this->classTypeMap.find(classType) != this->classTypeMap.end();
}

uint32_t GMetaArchiveWriterClassTypeTracker::getArchiveID(const string & classType) const
{
	MapType::const_iterator it = this->classTypeMap.find(classType);

	if(it == this->classTypeMap.end()) {
		return archiveIDNone;
	}
	else {
		return it->second;
	}
}

void GMetaArchiveWriterClassTypeTracker::addClassType(const string & classType, uint32_t archiveID)
{
	GASSERT(! this->hasClassType(classType));
	
	this->classTypeMap.insert(pair<string, uint32_t>(classType, archiveID));
}


GMetaArchiveWriter::GMetaArchiveWriter(const GMetaArchiveConfig & config, IMetaService * service, IMetaWriter * writer)
	: config(config), service(service), writer(writer), currentArchiveID(0)
{
	if(this->service) {
		this->service->addReference();
	}

	this->writer->addReference();
}

GMetaArchiveWriter::~GMetaArchiveWriter()
{
}

void G_API_CC GMetaArchiveWriter::writeObject(const char * name, const void * instance, const GMetaTypeData * metaType, IMetaSerializer * serializer)
{
	GMetaType type(*metaType);
	
//	if(this->trackPointer(archiveIDNone, instance, (type.isPointer() ? aptByPointer : aptByValue))) {
//		return;
//	}
	
	if(type.isPointer()) {
		this->doWriteValue(name, instance, type, serializer, aptByPointer);
	}
	else {
		this->doWriteValue(name, instance, type, serializer, aptByValue);
	}
}

void GMetaArchiveWriter::writeObjectHelper(const char * name, const void * instance, IMetaClass * metaClass, IMetaSerializer * serializer, GMetaArchivePointerType pointerType)
{
	uint32_t archiveID = this->getNextArchiveID();

	if(this->trackPointer(archiveID, instance, pointerType)) {
		return;
	}
	
	IMetaClass * outCastedMetaClass;
	uint32_t classTypeID = this->getClassTypeID(instance, metaClass, pointerType, &outCastedMetaClass);
	GScopedInterface<IMetaClass> castedMetaClass(outCastedMetaClass);

	this->beginWriteObject(name, archiveID, instance, castedMetaClass.get(), classTypeID);

	GBaseClassMap baseClassMap;
	this->doWriteObject(archiveID, instance, castedMetaClass.get(), serializer, pointerType, &baseClassMap);

	this->endWriteObject(name, archiveID, instance, castedMetaClass.get(), classTypeID);
}

void GMetaArchiveWriter::doWriteObject(uint32_t archiveID, const void * instance, IMetaClass * metaClass, IMetaSerializer * serializer, GMetaArchivePointerType pointerType, GBaseClassMap * baseClassMap)
{
	GScopedInterface<IMetaSerializer> serializerPointer;
	if(serializer == NULL && metaClass != NULL) {
		serializerPointer.reset(metaGetItemExtendType(metaClass, GExtendTypeCreateFlag_Serializer).getSerializer());
		serializer = serializerPointer.get();
	}

	if(serializer != NULL) {
		serializer->writeObject(this, this->writer.get(), archiveID, instance, metaClass);
	}
	else {
		if(metaClass == NULL) {
			serializeError(Error_Serialization_MissingMetaClass);
		}

		this->doDirectWriteObject(archiveID, instance, metaClass, baseClassMap);
	}
}

bool GMetaArchiveWriter::trackPointer(uint32_t archiveID, const void * instance, GMetaArchivePointerType pointerType)
{
	if(pointerType != aptIgnore) {
		if(this->config.allowTrackPointer()) {

			if(this->getPointerTracker()->hasPointer(instance)) {
				if(pointerType == aptByValue) {
					return false;
				}
				else {
					if(archiveID == archiveIDNone) {
						archiveID = this->getNextArchiveID();
					}
					this->writer->writeReferenceID("", archiveID, this->getPointerTracker()->getArchiveID(instance));
				}

				return true;
			}

			this->getPointerTracker()->addPointer(instance, archiveID);
		}
	}

	return false;
}

uint32_t GMetaArchiveWriter::getClassTypeID(const void * instance, IMetaClass * metaClass, GMetaArchivePointerType pointerType, IMetaClass ** outCastedMetaClass)
{
	uint32_t classTypeID = archiveIDNone;

	*outCastedMetaClass = NULL;

	if(metaClass != NULL) {
		if(pointerType == aptByPointer) {
			void * castedPtr;
			GScopedInterface<IMetaClass> castedMetaClass(findAppropriateDerivedClass(instance, metaClass, &castedPtr));
			if(! castedMetaClass->equals(metaClass)) {
				const char * typeName = castedMetaClass->getTypeName();
				if(this->getClassTypeTracker()->hasClassType(typeName)) {
					classTypeID = this->getClassTypeTracker()->getArchiveID(typeName);
				}
				else {
					classTypeID = this->getNextArchiveID();
					this->getClassTypeTracker()->addClassType(typeName, classTypeID);
					this->writer->writeClassType("", classTypeID, castedMetaClass.get());
				}
				*outCastedMetaClass = castedMetaClass.take();
			}
		}
		
		if(*outCastedMetaClass == NULL) {
			*outCastedMetaClass = metaClass;
			metaClass->addReference();
		}
	}

	return classTypeID;
}

void GMetaArchiveWriter::doDirectWriteObject(uint32_t archiveID, const void * instance, IMetaClass * metaClass, GBaseClassMap * baseClassMap)
{
	GScopedInterface<IMetaClass> baseClass;
	uint32_t i;
	uint32_t count;

	count = metaClass->getBaseCount();
	for(i = 0; i < count; ++i) {
		baseClass.reset(metaClass->getBaseClass(i));
		
		if(canSerializeBaseClass(this->config, baseClass.get(), metaClass)) {
			void * baseInstance = metaClass->castToBase(instance, i);
			if(! baseClassMap->hasMetaClass(baseInstance, baseClass.get())) {
				baseClassMap->addMetaClass(baseInstance, baseClass.get());
				this->doWriteObject(archiveIDNone, baseInstance, baseClass.get(), NULL, aptIgnore, baseClassMap);
			}
		}
	}

	this->doDirectWriteObjectWithoutBase(archiveID, instance, metaClass);
}

void GMetaArchiveWriter::doDirectWriteObjectWithoutBase(uint32_t archiveID, const void * instance, IMetaClass * metaClass)
{
	(void)archiveID;

	if(instance == NULL) {
		return;
	}

	GScopedInterface<IMetaAccessible> accessible;
	uint32_t i;
	uint32_t count;

	if(this->config.allowSerializeField()) {
		count = metaClass->getFieldCount();
		for(i = 0; i < count; ++i) {
			accessible.reset(metaClass->getFieldAt(i));

			if(canSerializeField(this->config, accessible.get(), metaClass)) {
				this->doWriteField(accessible->getName(), instance, accessible.get());
			}
		}
	}

	if(this->config.allowSerializeProperty()) {
		count = metaClass->getPropertyCount();
		for(i = 0; i < count; ++i) {
			accessible.reset(metaClass->getPropertyAt(i));
			if(canSerializeField(this->config, accessible.get(), metaClass)) {
				this->doWriteProperty(accessible->getName(), instance, accessible.get());
			}
		}
	}
}

void GMetaArchiveWriter::doWriteField(const char * name, const void * instance, IMetaAccessible * accessible)
{
	GMetaType metaType = metaGetItemType(accessible);
	size_t pointers = metaType.getPointerDimension();

	if(pointers <= 1) {
		GScopedInterface<IMetaSerializer> serializer(metaGetItemExtendType(accessible, GExtendTypeCreateFlag_Serializer).getSerializer());
		
		void * ptr;
		GMetaArchivePointerType pointerType;

		if(metaType.isArray()) {
			ptr = accessible->getAddress(instance);
			pointerType = aptByValue;
		}
		else {
			if(pointers == 0) {
				ptr = accessible->getAddress(instance);
				if(ptr == NULL) { // this happens when accessible is a property with both getter and setter.
					GVariant v(metaGetValue(accessible, instance));
					if(canFromVariant<void *>(v)) {
						ptr = fromVariant<void *>(v);
					}
				}
				pointerType = aptByValue;
			}
			else {
				ptr = fromVariant<void *>(metaGetValue(accessible, instance));
				pointerType = aptByPointer;
			}
		}

		GASSERT(ptr != NULL || serializer);

		this->doWriteValue(name, ptr, metaType, serializer.get(), pointerType);
	}
	else {
	}
}

void GMetaArchiveWriter::doWriteProperty(const char * name, const void * instance, IMetaAccessible * accessible)
{
	GMetaType metaType = metaGetItemType(accessible);
	size_t pointers = metaType.getPointerDimension();

	if(pointers == 0 && metaType.isFundamental()) {
		GVariant v(metaGetValue(accessible, instance));
		this->writer->writeFundamental(name, this->getNextArchiveID(), &v.data);
		return;
	}

	this->doWriteField(name, instance, accessible);
}

void GMetaArchiveWriter::doWriteValue(const char * name, const void * address, const GMetaType & metaType, IMetaSerializer * serializer, GMetaArchivePointerType pointerType)
{
	if(metaType.isArray()) {
		this->writeObjectHelper(name, address, NULL, serializer, aptByValue);
		return;
	}

	unsigned int pointers = metaType.getPointerDimension();

	if(pointers == 0 && metaType.isFundamental()) {
		GVariant v(readFundamental(address, metaType));
		this->writer->writeFundamental(name, this->getNextArchiveID(), &v.data);
		return;
	}

	if(pointers > 1) {
		serializeError(Error_Serialization_UnknownType);
	}

	if(metaType.baseIsClass()) {
		if(address == NULL && pointerType == aptByPointer) {
			this->writer->writeNullPointer(name);
		}
		else {
			GScopedInterface<IMetaClass> metaClass;
			GScopedInterface<IMetaSerializer> scopedSerializer;
			if(metaType.getBaseName() != NULL) {
				metaClass.reset(this->service->findClassByName(metaType.getBaseName()));
			}
			if(serializer == NULL) {
				scopedSerializer.reset(metaGetItemExtendType(metaClass, GExtendTypeCreateFlag_Serializer).getSerializer());
				serializer = scopedSerializer.get();
			}
			this->writeObjectHelper(name, address, metaClass.get(), serializer, pointerType);
		}
	}
}

uint32_t GMetaArchiveWriter::getNextArchiveID()
{
	++this->currentArchiveID;
	return this->currentArchiveID;
}

GMetaArchiveWriterPointerTracker * GMetaArchiveWriter::getPointerTracker()
{
	if(! this->pointerSolver) {
		this->pointerSolver.reset(new GMetaArchiveWriterPointerTracker);
	}

	return this->pointerSolver.get();
}

GMetaArchiveWriterClassTypeTracker * GMetaArchiveWriter::getClassTypeTracker()
{
	if(! this->classTypeTracker) {
		this->classTypeTracker.reset(new GMetaArchiveWriterClassTypeTracker);
	}

	return this->classTypeTracker.get();
}

void G_API_CC GMetaArchiveWriter::beginWriteObject(const char * name, uint32_t archiveID, const void * instance, IMetaClass * metaClass, uint32_t classTypeID)
{
	GMetaArchiveObjectInformation objectInformation;
	
	objectInformation.name = name;
	objectInformation.archiveID = archiveID;
	objectInformation.classTypeID = classTypeID;
	objectInformation.instance = instance;
	objectInformation.metaClass = metaClass;

	this->writer->beginWriteObject(&objectInformation);
}

void G_API_CC GMetaArchiveWriter::endWriteObject(const char * name, uint32_t archiveID, const void * instance, IMetaClass * metaClass, uint32_t classTypeID)
{
	GMetaArchiveObjectInformation objectInformation;
	
	objectInformation.name = name;
	objectInformation.archiveID = archiveID;
	objectInformation.classTypeID = classTypeID;
	objectInformation.instance = instance;
	objectInformation.metaClass = metaClass;

	this->writer->endWriteObject(&objectInformation);
}

IMetaArchiveWriter * createMetaArchiveWriter(uint32_t config, IMetaService * service, IMetaWriter * writer)
{
	return new GMetaArchiveWriter(GMetaArchiveConfig(config), service, writer);
}


} // namespace cpgf



