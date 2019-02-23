#pragma once

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <algorithm>

using namespace std;

#include "Parser.h"
#include "pkb.h"
#include "../SPA/Type.h"

static string varNameRegex = "([[:alpha:]]([[:alnum:]])*)";
static string constantRegex = "[[:digit:]]+";
static string spaceRegex = "[[:s:]]*";
static string openCurlyRegex = "\\{";

int Parser::parse(string fileName, PKB& p) {
	pkb = &p;
	loadFile(fileName);
	for (unsigned int i = 0; i < sourceCode.size(); i++) {
		int intent = getStatementIntent(sourceCode[i]);
		int result = 0;
		if (intent == KEY_PROCEDURE) {
			result = handleProcedure(sourceCode[i]);
		}
		else {
			if (!withinProcedure) {
				cout << "All statements should be contained within procedures. Error at line " << statementNumber << endl;
				return false;
			}
			else {
				if (expectElse && intent != KEY_ELSE) {
					cout << "Expected an else statement at line " << statementNumber << endl;
					result = -1;
				}
				else {
					if (intent == KEY_ASSIGN) {
						result = handleAssignment(sourceCode[i]);
						statementNumber++;
					}
					else if (intent == KEY_CLOSE_BRACKET) {
						result = handleCloseBracket(sourceCode[i]);
					}
					else if (intent == KEY_IF) {
						result = handleIf(sourceCode[i]);
						statementNumber++;
					}
					else if (intent == KEY_WHILE) {
						result = handleWhile(sourceCode[i]);
						statementNumber++;
					}
					else if (intent == KEY_PRINT) {
						result = handlePrint(sourceCode[i]);
						statementNumber++;
					}
					else if (intent == KEY_READ) {
						result = handleRead(sourceCode[i]);
						statementNumber++;
					}
					else if (intent == KEY_ELSE) {

					}
					else if (intent == KEY_CALL) {
						statementNumber++;
					}
					else {
						cout << "Statement of unknown type encountered at line " << statementNumber << endl;
						result = -1;
					}
				}
			}
		}
		//early termination if parsing fails at any point
		if (result != 0) {
			break;
		}
	}
	return 0;
}

int Parser::getStatementIntent(string line) {
	//check assignment first for potential variable names being keywords
	if (line.find("=", 0) != string::npos && line.find("<=") == string::npos && line.find("==") == string::npos
		&& line.find(">=") == string::npos && line.find("!=") == string::npos) {
		return KEY_ASSIGN;
	}
	//otherwise tokenise and find first token as keyword for statement.
	//tokenise to split on spaces and brackets that might appear
	vector<string> tokenLine = tokeniseString(line, " \t\n({");
	tokenLine[0] = leftTrim(tokenLine[0], " \t\n");
	tokenLine[0] = rightTrim(tokenLine[0], " \t\n");
	if (tokenLine[0] == "procedure") {
		return KEY_PROCEDURE;
	}
	if (tokenLine[0] == "while") {
		return KEY_WHILE;
	}
	if (tokenLine[0] == "if") {
		return KEY_IF;
	}
	if (tokenLine[0] == "read") {
		return KEY_READ;
	}
	if (tokenLine[0] == "print") {
		return KEY_PRINT;
	}
	if (tokenLine[0] == "else") {
		return KEY_ELSE;
	}
	if (tokenLine[0] == "while") {
		return KEY_WHILE;
	}
	if (tokenLine[0] == "if") {
		return KEY_IF;
	}
	if (tokenLine[0] == "else") {
		return KEY_ELSE;
	}
	if (tokenLine[0] == "call") {
		return KEY_CALL;
	}
	if (tokenLine[0] == "}") {
		return KEY_CLOSE_BRACKET;
	}
	return -1;
}

bool Parser::checkProcedure(string procLine) {
	//parse the procedure and check it is valid.
	string procedureRegexString = spaceRegex + "procedure" + spaceRegex + varNameRegex + spaceRegex + openCurlyRegex + spaceRegex;
	regex procedureRegex(procedureRegexString);
	if (!regex_match(procLine, procedureRegex)) {
		cout << "Procedure statement is invalid" << endl;
		return false;
	}
	return true;
}

