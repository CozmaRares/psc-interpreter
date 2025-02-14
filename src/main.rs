mod lexer;
mod tokens;

use std::io;

use anyhow::Result;
use lexer::Lexer;

fn main() -> Result<()> {
    let stdin = io::stdin();
    for line in stdin.lines() {
        let line = line?;
        println!("Read: {}", line);
        println!("Lexed: {:?}", Lexer::tokenize(&line));
    }
    Ok(())
}
