#include "internal.hh"
#include "rego.hh"
#include "trieste/wf.h"

#include <sstream>
#include <stdexcept>

namespace
{
  using namespace rego;
  namespace b = rego::bundle;
  namespace bi = rego::builtins;

  const uint32_t CRC32Init = 0;

  uint32_t crc32_add(uint32_t crc, const uint8_t* ptr, size_t len)
  {
    const uint32_t s_crc32[16] = {
      0,
      0x1db71064,
      0x3b6e20c8,
      0x26d930ac,
      0x76dc4190,
      0x6b6b51f4,
      0x4db26158,
      0x5005713c,
      0xedb88320,
      0xf00f9344,
      0xd6d6a3e8,
      0xcb61b38c,
      0x9b64c2b0,
      0x86d3d2d4,
      0xa00ae278,
      0xbdbdf21c};
    uint32_t crcu32 = (uint32_t)crc;
    if (ptr == nullptr)
    {
      return CRC32Init;
    }

    crcu32 = ~crcu32;
    for (size_t i = 0; i < len; ++i)
    {
      uint8_t b = *ptr++;
      crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
      crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
    }

    return ~crcu32;
  }

#if __cpp_lib_endian >= 201907L
#define LITTLEENDIAN constexpr(std::endian::native == std::endian::little)
#else
  inline bool is_littleendian()
  {
    const int value{0x01};
    const void* address = static_cast<const void*>(&value);
    const unsigned char* least_significant_address =
      static_cast<const unsigned char*>(address);
    return (*least_significant_address == 0x01);
  }
#define LITTLEENDIAN (is_littleendian())
#endif

  template <size_t LEN, typename Char>
  void write_endian(std::basic_ostream<Char>& stream, const uint8_t* ptr)
  {
    if LITTLEENDIAN
    {
      stream.write(reinterpret_cast<const Char*>(ptr), LEN);
      return;
    }

    auto ptr_end = reinterpret_cast<const Char*>(ptr + LEN - 1);
    for (size_t i = 0; i < LEN; ++i, ptr_end--)
    {
      stream.put(*ptr_end);
    }
  }

  template <size_t LEN, typename Char>
  void read_endian(std::basic_istream<Char>& stream, uint8_t* ptr)
  {
    if LITTLEENDIAN
    {
      stream.read(reinterpret_cast<Char*>(ptr), LEN);
      return;
    }

    auto ptr_end = reinterpret_cast<Char*>(ptr + LEN - 1);
    for (size_t i = 0; i < LEN; ++i, ptr_end--)
    {
      *ptr_end = stream.get();
    }
  }

  namespace bson
  {
    const int8_t StringId = 2;
    const int8_t DocumentId = 3;
    const int8_t ArrayId = 4;
    const int8_t BinaryId = 5;
    const int8_t BooleanId = 8;
    const int8_t NullId = 10;

    const uint8_t FalseId = 0;
    const uint8_t TrueId = 1;
    // All floats and integers are stored in user extension
    // binary fields because Rego supports arbitrary precision
    // due to representing all numbers as strings (a la JSON)
    const uint8_t IntStringId = 128;
    const uint8_t FloatStringId = 129;

    template <typename Char>
    void write_byte(std::basic_ostream<Char>& stream, uint8_t value)
    {
      stream.put((Char)value);
    }

    template <typename Char>
    void write_uint32(std::basic_ostream<Char>& stream, uint32_t value)
    {
      const uint8_t* parts = reinterpret_cast<const uint8_t*>(&value);
      write_endian<4>(stream, parts);
    }

    template <typename Char>
    void write_int32(std::basic_ostream<Char>& stream, int32_t value)
    {
      const uint8_t* parts = reinterpret_cast<const uint8_t*>(&value);
      write_endian<4>(stream, parts);
    }

    template <typename Char>
    void write_uint64(std::basic_ostream<Char>& stream, uint64_t value)
    {
      const uint8_t* parts = reinterpret_cast<const uint8_t*>(&value);
      write_endian<8>(stream, parts);
    }

    template <typename Char>
    void write_int64(std::basic_ostream<Char>& stream, int64_t value)
    {
      const uint8_t* parts = reinterpret_cast<const uint8_t*>(&value);
      write_endian<8>(stream, parts);
    }

    template <typename Char>
    void write_cstring(
      std::basic_ostream<Char>& stream, const std::string_view& value)
    {
      const Char* parts = reinterpret_cast<const Char*>(value.data());
      stream.write((const Char*)parts, value.size());
      stream.put(0);
    }

    template <typename Char>
    void write_string(
      std::basic_ostream<Char>& stream, const std::string_view& value)
    {
      const Char* parts = reinterpret_cast<const Char*>(value.data());
      write_int32(stream, value.size() + 1);
      stream.write((const Char*)parts, value.size());
      stream.put(0);
    }

    template <typename Char>
    void write_document(
      std::basic_ostream<Char>& stream, const std::string_view& value)
    {
      const Char* parts = reinterpret_cast<const Char*>(value.data());
      write_int32(stream, static_cast<int32_t>(value.size()));
      stream.write((const Char*)parts, value.size());
      stream.put(0);
    }

    template <typename Char>
    uint8_t read_byte(std::basic_istream<Char>& stream)
    {
      return static_cast<uint8_t>(stream.get());
    }

    template <typename Char>
    int8_t read_sbyte(std::basic_istream<Char>& stream)
    {
      return static_cast<int8_t>(stream.get());
    }

    template <typename Char>
    uint32_t read_uint32(std::basic_istream<Char>& stream)
    {
      uint32_t value;
      uint8_t* parts = reinterpret_cast<uint8_t*>(&value);
      read_endian<4>(stream, parts);
      return value;
    }