int Parser::handleProcedure(string procLine) {
	if (!checkProcedure(procLine)) {
		return -1;
	}
	size_t startPos = procLine.find("procedure");
	size_t endPos = procLine.find_first_of("{");
	string procedureName = procLine.substr(startPos + 9, endPos - startPos - 9);
	procedureName = leftTrim(procedureName, " \t");
	procedureName = rightTrim(procedureName, " \t");

	pkb->insertProc(procedureName);
	currProcedure = procedureName;
	withinProcedure = true;
	emptyProcedure = true;
	return 0;
}

bool Parser::checkAssignment(string assignmentLine) {
	size_t equalPos = assignmentLine.find_first_of("=");
	string lhsLine = assignmentLine.substr(0, equalPos);
	string rhsLine = assignmentLine.substr(equalPos + 1, string::npos);
	if (!isValidVarName(lhsLine)) {
		cout << "Error in left hand side of assignment statement at line" << statementNumber << endl;
		return false;
	}
	if (!checkExpr(rhsLine)) {
		cout << "Error in right hand side of assignment statement at line " << statementNumber << endl;
		return false;
	}
	return true;
}

bool Parser::checkExpr(string expr) {
	//start from the back, start scanning for + or -. Track brackets and ignore + or - inside them.
	int bracketTracker = 0;
	for (unsigned int currPos = expr.length() - 1; currPos > 0; currPos--) {
		if (expr[currPos] == '(') {
			bracketTracker--;
		}
		else if (expr[currPos] == ')') {
			bracketTracker++;
		}
		if (bracketTracker == 0) {
			if (expr[currPos] == '+' || expr[currPos] == '-') {
				string firstExpr = expr.substr(0, currPos);
				string secondTerm = expr.substr(currPos + 1, string::npos);
				return checkExpr(firstExpr) & checkTerm(secondTerm);
			}
		}
		else if (bracketTracker < 0) {
			cout << "Unexpected ( bracket encountered in assignment statement at line " << statementNumber << endl;
			return false;
		}
	}
	//reach the end with no + or -, check for single term
	return checkTerm(expr);
}

bool Parser::checkTerm(string term) {
	//similar logic to checkExpr
	//start from the back, start scanning for *, /, %. Track brackets and ignore delimiters inside them.
	int bracketTracker = 0;
	for (unsigned int currPos = term.length() - 1; currPos > 0; currPos--) {
		if (term[currPos] == '(') {
			bracketTracker--;
		}
		else if (term[currPos] == ')') {
			bracketTracker++;
		}
		if (bracketTracker == 0) {
			if (term[currPos] == '*' || term[currPos] == '/' || term[currPos] == '%') {
				string firstTerm = term.substr(0, currPos);
				string secondFactor = term.substr(currPos + 1, string::npos);
				return checkExpr(firstTerm) & checkTerm(secondFactor);
			}
		}
		else if (bracketTracker < 0) {
			cout << "Unexpected ( bracket encountered in assignment statement at line " << statementNumber << endl;
			return false;
		}
	}
	//reach the end with no * / %, check for single term
	return checkFactor(term);
}

bool Parser::checkFactor(string factor) {
	if (isValidVarName(factor)) {
		return true;
	}
	if (isValidConstant(factor)) {
		return true;
	}
	//else try to search for matching ( ) bracket without anything else, and containing an expr
	size_t openBracketPos = factor.find_first_of("(");
	size_t closeBracketPos = factor.find_first_of(")");
	if (openBracketPos != string::npos && closeBracketPos != string::npos) {
		return checkExpr(factor.substr(openBracketPos+1, closeBracketPos - openBracketPos - 1));
	}
	cout << "Failed in parsing a Factor in RHS of assignment statement at line " << statementNumber << endl;
	return false;
}

