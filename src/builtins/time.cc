#include "builtins.h"
#include "re2/stringpiece.h"
#include "rego.hh"

#include <chrono>

#if __cpp_lib_chrono >= 201907L
#include <inttypes.h>
#endif

namespace
{
  using namespace rego;
  using namespace std::chrono;
  namespace bi = rego::builtins;

  const std::size_t second_ns = 1000000000UL;
  const std::size_t minute_ns = 60UL * second_ns;
  const std::size_t hour_ns = 60UL * minute_ns;

  Node now_ns_decl = bi::Decl
    << bi::ArgSeq
    << (bi::Result << (bi::Name ^ "now")
                   << (bi::Description ^ "nanoseconds since epoch")
                   << (bi::Type << bi::Number));

  static thread_local Node cached = nullptr;
  struct NowNS : public BuiltInDef
  {
    NowNS() :
      BuiltInDef(
        Location("time.now_ns"),
        now_ns_decl,
        [this](const Nodes&) { return call(); },
        true)
    {}

    Node call()
    {
      if (cached != nullptr)
      {
        return cached->clone();
      }

      auto now = system_clock::now().time_since_epoch();
      auto now_ns = duration_cast<nanoseconds>(now).count();
      cached = Int ^ std::to_string(now_ns);
      return cached;
    }

    void clear() override
    {
      cached = nullptr;
    }
  };

  const std::map<std::string, double> duration_units = {
    {"ns", 1},
    {"us", 1000},
    {"µs", 1000},
    {"ms", 1000000},
    {"s", double(second_ns)},
    {"m", double(minute_ns)},
    {"h", double(hour_ns)}};

  nanoseconds do_parse_duration(const std::string& duration)
  {
    const char* duration_re =
      R"((-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][-+]?[0-9]+)?)((?:ns|us|µs|ms|s|m|h)))";
    const RE2 re(duration_re);
    assert(re.ok());

    std::string number;
    std::string unit;
    std::int64_t ns = 0;
    std::size_t start = 0;
    while (start < duration.size())
    {
      re2::StringPiece input(duration.c_str() + start, duration.size() - start);
      if (RE2::PartialMatch(input, re, &number, &unit))
      {
        double number_d = std::stod(number);
        double unit_ns = duration_units.at(unit);
        ns += std::int64_t(number_d * unit_ns);
        start += number.size() + unit.size();
      }
      else
      {
        break;
      }
    }

    return nanoseconds(ns);
  }

  Node parse_duration_ns(const Nodes& args)
  {
    Node duration = unwrap_arg(
      args, UnwrapOpt(0).type(JSONString).func("time.parse_duration_ns"));
    if (duration->type() == Error)
    {
      return duration;
    }

    std::int64_t ns = do_parse_duration(get_string(duration)).count();

    if (ns != 0)
    {
      return Int ^ std::to_string(ns);
    }

    return Undefined;
  }

