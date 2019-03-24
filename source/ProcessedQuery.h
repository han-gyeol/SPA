#pragma once

#include <unordered_map>
#include <unordered_set>
#include "Clause.h"
#include "PatternClause.h"
#include "SuchThatClause.h"

class ProcessedQuery {
public:
	std::unordered_map<std::string, Type> declarations;
	std::vector<DesignEntity> resultClElemList;

	std::unordered_set<Clause*> clauses;
	std::unordered_set<Clause*> withClauses;

	ProcessedQuery();

	bool insertDeclaration(const std::string& synonym,
		const Type& designEntityType);

	void addResultClElement(const DesignEntity& element);
	void addClause(Clause* clause);
	void addWithClause(Clause* withClause);
};