int Parser::handleAssignment(string assignmentLine) {
	//clear spaces within the line to make parsing easier
	//also remove the semicolon since not needed to check grammar of the assign statement
	string cleanedAssignment = "";
	for (unsigned int i = 0; i < assignmentLine.length(); i++) {
		if (assignmentLine[i] != ' ' && assignmentLine[i] != '\t' && assignmentLine[i] != '\n' && assignmentLine[i] != ';') {
			cleanedAssignment += assignmentLine[i];
		}
	}
	if (!checkAssignment(cleanedAssignment)) {
		return -1;
	}
	
	setParent(statementNumber);

	//Separate variable names, constants and operation/brackets from each other to pass to pkb
	vector<string> assignTokens = vector<string>();
	string lhsVar = assignmentLine.substr(0, assignmentLine.find_first_of("="));
	string rhs = assignmentLine.substr(assignmentLine.find_first_of("=") + 1, string::npos);
	string currToken = "";
	for (unsigned int i = 0; i < rhs.length(); i++) {
		//assignment should have only alphanum and bracket/op. Less than 48 in ascii must be a bracket/op
		if (rhs[i] < 48) {
			if (!currToken.empty()) {
				assignTokens.push_back(currToken);
				currToken = "";
				assignTokens.push_back(rhs.substr(i, 1));
			}
			else {
				currToken += rhs[i];
			}
		}
	}
	if (!currToken.empty()) {
		assignTokens.push_back(currToken);
	}
	setModifies(statementNumber, lhsVar);
	for (unsigned int i = 0; i < assignTokens.size(); i++) {
		if (isValidVarName(assignTokens[i])) {
			pkb->insertVar(assignTokens[i]);
			setUses(statementNumber, assignTokens[i]);
		}
		else if (isValidConstant(assignTokens[i])) {
			pkb->insertConstant(stoi(assignTokens[i]));
		}
	}
	//pkb->insertAssignStmt(statementNumber, lhsVar, assignTokens);
	return 0;
}

bool Parser::checkRead(string readLine) {
	string readRegexString = spaceRegex + "read" + spaceRegex + varNameRegex + spaceRegex + ";" + spaceRegex;
	regex readRegex(readRegexString);
	if (!regex_match(readLine, readRegex)) {
		cout << "Read statement is invalid at line " << statementNumber << endl;
		return false;
	}
	return true;
}

int Parser::handleRead(string readLine) {
	if (!checkRead(readLine)) {
		return -1;
	}
	//extract variable name
	size_t startPos = readLine.find_first_of("read");
	size_t endPos = readLine.find_first_of(";");
	string varName = readLine.substr(startPos + 4, endPos - startPos - 4);
	varName = leftTrim(varName, " \t");
	varName = rightTrim(varName, " \t");

	setParent(statementNumber);
	setModifies(statementNumber, varName);
	pkb->insertVar(varName);
	
	emptyProcedure = false;
	return 0;
}

bool Parser::checkPrint(string printLine) {
	string printRegexString = spaceRegex + "print" + spaceRegex + varNameRegex + spaceRegex + ";" + spaceRegex;
	regex printRegex(printRegexString);
	if (!regex_match(printLine, printRegex)) {
		cout << "Print statement is invalid at line " << statementNumber << endl;
		return false;
	}
	return true;
}

int Parser::handlePrint(string printLine) {
	if (!checkPrint(printLine)) {
		return -1;
	}
	//extract variable name
	size_t startPos = printLine.find_first_of("read");
	size_t endPos = printLine.find_first_of(";");
	string varName = printLine.substr(startPos + 5, endPos - startPos - 5);
	varName = leftTrim(varName, " \t");
	varName = rightTrim(varName, " \t");

	setParent(statementNumber);
	setUses(statementNumber, varName);
	pkb->insertVar(varName);
	return 0;
}

bool Parser::checkWhile(string whileLine) {
	//find the brackets on the cond expr, check that no unexpected tokens are in the while line
	//then call functions to check validity of the cond expr
	size_t firstOpenBracket = whileLine.find_first_of("(");
	size_t lastCloseBracket = whileLine.find_last_of(")");
	string truncWhileLine = whileLine.substr(0, firstOpenBracket) + whileLine.substr(lastCloseBracket+1, string::npos);
	string condExprLine = whileLine.substr(firstOpenBracket, lastCloseBracket - firstOpenBracket + 1);
	string whileRegexString = spaceRegex + "while" + spaceRegex + openCurlyRegex + spaceRegex;
	regex whileRegex(whileRegexString);
	if (!regex_match(truncWhileLine, whileRegex)) {
		cout << "Unexpected tokens in the white statement at line " << statementNumber << endl;
		return false;
	}
	//clean spaces or tabs to make parsing easier
	string cleanedCondExpr = "";
	for (unsigned int i = 0; i < condExprLine.length(); i++) {
		if (condExprLine[i] != ' ' && condExprLine[i] != '\t') {
			cleanedCondExpr += condExprLine[i];
		}
	}
	return checkCondExpr(condExprLine);
}

