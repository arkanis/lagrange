package de.arkanis.lagrange
package parser

import org.kiama.util.PositionedParserUtilities
import scala.util.parsing.combinator.RegexParsers

trait Lexer extends RegexParsers {
  lazy val TYPEVAR: Parser[String]    = """[A-Z][a-zA-Z0-9_]*""".r 
  lazy val VAR: Parser[String]        = """[a-zA-Z$_][a-zA-Z0-9$_]*""".r
  lazy val INT: Parser[Int]           = """[0-9]+""".r ^^ (_.toInt)
  lazy val TRUE: Parser[Boolean]      = """true""".r ^^^ true
  lazy val FALSE: Parser[Boolean]     = """false""".r ^^^ false
  lazy val TUPLEACCESSOR: Parser[Int] = """_[0-9]+""".r ^^ (_.substring(1).toInt)
  lazy val SC: Parser[String]         = """[;\n]""".r
}

trait Parser extends PositionedParserUtilities  with Lexer {
  
  	import ast.{ASTNode, Module, Declaration, FunctionDecl, Statement, BlockStmt, TypeDecl, Modifier, InMod, OutMod}
  
    lazy val parser: PackratParser[ASTNode] =
      phrase (module)
    
    lazy val module: PackratParser[Module] =
      declaration.+ ^^ { case decls => Module(decls) }
    
    lazy val declaration: PackratParser[Declaration] = 
    	( functionDecl
    	| typeDecl
      )
    
    lazy val functionDecl: PackratParser[FunctionDecl] =
      "function".? ~> (VAR ~ rep(modifier) ~ blockStmt) ^^ {
      	case name ~ modifiers ~ body => FunctionDecl(name, modifiers, body)
    	} 
      
    lazy val typeDecl: PackratParser[TypeDecl] =
      "type" ~> VAR <~ ("{" ~ "}") ^^ { case name => TypeDecl(name, List.empty) }
    
    lazy val arg: PackratParser[(String, String)] =
      VAR ~ VAR ^^ {
        case typeExpr ~ name => (typeExpr, name)
      }
      
    lazy val modifier: PackratParser[Modifier] =
      ( "in" ~> ("(" ~> repsep(arg, ",") <~ ")") ^^ { case args => InMod(args) }
      | "out" ~> ("(" ~> repsep(VAR, ",") <~ ")") ^^ { case types => OutMod(types) }
      )
      
    lazy val statement: PackratParser[Statement] =
      blockStmt
      
    lazy val blockStmt: PackratParser[BlockStmt] =
    	("{" ~> rep(statement) <~ "}" ^^ { case stmts => BlockStmt(stmts) }
    	| "do" ~> rep(statement) <~ "end" ^^ { case stmts => BlockStmt(stmts) }
    	)
}