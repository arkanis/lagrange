package de.arkanis.lagrange
package ast

import org.kiama.attribution.Attributable
import org.kiama.util.Positioned

abstract class ASTNode extends Attributable with Positioned

abstract class Statement extends ASTNode
case class EmptyStmt() extends Statement