#include "builtins.h"

namespace
{
  using namespace rego;

  const std::int64_t lillian_15821015 = 2299160;
  const std::int64_t unix_19700101 = 2440587;
  const std::int64_t epoch = unix_19700101 - lillian_15821015;
  const std::int64_t g1582 = epoch * 86400;
  const std::int64_t g1582ns100 = g1582 * 10000000;
  using uuid = std::array<std::uint8_t, 16>;

  enum class uuid_variant
  {
    ncs,
    rfc,
    microsoft,
    reserved
  };

  uuid_variant uuid_variant(const uuid& id)
  {
    if ((id[8] & 0x80) == 0x00)
      return uuid_variant::ncs;

    if ((id[8] & 0xC0) == 0x80)
      return uuid_variant::rfc;

    if ((id[8] & 0xE0) == 0xC0)
      return uuid_variant::microsoft;

    return uuid_variant::reserved;
  }

  bool is_hex(char c)
  {
    switch (c)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'a':
      case 'A':
      case 'b':
      case 'B':
      case 'c':
      case 'C':
      case 'd':
      case 'D':
      case 'e':
      case 'E':
      case 'f':
      case 'F':
        return true;

      default:
        return false;
    }
  }

  std::uint8_t hex_to_byte(char c)
  {
    switch (c)
    {
      case '0':
        return 0;

      case '1':
        return 1;

      case '2':
        return 2;

      case '3':
        return 3;

      case '4':
        return 4;

      case '5':
        return 5;

      case '6':
        return 6;

      case '7':
        return 7;

      case '8':
        return 8;

      case '9':
        return 9;

      case 'a':
      case 'A':
        return 10;

      case 'b':
      case 'B':
        return 11;

      case 'c':
      case 'C':
        return 12;

      case 'd':
      case 'D':
        return 13;

      case 'e':
      case 'E':
        return 14;

      case 'f':
      case 'F':
        return 15;
    }

    throw std::runtime_error("invalid hex character: " + std::to_string(c));
  }

  std::optional<uuid> uuid_from_string(const std::string& str)
  {
    bool firstDigit = true;
    size_t hasBraces = 0;
    size_t index = 0;

    uuid id{{0}};

    if (str.empty())
      return {};

    if (str.front() == '{')
      hasBraces = 1;
    if (hasBraces && str.back() != '}')
      return {};

    for (size_t i = hasBraces; i < str.size() - hasBraces; ++i)
    {
      if (str[i] == '-')
        continue;

      if (index >= 16 || !is_hex(str[i]))
      {
        return {};
      }

      if (firstDigit)
      {
        id[index] = hex_to_byte(str[i]) << 4;
        firstDigit = false;
      }
      else
      {
        id[index] = static_cast<uint8_t>(id[index] | hex_to_byte(str[i]));
        index++;
        firstDigit = true;
      }
    }

    if (index < 16)
    {
      return {};
    }

    return id;
  }

  std::string uuid_to_string(const uuid& id)
  {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; i++)
    {
      ss << std::setw(2) << int(id[i]);
      if (i == 3 || i == 5 || i == 7 || i == 9)
      {
        ss << '-';
      }
    }
    return ss.str();
  }

  uuid uuid_random(std::mt19937& generator)
  {
    std::uniform_int_distribution<uint32_t> distribution;
    uuid id;
    for (int i = 0; i < 16; i += 4)
      *reinterpret_cast<uint32_t*>(id.data() + i) = distribution(generator);

    // variant must be 10xxxxxx
    id[8] &= 0xBF;
    id[8] |= 0x80;

    // version must be 0100xxxx
    id[6] &= 0x4F;
    id[6] |= 0x40;

    return id;
  }

  BigInt get_time(const uuid& id)
  {
    std::int64_t time_low = (std::int64_t(id[0]) << 24) | (int(id[1]) << 16) |
      (int(id[2]) << 8) | int(id[3]);
    std::int64_t time_mid = (int(id[4]) << 8) | int(id[5]);
    std::int64_t time_hi = ((int(id[6]) << 8) | int(id[7])) & 0x0FFF;
    std::int64_t time = (time_hi << 48) | (time_mid << 32) | time_low;
    std::int64_t time_ns100 = time - g1582ns100;
    return time_ns100 * 100;
  }

  BigInt get_clock_sequence(const uuid& id)
  {
    std::int64_t clock_seq = (int(id[8]) << 8) | int(id[9]);
    return clock_seq & 0x3FFF;
  }

  std::string get_node_id(const uuid& id)
  {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 10; i < 16; i++)
    {
      ss << std::setw(2) << int(id[i]);
      if (i < 15)
      {
        ss << '-';
      }
    }
    return ss.str();
  }

  std::string get_mac_vars(const uuid& id)
  {
    switch (int(id[10]) & 0b11)
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

  BigInt get_id(const uuid& id)
  {
    return (std::int64_t(id[0]) << 24) | (int(id[1]) << 16) |
      (int(id[2]) << 8) | int(id[3]);
  }

  std::string get_domain(const uuid& id)
  {
    auto d = int(id[9]);
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
    Node uuid_node =
      unwrap_arg(args, UnwrapOpt(0).func("uuid.parse").type(JSONString));
    if (uuid_node->type() == Error)
    {
      return uuid_node;
    }

    std::string uuid_str = get_string(uuid_node);
    if (starts_with(uuid_str, "urn:uuid:"))
    {
      uuid_str = uuid_str.substr(9);
    }

    std::optional<uuid> maybe_id = uuid_from_string(uuid_str);

    if (maybe_id.has_value())
    {
      uuid id = maybe_id.value();
      Node result = NodeDef::create(Object);
      switch (uuid_variant(id))
      {
        case uuid_variant::ncs:
          result
            << (ObjectItem << Resolver::term("variant")
                           << Resolver::term("ncs"));
          break;
        case uuid_variant::rfc:
          result
            << (ObjectItem << Resolver::term("variant")
                           << Resolver::term("RFC4122"));
          break;
        case uuid_variant::microsoft:
          result
            << (ObjectItem << Resolver::term("variant")
                           << Resolver::term("microsoft"));
          break;
        case uuid_variant::reserved:
          result
            << (ObjectItem << Resolver::term("variant")
                           << Resolver::term("reserved"));
          break;

        default:
          return err(uuid_node, "unknown uuid variant");
      }

      std::size_t version = static_cast<std::size_t>(id[6] >> 4);
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
      return err(uuid_node, "invalid uuid");
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

      std::string uuid_str = uuid_to_string(uuid_random(generator));
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