  Node parse_duration_ns_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "duration")
                     << (bi::Description ^
                         "a duration like \"3m\"; see the [Go `time` package "
                         "documentation](https://golang.org/pkg/time/"
                         "#ParseDuration) for more details")
                     << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "ns")
                 << (bi::Description ^ "the `duration` in nanoseconds")
                 << (bi::Type << bi::Number));

  Node add_date_decl = bi::Decl
    << (bi::ArgSeq << (bi::Arg
                       << (bi::Name ^ "ns")
                       << (bi::Description ^ "nanoseconds since the epoch")
                       << (bi::Type << bi::Number))
                   << (bi::Arg << (bi::Name ^ "years")
                               << (bi::Description ^ "number of years to add")
                               << (bi::Type << bi::Number))
                   << (bi::Arg << (bi::Name ^ "months")
                               << (bi::Description ^ "number of months to add")
                               << (bi::Type << bi::Number))
                   << (bi::Arg << (bi::Name ^ "days")
                               << (bi::Description ^ "number of days to add")
                               << (bi::Type << bi::Number)))
    << (bi::Result << (bi::Name ^ "output")
                   << (bi::Description ^
                       "nanoseconds since the epoch representing the input "
                       "time, with years, months and days added")
                   << (bi::Type << bi::Number));

  Node clock_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "x")
                     << (bi::Description ^
                         "a number representing the nanoseconds since the "
                         "epoch (UTC); or a two-element array of the "
                         "nanoseconds, and a timezone string")
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type << bi::Number)
                             << (bi::Type
                                 << (bi::StaticArray
                                     << (bi::Type << bi::Number)
                                     << (bi::Type << bi::String)))))))
             << (bi::Result
                 << (bi::Name ^ "output")
                 << (bi::Description ^
                     "the `hour`, `minute` (0-59), and `second` (0-59) "
                     "representing the time of day for the nanoseconds since "
                     "epoch in the supplied timezone (or UTC)")
                 << (bi::Type
                     << (bi::StaticArray << (bi::Type << bi::Number)
                                         << (bi::Type << bi::Number)
                                         << (bi::Type << bi::Number))));

  Node date_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "x")
                     << (bi::Description ^
                         "a number representing the nanoseconds since the "
                         "epoch (UTC); or a two-element array of the "
                         "nanoseconds, and a timezone string")
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type << bi::Number)
                             << (bi::Type
                                 << (bi::StaticArray
                                     << (bi::Type << bi::Number)
                                     << (bi::Type << bi::String)))))))
             << (bi::Result
                 << (bi::Name ^ "output")
                 << (bi::Description ^
                     "an array of `year`, `month` (1-12), and `day` (1-31))")
                 << (bi::Type
                     << (bi::StaticArray << (bi::Type << bi::Number)
                                         << (bi::Type << bi::Number)
                                         << (bi::Type << bi::Number))));

  Node diff_decl = bi::Decl
    << (bi::ArgSeq
        << (bi::Arg << (bi::Name ^ "ns1")
                    << (bi::Description ^
                        "nanoseconds since the epoch; or a two-element array "
                        "of the nanoseconds, and a timezone string")
                    << (bi::Type
                        << (bi::TypeSeq << (bi::Type << bi::Number)
                                        << (bi::Type
                                            << (bi::StaticArray
                                                << (bi::Type << bi::Number)
                                                << (bi::Type << bi::String))))))
        << (bi::Arg << (bi::Name ^ "ns2")
                    << (bi::Description ^
                        "nanoseconds since the epoch; or a two-element array "
                        "of the nanoseconds, and a timezone string")
                    << (bi::Type
                        << (bi::TypeSeq
                            << (bi::Type << bi::Number)
                            << (bi::Type
                                << (bi::StaticArray
                                    << (bi::Type << bi::Number)
                                    << (bi::Type << bi::String)))))))
    << (bi::Result << (bi::Name ^ "output")
                   << (bi::Description ^
                       "difference between `ns1` and `ns2` (in their supplied "
                       "timezones, if supplied, or UTC) as array of numbers: "
                       "`[years, months, days, hours, minutes, seconds]`")
                   << (bi::Type
                       << (bi::StaticArray << (bi::Type << bi::Number)
                                           << (bi::Type << bi::Number)
                                           << (bi::Type << bi::Number)
                                           << (bi::Type << bi::Number)
                                           << (bi::Type << bi::Number)
                                           << (bi::Type << bi::Number))));

  Node format_decl = bi::Decl
    << (bi::ArgSeq
        << (bi::Arg
            << (bi::Name ^ "x")
            << (bi::Description ^
                "a number representing the nanoseconds since the epoch (UTC); "
                "or a two-element array of the nanoseconds, and a timezone "
                "string; or a three-element array of ns, timezone string and a "
                "layout string or golang defined formatting constant (see "
                "golang supported time formats)")
            << (bi::Type
                << (bi::TypeSeq
                    << (bi::Type << bi::Number)
                    << (bi::Type
                        << (bi::StaticArray << (bi::Type << bi::Number)
                                            << (bi::Type << bi::String)))
                    << (bi::Type
                        << (bi::StaticArray << (bi::Type << bi::Number)
                                            << (bi::Type << bi::String)
                                            << (bi::Type << bi::String)))))))
    << (bi::Result
        << (bi::Name ^ "formatted timestamp")
        << (bi::Description ^
            "the formatted timestamp represented for the nanoseconds since the "
            "epoch in the supplied timezone (or UTC)")
        << (bi::Type << bi::String));

  Node parse_ns_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "layout")
                     << (bi::Description ^
                         "format used for parsing, see the [Go `time` package "
                         "documentation](https://golang.org/pkg/time/#Parse) "
                         "for more details")
                     << (bi::Type << bi::String))
                 << (bi::Arg << (bi::Name ^ "value")
                             << (bi::Description ^
                                 "input to parse according to `layout`")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "ns")
                 << (bi::Description ^ "`value` in nanoseconds since epoch")
                 << (bi::Type << bi::Number));

  Node parse_rfc3339_ns_decl =
    bi::Decl << (bi::ArgSeq

                 << (bi::Arg << (bi::Name ^ "value")
                             << (bi::Description ^
                                 "input string to parse in RFC3339 format")
                             << (bi::Type << bi::String)))
             << (bi::Result
                 << (bi::Name ^ "ns")
                 << (bi::Description ^ "`value` in nanoseconds since epoch")
                 << (bi::Type << bi::Number));

  Node weekday_decl =
    bi::Decl << (bi::ArgSeq
                 << (bi::Arg
                     << (bi::Name ^ "x")
                     << (bi::Description ^
                         "a number representing the nanoseconds since the "
                         "epoch (UTC); or a two-element array of the "
                         "nanoseconds, and a timezone string")
                     << (bi::Type
                         << (bi::TypeSeq
                             << (bi::Type << bi::Number)
                             << (bi::Type
                                 << (bi::StaticArray
                                     << (bi::Type << bi::Number)
                                     << (bi::Type << bi::String)))))))
             << (bi::Result
                 << (bi::Name ^ "day")
                 << (bi::Description ^
                     "the weekday represented by `ns` nanoseconds since the "
                     "epoch in the supplied timezone (or UTC)")
                 << (bi::Type << bi::String));

