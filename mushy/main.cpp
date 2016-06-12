//
//  main.cpp
//  mushy
//
//  Created by Uli Kusterer on 08/06/16.
//  Copyright © 2016 Uli Kusterer. All rights reserved.
//

#include <iostream>
#include <functional>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>


using namespace std;


class state	whitespace_state( char currCh, class info& ioInfo );
class state	identifier_state( char currCh, class info& ioInfo );
class state	string_state( char currCh, class info& ioInfo );
class state	character_state( char currCh, class info& ioInfo );
class state	multi_line_comment_state( char currCh, class info& ioInfo );

void		parse_function_body( vector<class token>& tokens, vector<class token>::iterator& currToken, class program& theProgram, class classdesc& currClass, class funcdesc& currFunction );
class term	parse_expression( vector<class token>& tokens, vector<class token>::iterator& currToken, class program& theProgram, class classdesc& currClass, class funcdesc& currFunction );


#define PE_TOKEN_NAME	token_text(tokens,currToken)
#define PE_ERROR(...)	do { parse_error	err;\
		err.err_msg << __VA_ARGS__;\
		err.offset = token_offset(tokens,currToken);\
		err.line = token_line(tokens,currToken);\
		throw err; } while(0)


static const char*	indent( size_t indentLevel )
{
	const char*	tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	const char* outTabs = tabs +(strlen(tabs) -indentLevel);
	return outTabs;
}


class parse_error : public exception
{
public:
	parse_error() : offset(0), line(0) {}
	
    virtual const char* what() const _NOEXCEPT { return err_msg.str().c_str(); };
	
	stringstream	err_msg;
	size_t			offset;
	size_t			line;
};


class token
{
public:
	typedef enum {
		whitespace,
		identifier,
		operator_identifier,
		quoted_string,
		character,
		number,
		integer
	} token_kind;
	
	token() : kind(whitespace), offset(0), lineNumber(0) {  }
	
	token_kind		kind;
	std::string		text;
	size_t			offset;
	size_t			lineNumber;
};


string	token_text( const vector<token>& tokens, const vector<token>::iterator& tok )
{
	if( tok == tokens.end() )
		return "<end of file>";
	else
		return tok->text;
}

size_t	token_offset( const vector<token>& tokens, const vector<token>::iterator& tok )
{
	if( tok == tokens.end() )
		return 0;
	else
		return tok->offset;
}


size_t	token_line( const vector<token>& tokens, const vector<token>::iterator& tok )
{
	if( tok == tokens.end() )
		return 0;
	else
		return tok->lineNumber;
}


class state
{
public:
	state( state (*inFuncPtr)( char, class info& ) = nullptr ) : mFunc(inFuncPtr) {}

	state	operator()( char currCh, class info& ioInfo )	{ return mFunc( currCh, ioInfo ); };
	
	explicit operator bool()	{ return mFunc != nullptr; }
	
protected:
	state (*mFunc)( char, class info& );
};


class info
{
public:
	info( istream& inStream ) : stream(inStream), lineNumber(1) {}
	
	token			curr_token;
	vector<token>	tokens;
	istream&		stream;
	size_t			lineNumber;
};


class vardesc;
class functypedesc;
class funcdesc;


class varcontainer
{
public:
	virtual ~varcontainer() {}
	
	virtual void	print( size_t indentLevel ) const;
	
	map<string,vardesc>			variables;		// Variables that have been defined.
};


class varfunccontainer : public varcontainer
{
public:
	virtual ~varfunccontainer() {}
	
	virtual void	print( size_t indentLevel ) const;
	
	map<string,functypedesc>	function_types;	// Forward-declared functions.
	map<string,funcdesc>		functions;		// Function definitions.
};


class typedesc : public varfunccontainer
{
public:
	typedesc( string inName = "" ) : type_name(inName) {}
	typedesc( const typedesc& inOriginal ) : type_name(inOriginal.type_name), union_name(inOriginal.union_name), template_arguments(inOriginal.template_arguments), class_name(inOriginal.class_name), class_template_arguments(inOriginal.class_template_arguments) { variables = inOriginal.variables; function_types = inOriginal.function_types; functions = inOriginal.functions; }
	
	virtual void	print( size_t indentLevel ) const override;
	