int Parser::handleWhile(string whileLine) {
	if (!checkWhile(whileLine)) {
		return -1;
	}

	//set relationships
	setParent(statementNumber);
	setFollow(statementNumber);

	//update parent, container, follows trackers
	parentVector.push_back(statementNumber);
	containerTracker.push_back(WHILECONTAINER);
	allFollowStack.push_back(currentFollowVector);
	currentFollowVector.clear();

	//set uses relationships
	size_t openBracketPos = whileLine.find_first_of("(");
	size_t closeBracketPos = whileLine.find_last_of(")");
	string condExpr = whileLine.substr(openBracketPos + 1, closeBracketPos - openBracketPos - 1);
	vector<string> tokens = tokeniseString(condExpr, " \t&|()!");
	for (unsigned int i = 0; i < tokens.size(); i++) {
		if (isValidVarName(tokens[i])) {
			pkb->insertVar(tokens[i]);
			setUses(statementNumber, tokens[i]);
		}
		else if (isValidConstant(tokens[i])) {
			pkb->insertConstant(stoi(tokens[i]));
		}
	}
	return 0;
}

bool Parser::checkIf(string ifLine) {
	//check for brackets, then check the cond_expr
	//practically the same as while
	size_t firstOpenBracket = ifLine.find_first_of("(");
	size_t lastCloseBracket = ifLine.find_last_of(")");
	string truncIfLine = ifLine.substr(0, firstOpenBracket) + ifLine.substr(lastCloseBracket+1, string::npos);
	string condExprLine = ifLine.substr(firstOpenBracket, lastCloseBracket - firstOpenBracket + 1);
	string ifRegexString = spaceRegex + "if" + spaceRegex + "then" + spaceRegex + openCurlyRegex + spaceRegex;
	regex ifRegex(ifRegexString);
	if (!regex_match(truncIfLine, ifRegex)) {
		cout << "Unexpected tokens in the white statement at line " << statementNumber << endl;
		return false;
	}
	return checkCondExpr(condExprLine);
	return false;
}

int Parser::handleIf(string ifLine) {
	if (!checkIf(ifLine)) {
		return -1;
	}
	//set follow, parent relationships
	setParent(statementNumber);
	setFollow(statementNumber);

	//update parent, follow, container trackers
	parentVector.push_back(statementNumber);
	containerTracker.push_back(IFCONTAINER);
	allFollowStack.push_back(currentFollowVector);
	currentFollowVector.clear();

	//set uses relationships
	size_t openBracketPos = ifLine.find_first_of("(");
	size_t closeBracketPos = ifLine.find_last_of(")");
	string condExpr = ifLine.substr(openBracketPos + 1, closeBracketPos - openBracketPos - 1);
	vector<string> tokens = tokeniseString(condExpr, " \t&|()!");
	for (unsigned int i = 0; i < tokens.size(); i++) {
		if (isValidVarName(tokens[i])) {
			pkb->insertVar(tokens[i]);
			setUses(statementNumber, tokens[i]);
		}
		else if (isValidConstant(tokens[i])) {
			pkb->insertConstant(stoi(tokens[i]));
		}
	}
	return 0;
}

bool Parser::checkElse(string elseLine) {
	string elseRegexString = spaceRegex + "else" + spaceRegex + openCurlyRegex;
	regex elseRegex(elseRegexString);
	if (!regex_match(elseLine, elseRegex)) {
		cout << "Else statement has unexpected tokens just before line " << statementNumber << endl;
		return false;
	}
	return true;
}

int Parser::handleElse(string elseLine) {
	if (!checkElse(elseLine)) {
		return -1;
	}
	//reset booleans
	//reset follow tracker
	//set container tracker
	expectElse = false;
	containerTracker.push_back(ELSECONTAINER);
	currentFollowVector.clear();
	return 0;
}

