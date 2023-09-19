use clap::Parser;
use regorust::Interpreter;

#[derive(Parser)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[arg(short, long)]
    data: Vec<std::path::PathBuf>,

    #[arg(short, long)]
    input: Option<std::path::PathBuf>,

    query: String,
}

fn main() {
    let args = Args::parse();
    let rego = Interpreter::new();
    if let Some(input) = args.input {
        rego.set_input_json_file(input.as_path())
            .expect("Failed to read input file");
    }

    for data in args.data {
        if data.extension().unwrap() == "rego" {
            rego.add_module_file(data.as_path())
                .expect("Failed to load module file");
        } else {
            rego.add_data_json_file(data.as_path())
                .expect("Failed to load data file");
        }
    }

    println!(
        "{}",
        rego.query(args.query.as_str())
            .expect("Failed to evaluate query")
    );
}