	string				type_name;					// Name of this type.
	string				union_name;					// Name of the union this type belongs to.
	vector<typedesc>	template_arguments;			// Types for all template arguments.
	string				class_name;					// Name of the base class for this type.
	vector<typedesc>	class_template_arguments;	// Types for all template arguments to the base class.
};


void	typedesc::print( size_t indentLevel ) const
{
	cout << indent(indentLevel) << type_name;
	if( template_arguments.size() > 0 )
	{
		cout << "<";
		bool	isFirst = true;
		for( auto currArg : template_arguments )
		{
			if( !isFirst )
				cout << ", ";
			else
				isFirst = false;
			currArg.print(0);
		}
		cout << ">";
	}
	
	if( class_name.length() > 0 )
	{
		cout << " : " << class_name;
		if( class_template_arguments.size() > 0 )
		{
			cout << "<";
			bool	isFirst = true;
			for( auto currArg : class_template_arguments )
			{
				if( !isFirst )
					cout << ", ";
				else
					isFirst = false;
				currArg.print(indentLevel);
			}
			cout << ">";
		}
	}
	if( union_name.size() > 0 )
		cout << " @" << union_name;
	if( variables.size() > 0 || functions.size() > 0 || function_types.size() > 0 )
		cout << endl;
	varfunccontainer::print( indentLevel +1 );
}


class classdesc : public typedesc
{
public:
	classdesc( string inName = "" ) : typedesc(inName) {}
	
	
};


class vardesc : public typedesc
{
public:
	vardesc( string inName, const typedesc& inType ) : typedesc(inType), var_name(inName) {}
	vardesc() : typedesc("") {}
	
	virtual void	print( size_t indentLevel ) const override;
	
	string	var_name;
};


void	vardesc::print( size_t indentLevel ) const
{
	cout << indent( indentLevel );
	typedesc::print(0);
	cout << "\t" << var_name;
}


class term
{
public:
	typedef enum {
		quoted_string,
		character,
		integer,
		number,
		function_call,
		global_variable,
		variable,
		instance_variable,
		parameter
	} term_type;

	term( std::string inName = "" ) : func_name(inName), kind(function_call) {}
	
	void	print( size_t indentLevel ) const
	{
		switch( kind )
		{
			case quoted_string:
				cout << indent(indentLevel) << "\"" << func_name << "\"";
				break;
			case character:
				cout << indent(indentLevel) << "'" << func_name << "'";
				break;
			case integer:
				cout << indent(indentLevel) << func_name;
				break;
			case number:
				cout << indent(indentLevel) << func_name;
				break;
			case function_call:
			{
				cout << indent(indentLevel) << func_name << "( ";
				bool	isFirst = true;
				for( auto currParam : parameters )
				{
					if( !isFirst )
						cout << ", ";
					else
						isFirst = false;
					currParam.print(0);
				}
				cout << " )";
				break;
			}
			case variable:
				cout << indent(indentLevel) << func_name;
				break;
			case global_variable:
				cout << indent(indentLevel) << "@global(" << func_name << ")";
				break;
			case instance_variable:
				cout << indent(indentLevel) << "this." << func_name;
				break;
			case parameter:
				cout << indent(indentLevel) << "@parameter(" << func_name << ")";
				break;
		}
	}
	
	term_type			kind;
	std::string			func_name;
	vector<term>		parameters;
};


class functypedesc
{
public:
	functypedesc( string inName = "" ) : func_name(inName) {}
	virtual ~functypedesc() {}
	
	virtual void	print( size_t indentLevel ) const
	{
		cout << indent(indentLevel);
		return_type.print(0);
		cout << "\t" << func_name << "( ";
		for( auto currParam : param_types )
		{
			currParam.print(0);
		}
		cout << " )" << endl;
	}
	
	string				func_name;
	vector<vardesc>		param_types;
	typedesc			return_type;
};


class funcdesc : public functypedesc, public varcontainer
{
public:
	funcdesc( string inName = "" ) : functypedesc(inName), is_pure_virtual(false), is_override(false) {}
	
	virtual void	print( size_t indentLevel ) const override
	{
		functypedesc::print( indentLevel );
		varcontainer::print( indentLevel );
		if( commands.size() > 0 )
		{
			cout << indent(indentLevel) << "COMMANDS:" << endl;
			for( auto currCmd : commands )
			{
				currCmd.print( indentLevel +1 );
				cout << endl;
			}
		}
	}

	vector<term>	commands;
	
