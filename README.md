# Pseudocode Interpreter

## Project Description

An interpreter for a dynamic programming language based on pseudocode, featuring
customizable error messages, function names, keywords, and primitive
types.

> **Note** see [the documentation](docs.md) for more information about the language

## Getting Started

### Prerequisites

- g++

  Debian/Ubuntu:

  ```sh
  sudo apt install g++
  ```

### Installation

1. Clone the repo

   ```sh
   git clone https://github.com/CozmaRares/psc-interpreter.git
   cd psc-interpreter
   ```

2. Build the project

   ```sh
   g++ src/main.cpp -o bin/main
   ```

3. Start the Interpreter

   ```sh
   ./bin/main
   ```

## Reflection

I embarked on its development as a project for a high school-provided
certification. It all started because I wanted a better way to study for my
baccalaureate computer science exam. The exam included snippets of pseudo-code,
and I thought it would be great to have a tool that could check my answers and
help me understand how the code worked.

To accomplish this goal, I set out to create an interpreter for a dynamic
language based on pseudo-code syntax. This involved not only evaluating code but
also incorporating features for better error handling and customization. I
wanted to make it possible for users to customize error messages, function names,
keywords, and even primitive data types to facilitate translation and adapt the
interpreter to different languages or scenarios.

I went with C++ because it was the only programming language I knew at the time,
but it turned out to be a good choice because it can handle complicated stuff
well, has handy smart pointers that make managing memory easier, and it's pretty
speedy too.

In the end, this project helped me get my certification, but it also became a
fantastic study tool.
