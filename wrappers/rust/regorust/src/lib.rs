#![allow(non_camel_case_types)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use std::ffi::CString;
use std::fmt;
use std::ops::Index;
use std::path::Path;
use std::str;

/// Returns the build information as a string.
///
/// The string will be in the format
/// "{version} ({name}, {date}) {toolchain} on {platform}."
pub fn build_info() -> String {
    format!(
        "{} ({}, {}) {} on {}.",
        str::from_utf8(REGOCPP_VERSION).expect("cannot convert version to string"),
        str::from_utf8(REGOCPP_BUILD_NAME).expect("cannot convert build name to string"),
        str::from_utf8(REGOCPP_BUILD_DATE).expect("cannot convert build date to string"),
        str::from_utf8(REGOCPP_BUILD_TOOLCHAIN).expect("cannot convert build toolchain to string"),
        str::from_utf8(REGOCPP_PLATFORM).expect("cannot convert platform to string"),
    )
}

fn remove_quotes(s: String) -> String {
    if s.starts_with('"') && s.ends_with('"') {
        s[1..s.len() - 1].to_string()
    } else {
        s
    }
}

/// Interface for the Rego interpreter.
///
/// This wraps the Rego C API, and handles passing calls to
/// the C API and converting the results to Rust types.
///
/// # Examples
/// ```
/// # use regorust::*;
/// let rego = Interpreter::new();
/// match rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4") {
///   Ok(result) => {
///     let x = result.binding("x").expect("cannot get x");
///     let y = result.binding("y").expect("cannot get y");
///     println!("x = {}", x.json().unwrap());
///     println!("y = {}", y.json().unwrap());
///     # assert_eq!(x.json().expect("cannot convert x to JSON"), "5");
///     # assert_eq!(y.json().expect("cannot convert y to JSON"), "9.4");
///   }
///   Err(e) => {
///     panic!("error: {}", e);
///   }
/// }
/// ```
///
/// ```
/// # use regorust::*;
/// let input = r#"
///   {
///     "a": 10,
///     "b": "20",
///     "c": 30.0,
///     "d": true
///   }
/// "#;
/// let data0 = r#"
///   {
///     "one": {
///       "bar": "Foo",
///       "baz": 5,
///       "be": true,
///       "bop": 23.4
///     },
///     "two": {
///       "bar": "Bar",
///       "baz": 12.3,
///       "be": false,
///       "bop": 42
///     }
///   }
/// "#;
/// let data1 = r#"
///   {
///     "three": {
///       "bar": "Baz",
///       "baz": 15,
///       "be": true,
///       "bop": 4.23
///     }
///   }        
/// "#;
/// let module = r#"
///   package objects
///
///   rect := {`width`: 2, "height": 4}
///   cube := {"width": 3, `height`: 4, "depth": 5}
///   a := 42
///   b := false
///   c := null
///   d := {"a": a, "x": [b, c]}
///   index := 1
///   shapes := [rect, cube]
///   names := ["prod", `smoke1`, "dev"]
///   sites := [{"name": "prod"}, {"name": names[index]}, {"name": "dev"}]
///   e := {
///     a: "foo",
///     "three": c,
///     names[2]: b,
///     "four": d,
///   }
///   f := e["dev"]                
/// "#;
/// let rego = Interpreter::new();
/// rego.set_input_json(input);
/// rego.add_data_json(data0);
/// rego.add_data_json(data1);
/// rego.add_module("objects", module);
/// match rego.query("x=[data.one, input.b, data.objects.sites[1]]") {
///   Ok(result) => {
///     println!("{}", result.to_str().unwrap());
///     let x = result.binding("x").expect("cannot get x");
///     let data_one = x.index(0).unwrap();
///     if let NodeValue::String(bar) = data_one
///         .lookup("bar")
///         .unwrap()
///         .value()
///         .unwrap()
///     {
///       println!("data.one.bar = {}", bar);
///       # assert_eq!(bar, "Foo");
///     }
///   }
///   Err(e) => {
///    panic!("error: {}", e);
///   }
/// }
/// ```
#[derive(Debug)]
pub struct Interpreter {
    c_ptr: *mut regoInterpreter,
}

/// Interface for the Rego output.
///
/// Outputs can either be examined as strings, or inspected
/// using Rego Nodes. It is also possible to extract bindings
/// for specific variables.
///
/// # Examples
/// ```
/// # use regorust::*;
/// # let rego = Interpreter::new();
/// # match rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4") {
/// #   Ok(result) => {
/// match result.to_str() {
///   Ok(output_str) => {
///     println!("{}", output_str);
///   }
///   Err(err_str) => {
///     println!("error: {}", err_str);
///   }
/// }
/// #   }
/// #   Err(e) => {
/// #     panic!("error: {}", e);
/// #   }
/// # }
/// ```
///
/// ```
/// # use regorust::*;
/// # let rego = Interpreter::new();
/// # match rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4") {
/// #   Ok(result) => {
/// let x = result.binding("x").expect("cannot get x");
/// println!("x = {}", x.json().unwrap());
/// # assert_eq!(x.json().expect("cannot convert x to JSON"), "5");
/// #   }
/// #   Err(e) => {
/// #     panic!("error: {}", e);
/// #   }
/// # }
/// ```
#[derive(Debug)]
pub struct Output {
    c_ptr: *mut regoOutput,
}

/// Enumeration of different kinds of Rego Nodes that can be returned as output from a query.
///
/// The main kinds of nodes are:
/// * `Binding` - A variable binding. Has two children: a `Var` and a `Term`
/// * `Var` - A variable name. Has no children.
/// * `Term` - A term. Has one child: a `Scalar`, `Array`, `Set`, or `Object`
/// * `Scalar` - A scalar value. Has one child: a `String`, `Int`, `Float`, `True`, `False`, or `Null`
/// * `Array` - An array. Has zero or more `Term` children.
/// * `Set` - A set. Has zero or more `Term` children.
/// * `Object` - An object. Has zero or more `ObjectItem` children.
/// * `ObjectItem` - An object item. Has two `Term` children.
///
/// Errors are also represented as nodes:
/// * `Error` - An error. Has three children: an `ErrorMessage`, `ErrorAst`, and `ErrorCode`
/// * `ErrorMessage` - An error message.
/// * `ErrorAst` - A tree representing where in the input program the error occurred.
/// * `ErrorCode` - An error code.
/// * `ErrorSeq` - A sequence of errors. Has one or more `Error` children.
#[derive(Clone, Debug, Default)]
pub enum NodeKind {
    #[default]
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
    Terms,
    Bindings,
    Results,
    Result,
    Error,
    ErrorSeq,
    ErrorMessage,
    ErrorAst,
    ErrorCode,
    Internal,
}

/// Interface for Rego Node objects.
///
/// Rego Nodes are the basic building blocks of a Rego result. They
/// exist in a tree structure. Each node has a kind, which is one of
/// the variants of [`NodeKind`]. Each node also has zero or more
/// children, which are also nodes.
///
/// # Examples
/// ```
/// use regorust::*;
/// let rego = Interpreter::new();
/// match rego.query(r#"x={"a": 10, "b": "20", "c": [30.0, 60], "d": true, "e": null}"#) {
///   Ok(result) => {
///     let x = result.binding("x").expect("cannot get x");
///     println!("x = {}", x.json().unwrap());
///     if let NodeValue::Int(a) = x
///         .lookup("a")
///         .unwrap()
///         .value()
///         .unwrap()
///     {
///       println!("x['a'] = {}", a);
///       # assert_eq!(a, 10);
///     }
///
///     if let NodeValue::String(b) = x
///        .lookup("b")
///        .unwrap()
///        .value()
///        .unwrap()
///     {
///       println!("x['b'] = {}", b);
///       # assert_eq!(b, "20");
///     }
///
///     let c = x.lookup("c").unwrap();
///     if let NodeValue::Float(c0) = c.index(0)
///       .unwrap()
///       .value()
///       .unwrap()
///     {
///       println!("x['c'][0] = {}", c0);
///       # assert_eq!(c0, 30.0);
///     }
///
///     if let NodeValue::Int(c1) = c.index(1)
///       .unwrap()
///       .value()
///       .unwrap()
///     {
///       println!("x['c'][1] = {}", c1);
///       # assert_eq!(c1, 60);
///     }
///
///     if let NodeValue::Bool(d) = x
///       .lookup("d")
///       .unwrap()
///       .value()
///       .unwrap()
///     {
///       println!("x['d'] = {}", d);
///       # assert!(d);
///     }
///
///     if let NodeValue::Null = x
///       .lookup("e")
///       .unwrap()
///       .value()
///       .unwrap()
///     {
///       println!("x['e'] = null");
///     }
///   }
///   Err(e) => {
///     panic!("error: {}", e);
///   }
/// }
/// ```
#[derive(Debug)]
pub struct Node {
    c_ptr: *mut regoNode,
    children: Vec<Node>,
    /// The number of children of this node.
    pub size: usize,