    template <typename Char>
    int32_t read_int32(std::basic_istream<Char>& stream)
    {
      int32_t value;
      uint8_t* parts = reinterpret_cast<uint8_t*>(&value);
      read_endian<4>(stream, parts);
      return value;
    }

    template <typename Char>
    uint64_t read_uint64(std::basic_istream<Char>& stream)
    {
      uint64_t value;
      uint8_t* parts = reinterpret_cast<uint8_t*>(&value);
      read_endian<8>(stream, parts);
      return value;
    }

    template <typename Char>
    int64_t read_int64(std::basic_istream<Char>& stream)
    {
      int64_t value;
      uint8_t* parts = reinterpret_cast<uint8_t*>(&value);
      read_endian<8>(stream, parts);
      return value;
    }

    template <typename Char>
    uint64_t read_size(std::basic_istream<Char>& stream)
    {
      uint32_t value;
      uint8_t* parts = reinterpret_cast<uint8_t*>(&value);
      read_endian<4>(stream, parts);
      return static_cast<size_t>(value);
    }

    template <typename Char>
    std::basic_string<Char> read_cstring(std::basic_istream<Char>& stream)
    {
      std::basic_string<Char> value;
      Char c = stream.get();
      while (c != 0)
      {
        value.push_back(c);
        c = stream.get();
      }

      return value;
    }

    template <typename Char>
    std::basic_string<Char> read_string(std::basic_istream<Char>& stream)
    {
      size_t num_bytes = read_size(stream);
      std::basic_string<Char> value;
      value.resize(num_bytes - 1);
      stream.read(value.data(), num_bytes - 1);
      stream.get(); // consume the null character at the end
      return value;
    }

    template <typename Char>
    void write_term(
      std::basic_ostream<Char>& stream,
      const std::string& key,
      const Node& term);

    template <typename Char>
    void write_objectitem(std::basic_ostream<Char>& stream, const Node& objitem)
    {
      std::string key = to_key(objitem / Key);
      write_term(stream, key, objitem / Val);
    }

    template <typename Char>
    void write_object(std::basic_ostream<Char>& stream, const Node& obj)
    {
      std::basic_ostringstream<Char> bson_buf(std::ios::binary);
      for (auto& objitem : *obj)
      {
        write_objectitem(bson_buf, objitem);
      }

      write_document(stream, bson_buf.view());
    }

    template <typename Char>
    void write_array(std::basic_ostream<Char>& stream, const Node& array)
    {
      std::basic_ostringstream<Char> bson_buf(std::ios::binary);
      for (size_t i = 0; i < array->size(); ++i)
      {
        write_term(bson_buf, std::to_string(i), array->at(i));
      }

      write_document(stream, bson_buf.view());
    }

    template <typename Char>
    void write_term(
      std::basic_ostream<Char>& stream,
      const std::string& key,
      const Node& term)
    {
      Node value = term->front();
      if (value == Scalar)
      {
        value = value->front();
        if (value->in({Int, Float}))
        {
          write_byte(stream, BinaryId);
          write_cstring(stream, strip_quotes(key));
          std::string_view number = value->location().view();
          const Char* ptr = reinterpret_cast<const Char*>(number.data());
          write_int32(stream, static_cast<int32_t>(number.size()));
          stream.put(value == Int ? IntStringId : FloatStringId);
          stream.write((const Char*)ptr, number.size());
          return;
        }

        if (value->in({True, False}))
        {
          write_byte(stream, bson::BooleanId);
          write_cstring(stream, strip_quotes(key));
          write_byte(stream, value == True ? TrueId : FalseId);
          return;
        }

        if (value == Null)
        {
          write_byte(stream, NullId);
          write_cstring(stream, strip_quotes(key));
          return;
        }

        if (value == JSONString)
        {
          write_byte(stream, StringId);
          write_cstring(stream, strip_quotes(key));
          write_string(stream, strip_quotes(value->location().view()));
          return;
        }

        logging::Error() << "Invalid Scalar: " << value;
        throw std::runtime_error("Invalid Scalar node");
      }

      if (value->in({Array, Set}))
      {
        write_byte(stream, ArrayId);
        write_cstring(stream, strip_quotes(key));
        write_array(stream, value);
        return;
      }

      if (value == Object)
      {
        write_byte(stream, DocumentId);
        write_cstring(stream, strip_quotes(key));
        write_object(stream, value);
      }
    }

    template <typename Char>
    Node read_element(std::basic_istream<Char>& stream, int8_t element_id);

    template <typename Char>
    Node read_object(std::basic_istream<Char>& stream)
    {
      size_t size = static_cast<size_t>(read_int32(stream));
      uint64_t start = static_cast<uint64_t>(stream.tellg());
      uint64_t end = start + size + 1;

      Node object = NodeDef::create(Object);
      int8_t element_id = read_sbyte(stream);
      while (element_id != 0 && !stream.eof() && stream.tellg() < end)
      {
        auto key = read_cstring(stream);
        object << object_item(scalar(key), read_element(stream, element_id));
        element_id = read_sbyte(stream);
      }

      assert(element_id == 0 && stream.tellg() == end);

      return object;
    }

    template <typename Char>
    Node read_array(std::basic_istream<Char>& stream)
    {
      size_t size = static_cast<size_t>(read_int32(stream));
      uint64_t start = static_cast<uint64_t>(stream.tellg());
      uint64_t end = start + size + 1;

      Node array = NodeDef::create(Array);
      int8_t element_id = read_sbyte(stream);
      while (element_id != 0 && !stream.eof() && stream.tellg() < end)
      {
        read_cstring(stream);
        array << read_element(stream, element_id);
        element_id = read_sbyte(stream);
      }

      assert(element_id == 0 && stream.tellg() == start + size + 1);

      return array;
    }

