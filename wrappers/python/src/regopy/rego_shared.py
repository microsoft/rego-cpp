import ctypes
from enum import IntEnum
import os


class LogLevel(IntEnum):
    NONE = 0
    ERROR = 1
    OUTPUT = 2
    WARN = 3
    INFO = 4
    DEBUG = 5
    TRACE = 6


class Code(IntEnum):
    OK = 0
    ERROR = 1
    ERROR_BUFFER_TO_SMALL = 2
    ERROR_INVALID_LOG_LEVEL = 3
    ERROR_MANUAL_TZDATA_NOT_SUPPORTED = 4


class NodeKind(IntEnum):
    """Enumeration of node types."""

    Binding = 1000
    Var = 1001
    Term = 1002
    Scalar = 1003
    Array = 1004
    Set = 1005
    Object = 1006
    ObjectItem = 1007
    Int = 1008
    Float = 1009
    String = 1010
    True_ = 1011
    False_ = 1012
    Boolean = 1111  # simplifies issue with True and False
    Null = 1013
    Undefined = 1014
    Terms = 1015
    Bindings = 1016
    Results = 1017
    Result = 1018
    Error = 1800
    ErrorMessage = 1801
    ErrorAst = 1802
    ErrorCode = 1803
    ErrorSeq = 1804
    Internal = 1999


class RegoError(Exception):
    """Exception raised when an error occurs in the rego-cpp interpreter."""

    def __init__(self, message: str, code=Code.ERROR):
        """Initializer.

        Args:
            message (str): The error message.
        """
        self.code = code
        Exception.__init__(self, message)


module_dir = os.path.dirname(__file__)
if os.path.exists(os.path.join(module_dir, "rego_shared.dll")):
    path = os.path.join(module_dir, "rego_shared.dll")
elif os.path.exists(os.path.join(module_dir, "librego_shared.so")):
    path = os.path.join(module_dir, "librego_shared.so")
elif os.path.exists(os.path.join(module_dir, "librego_shared.dylib")):
    path = os.path.join(module_dir, "librego_shared.dylib")
else:
    raise RegoError("Could not find rego_shared library")

rego = ctypes.cdll.LoadLibrary(path)


rego.regoSetLogLevel.restype = ctypes.c_uint32
rego.regoSetLogLevel.argtypes = [ctypes.c_uint32]


def rego_set_log_level(level: LogLevel):
    res = rego.regoSetLogLevel(level)
    if res != Code.OK:
        raise RegoError("Could not set log level", res)


rego.regoSetLogLevelFromString.restype = ctypes.c_uint32
rego.regoSetLogLevelFromString.argtypes = [ctypes.c_char_p]


def rego_set_log_level_from_string(level: str):
    p = ctypes.create_string_buffer(level.encode("utf-8"))
    res = rego.regoSetLogLevelFromString(p)
    if res != Code.OK:
        raise RegoError(f"Could not set log level from string {level}", res)


rego.regoBuildInfo.restype = ctypes.c_char_p
rego.regoBuildInfo.argtypes = []


def rego_build_info() -> str:
    return rego.regoBuildInfo().decode("utf-8")


rego.regoVersion.restype = ctypes.c_char_p
rego.regoVersion.argtypes = []


def rego_version() -> str:
    return rego.regoVersion().decode("utf-8")


rego.regoGetError.restype = ctypes.c_char_p
rego.regoGetError.argtypes = [ctypes.c_void_p]


def rego_get_error(impl: ctypes.c_void_p) -> str:
    return rego.regoGetError(impl).decode("utf-8")


rego.regoNew.restype = ctypes.c_void_p
rego.regoNew.argtypes = []
rego_new = rego.regoNew


rego.regoNewV1.restype = ctypes.c_void_p
rego.regoNewV1.argtypes = [ctypes.c_char_p]
rego_new_v1 = rego.regoNewV1


rego.regoFree.restype = None
rego.regoFree.argtypes = [ctypes.c_void_p]
rego_free = rego.regoFree


