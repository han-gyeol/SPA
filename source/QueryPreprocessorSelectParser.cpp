#include <regex>
#include <unordered_set>
#include "Calls.h"
#include "CallsT.h"
#include "DesignEntity.h"
#include "Follows.h"
#include "FollowsT.h"
#include "ModifiesP.h"
#include "ModifiesS.h"
#include "Next.h"
#include "NextT.h"
#include "Parent.h"
#include "ParentT.h"
#include "PatternAssign.h"
#include "PatternIf.h"
#include "PatternWhile.h"
#include "QueryPreprocessorHelper.h"
#include "QueryPreprocessorDeclareParser.h"
#include "QueryPreprocessorSelectParser.h"
#include "UsesP.h"
#include "UsesS.h"

constexpr auto SPACE = ' ';
constexpr auto COMMA = ',';
constexpr auto CALL_OPERATOR = '.';
constexpr auto SELECT_START = 7;
constexpr auto LEFT_TUPLE_BRACKET = '<';
constexpr auto RIGHT_TUPLE_BRACKET = '>';

// Regular Expressions
constexpr char IDENT[] = "^[a-zA-z][a-zA-Z\\d]*$";
const regex QueryPreprocessorSelectParser::IDENT_REGEX(IDENT);

constexpr char INT[] = "^[123456789][\\d]*$";
const regex QueryPreprocessorSelectParser::INT_REGEX(INT);

constexpr char REL_REF[] = "^(Modifies|Uses|Calls|Calls\\*|Parent|Parent\\*|Follows|Follows\\*|Next|Next\\*|Affects|Affects\\*)$";
const regex QueryPreprocessorSelectParser::REL_REF_REGEX(REL_REF);

// Initializes a newly created QueryPreprocessorSelectParser.
QueryPreprocessorSelectParser::QueryPreprocessorSelectParser(const Statement& statement, ProcessedQuery& query)
				: STATEMENT(statement.VALUE),
				query(query) {
}

// Parse select clause.
bool QueryPreprocessorSelectParser::parse() {
	// remove "Select "
	std::string selectCl = STATEMENT;
	selectCl.erase(0, 7);
	
	// extract result clause
	size_t resultClSize = selectCl.find_first_of(SPACE);
	if (resultClSize == std::string::npos) {
		// case 0: there is only a result clause
		bool status = parseResultCl(selectCl);
		return status;
	}

	// case 1: there are other clauses
	std::string resultCl = selectCl.substr(0, resultClSize);
	selectCl.erase(0, resultClSize + 1);

	bool status = parseResultCl(resultCl);
	if (!status) {
		return false;
	}

	// extract such that, pattern, with clauses
	bool isSuchThat = false;
	while (selectCl.size() > 0) {
		std::string type;
		size_t typeSize;

		if (selectCl.find("such that ") == 0) {
			type = "such that";
			isSuchThat = true;
		} else if (selectCl.find("pattern ") == 0) {
			type = "pattern";
			isSuchThat = false;
		} else if (selectCl.find("with ") == 0) {
			type = "with";
			isSuchThat = false;
		} else if (selectCl.find("and ") == 0) {
			type = "and";
		} else {
			return false;
		}

		typeSize = type.size() + 1;

		std::string clause;

		size_t clauseEndPos = selectCl.find(SPACE, typeSize + 6);
		if (clauseEndPos == std::string::npos) {
			clause = selectCl.substr(typeSize);
		} else {
			clause = selectCl.substr(typeSize, clauseEndPos - typeSize);
		}

		selectCl.erase(0, clauseEndPos + 1);

		bool status;
		if (type == "such that") {
			status = parseSuchThatCl(clause);
		} else if (type == "pattern") {
			status = parsePatternCl(clause);
		} else if (type == "with") {
			status = parseWithCl(clause);
		} else {
			// Clauses should be in the form of rel(...)
			size_t relSize = clause.find('(');
			if (relSize == std::string::npos) {
				return false;
			}

			// If rel is found in declarations, the clause is a pattern clause
			std::string rel = clause.substr(0, relSize);
			if (query.declarations.find(rel) == query.declarations.end() && isSuchThat) {
				parseWithCl(clause);
			} else if (!isSuchThat) {
				parsePatternCl(clause);
			} else {
				return false;
			}
		}

		if (!status) {
			return false;
		}
	}

	return true;
}

// Parse result clause.
bool QueryPreprocessorSelectParser::parseResultCl(std::string& resultCl) {
	// if resultCl is a tuple of element
	// else if resultCl is a boolean
	// else resultCl is a element
	if (resultCl.front() == LEFT_TUPLE_BRACKET && resultCl.back() == RIGHT_TUPLE_BRACKET) {
		// remove bracket
		size_t resultClSize = resultCl.size() - 2;
		resultCl = resultCl.substr(1, resultClSize);

		// tokenize result clause into elements
		std::vector<std::string> elemList;
		elemList = QueryPreprocessorHelper::split(resultCl, COMMA);

		// parse individual element
		for (std::string elem : elemList) {
			bool status = parseElem(elem);
			if (!status) {
				return false;
			}
		}
	} else if (resultCl == "BOOLEAN") {
		DesignEntity element("", Type::BOOLEAN);
		resultClElemList.push_back(element);
	} else {
		bool status = parseElem(resultCl);
		if (!status) {
			return false;
		}
	}
}