bool Parser::checkCondExpr(string condExpr) {
	//check that brackets wrap the expression, without unexpected tokens
	if (condExpr.find_first_of("(") != 0 || condExpr.find_last_of(")") != (condExpr.length() - 1)) {
		cout << "Found unexpected token when expecting ( and ) around conditional expression at line " << statementNumber << endl;
		return false;
	}
	/*removing external brackets, look for an expected ! operator
	or if finding brackets, track in stack until we exit the brackets, and look for && or ||
	or if something else found, it should imply a rel expr */
	int bracketCount = 0;
	bool startOfExpr = true;
	bool checkForAndOr = false;
	bool noBrackets = true;
	for (unsigned int pos = 1; pos < condExpr.length() - 1; pos++) {
		if (startOfExpr) {
			if (condExpr[pos] == '!') {
				//extract out the internal expr
				string nextCondExpr = condExpr.substr(pos + 1, condExpr.length() - 2);
				return checkCondExpr(nextCondExpr);
			}
			else if (condExpr[pos] == '(') {
				startOfExpr = false;
				noBrackets = false;
				bracketCount++;
			}
			else {
				//else no delimiter expected, assume it is a rel expr and strip brackets
				string relExpr = condExpr.substr(1, condExpr.length() - 2);
				return checkRelExpr(relExpr);
			}
		}
		else if (checkForAndOr) {
			if (condExpr.substr(pos, 2) == "&&" || condExpr.substr(pos, 2) == "||") {
				//extract out the 2 expressions
				string firstCondExpr = condExpr.substr(0, pos);
				string secondCondExpr = condExpr.substr(pos + 2, string::npos);
				return checkCondExpr(firstCondExpr) & checkCondExpr(secondCondExpr);
			}
			else {
				cout << "Could not successfully parse conditional expression, missing and/or operator at line " << statementNumber << endl;
				return false;
			}
		}
		else {
			if (condExpr[pos] == '(') {
				bracketCount++;
			}
			else if (condExpr[pos] == ')') {
				bracketCount--;
				if (bracketCount == 0) {
					checkForAndOr = true;
				}
			}
		}
	}
	if (bracketCount > 0) {
		cout << "Mismatch in number of ( and ) brackets in conditional expression at line " << statementNumber << endl;
		return false;
	}
	cout << "Could not successfully parse the conditional expression at line " << statementNumber << endl;
	return false;
}																

bool Parser::checkRelExpr(string relExpr) {
	//should have only 1 of the relational operators
	//look for 2 character ones, then 1 character due to overlap in < and >
	size_t relOpPos = relExpr.find("==");
	relOpPos = min(relOpPos, relExpr.find("<="));
	relOpPos = min(relOpPos, relExpr.find(">="));
	relOpPos = min(relOpPos, relExpr.find("!="));
	int offset = 2;
	if (relOpPos == string::npos) {
		offset = 1;
		relOpPos = min(relOpPos, relExpr.find("<"));
		relOpPos = min(relOpPos, relExpr.find(">"));
	}
	if (relOpPos != string::npos) {
		string firstRelFactor = relExpr.substr(0, relOpPos);
		string secondRelFactor = relExpr.substr(relOpPos + offset, string::npos);
		firstRelFactor = leftTrim(rightTrim(firstRelFactor, " \t\r"), " \t");
		secondRelFactor = leftTrim(rightTrim(secondRelFactor, " \t\r"), " \t");
		return checkRelFactor(firstRelFactor) & checkRelFactor(secondRelFactor);
	}
	cout << "Could not find a relational operator at line " << statementNumber << endl;
	return false;
}

bool Parser::checkRelFactor(string relFactor) {
	relFactor = leftTrim(relFactor, " \t");
	relFactor = rightTrim(relFactor, " \t\n");
	if (isValidVarName(relFactor)) {
		return true;
	}
	if (isValidConstant(relFactor)) {
		return true;
	}
	return checkExpr(relFactor);
}

int Parser::handleCloseBracket(string closeBracket) {
	//if no container statements tracked, assume close bracket is for procedure
	if (containerTracker.size() < 1) {
		if (emptyProcedure) {
			cout << "A procedure cannot be empty" << endl;
			return -1;
		}
		withinProcedure = false;
		emptyProcedure = true;
		return 0;
	}
	else {
		//pop from parent stack, pop from stack of follow vectors for while, else
		//clear follow vector
		//set else checker and container for if
		currentFollowVector.clear();
		if (containerTracker.back() == WHILECONTAINER || containerTracker.back() == ELSECONTAINER) {
			if (parentVector.size() == 0) {
				cout << "Unexpected error when parsing end of container statement. Parent vector is empty. At line " << statementNumber << endl;
				return -1;
			}
			if (allFollowStack.size() == 0) {
				cout << "Unexpected error when parsing end of container statement. Follow stack is empty. At line " << statementNumber << endl;
				return -1;
			}
			parentVector.pop_back();
			currentFollowVector = allFollowStack.back();
			allFollowStack.pop_back();
		}
		else if (containerTracker.back() == IFCONTAINER) {
			expectElse = true;
		}
		containerTracker.pop_back();
	}
	return 0;
}