    template <typename Char>
    Node read_element(std::basic_istream<Char>& stream, int8_t element_id)
    {
      switch (element_id)
      {
        case BooleanId: {
          uint8_t boolean_id = read_byte(stream);
          if (boolean_id == TrueId)
          {
            return scalar(true);
          }

          if (boolean_id == FalseId)
          {
            return scalar(false);
          }

          logging::Error() << "Invalid boolean id: " << boolean_id;
          throw std::format_error("Invalid boolean id");
        }

        case NullId:
          return scalar();

        case StringId:
          return scalar(read_string(stream));

        case BinaryId: {
          size_t size = static_cast<size_t>(read_int32(stream));
          uint8_t subtype_id = read_byte(stream);
          if (!(subtype_id == IntStringId || subtype_id == FloatStringId))
          {
            logging::Error() << "Invalid binary subtype: " << subtype_id;
            throw std::format_error("Invalid binary subtype");
          }
          std::basic_string<Char> string;
          string.resize(size);
          stream.read(string.data(), size);
          return Scalar
            << (subtype_id == IntStringId ? Int ^ string : Float ^ string);
        }

        case ArrayId:
          return read_array(stream);

        case DocumentId:
          return read_object(stream);

        default:
          logging::Error() << "Invalid element id: " << element_id;
          throw std::format_error("Invalid element ID");
      }
    }

  }

  const char* Magic = "REGOBUND";
  const uint8_t RegoVersion = 1;
  const uint8_t RegoBinaryVersion = 1;
  const size_t NumReservedBytes = 5;
  const size_t NumForwardPointers = 4;
  const size_t HeaderSize = strlen(Magic) + 1 + 1 + 1 + NumReservedBytes +
    2 * sizeof(uint32_t) + sizeof(uint64_t) * (NumForwardPointers + 1);
  const int8_t StaticId = 1;
  const int8_t PlansId = 2;
  const int8_t FuncsId = 3;
  const int8_t DataId = 4;
  const int8_t FooterId = 5;
  const int8_t BITAny = 1;
  const int8_t BITNumber = 2;
  const int8_t BITString = 3;
  const int8_t BITBoolean = 4;
  const int8_t BITNull = 5;
  const int8_t BITArrayDynamic = 6;
  const int8_t BITArrayStatic = 7;
  const int8_t BITObjectDynamic = 8;
  const int8_t BITObjectStatic = 9;
  const int8_t BITObjectHybrid = 10;
  const int8_t BITSet = 11;
  const int8_t BITTypeSeq = 12;

  class oregostream
  {
  public:
    oregostream() : m_ostream(std::ios::binary) {}

    void write_bundle(const BundleDef& bundle)
    {
      write_header(bundle.local_count, bundle.query_plan);
      write_static(
        bundle.strings, bundle.builtin_functions, bundle.files, bundle.query);
      write_plans(bundle.plans);
      write_funcs(bundle.functions);
      write_data(bundle.data);
      update_header();
    }

    std::string_view view()
    {
      return m_ostream.view();
    }

  private:
    void write_header(size_t local_count, std::optional<size_t> query_plan)
    {
      for (size_t i = 0; i < strlen(Magic); ++i)
      {
        put(static_cast<uint8_t>(Magic[i]));
      }

      put(RegoVersion);
      put(RegoBinaryVersion);
      if (query_plan.has_value())
      {
        write_sbyte(static_cast<int8_t>(query_plan.value()));
      }
      else
      {
        write_sbyte(-1);
      }

      for (size_t i = 0; i < NumReservedBytes; ++i)
      {
        put(0);
      }

      write_size(local_count);
      write_uint32(0); // crc32 of everything after the header
      m_header_table = position();
      write_uint64(0); // size
      write_uint64(0); // loc(static)
      write_uint64(0); // loc(plans)
      write_uint64(0); // loc(funcs)
      write_uint64(0); // loc(data)
    }

    void update_header()
    {
      const int num_locs = 5;
      const int preamble_size =
        strlen(Magic) + 1 + 1 + 1 + NumReservedBytes + sizeof(uint32_t);
      std::string_view bytes = m_ostream.view();
      uint64_t start = HeaderSize;
      uint64_t size = bytes.size() - start;
      const uint8_t* ptr =
        reinterpret_cast<const uint8_t*>(bytes.data() + start);
      uint32_t crc32 = crc32_add(CRC32Init, ptr, size);
      m_ostream.seekp(preamble_size, std::ios::beg);
      write_uint32(crc32);
      write_uint64(size);
    }

    void update_forward_pointer(uint8_t id)
    {
      size_t loc = position();
      m_ostream.seekp(m_header_table + id * sizeof(uint64_t), std::ios::beg);
      write_uint64(loc);
      m_ostream.seekp(0, std::ios::end);
    }

    void write_static(
      const std::vector<Location>& strings,
      const std::map<Location, Node> builtin_functions,
      const std::vector<Source>& files,
      const Source query)
    {
      update_forward_pointer(StaticId);
      write_sbyte(StaticId);
      write_files(files);
      write_strings(strings);
      write_builtin_funcs(builtin_functions);
      if (query == nullptr)
      {
        write_sbyte(1);
      }
      else
      {
        write_sbyte(2);
        write_string(query->view());
      }
    }

    void write_plans(const std::vector<b::Plan> plans)
    {
      uint64_t start = position();
      write_sbyte(PlansId);
      write_size(plans.size());
      std::map<Location, size_t> locs;
      for (auto& plan : plans)
      {
        locs[plan.name] = position();
        write_plan(plan);
      }

      update_forward_pointer(PlansId);
      write_table(start, locs);
    }

