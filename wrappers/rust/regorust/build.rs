use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());

    Command::new("git")
        .args(&["clone", "https://github.com/matajoh/rego-cpp.git"])
        .current_dir(&out_path)
        .status()
        .expect("failed to execute process");

    let regocpp_path = out_path.join("rego-cpp");

    Command::new("git")
        .args(&["checkout", "rust-api"])
        .current_dir(&regocpp_path)
        .status()
        .expect("failed to execute process");

    Command::new("cmake")
        .args(&["-S", ".", "-B", "build", "--preset", "release-rust"])
        .current_dir(&regocpp_path)
        .status()
        .expect("failed to execute process");

    let regocpp_build_path = regocpp_path.join("build");

    Command::new("ninja")
        .args(&["install"])
        .current_dir(&regocpp_build_path)
        .status()
        .expect("failed to execute process");

    let regocpp_build_rust_path = regocpp_build_path.join("rust");
    let rego_path = out_path.join("rego");

    Command::new("cp")
        .args(&[
            "-r",
            regocpp_build_rust_path.to_str().unwrap(),
            rego_path.to_str().unwrap(),
        ])
        .status()
        .expect("failed to execute process");

    let libdir_path = rego_path.join("lib");

    let headers_path = rego_path.join("include").join("rego").join("rego_c.h");
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

    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