    /// The kind of this node.
    pub kind: NodeKind,
}

/// Format types for serializing bundles.
///
/// JSON bundles are written as a directory
/// with a `plan.json` containing the compiled virtual documents and execution plans,
/// a `data.json` containing the base documents merged into a single JSON hierarchy, and
/// the module source files.
///
/// Binary bundles are a single file containing all the same information in a compact,
/// portable format, described in this specification (TODO link).
#[derive(Clone, Debug, Default)]
pub enum BundleFormat {
    #[default]
    JSON,
    Binary,
}

/// Representation of a compiled Rego bundle. This can be produced by the
/// [`Interpreter::build()`] method as an alternative to immediate evaluation.
#[derive(Debug)]
pub struct Bundle {
    c_ptr: *mut regoBundle,
}

impl Bundle {
    fn new(c_ptr: *mut regoBundle) -> Self {
        Self { c_ptr }
    }

    pub fn ok(&self) -> bool {
        let c_ok = unsafe { regoBundleOk(self.c_ptr) };
        c_ok == 1
    }

    pub fn to_node(&self) -> Result<Node, Node> {
        let node_ptr = unsafe { regoBundleNode(self.c_ptr) };
        let output = Node::new(node_ptr);
        if self.ok() {
            Ok(output)
        } else {
            Err(output)
        }
    }
}

impl Drop for Bundle {
    fn drop(&mut self) {
        unsafe {
            regoFreeBundle(self.c_ptr);
        }
    }
}

impl PartialEq for Bundle {
    fn eq(&self, other: &Self) -> bool {
        return self.c_ptr == other.c_ptr;
    }
}

/// The Input interface allows the creation of inputs to a policy without
/// requiring serialization to JSON. The interface is that of a stack,
/// in which values are pushed and then various operations are used to turn
/// terminal types into more complex ones like objects and arrays. When used,
/// the Input will provide the top of the stack to any downstream consumer (such as
/// [`Interpreter::set_input()`]).
///
/// # Examples
///
/// ## Object
/// ```
/// # use regorust::*;
/// let input = Input::new().str("a").int(10).objectitem()
///     .str("b").str("20").objectitem()
///     .str("c").float(30.0).objectitem()
///     .str("d").bool(true).objectitem()
///     .object(4).validate()
///     .expect("Unable to create input");
/// let rego = Interpreter::new();
/// rego.set_input(&input).expect("Unable to set input");
/// let result = rego.query("x=input.a").expect("Failed query");  
/// let x = result.binding("x").expect("cannot get x");
/// println!("x = {}", x.json().unwrap());
/// # assert_eq!(x.json().expect("cannot convert x to JSON"), "10");
/// ```
///
/// ## Array
/// ```
/// # use regorust::*;
/// let input = Input::new().int(1).int(2).int(3).int(4).array(4)
///     .validate().expect("Unable to create input");
/// let rego = Interpreter::new();
/// rego.set_input(&input).expect("Unable to set input");
/// let result = rego.query("x=input").expect("Failed query");
/// let x = result.binding("x").expect("cannot get x");
/// println!("x = {}", x.json().unwrap());
/// # assert_eq!(x.json().expect("cannot convert x to JSON"), "[1,2,3,4]");
/// ```
#[derive(Debug)]
pub struct Input {
    c_ptr: *mut regoInput,
}

impl Input {
    pub fn new() -> Self {
        let input_ptr = unsafe { regoNewInput() };
        Self { c_ptr: input_ptr }
    }

    /// Push an integer onto the stack.
    pub fn int(self, value: i64) -> Self {
        unsafe { regoInputInt(self.c_ptr, value) };
        self
    }

    /// Push a float onto the stack.
    pub fn float(self, value: f64) -> Self {
        unsafe { regoInputFloat(self.c_ptr, value) };
        self
    }

    /// Push a string onto the stack.
    pub fn str(self, value: &str) -> Self {
        let value_cstr = CString::new(value).unwrap();
        let value_ptr = value_cstr.as_ptr();
        unsafe { regoInputString(self.c_ptr, value_ptr) };
        self
    }

    /// Push a boolean onto the stack.
    pub fn bool(self, value: bool) -> Self {
        unsafe { regoInputBoolean(self.c_ptr, if value { 1 } else { 0 }) };
        self
    }

    /// Push a null onto the stack.
    pub fn null(self) -> Self {
        unsafe { regoInputNull(self.c_ptr) };
        self
    }

    /// Take the top two values on the stack and turn them into an object item.
    /// The penultimate value on the stack will be used as
    /// the key, and the top of the stack will be the value for that key.
    /// Objects are constructed from object items.
    ///
    /// # Example
    ///
    /// ```
    /// # use regorust::*;
    /// let input = Input::new().str("a").int(10).objectitem().object(1)
    ///     .validate().expect("Unable to create input");
    ///
    /// let rego = Interpreter::new();
    /// rego.set_input(&input).expect("Unable to set input");
    /// let result = rego.query("x=input").expect("Failed query");
    /// let x = result.binding("x").expect("cannot get x");
    /// println!("x = {}", x.json().unwrap());
    /// # assert_eq!(x.json().expect("cannot convert x to JSON"), r#"{"a":10}"#);
    /// ```
    pub fn objectitem(self) -> Self {
        unsafe { regoInputObjectItem(self.c_ptr) };
        self
    }

    /// Take the top `size` values on the stack and turn them into an object.
    /// Note that all of these values must be object items in order for this to be valid.
    pub fn object(self, size: regoSize) -> Self {
        unsafe { regoInputObject(self.c_ptr, size) };
        self
    }

    /// Take the top `size` values on the stack and turn them into an array.
    /// Stack order will be used.
    ///
    /// # Example
    /// ```
    /// # use regorust::*;
    /// let input = Input::new().int(1).int(2).int(3).int(4).array(4)
    ///     .validate().expect("Unable to create input");
    /// let rego = Interpreter::new();
    /// rego.set_input(&input).expect("Unable to set input");
    /// let result = rego.query("x=input").expect("Failed query");
    /// let x = result.binding("x").expect("cannot get x");
    /// println!("x = {}", x.json().unwrap());
    /// # assert_eq!(x.json().expect("cannot convert x to JSON"), "[1,2,3,4]");
    /// ```
    pub fn array(self, size: regoSize) -> Self {
        unsafe { regoInputArray(self.c_ptr, size) };
        self
    }

    /// Take the top `size` values on the stack and turn them into a set.
    /// Identical items will be de-duplicated.
    ///
    /// # Example
    /// ```
    /// # use regorust::*;
    /// let input = Input::new().int(1).int(2).int(2).int(3).set(4)
    ///     .validate().expect("Unable to create input");
    /// let rego = Interpreter::new();
    /// rego.set_input(&input).expect("Unable to set input");
    /// let result = rego.query("x=input").expect("Failed query");
    /// let x = result.binding("x").expect("cannot get x");
    /// println!("x = {}", x.json().unwrap());
    /// # assert_eq!(x.json().expect("cannot convert x to JSON"), "[1,2,3]");
    /// ```
    pub fn set(self, size: regoSize) -> Self {
        unsafe { regoInputSet(self.c_ptr, size) };
        self
    }

