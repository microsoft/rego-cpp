package compartment

compartment_contains_export(compartment, export) {
  compartment.exports[_] == export
}

compartment_includes_file(compartment, filename) {
  compartment.code.inputs[_].file == filename
}

compartment_includes_file(compartment, filename) {
  compartment.data.inputs[_].file == filename
}

export_matches_import(export, importEntry)  {
  export.export_symbol = importEntry.export_symbol
}


export_for_import(importEntry) = entry {
  some possibleEntries
  some allExports
  allExports = {export | input.compartments[_].exports[_] = export}
  possibleEntries = [{"compartment": compartment, "entry": entry} | entry = input.compartments[compartment].exports[_]; export_matches_import(entry, importEntry)]
  count(possibleEntries) == 1
  compartment_includes_file(input.compartments[possibleEntries[0].compartment], importEntry.provided_by)
  entry := possibleEntries[0].entry
}

import_is_library_call(a) { a.kind = "LibraryFunction" }
import_is_MMIO(a) { a.kind = "MMIO" }
import_is_compartment_call(a) {
  a.kind = "CompartmentExport"
  a.function
}

import_is_call {
  import_is_library_call(importEntry)
}

import_is_call {
  import_is_compartment_call(importEntry)
}

mmio_imports_for_compartment(compartment) = entry {
  entry := [e | e = compartment.imports[_]; import_is_MMIO(e)]
}

mmio_is_device(importEntry, device) {
  importEntry.start = device.start
  importEntry.length = device.length
}

device_for_mmio_import(importEntry) = device {
  import_is_MMIO(importEntry)
  some devices
  devices = [{ i:d } | d = data.board.devices[i]; mmio_is_device(importEntry, d)]
  count(devices) == 1
  device := devices[0]
}

compartment_imports_device(compartment, device) {
  count([d | d = mmio_imports_for_compartment(compartment)[_] ; mmio_is_device(d, device)]) > 0
}

compartments_with_mmio_import(device) = compartments {
  compartments = [i | c = input.compartments[i]; compartment_imports_device(c, device)]
}

compartment_export_matching_symbol(compartmentName, symbol) = export {
  some compartment
  compartment = input.compartments[compartmentName]
  some exports
  exports = [e | e = compartment.exports[_]; re_match(symbol, export_entry_demangle(compartmentName, e.export_symbol))]
  count(exports) == 1
  export := exports[0]
}

compartments_calling_export(export) = compartments {
  compartments = [c | i = input.compartments[c].imports[_]; export_matches_import(i, export)]
}

compartments_calling_export_matching(compartmentName, export) = compartments {
  compartments = compartments_calling_export(compartment_export_matching_symbol(compartmentName, export))
}

# Helper that builds a map from symbol names to compartments.  This is
# to avoid some O(n^2) lookups.  Or would, if rego-cpp didn't recompute
# this every evaluation.
#entry_point_map := { entry.export_symbol : compartment | entry = input.compartments[compartment].exports[_] ; entry.kind = "Function"}

compartment_exports_function(callee, importEntry) {
  #entry_point_map[importEntry.export_symbol] == callee
  some possibleEntries
  possibleEntries = [entry | entry = input.compartments[callee].exports[_]; export_matches_import(entry, importEntry)]
  count(possibleEntries) == 1
  compartment_includes_file(input.compartments[callee], importEntry.provided_by)
}

# Helper to work around rego-cpp#117
check_export(exports, importEntry, compartment) {
  exports[importEntry.export_symbol] == compartment
}

compartments_calling(callee) = compartments {
  # Create a reverse map that maps from exported symbols to
  # compartments.  This avoids some O(n^2) lookups.  It would be nice
  # to cache this globally but rego-cpp doesn't support that yet.
  # This reduces the time for this query on the test suite from over
  # 6s to under 0.5s for me.
  some entry_point_map
  entry_point_map = { entry.export_symbol : compartment | entry = input.compartments[compartment].exports[_] ; entry.kind = "Function"}
  compartments = {c | i = input.compartments[c].imports[_]; check_export(entry_point_map, i, callee)}
}


# Helper for allow lists.  Functions cannot return sets, so this
# accepts an array of compartment names that match some property and
# evaluates to true if and only if each one is also present in the allow
# list.
allow_list(testArray, allowList) {
  some compartments
  compartments = {c | c:=testArray[_]}
  compartments & allowList == compartments
}

mmio_allow_list(mmioName, allowList) {
  allow_list(compartments_with_mmio_import(data.board.devices[mmioName]), allowList)
}

compartment_call_allow_list(compartmentName, exportPattern, allowList) {
  allow_list(compartments_calling_export_matching(compartmentName, exportPattern), allowList)
}

compartment_allow_list(compartmentName, allowList) {
  allow_list(compartments_calling(compartmentName), allowList)
}