// Parse such that clause.
bool QueryPreprocessorSelectParser::parseSuchThatCl(std::string& suchThatCl) {
	size_t relSize = suchThatCl.find('(');
	size_t closeBracketPos = suchThatCl.find(')');

	// close bracket should be the last character
	if (closeBracketPos + 1 != suchThatCl.size()) {
		return false;
	}

	// open bracket should exist
	if (relSize == std::string::npos) {
		return false;
	}

	// extract rel and check that it is legal
	std::string rel = suchThatCl.substr(0, relSize);
	if (!regex_match(rel, REL_REF_REGEX)) {
		return false;
	}

	// remove rel and the brackets
	suchThatCl = suchThatCl.substr(relSize + 1, closeBracketPos - relSize - 1);

	std::vector<string> parameters = QueryPreprocessorHelper::split(suchThatCl, COMMA);

	if (parameters.size() != 2) {
		return false;
	}

	size_t paramOneSize = suchThatCl.find(COMMA);
	if (paramOneSize == std::string::npos) {
		return false;
	}

	DesignEntity paramOne;
	DesignEntity paramTwo;

	if (!regex_match(parameters[0], IDENT_REGEX)) {
		paramOne = parseParameter(parameters[0]);
		return false;
	}

	if (!regex_match(parameters[1], IDENT_REGEX)) {
		paramTwo = parseParameter(parameters[1]);
		return false;
	}

	if (rel == "Calls") {
		Calls suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Calls*") {
		CallsT suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Follows") {
		Follows suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Follows*") {
		FollowsT suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Modifies") {
		ModifiesP suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Modifies") {
		ModifiesS suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Next") {
		Next suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Next*") {
		NextT suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Parent") {
		Parent suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Uses") {
		UsesP suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	} else if (rel == "Uses") {
		UsesS suchThatClause(paramOne, paramTwo);
		query.addClause(&suchThatClause);
	}

	return true;
}

bool QueryPreprocessorSelectParser::parsePatternCl(std::string& patternCl) {
	size_t synonymSize = patternCl.find('(');
	size_t closeBracketPos = patternCl.find(')');

	// first close bracket must be the last close bracket
	// close bracket must be the last character in pattern clause
	if (closeBracketPos + 1 != patternCl.size()) {
		return false;
	}

	// open bracket should exist
	if (synonymSize == std::string::npos) {
		return false;
	}

	// extract synonym and check that it exist in declarations
	std::string synonymString = patternCl.substr(0, synonymSize);
	std::unordered_map<std::string, Type>::const_iterator element = query.declarations.find(synonymString);
	if (element == query.declarations.end()) {
		return false;
	}
	Type designEntity = element->second;
	DesignEntity synonym(synonymString, designEntity);

	// erase synonym and the brackets
	patternCl = patternCl.substr(synonymSize + 1, closeBracketPos - synonymSize - 1);

	// parse pattern parameters
	std::vector<string> parameters = QueryPreprocessorHelper::split(patternCl, COMMA);
	if (designEntity == Type::ASSIGN && parameters.size() == 2) {
		// assign pattern
		DesignEntity paramOne = parseParameter(parameters[0]);
		DesignEntity paramTwo = parseParameter(parameters[1]);
		PatternAssign pattern(synonym, paramOne, paramTwo);
		query.addClause(&pattern);
	} else if (designEntity == Type::WHILE 
			&& parameters.size() == 2
			&& parameters[1] == "_") {
		// while pattern
		DesignEntity paramOne = parseParameter(parameters[0]);
		PatternWhile pattern(synonym, paramOne);
		query.addClause(&pattern);
	} else if (designEntity == Type::IF 
			&& parameters.size() == 3
			&& parameters[1] == "_"
			&& parameters[2] == "_") {
		// if pattern
		DesignEntity paramOne = parseParameter(parameters[0]);
		PatternIf pattern(synonym, paramOne);
		query.addClause(&pattern);
	} else {
		return false;
	}

	return true;
}