    void write_funcs(const std::vector<b::Function>& funcs)
    {
      uint64_t start = position();
      write_sbyte(FuncsId);
      write_size(funcs.size());
      std::map<Location, size_t> locs;
      for (auto& func : funcs)
      {
        locs[func.name] = position();
        write_func(func);
      }

      update_forward_pointer(FuncsId);
      write_table(start, locs);
    }

    void write_data(const Node& data)
    {
      update_forward_pointer(DataId);
      write_sbyte(DataId);
      WFContext ctx(wf_bundle);
      bson::write_object(m_ostream, data);
    }

    uint64_t position()
    {
      return static_cast<uint64_t>(m_ostream.tellp());
    }

    void put(const uint8_t value)
    {
      m_ostream.put(static_cast<std::ostream::char_type>(value));
    }

    template <size_t LEN>
    void write_endian(const uint8_t* ptr)
    {
      ::write_endian<LEN>(m_ostream, ptr);
    }

    void write(const uint8_t* ptr, size_t len)
    {
      m_ostream.write(
        reinterpret_cast<const std::ostream::char_type*>(ptr), len);
    }

    void write_int32(int32_t value)
    {
      const uint8_t* parts = reinterpret_cast<const uint8_t*>(&value);
      write_endian<4>(parts);
    }

    void write_uint32(uint32_t value)
    {
      const uint8_t* parts = reinterpret_cast<const uint8_t*>(&value);
      write_endian<4>(parts);
    }

    void write_byte(uint8_t value)
    {
      put((uint8_t)value);
    }

    void write_sbyte(int8_t value)
    {
      put((uint8_t)value);
    }

    void write_uint64(uint64_t value)
    {
      const uint8_t* parts = reinterpret_cast<const uint8_t*>(&value);
      write_endian<8>(parts);
    }

    void write_int64(int64_t value)
    {
      const uint8_t* parts = reinterpret_cast<const uint8_t*>(&value);
      write_endian<8>(parts);
    }

    void write_size(size_t value)
    {
      write_uint32(static_cast<uint32_t>(value));
    }

    void write_string(const std::string_view& value)
    {
      const uint8_t* ptr = reinterpret_cast<const uint8_t*>(value.data());
      write_int32(value.size());
      write(ptr, value.size());
    }

    void write_location(const Location& loc)
    {
      if (loc.source == nullptr)
      {
        write_byte(1);
        return;
      }

      write_byte(2);
      write_size(m_files[loc.source->origin()]);
      write_size(loc.pos);
      write_size(loc.len);
    }

    void write_strings(const std::vector<Location>& strings)
    {
      write_uint32(strings.size());
      for (auto& loc : strings)
      {
        write_string(loc.view());
      }
    }

    void write_bkv_list(const Node& object)
    {
      write_byte(static_cast<uint8_t>(object->size()));
      for (const Node& item : *object)
      {
        write_builtin_type(item / Key);
        write_builtin_type(item / Val);
      }
    }

    void write_builtin_type(const Node& type)
    {
      if (type == bi::Any)
      {
        write_byte(BITAny);
        return;
      }

      if (type == bi::Number)
      {
        write_byte(BITNumber);
        return;
      }

      if (type == bi::String)
      {
        write_byte(BITString);
        return;
      }

      if (type == bi::Boolean)
      {
        write_byte(BITBoolean);
        return;
      }

      if (type == bi::Null)
      {
        write_byte(BITNull);
        return;
      }

      if (type == bi::DynamicArray)
      {
        write_builtin_type(type->front());
        return;
      }

      if (type == bi::StaticArray)
      {
        write_byte(static_cast<uint8_t>(type->size()));
        for (auto& n : *type)
        {
          write_builtin_type(n);
        }
        return;
      }

      if (type == bi::DynamicObject)
      {
        write_builtin_type(type / Key);
        write_builtin_type(type / Val);
        return;
      }

      if (type == bi::StaticObject)
      {
        write_bkv_list(type);
        return;
      }

      if (type == bi::HybridObject)
      {
        write_builtin_type(type / bi::DynamicObject / Key);
        write_builtin_type(type / bi::DynamicObject / Val);
        write_bkv_list(type / bi::StaticObject);
      }

      if (type == bi::Set)
      {
        write_builtin_type(type->front());
        return;
      }

      if (type == bi::TypeSeq)
      {
        write_byte(static_cast<uint8_t>(type->size()));
        for (auto& n : *type)
        {
          write_builtin_type(n);
        }

        return;
      }

      throw std::runtime_error("Unsupported builtin type");
    }

    void write_builtin_arg(const Node& arg)
    {
      write_string((arg / bi::Name)->location().view());
      write_string((arg / bi::Description)->location().view());
      write_builtin_type(arg / bi::Type);
    }

    void write_builtin_decl(const Node& decl)
    {
      Node args = decl / bi::Args;
      Node ret = decl / bi::Return;

      if (args == bi::VarArgs)
      {
        write_byte(1);
      }
      else if (args == bi::ArgSeq)
      {
        write_byte(2);
        write_byte(static_cast<uint8_t>(args->size()));
        for (auto arg : *args)
        {
          write_builtin_arg(arg);
        }
      }
      else
      {
        logging::Error() << "Unsupported builtin args: " << args;
        throw std::runtime_error("Unsupported builtin args");
      }

      if (ret == bi::Void)
      {
        write_byte(1);
      }
      else if (ret == bi::Result)
      {
        write_byte(2);
        write_string((ret / bi::Name)->location().view());
        write_string((ret / bi::Description)->location().view());
        write_builtin_type(ret / bi::Type);
      }
      else
      {
        logging::Error() << "Unsupported builtin return: " << ret;
        throw std::runtime_error("Unsupported builtin return");
      }
    }