	bool	is_pure_virtual;
	bool	is_override;
};


class program : public varfunccontainer
{
public:
	program();
	
	virtual void	print( size_t indentLevel ) const override;
	
	map<string,typedesc>		types;			// Forward-declared types.
	map<string,classdesc>		classes;		// Class definitions.
	map<string,size_t>			binary_operator_priorities;
};


void	program::print( size_t indentLevel ) const
{
	varfunccontainer::print( indentLevel );
	cout << "TYPES:" << endl;
	for( auto currType : types )
	{
		currType.second.print( indentLevel +1 );
		cout << endl;
	}
	cout << "CLASSES:" << endl;
	for( const pair<string,classdesc>& currClass : classes )
	{
		currClass.second.print( indentLevel +1 );
	}
}


void	varcontainer::print( size_t indentLevel ) const
{
	if( variables.size() > 0 )
	{
		cout << indent(indentLevel) << "VARIABLES:" << endl;
		for( auto currVar : variables )
		{
			currVar.second.print(indentLevel +1);
			cout << endl;
		}
	}
}


void	varfunccontainer::print( size_t indentLevel ) const
{
	if( variables.size() > 0 || functions.size() > 0 || function_types.size() > 0 )
	{
		varcontainer::print( indentLevel );
		cout << indent(indentLevel) << "FUNCTIONS:" << endl;
		for( const std::pair<string,funcdesc>& currVar : functions )
		{
			currVar.second.print(indentLevel +1);
		}
		cout << indent(indentLevel) << "FUNCTION DECLARATIONS:" << endl;
		for( auto currVar : function_types )
		{
			currVar.second.print(indentLevel +1);
		}
	}
}


program::program()
{
	types["bool"] = typedesc("bool");
	types["int32_t"] = typedesc("int32_t");
	types["uint32_t"] = typedesc("uint32_t");
	types["int16_t"] = typedesc("int16_t");
	types["uint16_t"] = typedesc("uint16_t");
	types["int8_t"] = typedesc("int8_t");
	types["uint8_t"] = typedesc("uint8_t");
	types["void"] = typedesc("void");
	
	binary_operator_priorities["="] = 1000;
	binary_operator_priorities["+"] = 5000;
	binary_operator_priorities["-"] = 5000;
	binary_operator_priorities["*"] = 6000;
	binary_operator_priorities["/"] = 6000;
	binary_operator_priorities["%"] = 6000;
}

void	finish_token( info& ioInfo )
{
	if( ioInfo.curr_token.kind != token::quoted_string && ioInfo.curr_token.kind != token::character
		&& ioInfo.curr_token.text.length() == 0 )
		return;
	
	if( ioInfo.curr_token.kind != token::whitespace )
	{
		ioInfo.curr_token.lineNumber = ioInfo.lineNumber;
		ioInfo.tokens.push_back( ioInfo.curr_token );
		ioInfo.curr_token.text.erase();
		ioInfo.curr_token.kind = token::whitespace;
	}
}


bool	is_operator( char currCh )
{
	switch( currCh )
	{
		case '[':
		case ']':
		case '{':
		case '}':
		case '(':
		case ')':
		case '.':
		case ',':
		case ':':
		case ';':
		case '<':
		case '>':
		case '!':
		case '%':
		case '^':
		case '&':
		case '*':
		case '-':
		case '+':
		case '/':
		case '=':
		case '?':
			return true;
			break;
		
		default:
			return false;
	}
	
	return false;
}


char	decode_escape( char escapeChar )
{
	switch( escapeChar )
	{
		case 'r':
			return '\r';
		case 'n':
			return '\n';
		case 't':
			return '\t';
		case '"':
			return '"';
		case '\'':
			return '\'';
		case '\\':
			return '\\';
	}
	
	return ' ';
}


state	string_state( char currCh, class info& ioInfo )
{
	switch( currCh )
	{
		case '"':
			finish_token( ioInfo );
			return whitespace_state;
			break;
		
		case '\\':
		{
			char	escapedChar = 0;
			ioInfo.stream.get( escapedChar );
			escapedChar = decode_escape(escapedChar);
			ioInfo.curr_token.text.append( 1, currCh );
			break;
		}
		
		default:
			ioInfo.curr_token.text.append( 1, currCh );
			break;
	}
	
	return string_state;
}


