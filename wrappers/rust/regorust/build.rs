use std::env;
use std::num::NonZeroUsize;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    let mut regocpp_path = out_path.join("rego-cpp");
    let num_procs = std::thread::available_parallelism().unwrap_or(NonZeroUsize::new(1).unwrap());
    let rego_git_repo = std::env::var("REGOCPP_REPO")
        .unwrap_or("https://github.com/microsoft/rego-cpp.git".to_string());
    if rego_git_repo == "LOCAL" {
        regocpp_path =
            PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR not set"))
                .ancestors()
                .nth(3)
                .map(|path| path.to_path_buf())
                .expect("unable to resolve repository root");
    } else {
        let rego_git_tag = std::env::var("REGOCPP_TAG").unwrap_or("main".to_string());

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
    }

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

    let crypto_backend = if cfg!(windows) { "bcrypt" } else { "openssl3" };

    let mut cmake_args = vec![
        "-S".to_string(),
        ".".to_string(),
        "-B".to_string(),
        "build".to_string(),
        format!("-DCMAKE_BUILD_TYPE={}", regocpp_config),
        "-DCMAKE_INSTALL_PREFIX=rust".to_string(),
        "-DREGOCPP_COPY_EXAMPLES=ON".to_string(),
        format!("-DREGOCPP_CRYPTO_BACKEND={}", crypto_backend),
    ];

    if let Ok(openssl_root) = env::var("OPENSSL_ROOT_DIR") {
        cmake_args.push(format!("-DOPENSSL_ROOT_DIR={}", openssl_root));
    }

    Command::new("cmake")
        .args(&cmake_args)
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
    let header_path = rego_path.join("include").join("rego").join("rego_c.h");
    let header_path_str = header_path.to_str().unwrap();

    println!("cargo:rustc-link-search={}", libdir_path.to_str().unwrap());
    println!("cargo:rustc-link-lib=static:+whole-archive=trieste-json");
    println!("cargo:rustc-link-lib=static:+whole-archive=trieste-yaml");
    println!("cargo:rustc-link-lib=static=rego");
    if cfg!(windows) {
        println!("cargo:rustc-link-lib=static:+whole-archive=snmalloc-new-override");
        println!("cargo:rustc-link-arg=mincore.lib");
        println!("cargo:rustc-link-arg=shell32.lib");
        // Windows bcrypt crypto backend
        println!("cargo:rustc-link-arg=bcrypt.lib");
        println!("cargo:rustc-link-arg=crypt32.lib");
    } else if cfg!(target_os = "macos") {
        println!("cargo:rustc-link-lib=c++");
        // OpenSSL dynamic libraries for crypto/JWT builtins
        if let Ok(openssl_root) = env::var("OPENSSL_ROOT_DIR") {
            println!("cargo:rustc-link-search={}/lib", openssl_root);
        }
        println!("cargo:rustc-link-lib=ssl");
        println!("cargo:rustc-link-lib=crypto");
    } else {
        println!("cargo:rustc-link-lib=static:+whole-archive=snmalloc-new-override");
        println!("cargo:rustc-link-lib=stdc++");
        // OpenSSL dynamic libraries for crypto/JWT builtins
        println!("cargo:rustc-link-lib=ssl");
        println!("cargo:rustc-link-lib=crypto");
    }
    println!("cargo:rerun-if-changed={}", header_path_str);

    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header(header_path_str)
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
