use regorust::Interpreter;
use clap::Parser;

#[derive(Parser)]
struct Args {
    #[arg(short, long)]
    data: Vec<std::path::PathBuf>,

    #[arg(short, long)]
    input: Option<std::path::PathBuf>,

    #[arg(last = true)]
    query: String,
}

fn main() {
    
}