state	character_state( char currCh, class info& ioInfo )
{
	switch( currCh )
	{
		case '\'':
			finish_token( ioInfo );
			return whitespace_state;
			break;
		
		case '\\':
		{
			char	escapedChar = 0;
			ioInfo.stream.get( escapedChar );
			escapedChar = decode_escape(escapedChar);
			ioInfo.curr_token.text.append( 1, currCh );
			break;
		}
		
		default:
			ioInfo.curr_token.text.append( 1, currCh );
			break;
	}
	
	return character_state;
}


state	identifier_state( char currCh, class info& ioInfo )
{
	switch( currCh )
	{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			finish_token( ioInfo );
			return whitespace_state;
			break;
		
		case '"':
			finish_token( ioInfo );
			ioInfo.curr_token.kind = token::quoted_string;
			return string_state;
			break;

		case '\'':
			finish_token( ioInfo );
			ioInfo.curr_token.kind = token::character;
			return character_state;
			break;
		
		default:
			if( is_operator(currCh) )
			{
				finish_token( ioInfo );
				ioInfo.curr_token.kind = token::operator_identifier;
				ioInfo.curr_token.text.append( 1, currCh );
				finish_token( ioInfo );
				ioInfo.curr_token.kind = token::identifier;
			}
			else
				ioInfo.curr_token.text.append( 1, currCh );
			break;
	}
	
	return identifier_state;
}


state	single_line_comment_state( char currCh, class info& ioInfo )
{
	if( currCh == '\r' || currCh == '\n' )
	{
		finish_token( ioInfo );
		return whitespace_state;
	}
	else
		return single_line_comment_state;
}


state	possible_multi_line_comment_end_state( char currCh, class info& ioInfo )
{
	if( currCh == '/' )
		return whitespace_state;
	else if( currCh == '*' )
		return possible_multi_line_comment_end_state;
	else
		return multi_line_comment_state;
}


state	multi_line_comment_state( char currCh, class info& ioInfo )
{
	if( currCh == '*' )
		return possible_multi_line_comment_end_state;
	else
		return single_line_comment_state;
}


state	possible_comment_state( char currCh, class info& ioInfo )
{
	if( currCh == '/' )	// Single-line comment!
	{
		return single_line_comment_state;
	}
	else if( currCh == '*' )	// Multi-line comment!
	{
		return multi_line_comment_state;
	}
	else
	{
		finish_token( ioInfo );
		ioInfo.curr_token.kind = token::identifier;
		ioInfo.curr_token.text.append( 1, '/' );
		finish_token( ioInfo );
		return whitespace_state( currCh, ioInfo );
	}
}


state	whitespace_state( char currCh, class info& ioInfo )
{
	switch( currCh )
	{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			break;
		
		case '"':
			finish_token( ioInfo );
			ioInfo.curr_token.kind = token::quoted_string;
			return string_state;
			break;

		case '\'':
			finish_token( ioInfo );
			ioInfo.curr_token.kind = token::character;
			return character_state;
			break;
		
		case '/':
			return possible_comment_state;
			break;
		
		default:
			if( is_operator(currCh) )
			{
				finish_token( ioInfo );
				ioInfo.curr_token.kind = token::operator_identifier;
				ioInfo.curr_token.text.append( 1, currCh );
				finish_token( ioInfo );
				ioInfo.curr_token.kind = token::identifier;
			}
			else
			{
				finish_token( ioInfo );
				ioInfo.curr_token.kind = token::identifier;
				return identifier_state( currCh, ioInfo );
			}
			break;
	}
	
	return whitespace_state;
}


vector<token>	tokenize( istream& inStream )
{
	info			currInfo( inStream );
	state			currState = whitespace_state;
	bool			justHadCR = false;
	
	while( true )
	{
		char	currCh = 0;
		inStream.get( currCh );
		if( currCh == '\0' || inStream.eof() )
			break;
		if( currCh == '\r' )
		{
			currInfo.lineNumber++;
			justHadCR = true;
		}
		else if( currCh == '\n' && !justHadCR )
			currInfo.lineNumber++;
		else
			justHadCR = false;
		currState = currState( currCh, currInfo );
		currInfo.curr_token.offset++;
	}
	finish_token( currInfo );
	
	return currInfo.tokens;
}


