#![allow(non_camel_case_types)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub mod rego;

#[cfg(test)]
mod tests {

}