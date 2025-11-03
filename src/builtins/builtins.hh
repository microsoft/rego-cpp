#pragma once

#include "internal.hh"
#include "rego.hh"

namespace rego
{
  namespace builtins
  {
    BuiltIn array(const Location& name);
    BuiltIn aws(const Location& name);
    BuiltIn base64(const Location& name);
    BuiltIn base64url(const Location& name);
    BuiltIn bits(const Location& name);
    BuiltIn core(const Location& name);
    BuiltIn crypto(const Location& name);
    BuiltIn glob(const Location& name);
    BuiltIn graph(const Location& name);
    BuiltIn graphql(const Location& name);
    BuiltIn hex(const Location& name);
    BuiltIn http(const Location& name);
    BuiltIn internal(const Location& name);
    BuiltIn json(const Location& name);
    BuiltIn jwt(const Location& name);
    BuiltIn net(const Location& name);
    BuiltIn numbers(const Location& name);
    BuiltIn object(const Location& name);
    BuiltIn opa(const Location& name);
    BuiltIn rand(const Location& name);
    BuiltIn rego(const Location& name);
    BuiltIn regex(const Location& name);
    BuiltIn semver(const Location& name);
    BuiltIn strings(const Location& name);
    BuiltIn time(const Location& name);
    BuiltIn units(const Location& name);
    BuiltIn urlquery(const Location& name);
    BuiltIn uuid(const Location& name);
    BuiltIn yaml(const Location& name);
  }
}