size_t	priority_for_binary_operator( const program& program, const string& opName )
{
	auto foundOperator = program.binary_operator_priorities.find( opName );
	
	return( (foundOperator == program.binary_operator_priorities.end()) ? 0 : foundOperator->second );
}


typedesc	parse_type( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram )
{
	typedesc	theType;
	bool		nothingYet = true;
	
	if( currToken->kind == token::identifier && currToken->text.compare("unsigned") == 0 )
	{
		theType.type_name.append( currToken->text );
		
		currToken++;
		
		nothingYet = false;
	}
	
	if( currToken->kind == token::identifier && currToken->text.compare("long") == 0 )
	{
		theType.type_name.append( currToken->text );
		
		currToken++;
		
		nothingYet = false;
		
		if( currToken->kind == token::identifier && currToken->text.compare("long") == 0 )
		{
			theType.type_name.append( 1, ' ' );
			theType.type_name.append( currToken->text );
			
			currToken++;
		
			nothingYet = false;
		}
	}
	else if( currToken->kind == token::identifier && currToken->text.compare("short") == 0 )
	{
		theType.type_name.append( currToken->text );
		
		currToken++;
		
		nothingYet = false;
	}

	if( currToken->kind == token::identifier && currToken->text.compare("int") == 0 )
	{
		theType.type_name.append( currToken->text );
		
		currToken++;
		
		nothingYet = false;
	}
	else if( currToken->kind == token::identifier && currToken->text.compare("char") == 0 )
	{
		theType.type_name.append( currToken->text );
		
		currToken++;
		
		nothingYet = false;
	}
	
	if( nothingYet && currToken->kind == token::identifier )
	{
		map<string,typedesc>::iterator itty = theProgram.types.find( currToken->text );
		if( itty != theProgram.types.end() )
		{
			theType = itty->second;
			currToken++;
		}
	}
	else if( nothingYet )
	{
		PE_ERROR( "Expected type here, found " << PE_TOKEN_NAME);
	}
	
	if( !nothingYet && currToken != tokens.end() && currToken->kind == token::operator_identifier && currToken->text.compare("<") == 0 )
	{
		currToken++;
		
		while( true )
		{
			typedesc currTemplateType = parse_type( tokens, currToken, theProgram );
			if( currTemplateType.type_name.length() > 0 )
			{
				if( currToken == tokens.end() )
					PE_ERROR( "Expected '>' here, found " << PE_TOKEN_NAME);
				else if( currToken->kind == token::operator_identifier && currToken->text.compare(">") == 0 )
				{
					currToken++;
					break;
				}
				else if( currToken->kind != token::operator_identifier || currToken->text.compare(",") != 0 )
					currToken++;
			}
			theType.template_arguments.push_back(currTemplateType);
		}
	}
	
	return theType;
}


void	parse_function_parameters( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram, funcdesc& currFunction )
{
	if( currToken == tokens.end() || (currToken->kind == token::operator_identifier && currToken->text.compare(")") == 0) )
		return;
	
	while( true )
	{
		vardesc theVar( "", parse_type( tokens, currToken, theProgram ) );
		if( theVar.type_name.length() == 0 )
			PE_ERROR( "Expected parameter type here, found " << PE_TOKEN_NAME);
		
		if( currToken == tokens.end() || currToken->kind != token::identifier )
			PE_ERROR( "Expected parameter name after" << theVar.type_name << ", found " << PE_TOKEN_NAME);
		
		theVar.var_name = currToken->text;
		currFunction.param_types.push_back( theVar );
		currToken++;

		if( currToken == tokens.end() || currToken->kind != token::operator_identifier )
			PE_ERROR("Expected ',' or ')' here, found " << PE_TOKEN_NAME);
		
		if( currToken->text.compare(")") == 0 )	// End of list.
			break;
		else if( currToken->text.compare(",") == 0 )	// Another param follows.
			currToken++;
		else
			PE_ERROR("Expected ',' or ')' here, found " << PE_TOKEN_NAME);
	}
}

