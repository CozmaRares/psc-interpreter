mod tokens;
mod lexer;

use std::io;

use anyhow::Result;

fn main() -> Result<()> {
    let stdin = io::stdin();
    for line in stdin.lines() {
        let line = line?;
        println!("Read: {}", line);
    }
    Ok(())
}
