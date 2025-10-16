package policy

default allow := false

issuer_allowed if {
    input.phdr.cwt.iss == "did:x509:0:sha256:HnwZ4lezuxq_GVcl_Sk7YWW170qAD0DZBLXilXet0jg::eku:1.3.6.1.4.1.311.10.3.13"
}

seconds_since_epoch := time.now_ns() / 1000000000

iat_in_the_past if {
    input.phdr.cwt.iat < seconds_since_epoch
}

svn_undefined if {
    not input.phdr.cwt.svn
}

svn_positive if {
    input.phdr.cwt.svn >= 0
}

allow if {
    issuer_allowed
    iat_in_the_past
    svn_undefined
}

allow if {
    issuer_allowed
    iat_in_the_past
    svn_positive
}