#![allow(non_camel_case_types)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub mod rego;

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;

    #[test]
    fn input_data() {
        let input = PathBuf::from("../rego/examples/input0.json")
            .canonicalize()
            .expect("cannot canonicalize path");
        let data0 = include_str!("../rego/examples/data0.json");
        let data1 = PathBuf::from("../rego/examples/data1.json")
            .canonicalize()
            .expect("cannot canonicalize path");
        let module = include_str!("../rego/examples/objects.rego");
        let rego = rego::Interpreter::new();
        rego.add_input_json_file(input.as_path())
            .expect("cannot add input file");
        rego.add_data_json(data0)
            .expect("cannot add data as string");
        rego.add_data_json_file(data1.as_path())
            .expect("cannot add data file");
        rego.add_module("objects", module)
            .expect("cannot add module as string");
        match rego.query("x=[[data.one, input.b, data.objects.sites[1]] = x") {
            Ok(result) => match result.to_str() {
                Ok(val) => {
                    assert_eq!(val, "x=[{\"bar\":\"Foo\", \"baz\":5, \"be\":true, \"bop\":23.4}, \"20\", {\"name\":\"smoke1\"}]")
                }
                Err(err) => {
                    panic!("unexpected result: {}", err);
                }
            },
            Err(e) => {
                panic!("error: {}", e);
            }
        }
    }
}
