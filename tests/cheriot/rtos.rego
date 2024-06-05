package rtos

import future.keywords

is_allocator_capability(capability) {
  capability.kind == "SealedObject"
  capability.sealing_type.compartment == "alloc"
  capability.sealing_type.key == "MallocKey"
}

decode_allocator_capability(capability) = decoded {
  is_allocator_capability(capability)
  some quota
  quota = integer_from_hex_string(capability.contents, 0, 4)
  # Remaining words are all zero
  integer_from_hex_string(capability.contents, 4, 4) == 0
  integer_from_hex_string(capability.contents, 8, 4) == 0
  integer_from_hex_string(capability.contents, 12, 4) == 0
  integer_from_hex_string(capability.contents, 16, 4) == 0
  integer_from_hex_string(capability.contents, 20, 4) == 0
  decoded := { "quota": quota }
}

all_sealed_allocator_capabilities_are_valid {
  some allocatorCapabilities
  allocatorCapabilities = [ c | c = input.compartments[_].imports[_] ; is_allocator_capability(c) ]
  every c in allocatorCapabilities {
    decode_allocator_capability(c)
  }
}


valid {
  all_sealed_allocator_capabilities_are_valid
  # Only the allocator may access the revoker.
  data.compartment.mmio_allow_list("revoker", {"allocator"})
  # Only the scheduler may access the interrupt controllers.
  data.compartment.mmio_allow_list("clint", {"scheduler"})
  data.compartment.mmio_allow_list("plic", {"scheduler"})
}