#if __cpp_lib_chrono >= 201907L
  const std::size_t milli_ns = 1000000UL;
  const std::size_t day_ns = 24UL * hour_ns;

  const std::map<std::string, std::string> predefined_layouts = {
    {"Layout",
     "01/02 03:04:05PM '06 -0700"}, // The reference time, in numerical order.
    {"ANSIC", "Mon Jan _2 15:04:05 2006"},
    {"UnixDate", "Mon Jan _2 15:04:05 MST 2006"},
    {"RubyDate", "Mon Jan 02 15:04:05 -0700 2006"},
    {"RFC822", "02 Jan 06 15:04 MST"},
    {"RFC822Z", "02 Jan 06 15:04 -0700"}, // RFC822 with numeric zone
    {"RFC850", "Monday, 02-Jan-06 15:04:05 MST"},
    {"RFC1123", "Mon, 02 Jan 2006 15:04:05 MST"},
    {"RFC1123Z",
     "Mon, 02 Jan 2006 15:04:05 -0700"}, // RFC1123 with numeric zone
    {"RFC3339", "2006-01-02T15:04:05Z07:00"},
    {"RFC3339Nano", "2006-01-02T15:04:05.999999999Z07:00"},
    {"Kitchen", "3:04PM"},
    // Handy time stamps.
    {"Stamp", "Jan _2 15:04:05"},
    {"StampMilli", "Jan _2 15:04:05.000"},
    {"StampMicro", "Jan _2 15:04:05.000000"},
    {"StampNano", "Jan _2 15:04:05.000000000"},
  };

  const std::map<std::string, std::string> go_to_cpp = {
    {"2006", "%Y"},    {"06", "%y"},        {"Jan", "%b"},
    {"January", "%B"}, {"01", "%m"},        {"1", "%m"},
    {"Mon", "%a"},     {"Monday", "%A"},    {"2", "%d"},
    {"_2", "%d"},      {"02", "%d"},        {"__2", "%j"},
    {"002", "%j"},     {"15", "%H"},        {"3", "%I"},
    {"03", "%I"},      {"4", "%M"},         {"04", "%M"},
    {"5", "%S"},       {"05", "%S"},        {"PM", "%p"},
    {"PM", "%p"},      {"-0700", "%z"},     {"-07:00", "%Ez"},
    {"-07", "%z"},     {"-070000", "%z"},   {"-07:00:00", "%Ez"},
    {"Z0700", "%Z"},   {"Z07:00", "%Ez"},   {"Z07", "%Z"},
    {"Z070000", "%Z"}, {"Z07:00:00", "%Ez"}};

  enum class Resolution
  {
    Seconds,
    Nano,
    NanoZero,
    Micro,
    MicroZero,
    Milli,
    MilliZero
  };

  enum class InfoMode
  {
    Time,
    TimeZone,
    TimeFormat,
    TimeZoneFormat
  };

  const std::map<std::string, Resolution> res_lookup = {
    {".000000000", Resolution::Nano},
    {".000000", Resolution::Micro},
    {".000", Resolution::Milli},
    {".999999999", Resolution::NanoZero},
    {".999999", Resolution::MicroZero},
    {".999", Resolution::MilliZero},
    {"", Resolution::Seconds}};

  struct TimeInfo
  {
    InfoMode mode;
    nanoseconds ns;
    std::string time_zone;
    std::string format;
    Resolution resolution;
    size_t fraction_start;
    Node error;
  };

  Node add_date(const Nodes& args)
  {
    Node ns_node =
      unwrap_arg(args, UnwrapOpt(0).type(Int).func("time.add_date"));
    if (ns_node->type() == Error)
    {
      return ns_node;
    }

    Node years_node =
      unwrap_arg(args, UnwrapOpt(1).type(Int).func("time.add_date"));
    if (years_node->type() == Error)
    {
      return years_node;
    }

    Node months_node =
      unwrap_arg(args, UnwrapOpt(2).type(Int).func("time.add_date"));

    if (months_node->type() == Error)
    {
      return months_node;
    }

    Node days_node =
      unwrap_arg(args, UnwrapOpt(3).type(Int).func("time.add_date"));
    if (days_node->type() == Error)
    {
      return days_node;
    }

    auto maybe_ns = get_int(ns_node).to_int();
    if (!maybe_ns.has_value())
    {
      return err(
        ns_node,
        "time.add_date: first operand is not a valid integer",
        EvalTypeError);
    }
    nanoseconds ns(maybe_ns.value());
    days days_since_epoch = floor<days>(ns);
    year_month_day ymd{sys_days(days_since_epoch)};
    auto maybe_years = get_int(years_node).to_int();
    if (!maybe_years.has_value())
    {
      return err(
        years_node,
        "time.add_date: second operand is not a valid integer",
        EvalTypeError);
    }
    auto maybe_months = get_int(months_node).to_int();
    if (!maybe_months.has_value())
    {
      return err(
        months_node,
        "time.add_date: third operand is not a valid integer",
        EvalTypeError);
    }
    auto maybe_days = get_int(days_node).to_int();
    if (!maybe_days.has_value())
    {
      return err(
        days_node,
        "time.add_date: fourth operand is not a valid integer",
        EvalTypeError);
    }

    years add_y = years(static_cast<int>(maybe_years.value()));
    months add_m = months(static_cast<int>(maybe_months.value()));
    ymd += add_y;
    ymd += add_m;
    std::int64_t days = sys_days(ymd).time_since_epoch().count();
    days += maybe_days.value();

    ns -= days_since_epoch;
    BigInt time = days * BigInt(day_ns) + ns.count();

    if (
      time < std::numeric_limits<std::int64_t>::min() ||
      time > std::numeric_limits<std::int64_t>::max())
    {
      return err(ns_node, "time outside of valid range", EvalBuiltInError);
    }

    return Resolver::term(time);
  }

  std::optional<nanoseconds> get_timestamp(Node x)
  {
    BigInt x_int = get_int(x);
    if (x_int > std::numeric_limits<nanoseconds::rep>::max())
    {
      return std::nullopt;
    }

    auto maybe_ns = x_int.to_int();
    if (!maybe_ns.has_value())
    {
      return std::nullopt;
    }

    return nanoseconds(maybe_ns.value());
  }

  std::pair<std::string, std::pair<Resolution, size_t>> get_format(
    const std::string& go_format_or_layout)
  {
    std::string go_format = go_format_or_layout;
    if (predefined_layouts.contains(go_format))
    {
      go_format = predefined_layouts.at(go_format);
    }

    Resolution res = Resolution::Seconds;
    size_t res_i = std::string::npos;
    std::stringstream cpp_format;
    size_t i = 0;
    while (i < go_format.size())
    {
      bool found = false;
      size_t max_len = std::min<size_t>(12, go_format.size() - i);
      for (size_t len = max_len; len > 0; --len)
      {
        auto query = go_format.substr(i, len);
        auto maybe_res = res_lookup.find(query);
        if (maybe_res != res_lookup.end())
        {
          res_i = i;
          res = maybe_res->second;
          i += len;
          found = true;
          break;
        }

        auto maybe_cpp = go_to_cpp.find(query);
        if (maybe_cpp != go_to_cpp.end())
        {
          cpp_format << maybe_cpp->second;
          i += len;
          found = true;
          break;
        }
      }
      if (!found)
      {
        cpp_format << go_format[i];
        i++;
      }
    }

    return {cpp_format.str(), {res, res_i}};
  }

  TimeInfo process_arg(
    const Nodes& args, const std::string& func, std::size_t index)
  {
    Node x = unwrap_arg(args, UnwrapOpt(index).types({Int, Array}).func(func));
    TimeInfo info = {
      InfoMode::Time,
      nanoseconds::zero(),
      "",
      "",
      Resolution::Seconds,
      std::string::npos,
      nullptr};

    if (x->type() == Error)
    {
      info.error = x;
      return info;
    }

    if (x->type() == Int)
    {
      auto maybe_ns = get_timestamp(x);
      if (!maybe_ns.has_value())
      {
        info.error = err(x, "timestamp too big", EvalBuiltInError);
        return info;
      }

      info.ns = maybe_ns.value();
      return info;
    }

    if (x->size() < 2 || x->size() > 3)
    {
      info.error = err(x, "time.format: invalid argument", EvalTypeError);
      return info;
    }

    auto maybe_int = unwrap(x->at(0), Int);
    if (!maybe_int.success)
    {
      info.error =
        err(x->at(0), "time.format: invalid timestamp", EvalTypeError);
      return info;
    }

    auto maybe_ns = get_timestamp(maybe_int.node);
    if (!maybe_ns.has_value())
    {
      info.error = err(maybe_int.node, "timestamp too big", EvalBuiltInError);
      return info;
    }

    info.ns = maybe_ns.value();

    auto maybe_timezone = unwrap(x->at(1), JSONString);
    if (!maybe_timezone.success)
    {
      info.error =
        err(x->at(1), "time.format: invalid time zone", EvalTypeError);
      return info;
    }

    info.time_zone = get_string(maybe_timezone.node);

    if (x->size() == 2)
    {
      if (!info.time_zone.empty())
      {
        info.mode = InfoMode::TimeZone;
      }

      return info;
    }

    auto maybe_format = unwrap(x->at(2), JSONString);
    if (!maybe_format.success)
    {
      info.error =
        err(x->at(2), "time.format: invalid format string", EvalTypeError);
      return info;
    }

    auto [format, res] = get_format(get_string(maybe_format.node));
    info.format = format;
    info.resolution = res.first;
    info.fraction_start = res.second;
    if (info.time_zone.empty())
    {
      info.mode = InfoMode::TimeFormat;
    }
    else
    {
      info.mode = InfoMode::TimeZoneFormat;
    }

    return info;
  }

  Node time_string(const TimeInfo& info)
  {
    time_point<system_clock, nanoseconds> tp(info.ns);
    std::string time_s = std::format("{0:%FT%TZ}", tp);
    return JSONString ^ time_s;
  }

  Node timezone_string(const TimeInfo& info)
  {
    auto zone = locate_zone(info.time_zone);
    zoned_time<nanoseconds> zt_ns(zone, sys_time<nanoseconds>(info.ns));
    auto time_s = std::format("{0:%FT%T%Ez}", zt_ns);
    return JSONString ^ time_s;
  }

  std::string insert_fraction(const std::string& time_s, const TimeInfo& info)
  {
    const int fraction_len = 32;
    char fraction[fraction_len];
    switch (info.resolution)
    {
      case Resolution::Nano:
        snprintf(fraction, fraction_len, ".%09" PRId64, info.ns.count());
        break;

      case Resolution::NanoZero:
        snprintf(fraction, fraction_len, ".%09" PRId64, info.ns.count());
        break;

      case Resolution::Micro:
        snprintf(fraction, fraction_len, ".%06" PRId64, info.ns.count() / 1000);
        break;

      case Resolution::MicroZero:
        snprintf(fraction, fraction_len, ".%06" PRId64, info.ns.count() / 1000);
        break;

      case Resolution::Milli:
        snprintf(
          fraction, fraction_len, ".%03" PRId64, info.ns.count() / 1000000);
        break;

      case Resolution::MilliZero:
        snprintf(
          fraction, fraction_len, ".%03" PRId64, info.ns.count() / 1000000);
        break;

      default:
        // do nothing
        break;
    }

    return time_s.substr(0, info.fraction_start) + fraction +
      time_s.substr(info.fraction_start);
  }

  Node timeformat_string(const TimeInfo& info)
  {
    time_point<system_clock, nanoseconds> tp(info.ns);
    std::string fmt = std::format("{{0:{0}}}", info.format);
    auto time_s = std::vformat(fmt, std::make_format_args(tp));
    if (info.resolution != Resolution::Seconds)
    {
      time_s = insert_fraction(time_s, info);
    }

    return JSONString ^ time_s;
  }

  Node timezoneformat_string(const TimeInfo& info)
  {
    auto zone = locate_zone(info.time_zone);
    seconds s(floor<seconds>(info.ns));
    zoned_time<seconds> zt(zone, sys_time<seconds>(s));
    std::string fmt = std::format("{{0:{0}}}", info.format);
    auto time_s = std::vformat(fmt, std::make_format_args(zt));

    if (info.resolution != Resolution::Seconds)
    {
      time_s = insert_fraction(time_s, info);
    }

    return JSONString ^ time_s;
  }

  Node info_string(const TimeInfo& info)
  {
    switch (info.mode)
    {
      case InfoMode::Time:
        return time_string(info);
      case InfoMode::TimeZone:
        return timezone_string(info);
      case InfoMode::TimeFormat:
        return timeformat_string(info);
      case InfoMode::TimeZoneFormat:
        return timezoneformat_string(info);
      default:
        throw std::runtime_error("Unsupported TimeInfo mode");
    }
  }

  Node clock_(const Nodes& args)
  {
    TimeInfo info = process_arg(args, "time.clock", 0);
    if (info.error != nullptr)
    {
      return info.error;
    }

    hh_mm_ss<nanoseconds> time;
    if (info.mode == InfoMode::Time)
    {
      auto tp = sys_time<nanoseconds>(info.ns);
      auto dp = floor<days>(tp);
      time = hh_mm_ss<nanoseconds>(tp - dp);
    }
    else
    {
      auto zone = locate_zone(info.time_zone);
      zoned_time<nanoseconds> zt_ns(zone, sys_time<nanoseconds>(info.ns));
      auto tp = zt_ns.get_local_time();
      auto dp = floor<days>(tp);
      time = hh_mm_ss<nanoseconds>(tp - dp);
    }

    std::int64_t hours_int = time.hours().count();
    std::int64_t minutes_int = time.minutes().count();
    std::int64_t seconds_int = time.seconds().count();
    return Array << Resolver::term(BigInt(hours_int))
                 << Resolver::term(BigInt(minutes_int))
                 << Resolver::term(BigInt(seconds_int));
  }

  Node date_(const Nodes& args)
  {
    TimeInfo info = process_arg(args, "time.date", 0);
    if (info.error != nullptr)
    {
      return info.error;
    }

    year_month_day ymd;
    if (info.mode == InfoMode::Time)
    {
      time_point<system_clock, nanoseconds> tp(info.ns);
      ymd = {floor<days>(tp)};
    }
    else
    {
      auto zone = locate_zone(info.time_zone);
      zoned_time<nanoseconds> zt_ns(zone, sys_time<nanoseconds>(info.ns));
      auto ld = floor<days>(zt_ns.get_local_time());
      ymd = year_month_day(ld);
    }

    std::int64_t year = int(ymd.year());
    std::int64_t month = unsigned(ymd.month());
    std::int64_t day = unsigned(ymd.day());
    return Array << Resolver::term(BigInt(year))
                 << Resolver::term(BigInt(month))
                 << Resolver::term(BigInt(day));
  }

  Node diff(const Nodes& args)
  {
    TimeInfo info1 = process_arg(args, "time.diff", 0);
    if (info1.error != nullptr)
    {
      return info1.error;
    }

    TimeInfo info2 = process_arg(args, "time.diff", 1);
    if (info2.error != nullptr)
    {
      return info2.error;
    }

    sys_time<nanoseconds> tp1;
    sys_time<nanoseconds> tp2;

    if (info1.mode == InfoMode::Time)
    {
      tp1 = sys_time<nanoseconds>(info1.ns);
    }
    else
    {
      auto zone = locate_zone(info1.time_zone);
      zoned_time<nanoseconds> zt_ns(zone, sys_time<nanoseconds>(info1.ns));
      tp1 = zt_ns.get_sys_time();
    }

    if (info2.mode == InfoMode::Time)
    {
      tp2 = sys_time<nanoseconds>(info2.ns);
    }
    else
    {
      auto zone = locate_zone(info2.time_zone);
      zoned_time<nanoseconds> zt_ns(zone, sys_time<nanoseconds>(info2.ns));
      tp2 = zt_ns.get_sys_time();
    }

    seconds diff_s = duration_cast<seconds>(tp1 > tp2 ? tp1 - tp2 : tp2 - tp1);
    years diff_years = floor<years>(diff_s);
    diff_s -= seconds(diff_years.count() * 365 * 24 * 60 * 60);
    months diff_months = floor<months>(diff_s);
    diff_s -= diff_months;
    days diff_days = floor<days>(diff_s);
    diff_s -= diff_days;
    hours diff_hours = floor<hours>(diff_s);
    diff_s -= diff_hours;
    minutes diff_minutes = floor<minutes>(diff_s);
    diff_s -= diff_minutes;

    std::int64_t years_int = diff_years.count();
    std::int64_t months_int = diff_months.count();
    std::int64_t days_int = diff_days.count();
    std::int64_t hours_int = diff_hours.count();
    std::int64_t minutes_int = diff_minutes.count();
    std::int64_t seconds_int = diff_s.count();
    return Array << Resolver::term(BigInt(years_int))
                 << Resolver::term(BigInt(months_int))
                 << Resolver::term(BigInt(days_int))
                 << Resolver::term(BigInt(hours_int))
                 << Resolver::term(BigInt(minutes_int))
                 << Resolver::term(BigInt(seconds_int));
  }

  Node format(const Nodes& args)
  {
    TimeInfo info = process_arg(args, "time.format", 0);
    if (info.error != nullptr)
    {
      return info.error;
    }

    return info_string(info);
  }

  Node do_parse(const std::string& layout, const Node& value)
  {
    std::string value_str = get_string(value);
    auto [format, res] = get_format(layout);
    std::string parse_fmt = format;
    auto start = parse_fmt.find("%S");
    if (start != std::string::npos)
    {
      switch (res.first)
      {
        case Resolution::Nano:
        case Resolution::NanoZero:
          parse_fmt.replace(start, 2, "%12S");
          break;

        case Resolution::Micro:
        case Resolution::MicroZero:
          parse_fmt.replace(start, 2, "%9S");
          break;

        case Resolution::Milli:
        case Resolution::MilliZero:
          parse_fmt.replace(start, 2, "%6S");
          break;

        default:
          break;
      }
    }

    std::istringstream is(value_str);
    sys_time<nanoseconds> time_ns;
    is >> parse(parse_fmt, time_ns);

    if (is.fail())
    {
      return err(value, "Unable to parse time", EvalBuiltInError);
    }

    BigInt ns = time_ns.time_since_epoch().count();

    // BEGIN workaround
    // Integer overflow bug in std::chrono::parse()
    sys_time<milliseconds> time_ms;
    is = std::istringstream(value_str);
    is >> parse(format, time_ms);
    BigInt test_ns =
      BigInt(time_ms.time_since_epoch().count()) * BigInt(milli_ns);
    BigInt diff = (ns - test_ns).abs();

    if (diff > day_ns * 365)
    {
      logging::Error() << diff.loc().view()
                       << " ns is greater than a year, likely an "
                          "integer overflow bug in std::chrono::parse()";
      return err(value, "time outside of valid range", EvalBuiltInError);
    }
    // END workaround

    return Resolver::term(ns);
  }

  Node parse_ns(const Nodes& args)
  {
    Node layout =
      unwrap_arg(args, UnwrapOpt(0).type(JSONString).func("time.parse_ns"));
    if (layout->type() == Error)
    {
      return layout;
    }

    Node value =
      unwrap_arg(args, UnwrapOpt(1).type(JSONString).func("time.parse_ns"));
    if (value->type() == Error)
    {
      return value;
    }

    std::string layout_str = get_string(layout);
    return do_parse(layout_str, value);
  }

  Node parse_rfc3339_ns(const Nodes& args)
  {
    Node value = unwrap_arg(
      args, UnwrapOpt(0).type(JSONString).func("time.parse_rfc3339_ns"));
    if (value->type() == Error)
    {
      return value;
    }

    return do_parse("RFC3339Nano", value);
  }

  Node weekday(const Nodes& args)
  {
    TimeInfo info = process_arg(args, "time.weekday", 0);
    if (info.error != nullptr)
    {
      return info.error;
    }

    info.mode = InfoMode::TimeZoneFormat;
    info.format = "%A";
    if (info.time_zone.empty())
    {
      info.time_zone = "UTC";
    }

    return info_string(info);
  }

