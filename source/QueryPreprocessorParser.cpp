#include <regex>
#include <unordered_set>
#include <iostream>
#include "DesignEntity.h"
#include "QueryPreprocessorHelper.h"
#include "QueryPreprocessorDeclareParser.h"
#include "QueryPreprocessorSelectParser.h"
#include "QueryPreprocessorParser.h"

// Initializes a newly created QueryPreprocessorParser.
QueryPreprocessorParser::QueryPreprocessorParser(const std::vector<std::string>& statements)
	: STATEMENTS(statements) {
	query = ProcessedQuery();
}

// Parses the statements.
bool QueryPreprocessorParser::parse() {
	size_t numberOfDeclarativeStatements = STATEMENTS.size() - 1;

	if (numberOfDeclarativeStatements == 0) {
		return false;
	}

	for (size_t index = 0; index < numberOfDeclarativeStatements; index++) {
		QueryPreprocessorDeclareParser declareParser(STATEMENTS[index], query);
		bool status = declareParser.parse();
		if (!status) {
			return false;
		}
	}

	QueryPreprocessorSelectParser selectParser(STATEMENTS.back(), query);
	return selectParser.parse();
}

// Returns the processed query.
ProcessedQuery QueryPreprocessorParser::getQuery() {
	return query;
}