rego.regoAddModule.restype = ctypes.c_uint32
rego.regoAddModule.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]


def rego_add_module(impl: ctypes.c_void_p, name: str, module: str):
    p_name = ctypes.create_string_buffer(name.encode("utf-8"))
    p_module = ctypes.create_string_buffer(module.encode("utf-8"))
    res = rego.regoAddModule(impl, p_name, p_module)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoAddDataJSON.restype = ctypes.c_uint32
rego.regoAddDataJSON.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_add_data_json(impl: ctypes.c_void_p, data: str):
    p_data = ctypes.create_string_buffer(data.encode("utf-8"))
    res = rego.regoAddDataJSON(impl, p_data)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoSetInputTerm.restype = ctypes.c_uint32
rego.regoSetInputTerm.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_set_input_term(impl: ctypes.c_void_p, term: str):
    p_input = ctypes.create_string_buffer(term.encode("utf-8"))
    res = rego.regoSetInputTerm(impl, p_input)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoSetDebugEnabled.restype = None
rego.regoSetDebugEnabled.argtypes = [ctypes.c_void_p, ctypes.c_bool]


def rego_set_debug_enabled(impl: ctypes.c_void_p, enabled: bool):
    return rego.regoSetDebugEnabled(impl, enabled)


rego.regoGetDebugEnabled.restype = ctypes.c_bool
rego.regoGetDebugEnabled.argtypes = [ctypes.c_void_p]


def rego_get_debug_enabled(impl: ctypes.c_void_p) -> bool:
    return rego.regoGetDebugEnabled(impl)


rego.regoSetDebugPath.restype = ctypes.c_uint32
rego.regoSetDebugPath.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_set_debug_path(impl: ctypes.c_void_p, path: str):
    p_path = ctypes.create_string_buffer(path.encode("utf-8"))
    res = rego.regoSetDebugPath(impl, p_path)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)


rego.regoSetWellFormedChecksEnabled.restype = None
rego.regoSetWellFormedChecksEnabled.argtypes = [ctypes.c_void_p, ctypes.c_bool]


def rego_set_wf_checks_enabled(impl: ctypes.c_void_p, enabled: bool):
    return rego.regoSetWellFormedChecksEnabled(impl, enabled)


rego.regoGetWellFormedChecksEnabled.restype = ctypes.c_bool
rego.regoGetWellFormedChecksEnabled.argtypes = [ctypes.c_void_p]


def rego_get_wf_checks_enabled(impl: ctypes.c_void_p) -> bool:
    return rego.regoGetWellFormedChecksEnabled(impl)


rego.regoSetStrictBuiltInErrors.restype = None
rego.regoSetStrictBuiltInErrors.argtypes = [ctypes.c_void_p, ctypes.c_bool]


def rego_set_strick_builtin_errors(impl: ctypes.c_void_p, enabled: bool):
    return rego.regoSetStrictBuiltInErrors(impl, enabled)


rego.regoGetStrictBuiltInErrors.restype = ctypes.c_bool
rego.regoGetStrictBuiltInErrors.argtypes = [ctypes.c_void_p]


def rego_get_strict_builtin_errors(impl: ctypes.c_void_p) -> bool:
    return rego.regoGetStrictBuiltInErrors(impl)


rego.regoIsBuiltIn.restype = ctypes.c_bool
rego.regoIsBuiltIn.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_is_builtin(impl: ctypes.c_void_p, name: str) -> bool:
    p_name = ctypes.create_string_buffer(name.encode("utf-8"))
    return rego.regoIsBuiltIn(impl, p_name)


rego.regoQuery.restype = ctypes.c_void_p
rego.regoQuery.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def rego_query(impl: ctypes.c_void_p, query: str) -> ctypes.c_void_p:
    p_query = ctypes.create_string_buffer(query.encode("utf-8"))
    p_output = rego.regoQuery(impl, p_query)
    if p_output == 0:
        raise RegoError(rego_get_error(impl))

    return p_output


# Output functions

rego.regoOutputOk.restype = ctypes.c_bool
rego.regoOutputOk.argtypes = [ctypes.c_void_p]


