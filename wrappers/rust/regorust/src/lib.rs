#![allow(non_camel_case_types)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub mod rego;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn query_math() {
        let rego = rego::Interpreter::new();
        match rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4") {
            Ok(result) => {
                let x = result.binding("x").expect("cannot get x");
                let y = result.binding("y").expect("cannot get y");
                assert_eq!(x.json().expect("cannot convert x to JSON"), "5");
                assert_eq!(y.json().expect("cannot convert y to JSON"), "9.4");
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        }
    }

    #[test]
    fn input_data() {
        let input = include_str!("../rego/examples/input0.json");
        let data0 = include_str!("../rego/examples/data0.json");
        let data1 = include_str!("../rego/examples/data1.json");
        let module = include_str!("../rego/examples/objects.rego");
        let rego = rego::Interpreter::new();
        rego.add_input_json(input).expect("cannot add input");
        rego.add_data_json(data0)
            .expect("cannot add data as string");
        rego.add_data_json(data1).expect("cannot add data file");
        rego.add_module("objects", module)
            .expect("cannot add module");
        match rego.query("x=[data.one, input.b, data.objects.sites[1]]") {
            Ok(result) => match result.to_str() {
                Ok(val) => {
                    assert_eq!(val, "x = [{\"bar\":\"Foo\", \"baz\":5, \"be\":true, \"bop\":23.4}, \"20\", {\"name\":\"smoke1\"}]\n")
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
