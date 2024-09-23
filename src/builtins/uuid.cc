#include "builtins.h"

#include <uuid.h>

namespace
{
  using namespace rego;

  const std::int64_t lillian_15821015 = 2299160;
  const std::int64_t unix_19700101 = 2440587;
  const std::int64_t epoch = unix_19700101 - lillian_15821015;
  const std::int64_t g1582 = epoch * 86400;
  const std::int64_t g1582ns100 = g1582 * 10000000;

  BigInt get_time(const uuids::uuid& id)
  {
    auto bytes = id.as_bytes();
    std::int64_t time_low = (std::int64_t(bytes[0]) << 24) |
      (int(bytes[1]) << 16) | (int(bytes[2]) << 8) | int(bytes[3]);
    std::int64_t time_mid = (int(bytes[4]) << 8) | int(bytes[5]);
    std::int64_t time_hi = ((int(bytes[6]) << 8) | int(bytes[7])) & 0x0FFF;
    std::int64_t time = (time_hi << 48) | (time_mid << 32) | time_low;
    std::int64_t time_ns100 = time - g1582ns100;
    return time_ns100 * 100;
  }

  BigInt get_clock_sequence(const uuids::uuid& id)
  {
    auto bytes = id.as_bytes();
    std::int64_t clock_seq = (int(bytes[8]) << 8) | int(bytes[9]);
    return clock_seq & 0x3FFF;
  }

  std::string get_node_id(const uuids::uuid& id)
  {
    auto bytes = id.as_bytes();
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 10; i < 16; i++)
    {
      ss << std::setw(2) << int(bytes[i]);
      if (i < 15)
      {
        ss << '-';
      }
    }
    return ss.str();
  }

  std::string get_mac_vars(const uuids::uuid& id)
  {
    auto bytes = id.as_bytes();
    switch (int(bytes[10]) & 0b11)
    {
      case 0xb11:
        return "local:multicast";

      case 0b10:
        return "local:unicast";

      case 0b01:
        return "global:multicast";

      default:
        return "global:unicast";
    }
  }

  BigInt get_id(const uuids::uuid& id)
  {
    auto bytes = id.as_bytes();
    return (std::int64_t(bytes[0]) << 24) | (int(bytes[1]) << 16) |
      (int(bytes[2]) << 8) | int(bytes[3]);
  }

  std::string get_domain(const uuids::uuid& id)
  {
    auto bytes = id.as_bytes();
    auto d = int(bytes[9]);
    switch (d)
    {
      case 0:
        return "Person";

      case 1:
        return "Group";

      case 2:
        return "Org";

      default:
        return "Domain" + std::to_string(d);
    }
  }

  Node parse(const Nodes& args)
  {
    Node uuid =
      unwrap_arg(args, UnwrapOpt(0).func("uuid.parse").type(JSONString));
    if (uuid->type() == Error)
    {
      return uuid;
    }

    std::string uuid_str = get_string(uuid);
    if (starts_with(uuid_str, "urn:uuid:"))
    {
      uuid_str = uuid_str.substr(9);
    }

    std::optional<uuids::uuid> maybe_id = uuids::uuid::from_string(uuid_str);

    if (maybe_id.has_value())
    {
      uuids::uuid id = maybe_id.value();
      Node result = NodeDef::create(Object);
      switch (id.variant())
      {
        case uuids::uuid_variant::ncs:
          result
            << (ObjectItem << Resolver::term("variant")
                           << Resolver::term("ncs"));
          break;
        case uuids::uuid_variant::rfc:
          result
            << (ObjectItem << Resolver::term("variant")
                           << Resolver::term("RFC4122"));
          break;
        case uuids::uuid_variant::microsoft:
          result
            << (ObjectItem << Resolver::term("variant")
                           << Resolver::term("microsoft"));
          break;
        case uuids::uuid_variant::reserved:
          result
            << (ObjectItem << Resolver::term("variant")
                           << Resolver::term("reserved"));
          break;

        default:
          return err(uuid, "unknown uuid variant");
      }

      auto bytes = id.as_bytes();
      std::uint64_t version = static_cast<std::uint64_t>(bytes[6] >> 4);
      result
        << (ObjectItem << Resolver::term("version")
                       << Resolver::term(BigInt(version)));

      if (version == 1 || version == 2)
      {
        result
          << (ObjectItem << Resolver::term("time")
                         << Resolver::term(get_time(id)));
        result
          << (ObjectItem << Resolver::term("clocksequence")
                         << Resolver::term(get_clock_sequence(id)));
        result
          << (ObjectItem << Resolver::term("nodeid")
                         << Resolver::term(get_node_id(id)));
        result
          << (ObjectItem << Resolver::term("macvariables")
                         << Resolver::term(get_mac_vars(id)));
        if (version == 2)
        {
          result
            << (ObjectItem << Resolver::term("id")
                           << Resolver::term(get_id(id)));
          result
            << (ObjectItem << Resolver::term("domain")
                           << Resolver::term(get_domain(id)));
        }
      }

      return result;
    }
    else
    {
      return err(uuid, "invalid uuid");
    }
  }

  struct UUIDRFC4122 : public BuiltInDef
  {
    std::random_device rd;
    std::mt19937 generator;
    std::map<std::string, Node> cache;

    UUIDRFC4122() :
      BuiltInDef(Location("uuid.rfc4122"), 1, [this](const Nodes& args) {
        return call(args);
      })
    {
      auto seed_data = std::array<int, std::mt19937::state_size>{};
      std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
      std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
      generator = std::mt19937(seq);
    }

    Node call(const Nodes& args)
    {
      Node k =
        unwrap_arg(args, UnwrapOpt(0).func("uuid.rfc4122").type(JSONString));
      if (k->type() == Error)
      {
        return k;
      }

      std::string k_str = get_string(k);
      if (contains(cache, k_str))
      {
        return cache[k_str]->clone();
      }

      uuids::uuid_random_generator gen(generator);
      std::string uuid_str = uuids::to_string(gen());
      Node uuid = JSONString ^ uuid_str;
      cache[k_str] = uuid;
      return uuid;
    }

    void clear() override
    {
      cache.clear();
    }
  };
}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> uuid()
    {
      return {
        BuiltInDef::create(Location("uuid.parse"), 1, parse),
        std::make_shared<UUIDRFC4122>(),
      };
    }
  }
}