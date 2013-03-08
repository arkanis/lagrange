package language.kiama
package prototype

import org.kiama.output.PrettyPrinter
import org.kiama.util.ParsingREPL
import Syntax.Exp

/**
 * Run with:
 *   scala -cp ./bin:./lib/kiama_2.9.2-1.4.0.jar:./lib/jline-1.0.jar org.kiama.owntest.Main
 */
object Main extends Parser with Evaluator with ParsingREPL[Exp] {
  
  def start: Parser[Exp] = parser
  override def prompt = "> "
    
  def process(tree: Exp) {
    println(tree)
  }
}