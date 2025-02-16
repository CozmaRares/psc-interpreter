mod lexer;
mod nodes;
mod parser;
mod tokens;

use std::io;

use anyhow::Result;
use lexer::Lexer;
use parser::Parser;

fn main() -> Result<()> {
    let stdin = io::stdin();
    for line in stdin.lines() {
        let line = line?;
        println!("Read: {}", line);

        let tokens = Lexer::tokenize(&line).unwrap();
        println!("Tokens: {:#?}", tokens);
        println!("AST: {:#?}", Parser::parse(tokens).unwrap());
    }
    Ok(())
}
