use std::env;
use std::num::NonZeroUsize;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let regocpp_path = out_path.join("rego-cpp");
    let num_procs = std::thread::available_parallelism().unwrap_or(NonZeroUsize::new(1).unwrap());
    let rego_git_repo = std::env::var("REGOCPP_C_WRAPPER_REPO").unwrap_or("https://github.com/microsoft/rego-cpp.git".to_string());
    let rego_git_tag = std::env::var("REGOCPP_C_WRAPPER_TAG").unwrap_or("main".to_string());

    if !regocpp_path.exists() {
        Command::new("git")
            .args(&["clone", &rego_git_repo])
            .current_dir(&out_path)
            .status()
            .expect("failed to execute process");
    } else {
        Command::new("git")
            .args(&["pull", "--rebase"])
            .current_dir(&regocpp_path)
            .status()
            .expect("failed to execute process");
    }

    Command::new("git")
        .args(&["checkout", &rego_git_tag])
        .current_dir(&regocpp_path)
        .status()
        .expect("failed to execute process");

    let regocpp_config = match std::env::var("PROFILE").as_ref() {
        Ok(build_type) => {
            if build_type == "debug" {
                "RelWithDebInfo"
            } else {
                "Release"
            }
        }
        Err(_) => "Release",
    };

    Command::new("cmake")
        .args(&[
            "-S",
            ".",
            "-B",
            "build",
            "-DCMAKE_BUILD_TYPE={}"
                .replace("{}", regocpp_config)
                .as_str(),
            "-DCMAKE_INSTALL_PREFIX=rust",
            "-DREGOCPP_COPY_EXAMPLES=ON",
        ])
        .current_dir(&regocpp_path)
        .status()
        .expect("failed to execute process");

    let regocpp_build_path = regocpp_path.join("build");

    Command::new("cmake")
        .args(&[
            "--build",
            ".",
            "--config",
            regocpp_config,
            "--parallel",
            &num_procs.to_string(),
            "--target",
            "install",
        ])
        .current_dir(&regocpp_build_path)
        .status()
        .expect("failed to execute process");

    let rego_path = regocpp_path.join("rust");
    let libdir_path = rego_path.join("lib");
    let headers_path = rego_path.join("include").join("rego").join("rego_c.h");
    let headers_path_str = headers_path.to_str().expect("Path is not a valid string");

    println!("cargo:rustc-link-search={}", libdir_path.to_str().unwrap());
    println!("cargo:rustc-link-lib=static=re2");
    println!("cargo:rustc-link-lib=static=snmallocshim-static");
    println!("cargo:rustc-link-lib=static=rego");
    if cfg!(windows) {
        println!("cargo:rustc-link-arg=mincore.lib");
    } else {
        println!("cargo:rustc-link-lib=stdc++");
    }
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
