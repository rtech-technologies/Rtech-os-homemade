use alloc::string::String;
use alloc::vec::Vec;

#[derive(Debug)]
pub enum Token {
    Print,
    StringLiteral(String),
    Number(i32),
    EOF,
}

pub fn tokenize(input: &str) -> Vec<Token> {
    let mut tokens = Vec::new();
    let mut chars = input.chars().peekable();

    while let Some(&c) = chars.peek() {
        if c.is_whitespace() {
            chars.next();
        } else if c.is_alphabetic() {
            let mut word = String::new();
            while let Some(&ch) = chars.peek() {
                if ch.is_alphabetic() {
                    word.push(chars.next().unwrap());
                } else {
                    break;
                }
            }
            if word == "print" {
                tokens.push(Token::Print);
            }
        } else if c == '"' {
            chars.next();
            let mut literal = String::new();
            while let Some(&ch) = chars.peek() {
                if ch != '"' {
                    literal.push(chars.next().unwrap());
                } else {
                    break;
                }
            }
            chars.next();
            tokens.push(Token::StringLiteral(literal));
        } else if c.is_numeric() {
            let mut num_str = String::new();
            while let Some(&ch) = chars.peek() {
                if ch.is_numeric() {
                    num_str.push(chars.next().unwrap());
                } else {
                    break;
                }
            }
            if let Ok(num) = num_str.parse::<i32>() {
                tokens.push(Token::Number(num));
            }
        } else {
            chars.next();
        }
    }
    tokens.push(Token::EOF);
    tokens
}

pub fn execute(input: &str) {
    let tokens = tokenize(input);
    let mut iter = tokens.iter();

    while let Some(token) = iter.next() {
        match token {
            Token::Print => {
                if let Some(next_token) = iter.next() {
                    match next_token {
                        Token::StringLiteral(s) => {
                            crate::println!("{}", s);
                            crate::serial_println!("{}", s);
                        }
                        Token::Number(n) => {
                            crate::println!("{}", n);
                            crate::serial_println!("{}", n);
                        }
                        _ => {}
                    }
                }
            }
            Token::EOF => break,
            _ => {}
        }
    }
}
