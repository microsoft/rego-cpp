use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    Command::new("git")
        .args(&["clone", "https://github.com/matajoh/rego-cpp.git"])
        .status()
        .expect("failed to execute process");

    Command::new("git")
        .args(&["checkout", "rust-api"])
        .current_dir("rego-cpp")
        .status()
        .expect("failed to execute process");

    Command::new("cmake")
        .args(&["-S", ".", "-B", "build", "--preset", "rust"])
        .current_dir("rego-cpp")
        .status()
        .expect("failed to execute process");

    let libdir_path = PathBuf::from("rego/lib")
        .canonicalize()
        .expect("cannot canonicalize path");

    let headers_path = PathBuf::from("rego/include/rego/rego_c.h")
        .canonicalize()
        .expect("cannot canonicalize path");
    let headers_path_str = headers_path.to_str().expect("Path is not a valid string");

    println!("cargo:rustc-link-search={}", libdir_path.to_str().unwrap());
    println!("cargo:rustc-link-lib=static=re2");
    println!("cargo:rustc-link-lib=static=snmallocshim-static");
    println!("cargo:rustc-link-lib=static=rego");
    println!("cargo:rustc-link-lib=stdc++");
    println!("cargo:rerun-if-changed={}", headers_path.to_str().unwrap());

    let bindings = bindgen::Builder::default()
        .header(headers_path_str)
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
