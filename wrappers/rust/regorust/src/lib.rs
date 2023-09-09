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
            Ok(result) => {
                let x = result.binding("x").expect("cannot get x");
                let data_one = x.index(0).expect("cannot get data.one");
                if let rego::NodeValue::String(bar) = data_one
                    .lookup("bar")
                    .expect("bar key missing")
                    .value()
                    .expect("bar value missing")
                {
                    assert_eq!(bar, "Foo");
                } else {
                    panic!("bar is not a string");
                }

                if let rego::NodeValue::Bool(be) = data_one
                    .lookup("be")
                    .expect("be key missing")
                    .value()
                    .expect("be value missing")
                {
                    assert_eq!(be, true);
                } else {
                    panic!("be is not a bool");
                }

                if let rego::NodeValue::Int(baz) = data_one
                    .lookup("baz")
                    .expect("baz key missing")
                    .value()
                    .expect("baz value missing")
                {
                    assert_eq!(baz, 5);
                } else {
                    panic!("baz is not an int");
                }

                if let rego::NodeValue::Float(bop) = data_one
                    .lookup("bop")
                    .expect("bop key missing")
                    .value()
                    .expect("bop value missing")
                {
                    assert_eq!(bop, 23.4);
                } else {
                    panic!("bop is not a float");
                }

                if let rego::NodeValue::String(input_b) = x
                    .index(1)
                    .expect("cannot get input.b")
                    .value()
                    .expect("input.b value missing")
                {
                    assert_eq!(input_b, "20");
                } else {
                    panic!("input.b is not a string");
                }

                let data_objects_sites_1 = x.index(2).expect("cannot get data.objects.sites[1]");
                if let rego::NodeValue::String(name) = data_objects_sites_1
                    .lookup("name")
                    .expect("name key missing")
                    .value()
                    .expect("name value missing")
                {
                    assert_eq!(name, "smoke1");
                } else {
                    panic!("name is not a string");
                }
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        }
    }
}