    void write_builtin_funcs(const std::map<Location, Node>& builtin_funcs)
    {
      write_size(builtin_funcs.size());
      for (auto [name, decl] : builtin_funcs)
      {
        write_string(name.view());
        write_builtin_decl(decl);
      }
    }

    void write_files(const std::vector<Source>& files)
    {
      write_size(files.size());
      for (auto& file : files)
      {
        write_string(file->origin());
        write_string(file->view());
      }
    }

    void write_table(uint64_t pos, const std::map<Location, size_t>& locs)
    {
      write_uint64(pos);
      write_size(locs.size());
      for (auto& [name, loc] : locs)
      {
        write_string(name.view());
        write_uint64(loc);
      }
    }

    void write_operand(const b::Operand& operand)
    {
      int8_t id = static_cast<int8_t>(operand.type);
      switch (operand.type)
      {
        case b::OperandType::Local:
        case b::OperandType::String:
          write_byte(id);
          write_size(operand.index);
          break;

        case b::OperandType::True:
        case b::OperandType::False:
          write_byte(id);
          break;

        default:
          throw std::runtime_error("Not an operand (index or value)");
      }
    }

    void write_operand_array(const std::vector<b::Operand>& operands)
    {
      put(static_cast<uint8_t>(operands.size()));
      for (auto& op : operands)
      {
        write_operand(op);
      }
    }

    void write_ipath(const std::vector<size_t>& path)
    {
      put(static_cast<uint8_t>(path.size()));
      for (auto& i : path)
      {
        write_int32(static_cast<int32_t>(i));
      }
    }

    void write_statement(const b::Statement& statement)
    {
      write_byte(static_cast<int8_t>(statement.type));
      write_location(statement.location);

      switch (statement.type)
      {
        case b::StatementType::ArrayAppend:
          write_size(statement.target);
          write_operand(statement.op0);
          break;

        case b::StatementType::AssignInt:
          write_int64(statement.op0.value);
          write_size(statement.target);
          break;

        case b::StatementType::AssignVarOnce:
        case b::StatementType::AssignVar:
          write_operand(statement.op0);
          write_size(statement.target);
          break;

        case b::StatementType::Block:
          write_blocks(statement.ext->blocks());
          break;

        case b::StatementType::Break:
          write_size(statement.op0.index);
          break;

        case b::StatementType::Call:
          write_string(statement.ext->call().func.view());
          write_operand_array(statement.ext->call().ops);
          write_size(statement.target);
          break;

        case b::StatementType::CallDynamic:
          write_operand_array(statement.ext->call_dynamic().path);
          write_operand_array(statement.ext->call_dynamic().ops);
          write_size(statement.target);
          break;

        case b::StatementType::Dot:
          write_operand(statement.op0);
          write_operand(statement.op1);
          write_size(statement.target);
          break;

        case b::StatementType::Equal:
          write_operand(statement.op0);
          write_operand(statement.op1);
          break;

        case b::StatementType::IsArray:
          write_operand(statement.op0);
          break;

        case b::StatementType::IsDefined:
          write_size(statement.target);
          break;

        case b::StatementType::IsObject:
          write_operand(statement.op0);
          break;

        case b::StatementType::IsSet:
          write_operand(statement.op0);
          break;

        case b::StatementType::IsUndefined:
          write_size(statement.target);
          break;

        case b::StatementType::Len:
          write_operand(statement.op0);
          write_size(statement.target);
          break;

        case b::StatementType::MakeArray:
          write_int32(static_cast<int32_t>(statement.op0.value));
          write_size(statement.target);
          break;

        case b::StatementType::MakeNull:
          write_size(statement.target);
          break;

        case b::StatementType::MakeNumberInt:
          write_int64(statement.op0.value);
          write_size(statement.target);
          break;

        case b::StatementType::MakeNumberRef:
          write_size(statement.op0.index);
          write_size(statement.target);
          break;

        case b::StatementType::MakeObject:
          write_size(statement.target);
          break;

        case b::StatementType::MakeSet:
          write_size(statement.target);
          break;

        case b::StatementType::NotEqual:
          write_operand(statement.op0);
          write_operand(statement.op1);
          break;

        case b::StatementType::Not:
          write_block(statement.ext->block());
          break;

        case b::StatementType::ObjectInsertOnce:
        case b::StatementType::ObjectInsert:
          write_operand(statement.op0);
          write_operand(statement.op1);
          write_size(statement.target);
          break;

        case b::StatementType::ObjectMerge:
          write_size(statement.op0.index);
          write_size(statement.op1.index);
          write_size(statement.target);
          break;

        case b::StatementType::ResetLocal:
        case b::StatementType::ResultSetAdd:
        case b::StatementType::ReturnLocal:
          write_size(statement.target);
          break;

        case b::StatementType::Scan:
          write_size(statement.target);
          write_size(statement.op0.index);
          write_size(statement.op1.index);
          write_block(statement.ext->block());
          break;

        case b::StatementType::SetAdd:
          write_operand(statement.op0);
          write_size(statement.target);
          break;

        case b::StatementType::With:
          write_size(statement.target);
          write_operand(statement.op0);
          write_ipath(statement.ext->with().path);
          write_block(statement.ext->with().block);
          break;

        case b::StatementType::Nop:
          logging::Info() << "Stripping NopStmt during serialization";
          break;
      }
    }