#else

  const std::string PlaceholderMessage =
    "timezone support not available on this platform";

#endif

}

namespace rego
{
  namespace builtins
  {
    std::vector<BuiltIn> time()
    {
      return {
        std::make_shared<NowNS>(),
        BuiltInDef::create(
          Location("time.parse_duration_ns"),
          parse_duration_ns_decl,
          parse_duration_ns),
#if __cpp_lib_chrono >= 201907L
        BuiltInDef::create(Location("time.add_date"), add_date_decl, add_date),
        BuiltInDef::create(Location("time.format"), format_decl, format),
        BuiltInDef::create(Location("time.diff"), diff_decl, diff),
        BuiltInDef::create(Location("time.parse_ns"), parse_ns_decl, parse_ns),
        BuiltInDef::create(
          Location("time.parse_rfc3339_ns"),
          parse_rfc3339_ns_decl,
          parse_rfc3339_ns),
        BuiltInDef::create(Location("time.weekday"), weekday_decl, ::weekday),
        BuiltInDef::create(Location("time.clock"), clock_decl, clock_),
        BuiltInDef::create(Location("time.date"), date_decl, date_),
#else
        BuiltInDef::placeholder(
          Location("time.add_date"), add_date_decl, PlaceholderMessage),
        BuiltInDef::placeholder(
          Location("time.format"), format_decl, PlaceholderMessage),
        BuiltInDef::placeholder(
          Location("time.diff"), diff_decl, PlaceholderMessage),
        BuiltInDef::placeholder(
          Location("time.parse_ns"), parse_ns_decl, PlaceholderMessage),
        BuiltInDef::placeholder(
          Location("time.parse_rfc3339_ns"),
          parse_rfc3339_ns_decl,
          PlaceholderMessage),
        BuiltInDef::placeholder(
          Location("time.weekday"), weekday_decl, PlaceholderMessage),
        BuiltInDef::placeholder(
          Location("time.clock"), clock_decl, PlaceholderMessage),
        BuiltInDef::placeholder(
          Location("time.date"), date_decl, PlaceholderMessage),
#endif
      };
    }
  }
}