use clap::{Parser, Subcommand, ValueEnum};
use regorust::{BundleFormat, Interpreter, LogLevel};

#[derive(Parser)]
#[command(author, version, about, long_about = None)]
struct Args {
    #[arg(short, long)]
    loglevel: Option<LogLevelOption>,

    #[command(subcommand)]
    command: Commands,
}

#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, ValueEnum)]
enum LogLevelOption {
    None,
    Debug,
    Info,
    Warn,
    Error,
    Output,
    Trace,
}

#[derive(Subcommand)]
enum Commands {
    Eval {
        query: String,

        #[arg(short, long)]
        data: Vec<std::path::PathBuf>,

        #[arg(short, long)]
        input: Option<std::path::PathBuf>,
    },

    Build {
        #[arg(short, long)]
        query: Option<String>,

        #[arg(short, long)]
        data: Vec<std::path::PathBuf>,

        #[arg(short, long)]
        entrypoints: Vec<String>,

        #[arg(short, long)]
        bundle: Option<std::path::PathBuf>,

        #[arg(long)]
        binary: bool,
    },

    Run {
        #[arg(short, long)]
        input: Option<std::path::PathBuf>,

        #[arg(short, long)]
        entrypoint: Option<String>,

        #[arg(short, long)]
        bundle: Option<std::path::PathBuf>,

        #[arg(long)]
        binary: bool,
    },
}

fn main() {
    let args = Args::parse();
    let default_path = std::path::PathBuf::from("bundle");
    let rego = Interpreter::new();

    match args.loglevel {
        Some(LogLevelOption::None) => rego.set_log_level(LogLevel::None),
        Some(LogLevelOption::Debug) => rego.set_log_level(LogLevel::Debug),
        Some(LogLevelOption::Info) => rego.set_log_level(LogLevel::Info),
        Some(LogLevelOption::Warn) => rego.set_log_level(LogLevel::Warn),
        Some(LogLevelOption::Error) => rego.set_log_level(LogLevel::Error),
        Some(LogLevelOption::Output) => rego.set_log_level(LogLevel::Output),
        Some(LogLevelOption::Trace) => rego.set_log_level(LogLevel::Trace),
        None => rego.set_log_level(LogLevel::Output),
    }
    .expect("Unable to set log level");

    match &args.command {
        Commands::Eval { query, data, input } => {
            if let Some(i) = input {
                rego.set_input_json_file(i.as_path())
                    .expect("Failed to read input file");
            }

            for d in data {
                if d.extension().unwrap() == "rego" {
                    rego.add_module_file(d.as_path())
                        .expect("Failed to load module file");
                } else {
                    rego.add_data_json_file(d.as_path())
                        .expect("Failed to load data file");
                }
            }

            println!(
                "{}",
                rego.query(query.as_str())
                    .expect("Failed to evaluate query")
            );
        }

        Commands::Build {
            query,
            data,
            entrypoints,
            bundle,
            binary,
        } => {
            for d in data {
                if d.extension().unwrap() == "rego" {
                    rego.add_module_file(d.as_path())
                        .expect("Failed to load module file");
                } else {
                    rego.add_data_json_file(d.as_path())
                        .expect("Failed to load data file");
                }
            }

            let bundle_path = if let Some(b) = bundle {
                b.as_path()
            } else {
                default_path.as_path()
            };

            match rego.build(query, entrypoints) {
                Ok(b) => {
                    rego.save_bundle(
                        bundle_path,
                        &b,
                        if *binary {
                            BundleFormat::Binary
                        } else {
                            BundleFormat::JSON
                        },
                    )
                    .expect("Unable to save bundle");
                    println!("Bundle built successfully");
                }

                Err(msg) => {
                    println!("Unable to build bundle: {}", msg);
                }
            }
        }

        Commands::Run {
            input,
            entrypoint,
            bundle,
            binary,
        } => {
            if let Some(i) = input {
                rego.set_input_json_file(i.as_path())
                    .expect("Failed to read input file");
            }

            let bundle_path = if let Some(b) = bundle {
                b.as_path()
            } else {
                default_path.as_path()
            };

            let bundle = rego
                .load_bundle(
                    bundle_path,
                    if *binary {
                        BundleFormat::Binary
                    } else {
                        BundleFormat::JSON
                    },
                )
                .expect("Unable to load bundle");

            if let Some(e) = entrypoint {
                println!(
                    "{}",
                    rego.query_bundle_entrypoint(&bundle, e.as_str())
                        .expect("Failed to query bundle")
                );
                return;
            } else {
                println!(
                    "{}",
                    rego.query_bundle(&bundle).expect("Failed to query bundle")
                );
            }
        }
    }
}
