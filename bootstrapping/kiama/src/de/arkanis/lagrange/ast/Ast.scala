package de.arkanis.lagrange
package ast

import org.kiama.attribution.Attributable
import org.kiama.util.Positioned
  
abstract class ASTNode extends Attributable with Positioned

case class Module(decls: List[Declaration]) extends ASTNode


/**
 * Declarations
 * ------------
 */
abstract class Declaration extends ASTNode {
  def name: String
  def modifiers: List[Modifier]
}
case class FunctionDecl(name: String, modifiers: List[Modifier], body: BlockStmt) extends Declaration
case class TypeDecl(name: String, modifiers: List[Modifier]) extends Declaration


/**
 * Modifiers
 * ---------
 */
abstract class Modifier extends ASTNode
case class InMod(args: List[(String, String)]) extends Modifier
case class OutMod(returns: List[String]) extends Modifier


/**
 * Statements
 * ----------
 */
abstract class Statement extends ASTNode

case class BlockStmt(stmts: List[Statement]) extends Statement
case class ReturnStmt(exprs: List[Expression]) extends Statement

/**
 * Expressions
 * -----------
 */
abstract class Expression extends ASTNode