    void write_block(const b::Block& block)
    {
      write_size(block.size());
      for (auto& stmt : block)
      {
        write_statement(stmt);
      }
    }

    void write_blocks(const std::vector<b::Block>& blocks)
    {
      write_size(blocks.size());
      for (auto& block : blocks)
      {
        write_block(block);
      }
    }

    void write_plan(const b::Plan& plan)
    {
      write_string(plan.name.view());
      write_blocks(plan.blocks);
    }

    void write_path(const std::vector<Location>& path)
    {
      put(static_cast<uint8_t>(path.size()));
      for (auto& loc : path)
      {
        write_string(loc.view());
      }
    }

    void write_params(const std::vector<size_t>& params)
    {
      put(static_cast<uint8_t>(params.size()));
      for (auto& local : params)
      {
        write_size(local);
      }
    }

    void write_func(const b::Function& func)
    {
      write_string(func.name.view());
      write_path(func.path);
      write_params(func.parameters);
      write_size(func.result);
      write_blocks(func.blocks);
    }

  private:
    std::ostringstream m_ostream;
    std::map<std::string, size_t> m_files;
    uint64_t m_header_table;
  };

  class iregostream
  {
  public:
    iregostream(std::string&& bytes) : m_istream(bytes, std::ios::binary) {}

    Bundle read_bundle(size_t local_count, int8_t query_plan)
    {
      BundleDef bundle;
      bundle.local_count = local_count;
      if (query_plan >= 0)
      {
        bundle.query_plan = static_cast<size_t>(query_plan);
      }

      read_static(bundle);
      read_plans(bundle);
      read_funcs(bundle);
      read_data(bundle);
      return std::make_shared<BundleDef>(std::move(bundle));
    }

  private:
    uint8_t read_byte()
    {
      return static_cast<uint8_t>(m_istream.get());
    }

    int8_t read_sbyte()
    {
      return static_cast<int8_t>(m_istream.get());
    }

    int32_t read_int32()
    {
      return bson::read_int32(m_istream);
    }

    int64_t read_int64()
    {
      return bson::read_int64(m_istream);
    }

    void skip_uint64()
    {
      m_istream.seekg(sizeof(uint64_t), std::ios::cur);
    }

    size_t read_size()
    {
      return bson::read_size(m_istream);
    }

    Location read_location()
    {
      int8_t loc_id = read_sbyte();
      if (loc_id == 1)
      {
        return {nullptr, 0, 0};
      }

      if (loc_id != 2)
      {
        logging::Error() << "Unexpected location id: " << loc_id;
        throw std::format_error("Unexpected location id");
      }

      size_t idx = read_size();
      size_t pos = read_size();
      size_t len = read_size();
      return Location(m_files[idx], pos, len);
    }

    std::string read_string()
    {
      size_t size = static_cast<size_t>(read_int32());
      std::string value;
      value.resize(size);
      m_istream.read(value.data(), size);
      return value;
    }

    void skip_string()
    {
      size_t size = static_cast<size_t>(read_int32());
      m_istream.seekg(size, std::ios::cur);
    }

    uint64_t position()
    {
      return static_cast<uint64_t>(m_istream.tellg());
    }

    void assert_id(int8_t expected, const char* error)
    {
      int8_t actual = bson::read_sbyte(m_istream);
      if (actual != expected)
      {
        throw std::format_error(error);
      }
    }

    void read_files(std::vector<Source>& files)
    {
      size_t size = read_size();
      for (size_t i = 0; i < size; ++i)
      {
        std::string origin = read_string();
        std::string contents = read_string();
        files.push_back(SourceDef::synthetic(contents, origin));
      }
    }

    void read_strings(std::vector<Location>& strings)
    {
      size_t size = read_size();
      for (size_t i = 0; i < size; ++i)
      {
        strings.push_back(read_string());
      }
    }

    Nodes read_bkv_list()
    {
      size_t size = read_byte();
      Nodes nodes;
      for (size_t i = 0; i < size; ++i)
      {
        nodes.push_back(
          bi::ObjectItem << read_builtin_type() << read_builtin_type());
      }

      return nodes;
    }

    Nodes read_bt_list()
    {
      Nodes nodes;
      size_t size = read_byte();
      for (size_t i = 0; i < size; ++i)
      {
        nodes.push_back(read_builtin_type());
      }

      return nodes;
    }

    Node read_builtin_type()
    {
      int8_t type_id = read_sbyte();
      switch (type_id)
      {
        case BITAny:
          return bi::Type << (bi::Any ^ "any");

        case BITNumber:
          return bi::Type << (bi::Number ^ "number");

        case BITString:
          return bi::Type << (bi::String ^ "string");

        case BITBoolean:
          return bi::Type << (bi::Boolean ^ "boolean");

        case BITNull:
          return bi::Type << (bi::Null ^ "null");

        case BITArrayDynamic:
          return bi::Type << (bi::DynamicArray << read_builtin_type());

        case BITArrayStatic: {
          return bi::Type << (bi::StaticArray << read_bt_list());
        }

        case BITObjectDynamic:
          return bi::Type
            << (bi::DynamicObject << read_builtin_type()
                                  << read_builtin_type());

        case BITObjectStatic:
          return bi::Type << (bi::StaticObject << read_bkv_list());

        case BITObjectHybrid:
          return bi::Type
            << (bi::HybridObject << (bi::DynamicObject << read_builtin_type()
                                                       << read_builtin_type())
                                 << (bi::StaticObject << read_bkv_list()));

        case BITSet:
          return bi::Type << (bi::Set << read_builtin_type());

        case BITTypeSeq:
          return bi::Type << (bi::TypeSeq << read_bt_list());

        default:
          logging::Error() << "Unrecognized built-in type id: " << type_id;
          throw std::format_error("Unrecognized built-in type id");
      }
    }

