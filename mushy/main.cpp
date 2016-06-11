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


using namespace std;


class state	whitespace_state( char currCh, class info& ioInfo );
class state	identifier_state( char currCh, class info& ioInfo );
class state	string_state( char currCh, class info& ioInfo );
class state	character_state( char currCh, class info& ioInfo );
class state	multi_line_comment_state( char currCh, class info& ioInfo );

void	parse_function_body( vector<class token>& tokens, vector<class token>::iterator& currToken, class program& theProgram, class funcdesc& currFunction );


class token
{
public:
	typedef enum {
		whitespace,
		identifier,
		string,
		character,
		number,
		integer
	} token_kind;
	
	token() : kind(whitespace) {  }
	
	token_kind		kind;
	std::string		text;
};


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
	info( istream& inStream ) : stream(inStream) {}
	
	token			curr_token;
	vector<token>	tokens;
	istream&		stream;
};


class vardesc;
class functypedesc;
class funcdesc;


class varfunccontainer
{
public:
	void	print( size_t indentLevel );
	
	map<string,vardesc>			variables;		// Variables that have been defined.
	map<string,functypedesc>	function_types;	// Forward-declared functions.
	map<string,funcdesc>		functions;		// Function definitions.
};


class typedesc : public varfunccontainer
{
public:
	typedesc( string inName = "" ) : type_name(inName) {}
	typedesc( const typedesc& inOriginal ) : type_name(inOriginal.type_name), union_name(inOriginal.union_name), template_arguments(inOriginal.template_arguments), class_name(inOriginal.class_name), class_template_arguments(inOriginal.class_template_arguments) {}
	
	void	print( size_t indentLevel )
	{
		cout << type_name;
		if( template_arguments.size() > 0 )
		{
			cout << "<";
			for( auto currArg : template_arguments )
			{
				currArg.print(indentLevel);
			}
			cout << ">";
		}

		cout << " : " << class_name;
		if( class_template_arguments.size() > 0 )
		{
			cout << "<";
			for( auto currArg : class_template_arguments )
			{
				currArg.print(indentLevel);
			}
			cout << ">";
		}
		if( union_name.size() > 0 )
			cout << " @" << union_name;
		cout << endl;
	}
	
	string				type_name;					// Name of this type.
	string				union_name;					// Name of the union this type belongs to.
	vector<typedesc>	template_arguments;			// Types for all template arguments.
	string				class_name;					// Name of the base class for this type.
	vector<typedesc>	class_template_arguments;	// Types for all template arguments to the base class.
};


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
	
	string	var_name;
};


class functypedesc
{
public:
	functypedesc( string inName = "" ) : func_name(inName) {}
	
	void	print( size_t idx )
	{
		cout << func_name << "(";
		for( auto currParam : param_types )
		{
			currParam.print( idx );
		}
		cout << ")" << endl;
	}
	
	string				func_name;
	vector<vardesc>		param_types;
};


class funcdesc : public functypedesc
{
public:
	funcdesc( string inName = "" ) : functypedesc(inName), is_pure_virtual(false) {}
	
	bool	is_pure_virtual;
	bool	is_override;
};


class program : public varfunccontainer
{
public:
	program();
	
	void	print()
	{
		varfunccontainer::print(0);
		for( auto currType : types )
		{
			currType.second.print(0);
		}
		for( auto currClass : classes )
		{
			currClass.second.print(0);
		}
	}
	
	map<string,typedesc>		types;			// Forward-declared types.
	map<string,classdesc>		classes;		// Class definitions.
};