def rego_output_ok(impl: ctypes.c_void_p) -> bool:
    return rego.regoOutputOk(impl)


rego.regoOutputJSONSize.restype = ctypes.c_uint32
rego.regoOutputJSONSize.argtypes = [ctypes.c_void_p]
rego.regoOutputJSON.restype = ctypes.c_uint32
rego.regoOutputJSON.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_output_string(impl: ctypes.c_void_p) -> str:
    size = rego.regoOutputJSONSize(impl)
    buf = ctypes.create_string_buffer(size)
    res = rego.regoOutputJSON(impl, buf, size)
    if res != Code.OK:
        raise RegoError(rego_get_error(impl), res)

    return buf.value.decode("utf-8")


rego.regoOutputNode.restype = ctypes.c_void_p
rego.regoOutputNode.argtypes = [ctypes.c_void_p]
rego_output_node = rego.regoOutputNode


rego.regoOutputExpressionsAtIndex.restype = ctypes.c_void_p
rego.regoOutputExpressionsAtIndex.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
rego_output_expressions_at_index = rego.regoOutputExpressionsAtIndex


rego.regoOutputBindingAtIndex.restype = ctypes.c_void_p
rego.regoOutputBindingAtIndex.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_char_p]


def rego_output_binding_at_index(impl: ctypes.c_void_p, index: int, name: str) -> ctypes.c_void_p:
    p_name = ctypes.create_string_buffer(name.encode("utf-8"))
    p_node = rego.regoOutputBindingAtIndex(impl, index, p_name)
    if p_node == 0:
        raise RegoError(rego_get_error(impl))

    return p_node


rego.regoFreeOutput.restype = None
rego.regoFreeOutput.argtypes = [ctypes.c_void_p]
rego_free_output = rego.regoFreeOutput


# Node functions
rego.regoNodeType.restype = ctypes.c_uint32
rego.regoNodeType.argtypes = [ctypes.c_void_p]


def rego_node_type(impl: ctypes.c_void_p) -> NodeKind:
    kind = NodeKind(rego.regoNodeType(impl))
    if kind in (NodeKind.True_, NodeKind.False_):
        return NodeKind.Boolean

    return kind


rego.regoNodeTypeName.restype = ctypes.c_char_p
rego.regoNodeTypeName.argtypes = [ctypes.c_void_p]


def rego_node_type_name(impl: ctypes.c_void_p) -> str:
    return rego.regoNodeTypeName(impl).decode("utf-8")


rego.regoNodeValueSize.restype = ctypes.c_uint32
rego.regoNodeValueSize.argtypes = [ctypes.c_void_p]
rego.regoNodeValue.restype = ctypes.c_uint32
rego.regoNodeValue.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_node_value(impl: ctypes.c_void_p) -> str:
    size = rego.regoNodeValueSize(impl)
    buf = ctypes.create_string_buffer(size)
    res = rego.regoNodeValue(impl, buf, size)
    if res != Code.OK:
        raise RegoError("Unable to retrieve node value", res)

    return buf.value.decode("utf-8")


rego.regoNodeSize.restype = ctypes.c_uint32
rego.regoNodeSize.argtypes = [ctypes.c_void_p]
rego_node_size = rego.regoNodeSize


rego.regoNodeGet.restype = ctypes.c_void_p
rego.regoNodeGet.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
rego_node_get = rego.regoNodeGet


rego.regoNodeJSONSize.restype = ctypes.c_uint32
rego.regoNodeJSONSize.argtypes = [ctypes.c_void_p]
rego.regoNodeJSON.restype = ctypes.c_uint32
rego.regoNodeJSON.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]


def rego_node_json(impl: ctypes.c_void_p) -> str:
    size = rego.regoNodeJSONSize(impl)
    buf = ctypes.create_string_buffer(size)
    res = rego.regoNodeJSON(impl, buf, size)
    if res != Code.OK:
        raise RegoError("Unable to retrieve node JSON", res)

    return buf.value.decode("utf-8")