    Node read_builtin_arg()
    {
      Node name = bi::Name ^ read_string();
      Node desc = bi::Description ^ read_string();
      return bi::Arg << name << desc << read_builtin_type();
    }

    Node read_builtin_decl()
    {
      Node args = NodeDef::create(bi::VarArgs);
      int8_t args_id = read_sbyte();
      if (args_id == 2)
      {
        args = NodeDef::create(bi::ArgSeq);
        uint8_t num_args = read_byte();
        for (size_t i = 0; i < num_args; ++i)
        {
          args << read_builtin_arg();
        }
      }

      Node ret = NodeDef::create(bi::Void);
      int8_t ret_id = read_sbyte();
      if (ret_id == 2)
      {
        Node name = bi::Name ^ read_string();
        Node desc = bi::Description ^ read_string();
        ret = bi::Result << name << desc << read_builtin_type();
      }

      return bi::Decl << args << ret;
    }

    void read_builtin_funcs(std::map<Location, Node>& builtin_funcs)
    {
      size_t size = read_size();
      for (size_t i = 0; i < size; ++i)
      {
        std::string name = read_string();
        builtin_funcs[name] = read_builtin_decl();
      }
    }

    void read_static(BundleDef& bundle)
    {
      assert_id(StaticId, "Static ID byte missing");
      read_files(bundle.files);
      read_strings(bundle.strings);
      read_builtin_funcs(bundle.builtin_functions);
      int8_t query_id = read_sbyte();
      if (query_id == 2)
      {
        bundle.query = SourceDef::synthetic(read_string(), "query");
      }
    }

    void skip_table()
    {
      skip_uint64();
      size_t size = read_size();
      for (size_t i = 0; i < size; ++i)
      {
        skip_string();
        skip_uint64();
      }
    }

    b::Operand read_operand(b::Operand& op)
    {
      op.type = static_cast<b::OperandType>(read_sbyte());
      switch (op.type)
      {
        case b::OperandType::Local:
        case b::OperandType::String:
          op.index = read_size();
          return op;

        case b::OperandType::True:
        case b::OperandType::False:
          return op;

        default:
          throw std::runtime_error("Invalid statement operand type");
      }
    }

    void read_operand_array(std::vector<b::Operand>& operands)
    {
      size_t size = read_byte();
      for (size_t i = 0; i < size; ++i)
      {
        b::Operand op;
        read_operand(op);
        operands.push_back(op);
      }
    }

    void read_ipath(std::vector<size_t>& path)
    {
      size_t size = read_byte();
      for (size_t i = 0; i < size; ++i)
      {
        path.push_back(static_cast<size_t>(read_int32()));
      }
    }

    b::Statement read_statement()
    {
      b::Statement statement;
      statement.type = static_cast<b::StatementType>(read_sbyte());
      statement.location = read_location();

      switch (statement.type)
      {
        case b::StatementType::ArrayAppend:
          statement.target = read_size();
          read_operand(statement.op0);
          break;

        case b::StatementType::AssignInt:
          statement.op0.value = read_int64();
          statement.target = read_size();
          break;

        case b::StatementType::AssignVarOnce:
        case b::StatementType::AssignVar:
          read_operand(statement.op0);
          statement.target = read_size();
          break;

        case b::StatementType::Block: {
          std::vector<b::Block> blocks;
          read_blocks(blocks);
          statement.ext = std::make_shared<b::StatementExt>(std::move(blocks));
        }
        break;

        case b::StatementType::Break:
          statement.op0.index = read_size();
          break;

        case b::StatementType::Call: {
          b::CallExt call;
          call.func = read_string();
          read_operand_array(call.ops);
          statement.ext = std::make_shared<b::StatementExt>(std::move(call));
          statement.target = read_size();
        }
        break;

        case b::StatementType::CallDynamic: {
          b::CallDynamicExt call;
          read_operand_array(call.path);
          read_operand_array(call.ops);
          statement.ext = std::make_shared<b::StatementExt>(std::move(call));
          statement.target = read_size();
        }
        break;

        case b::StatementType::Dot:
          read_operand(statement.op0);
          read_operand(statement.op1);
          statement.target = read_size();
          break;

        case b::StatementType::Equal:
          read_operand(statement.op0);
          read_operand(statement.op1);
          break;

        case b::StatementType::IsArray:
          read_operand(statement.op0);
          break;

        case b::StatementType::IsDefined:
          statement.target = read_size();
          break;

        case b::StatementType::IsObject:
          read_operand(statement.op0);
          break;

        case b::StatementType::IsSet:
          read_operand(statement.op0);
          break;

        case b::StatementType::IsUndefined:
          statement.target = read_size();
          break;

        case b::StatementType::Len:
          read_operand(statement.op0);
          statement.target = read_size();
          break;

        case b::StatementType::MakeArray:
          statement.op0.value = read_int32();
          statement.target = read_size();
          break;

        case b::StatementType::MakeNull:
          statement.target = read_size();
          break;

        case b::StatementType::MakeNumberInt:
          statement.op0.value = read_int64();
          statement.target = read_size();
          break;

        case b::StatementType::MakeNumberRef:
          statement.op0.index = read_size();
          statement.target = read_size();
          break;

        case b::StatementType::MakeObject:
          statement.target = read_size();
          break;

        case b::StatementType::MakeSet:
          statement.target = read_size();
          break;

        case b::StatementType::NotEqual:
          read_operand(statement.op0);
          read_operand(statement.op1);
          break;

        case b::StatementType::Not: {
          b::Block block;
          read_block(block);
          statement.ext = std::make_shared<b::StatementExt>(std::move(block));
        }
        break;

        case b::StatementType::ObjectInsertOnce:
        case b::StatementType::ObjectInsert:
          read_operand(statement.op0);
          read_operand(statement.op1);
          statement.target = read_size();
          break;

        case b::StatementType::ObjectMerge:
          statement.op0.index = read_size();
          statement.op1.index = read_size();
          statement.target = read_size();
          break;

        case b::StatementType::ResetLocal:
        case b::StatementType::ResultSetAdd:
        case b::StatementType::ReturnLocal:
          statement.target = read_size();
          break;

        case b::StatementType::Scan:
          statement.target = read_size();
          statement.op0.index = read_size();
          statement.op1.index = read_size();
          {
            b::Block block;
            read_block(block);
            statement.ext = std::make_shared<b::StatementExt>(std::move(block));
          }
          break;

        case b::StatementType::SetAdd:
          read_operand(statement.op0);
          statement.target = read_size();
          break;

        case b::StatementType::With:
          statement.target = read_size();
          read_operand(statement.op0);
          {
            b::WithExt with;
            read_ipath(with.path);
            read_block(with.block);
            statement.ext = std::make_shared<b::StatementExt>(std::move(with));
          }
          break;

        case b::StatementType::Nop:
          logging::Info() << "Stripping NopStmt during serialization";
          break;
      }

      return statement;
    }