void	varfunccontainer::print( size_t indentLevel )
{
	for( auto currVar : variables )
	{
		currVar.second.print(indentLevel);
	}
	for( auto currVar : functions )
	{
		currVar.second.print(indentLevel);
	}
	for( auto currVar : function_types )
	{
		currVar.second.print(indentLevel);
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
}

void	finish_token( info& ioInfo )
{
	if( ioInfo.curr_token.kind != token::whitespace )
	{
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
			ioInfo.curr_token.kind = token::string;
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
				ioInfo.curr_token.kind = token::identifier;
				ioInfo.curr_token.text.append( 1, currCh );
				finish_token( ioInfo );
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
		ioInfo.curr_token.kind = token::identifier;
		ioInfo.curr_token.text.append( 1, currCh );
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
			ioInfo.curr_token.kind = token::string;
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
				ioInfo.curr_token.kind = token::identifier;
				ioInfo.curr_token.text.append( 1, currCh );
				finish_token( ioInfo );
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
	
	while( true )
	{
		char	currCh = 0;
		inStream.get( currCh );
		if( inStream.eof() )
			break;
		currState = currState( currCh, currInfo );
	}
	finish_token( currInfo );
	
	return currInfo.tokens;
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
		}
		
		currToken++;
	}
	else if( nothingYet )
		throw runtime_error("Expected type here");
	
	if( !nothingYet && currToken != tokens.end() && currToken->kind == token::identifier && currToken->text.compare("<") == 0 )
	{
		currToken++;
		
		while( true )
		{
			typedesc currTemplateType = parse_type( tokens, currToken, theProgram );
			if( currTemplateType.type_name.length() > 0 )
			{
				if( currToken == tokens.end() )
					throw runtime_error("Expected > here, found end of file.");
				else if( currToken->kind == token::identifier && currToken->text.compare(">") == 0 )
				{
					currToken++;
					break;
				}
				else if( currToken->kind != token::identifier || currToken->text.compare(",") != 0 )
					currToken++;
			}
			theType.template_arguments.push_back(currTemplateType);
		}
	}
	
	return theType;
}


void	parse_function_parameters( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram, funcdesc& currFunction )
{
	if( currToken == tokens.end() || (currToken->kind != token::identifier || currToken->text.compare(")") == 0) )
		return;
	
	while( true )
	{
		vardesc theVar( "", parse_type( tokens, currToken, theProgram ) );
		if( theVar.type_name.length() == 0 )
			throw runtime_error("Expected parameter type here.");
		
		if( currToken == tokens.end() || currToken->kind != token::identifier )
			throw runtime_error("Expected parameter name after type here.");
		
		theVar.var_name = currToken->text;
		currFunction.param_types.push_back( theVar );
		currToken++;

		if( currToken == tokens.end() || currToken->kind != token::identifier )
			throw runtime_error("Expected ',' or ')' here.");
		
		if( currToken->text.compare(")") == 0 )	// End of list.
			break;
		else if( currToken->text.compare(",") == 0 )	// Another param follows.
			currToken++;
		else
			throw runtime_error("Expected ',' or ')' here.");
	}
}

void	parse_var_or_function( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram, varfunccontainer& currClass, bool isOverride )
{
	typedesc	theType = parse_type( tokens, currToken,  theProgram );
	
	if( currToken == tokens.end() || currToken->kind != token::identifier )
		throw runtime_error( "Expected identifier after type at top level." );
	
	string	thingName = currToken->text;
	currToken++;
	
	if( currToken == tokens.end() || currToken->kind != token::identifier )
		throw runtime_error( "Expected semicolon after variable name, or opening bracket after function name." );
	
	if( !isOverride && currToken->text.compare(";") == 0 )	// Variable!
	{
		if( currClass.variables.find(thingName) != currClass.variables.end() )
			throw runtime_error( "A global of this name already exists." );
		currClass.variables[thingName] = vardesc(thingName, theType);
		currToken++;
	}
	else if( currToken->text.compare("(") == 0 )	// Function!
	{
		currToken++;
		
		funcdesc	newFunction( thingName );
		newFunction.is_override = isOverride;
		
		parse_function_parameters( tokens, currToken, theProgram, newFunction );
		
		if( currToken == tokens.end() || currToken->kind != token::identifier || currToken->text.compare(")") != 0 )
			throw runtime_error( "Expected ')' at end of function parameter list." );
		currToken++;
		
		if( currToken == tokens.end() || currToken->kind != token::identifier )
			throw runtime_error( "Expected ';' or '{' after function parameter list." );
		if( currToken->text.compare(";") == 0 )
		{
			currClass.function_types[thingName] = newFunction;
			currToken ++;
		}
		else if( currToken->text.compare("=") == 0 )
		{	// pure virtual declaration:
			currToken++;
			
			if( currToken == tokens.end() || currToken->kind != token::identifier || currToken->text.compare("null") != 0 )
				throw runtime_error( "Expected 'null' after '=' after function declaration." );

			currToken++;
			
			if( currToken == tokens.end() || currToken->kind != token::identifier || currToken->text.compare(";") != 0 )
				throw runtime_error( "Expected ';' after pure virtual function declaration." );
			
			newFunction.is_pure_virtual = true;
			currClass.function_types[thingName] = newFunction;
			currClass.functions[thingName] = newFunction;
			currToken ++;
		}
		else if( currToken->text.compare("{") == 0 )
		{
			currToken ++;
			
			parse_function_body( tokens, currToken, theProgram, newFunction );
			
			currClass.function_types[thingName] = newFunction;
			currClass.functions[thingName] = newFunction;
			
			if( currToken == tokens.end() || currToken->kind != token::identifier || currToken->text.compare("}") != 0 )
				throw runtime_error( "Expected '}' at end of function body." );
		}
	}
}