void	parse_var_or_function( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram, varfunccontainer& container, classdesc& currClass, bool isOverride )
{
	typedesc	theType = parse_type( tokens, currToken,  theProgram );
	
	if( currToken == tokens.end() || currToken->kind != token::identifier )
		PE_ERROR("Expected identifier after " << theType.type_name << ", found " << PE_TOKEN_NAME);
	
	string	thingName = currToken->text;
	currToken++;
	
	if( currToken == tokens.end() || currToken->kind != token::operator_identifier )
		PE_ERROR( "Expected semicolon after variable name, or opening bracket after function name, found " << PE_TOKEN_NAME );
	
	if( !isOverride && currToken->text.compare(";") == 0 )	// Variable!
	{
		if( container.variables.find(thingName) != container.variables.end() )
			PE_ERROR( "A variable named " << thingName << "already exists" );
		container.variables[thingName] = vardesc(thingName, theType);
		currToken++;
	}
	else if( currToken->text.compare("(") == 0 )	// Function!
	{
		currToken++;
		
		funcdesc	newFunction( thingName );
		newFunction.return_type = theType;
		newFunction.is_override = isOverride;
		
		parse_function_parameters( tokens, currToken, theProgram, newFunction );
		
		if( currToken == tokens.end() || currToken->kind != token::operator_identifier || currToken->text.compare(")") != 0 )
			PE_ERROR( "Expected ')' at end of function parameter list, found " << PE_TOKEN_NAME );
		currToken++;
		
		if( currToken == tokens.end() || currToken->kind != token::operator_identifier )
			PE_ERROR( "Expected ';' or '{' after function parameter list, found " << PE_TOKEN_NAME );
		if( currToken->text.compare(";") == 0 )
		{
			container.function_types[thingName] = newFunction;
			currToken ++;
		}
		else if( currToken->text.compare("=") == 0 )
		{	// pure virtual declaration:
			currToken++;
			
			if( currToken == tokens.end() || currToken->kind != token::identifier || currToken->text.compare("null") != 0 )
				PE_ERROR( "Expected 'null' following a '=' after a function declaration, found " << PE_TOKEN_NAME );

			currToken++;
			
			if( currToken == tokens.end() || currToken->kind != token::operator_identifier || currToken->text.compare(";") != 0 )
				PE_ERROR( "Expected ';' after pure virtual function declaration, found " << PE_TOKEN_NAME );
			
			newFunction.is_pure_virtual = true;
			container.function_types[thingName] = newFunction;
			container.functions[thingName] = newFunction;
			currToken ++;
		}
		else if( currToken->text.compare("{") == 0 )
		{
			currToken ++;
			
			parse_function_body( tokens, currToken, theProgram, currClass, newFunction );
			
			container.function_types[thingName] = newFunction;
			container.functions[thingName] = newFunction;
			
			if( currToken == tokens.end() || currToken->kind != token::operator_identifier || currToken->text.compare("}") != 0 )
				PE_ERROR( "Expected '}' at end of function body, found " << PE_TOKEN_NAME );
			
			currToken++;
		}
	}
}


term	parse_term( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram, classdesc& currClass, funcdesc& currFunction )
{
	term		result;
	if( currToken == tokens.end() )
		return result;
	
	if( currToken->kind == token::quoted_string )
	{
		result.kind = term::quoted_string;
		result.func_name = currToken->text;
		currToken++;
	}
	else if( currToken->kind == token::character )
	{
		result.kind = term::character;
		result.func_name = currToken->text;
		currToken++;
	}
	else if( currToken->kind == token::integer )
	{
		result.kind = term::integer;
		result.func_name = currToken->text;
		currToken++;
	}
	else if( currToken->kind == token::number )
	{
		result.kind = term::number;
		result.func_name = currToken->text;
		currToken++;
	}
	else if( currToken->kind == token::operator_identifier && currToken->text == "(" )
	{
		currToken++;
		
		result = parse_expression( tokens, currToken, theProgram, currClass, currFunction );
		
		if( currToken->kind != token::operator_identifier || currToken->text != ")" )
			PE_ERROR( "Expected ')' at end of bracketed expression, found " << PE_TOKEN_NAME );
		
		currToken++;
	}
	else if( currToken->kind == token::operator_identifier )
	{
		result.func_name = currToken->text;
		currToken++;
		result.parameters.push_back( parse_term( tokens, currToken, theProgram, currClass, currFunction ) );
	}
	else if( currToken->kind == token::identifier )
	{
		if( currToken->text == "this" )
		{
			result.kind = term::parameter;
			result.func_name = "this";
			currToken++;
			return result;
		}
		
		auto	foundVar = currFunction.variables.find( currToken->text );
		if( foundVar != currFunction.variables.end() )
			result.kind = term::variable;
		
		if( result.kind == term::function_call )
		{
			for( auto currParam : currFunction.param_types )
			{
				if( currParam.var_name == currToken->text )
				{
					result.kind = term::parameter;
					break;
				}
			}
		}

		if( result.kind == term::function_call )
		{
			foundVar = currClass.variables.find( currToken->text );
			if( foundVar != currClass.variables.end() )
				result.kind = term::instance_variable;
		}

		if( result.kind == term::function_call )
		{
			foundVar = theProgram.variables.find( currToken->text );
			if( foundVar != theProgram.variables.end() )
				result.kind = term::global_variable;
		}
		
		result.func_name = currToken->text;
		currToken++;
	}
	else
		PE_ERROR( "Expected term here, found " << PE_TOKEN_NAME );
	
	return result;
}