    pub fn validate(self) -> Result<Self, &'static str> {
        let status = unsafe { regoInputValidate(self.c_ptr) };
        match status {
            REGO_OK => Ok(self),
            REGO_ERROR_INPUT_NULL => Err("Input pointer is null"),
            REGO_ERROR_INPUT_MISSING_ARGUMENTS => {
                Err("Not enough arguments for object item (push a key and a value)")
            }
            REGO_ERROR_INPUT_OBJECT_ITEM => Err("Objects must be built from object items"),
            _ => Err("Unknown error"),
        }
    }
}

impl Drop for Input {
    fn drop(&mut self) {
        unsafe {
            regoFreeInput(self.c_ptr);
        }
    }
}

impl PartialEq for Input {
    fn eq(&self, other: &Self) -> bool {
        return self.c_ptr == other.c_ptr;
    }
}

/// Represents the value of a Rego Node.
///
/// The value of a node is only valid for certain kinds of nodes,
/// namely `Int`, `Float`, `String`, `True`, `False`, and `Null`.
/// It will automatically unpack `Scalar` and `Term` nodes which
/// contain these kinds of values.
#[derive(PartialEq, Debug)]
pub enum NodeValue {
    Var(String),
    Int(i64),
    Float(f64),
    String(String),
    Bool(bool),
    Null,
}

pub enum LogLevel {
    None,
    Debug,
    Info,
    Warn,
    Error,
    Output,
    Trace,
}

pub fn log_level_from_string(level: &str) -> Result<LogLevel, &'static str> {
    match level.to_lowercase().as_str() {
        "none" => Ok(LogLevel::None),
        "debug" => Ok(LogLevel::Debug),
        "info" => Ok(LogLevel::Info),
        "warn" => Ok(LogLevel::Warn),
        "error" => Ok(LogLevel::Error),
        "output" => Ok(LogLevel::Output),
        "trace" => Ok(LogLevel::Trace),
        _ => Err("Invalid log level"),
    }
}

impl Interpreter {
    /// Creates a new Rego interpreter.
    pub fn new() -> Self {
        let interpreter_ptr = unsafe { regoNew() };
        Self {
            c_ptr: interpreter_ptr,
        }
    }

    /// Returns the error message for the last operation.
    fn get_error(&self) -> String {
        let size = unsafe { regoErrorSize(self.c_ptr) };

        let buf = vec![32 as u8; (size as usize) - 1];
        let input_cstr = CString::new(buf).unwrap();
        let input_ptr = CString::into_raw(input_cstr);
        let err: regoEnum = unsafe { regoError(self.c_ptr, input_ptr, size) };
        if err != REGO_OK {
            return "Unknown error: unable to obtain error message".to_string();
        } else {
            let output_cstr: CString = unsafe { CString::from_raw(input_ptr) };
            output_cstr.into_string().unwrap()
        }
    }

    /// Sets the level of logging produced by the library.
    pub fn set_log_level(&self, log_level: LogLevel) -> Result<(), &'static str> {
        let level = match log_level {
            LogLevel::None => REGO_LOG_LEVEL_NONE,
            LogLevel::Debug => REGO_LOG_LEVEL_DEBUG,
            LogLevel::Info => REGO_LOG_LEVEL_INFO,
            LogLevel::Warn => REGO_LOG_LEVEL_WARN,
            LogLevel::Error => REGO_LOG_LEVEL_ERROR,
            LogLevel::Output => REGO_LOG_LEVEL_OUTPUT,
            LogLevel::Trace => REGO_LOG_LEVEL_TRACE,
        };
        let result: regoEnum = unsafe { regoSetLogLevel(self.c_ptr, level) };

