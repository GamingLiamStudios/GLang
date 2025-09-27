use nom::{
    Finish,
    Parser,
};
use nom_language::error::convert_error;
use tracing::info;

mod parser;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let fmt_subscriber = tracing_subscriber::fmt::Subscriber::builder()
        .with_max_level(tracing::Level::TRACE)
        .finish();
    tracing::subscriber::set_global_default(fmt_subscriber)?;

    info!("Hello world!");

    let args: Vec<String> = std::env::args().collect();
    if args.len() != 2 {
        std::process::exit(1);
    }

    let source = std::fs::read_to_string(&args[1])?;
    let (_, program) = nom::combinator::all_consuming(parser::program)
        .parse_complete(&source)
        .finish()
        .map_err(|e| e.to_string())?;

    println!("{program:#?}");

    Ok(())
}

#[cfg(test)]
mod test {
    use inkwell::{
        OptimizationLevel,
        context::Context,
        execution_engine::JitFunction,
    };

    // Kinda redundant, but eh it's nice to have
    #[test]
    fn llvm_basic_jit() -> Result<(), Box<dyn std::error::Error>> {
        let context = Context::create();
        let module = context.create_module("the_bees");
        let builder = context.create_builder();

        let i64_type = context.custom_width_int_type(32);
        let fn_type = i64_type.fn_type(&[i64_type.into(), i64_type.into()], false);

        let func = module.add_function("sum", fn_type, None);
        let block = context.append_basic_block(func, "entry");
        builder.position_at_end(block);

        let param_a = func.get_nth_param(0).expect("shit").into_int_value();
        let param_b = func.get_nth_param(1).expect("shit").into_int_value();

        let sum = builder.build_int_add(param_a, param_b, "sum")?;
        builder.build_return(Some(&sum))?;

        let execution_engine = module.create_jit_execution_engine(OptimizationLevel::None)?;

        let sum: JitFunction<'_, unsafe extern "C" fn(u64, u64) -> u64> =
            unsafe { execution_engine.get_function("sum")? };

        let x = 1u64;
        let y = 2u64;

        unsafe {
            assert_eq!(sum.call(x, y), x + y);
        }

        Ok(())
    }
}