term	parse_expression( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram, classdesc& currClass, funcdesc& currFunction )
{
	term		result;
	if( currToken == tokens.end() )
		return result;
	
	term	argOne = parse_term( tokens, currToken, theProgram, currClass, currFunction );
	if( currToken == tokens.end() )
		return argOne;
	
	size_t		currPriority = priority_for_binary_operator(theProgram,currToken->text);
	if( currPriority == 0 )
		return argOne;
	
	// Set up a "fake" operator to start with so loop below can treat it
	//	just like any other operator to its left. This operator is lowest
	//	priority, meaning all other
	result.func_name = "__dummy_operator";	// Absolute lowest priority.
	result.kind = term::function_call;
	result.parameters.push_back( term() );
	result.parameters.push_back( argOne );
	
	term*	rightmost = &result;

	while( (currPriority = priority_for_binary_operator(theProgram,currToken->text)) > 0 )
	{
		size_t	prevPriority = priority_for_binary_operator( theProgram,rightmost->func_name );
		string	opName = currToken->text;
		currToken++;
		if( currPriority > prevPriority )
		{
			term	currOp( opName );
			currOp.kind = term::function_call;
			currOp.parameters.push_back( rightmost->parameters[1] );
			currOp.parameters.push_back( parse_term( tokens, currToken, theProgram, currClass, currFunction ) );
			rightmost->parameters[1] = currOp;
			rightmost = &rightmost->parameters[1];
		}
		else
		{
			term	currOp( opName );
			currOp.kind = term::function_call;
			currOp.parameters.push_back( *rightmost );
			currOp.parameters.push_back( parse_term( tokens, currToken, theProgram, currClass, currFunction ) );
			*rightmost = currOp;
			rightmost = &rightmost->parameters[1];
		}
	}
	
	return result.parameters[1];	// *always* __dummy_operator's right argument, as that has lowest priority.
}


void	parse_function_body( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram, classdesc& currClass, funcdesc& currFunction )
{
	while( true )
	{
		if( currToken == tokens.end() || (currToken->kind == token::operator_identifier && currToken->text.compare("}") == 0 ) )
			break;
		
		if( currToken->kind == token::identifier && currToken->text.compare("return") == 0 )
		{
			currToken++;
			
			term	currCommand( "return" );
			term	expr = parse_expression( tokens, currToken, theProgram, currClass, currFunction );
			currCommand.parameters.push_back( expr );
			currFunction.commands.push_back( currCommand );
		}
		else
		{
			typedesc	theType = parse_type( tokens, currToken, theProgram );
			string		varName = "";
			if( theType.type_name != "" )
			{
				if( currToken == tokens.end() || currToken->kind != token::identifier )
					PE_ERROR( "Expected identifier for variable name here, found " << PE_TOKEN_NAME );
				varName = currToken->text;
				
				currToken++;
				
				if( currToken == tokens.end() || currToken->kind != token::operator_identifier )
					PE_ERROR( "Expected ';' or '=' here, found " << PE_TOKEN_NAME );
				
				currFunction.variables[varName] = vardesc(varName,theType);
				if( currToken->text == ";" )
				{
					currToken++;
					continue;
				}
				if( currToken->text != "=" )
					PE_ERROR( "Expected ';' or '=' here, found " << PE_TOKEN_NAME );
				currToken++;
			}
			term	expr = parse_expression( tokens, currToken, theProgram, currClass, currFunction );
			if( varName.length() != 0 )
			{
				term	assignmentStmt("=");
				assignmentStmt.parameters.push_back( term(varName) );
				assignmentStmt.parameters[0].kind = term::variable;
				assignmentStmt.parameters.push_back( expr );
				currFunction.commands.push_back( assignmentStmt );
			}
			else
				currFunction.commands.push_back( expr );
		}
		
		if( currToken->kind != token::operator_identifier || currToken->text != ";" )
			PE_ERROR( "Expected ';' here, found " << PE_TOKEN_NAME );
		
		currToken++;
	}
}



