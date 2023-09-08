use std::ffi::{CStr, CString};
use std::path::Path;

pub struct Interpreter {
    c_ptr: *mut crate::regoInterpreter
}

pub struct Output {
    c_ptr: *mut crate::regoResult
}

pub struct Node {
    c_ptr: *mut crate::regoNode
}

fn set_logging_enabled(enabled: &bool) {
    crate::regoSetLoggingEnabled(enabled);
}

impl Interpreter {
    pub fn new() -> Self {
        let interpreter_ptr = unsafe {
            crate::regoNew()
        };
        Self {
            c_ptr: interpreter_ptr
        }
    }

    pub fn add_module_file(&self, path: &Path) -> Result<(), str>
    {

    }

    pub fn query(&self, query: &str) -> QueryResult {
        let query_cstr = CString::new(query).unwrap();
        let query_ptr = query_cstr.as_ptr();
        let result_ptr = unsafe {
            crate::regoQuery(self.c_ptr, query_ptr)
        };
        QueryResult {
            c_ptr: result_ptr
        }
    }
}

impl Drop for Interpreter {
    fn drop(&mut self) {
        unsafe {
            crate::regoFree(self.c_ptr);
        }
    }
}

impl Drop for QueryResult {
    fn drop(&mut self) {
        unsafe {
            crate::regoFreeResult(self.c_ptr);
        }
    }
}

impl QueryResult {
    pub fn to_str(&self) -> &str {
        let c_str = unsafe {
            crate::regoResultString(self.c_ptr)
        };
        let result: &CStr = unsafe {
            CStr::from_ptr(c_str)
        };
        result.to_str().unwrap()
    }

    pub fn to_node(&self) -> Node {
        let node_ptr = unsafe {
            crate::regoResultNode(self.c_ptr)
        };
        Node {
            c_ptr: node_ptr
        }
    }
}

impl Node {

}