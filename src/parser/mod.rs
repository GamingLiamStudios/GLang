use nom::{
    IResult,
    Parser,
    branch::alt,
    bytes::complete::{
        take_while,
        take_while1,
    },
    character::satisfy,
    combinator::{
        fail,
        map_res,
        opt,
        recognize,
    },
    error::context,
    multi::{
        many0,
        separated_list0,
    },
    sequence::{
        delimited,
        preceded,
    },
};
use nom_language::precedence::{
    Assoc,
    Operation,
    binary_op,
    precedence,
    unary_op,
};
use nom_supreme::{
    ParserExt,
    error::ErrorTree,
    tag::complete::tag,
};

#[derive(Debug, Clone)]
pub struct Type<'a> {
    ident: &'a str,
}

#[derive(Debug, Clone)]
pub enum Expr<'a> {
    Ident(&'a str),
    Num(u64), // TODO: Allow arbitrary-length integers

    Neg(Box<Expr<'a>>),
    Not(Box<Expr<'a>>),

    Add(Box<Expr<'a>>, Box<Expr<'a>>),
    Sub(Box<Expr<'a>>, Box<Expr<'a>>),
    Mul(Box<Expr<'a>>, Box<Expr<'a>>),
    Div(Box<Expr<'a>>, Box<Expr<'a>>),

    Block {
        stmts:       Vec<Expr<'a>>,
        return_expr: Option<Box<Expr<'a>>>,
    },
    If {
        cond:     Box<Expr<'a>>,
        if_true:  Box<Expr<'a>>,
        if_false: Option<Box<Expr<'a>>>,
    },
}

#[derive(Debug, Clone)]
pub struct Const<'a> {
    name: &'a str,
    ty:   Type<'a>,

    value: Expr<'a>,
}

#[derive(Debug, Clone)]
pub struct Function<'a> {
    name:      &'a str,
    return_ty: Option<Type<'a>>, // Can be void
    arguments: Vec<(&'a str, Type<'a>)>,

    value: Expr<'a>,
}

#[derive(Debug, Clone)]
pub struct ExternFunction<'a> {
    name:      &'a str,
    return_ty: Option<Type<'a>>, // Can be void

    arguments: Vec<(&'a str, Type<'a>)>,
}

#[derive(Debug, Clone)]
pub enum RootType<'a> {
    Const(Const<'a>),
    Function(Function<'a>),
    ExternFunction(ExternFunction<'a>),
}

// whitespace is the bane of my existance
pub fn whitespace0(input: &str) -> IResult<&str, &str, ErrorTree<&str>> {
    take_while(|c: char| c.is_whitespace()).parse_complete(input)
}

pub fn whitespace1(input: &str) -> IResult<&str, &str, ErrorTree<&str>> {
    take_while1(|c: char| c.is_whitespace()).parse_complete(input)
}

pub fn ident(input: &str) -> IResult<&str, &str, ErrorTree<&str>> {
    recognize(
        satisfy(|c| c.is_alphabetic() || c == '_')
            .and(take_while(|c: char| c.is_alphanumeric() || c == '_')),
    )
    .parse_complete(input)
}

pub fn ty(input: &str) -> IResult<&str, Type<'_>, ErrorTree<&str>> {
    ident.map(|ident| Type { ident }).parse_complete(input)
}

// TODO: Allow '_' grouping of digits
// TODO: Allow arbitrary int sizes > 64bit
pub fn num_base16(input: &str) -> IResult<&str, u64, ErrorTree<&str>> {
    map_res(
        preceded(
            alt((tag("0x"), tag("0X"))),
            take_while(|c: char| c.is_ascii_hexdigit()),
        ),
        |digits| u64::from_str_radix(digits, 16),
    )
    .parse_complete(input)
}

pub fn num_base10(input: &str) -> IResult<&str, u64, ErrorTree<&str>> {
    map_res(take_while(|c: char| c.is_ascii_digit()), |digits: &str| {
        digits.parse::<u64>()
    })
    .parse_complete(input)
}

pub fn num(input: &str) -> IResult<&str, u64, ErrorTree<&str>> {
    alt((num_base16, num_base10)).parse_complete(input)
}

pub fn block(input: &str) -> IResult<&str, Expr<'_>, ErrorTree<&str>> {
    delimited(
        tag("{"),
        (
            many0(delimited(whitespace0, expr, (whitespace0, tag(";")))),
            opt(preceded(whitespace0, expr)),
        ),
        (whitespace0, tag("}")),
    )
    .map(|(stmts, return_expr)| Expr::Block {
        stmts,
        return_expr: return_expr.map(Box::new),
    })
    .parse_complete(input)
}

