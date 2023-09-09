use std::ffi::{CStr, CString};
use std::ops::{Drop, Index};
use std::path::Path;

pub struct Interpreter {
    c_ptr: *mut crate::regoInterpreter,
}

pub struct Output {
    c_ptr: *mut crate::regoOutput,
}

#[derive(Clone)]
pub struct Node {
    c_ptr: *mut crate::regoNode,
    children: Vec<Node>,
    pub size: usize,
    pub kind: NodeKind,
}

pub struct NodeIterator {
    pub node: Node,
    pub index: usize,
}

pub fn set_logging_enabled(enabled: bool) {
    let c_enabled: crate::regoBoolean = if enabled { 1 } else { 0 };
    unsafe {
        crate::regoSetLoggingEnabled(c_enabled);
    }
}

impl Interpreter {
    pub fn new() -> Self {
        let interpreter_ptr = unsafe { crate::regoNew() };
        Self {
            c_ptr: interpreter_ptr,
        }
    }

    pub fn get_error(&self) -> &str {
        let c_str = unsafe { crate::regoGetError(self.c_ptr) };
        let result: &CStr = unsafe { CStr::from_ptr(c_str) };
        result.to_str().unwrap()
    }

    pub fn add_module_file(&self, path: &Path) -> Result<(), &str> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let result = unsafe { crate::regoAddModuleFile(self.c_ptr, path_ptr) };
        if result == 0 {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    pub fn add_module(&self, name: &str, source: &str) -> Result<(), &str> {
        let name_cstr = CString::new(name).unwrap();
        let name_ptr = name_cstr.as_ptr();
        let source_cstr = CString::new(source).unwrap();
        let source_ptr = source_cstr.as_ptr();
        let result = unsafe { crate::regoAddModule(self.c_ptr, name_ptr, source_ptr) };
        if result == 0 {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    pub fn add_data_json_file(&self, path: &Path) -> Result<(), &str> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let result = unsafe { crate::regoAddDataJSONFile(self.c_ptr, path_ptr) };
        if result == 0 {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    pub fn add_data_json(&self, data: &str) -> Result<(), &str> {
        let data_cstr = CString::new(data).unwrap();
        let data_ptr = data_cstr.as_ptr();
        let result = unsafe { crate::regoAddDataJSON(self.c_ptr, data_ptr) };
        if result == 0 {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    pub fn add_input_json_file(&self, path: &Path) -> Result<(), &str> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let result = unsafe { crate::regoAddInputJSONFile(self.c_ptr, path_ptr) };
        if result == 0 {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    pub fn add_input_json(&self, input: &str) -> Result<(), &str> {
        let input_cstr = CString::new(input).unwrap();
        let input_ptr = input_cstr.as_ptr();
        let result = unsafe { crate::regoAddInputJSON(self.c_ptr, input_ptr) };
        if result == 0 {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    pub fn set_debug_enabled(&self, enabled: bool) {
        let c_enabled: crate::regoBoolean = if enabled { 1 } else { 0 };
        unsafe {
            crate::regoSetDebugEnabled(self.c_ptr, c_enabled);
        }
    }

    pub fn get_debug_enabled(&self) -> bool {
        let c_enabled = unsafe { crate::regoGetDebugEnabled(self.c_ptr) };
        c_enabled == 1
    }

    pub fn get_debug_path(&self) -> &str {
        let c_str = unsafe { crate::regoGetDebugPath(self.c_ptr) };
        let result: &CStr = unsafe { CStr::from_ptr(c_str) };
        result.to_str().unwrap()
    }

    pub fn set_debug_path(&self, path: &Path) -> Result<(), &str> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let result = unsafe { crate::regoSetDebugPath(self.c_ptr, path_ptr) };
        if result == 0 {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    pub fn set_well_formed_checks_enabled(&self, enabled: bool) {
        let c_enabled: crate::regoBoolean = if enabled { 1 } else { 0 };
        unsafe {
            crate::regoSetWellFormedChecksEnabled(self.c_ptr, c_enabled);
        }
    }

    pub fn get_well_formed_checks_enabled(&self) -> bool {
        let c_enabled = unsafe { crate::regoGetWellFormedChecksEnabled(self.c_ptr) };
        c_enabled == 1
    }

    pub fn query(&self, query: &str) -> Result<Output, &str> {
        let query_cstr = CString::new(query).unwrap();
        let query_ptr = query_cstr.as_ptr();
        let output_ptr = unsafe { crate::regoQuery(self.c_ptr, query_ptr) };
        if output_ptr == std::ptr::null_mut() {
            Err(self.get_error())
        } else {
            Ok(Output { c_ptr: output_ptr })
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

impl Drop for Output {
    fn drop(&mut self) {
        unsafe {
            crate::regoFreeOutput(self.c_ptr);
        }
    }
}

impl Output {
    pub fn ok(&self) -> bool {
        let c_ok = unsafe { crate::regoOutputOk(self.c_ptr) };
        c_ok == 1
    }

    pub fn to_str(&self) -> Result<&str, &str> {
        let c_str = unsafe { crate::regoOutputString(self.c_ptr) };
        let result: &CStr = unsafe { CStr::from_ptr(c_str) };
        let output_str = result.to_str().unwrap();
        if self.ok() {
            Ok(output_str)
        } else {
            Err(output_str)
        }
    }

    pub fn to_node(&self) -> Result<Node, Node> {
        let node_ptr = unsafe { crate::regoOutputNode(self.c_ptr) };
        let output = Node::new(node_ptr);
        if self.ok() {
            Ok(output)
        } else {
            Err(output)
        }
    }

    pub fn binding(&self, name: &str) -> Result<Node, ()> {
        if !self.ok() {
            return Err(());
        }

        let name_cstr = CString::new(name).unwrap();
        let name_ptr = name_cstr.as_ptr();
        let node_ptr = unsafe { crate::regoOutputBinding(self.c_ptr, name_ptr) };

        if node_ptr == std::ptr::null_mut() {
            Err(())
        } else {
            Ok(Node::new(node_ptr))
        }
    }
}

#[derive(Clone)]
pub enum NodeKind {
    Undefined,
    Binding,
    Var,
    Term,
    Scalar,
    Array,
    Set,
    Object,
    ObjectItem,
    Int,
    Float,
    String,
    True,
    False,
    Null,
    Error,
    ErrorMessage,
    ErrorAst,
    ErrorCode,
    Internal,
}

impl NodeKind {
    fn new(c_kind: crate::regoEnum) -> Self {
        match c_kind {
            crate::REGO_NODE_UNDEFINED => NodeKind::Undefined,
            crate::REGO_NODE_BINDING => NodeKind::Binding,
            crate::REGO_NODE_VAR => NodeKind::Var,
            crate::REGO_NODE_TERM => NodeKind::Term,
            crate::REGO_NODE_SCALAR => NodeKind::Scalar,
            crate::REGO_NODE_ARRAY => NodeKind::Array,
            crate::REGO_NODE_SET => NodeKind::Set,
            crate::REGO_NODE_OBJECT => NodeKind::Object,
            crate::REGO_NODE_OBJECT_ITEM => NodeKind::ObjectItem,
            crate::REGO_NODE_INT => NodeKind::Int,
            crate::REGO_NODE_FLOAT => NodeKind::Float,
            crate::REGO_NODE_STRING => NodeKind::String,
            crate::REGO_NODE_TRUE => NodeKind::True,
            crate::REGO_NODE_FALSE => NodeKind::False,
            crate::REGO_NODE_NULL => NodeKind::Null,
            crate::REGO_NODE_ERROR => NodeKind::Error,
            crate::REGO_NODE_ERROR_MESSAGE => NodeKind::ErrorMessage,
            crate::REGO_NODE_ERROR_AST => NodeKind::ErrorAst,
            crate::REGO_NODE_ERROR_CODE => NodeKind::ErrorCode,
            crate::REGO_NODE_INTERNAL => NodeKind::Internal,
            _ => panic!("Unknown node kind"),
        }
    }
}

#[derive(PartialEq, Debug)]
pub enum NodeValue {
    Var(String),
    Int(i64),
    Float(f64),
    String(String),
    Bool(bool),
    Null,
}

impl Node {
    pub fn new(c_ptr: *mut crate::regoNode) -> Self {
        let num_children = unsafe { crate::regoNodeSize(c_ptr) };
        let mut children = Vec::with_capacity(num_children as usize);
        for i in 0..num_children {
            let child_ptr = unsafe { crate::regoNodeGet(c_ptr, i) };
            children.push(Node::new(child_ptr));
        }
        let kind = unsafe { crate::regoNodeType(c_ptr) };
        Self {
            c_ptr,
            children,
            size: num_children as usize,
            kind: NodeKind::new(kind),
        }
    }

    pub fn kind_name(&self) -> &str {
        let c_str = unsafe { crate::regoNodeTypeName(self.c_ptr) };
        let result: &CStr = unsafe { CStr::from_ptr(c_str) };
        result.to_str().unwrap()
    }

    fn scalar_value(&self) -> Result<String, &str> {
        let size = unsafe { crate::regoNodeValueSize(self.c_ptr) };

        let mut v = vec![0 as i8; size as usize];
        let v_ptr = v.as_mut_ptr();
        let err: crate::regoEnum = unsafe { crate::regoNodeValue(self.c_ptr, v_ptr, size) };
        if err != crate::REGO_OK {
            return Err("Error getting node value");
        } else {
            let result: CString = unsafe { CString::from_raw(v_ptr) };
            Ok(result.into_string().unwrap())
        }
    }

    pub fn value(&self) -> Option<NodeValue> {
        match self.kind {
            NodeKind::Term => self.children[0].value(),
            NodeKind::Scalar => self.children[0].value(),
            NodeKind::Null => Some(NodeValue::Null),
            NodeKind::True => Some(NodeValue::Bool(true)),
            NodeKind::False => Some(NodeValue::Bool(false)),
            NodeKind::Var => self.scalar_value().ok().map(|s| NodeValue::Var(s)),
            NodeKind::Int => self
                .scalar_value()
                .ok()
                .map(|s| NodeValue::Int(s.parse().unwrap())),
            NodeKind::Float => self
                .scalar_value()
                .ok()
                .map(|s| NodeValue::Float(s.parse().unwrap())),
            NodeKind::String => self
                .scalar_value()
                .ok()
                .map(|s| NodeValue::String(s[1..s.len() - 1].to_string())),
            _ => None,
        }
    }

    pub fn json(&self) -> Result<String, &str> {
        let size = unsafe { crate::regoNodeJSONSize(self.c_ptr) };

        let mut v = vec![0 as i8; size as usize];
        let v_ptr = v.as_mut_ptr();
        let err: crate::regoEnum = unsafe { crate::regoNodeJSON(self.c_ptr, v_ptr, size) };
        if err != crate::REGO_OK {
            return Err("Error converting node to JSON");
        } else {
            let result: CString = unsafe { CString::from_raw(v_ptr) };
            let json = result.into_string().unwrap();
            if json.starts_with('"') && json.ends_with('"') {
                return Ok(json[1..json.len() - 1].to_string());
            } else {
                return Ok(json);
            }
        }
    }

    pub fn size(&self) -> usize {
        let c_size = unsafe { crate::regoNodeSize(self.c_ptr) };
        c_size as usize
    }

    fn lookup_object(&self, key: &str) -> Result<&Node, &str> {
        for child in self.children.iter() {
            let key_json = child.children[0].json()?;
            if key_json == key {
                return Ok(&child.children[1]);
            }
        }

        Err("key not found")
    }

    fn lookup_set(&self, item: &str) -> Result<&Node, &str> {
        for child in self.children.iter() {
            let item_json = child.json()?;
            if item_json == item {
                return Ok(&child);
            }
        }

        Err("item not found")
    }

    pub fn lookup(&self, key: &str) -> Result<&Node, &str> {
        match self.kind {
            NodeKind::Term => self.children[0].lookup(key),
            NodeKind::Object => self.lookup_object(key),
            NodeKind::Set => self.lookup_set(key),
            _ => Err("Must be an object or a set"),
        }
    }

    pub fn index(&self, index: usize) -> Result<&Node, &str> {
        match self.kind {
            NodeKind::Term => self.children[0].index(index),
            NodeKind::Array => Ok(&self.children[index]),
            _ => Err("Must be an array"),
        }
    }

    pub fn at(&self, index: usize) -> Result<&Node, &str> {
        if index >= self.size() {
            return Err("Index out of bounds");
        }

        Ok(&self.children[index])
    }

    pub fn iter(&self) -> std::slice::Iter<'_, Node> {
        return self.children.iter();
    }
}

impl Index<usize> for Node {
    type Output = Node;

    fn index(&self, index: usize) -> &Self::Output {
        &self.children[index]
    }
}