void	parse_function_body( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram, funcdesc& currFunction )
{
	while( true )
	{
		if( currToken == tokens.end() || (currToken->kind == token::identifier && currToken->text.compare("}") == 0 ) )
			break;
		
		if( currToken->kind == token::identifier && currToken->text.compare("return") == 0 )
		{
			currToken++;
			
			command	currCommand( "return" );
			expression	expr = parse_expression( tokens, currToken, theProgram, currFunction, currCommand );
			currCommand.parameters.push_back( expr );
			currFunction.commands.push_back( currCommand );
		}
		else
			throw runtime_error( "Unknown element in function body." );
	}
}



void	parse_top_level_construct( vector<token>& tokens, vector<token>::iterator& currToken, program& theProgram )
{
	if( currToken->kind == token::identifier && (currToken->text.compare("class") == 0
												|| currToken->text.compare("struct") == 0) )
	{
		currToken++;
		
		if( currToken->kind != token::identifier )
			throw runtime_error( "Expected identifier after 'class'." );
		
		string		className = currToken->text;
		string		baseClassName = "object";
		string		unionName = "";
		bool		mayBeDeclaration = true;
		bool		isDeclaration = false;
		
		currToken++;
		
		if( currToken->kind == token::identifier && currToken->text.compare(":") == 0 )
		{
			currToken++;
			
			if( currToken->kind != token::identifier )
				throw runtime_error( "Expected base class name after ':'." );
			
			baseClassName = currToken->text;
			
			currToken++;
			
			mayBeDeclaration = false;
		}
		if( currToken->kind == token::identifier && currToken->text.compare("@union") == 0 )
		{
			currToken++;
			
			if( currToken->kind != token::identifier )
				throw runtime_error( "Expected identifier after '@union'." );
			
			unionName = currToken->text;
			
			currToken++;
			
			mayBeDeclaration = false;
		}

		classdesc	newClass;
		newClass.type_name = className;
		newClass.class_name = baseClassName;
		newClass.union_name = unionName;

		if( mayBeDeclaration && currToken != tokens.end() && currToken->kind == token::identifier && currToken->text.compare(";") == 0 )
		{	// declaration:
			currToken++;
			isDeclaration = true;
		}
		else if( currToken != tokens.end() && (currToken->kind == token::identifier && currToken->text.compare("{") == 0) )
		{	// definition:
			currToken++;
			
			while( true )
			{
				if( currToken == tokens.end() || (currToken->kind == token::identifier && currToken->text.compare("}") == 0 ) )
					break;
				
				bool	isOverride = false;
				if( currToken->kind == token::identifier && currToken->text.compare("override") == 0 )
				{
					currToken++;
					if( currToken == tokens.end() )
						throw runtime_error( "Expected method declaration or definition after 'override'." );
					isOverride = true;
				}
				parse_var_or_function( tokens, currToken, theProgram, newClass, isOverride );
			}
			
			if( currToken == tokens.end() || currToken->kind != token::identifier || currToken->text.compare("}") != 0 )
				throw runtime_error( "Expected } at end of class/struct." );
			
			currToken++;
		}
		else
		{
			throw runtime_error( "This class declaration/definition is incomplete." );
		}
		
		if( !isDeclaration )
		{
			if( theProgram.classes.find(className) != theProgram.classes.end() )
				throw runtime_error("A class of this name already exists.");
			theProgram.classes[className] = newClass;
		}
		theProgram.types[className] = newClass;
	}
	else
	{
		parse_var_or_function( tokens, currToken, theProgram, theProgram, false );
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
	catch( const exception& err )
	{
		cout << err.what() << endl;
		
		result = EXIT_FAILURE;
	}
	
	theProgram.print();
	
    return result;
}