pub fn if_condition(input: &str) -> IResult<&str, Expr<'_>, ErrorTree<&str>> {
    (
        tag("if"),
        preceded(whitespace1, expr),
        preceded(whitespace0, block),
    )
        .context("if_stmt")
        .map(|(_, cond, if_true)| Expr::If {
            cond:     Box::new(cond),
            if_true:  Box::new(if_true),
            if_false: None,
        })
        .parse_complete(input)
}

pub fn expr(input: &str) -> IResult<&str, Expr<'_>, ErrorTree<&str>> {
    precedence(
        alt((unary_op(1, tag("-")), unary_op(1, tag("!")))),
        fail(),
        alt((
            binary_op(2, Assoc::Left, tag("*")),
            binary_op(2, Assoc::Left, tag("/")),
            binary_op(3, Assoc::Left, tag("+")),
            binary_op(3, Assoc::Left, tag("-")),
        )),
        delimited(
            whitespace0,
            alt((
                ident.map(Expr::Ident),
                num.map(Expr::Num),
                //if_condition,
                delimited(
                    tag("("),
                    delimited(whitespace0, expr, whitespace0),
                    tag(")"),
                ),
            )),
            whitespace0,
        ),
        |op: Operation<&str, &str, &str, Expr<'_>>| {
            // <Prefix, Postfix, Binary, Value>
            use nom_language::precedence::Operation::{
                Binary,
                Prefix,
            };
            match op {
                Prefix("-", o) => Ok(Expr::Neg(Box::new(o))),
                Prefix("!", o) => Ok(Expr::Not(Box::new(o))),
                Binary(lhs, "*", rhs) => Ok(Expr::Mul(Box::new(lhs), Box::new(rhs))),
                Binary(lhs, "/", rhs) => Ok(Expr::Div(Box::new(lhs), Box::new(rhs))),
                Binary(lhs, "+", rhs) => Ok(Expr::Add(Box::new(lhs), Box::new(rhs))),
                Binary(lhs, "-", rhs) => Ok(Expr::Sub(Box::new(lhs), Box::new(rhs))),
                _ => Err("Invalid combination"),
            }
        },
    )
    .parse_complete(input)
}

pub fn root_const(input: &str) -> IResult<&str, Const<'_>, ErrorTree<&str>> {
    context(
        "const",
        (
            tag("const"),
            preceded(whitespace1, ident),
            preceded(whitespace0, tag(":")),
            preceded(whitespace0, ty),
            preceded(whitespace0, tag("=")),
            preceded(whitespace0, expr),
            preceded(whitespace0, tag(";")),
        ),
    )
    .map(|(_, name, _, ty, _, value, ..)| Const { name, ty, value })
    .parse_complete(input)
}

// TODO: Function Modifiers (const, templates, etc)
pub fn function(input: &str) -> IResult<&str, Function<'_>, ErrorTree<&str>> {
    (
        tag("fn"),
        delimited(whitespace1, ident, whitespace0),
        delimited(
            tag("("),
            separated_list0(
                tag(","),
                delimited(
                    whitespace0,
                    (ident, delimited(whitespace0, tag(":"), whitespace0), ty)
                        .map(|(name, _, ty)| (name, ty)),
                    whitespace0,
                ),
            ),
            tag(")"),
        ),
        preceded(whitespace0, opt((tag("->"), preceded(whitespace0, ty)))),
        preceded(whitespace0, block),
    )
        .map(|(_, name, arguments, ty, value)| Function {
            name,
            return_ty: ty.map(|(_, ty)| ty),
            arguments,
            value,
        })
        .parse_complete(input)
}

pub fn extern_function(input: &str) -> IResult<&str, ExternFunction<'_>, ErrorTree<&str>> {
    context(
        "extern_fn",
        (
            tag("extern"),
            preceded(whitespace1, tag("fn")),
            delimited(whitespace1, ident, whitespace0),
            delimited(
                tag("("),
                separated_list0(
                    tag(","),
                    delimited(
                        whitespace0,
                        (ident, delimited(whitespace0, tag(":"), whitespace0), ty)
                            .map(|(name, _, ty)| (name, ty)),
                        whitespace0,
                    ),
                ),
                tag(")"),
            ),
            preceded(whitespace0, opt((tag("->"), preceded(whitespace0, ty)))),
            preceded(whitespace0, tag(";")),
        ),
    )
    .map(|(_, _, name, arguments, ty, ..)| ExternFunction {
        name,
        return_ty: ty.map(|(_, ty)| ty),
        arguments,
    })
    .parse_complete(input)
}

pub fn program(input: &str) -> IResult<&str, Vec<RootType<'_>>, ErrorTree<&str>> {
    many0(delimited(
        whitespace0,
        alt((
            root_const.map(RootType::Const),
            function.map(RootType::Function),
            extern_function.map(RootType::ExternFunction),
        )),
        whitespace0,
    ))
    .parse_complete(input)
}
