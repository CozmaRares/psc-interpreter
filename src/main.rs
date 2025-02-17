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
        let tokens = match Lexer::tokenize(&line) {
            Ok(tokens) => tokens,
            Err(err) => {
                println!("{}", err);
                continue;
            }
        };
        println!("{:#?}", tokens);
        println!("{:#?}", Parser::parse(tokens));
        println!()
    }
    Ok(())
}
