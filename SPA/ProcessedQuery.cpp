#include "Clause.h"
#include "Follows.h"
#include "FollowsT.h"
#include "ModifiesS.h"
#include "Parent.h"
#include "ParentT.h"
#include "Pattern.h"
#include "UsesS.h"
#include "ProcessedQuery.h"

ProcessedQuery::ProcessedQuery() {

}

bool ProcessedQuery::insertDeclaration(const std::string& synonym,
	const Type& type) {
	return declarations.insert({ synonym, type }).second;
}

void ProcessedQuery::addSynonym(const std::string& newSynonym) {
	synonym = newSynonym;
}

bool ProcessedQuery::addSuchThatClause(const RelationshipType& type,
	const DesignEntity& paramOne,
	const DesignEntity& paramTwo) {
	if (hasSuchThatClause) {
		return false;
	}

	hasSuchThatClause = true;

	suchThatClause.type = type;
	suchThatClause.paramOne = paramOne;
	suchThatClause.paramTwo = paramTwo;

	return true;
}

bool ProcessedQuery::addPatternClause(const DesignEntity& assign,
	const DesignEntity& paramOne,
	const DesignEntity& paramTwo) {
	if (hasPatternClause) {
		return false;
	}

	hasPatternClause = true;

	patternClause.assign = assign;
	patternClause.paramOne = paramOne;
	patternClause.paramTwo = paramTwo;

	return true;
}

std::unordered_set<Clause*> ProcessedQuery::getClauses() {
	unordered_set<Clause*> clauses;
	if (hasSuchThatClause) {
		std::string paramOneValue = suchThatClause.paramOne.getValue();
		std::string paramTwoValue = suchThatClause.paramTwo.getValue();

		if (suchThatClause.paramOne.getType() == Type::ASSIGN) {
			Type paramOneType = declarations.find(paramOneValue)->second;
			suchThatClause.paramOne.setType(paramOneType);
		}

		if (suchThatClause.paramTwo.getType() == Type::ASSIGN) {
			Type paramTwoType = declarations.find(paramTwoValue)->second;
			suchThatClause.paramTwo.setType(paramTwoType);
		}

		if (suchThatClause.type == RelationshipType::MODIFIES_S) {
			ModifiesS modifiess(suchThatClause.paramOne, suchThatClause.paramTwo);
			clauses.insert(&modifiess);
		}
		else if (suchThatClause.type == RelationshipType::FOLLOWS) {
			Follows follows(suchThatClause.paramOne, suchThatClause.paramTwo);
			clauses.insert(&follows);
		}
		else if (suchThatClause.type == RelationshipType::FOLLOWS_T) {
			FollowsT followsT(suchThatClause.paramOne, suchThatClause.paramTwo);
			clauses.insert(&followsT);
		}
		else if (suchThatClause.type == RelationshipType::PARENT) {
			Parent parent(suchThatClause.paramOne, suchThatClause.paramTwo);
			clauses.insert(&parent);
		}
		else if (suchThatClause.type == RelationshipType::PARENT_T) {
			ParentT parentT(suchThatClause.paramOne, suchThatClause.paramTwo);
			clauses.insert(&parentT);
		}
		else {
			UsesS usesS(suchThatClause.paramOne, suchThatClause.paramTwo);
			clauses.insert(&usesS);
		}
	}

	if (hasPatternClause) {
		Pattern pattern(patternClause.assign, patternClause.paramOne, patternClause.paramTwo);
		clauses.insert(&pattern);
	}

	return clauses;
}

DesignEntity ProcessedQuery::getSelectedSynonym() {
	return DesignEntity(synonym, declarations[synonym]);
}