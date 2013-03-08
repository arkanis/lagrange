package de.arkanis.lagrange
package parser

import org.kiama.util.PositionedParserUtilities
import scala.util.parsing.combinator.RegexParsers

trait Lexer extends RegexParsers {
  lazy val TYPEVAR: Parser[Symbol]    = """[A-Z][a-zA-Z0-9_]*""".r ^^ (Symbol(_))
  lazy val VAR: Parser[Symbol]        = """[a-zA-Z$_][a-zA-Z0-9$_]*""".r ^^ (Symbol(_))
  lazy val INT: Parser[Int]           = """[0-9]+""".r ^^ (_.toInt)
  lazy val TRUE: Parser[Boolean]      = """true""".r ^^^ true
  lazy val FALSE: Parser[Boolean]     = """false""".r ^^^ false
  lazy val TUPLEACCESSOR: Parser[Int] = """_[0-9]+""".r ^^ (_.substring(1).toInt)
  lazy val SC: Parser[String]         = """[;\n]""".r
}

trait Parser extends PositionedParserUtilities  with Lexer {
  
  	import ast.{ASTNode, Statement, EmptyStmt}
  
    lazy val parser: PackratParser[ASTNode] =
      phrase (statement)
    
    lazy val statement: PackratParser[Statement] =
      ";" ^^^ EmptyStmt()
     
}