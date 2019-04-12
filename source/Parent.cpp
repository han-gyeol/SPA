#include "Parent.h"

Parent::Parent(const DesignEntity& firstPara, const DesignEntity& secondPara) {
	paraOne = firstPara;
	paraTwo = secondPara;
	type = ClauseType::PARENT;
	setSynonyms();
}

Result Parent::evaluate(const PKB& pkb) {
	this->pkb = pkb;
	Type paraOneType = paraOne.getType();
	Type paraTwoType = paraTwo.getType();
	string paraOneValue = paraOne.getValue();
	string paraTwoValue = paraTwo.getValue();

	Result result;

	if (paraOneType == FIXED) {
		if (paraTwoType == STATEMENT || paraTwoType == READ || paraTwoType == PRINT 
			|| paraTwoType == WHILE || paraTwoType == IF || paraTwoType == ASSIGN || paraTwoType == CALL || paraTwoType == PROGLINE) {
			result = this->evaluateFixedSynonym(paraOneValue, paraTwoValue, paraTwoType);
		}
		else if (paraTwoType == UNDERSCORE) {
			result = this->evaluateFixedUnderscore(paraOneValue);
		}
		else if (paraTwoType == FIXED) {
			result = this->evaluateFixedFixed(paraOneValue, paraTwoValue);
		}
		else {
			result.setPassed(false);
		}
	}
	else if (paraOneType == STATEMENT || paraOneType == WHILE || paraOneType == IF || paraOneType == PROGLINE) {
		if (paraTwoType == STATEMENT || paraTwoType == READ || paraTwoType == PRINT
			|| paraTwoType == WHILE || paraTwoType == IF || paraTwoType == ASSIGN || paraTwoType == CALL || paraTwoType == PROGLINE) {
			result = this->evaluateSynonymSynonym(paraOneValue, paraTwoValue, paraOneType, paraTwoType);
		}
		else if (paraTwoType == UNDERSCORE) {
			result = this->evaluateSynonymUnderscore(paraOneValue, paraOneType);
		}
		else if (paraTwoType == FIXED) {
			result = this->evaluateSynonymFixed(paraOneValue, paraTwoValue, paraOneType);
		}
		else {
			result.setPassed(false);
		}
	}
	else if (paraOneType == UNDERSCORE) {
		if (paraTwoType == STATEMENT || paraTwoType == READ || paraTwoType == PRINT
			|| paraTwoType == WHILE || paraTwoType == IF || paraTwoType == ASSIGN || paraTwoType == CALL || paraTwoType == PROGLINE) {
			result = this->evaluateUnderscoreSynonym(paraTwoValue, paraTwoType);
		}
		else if (paraTwoType == UNDERSCORE) {
			result = this->evaluateUnderscoreUnderscore();
		}
		else if (paraTwoType == FIXED) {
			result = this->evaluateUnderscoreFixed(paraTwoValue);
		}
		else {
			result.setPassed(false);
		}
	}
	else {
		result.setPassed(false);
	}
	return result;
}

// case Parent(12, w)
Result Parent::evaluateFixedSynonym(const string& parentStmtNum, const string& childrenSynonym, const Type& childrenType) {
	Result result;
	unordered_set<int> answer = pkb.getChildrenOf(stoi(parentStmtNum), childrenType);
	if (!answer.empty()) {
		result.setPassed(true);
		result.setAnswer(childrenSynonym, answer);
	}
	else {
		result.setPassed(false);
	}
	return result;
}

// case Parent(3, _)
Result Parent::evaluateFixedUnderscore(const string& parentStmtNum) {
	Result result;
	result.setPassed(pkb.hasChildren(stoi(parentStmtNum)));
	return result;
}

// case Parent(4, 6)
Result Parent::evaluateFixedFixed(const string& parentStmtNum, const string& childStmtNum) {
	Result result;
	result.setPassed(pkb.isParent(stoi(parentStmtNum), stoi(childStmtNum)));
	return result;
}

// case Parent(i, a)
Result Parent::evaluateSynonymSynonym(const string& parentSynonym, const string& childSynonym, const Type& parentType, const Type& childType) {
	Result result;
	if (parentSynonym == childSynonym) {
		result.setPassed(false);
		return result;
	}
	unordered_map<int, unordered_set<int>> answer = pkb.getParentChildrenPairs(parentType, childType);
	if (!answer.empty()) {
		result.setPassed(true);
		result.setAnswer(parentSynonym, childSynonym, answer);
	}
	else {
		result.setPassed(false);
	}
	return result;
}

// case Parent(w, _)
Result Parent::evaluateSynonymUnderscore(const string& parentSynonym, const Type& parentType) {
	Result result;
	unordered_set<int> answer = pkb.getParentStmts(parentType);
	if (!answer.empty()) {
		result.setPassed(true);
		result.setAnswer(parentSynonym, answer);
	}
	else {
		result.setPassed(false);
	}
	return result;
}

// case Parent(i, 12)
Result Parent::evaluateSynonymFixed(const string& parentSynonym, const string& childStmtNum, const Type& parentType) {
	Result result;
	int answer = pkb.getParentOf(stoi(childStmtNum), parentType);
	if (answer != -1) {
		result.setPassed(true);
		result.setAnswer(parentSynonym, answer);
	}
	else {
		result.setPassed(false);
	}
	return result;
}

// case Parent(_, a)
Result Parent::evaluateUnderscoreSynonym(const string& childSynonym, const Type& childType) {
	Result result;
	unordered_set<int> answer = pkb.getChildrenStmts(childType);
	if (!answer.empty()) {
		result.setPassed(true);
		result.setAnswer(childSynonym, answer);
	}
	else {
		result.setPassed(false);
	}
	return result;
}

// case Parent(_, _)
Result Parent::evaluateUnderscoreUnderscore() {
	Result result;
	unordered_set<int> parents = pkb.getParentStmts(Type::STATEMENT);
	result.setPassed(!parents.empty());
	return result;
}

// case Parent(_, 23)
Result Parent::evaluateUnderscoreFixed(const string& childStmtNum) {
	Result result;
	result.setPassed(pkb.hasParent(stoi(childStmtNum)));
	return result;
}
