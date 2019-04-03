#include "DesignEntity.h"
#include "QueryPreprocessorHelper.h"

DesignEntity::DesignEntity() {}

DesignEntity::DesignEntity(const string& value, const Type& type) {
	this->value = value;
	this->type = type;
	this->attrRef = AttrRef::UNASSIGNED;
}

DesignEntity::DesignEntity(const string& value, const Type& type, const AttrRef& attrRef) {
	this->value = value;
	this->type = type;
	this->attrRef = attrRef;
}

void DesignEntity::setValue(const string& value) {
	this->value = value;
}

void DesignEntity::setType(const Type& type) {
	this->type = type;
}

void DesignEntity::setAttrRef(const AttrRef& attrRef) {
	this->attrRef = attrRef;
}

string DesignEntity::getValue() {
	return this->value;
}

Type DesignEntity::getType() {
	return this->type;
}

AttrRef DesignEntity::getAttrRef() {
	return this->attrRef;
}

bool DesignEntity::isType(const Type& other) const {
	return type == other;
}

bool DesignEntity::isAnyType(const std::vector<Type>& types) const {
	for (Type index : types) {
		if (type == index) {
			return true;
		}
	}

	return false;
}

bool DesignEntity::isStmtNo() const {
	return type == Type::FIXED && QueryPreprocessorHelper::isInt(value);
}

bool DesignEntity::isVar() const {
	return type == Type::FIXED && QueryPreprocessorHelper::isVar(value);
}
