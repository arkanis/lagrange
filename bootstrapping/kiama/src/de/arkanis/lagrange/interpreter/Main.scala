package de.arkanis.lagrange
package interpreter

import org.kiama.output.PrettyPrinter
import org.kiama.util.{ParsingREPL, Emitter}
import org.kiama.attribution.Attribution.initTree
import org.kiama.util.Messaging._

import ast.ASTNode
import parser.Parser

object Main extends Parser with ParsingREPL[ASTNode] {
  
  def start: Parser[ASTNode] = parser
  override def prompt = "> "
    
  def process(tree: ASTNode) {
    println(tree)
  }
}