vector<string> Parser::loadFile(string fileName) {
	ifstream sourceFile;
	sourceFile.open(fileName);
	string currLine;
	string allSourceCode = "";
	while (getline(sourceFile, currLine)) {
		allSourceCode += currLine;
	}
	allSourceCode = rightTrim(allSourceCode, " \t\n");
	while (!allSourceCode.empty()) {
		allSourceCode = leftTrim(allSourceCode, " \t\n");
		/*Break up into lines in 3 cases
		First is semicolon, second is open brackets, third is close brackets */
		size_t delimitPos = allSourceCode.find_first_of(";{}");
		while (delimitPos != string::npos) {
			string currStatement = allSourceCode.substr(0, delimitPos+1);
			sourceCode.push_back(currStatement);
			allSourceCode = allSourceCode.substr(delimitPos + 1, string::npos);
			delimitPos = allSourceCode.find_first_of(";{}");
		}
		if (!allSourceCode.empty()) {
			sourceCode.push_back(allSourceCode);
		}
	}
	return sourceCode;
}

bool Parser::isValidVarName(string line) {
	string varNameRegexString = spaceRegex + varNameRegex + spaceRegex;
	regex varRegex(varNameRegexString);
	if (!regex_match(line, varRegex)) {
		return false;
	}
	return true;
}

bool Parser::isValidConstant(string line) {
	string constantRegexString = spaceRegex + constantRegex + spaceRegex;
	regex constRegex(constantRegexString);
	if (!regex_match(line, constRegex)) {
		return false;
	}
	return true;
}

string Parser::leftTrim(string line, string targetChar) {
	size_t startpos = line.find_first_not_of(targetChar);
	if (string::npos != startpos) {
		line = line.substr(startpos);
	}
	return line;
}

string Parser::rightTrim(string line, string targetChar) {
	size_t endpos = line.find_last_not_of(targetChar);
	if (string::npos != endpos) {
		line = line.substr(0, endpos + 1);
	}
	return line;
}

vector<string> Parser::tokeniseString(string toTokenise, string delimiters) {
	vector<string> tokenList = vector<string>();
	string currToken;
	while (!toTokenise.empty()) {
		size_t delimiterPos = toTokenise.find_first_of(delimiters);
		string token = toTokenise.substr(0, delimiterPos);
		if (delimiterPos == string::npos) {
			toTokenise = "";
		}
		else {
			toTokenise = toTokenise.substr(delimiterPos + 1, string::npos);
		}
		if (!token.empty()) {
			tokenList.push_back(token);
		}
	}
	return tokenList;
}

bool Parser::setParent(int currStatementNum) {
	for (unsigned int i = 0; i < parentVector.size(); i++) {
		pkb->setParentT(parentVector[i], currStatementNum);
	}
	if (parentVector.size() > 0) {
		pkb->setParent(parentVector.back(), currStatementNum);
	}
	return true;
}

bool Parser::setFollow(int currStatementNum) {
	for (unsigned int i = 0; i < currentFollowVector.size(); i++) {
		pkb->setFollowedByT(currentFollowVector[i], currStatementNum);
	}
	if (currentFollowVector.size() > 0) {
		pkb->setFollowedBy(currentFollowVector.back(), currStatementNum);
	}
	return true;
}

bool Parser::setModifies(int currStatementNum, string varName) {
	for (unsigned int i = 0; i < parentVector.size(); i++) {
		pkb->setModifies(parentVector[i], varName);
	}
	pkb->setModifies(currStatementNum, varName);
	return true;
}

bool Parser::setUses(int currStatementNum, string varName) {
	for (unsigned int i = 0; i < parentVector.size(); i++) {
		pkb->setUses(parentVector[i], varName);
	}
	pkb->setUses(currStatementNum, varName);
	return true;
}

void Parser::setWithinProcedure(bool setting) {
	withinProcedure = setting;
}