void	parse_top_level_construct( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram )
{
	if( currToken->kind == token::identifier && (currToken->text.compare("class") == 0
												|| currToken->text.compare("struct") == 0) )
	{
		currToken++;
		
		if( currToken->kind != token::identifier )
			PE_ERROR( "Expected identifier after 'class', found " << PE_TOKEN_NAME );
		
		string		className = currToken->text;
		string		baseClassName = "object";
		string		unionName = "";
		bool		mayBeDeclaration = true;
		bool		isDeclaration = false;
		
		currToken++;
		
		if( currToken->kind == token::operator_identifier && currToken->text.compare(":") == 0 )
		{
			currToken++;
			
			if( currToken->kind != token::identifier )
				PE_ERROR( "Expected base class name after ':', found " << PE_TOKEN_NAME );
			
			baseClassName = currToken->text;
			
			currToken++;
			
			mayBeDeclaration = false;
		}
		if( currToken->kind == token::identifier && currToken->text.compare("@union") == 0 )
		{
			currToken++;
			
			if( currToken->kind != token::identifier )
				PE_ERROR( "Expected identifier after '@union', found " << PE_TOKEN_NAME );
			
			unionName = currToken->text;
			
			currToken++;
			
			mayBeDeclaration = false;
		}

		classdesc	newClass;
		newClass.type_name = className;
		newClass.class_name = baseClassName;
		newClass.union_name = unionName;

		if( mayBeDeclaration && currToken != tokens.end() && currToken->kind == token::operator_identifier && currToken->text.compare(";") == 0 )
		{	// declaration:
			currToken++;
			isDeclaration = true;
		}
		else if( currToken != tokens.end() && (currToken->kind == token::operator_identifier && currToken->text.compare("{") == 0) )
		{	// definition:
			currToken++;
			
			while( true )
			{
				if( currToken == tokens.end() || (currToken->kind == token::operator_identifier && currToken->text.compare("}") == 0 ) )
					break;
				
				bool	isOverride = false;
				if( currToken->kind == token::identifier && currToken->text.compare("override") == 0 )
				{
					currToken++;
					if( currToken == tokens.end() )
						PE_ERROR( "Expected method declaration or definition after 'override', found " << PE_TOKEN_NAME );
					isOverride = true;
				}
				parse_var_or_function( tokens, currToken, theProgram, newClass, newClass, isOverride );
			}
			
			if( currToken == tokens.end() || currToken->kind != token::operator_identifier || currToken->text.compare("}") != 0 )
				PE_ERROR( "Expected '}' at end of class/struct, found " << PE_TOKEN_NAME );
			
			currToken++;
		}
		else
		{
			throw runtime_error( "This class declaration/definition is incomplete." );
		}
		
		if( !isDeclaration )
		{
			if( theProgram.classes.find(className) != theProgram.classes.end() )
				PE_ERROR( "A class named '" << className << "' already exists" );
			theProgram.classes[className] = newClass;
		}
		theProgram.types[className] = newClass;
	}
	else
	{
		classdesc	dummy_class("__dummy_class");
		parse_var_or_function( tokens, currToken, theProgram, theProgram, dummy_class, false );
	}
}


int main( int argc, const char * argv[] )
{
	int						result = EXIT_SUCCESS;
	program					theProgram;

	try
	{
		ifstream				file( argv[1] );
		
		vector<token>			tokens = tokenize(file);
		vector<token>::iterator	currToken = tokens.begin();
		
		while( currToken != tokens.end() )
		{
			parse_top_level_construct( tokens, currToken, theProgram );
		}
	}
	catch( const parse_error& err )
	{
		cout << argv[1] << ":" << err.line << ":" << err.offset << ":" << err.what() << endl;
		
		result = EXIT_FAILURE;
	}
	catch( const exception& err )
	{
		cout << err.what() << endl;
		
		result = EXIT_FAILURE;
	}
	
	theProgram.print( 0 );
	
    return result;
}
