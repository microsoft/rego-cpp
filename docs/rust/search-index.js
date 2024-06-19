var searchIndex = new Map(JSON.parse('[\
["regorust",{"doc":"","t":"PPPPPPPPPPPPPPPPPPFGFGGPPPPPFPSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSPPPPPPPPPPPPPPNNNNNNNNNNNNNNNNNNNHNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNONNNNNHHHHIIHHHHHHIHIHHHHHHHHIHHHHHHHHHHHHHHHHHHINNNNHNNNNONNNNNNNNNNNNNNNNNNNNNNNN","n":["Array","Binding","Bindings","Bool","Debug","Error","Error","ErrorAst","ErrorCode","ErrorMessage","ErrorSeq","False","Float","Float","Info","Int","Int","Internal","Interpreter","LogLevel","Node","NodeKind","NodeValue","None","Null","Null","Object","ObjectItem","Output","Output","REGOCPP_BUILD_DATE","REGOCPP_BUILD_NAME","REGOCPP_BUILD_TOOLCHAIN","REGOCPP_GIT_HASH","REGOCPP_OPA_VERSION","REGOCPP_PLATFORM","REGOCPP_VERSION","REGOCPP_VERSION_MAJOR","REGOCPP_VERSION_MINOR","REGOCPP_VERSION_REVISION","REGO_ERROR","REGO_ERROR_BUFFER_TOO_SMALL","REGO_ERROR_INVALID_LOG_LEVEL","REGO_LOG_LEVEL_DEBUG","REGO_LOG_LEVEL_ERROR","REGO_LOG_LEVEL_INFO","REGO_LOG_LEVEL_NONE","REGO_LOG_LEVEL_OUTPUT","REGO_LOG_LEVEL_TRACE","REGO_LOG_LEVEL_WARN","REGO_NODE_ARRAY","REGO_NODE_BINDING","REGO_NODE_BINDINGS","REGO_NODE_ERROR","REGO_NODE_ERROR_AST","REGO_NODE_ERROR_CODE","REGO_NODE_ERROR_MESSAGE","REGO_NODE_ERROR_SEQ","REGO_NODE_FALSE","REGO_NODE_FLOAT","REGO_NODE_INT","REGO_NODE_INTERNAL","REGO_NODE_NULL","REGO_NODE_OBJECT","REGO_NODE_OBJECT_ITEM","REGO_NODE_RESULT","REGO_NODE_RESULTS","REGO_NODE_SCALAR","REGO_NODE_SET","REGO_NODE_STRING","REGO_NODE_TERM","REGO_NODE_TERMS","REGO_NODE_TRUE","REGO_NODE_UNDEFINED","REGO_NODE_VAR","REGO_OK","Result","Results","Scalar","Set","String","String","Term","Terms","Trace","True","Undefined","Var","Var","Warn","add_data_json","add_data_json_file","add_module","add_module_file","at","binding","binding_at_index","borrow","borrow","borrow","borrow","borrow","borrow","borrow_mut","borrow_mut","borrow_mut","borrow_mut","borrow_mut","borrow_mut","build_info","clone","clone_into","default","drop","drop","eq","eq","eq","eq","expressions","expressions_at_index","fmt","fmt","fmt","fmt","fmt","fmt","fmt","from","from","from","from","from","from","get_debug_enabled","get_strict_built_in_errors","get_well_formed_checks_enabled","index","index","into","into","into","into","into","into","iter","json","kind","kind_name","lookup","new","ok","query","regoAddDataJSON","regoAddDataJSONFile","regoAddModule","regoAddModuleFile","regoBoolean","regoEnum","regoFree","regoFreeOutput","regoGetDebugEnabled","regoGetError","regoGetStrictBuiltInErrors","regoGetWellFormedChecksEnabled","regoInterpreter","regoNew","regoNode","regoNodeGet","regoNodeJSON","regoNodeJSONSize","regoNodeSize","regoNodeType","regoNodeTypeName","regoNodeValue","regoNodeValueSize","regoOutput","regoOutputBinding","regoOutputBindingAtIndex","regoOutputExpressions","regoOutputExpressionsAtIndex","regoOutputNode","regoOutputOk","regoOutputSize","regoOutputString","regoQuery","regoSetDebugEnabled","regoSetDebugPath","regoSetInputJSON","regoSetInputJSONFile","regoSetInputTerm","regoSetLogLevel","regoSetLogLevelFromString","regoSetStrictBuiltInErrors","regoSetWellFormedChecksEnabled","regoSize","set_debug_enabled","set_debug_path","set_input_json","set_input_json_file","set_log_level","set_strict_built_in_errors","set_well_formed_checks_enabled","size","size","size","to_node","to_owned","to_str","to_string","to_string","try_from","try_from","try_from","try_from","try_from","try_from","try_into","try_into","try_into","try_into","try_into","try_into","type_id","type_id","type_id","type_id","type_id","type_id","value"],"q":[[0,"regorust"],[230,"core::result"],[231,"std::path"],[232,"alloc::string"],[233,"core::fmt"],[234,"core::fmt"],[235,"core::any"],[236,"core::option"]],"d":["","","","","","","","","","","","","","","","","","","Interface for the Rego interpreter.","","Interface for Rego Node objects.","Enumeration of different kinds of Rego Nodes that can be …","Represents the value of a Rego Node.","","","","","","Interface for the Rego output.","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","Adds a base document from a string.","Adds a base document from a file.","Adds a Rego module from a string.","Adds a Rego module from a file.","Returns the raw child node at a particular index.","Attempts to return the binding for the given variable name.","Attempts to return the binding for the given variable name.","","","","","","","","","","","","","Returns the build information as a string.","","","","","","","","","","Attempts to return the expressions at a given result index.","Attempts to return the expressions at a given result index.","","","","","","","","Returns the argument unchanged.","Returns the argument unchanged.","Returns the argument unchanged.","Returns the argument unchanged.","Returns the argument unchanged.","Returns the argument unchanged.","Returns whether the Rego interpreter is in debug mode.","Returns whether the interpreter will forward errors thrown …","Returns whether the Rego interpreter will perform …","","Looks up an index in an array.","Calls <code>U::from(self)</code>.","Calls <code>U::from(self)</code>.","Calls <code>U::from(self)</code>.","Calls <code>U::from(self)</code>.","Calls <code>U::from(self)</code>.","Calls <code>U::from(self)</code>.","Returns an iterator over the child nodes.","Returns the node as a JSON string.","The kind of this node.","Returns a human-readable string representation of the node …","Looks up a key in an object or a set.","Creates a new Rego interpreter.","Returns whether the output is ok.","This method performs a query against the current set of …","Adds a base document from the specified string.","Adds a base document from the file at the specified path.","Adds a module (e.g. virtual document) from the specified …","Adds a module (e.g. virtual document) from the file at the …","","","Frees a Rego interpreter.","Frees a Rego output.","Gets the debug mode of the interpreter.","Returns the most recently thrown error.","Gets whether strict built-in errors are enabled.","Gets whether well-formed checks are enabled.","","Allocates and initializes a new Rego interpreter.","","Returns the child node at the specified index.","Populate a buffer with the JSON representation of the node.","Returns the number of bytes needed to store a 0-terminated …","Returns the number of children of the node.","Returns an enumeration value indicating the nodes type.","Returns the name of the node type as a human-readable …","Populate a buffer with the node value.","Returns the number of bytes needed to store a 0-terminated …","","Returns the bound value for a given variable name at the …","Returns the bound value for a given variable name.","Returns a node containing a list of terms resulting from …","Returns a node containing a list of terms resulting from …","Returns the node containing the output of the query.","Returns whether the output is ok.","Returns the number of results in the output.","Returns the output represented as a human-readable string.","Performs a query against the current base and virtual …","Sets the debug mode of the interpreter.","Sets the path to the debug directory.","Sets the current input document from the specified string.","Sets the current input document from the file at the …","Sets the current input document from the specified string.","Sets the level of logging.","Sets the level of logging.","Sets whether the built-ins should throw errors.","Sets whether to perform well-formed checks after each …","","Sets whether the Rego interpreter is in debug mode.","Sets the path to the directory where the Rego interpreter …","Sets the input JSON expression from a string.","Sets the input JSON expression from a file.","Sets the level of logging produced by the library.","Sets whether the interpreter will forward errors thrown by …","Sets whether the Rego interpreter will perform …","Returns the number of results in the output.","Returns the number of child nodes.","The number of children of this node.","Returns the output node.","","Returns the output as a JSON-encoded string.","","","","","","","","","","","","","","","","","","","","","Returns the value of the node."],"i":[11,11,11,13,17,17,11,11,11,11,11,11,11,13,17,11,13,11,0,0,0,0,0,17,11,13,11,11,0,17,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,11,11,11,11,11,13,11,11,17,11,11,11,13,17,1,1,1,1,6,8,8,17,1,8,11,6,13,17,1,8,11,6,13,0,11,11,11,1,8,1,8,6,13,8,8,1,8,8,11,6,6,13,17,1,8,11,6,13,1,1,1,6,6,17,1,8,11,6,13,6,6,6,6,6,1,8,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1,1,8,6,6,8,11,8,8,6,17,1,8,11,6,13,17,1,8,11,6,13,17,1,8,11,6,13,6],"f":"``````````````````````````````````````````````````````````````````````````````````````````{{bd}{{h{fd}}}}{{bj}{{h{fd}}}}{{bdd}{{h{fd}}}}1{{ln}{{h{ld}}}}{{A`d}{{h{ll}}}}{{A`Abd}{{h{ll}}}}{ce{}{}}00000000000{{}Ad}{AfAf}{{ce}f{}{}}{{}Af}{bf}{A`f}{{bb}Ah}{{A`A`}Ah}{{ll}Ah}{{AjAj}Ah}{A`{{h{ll}}}}{{A`Ab}{{h{ll}}}}{{bAl}An}{{A`Al}An}0{{AfAl}An}{{lAl}An}0{{AjAl}An}{cc{}}00000{bAh}00{{ln}c{}}{{ln}{{h{ld}}}}{ce{}{}}00000{l{{B`{l}}}}{l{{h{Add}}}}`{ld}{{ld}{{h{ld}}}}{{}b}{A`Ah}{{bd}{{h{A`d}}}}```````````````````````````````````````````{{bAh}f}{{bj}{{h{fd}}}}{{bd}{{h{fd}}}}1{Bb{{h{fd}}}}33{A`Ab}{ln}`{A`{{h{ll}}}}>{A`{{h{dd}}}}{cAd{}}0{c{{h{e}}}{}{}}00000000000{cBd{}}00000{l{{Bf{Aj}}}}","c":[],"p":[[5,"Interpreter",0],[1,"str"],[1,"unit"],[6,"Result",230],[5,"Path",231],[5,"Node",0],[1,"usize"],[5,"Output",0],[8,"regoSize",0],[5,"String",232],[6,"NodeKind",0],[1,"bool"],[6,"NodeValue",0],[5,"Formatter",233],[8,"Result",233],[5,"Iter",234],[6,"LogLevel",0],[5,"TypeId",235],[6,"Option",236]],"b":[[122,"impl-Debug-for-Output"],[123,"impl-Display-for-Output"],[125,"impl-Display-for-Node"],[126,"impl-Debug-for-Node"],[137,"impl-Index%3Cusize%3E-for-Node"],[138,"impl-Node"]]}]\
]'));
if (typeof exports !== 'undefined') exports.searchIndex = searchIndex;
else if (window.initSearch) window.initSearch(searchIndex);