        match result {
            REGO_OK => Ok(()),
            REGO_LOG_LEVEL_ERROR => Err("Invalid log level"),
            _ => Err("Unknown error"),
        }
    }

    /// Sets the level of logging produced by the library.
    pub fn get_log_level(&self) -> Result<LogLevel, &'static str> {
        let result: regoEnum = unsafe { regoGetLogLevel(self.c_ptr) };

        match result {
            REGO_LOG_LEVEL_NONE => Ok(LogLevel::None),
            REGO_LOG_LEVEL_DEBUG => Ok(LogLevel::Debug),
            REGO_LOG_LEVEL_INFO => Ok(LogLevel::Info),
            REGO_LOG_LEVEL_WARN => Ok(LogLevel::Warn),
            REGO_LOG_LEVEL_ERROR => Ok(LogLevel::Error),
            REGO_LOG_LEVEL_OUTPUT => Ok(LogLevel::Output),
            REGO_LOG_LEVEL_TRACE => Ok(LogLevel::Trace),
            _ => Err("Unknown error"),
        }
    }

    /// Adds a Rego module from a file.
    ///
    /// The file must be valid Rego, and is equivalent to adding
    /// the module as a string using `add_module`.
    ///
    /// # Example module file
    /// ```rego
    /// package scalars
    ///
    /// greeting := "Hello"
    /// max_height := 42
    /// pi := 3.14159
    /// allowed := true
    /// location := null
    /// ```
    pub fn add_module_file(&self, path: &Path) -> Result<(), String> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let result = unsafe { regoAddModuleFile(self.c_ptr, path_ptr) };
        if result == REGO_OK {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    /// Adds a Rego module from a string.
    ///
    /// # Example
    /// ```
    /// # use regorust::*;
    /// let module = r#"
    ///   package scalars
    ///
    ///   greeting := "Hello"
    ///   max_height := 42
    ///   pi := 3.14159
    ///   allowed := true
    ///   location := null
    /// "#;
    /// let rego = Interpreter::new();
    /// rego.add_module("scalars", module);
    /// let result = rego.query("data.scalars.greeting").unwrap();
    /// println!("{}", result.to_str().unwrap());
    /// # assert_eq!(result.to_str().unwrap(), r#"{"expressions":["Hello"]}"#);
    /// ```
    pub fn add_module(&self, name: &str, source: &str) -> Result<(), String> {
        let name_cstr = CString::new(name).unwrap();
        let name_ptr = name_cstr.as_ptr();
        let source_cstr = CString::new(source).unwrap();
        let source_ptr = source_cstr.as_ptr();
        let result = unsafe { regoAddModule(self.c_ptr, name_ptr, source_ptr) };
        if result == REGO_OK {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    /// Adds a base document from a file.
    ///
    /// The file must be a valid JSON object, and is equivalent to adding
    /// the document as a string using `add_data_json`.
    ///
    /// # Example document file
    /// ```json
    /// {
    ///    "one": {
    ///        "bar": "Foo",
    ///        "baz": 5,
    ///        "be": true,
    ///        "bop": 23.4
    ///    },
    ///    "two": {
    ///        "bar": "Bar",
    ///      "baz": 12.3,
    ///        "be": false,
    ///        "bop": 42
    ///    }
    /// }
    /// ```
    pub fn add_data_json_file(&self, path: &Path) -> Result<(), String> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let result = unsafe { regoAddDataJSONFile(self.c_ptr, path_ptr) };
        if result == REGO_OK {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    /// Adds a base document from a string.
    ///
    /// The string must contain a valid JSON object.
    ///
    /// # Example
    /// ```
    /// # use regorust::*;
    /// let data = r#"
    ///   {
    ///     "one": {
    ///       "bar": "Foo",
    ///       "baz": 5,
    ///       "be": true,
    ///       "bop": 23.4
    ///     },
    ///     "two": {
    ///       "bar": "Bar",
    ///       "baz": 12.3,
    ///       "be": false,
    ///       "bop": 42
    ///     }
    ///   }
    /// "#;
    /// let rego = Interpreter::new();
    /// rego.add_data_json(data);
    /// let result = rego.query("data.one.bar").unwrap();
    /// println!("{}", result.to_str().unwrap());
    /// # assert_eq!(result.to_str().unwrap(), r#"{"expressions":["Foo"]}"#);
    /// ```
    pub fn add_data_json(&self, data: &str) -> Result<(), String> {
        let data_cstr = CString::new(data).unwrap();
        let data_ptr = data_cstr.as_ptr();
        let result = unsafe { regoAddDataJSON(self.c_ptr, data_ptr) };
        if result == REGO_OK {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    /// Sets the input JSON expression from a file.
    ///
    /// The input must be a single valid JSON value. It is equivalent to adding
    /// the input file as a string using `add_input_json`.
    ///
    /// # Example input files
    /// ```json
    /// "Hello, rego"
    /// ```
    ///
    /// ```json
    /// 42
    /// ```
    ///
    /// ```json
    /// [1, 2, 3]
    /// ```
    ///
    /// ```json
    /// {"a": 1, "b": 2}
    /// ```
    pub fn set_input_json_file(&self, path: &Path) -> Result<(), String> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let result = unsafe { regoSetInputJSONFile(self.c_ptr, path_ptr) };
        if result == REGO_OK {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    /// Sets the input JSON expression from a string.
    ///
    /// The input must be a single valid JSON value.
    ///
    /// # Example input files
    ///
    /// ```json
    /// "Hello, rego"
    /// ```
    ///
    /// ```json
    /// 42
    /// ```
    ///
    /// ```json
    /// [1, 2, 3]
    /// ```
    ///
    /// ```json
    /// {"a": 1, "b": 2}
    /// ```
    ///
    /// # Example
    /// ```
    /// # use regorust::*;
    /// let input = r#"
    ///   {
    ///     "a": 10,
    ///     "b": "20",
    ///     "c": 30.0,
    ///     "d": true
    ///   }
    /// "#;
    /// let rego = Interpreter::new();
    /// rego.set_input_json(input);
    /// let result = rego.query("input.a").unwrap();
    /// println!("{}", result.to_str().unwrap());
    /// # assert_eq!(result.to_str().unwrap(), r#"{"expressions":[10]}"#);
    /// ```
    pub fn set_input_json(&self, input: &str) -> Result<(), String> {
        let input_cstr = CString::new(input).unwrap();
        let input_ptr = input_cstr.as_ptr();
        let result = unsafe { regoSetInputTerm(self.c_ptr, input_ptr) };
        if result == REGO_OK {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    /// Sets the input for the current policy directly.
    ///
    /// See [`Input`] for more information.
    pub fn set_input(&self, input: &Input) -> Result<(), String> {
        let result = unsafe { regoSetInput(self.c_ptr, input.c_ptr) };
        if result == REGO_OK {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    /// Sets whether the Rego interpreter is in debug mode.
    ///
    /// When debug mode is enabled, the Rego interpreter will output extensive
    /// debugging information about the compliation process, including intermediary
    /// ASTs and the generated bytecode. This is mostly useful for debugging the
    /// Rego compiler itself, and is not useful for debugging Rego policies.
    pub fn set_debug_enabled(&self, enabled: bool) {
        let c_enabled: regoBoolean = if enabled { 1 } else { 0 };
        unsafe {
            regoSetDebugEnabled(self.c_ptr, c_enabled);
        }
    }

    /// Returns whether the Rego interpreter is in debug mode.
    pub fn get_debug_enabled(&self) -> bool {
        let c_enabled = unsafe { regoGetDebugEnabled(self.c_ptr) };
        c_enabled == 1
    }

    /// Sets the path to the directory where the Rego interpreter will write debug AST files.
    ///
    /// This is only useful when debug mode is enabled.
    pub fn set_debug_path(&self, path: &Path) -> Result<(), String> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let result = unsafe { regoSetDebugPath(self.c_ptr, path_ptr) };
        if result == REGO_OK {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    /// Sets whether the Rego interpreter will perform well-formedness checks.
    ///
    /// When enabled, the Rego interpreter will perform well-formedness checks on
    /// the AST during each pass of the compiler.
    pub fn set_well_formed_checks_enabled(&self, enabled: bool) {
        let c_enabled: regoBoolean = if enabled { 1 } else { 0 };
        unsafe {
            regoSetWellFormedChecksEnabled(self.c_ptr, c_enabled);
        }
    }

    /// Returns whether the Rego interpreter will perform well-formedness checks.
    pub fn get_well_formed_checks_enabled(&self) -> bool {
        let c_enabled = unsafe { regoGetWellFormedChecksEnabled(self.c_ptr) };
        c_enabled == 1
    }

    /// Sets whether the interpreter will forward errors thrown by the built-ins.
    ///
    /// By default, the Rego interpreter will catch errors thrown by the built-ins
    /// and return them as an Undefined result. When this is enabled, the interpreter
    /// will instead forward the error to the caller.
    pub fn set_strict_built_in_errors(&self, enabled: bool) {
        let c_enabled: regoBoolean = if enabled { 1 } else { 0 };
        unsafe {
            regoSetStrictBuiltInErrors(self.c_ptr, c_enabled);
        }
    }

    /// Returns whether the interpreter will forward errors thrown by the built-ins.
    pub fn get_strict_built_in_errors(&self) -> bool {
        let c_enabled = unsafe { regoGetStrictBuiltInErrors(self.c_ptr) };
        c_enabled == 1
    }

    /// Returns whether the interpreter has a built-in with the given name.
    ///
    /// # Example
    /// ```
    /// # use regorust::*;
    /// let rego = Interpreter::new();
    /// println!("{}", rego.is_built_in("json.unmarshal"));
    /// # assert!(rego.is_built_in("json.unmarshal"));
    /// ```
    pub fn is_built_in(&self, name: &str) -> bool {
        let name_cstr = CString::new(name).unwrap();
        let name_ptr = name_cstr.as_ptr();
        let c_is_built_in = unsafe { regoIsAvailableBuiltIn(self.c_ptr, name_ptr) };
        c_is_built_in == 1
    }

    /// This method performs a query against the current set of base and virtual documents.
    ///
    /// While the Rego interpreter can be used to perform simple queries, in most cases
    /// users will want to load one or more base documents (using [`Self::add_data_json()`] or
    /// [`Self::add_data_json_file()`]) and one or more Rego modules (using [`Self::add_module()`] or
    /// [`Self::add_module_file()`]). Then, multiple queries can be performed by providing an input
    /// (using [`Self::set_input_json()`] or [`Self::set_input_json_file()`]) and then calling this
    /// method.
    ///
    /// # Example
    /// ```
    /// # use regorust::*;
    /// let input0 = r#"{"a": 10}"#;
    /// let input1 = r#"{"a": 4}"#;
    /// let input2 = r#"{"a": 7}"#;
    /// let multi = r#"
    ///   package multi
    ///   
    ///   default a := 0
    ///   
    ///   a := val {
    ///       input.a > 0
    ///       input.a < 10
    ///       input.a % 2 == 1
    ///       val := input.a * 10
    ///   } {
    ///       input.a > 0
    ///       input.a < 10
    ///       input.a % 2 == 0
    ///       val := input.a * 10 + 1
    ///   }
    ///   
    ///   a := input.a / 10 {
    ///       input.a >= 10
    ///   }
    /// "#;
    /// let rego = Interpreter::new();
    /// rego.add_module("multi", multi);
    /// rego.set_input_json(input0);
    /// let result = rego.query("data.multi.a").unwrap();
    /// println!("{}", result.to_str().unwrap());
    /// # assert_eq!(result.to_str().unwrap(), r#"{"expressions":[1]}"#);
    /// rego.set_input_json(input1);
    /// let result = rego.query("data.multi.a").unwrap();
    /// println!("{}", result.to_str().unwrap());
    /// # assert_eq!(result.to_str().unwrap(), r#"{"expressions":[41]}"#);
    /// rego.set_input_json(input2);
    /// let result = rego.query("data.multi.a").unwrap();
    /// println!("{}", result.to_str().unwrap());
    /// # assert_eq!(result.to_str().unwrap(), r#"{"expressions":[70]}"#);
    /// ```
    pub fn query(&self, query: &str) -> Result<Output, String> {
        let query_cstr = CString::new(query).unwrap();
        let query_ptr = query_cstr.as_ptr();
        let output_ptr = unsafe { regoQuery(self.c_ptr, query_ptr) };
        if output_ptr == std::ptr::null_mut() {
            Err(self.get_error())
        } else {
            Ok(Output::new(output_ptr))
        }
    }

    /// Build a bundle from the current base and virtual documents, along
    /// with `query` and the provided `entrypoints`. There must be at least
    /// one entrypoint or a query in order for the build to be successful.
    ///
    /// # Example
    ///
    /// ```
    /// # use regorust::*;
    /// let input0 = r#"{
    ///     "x": 104,
    ///     "y": 119
    /// }"#;
    /// let data = r#"{
    ///     "a": 7,
    ///     "b": 13
    /// }"#;
    /// let module = r#"
    ///  package example
    ///
    ///  foo := data.a * input.x + data.b * input.y
    ///  bar := data.b * input.x + data.a * input.y
    /// "#;
    /// let rego_build = Interpreter::new();
    /// rego_build.add_data_json(data);
    /// rego_build.add_module("example.rego", module);
    /// let bundle = rego_build.build(&Some("x=data.example.foo + data.example.bar"),
    ///                               &["example/foo", "example/bar"]).expect("Unable to build bundle");
    /// let rego_run = Interpreter::new();
    /// rego_run.set_input_json(input0);
    /// let result = rego_run.query_bundle(&bundle).expect("Failed bundle query");
    /// let x = result.binding("x").unwrap();
    /// println!("x = {}", x.json().unwrap());
    /// # assert_eq!(x.json().expect("cannot convert x to JSON"), "4460");
    ///
    /// let result = rego_run.query_bundle_entrypoint(&bundle, "example/foo").expect("Failed bundle entrypoint query");
    /// println!("{}", result.to_str().unwrap());
    /// # assert_eq!(result.to_str().unwrap(), r#"{"expressions":[2275]}"#);
    /// ```
    pub fn build<T: AsRef<str>>(
        &self,
        query: &Option<T>,
        entrypoints: &[T],
    ) -> Result<Bundle, String> {
        match query {
            Some(value) => {
                let query_cstr = CString::new(value.as_ref()).unwrap();
                let query_ptr = query_cstr.as_ptr();
                let result = unsafe { regoSetQuery(self.c_ptr, query_ptr) };
                if result != REGO_OK {
                    return Err(self.get_error());
                }
            }
            None => (),
        }

        for e in entrypoints {
            let e_cstr = CString::new(e.as_ref()).unwrap();
            let e_ptr = e_cstr.as_ptr();
            let result = unsafe { regoAddEntrypoint(self.c_ptr, e_ptr) };
            if result != REGO_OK {
                return Err(self.get_error());
            }
        }

        if entrypoints.is_empty() && query.is_none() {
            return Err("Must provide at least one entrypoint or a query".to_string());
        }

        let bundle_ptr = unsafe { regoBuild(self.c_ptr) };
        if bundle_ptr == std::ptr::null_mut() {
            Err(self.get_error())
        } else {
            Ok(Bundle::new(bundle_ptr))
        }
    }

    /// Loads a bundle from the disk.
    pub fn load_bundle(&self, path: &Path, format: BundleFormat) -> Result<Bundle, String> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let bundle_ptr = match format {
            BundleFormat::JSON => unsafe { regoBundleLoad(self.c_ptr, path_ptr) },
            BundleFormat::Binary => unsafe { regoBundleLoadBinary(self.c_ptr, path_ptr) },
        };

        if bundle_ptr == std::ptr::null_mut() {
            Err(self.get_error())
        } else {
            Ok(Bundle::new(bundle_ptr))
        }
    }

    /// Saves a bundle to the disk.
    ///
    /// There are two modes for bundle serialization. The JSON serialization scheme creates
    /// a directory at the specified path and then writes at least two files: `plan.json`
    /// (which contains the compiled virtual documents and plans for execution) and
    /// `data.json` (which contains the base documents merged into a single JSON hierarchy).
    /// Module source files will also be copied into the directory.
    ///
    /// The second mode is binary serialization. This uses the Rego Binary Bundle format
    /// (TODO URL) to create a single file which contains all the bundle information.
    ///
    /// # Example
    ///
    /// ```
    /// # use regorust::*;
    /// # use std::env;
    /// let rego_build = Interpreter::new();
    /// let bundle = rego_build.build(&Some("a=1"), &[]).expect("Failed to build bundle");
    /// let mut bundle_dir = env::temp_dir();
    /// bundle_dir.push("bundle");
    /// let mut bundle_file = env::temp_dir();
    /// bundle_file.push("bundle.rbb");
    /// rego_build.save_bundle(&bundle_dir, &bundle, BundleFormat::JSON).expect("Unable to save bundle");
    /// rego_build.save_bundle(&bundle_file, &bundle, BundleFormat::Binary).expect("Unable to save binary bundle");
    ///
    /// let rego_run = Interpreter::new();
    /// let bundle = rego_run.load_bundle(&bundle_dir, BundleFormat::JSON).expect("Unable to load bundle from disk");
    /// let result = rego_run.query_bundle(&bundle).expect("Failed bundle query");
    /// let a = result.binding("a").unwrap();
    /// println!("a = {}", a.json().unwrap());
    /// # assert_eq!(a.json().expect("cannot convert a to JSON"), "1");
    ///
    /// let bundle = rego_run.load_bundle(&bundle_file, BundleFormat::Binary).expect("Unable to load bundle from disk");
    /// let result = rego_run.query_bundle(&bundle).expect("Failed bundle query");
    /// let a = result.binding("a").unwrap();
    /// println!("a = {}", a.json().unwrap());
    /// # assert_eq!(a.json().expect("cannot convert a to JSON"), "1");
    /// ```
    pub fn save_bundle(
        &self,
        path: &Path,
        bundle: &Bundle,
        format: BundleFormat,
    ) -> Result<(), String> {
        let path_str = path.to_str().unwrap();
        let path_cstr = CString::new(path_str).unwrap();
        let path_ptr = path_cstr.as_ptr();
        let result = match format {
            BundleFormat::JSON => unsafe { regoBundleSave(self.c_ptr, path_ptr, bundle.c_ptr) },
            BundleFormat::Binary => unsafe {
                regoBundleSaveBinary(self.c_ptr, path_ptr, bundle.c_ptr)
            },
        };

        if result == REGO_OK {
            Ok(())
        } else {
            Err(self.get_error())
        }
    }

    /// Performs a query using the compiled policy in the bundle.
    ///
    /// This method requires that the entrypoint specified by `entrypoint`
    /// was provided to [`Interpreter::build()`]. Otherwise, it will fail.
    pub fn query_bundle_entrypoint(
        &self,
        bundle: &Bundle,
        entrypoint: &str,
    ) -> Result<Output, String> {
        let entrypoint_cstr = CString::new(entrypoint).unwrap();
        let entrypoint_ptr = entrypoint_cstr.as_ptr();
        let output_ptr =
            unsafe { regoBundleQueryEntrypoint(self.c_ptr, bundle.c_ptr, entrypoint_ptr) };

        if output_ptr == std::ptr::null_mut() {
            Err(self.get_error())
        } else {
            Ok(Output::new(output_ptr))
        }
    }

    /// Performs a query using the compiled policy in the bundle.
    ///
    /// This method requires that a query was provided to [`Interpreter::build()`].
    /// Otherwise, it will fail.
    pub fn query_bundle(&self, bundle: &Bundle) -> Result<Output, String> {
        let output_ptr = unsafe { regoBundleQuery(self.c_ptr, bundle.c_ptr) };

        if output_ptr == std::ptr::null_mut() {
            Err(self.get_error())
        } else {
            Ok(Output::new(output_ptr))
        }
    }
}

impl Drop for Interpreter {
    fn drop(&mut self) {
        unsafe {
            regoFree(self.c_ptr);
        }
    }
}

impl PartialEq for Interpreter {
    fn eq(&self, other: &Self) -> bool {
        return self.c_ptr == other.c_ptr;
    }
}

impl Drop for Output {
    fn drop(&mut self) {
        unsafe {
            regoFreeOutput(self.c_ptr);
        }
    }
}

impl Output {
    fn new(c_ptr: *mut regoOutput) -> Self {
        Self { c_ptr }
    }

    /// Returns whether the output is ok.
    ///
    /// The output of a successful query will always be a Node. However, if there
    /// was an error that arose with the Rego engine, then this Node will be of
    /// kind [`NodeKind::ErrorSeq`] and contain one or more errors. This method
    /// gives a quick way, without inspecting the result node, of finding whether
    /// it is ok.
    pub fn ok(&self) -> bool {
        let c_ok = unsafe { regoOutputOk(self.c_ptr) };
        c_ok == 1
    }

    /// Returns the output as a JSON-encoded string.
    ///
    /// If the result of [`Self::ok()`] is false, the result will be an string
    /// containing error information.
    pub fn to_str(&self) -> Result<String, String> {
        let size = unsafe { regoOutputJSONSize(self.c_ptr) };

        let buf = vec![32 as u8; (size as usize) - 1];
        let input_cstr = CString::new(buf).unwrap();
        let input_ptr = CString::into_raw(input_cstr);
        let err: regoEnum = unsafe { regoOutputJSON(self.c_ptr, input_ptr, size) };
        if err != REGO_OK {
            return Err("Error obtaining output string".to_string());
        } else {
            let output_cstr = unsafe { CString::from_raw(input_ptr) };
            let output = output_cstr.into_string().unwrap();
            return Ok(output);
        }
    }

    /// Returns the output node.
    ///
    /// The value of the [`Result`] enum will reflect the output of
    /// [`Self::ok()`].
    pub fn to_node(&self) -> Result<Node, Node> {
        let node_ptr = unsafe { regoOutputNode(self.c_ptr) };
        let output = Node::new(node_ptr);
        if self.ok() {
            Ok(output)
        } else {
            Err(output)
        }
    }

    /// Attempts to return the binding for the given variable name.
    ///
    /// If the output is not OK or the variable is not bound, then
    /// this will return an [`Result::Err`] with the output node.
    /// Otherwise it will return the bound value for the variable.
    pub fn binding_at_index(&self, index: regoSize, name: &str) -> Result<Node, Node> {
        if !self.ok() {
            return self.to_node();
        }

        let name_cstr = CString::new(name).unwrap();
        let name_ptr = name_cstr.as_ptr();
        let node_ptr = unsafe { regoOutputBindingAtIndex(self.c_ptr, index, name_ptr) };

        if node_ptr == std::ptr::null_mut() {
            self.to_node()
        } else {
            Ok(Node::new(node_ptr))
        }
    }

    /// Attempts to return the binding for the given variable name.
    ///
    /// If the output is not OK or the variable is not bound, then
    /// this will return an [`Result::Err`] with the output node.
    /// Otherwise it will return the bound value for the variable.
    pub fn binding(&self, name: &str) -> Result<Node, Node> {
        self.binding_at_index(0, name)
    }

    /// Attempts to return the expressions at a given result index.
    pub fn expressions_at_index(&self, index: regoSize) -> Result<Node, Node> {
        if !self.ok() {
            return self.to_node();
        }

        let node_ptr = unsafe { regoOutputExpressionsAtIndex(self.c_ptr, index) };

        if node_ptr == std::ptr::null_mut() {
            self.to_node()
        } else {
            Ok(Node::new(node_ptr))
        }
    }

    /// Attempts to return the expressions at a given result index.
    pub fn expressions(&self) -> Result<Node, Node> {
        self.expressions_at_index(0)
    }

    /// Returns the number of results in the output.
    pub fn size(&self) -> regoSize {
        unsafe { regoOutputSize(self.c_ptr) }
    }
}

impl PartialEq for Output {
    fn eq(&self, other: &Self) -> bool {
        return self.c_ptr == other.c_ptr;
    }
}

impl fmt::Display for Output {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.to_str() {
            Ok(output_str) => write!(f, "{}", output_str),
            Err(err_str) => write!(f, "{}", err_str),
        }
    }
}

impl NodeKind {
    fn new(c_kind: regoEnum) -> Self {
        match c_kind {
            REGO_NODE_UNDEFINED => NodeKind::Undefined,
            REGO_NODE_BINDING => NodeKind::Binding,
            REGO_NODE_VAR => NodeKind::Var,
            REGO_NODE_TERM => NodeKind::Term,
            REGO_NODE_SCALAR => NodeKind::Scalar,
            REGO_NODE_ARRAY => NodeKind::Array,
            REGO_NODE_SET => NodeKind::Set,
            REGO_NODE_OBJECT => NodeKind::Object,
            REGO_NODE_OBJECT_ITEM => NodeKind::ObjectItem,
            REGO_NODE_INT => NodeKind::Int,
            REGO_NODE_FLOAT => NodeKind::Float,
            REGO_NODE_STRING => NodeKind::String,
            REGO_NODE_TRUE => NodeKind::True,
            REGO_NODE_FALSE => NodeKind::False,
            REGO_NODE_NULL => NodeKind::Null,
            REGO_NODE_TERMS => NodeKind::Terms,
            REGO_NODE_BINDINGS => NodeKind::Bindings,
            REGO_NODE_RESULTS => NodeKind::Results,
            REGO_NODE_RESULT => NodeKind::Result,
            REGO_NODE_ERROR => NodeKind::Error,
            REGO_NODE_ERROR_SEQ => NodeKind::ErrorSeq,
            REGO_NODE_ERROR_MESSAGE => NodeKind::ErrorMessage,
            REGO_NODE_ERROR_AST => NodeKind::ErrorAst,
            REGO_NODE_ERROR_CODE => NodeKind::ErrorCode,
            REGO_NODE_INTERNAL => NodeKind::Internal,
            _ => panic!("Unknown node kind"),
        }
    }
}

impl Node {
    fn new(c_ptr: *mut regoNode) -> Self {
        let num_children = unsafe { regoNodeSize(c_ptr) };
        let mut children = Vec::with_capacity(num_children as usize);
        for i in 0..num_children {
            let child_ptr = unsafe { regoNodeGet(c_ptr, i) };
            children.push(Node::new(child_ptr));
        }
        let kind = unsafe { regoNodeType(c_ptr) };
        Self {
            c_ptr,
            children,
            size: num_children as usize,
            kind: NodeKind::new(kind),
        }
    }

    /// Returns a human-readable string representation of the node kind.
    pub fn kind_name(&self) -> Result<String, &str> {
        let size = unsafe { regoNodeTypeNameSize(self.c_ptr) };

        let buf = vec![32 as u8; (size as usize) - 1];
        let input_cstr = CString::new(buf).unwrap();
        let input_ptr = CString::into_raw(input_cstr);
        let err: regoEnum = unsafe { regoNodeTypeName(self.c_ptr, input_ptr, size) };
        if err != REGO_OK {
            return Err("Error getting node kind name");
        } else {
            let output_cstr: CString = unsafe { CString::from_raw(input_ptr) };
            Ok(output_cstr.into_string().unwrap())
        }
    }

    fn scalar_value(&self) -> Result<String, &str> {
        let size = unsafe { regoNodeValueSize(self.c_ptr) };

        let buf = vec![32 as u8; (size as usize) - 1];
        let input_cstr = CString::new(buf).unwrap();
        let input_ptr = CString::into_raw(input_cstr);
        let err: regoEnum = unsafe { regoNodeValue(self.c_ptr, input_ptr, size) };
        if err != REGO_OK {
            return Err("Error getting node value");
        } else {
            let output_cstr: CString = unsafe { CString::from_raw(input_ptr) };
            Ok(output_cstr.into_string().unwrap())
        }
    }

    /// Returns the value of the node.
    ///
    /// If the node has a singular value (i.e is a `Scalar` or a `Term` containing a scalar)
    /// then this will return that value. Otherwise it will return `[Option::None]`.
    ///
    /// # Examples
    /// ```
    /// # use regorust::*;
    /// # let rego = Interpreter::new();
    /// # match rego.query(r#"x=10; y="20"; z=true"#) {
    /// #  Ok(result) => {
    /// let x = result.binding("x").expect("cannot get x");
    /// if let NodeValue::Int(x) = result
    ///   .binding("x")
    ///   .unwrap()
    ///   .value()
    ///   .unwrap()
    /// {
    ///   println!("x = {}", x);
    ///   # assert_eq!(x, 10);
    /// }
    ///
    /// if let NodeValue::String(y) = result
    ///   .binding("y")
    ///   .unwrap()
    ///   .value()
    ///   .unwrap()
    /// {
    ///   println!("y = {}", y);
    ///   # assert_eq!(y, "20");
    /// }
    ///
    /// if let NodeValue::Bool(z) = result
    ///   .binding("z")
    ///   .unwrap()
    ///   .value()
    ///   .unwrap()
    /// {
    ///   println!("z = {}", z);
    ///   # assert!(z);
    /// }
    /// # }
    /// # Err(e) => {
    /// # panic!("error: {}", e);
    /// # }
    /// # }
    /// ```
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
                .map(|s| NodeValue::String(remove_quotes(s))),
            _ => None,
        }
    }

    /// Returns the node as a JSON string.
    ///
    /// The result of this function will a valid JSON representation of the
    /// node. If the node cannot be transformed into JSON this will return
    /// an error.
    ///
    /// # Examples
    /// ```
    /// # use regorust::*;
    /// # let rego = Interpreter::new();
    /// # match rego.query(r#"x=10; y="20"; z=true"#) {
    /// #  Ok(result) => {
    /// let x = result.binding("x").expect("cannot get x");
    /// println!("x = {}", x.json().unwrap());
    /// # assert_eq!(x.json().unwrap(), "10");
    /// let y = result.binding("y").expect("cannot get y");
    /// println!("y = {}", y.json().unwrap());
    /// # assert_eq!(y.json().unwrap(), r#""20""#);
    /// let z = result.binding("z").expect("cannot get z");
    /// println!("z = {}", z.json().unwrap());
    /// # assert_eq!(z.json().unwrap(), "true");
    /// # }
    /// # Err(e) => {
    /// # panic!("error: {}", e);
    /// # }
    /// # }
    /// ```
    pub fn json(&self) -> Result<String, &str> {
        let size = unsafe { regoNodeJSONSize(self.c_ptr) };

        let buf = vec![32 as u8; (size as usize) - 1];
        let input_cstr = CString::new(buf).unwrap();
        let input_ptr = CString::into_raw(input_cstr);
        let err: regoEnum = unsafe { regoNodeJSON(self.c_ptr, input_ptr, size) };
        if err != REGO_OK {
            return Err("Error converting node to JSON");
        } else {
            let output_cstr = unsafe { CString::from_raw(input_ptr) };
            let output = output_cstr.into_string().unwrap();
            return Ok(output);
        }
    }

    /// Returns the number of child nodes.
    pub fn size(&self) -> usize {
        let c_size = unsafe { regoNodeSize(self.c_ptr) };
        c_size as usize
    }

    fn lookup_object(&self, key: &str) -> Result<&Node, &str> {
        for child in self.children.iter() {
            let key_json = child[0].json()?;
            if key_json.starts_with('"') && key_json.ends_with('"') {
                // allow querying of string keys without the quotes
                let without_quotes = key_json[1..key_json.len() - 1].to_string();
                if without_quotes == key {
                    return Ok(&child[1]);
                }
            }

            if key_json == key {
                return Ok(&child[1]);
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

    /// Looks up a key in an object or a set.
    ///
    /// The key argument should be a JSON representation of the key, and can be
    /// both simple types (e.g. strings, ints, bools) or complex types
    /// (e.g. objects, arrays). The returned result will be value node of the
    /// object item with a matching key, or conversely the node with the matching
    /// value in the set. If no matching value is found, or if the node is not an
    /// object or a set, an error will be returned.
    ///
    /// # Examples
    /// ```
    /// # use regorust::*;
    /// # let rego = Interpreter::new();
    /// let output = rego.query(r#"x={"a": 1}; y={["foo", false], 5}; z=[1, "bar"]"#).unwrap();
    /// println!("{}", output.to_str().unwrap());
    /// let x = output.binding("x").unwrap();
    /// println!(r#"x["a"] = {}"#, x.lookup("a").unwrap().json().unwrap());
    /// # assert_eq!(x.lookup("a").unwrap().json().unwrap(), "1");
    /// let y = output.binding("y").unwrap();
    /// println!(r#"y[["foo", false]] = {}"#, y.lookup(r#"["foo",false]"#).unwrap().json().unwrap());
    /// # assert_eq!(y.lookup(r#"["foo",false]"#).unwrap().json().unwrap(), r#"["foo",false]"#);
    /// let z = output.binding("z").unwrap();
    /// match z.lookup("bar") {
    ///   Ok(_) => panic!("bar should not be found"),
    ///   Err(err) => println!("{}", err)
    /// }
    /// ```
    pub fn lookup(&self, key: &str) -> Result<&Node, &str> {
        match self.kind {
            NodeKind::Term => self.children[0].lookup(key),
            NodeKind::Object => self.lookup_object(key),
            NodeKind::Set => self.lookup_set(key),
            _ => Err("Must be an object or a set"),
        }
    }

    /// Looks up an index in an array.
    ///
    /// If the index falls outside the range of the array, the result will be an error.
    /// Similarly, this method will only work if the Node is of [`NodeKind::Array`].
    ///
    /// # Examples
    /// ```
    /// # use regorust::*;
    /// # let rego = Interpreter::new();
    /// let output = rego.query(r#"x=[1, 2, 3]; y={"0": "a"}"#).unwrap();
    /// let x = output.binding("x").unwrap();
    /// println!(r#"x[0] = {}"#, x.index(0).unwrap().json().unwrap());
    /// # assert_eq!(x.index(0).unwrap().json().unwrap(), "1");
    /// let y = output.binding("y").unwrap();
    /// match y.index(0) {
    ///   Ok(_) => panic!("0 should not be found"),
    ///   Err(err) => println!("{}", err)
    /// }
    pub fn index(&self, index: usize) -> Result<&Node, &str> {
        match self.kind {
            NodeKind::Term => self.children[0].index(index),
            NodeKind::Array => {
                if index >= self.size() {
                    return Err("Index out of bounds");
                }

                Ok(&self.children[index])
            }
            NodeKind::Terms => {
                if index >= self.size() {
                    return Err("Index out of bounds");
                }

                Ok(&self.children[index])
            }
            NodeKind::Results => {
                if index >= self.size() {
                    return Err("Index out of bounds");
                }

                Ok(&self.children[index])
            }
            _ => Err("Must be an array"),
        }
    }

    /// Returns the raw child node at a particular index.
    ///
    /// If the index falls outside the range of the array, the result will be an error.
    pub fn at(&self, index: usize) -> Result<&Node, &str> {
        if index >= self.size() {
            return Err("Index out of bounds");
        }

        Ok(&self.children[index])
    }

    /// Returns an iterator over the child nodes.
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

impl PartialEq for Node {
    fn eq(&self, other: &Self) -> bool {
        return self.c_ptr == other.c_ptr;
    }
}

impl fmt::Display for Node {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.json() {
            Ok(json) => write!(f, "{}", json),
            Err(err) => write!(f, "{}", err),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn query_math() {
        let rego = Interpreter::new();
        rego.set_log_level(LogLevel::Info)
            .expect("cannot set log level");
        match rego.query("x=5;y=x + (2 - 4 * 0.25) * -3 + 7.4") {
            Ok(result) => {
                let x = result.binding("x").expect("cannot get x");
                let y = result.binding("y").expect("cannot get y");
                println!("x = {}", x.json().unwrap());
                println!("y = {}", y.json().unwrap());
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
        let input = r#"
            {
                "a": 10,
                "b": "20",
                "c": 30.0,
                "d": true
            }
        "#;
        let data0 = r#"
            {
                "one": {
                    "bar": "Foo",
                    "baz": 5,
                    "be": true,
                    "bop": 23.4
                },
                "two": {
                    "bar": "Bar",
                    "baz": 12.3,
                    "be": false,
                    "bop": 42
                }
            }
        "#;
        let data1 = r#"
            {
                "three": {
                    "bar": "Baz",
                    "baz": 15,
                    "be": true,
                    "bop": 4.23
                }
            }        
        "#;
        let module = r#"
        package objects

        rect := {`width`: 2, "height": 4}
        cube := {"width": 3, `height`: 4, "depth": 5}
        a := 42
        b := false
        c := null
        d := {"a": a, "x": [b, c]}
        index := 1
        shapes := [rect, cube]
        names := ["prod", `smoke1`, "dev"]
        sites := [{"name": "prod"}, {"name": names[index]}, {"name": "dev"}]
        e := {
            a: "foo",
            "three": c,
            names[2]: b,
            "four": d,
        }
        f := e["dev"]                
        "#;
        let rego = Interpreter::new();
        rego.set_input_json(input).expect("cannot set input");
        rego.add_data_json(data0)
            .expect("cannot add data as string");
        rego.add_data_json(data1).expect("cannot add data file");
        rego.add_module("objects", module)
            .expect("cannot add module");
        match rego.query("x=[data.one, input.b, data.objects.sites[1]]") {
            Ok(result) => {
                let x = result.binding("x").expect("cannot get x");
                let data_one = x.index(0).expect("cannot get data.one");
                if let NodeValue::String(bar) = data_one
                    .lookup("bar")
                    .expect("bar key missing")
                    .value()
                    .expect("bar value missing")
                {
                    assert_eq!(bar, "Foo");
                } else {
                    panic!("bar is not a string");
                }

                if let NodeValue::Bool(be) = data_one
                    .lookup("be")
                    .expect("be key missing")
                    .value()
                    .expect("be value missing")
                {
                    assert_eq!(be, true);
                } else {
                    panic!("be is not a bool");
                }

                if let NodeValue::Int(baz) = data_one
                    .lookup("baz")
                    .expect("baz key missing")
                    .value()
                    .expect("baz value missing")
                {
                    assert_eq!(baz, 5);
                } else {
                    panic!("baz is not an int");
                }

                if let NodeValue::Float(bop) = data_one
                    .lookup("bop")
                    .expect("bop key missing")
                    .value()
                    .expect("bop value missing")
                {
                    assert_eq!(bop, 23.4);
                } else {
                    panic!("bop is not a float");
                }

                if let NodeValue::String(input_b) = x
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
                if let NodeValue::String(name) = data_objects_sites_1
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

    #[test]
    fn node_access() {
        let rego = Interpreter::new();
        match rego.query(r#"x={"a": 10, "b": "20", "c": [30.0, 60], "d": true, "e": null}"#) {
            Ok(result) => {
                let x = result.binding("x").expect("cannot get x");
                println!("x = {}", x.json().unwrap());
                if let NodeValue::Int(a) = x.lookup("a").unwrap().value().unwrap() {
                    assert_eq!(a, 10);
                }

                if let NodeValue::String(b) = x.lookup("b").unwrap().value().unwrap() {
                    assert_eq!(b, "20");
                }

                let c = x.lookup("c").unwrap();
                if let NodeValue::Float(c0) = c.index(0).unwrap().value().unwrap() {
                    assert_eq!(c0, 30.0);
                }

                if let NodeValue::Int(c1) = c.index(1).unwrap().value().unwrap() {
                    assert_eq!(c1, 60);
                }

                if let NodeValue::Bool(d) = x.lookup("d").unwrap().value().unwrap() {
                    assert!(d);
                }
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        }
    }

    #[test]
    fn multiple_inputs() {
        let input0 = r#"{"a": 10}"#;
        let input1 = r#"{"a": 4}"#;
        let input2 = r#"{"a": 7}"#;
        let module = r#"
            package multi

            default a := 0
            
            a := val {
                input.a > 0
                input.a < 10
                input.a % 2 == 1
                val := input.a * 10
            } {
                input.a > 0
                input.a < 10
                input.a % 2 == 0
                val := input.a * 10 + 1
            }
            
            a := input.a / 10 {
                input.a >= 10
            }        
        "#;
        let rego = Interpreter::new();
        rego.add_module("multi", module).expect("cannot add module");
        rego.set_input_json(input0).expect("cannot set input");
        match rego.query("x = data.multi.a") {
            Ok(output) => {
                if let NodeValue::Int(a) = output
                    .binding("x")
                    .expect("cannot get x")
                    .value()
                    .expect("cannot get x value")
                {
                    assert_eq!(a, 1);
                }
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        }

        rego.set_input_json(input1).expect("cannot set input");
        match rego.query("x = data.multi.a") {
            Ok(output) => {
                if let NodeValue::Int(a) = output
                    .binding("x")
                    .expect("cannot get x")
                    .value()
                    .expect("cannot get x value")
                {
                    assert_eq!(a, 41);
                }
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        }

        rego.set_input_json(input2).expect("cannot set input");
        match rego.query("x = data.multi.a") {
            Ok(output) => {
                if let NodeValue::Int(a) = output
                    .binding("x")
                    .expect("cannot get x")
                    .value()
                    .expect("cannot get x value")
                {
                    assert_eq!(a, 70);
                }
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        }
    }

    #[test]
    fn doublequotes() {
        let rego = Interpreter::new();
        match rego.query(r#"x := {"bar": sprintf("%s", ["foo"])}"#) {
            Ok(value) => {
                let x = value.binding("x").expect("cannot get x");
                if let NodeValue::String(bar) = x
                    .lookup("bar")
                    .expect("bar key missing")
                    .value()
                    .expect("bar value missing")
                {
                    assert_eq!(bar, "foo");
                }
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        }
    }

    #[test]
    fn badmodule() {
        let rego = Interpreter::new();
        match rego.add_module("bad", "package bad\nx <> 10") {
            Ok(_) => {
                panic!("should not be able to add bad module");
            }
            Err(e) => {
                println!("error: {}", e);
            }
        }
    }

    #[test]
    fn build() {
        let input0 = Input::new()
            .str("x")
            .int(104)
            .objectitem()
            .str("y")
            .int(119)
            .objectitem()
            .object(2)
            .validate()
            .expect("Unable to construct input");
        let data = r#"{
            "a": 7,
            "b": 13
        }"#;
        let module = r#"
            package test

            foo := data.a * input.x + data.b * input.y
            bar := data.b * input.x + data.a * input.y        
        "#;

        let mut bundle_dir = std::env::temp_dir();
        bundle_dir.push("bundle");
        let mut bundle_file = std::env::temp_dir();
        bundle_file.push("bundle.rbb");

        let rego_build = Interpreter::new();
        rego_build.add_data_json(data).expect("Unable to add data");
        rego_build
            .add_module("test.rego", module)
            .expect("Unable to add module");
        let bundle = rego_build
            .build(
                &Some("x=data.test.foo + data.test.bar"),
                &["test/foo", "test/bar"],
            )
            .expect("Failed to build bundle");

        rego_build
            .save_bundle(bundle_dir.as_path(), &bundle, BundleFormat::JSON)
            .expect("Failed to save bundle as JSON");
        rego_build
            .save_bundle(bundle_file.as_path(), &bundle, BundleFormat::Binary)
            .expect("Failed to save bundle as Binary");

        bundle_dir.push("plan.json");
        bundle_dir.try_exists().expect("plan.json is missing");
        bundle_dir.pop();
        bundle_dir.push("data.json");
        bundle_dir.try_exists().expect("data.json is missing");
        bundle_dir.pop();
        bundle_dir.push("test.rego");
        bundle_dir.try_exists().expect("test.rego is missing");
        bundle_dir.pop();

        let rego_run = Interpreter::new();
        let bundle = rego_run
            .load_bundle(bundle_dir.as_path(), BundleFormat::JSON)
            .expect("Unable to load JSON bundle");
        rego_run.set_input(&input0).expect("Unable to set input");
        match rego_run.query_bundle(&bundle) {
            Ok(output) => {
                if let NodeValue::Int(a) = output
                    .binding("x")
                    .expect("cannot get x")
                    .value()
                    .expect("cannot get x value")
                {
                    assert_eq!(a, 4460);
                }
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        };

        match rego_run.query_bundle_entrypoint(&bundle, "test/foo") {
            Ok(output) => {
                let expressions = output.expressions().expect("Unable to get expressions");
                if let NodeValue::Int(a) =
                    expressions[0].value().expect("cannot get expression value")
                {
                    assert_eq!(a, 2275);
                }
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        };

        let bundle_bin = rego_run
            .load_bundle(bundle_file.as_path(), BundleFormat::Binary)
            .expect("Unable to load binary bundle");
        match rego_run.query_bundle(&bundle_bin) {
            Ok(output) => {
                if let NodeValue::Int(a) = output
                    .binding("x")
                    .expect("cannot get x")
                    .value()
                    .expect("cannot get x value")
                {
                    assert_eq!(a, 4460);
                }
            }
            Err(e) => {
                panic!("error: {}", e);
            }
        };
    }
}
