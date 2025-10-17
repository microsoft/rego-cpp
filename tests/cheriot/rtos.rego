package rtos

is_allocator_capability(capability) if {
    capability.kind == "SealedObject"
    capability.sealing_type.compartment == "allocator"
    capability.sealing_type.key == "MallocKey"
}

is_allocator_capability(capability) if {
    capability.kind == "SealedObject"
    capability.sealing_type.compartment == "alloc"
    capability.sealing_type.key == "MallocKey"
}

decode_allocator_capability(capability) = decoded if {
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

all_sealed_allocator_capabilities_are_valid if {
    some allocatorCapabilities
    allocatorCapabilities = [ c | c = input.compartments[_].imports[_] ; is_allocator_capability(c) ]
    every c in allocatorCapabilities {
        decode_allocator_capability(c)
    }
}

# Check that the allocator imports the hazard list with the correct permissions.
allocator_hazard_list_permissions_are_valid if {
    some hazardListImport
    hazardListImport = [ i | i = input.compartments.allocator.imports[_] ; i.shared_object == "allocator_hazard_pointers"]
    every i in hazardListImport {
        i.permits_load == true
        i.permits_load_store_capabilities == true
        i.permits_load_mutable == false
        i.permits_store == false
    }
}

valid if {
    all_sealed_allocator_capabilities_are_valid
    # Only the allocator may access the revoker.
    data.compartment.mmio_allow_list("revoker", {"allocator"})
    # Only the scheduler may access the interrupt controllers.
    data.compartment.mmio_allow_list("clint", {"scheduler"})
    data.compartment.mmio_allow_list("plic", {"scheduler"})
    # Only the allocator may access the hazard list (the switcher can
    # as well via another mechanism)
    data.compartment.shared_object_allow_list("allocator_hazard_pointers", {"allocator"})
    # Only the allocator may write to the epoch.
    # Currently, only the compartment-helpers library reads the epoch,
    # but it isn't a security problem if anything else does.
    data.compartment.shared_object_writeable_allow_list("allocator_epoch", {"allocator"})
    # Size of hazard list and allocator epoch.
    some hazardList
    hazardList = data.compartment.shared_object("allocator_hazard_pointers")
    # Two hazard pointers per thread.
    hazardList.end - hazardList.start = count(input.threads) * 2 * 8
    some epoch
    epoch = data.compartment.shared_object("allocator_epoch")
    # 32-bit epoch
    epoch.end - epoch.start = 4
}