bool QueryPreprocessorSelectParser::parseWithCl(std::string& withCl) {
	size_t lhsSize = withCl.find('=');
	if (lhsSize == std::string::npos) {
		return false;
	}

	std::string lhs = withCl.substr(0, lhsSize);
	std::string rhs = withCl.substr(lhsSize + 1);

	size_t lhsSynonymSize = lhs.find('.');
	std::string lhsSynonym;
	std::string lhsAttrRef;

	if (lhsSynonymSize != std::string::npos) {
		std::string synonymString = lhs.substr(0, lhsSynonymSize);
		DesignEntity synonym = parseParameter(synonymString);
		unordered_map<std::string, Type>::const_iterator element = query.declarations.find(synonymString);
		if (element == query.declarations.end()) {
			return false;
		}

		lhsAttrRef = lhs.substr(lhsSynonymSize + 1);
	} else {
		lhsSynonym = lhs;
		lhsAttrRef = "";
	}

	size_t rhsSynonymSize = rhs.find(CALL_OPERATOR);
	std::string rhsSynonym;
	std::string rhsAttrRef;

	if (rhsSynonymSize != std::string::npos) {
		rhsSynonym = rhs.substr(0, rhsSynonymSize);
		rhsAttrRef = rhs.substr(rhsSynonymSize + 1);
	} else {
		rhsSynonym = rhs;
		rhsAttrRef = "";
	}
}

// parse element
bool QueryPreprocessorSelectParser::parseElem(std::string& elem) {
	// if elem is synonym
	// else elem is synonym.attrRef
	if (elem.find(CALL_OPERATOR) == std::string::npos) {
		// check if synonym is valid
		if (!regex_match(elem, IDENT_REGEX)) {
			return false;
		}

		// synonym does not exist in declare clause
		std::unordered_map<std::string, Type>::const_iterator result;
		result = query.declarations.find(elem);
		if (result == query.declarations.end()) {
			return false;
		}

		Type designEntity = result->second;
		DesignEntity element(elem, designEntity);
		resultClElemList.push_back(element);
	} else {
		size_t resultClSynonymEnd = elem.find(CALL_OPERATOR);
		std::string synonym = elem.substr(0, resultClSynonymEnd);
		std::string attrRefString = elem.substr(resultClSynonymEnd + 1);
		
		// synonym is invalid
		if (!regex_match(synonym, IDENT_REGEX)) {
			return false;
		}

		// synonym does not exist in declare clause
		std::unordered_map<std::string, Type>::const_iterator result;
		result = query.declarations.find(synonym);
		if (result == query.declarations.end()) {
			return false;
		}

		Type designEntity = result->second;

		// attrRef is invalid
		if (attrRefString != "procName"
				&& attrRefString != "varName"
				&& attrRefString != "value"
				&& attrRefString != "stmt#") {
			return false;
		}

		AttrRef attrRef = QueryPreprocessorHelper::getAttrRef(attrRefString);

		if (!isValidSynonymAttrRefPair(designEntity, attrRef)) {
			return false;
		}

		DesignEntity element(synonym, designEntity, attrRef);
		resultClElemList.push_back(element);
	}
}

bool QueryPreprocessorSelectParser::isValidSynonymAttrRefPair(Type de,
		AttrRef attrRef) {
	if (attrRef == AttrRef::PROC_NAME) {
		if (de != Type::PROCEDURE && de != Type::CALL) {
			return false;
		}
	} else if (attrRef == AttrRef::VAR_NAME) {
		if (de != Type::VARIABLE && de != Type::READ && de != Type::PRINT) {
			return false;
		}
	} else if (attrRef == AttrRef::VALUE) {
		if (de != Type::CONSTANT) {
			return false;
		}
	} else if (attrRef == AttrRef::STMT_NUM) {
		if (de != Type::STATEMENT && de != Type::READ && de != Type::PRINT
			 && de != Type::CALL && de != Type::WHILE && de != Type::IF
			 && de != Type::ASSIGN) {
			return false;
		}
	}
	
	return true;
}

DesignEntity QueryPreprocessorSelectParser::parseParameter(std::string& parameter) {
	if (parameter == "_") {
		// underscore
		return DesignEntity("", Type::UNDERSCORE);
	} else if (regex_match(parameter, INT_REGEX)) {
		// statement number
		return DesignEntity(parameter, Type::FIXED);
	} else if (parameter.front() == '"' && parameter.back() == '"') {
		// constant

		// remove quotations
		parameter.erase(0);
		parameter.erase(parameter.size() - 1);

		if (regex_match(parameter, IDENT_REGEX)) {
			return DesignEntity(parameter, Type::FIXED);
		}

		return DesignEntity("", Type::INVALID);
	} else if (regex_match(parameter, IDENT_REGEX)
			&& query.declarations.find(parameter) != query.declarations.end()) {
		Type designEntity = query.declarations.find(parameter)->second;
		return DesignEntity(parameter, designEntity);
	} else if (parameter.find("_\"") == 0 
			&& parameter.find("\"_") == parameter.size() - 2) {
		parameter.erase(0, 2);
		parameter.erase(parameter.size() - 2, 2);

		if (regex_match(parameter, IDENT_REGEX)) {
			return DesignEntity(parameter, Type::FIXED);
		}

		return DesignEntity("", Type::INVALID);
	}

	return DesignEntity("", Type::INVALID);
}
