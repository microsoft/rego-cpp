use std::env;
use std::path::PathBuf;

fn main() {
  let libdir_path = PathBuf::from("rego/lib")
    .canonicalize()
    .expect("cannot canonicalize path");

  let headers_path = PathBuf::from("rego/include/rego/rego_c.h")
    .canonicalize()
    .expect("cannot canonicalize path");
  let headers_path_str = headers_path.to_str().expect("Path is not a valid string");

  println!("cargo:rustc-link-search={}", libdir_path.to_str().unwrap());
  println!("cargo:rustc-link-lib=static=rego");
  println!("cargo:rerun-if-changed={}", headers_path.to_str().unwrap());

  let bindings = bindgen::Builder::default()
    .header(headers_path_str)
    .parse_callbacks(Box::new(bindgen::CargoCallbacks))
    .generate()
    .expect("Unable to generate bindings");

  let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
  bindings.write_to_file(out_path.join("bindings.rs")).expect("Couldn't write bindings!");
}