    void read_block(b::Block& block)
    {
      size_t size = read_size();
      for (size_t i = 0; i < size; ++i)
      {
        block.push_back(read_statement());
      }
    }

    void read_blocks(std::vector<b::Block>& blocks)
    {
      size_t size = read_size();
      blocks.resize(size);
      for (size_t i = 0; i < size; ++i)
      {
        read_block(blocks[i]);
      }
    }

    b::Plan read_plan()
    {
      b::Plan plan;
      plan.name = read_string();
      read_blocks(plan.blocks);
      return plan;
    }

    void read_plans(BundleDef& bundle)
    {
      assert_id(PlansId, "Plans ID byte missing");
      size_t num_plans = read_size();
      for (size_t i = 0; i < num_plans; ++i)
      {
        b::Plan plan = read_plan();
        bundle.name_to_plan[plan.name] = i;
        bundle.plans.push_back(plan);
      }
      skip_table();
    }

    void read_path(std::vector<Location>& path)
    {
      size_t size = read_byte();
      for (size_t i = 0; i < size; ++i)
      {
        path.push_back(read_string());
      }
    }

    void read_params(std::vector<size_t>& params)
    {
      size_t size = read_byte();
      for (size_t i = 0; i < size; ++i)
      {
        params.push_back(read_size());
      }
    }

    b::Function read_func()
    {
      b::Function func;
      func.name = read_string();
      read_path(func.path);
      read_params(func.parameters);
      func.result = read_size();
      read_blocks(func.blocks);
      func.arity = func.parameters.size();
      func.cacheable = func.arity == 2;
      return func;
    }

    void read_funcs(BundleDef& bundle)
    {
      assert_id(FuncsId, "Funcs ID byte missing");
      size_t num_funcs = read_size();
      for (size_t i = 0; i < num_funcs; ++i)
      {
        b::Function func = read_func();
        bundle.name_to_func[func.name] = i;
        bundle.functions.push_back(func);
      }
      skip_table();
    }

    void read_data(BundleDef& bundle)
    {
      assert_id(DataId, "Data ID missing");
      bundle.data = bson::read_object(m_istream);
    }

    std::istringstream m_istream;
    std::vector<Source> m_files;
  };

}

namespace rego
{
  void BundleDef::save(std::ostream& ostream) const
  {
    oregostream stream;
    stream.write_bundle(*this);
    std::string_view view = stream.view();
    ostream.write(view.data(), view.size());
  }

  Bundle BundleDef::load(std::istream& istream)
  {
    char magic[9];
    istream.read(magic, strlen(Magic));
    magic[8] = 0;
    if (strcmp(magic, Magic))
    {
      logging::Error() << "Mismatched header: " << magic;
      throw std::invalid_argument("Mismatched header");
    }

    char version = istream.get();
    if (version != RegoVersion)
    {
      logging::Error() << "Unsupported rego version: " << version << "Only "
                       << RegoVersion << " is supported.";
      throw std::invalid_argument("Unsupported rego version");
    }

    version = istream.get();
    if (version != RegoBinaryVersion)
    {
      logging::Error() << "Unsupported rego binary version: " << version
                       << "Only " << RegoBinaryVersion << " is supported.";
      throw std::invalid_argument("Unsupported rego binary version");
    }

    int8_t query_plan = static_cast<int8_t>(istream.get());

    istream.seekg(NumReservedBytes, std::ios::cur); // reserved
    uint32_t local_count = bson::read_uint32(istream);
    uint32_t expected_crc32 = bson::read_uint32(istream);
    uint64_t size = bson::read_uint64(istream);
    istream.seekg(HeaderSize, std::ios::beg); // seek to the end of the header

    std::string bytes;
    bytes.resize(size);
    istream.read(bytes.data(), size);

    uint32_t actual_crc32 = crc32_add(
      CRC32Init, reinterpret_cast<const uint8_t*>(bytes.data()), size);
    if (actual_crc32 != expected_crc32)
    {
      logging::Error() << "Mismatched CRC: " << actual_crc32
                       << " != " << expected_crc32;
      throw std::invalid_argument("Mismatched CRC");
    }

    iregostream stream(std::move(bytes));
    return stream.read_bundle(local_count, query_plan);
  }
}