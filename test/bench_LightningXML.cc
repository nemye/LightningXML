/// @file bench_LightningXML.cc
/// @brief Performance benchmarks for LightningXML's pull parser.

#include <benchmark/benchmark.h>

#include <charconv>
#include <cstring>
#include <format>
#include <memory_resource>
#include <string>
#include <string_view>
#include <vector>

#include "LightningXML.hh"
#include "test_Helpers.hh"

// Optional fields (default): no presence tracking is emitted.
struct OptItem {
  int id{};
  std::string_view title;
  std::string_view desc;
  int status{};
};

template<>
struct xmlight::XmlMetadata<OptItem> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("id", &OptItem::id),
                                                 xmlight::field("title", &OptItem::title),
                                                 xmlight::field("desc", &OptItem::desc),
                                                 xmlight::field("status", &OptItem::status));
};

struct OptItemList {
  std::vector<OptItem> items;
};

template<>
struct xmlight::XmlMetadata<OptItemList> {
  static constexpr auto fields = std::make_tuple(xmlight::vecField("Item", &OptItemList::items));
};

// Required fields: same layout, every field marked required (worst case).
struct ReqItem {
  int id{};
  std::string_view title;
  std::string_view desc;
  int status{};
};

template<>
struct xmlight::XmlMetadata<ReqItem> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("id", &ReqItem::id, true),
                                                 xmlight::field("title", &ReqItem::title, true),
                                                 xmlight::field("desc", &ReqItem::desc, true),
                                                 xmlight::field("status", &ReqItem::status, true));
};

struct ReqItemList {
  std::vector<ReqItem> items;
};

template<>
struct xmlight::XmlMetadata<ReqItemList> {
  static constexpr auto fields =
      std::make_tuple(xmlight::vecField("Item", &ReqItemList::items, true));
};

// Owning std::string fields: exercises the raw-copy and normalization paths.
struct NormItem {
  int id{};
  std::string title;
  std::string desc;
  int status{};
};

template<>
struct xmlight::XmlMetadata<NormItem> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("id", &NormItem::id),
                                                 xmlight::field("title", &NormItem::title),
                                                 xmlight::field("desc", &NormItem::desc),
                                                 xmlight::field("status", &NormItem::status));
};

struct NormItemList {
  std::vector<NormItem> items;
};

template<>
struct xmlight::XmlMetadata<NormItemList> {
  static constexpr auto fields = std::make_tuple(xmlight::vecField("Item", &NormItemList::items));
};

static auto buildTreeXml(std::string& xml, int depth, int branching) -> void {
  if (depth <= 0) {
    xml += "<Node/>";
    return;
  }
  xml += "<Node>";
  for (int i = 0; i < branching; ++i) {
    buildTreeXml(xml, depth - 1, branching);
  }
  xml += "</Node>";
}

static auto generateLargeXml(size_t count) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n<Users>\n";
  xml.reserve(count * 100 + 30);
  for (size_t i = 0; i < count; ++i) {
    xml += std::format("  <User id=\"{}\">\n", i);
    xml += std::format("    <Name>Benchmark User {}</Name>\n", i);
    xml += std::format("    <Email>user{}@example.com</Email>\n", i);
    xml += "  </User>\n";
  }
  xml += "</Users>";
  return xml;
}

static auto generateFlatXml(size_t count) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n<FlatList>\n";
  xml.reserve(count * 150 + 50);
  for (size_t i = 0; i < count; ++i) {
    xml += std::format("  <Item id=\"{}\">\n", i);
    xml += std::format("    <title>Item Title {}</title>\n", i);
    xml += "    <desc>Some relatively short description text here.</desc>\n";
    xml += "    <status>1</status>\n";
    xml += "  </Item>\n";
  }
  xml += "</FlatList>";
  return xml;
}

static auto generateDeepXml(size_t count) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n<DeepList>\n";
  xml.reserve(count * 150 + 50);
  for (size_t i = 0; i < count; ++i) {
    xml += "  <L1><L2><L3><L4><L5>\n";
    xml += std::format("    <v>{}</v>\n", i);
    xml += "  </L5></L4></L3></L2></L1>\n";
  }
  xml += "</DeepList>";
  return xml;
}

static auto generateAttrXml(size_t count) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n<AttrList>\n";
  xml.reserve(count * 200 + 50);
  for (size_t i = 0; i < count; ++i) {
    xml += std::format("  <Item a1=\"{}\" a2=\"2\" a3=\"3\" a4=\"4\" a5=\"5\" ", i);
    xml += "s1=\"str1\" s2=\"str2\" s3=\"str3\" s4=\"str4\" s5=\"str5\"/>\n";
  }
  xml += "</AttrList>";
  return xml;
}

static auto generateOrgXml(size_t teams, size_t members) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n";
  xml += R"(<Organization id="1" name="Acme Corp">)"
         "\n";
  for (size_t d = 0; d < 2; ++d) {
    xml += std::format("  <Department id=\"{}\" name=\"Dept{}\">\n", d, d);
    for (size_t t = 0; t < teams; ++t) {
      size_t tid = d * teams + t;
      xml += std::format("    <Team id=\"{}\" name=\"Team{}\">\n", tid, tid);
      for (size_t m = 0; m < members; ++m) {
        const size_t mid = tid * members + m;
        xml += std::format("      <Member id=\"{}\" role=\"Engineer\">\n", mid);
        xml += std::format("        <FullName>Member {}</FullName>\n", mid);
        xml += std::format("        <Email>m{}@acme.com</Email>\n", mid);
        xml +=
            "        <Skills><Skill>C++</Skill><Skill>Python</Skill>"
            "<Skill>Rust</Skill></Skills>\n";
        xml += "      </Member>\n";
      }
      xml += "    </Team>\n";
    }
    xml += "  </Department>\n";
  }
  xml += "</Organization>";
  return xml;
}

static auto generateTreeXml(int depth, int branching) -> std::string {
  std::string xml;
  xml.reserve(1 << (depth + 4));
  buildTreeXml(xml, depth, branching);
  return xml;
}

static auto generateCommentHeavyXml(size_t count) -> std::string {
  // ~500 byte comments to exercise memchr-accelerated scan_to_delimiter.
  const std::string filler(480, '=');
  std::string xml = "<?xml version=\"1.0\"?>\n<Users>\n";
  xml.reserve(count * 700);
  for (size_t i = 0; i < count; ++i) {
    xml += std::format("  <!-- comment {} {} -->\n", i, filler);
    xml += std::format("  <User id=\"{}\">\n", i);
    xml += std::format("    <Name>User {}</Name>\n", i);
    xml += std::format("    <Email>u{}@e.com</Email>\n", i);
    xml += "  </User>\n";
  }
  xml += "</Users>";
  return xml;
}

static auto generateUnknownHeavyXml(size_t count) -> std::string {
  // Each User carries a large unmapped <Meta> subtree the parser must skip:
  // nested elements, attributes (including a quoted '>'), comments, CDATA.
  std::string xml = "<?xml version=\"1.0\"?>\n<Users>\n";
  xml.reserve(count * 700);
  for (size_t i = 0; i < count; ++i) {
    xml += std::format("  <User id=\"{}\">\n", i);
    xml += std::format("    <Name>User {}</Name>\n", i);
    xml += "    <Meta source=\"import\" rev=\"4\">\n";
    xml += "      <Created by=\"sys\">2026-01-01T00:00:00Z</Created>\n";
    xml += "      <Tags><Tag v=\"a\"/><Tag v=\"b\"/><Tag v=\"c\"/></Tags>\n";
    xml += "      <Note label=\"x > y\">free text of moderate length here";
    xml += " to give the scanner something to chew on</Note>\n";
    xml += "      <!-- audit: imported > converted -->\n";
    xml += "      <![CDATA[ raw <blob> data ]]>\n";
    xml += "      <Nested><Deep><Deeper attr=\"q\">zzz</Deeper></Deep>";
    xml += "</Nested>\n";
    xml += "    </Meta>\n";
    xml += std::format("    <Email>u{}@e.com</Email>\n", i);
    xml += "  </User>\n";
  }
  xml += "</Users>";
  return xml;
}

// Pre-generate payloads
const std::string kFlatXml = generateFlatXml(2000);
const std::string kDeepXml = generateDeepXml(2000);
const std::string kAttrXml = generateAttrXml(2000);
const std::string kSmallXml = generateLargeXml(1);
const std::string kLargeXml = generateLargeXml(10000);
const std::string kOrgXml = generateOrgXml(20, 10);
const std::string kTreeXml = generateTreeXml(14, 2);
const std::string kCommentXml = generateCommentHeavyXml(1000);
const std::string kUnknownXml = generateUnknownHeavyXml(2000);
static const std::string kCatalogXml = R"(<?xml version="1.0"?>
<catalog>
   <book id="bk101">
      <author>Gambardella, Matthew</author>
      <title>XML Developer's Guide</title>
      <genre>Computer</genre>
      <price>44.95</price>
      <publish_date>2000-10-01</publish_date>
      <description>An in-depth look at creating applications
      with XML.</description>
   </book>
   <book id="bk102">
      <author>Ralls, Kim</author>
      <title>Midnight Rain</title>
      <genre>Fantasy</genre>
      <price>5.95</price>
      <publish_date>2000-12-16</publish_date>
      <description>A former architect battles corporate zombies,
      an evil sorceress, and her own childhood to become queen
      of the world.</description>
   </book>
   <book id="bk103">
      <author>Corets, Eva</author>
      <title>Maeve Ascendant</title>
      <genre>Fantasy</genre>
      <price>5.95</price>
      <publish_date>2000-11-17</publish_date>
      <description>After the collapse of a nanotechnology
      society in England, the young survivors lay the
      foundation for a new society.</description>
   </book>
   <book id="bk104">
      <author>Corets, Eva</author>
      <title>Oberon's Legacy</title>
      <genre>Fantasy</genre>
      <price>5.95</price>
      <publish_date>2001-03-10</publish_date>
      <description>In post-apocalypse England, the mysterious
      agent known only as Oberon helps to create a new life
      for the inhabitants of London. Sequel to Maeve
      Ascendant.</description>
   </book>
   <book id="bk105">
      <author>Corets, Eva</author>
      <title>The Sundered Grail</title>
      <genre>Fantasy</genre>
      <price>5.95</price>
      <publish_date>2001-09-10</publish_date>
      <description>The two daughters of Maeve, half-sisters,
      battle one another for control of England. Sequel to
      Oberon's Legacy.</description>
   </book>
   <book id="bk106">
      <author>Randall, Cynthia</author>
      <title>Lover Birds</title>
      <genre>Romance</genre>
      <price>4.95</price>
      <publish_date>2000-09-02</publish_date>
      <description>When Carla meets Paul at an ornithology
      conference, tempers fly as feathers get ruffled.</description>
   </book>
   <book id="bk107">
      <author>Thurman, Paula</author>
      <title>Splish Splash</title>
      <genre>Romance</genre>
      <price>4.95</price>
      <publish_date>2000-11-02</publish_date>
      <description>A deep sea diver finds true love twenty
      thousand leagues beneath the sea.</description>
   </book>
   <book id="bk108">
      <author>Knorr, Stefan</author>
      <title>Creepy Crawlies</title>
      <genre>Horror</genre>
      <price>4.95</price>
      <publish_date>2000-12-06</publish_date>
      <description>An anthology of horror stories about roaches,
      centipedes, scorpions  and other insects.</description>
   </book>
   <book id="bk109">
      <author>Kress, Peter</author>
      <title>Paradox Lost</title>
      <genre>Science Fiction</genre>
      <price>6.95</price>
      <publish_date>2000-11-02</publish_date>
      <description>After an inadvertant trip through a Heisenberg
      Uncertainty Device, James Salway discovers the problems
      of being quantum.</description>
   </book>
   <book id="bk110">
      <author>O'Brien, Tim</author>
      <title>Microsoft .NET: The Programming Bible</title>
      <genre>Computer</genre>
      <price>36.95</price>
      <publish_date>2000-12-09</publish_date>
      <description>Microsoft's .NET initiative is explored in
      detail in this deep programmer's reference.</description>
   </book>
   <book id="bk111">
      <author>O'Brien, Tim</author>
      <title>MSXML3: A Comprehensive Guide</title>
      <genre>Computer</genre>
      <price>36.95</price>
      <publish_date>2000-12-01</publish_date>
      <description>The Microsoft MSXML3 parser is covered in
      detail, with attention to XML DOM interfaces, XSLT processing,
      SAX and more.</description>
   </book>
   <book id="bk112">
      <author>Galos, Mike</author>
      <title>Visual Studio 7: A Comprehensive Guide</title>
      <genre>Computer</genre>
      <price>49.95</price>
      <publish_date>2001-04-16</publish_date>
      <description>Microsoft Visual Studio 7 is explored in depth,
      looking at how Visual Basic, Visual C++, C#, and ASP+ are
      integrated into a comprehensive development
      environment.</description>
   </book>
</catalog>)";

static auto bmParseSmallXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kSmallXml};
    Users users;
    bool ok = xmlight::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kSmallXml.size()));
}

static auto bmParseLargeXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kLargeXml};
    Users users;
    bool ok = xmlight::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kLargeXml.size()));
}

static auto bmParseFlatXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kFlatXml};
    FlatList list;
    bool ok = xmlight::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

static auto bmParseDeepXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kDeepXml};
    DeepList list;
    bool ok = xmlight::deserialize(parser, "DeepList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kDeepXml.size()));
}

static auto bmParseAttrXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kAttrXml};
    AttrList list;
    bool ok = xmlight::deserialize(parser, "AttrList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kAttrXml.size()));
}

static auto bmParseOrgXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kOrgXml};
    Organization org;
    bool ok = xmlight::deserialize(parser, "Organization", org);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(org);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kOrgXml.size()));
}

static auto bmParseTreeXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kTreeXml};
    TreeNode root;
    bool ok = xmlight::deserialize(parser, "Node", root);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(root);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kTreeXml.size()));
}

static auto bmParseCommentHeavyXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kCommentXml};
    Users users;
    bool ok = xmlight::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCommentXml.size()));
}

static auto bmParseUnknownHeavyXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kUnknownXml};
    Users users;
    bool ok = xmlight::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kUnknownXml.size()));
}

static auto bmParseCatalog(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kCatalogXml};
    Catalog catalog;
    bool ok = xmlight::deserialize(parser, "catalog", catalog);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(catalog);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCatalogXml.size()));
}

// The default-Parser workloads re-run under xmlight::StrictParser, isolating the
// cost of the well-formedness scans (']]>' in content, '<' in attribute values,
// duplicate attributes) plus normalization on the owning-string Catalog. Puts a
// fully-conforming configuration next to the validating tree parsers.
static auto bmStrictParseFlatXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kFlatXml};
    FlatList list;
    bool ok = xmlight::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

static auto bmStrictParseDeepXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kDeepXml};
    DeepList list;
    bool ok = xmlight::deserialize(parser, "DeepList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kDeepXml.size()));
}

static auto bmStrictParseAttrXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kAttrXml};
    AttrList list;
    bool ok = xmlight::deserialize(parser, "AttrList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kAttrXml.size()));
}

static auto bmStrictParseSmallXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kSmallXml};
    Users users;
    bool ok = xmlight::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kSmallXml.size()));
}

static auto bmStrictParseLargeXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kLargeXml};
    Users users;
    bool ok = xmlight::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kLargeXml.size()));
}

static auto bmStrictParseOrgXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kOrgXml};
    Organization org;
    bool ok = xmlight::deserialize(parser, "Organization", org);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(org);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kOrgXml.size()));
}

static auto bmStrictParseTreeXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kTreeXml};
    TreeNode root;
    bool ok = xmlight::deserialize(parser, "Node", root);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(root);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kTreeXml.size()));
}

static auto bmStrictParseCommentHeavyXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kCommentXml};
    Users users;
    bool ok = xmlight::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCommentXml.size()));
}

static auto bmStrictParseCatalog(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kCatalogXml};
    Catalog catalog;
    bool ok = xmlight::deserialize(parser, "catalog", catalog);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(catalog);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCatalogXml.size()));
}

// Identical members and payload (kFlatXml); the only difference is whether
// fields are declared required, isolating the per-element presence tracking
// (bitmask OR per field + mask compare per close tag) the optional variant
// compiles away. Both records mark every field required (worst case).
static auto bmParseOptionalFields(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kFlatXml};
    OptItemList list;
    bool ok = xmlight::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

static auto bmParseRequiredFields(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kFlatXml};
    ReqItemList list;
    bool ok = xmlight::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

// Same kFlatXml payload into owning std::string fields. The delta between the
// non-normalizing baseline below and the NormalizingParser run bundles the
// owning-copy cost and the per-byte normalization scan; the payload is
// entity-free, so it measures normalization on plain content.
static auto bmParseOwnedRawStrings(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::Parser parser{kFlatXml};
    NormItemList list;
    bool ok = xmlight::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

static auto bmParseNormalizedFields(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::NormalizingParser parser{kFlatXml};
    NormItemList list;
    bool ok = xmlight::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

static auto bmStrictParseNormalizedFields(benchmark::State& state) -> void {
  for (auto _ : state) {
    xmlight::StrictParser parser{kFlatXml};
    NormItemList list;
    bool ok = xmlight::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

// Serializer benchmarks: parse the payload once, then produce a fresh string
// per iteration (the serialize() public API shape). Bytes are output bytes.
template<bool PRETTY, typename T>
static auto runSerialize(benchmark::State& state, std::string_view src, std::string_view root,
                         T& obj) -> void {
  xmlight::Parser parser{src};
  if (!xmlight::deserialize(parser, root, obj)) {
    state.SkipWithError("payload parse failed");
    return;
  }
  size_t out_bytes = 0;
  for (auto _ : state) {
    std::string xml = xmlight::serialize<PRETTY>(root, obj);
    out_bytes = xml.size();
    benchmark::DoNotOptimize(xml);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(out_bytes));
}

// Internal reference: the same tree payload parsed into pmr nodes backed by a
// monotonic buffer, isolating the allocator share of bmParseTreeXml. Different
// record type and ownership semantics -- never a README table cell.
struct PmrTreeNode {
  using allocator_type = std::pmr::polymorphic_allocator<>;
  PmrTreeNode() = default;
  explicit PmrTreeNode(const allocator_type& alloc) : children(alloc) {}
  PmrTreeNode(PmrTreeNode&& other, const allocator_type& alloc)
      : children(std::move(other.children), alloc) {}
  std::pmr::vector<PmrTreeNode> children;
};
template<>
struct xmlight::XmlMetadata<PmrTreeNode> {
  static constexpr auto fields = std::make_tuple(xmlight::vecField("Node", &PmrTreeNode::children));
};

static auto bmParseTreePooledXml(benchmark::State& state) -> void {
  std::vector<char> pool_buf(16U << 20);
  for (auto _ : state) {
    std::pmr::monotonic_buffer_resource pool{pool_buf.data(), pool_buf.size(),
                                             std::pmr::null_memory_resource()};
    xmlight::Parser parser{kTreeXml};
    PmrTreeNode root{std::pmr::polymorphic_allocator<>{&pool}};
    bool ok = xmlight::deserialize(parser, "Node", root);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(root);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kTreeXml.size()));
}

static auto bmSerializeLargeXml(benchmark::State& state) -> void {
  Users users;
  runSerialize<false>(state, kLargeXml, "Users", users);
}

static auto bmSerializeAttrXml(benchmark::State& state) -> void {
  AttrList list;
  runSerialize<false>(state, kAttrXml, "AttrList", list);
}

static auto bmSerializeCatalogPretty(benchmark::State& state) -> void {
  Catalog catalog;
  runSerialize<true>(state, kCatalogXml, "catalog", catalog);
}

BENCHMARK(bmParseFlatXml);
BENCHMARK(bmParseDeepXml);
BENCHMARK(bmParseAttrXml);
BENCHMARK(bmParseSmallXml);
BENCHMARK(bmParseLargeXml);
BENCHMARK(bmParseOrgXml);
BENCHMARK(bmParseTreeXml);
BENCHMARK(bmParseCommentHeavyXml);
BENCHMARK(bmParseUnknownHeavyXml);
BENCHMARK(bmParseCatalog);
BENCHMARK(bmParseOptionalFields);
BENCHMARK(bmParseRequiredFields);
BENCHMARK(bmParseOwnedRawStrings);
BENCHMARK(bmParseNormalizedFields);
BENCHMARK(bmStrictParseNormalizedFields);

BENCHMARK(bmStrictParseFlatXml);
BENCHMARK(bmStrictParseDeepXml);
BENCHMARK(bmStrictParseAttrXml);
BENCHMARK(bmStrictParseSmallXml);
BENCHMARK(bmStrictParseLargeXml);
BENCHMARK(bmStrictParseOrgXml);
BENCHMARK(bmStrictParseTreeXml);
BENCHMARK(bmStrictParseCommentHeavyXml);
BENCHMARK(bmStrictParseCatalog);

BENCHMARK(bmParseTreePooledXml);
BENCHMARK(bmSerializeLargeXml);
BENCHMARK(bmSerializeAttrXml);
BENCHMARK(bmSerializeCatalogPretty);

#ifdef LIGHTNINGXML_HAS_PUGIXML
#include <pugixml.hpp>

static auto childAsInt(const pugi::xml_node& node, const char* name) -> int {
  const char* text = node.child_value(name);
  int out{};
  std::from_chars(text, text + std::strlen(text), out);
  return out;
}

static auto bmPugiParseSmallXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kSmallXml.data(), kSmallXml.size());
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    for (const auto& user : doc.child("Users").children("User")) {
      ids.push_back(user.attribute("id").as_int());
      names.push_back(user.child_value("Name"));
      emails.push_back(user.child_value("Email"));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kSmallXml.size()));
}

static auto bmPugiParseLargeXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kLargeXml.data(), kLargeXml.size());
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    for (const auto& user : doc.child("Users").children("User")) {
      ids.push_back(user.attribute("id").as_int());
      names.push_back(user.child_value("Name"));
      emails.push_back(user.child_value("Email"));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kLargeXml.size()));
}

static auto bmPugiParseFlatXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kFlatXml.data(), kFlatXml.size());
    FlatList list;
    for (const auto& item : doc.child("FlatList").children("Item")) {
      FlatItem it;
      it.id = item.attribute("id").as_int();
      it.title = item.child_value("title");
      it.description = item.child_value("desc");
      it.status = childAsInt(item, "status");
      list.items.push_back(it);
    }
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

static auto bmPugiParseDeepXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kDeepXml.data(), kDeepXml.size());
    std::vector<int> values;
    for (const auto& l1 : doc.child("DeepList").children("L1")) {
      values.push_back(childAsInt(l1.child("L2").child("L3").child("L4").child("L5"), "v"));
    }
    benchmark::DoNotOptimize(values);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kDeepXml.size()));
}

static auto bmPugiParseAttrXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kAttrXml.data(), kAttrXml.size());
    std::vector<int> a1s, a2s, a3s, a4s, a5s;
    std::vector<std::string_view> s1s, s2s, s3s, s4s, s5s;
    for (const auto& item : doc.child("AttrList").children("Item")) {
      a1s.push_back(item.attribute("a1").as_int());
      a2s.push_back(item.attribute("a2").as_int());
      a3s.push_back(item.attribute("a3").as_int());
      a4s.push_back(item.attribute("a4").as_int());
      a5s.push_back(item.attribute("a5").as_int());
      s1s.push_back(item.attribute("s1").value());
      s2s.push_back(item.attribute("s2").value());
      s3s.push_back(item.attribute("s3").value());
      s4s.push_back(item.attribute("s4").value());
      s5s.push_back(item.attribute("s5").value());
    }
    benchmark::DoNotOptimize(a1s);
    benchmark::DoNotOptimize(s1s);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kAttrXml.size()));
}

static auto bmPugiParseOrgXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kOrgXml.data(), kOrgXml.size());
    Organization org;
    auto org_node = doc.child("Organization");
    org.id = org_node.attribute("id").as_int();
    org.name = org_node.attribute("name").value();
    for (const auto& dn : org_node.children("Department")) {
      OrgDepartment dept;
      dept.id = dn.attribute("id").as_int();
      dept.name = dn.attribute("name").value();
      for (const auto& tn : dn.children("Team")) {
        OrgTeam team;
        team.id = tn.attribute("id").as_int();
        team.name = tn.attribute("name").value();
        for (const auto& mn : tn.children("Member")) {
          OrgMember member;
          member.id = mn.attribute("id").as_int();
          member.role = mn.attribute("role").value();
          member.full_name = mn.child_value("FullName");
          member.email = mn.child_value("Email");
          for (const auto& sn : mn.child("Skills").children("Skill")) {
            member.skills.items.push_back(sn.child_value());
          }
          team.members.push_back(member);
        }
        dept.teams.push_back(team);
      }
      org.departments.push_back(dept);
    }
    benchmark::DoNotOptimize(org);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kOrgXml.size()));
}

static auto pugiBuildTree(TreeNode& node, const pugi::xml_node& xn) -> void {
  for (const auto& child : xn.children("Node")) {
    node.children.emplace_back();
    pugiBuildTree(node.children.back(), child);
  }
}

static auto bmPugiParseTreeXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kTreeXml.data(), kTreeXml.size());
    TreeNode root;
    pugiBuildTree(root, doc.child("Node"));
    benchmark::DoNotOptimize(root);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kTreeXml.size()));
}

static auto bmPugiParseCommentHeavyXml(benchmark::State& state) -> void {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kCommentXml.data(), kCommentXml.size());
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    for (const auto& user : doc.child("Users").children("User")) {
      ids.push_back(user.attribute("id").as_int());
      names.push_back(user.child_value("Name"));
      emails.push_back(user.child_value("Email"));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCommentXml.size()));
}

static auto bmPugiParseCatalog(benchmark::State& state) -> void {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kCatalogXml.data(), kCatalogXml.size());
    Catalog catalog;
    for (const auto& node : doc.child("catalog").children("book")) {
      Book& b = catalog.books.emplace_back();
      b.id = node.attribute("id").value();
      b.author = node.child_value("author");
      b.title = node.child_value("title");
      b.genre = node.child_value("genre");
      b.price = node.child_value("price");
      b.publish_date = node.child_value("publish_date");
      b.description = node.child_value("description");
    }
    benchmark::DoNotOptimize(catalog);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCatalogXml.size()));
}

BENCHMARK(bmPugiParseFlatXml);
BENCHMARK(bmPugiParseDeepXml);
BENCHMARK(bmPugiParseAttrXml);
BENCHMARK(bmPugiParseSmallXml);
BENCHMARK(bmPugiParseLargeXml);
BENCHMARK(bmPugiParseOrgXml);
BENCHMARK(bmPugiParseTreeXml);
BENCHMARK(bmPugiParseCommentHeavyXml);
BENCHMARK(bmPugiParseCatalog);
#endif  // LIGHTNINGXML_HAS_PUGIXML

// Comparison benchmarks (RapidXML, pugixml, libxml2) on identical payloads,
// emitting the identical output structures as the LightningXML benchmarks above, so
// only the parsing strategy differs. The four span a feature/performance
// spectrum: LightningXML (zero-copy, non-validating) -> RapidXML (in-situ DOM,
// predefined entities) -> pugixml (owning DOM, normalizing) -> libxml2 (copies,
// decodes, fully validates). Destructive parsers copy the source inside the
// timed loop for parity with pugixml's load_buffer. String accessors below
// return pointers into each library's own tree to avoid charging them for
// copies LightningXML also skips.

#ifdef LIGHTNINGXML_HAS_RAPIDXML
#include <boost/property_tree/detail/rapidxml.hpp>

namespace rx = boost::property_tree::detail::rapidxml;
using RxNode = rx::xml_node<>;

static auto rxChildSv(RxNode* n, const char* name) -> std::string_view {
  auto* c = n->first_node(name);
  return c ? std::string_view(c->value(), c->value_size()) : std::string_view{};
}

static auto rxAttrSv(RxNode* n, const char* name) -> std::string_view {
  auto* a = n->first_attribute(name);
  return a ? std::string_view(a->value(), a->value_size()) : std::string_view{};
}

static auto rxAttrInt(RxNode* n, const char* name) -> int {
  auto* a = n->first_attribute(name);
  int out{};
  if (a) {
    std::from_chars(a->value(), a->value() + a->value_size(), out);
  }
  return out;
}

static auto rxChildInt(RxNode* n, const char* name) -> int {
  auto* c = n->first_node(name);
  int out{};
  if (c) {
    std::from_chars(c->value(), c->value() + c->value_size(), out);
  }
  return out;
}

// Each workload is parameterized on the RapidXML parse flags so it can run in
// two feature points:
//   * parse_default (0): create data nodes and element values, decode the
//     predefined entities, null-terminate strings -- the apples-to-apples
//     "fully parse into a usable tree" mode aligned with pugixml/libxml2.
//   * parse_fastest: drop data nodes, entity translation, and string
//     terminators. The fastest mode RapidXML is known for, trading those
//     features for speed. All accessors here read value()/value_size() (never
//     a C-string), so they remain correct without the null terminators.
// Both modes leave zero-copy pointers into the mutable buffer; we always read
// through value_size() so the same extraction code serves either flag set.

// RapidXML parses in place into a mutable, null-terminated buffer. This helper
// produces that buffer; the copy is intentionally part of the timed work (the
// analogue of pugixml's internal load_buffer copy).
static auto rxBuffer(const std::string& src) -> std::vector<char> {
  std::vector<char> buf(src.begin(), src.end());
  buf.push_back('\0');
  return buf;
}

template<int Flags>
static auto rxRunUsers(benchmark::State& state, const std::string& src) -> void {
  for (auto _ : state) {
    auto buf = rxBuffer(src);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    auto* root = doc.first_node("Users");
    for (auto* u = root->first_node("User"); u; u = u->next_sibling("User")) {
      ids.push_back(rxAttrInt(u, "id"));
      names.push_back(rxChildSv(u, "Name"));
      emails.push_back(rxChildSv(u, "Email"));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(src.size()));
}

template<int Flags>
static auto rxRunFlat(benchmark::State& state) -> void {
  for (auto _ : state) {
    auto buf = rxBuffer(kFlatXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    FlatList list;
    auto* root = doc.first_node("FlatList");
    for (auto* item = root->first_node("Item"); item; item = item->next_sibling("Item")) {
      FlatItem it;
      it.id = rxAttrInt(item, "id");
      it.title = rxChildSv(item, "title");
      it.description = rxChildSv(item, "desc");
      it.status = rxChildInt(item, "status");
      list.items.push_back(it);
    }
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

template<int Flags>
static auto rxRunDeep(benchmark::State& state) -> void {
  for (auto _ : state) {
    auto buf = rxBuffer(kDeepXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    std::vector<int> values;
    auto* root = doc.first_node("DeepList");
    for (auto* l1 = root->first_node("L1"); l1; l1 = l1->next_sibling("L1")) {
      auto* l5 = l1->first_node("L2")->first_node("L3")->first_node("L4")->first_node("L5");
      values.push_back(rxChildInt(l5, "v"));
    }
    benchmark::DoNotOptimize(values);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kDeepXml.size()));
}

template<int Flags>
static auto rxRunAttr(benchmark::State& state) -> void {
  for (auto _ : state) {
    auto buf = rxBuffer(kAttrXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    std::vector<int> a1s, a2s, a3s, a4s, a5s;
    std::vector<std::string_view> s1s, s2s, s3s, s4s, s5s;
    auto* root = doc.first_node("AttrList");
    for (auto* item = root->first_node("Item"); item; item = item->next_sibling("Item")) {
      a1s.push_back(rxAttrInt(item, "a1"));
      a2s.push_back(rxAttrInt(item, "a2"));
      a3s.push_back(rxAttrInt(item, "a3"));
      a4s.push_back(rxAttrInt(item, "a4"));
      a5s.push_back(rxAttrInt(item, "a5"));
      s1s.push_back(rxAttrSv(item, "s1"));
      s2s.push_back(rxAttrSv(item, "s2"));
      s3s.push_back(rxAttrSv(item, "s3"));
      s4s.push_back(rxAttrSv(item, "s4"));
      s5s.push_back(rxAttrSv(item, "s5"));
    }
    benchmark::DoNotOptimize(a1s);
    benchmark::DoNotOptimize(s1s);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kAttrXml.size()));
}

static auto rxBuildOrgMember(OrgMember& member, RxNode* mn) -> void {
  member.id = rxAttrInt(mn, "id");
  member.role = rxAttrSv(mn, "role");
  member.full_name = rxChildSv(mn, "FullName");
  member.email = rxChildSv(mn, "Email");
  auto* skills = mn->first_node("Skills");
  for (auto* sn = skills->first_node("Skill"); sn; sn = sn->next_sibling("Skill")) {
    member.skills.items.push_back(std::string_view(sn->value(), sn->value_size()));
  }
}

template<int Flags>
static auto rxRunOrg(benchmark::State& state) -> void {
  for (auto _ : state) {
    auto buf = rxBuffer(kOrgXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    Organization org;
    auto* org_node = doc.first_node("Organization");
    org.id = rxAttrInt(org_node, "id");
    org.name = rxAttrSv(org_node, "name");
    for (auto* dn = org_node->first_node("Department"); dn; dn = dn->next_sibling("Department")) {
      OrgDepartment dept;
      dept.id = rxAttrInt(dn, "id");
      dept.name = rxAttrSv(dn, "name");
      for (auto* tn = dn->first_node("Team"); tn; tn = tn->next_sibling("Team")) {
        OrgTeam team;
        team.id = rxAttrInt(tn, "id");
        team.name = rxAttrSv(tn, "name");
        for (auto* mn = tn->first_node("Member"); mn; mn = mn->next_sibling("Member")) {
          OrgMember member;
          rxBuildOrgMember(member, mn);
          team.members.push_back(member);
        }
        dept.teams.push_back(team);
      }
      org.departments.push_back(dept);
    }
    benchmark::DoNotOptimize(org);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kOrgXml.size()));
}

static auto rxBuildTree(TreeNode& node, RxNode* xn) -> void {
  for (auto* child = xn->first_node("Node"); child; child = child->next_sibling("Node")) {
    node.children.emplace_back();
    rxBuildTree(node.children.back(), child);
  }
}

template<int Flags>
static auto rxRunTree(benchmark::State& state) -> void {
  for (auto _ : state) {
    auto buf = rxBuffer(kTreeXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    TreeNode root;
    rxBuildTree(root, doc.first_node("Node"));
    benchmark::DoNotOptimize(root);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kTreeXml.size()));
}

template<int Flags>
static auto rxRunCatalog(benchmark::State& state) -> void {
  for (auto _ : state) {
    auto buf = rxBuffer(kCatalogXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    Catalog catalog;
    auto* root = doc.first_node("catalog");
    for (auto* node = root->first_node("book"); node; node = node->next_sibling("book")) {
      Book& b = catalog.books.emplace_back();
      b.id = std::string(rxAttrSv(node, "id"));
      b.author = std::string(rxChildSv(node, "author"));
      b.title = std::string(rxChildSv(node, "title"));
      b.genre = std::string(rxChildSv(node, "genre"));
      b.price = std::string(rxChildSv(node, "price"));
      b.publish_date = std::string(rxChildSv(node, "publish_date"));
      b.description = std::string(rxChildSv(node, "description"));
    }
    benchmark::DoNotOptimize(catalog);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCatalogXml.size()));
}

// parse_default (full tree + entity decoding)
static auto bmRapidXmlParseFlatXml(benchmark::State& s) -> void {
  rxRunFlat<0>(s);
}
static auto bmRapidXmlParseDeepXml(benchmark::State& s) -> void {
  rxRunDeep<0>(s);
}
static auto bmRapidXmlParseAttrXml(benchmark::State& s) -> void {
  rxRunAttr<0>(s);
}
static auto bmRapidXmlParseSmallXml(benchmark::State& s) -> void {
  rxRunUsers<0>(s, kSmallXml);
}
static auto bmRapidXmlParseLargeXml(benchmark::State& s) -> void {
  rxRunUsers<0>(s, kLargeXml);
}
static auto bmRapidXmlParseOrgXml(benchmark::State& s) -> void {
  rxRunOrg<0>(s);
}
static auto bmRapidXmlParseTreeXml(benchmark::State& s) -> void {
  rxRunTree<0>(s);
}
static auto bmRapidXmlParseCommentHeavyXml(benchmark::State& s) -> void {
  rxRunUsers<0>(s, kCommentXml);
}
static auto bmRapidXmlParseCatalog(benchmark::State& s) -> void {
  rxRunCatalog<0>(s);
}

// parse_fastest (no data nodes / no entity translation / no terminators)
static auto bmRapidXmlFastParseFlatXml(benchmark::State& s) -> void {
  rxRunFlat<rx::parse_fastest>(s);
}
static auto bmRapidXmlFastParseDeepXml(benchmark::State& s) -> void {
  rxRunDeep<rx::parse_fastest>(s);
}
static auto bmRapidXmlFastParseAttrXml(benchmark::State& s) -> void {
  rxRunAttr<rx::parse_fastest>(s);
}
static auto bmRapidXmlFastParseSmallXml(benchmark::State& s) -> void {
  rxRunUsers<rx::parse_fastest>(s, kSmallXml);
}
static auto bmRapidXmlFastParseLargeXml(benchmark::State& s) -> void {
  rxRunUsers<rx::parse_fastest>(s, kLargeXml);
}
static auto bmRapidXmlFastParseOrgXml(benchmark::State& s) -> void {
  rxRunOrg<rx::parse_fastest>(s);
}
static auto bmRapidXmlFastParseTreeXml(benchmark::State& s) -> void {
  rxRunTree<rx::parse_fastest>(s);
}
static auto bmRapidXmlFastParseCommentHeavyXml(benchmark::State& s) -> void {
  rxRunUsers<rx::parse_fastest>(s, kCommentXml);
}
static auto bmRapidXmlFastParseCatalog(benchmark::State& s) -> void {
  rxRunCatalog<rx::parse_fastest>(s);
}

BENCHMARK(bmRapidXmlParseFlatXml);
BENCHMARK(bmRapidXmlParseDeepXml);
BENCHMARK(bmRapidXmlParseAttrXml);
BENCHMARK(bmRapidXmlParseSmallXml);
BENCHMARK(bmRapidXmlParseLargeXml);
BENCHMARK(bmRapidXmlParseOrgXml);
BENCHMARK(bmRapidXmlParseTreeXml);
BENCHMARK(bmRapidXmlParseCommentHeavyXml);
BENCHMARK(bmRapidXmlParseCatalog);

BENCHMARK(bmRapidXmlFastParseFlatXml);
BENCHMARK(bmRapidXmlFastParseDeepXml);
BENCHMARK(bmRapidXmlFastParseAttrXml);
BENCHMARK(bmRapidXmlFastParseSmallXml);
BENCHMARK(bmRapidXmlFastParseLargeXml);
BENCHMARK(bmRapidXmlFastParseOrgXml);
BENCHMARK(bmRapidXmlFastParseTreeXml);
BENCHMARK(bmRapidXmlFastParseCommentHeavyXml);
BENCHMARK(bmRapidXmlFastParseCatalog);
#endif  // LIGHTNINGXML_HAS_RAPIDXML

#ifdef LIGHTNINGXML_HAS_LIBXML2
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>

// Find the first element child of `parent` named `name` (nullptr if none).
static auto x2Child(xmlNode* parent, const char* name) -> xmlNode* {
  for (xmlNode* c = parent->children; c != nullptr; c = c->next) {
    if (c->type == XML_ELEMENT_NODE && xmlStrcmp(c->name, BAD_CAST name) == 0) {
      return c;
    }
  }
  return nullptr;
}

// Text content of an element, as a pointer into the tree (no extra allocation),
// matching pugixml's child_value(). Returns the first text/CDATA child.
static auto x2Text(xmlNode* n) -> const char* {
  if (n == nullptr) {
    return "";
  }
  for (xmlNode* c = n->children; c != nullptr; c = c->next) {
    if (c->type == XML_TEXT_NODE || c->type == XML_CDATA_SECTION_NODE) {
      return reinterpret_cast<const char*>(c->content);
    }
  }
  return "";
}

// Attribute value as a pointer into the tree (no extra allocation), unlike
// xmlGetProp which copies and must be freed.
static auto x2Attr(xmlNode* n, const char* name) -> const char* {
  for (xmlAttr* a = n->properties; a != nullptr; a = a->next) {
    if (xmlStrcmp(a->name, BAD_CAST name) == 0) {
      return a->children != nullptr ? reinterpret_cast<const char*>(a->children->content) : "";
    }
  }
  return "";
}

static auto x2TextInt(xmlNode* n) -> int {
  const char* t = x2Text(n);
  int out{};
  std::from_chars(t, t + std::strlen(t), out);
  return out;
}

static auto x2AttrInt(xmlNode* n, const char* name) -> int {
  const char* t = x2Attr(n, name);
  int out{};
  std::from_chars(t, t + std::strlen(t), out);
  return out;
}

static auto bmLibXml2ParseSmallXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc = xmlReadMemory(kSmallXml.data(), static_cast<int>(kSmallXml.size()), "bench.xml",
                                nullptr, 0);
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* u = root->children; u != nullptr; u = u->next) {
      if (u->type != XML_ELEMENT_NODE || xmlStrcmp(u->name, BAD_CAST "User")) {
        continue;
      }
      ids.push_back(x2AttrInt(u, "id"));
      names.push_back(x2Text(x2Child(u, "Name")));
      emails.push_back(x2Text(x2Child(u, "Email")));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kSmallXml.size()));
}

static auto bmLibXml2ParseLargeXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc = xmlReadMemory(kLargeXml.data(), static_cast<int>(kLargeXml.size()), "bench.xml",
                                nullptr, 0);
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* u = root->children; u != nullptr; u = u->next) {
      if (u->type != XML_ELEMENT_NODE || xmlStrcmp(u->name, BAD_CAST "User")) {
        continue;
      }
      ids.push_back(x2AttrInt(u, "id"));
      names.push_back(x2Text(x2Child(u, "Name")));
      emails.push_back(x2Text(x2Child(u, "Email")));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kLargeXml.size()));
}

static auto bmLibXml2ParseFlatXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kFlatXml.data(), static_cast<int>(kFlatXml.size()), "bench.xml", nullptr, 0);
    FlatList list;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* item = root->children; item != nullptr; item = item->next) {
      if (item->type != XML_ELEMENT_NODE || xmlStrcmp(item->name, BAD_CAST "Item")) {
        continue;
      }
      FlatItem it;
      it.id = x2AttrInt(item, "id");
      it.title = x2Text(x2Child(item, "title"));
      it.description = x2Text(x2Child(item, "desc"));
      it.status = x2TextInt(x2Child(item, "status"));
      list.items.push_back(it);
    }
    benchmark::DoNotOptimize(list);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

static auto bmLibXml2ParseDeepXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kDeepXml.data(), static_cast<int>(kDeepXml.size()), "bench.xml", nullptr, 0);
    std::vector<int> values;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* l1 = root->children; l1 != nullptr; l1 = l1->next) {
      if (l1->type != XML_ELEMENT_NODE || xmlStrcmp(l1->name, BAD_CAST "L1")) {
        continue;
      }
      xmlNode* l5 = x2Child(x2Child(x2Child(x2Child(l1, "L2"), "L3"), "L4"), "L5");
      values.push_back(x2TextInt(x2Child(l5, "v")));
    }
    benchmark::DoNotOptimize(values);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kDeepXml.size()));
}

static auto bmLibXml2ParseAttrXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kAttrXml.data(), static_cast<int>(kAttrXml.size()), "bench.xml", nullptr, 0);
    std::vector<int> a1s, a2s, a3s, a4s, a5s;
    std::vector<std::string_view> s1s, s2s, s3s, s4s, s5s;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* item = root->children; item != nullptr; item = item->next) {
      if (item->type != XML_ELEMENT_NODE || xmlStrcmp(item->name, BAD_CAST "Item")) {
        continue;
      }
      a1s.push_back(x2AttrInt(item, "a1"));
      a2s.push_back(x2AttrInt(item, "a2"));
      a3s.push_back(x2AttrInt(item, "a3"));
      a4s.push_back(x2AttrInt(item, "a4"));
      a5s.push_back(x2AttrInt(item, "a5"));
      s1s.push_back(x2Attr(item, "s1"));
      s2s.push_back(x2Attr(item, "s2"));
      s3s.push_back(x2Attr(item, "s3"));
      s4s.push_back(x2Attr(item, "s4"));
      s5s.push_back(x2Attr(item, "s5"));
    }
    benchmark::DoNotOptimize(a1s);
    benchmark::DoNotOptimize(s1s);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kAttrXml.size()));
}

static auto x2BuildOrgMember(OrgMember& member, xmlNode* mn) -> void {
  member.id = x2AttrInt(mn, "id");
  member.role = x2Attr(mn, "role");
  member.full_name = x2Text(x2Child(mn, "FullName"));
  member.email = x2Text(x2Child(mn, "Email"));
  xmlNode* skills = x2Child(mn, "Skills");
  for (xmlNode* sn = skills->children; sn != nullptr; sn = sn->next) {
    if (sn->type == XML_ELEMENT_NODE && xmlStrcmp(sn->name, BAD_CAST "Skill") == 0) {
      member.skills.items.push_back(x2Text(sn));
    }
  }
}

static auto bmLibXml2ParseOrgXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kOrgXml.data(), static_cast<int>(kOrgXml.size()), "bench.xml", nullptr, 0);
    Organization org;
    xmlNode* org_node = xmlDocGetRootElement(doc);
    org.id = x2AttrInt(org_node, "id");
    org.name = x2Attr(org_node, "name");
    for (xmlNode* dn = org_node->children; dn != nullptr; dn = dn->next) {
      if (dn->type != XML_ELEMENT_NODE || xmlStrcmp(dn->name, BAD_CAST "Department")) {
        continue;
      }
      OrgDepartment dept;
      dept.id = x2AttrInt(dn, "id");
      dept.name = x2Attr(dn, "name");
      for (xmlNode* tn = dn->children; tn != nullptr; tn = tn->next) {
        if (tn->type != XML_ELEMENT_NODE || xmlStrcmp(tn->name, BAD_CAST "Team")) {
          continue;
        }
        OrgTeam team;
        team.id = x2AttrInt(tn, "id");
        team.name = x2Attr(tn, "name");
        for (xmlNode* mn = tn->children; mn != nullptr; mn = mn->next) {
          if (mn->type != XML_ELEMENT_NODE || xmlStrcmp(mn->name, BAD_CAST "Member")) {
            continue;
          }
          OrgMember member;
          x2BuildOrgMember(member, mn);
          team.members.push_back(member);
        }
        dept.teams.push_back(team);
      }
      org.departments.push_back(dept);
    }
    benchmark::DoNotOptimize(org);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kOrgXml.size()));
}

static auto x2BuildTree(TreeNode& node, xmlNode* xn) -> void {
  for (xmlNode* c = xn->children; c != nullptr; c = c->next) {
    if (c->type == XML_ELEMENT_NODE && xmlStrcmp(c->name, BAD_CAST "Node") == 0) {
      node.children.emplace_back();
      x2BuildTree(node.children.back(), c);
    }
  }
}

static auto bmLibXml2ParseTreeXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kTreeXml.data(), static_cast<int>(kTreeXml.size()), "bench.xml", nullptr, 0);
    TreeNode root;
    x2BuildTree(root, xmlDocGetRootElement(doc));
    benchmark::DoNotOptimize(root);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kTreeXml.size()));
}

static auto bmLibXml2ParseCommentHeavyXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc = xmlReadMemory(kCommentXml.data(), static_cast<int>(kCommentXml.size()),
                                "bench.xml", nullptr, 0);
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* u = root->children; u != nullptr; u = u->next) {
      if (u->type != XML_ELEMENT_NODE || xmlStrcmp(u->name, BAD_CAST "User")) {
        continue;
      }
      ids.push_back(x2AttrInt(u, "id"));
      names.push_back(x2Text(x2Child(u, "Name")));
      emails.push_back(x2Text(x2Child(u, "Email")));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCommentXml.size()));
}

static auto bmLibXml2ParseCatalog(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc = xmlReadMemory(kCatalogXml.data(), static_cast<int>(kCatalogXml.size()),
                                "bench.xml", nullptr, 0);
    Catalog catalog;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* node = root->children; node != nullptr; node = node->next) {
      if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, BAD_CAST "book")) {
        continue;
      }
      Book& b = catalog.books.emplace_back();
      b.id = x2Attr(node, "id");
      b.author = x2Text(x2Child(node, "author"));
      b.title = x2Text(x2Child(node, "title"));
      b.genre = x2Text(x2Child(node, "genre"));
      b.price = x2Text(x2Child(node, "price"));
      b.publish_date = x2Text(x2Child(node, "publish_date"));
      b.description = x2Text(x2Child(node, "description"));
    }
    benchmark::DoNotOptimize(catalog);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCatalogXml.size()));
}

BENCHMARK(bmLibXml2ParseFlatXml);
BENCHMARK(bmLibXml2ParseDeepXml);
BENCHMARK(bmLibXml2ParseAttrXml);
BENCHMARK(bmLibXml2ParseSmallXml);
BENCHMARK(bmLibXml2ParseLargeXml);
BENCHMARK(bmLibXml2ParseOrgXml);
BENCHMARK(bmLibXml2ParseTreeXml);
BENCHMARK(bmLibXml2ParseCommentHeavyXml);
BENCHMARK(bmLibXml2ParseCatalog);

// The pull/streaming counterpart to the DOM benchmarks above. A streaming
// parser never materializes a tree, so it cannot hand back zero-copy views that
// outlive the cursor: every field has to be copied out as it streams past.
// These benchmarks therefore extract into owning storage (std::string / owning
// structs), which is the honest, idiomatic way to use a streaming reader -- and
// the copy cost is exactly the streaming-vs-DOM trade-off being measured.

namespace {

constexpr int kElem = XML_READER_TYPE_ELEMENT;
constexpr int kEndElem = XML_READER_TYPE_END_ELEMENT;
constexpr int kText = XML_READER_TYPE_TEXT;
constexpr int kCData = XML_READER_TYPE_CDATA;

auto srNameIs(xmlTextReaderPtr r, const char* name) -> bool {
  return xmlStrcmp(xmlTextReaderConstName(r), BAD_CAST name) == 0;
}

auto srAttrStr(xmlTextReaderPtr r, const char* name) -> std::string {
  xmlChar* v = xmlTextReaderGetAttribute(r, BAD_CAST name);
  std::string out = (v != nullptr) ? reinterpret_cast<const char*>(v) : "";
  if (v != nullptr) {
    xmlFree(v);
  }
  return out;
}

auto srAttrInt(xmlTextReaderPtr r, const char* name) -> int {
  xmlChar* v = xmlTextReaderGetAttribute(r, BAD_CAST name);
  int out{};
  if (v != nullptr) {
    const char* p = reinterpret_cast<const char*>(v);
    std::from_chars(p, p + std::strlen(p), out);
    xmlFree(v);
  }
  return out;
}

// Reader is positioned on a start element; gather its text content (copied out)
// and leave the reader on the element's matching end tag.
auto srText(xmlTextReaderPtr r) -> std::string {
  if (xmlTextReaderIsEmptyElement(r)) {
    return {};
  }
  std::string out;
  const int depth = xmlTextReaderDepth(r);
  while (xmlTextReaderRead(r) == 1) {
    const int t = xmlTextReaderNodeType(r);
    if (t == kEndElem && xmlTextReaderDepth(r) == depth) {
      break;
    }
    if (t == kText || t == kCData) {
      const xmlChar* v = xmlTextReaderConstValue(r);
      if (v != nullptr) {
        out += reinterpret_cast<const char*>(v);
      }
    }
  }
  return out;
}

auto srTextInt(xmlTextReaderPtr r) -> int {
  const std::string s = srText(r);
  int out{};
  std::from_chars(s.data(), s.data() + s.size(), out);
  return out;
}

// Owning mirrors of the Org/Member records (streaming cannot retain views).
struct SrMember {
  int id{};
  std::string role, full_name, email;
  std::vector<std::string> skills;
};
struct SrTeam {
  int id{};
  std::string name;
  std::vector<SrMember> members;
};
struct SrDept {
  int id{};
  std::string name;
  std::vector<SrTeam> teams;
};
struct SrOrg {
  int id{};
  std::string name;
  std::vector<SrDept> depts;
};

auto srOpen(const std::string& src) -> xmlTextReaderPtr {
  return xmlReaderForMemory(src.data(), static_cast<int>(src.size()), "bench.xml", nullptr, 0);
}

}  // namespace

static auto srRunUsers(benchmark::State& state, const std::string& src) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = srOpen(src);
    std::vector<int> ids;
    std::vector<std::string> names, emails;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem) {
        continue;
      }
      if (srNameIs(r, "User")) {
        ids.push_back(srAttrInt(r, "id"));
      } else if (srNameIs(r, "Name")) {
        names.push_back(srText(r));
      } else if (srNameIs(r, "Email")) {
        emails.push_back(srText(r));
      }
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(src.size()));
}

static auto bmLibXml2ReaderParseSmallXml(benchmark::State& state) -> void {
  srRunUsers(state, kSmallXml);
}
static auto bmLibXml2ReaderParseLargeXml(benchmark::State& state) -> void {
  srRunUsers(state, kLargeXml);
}
static auto bmLibXml2ReaderParseCommentHeavyXml(benchmark::State& state) -> void {
  srRunUsers(state, kCommentXml);
}

static auto bmLibXml2ReaderParseFlatXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = srOpen(kFlatXml);
    std::vector<int> ids, statuses;
    std::vector<std::string> titles, descs;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem) {
        continue;
      }
      if (srNameIs(r, "Item")) {
        ids.push_back(srAttrInt(r, "id"));
      } else if (srNameIs(r, "title")) {
        titles.push_back(srText(r));
      } else if (srNameIs(r, "desc")) {
        descs.push_back(srText(r));
      } else if (srNameIs(r, "status")) {
        statuses.push_back(srTextInt(r));
      }
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(titles);
    benchmark::DoNotOptimize(descs);
    benchmark::DoNotOptimize(statuses);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kFlatXml.size()));
}

static auto bmLibXml2ReaderParseDeepXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = srOpen(kDeepXml);
    std::vector<int> values;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) == kElem && srNameIs(r, "v")) {
        values.push_back(srTextInt(r));
      }
    }
    benchmark::DoNotOptimize(values);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kDeepXml.size()));
}

static auto bmLibXml2ReaderParseAttrXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = srOpen(kAttrXml);
    std::vector<int> a1s, a2s, a3s, a4s, a5s;
    std::vector<std::string> s1s, s2s, s3s, s4s, s5s;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem || !srNameIs(r, "Item")) {
        continue;
      }
      a1s.push_back(srAttrInt(r, "a1"));
      a2s.push_back(srAttrInt(r, "a2"));
      a3s.push_back(srAttrInt(r, "a3"));
      a4s.push_back(srAttrInt(r, "a4"));
      a5s.push_back(srAttrInt(r, "a5"));
      s1s.push_back(srAttrStr(r, "s1"));
      s2s.push_back(srAttrStr(r, "s2"));
      s3s.push_back(srAttrStr(r, "s3"));
      s4s.push_back(srAttrStr(r, "s4"));
      s5s.push_back(srAttrStr(r, "s5"));
    }
    benchmark::DoNotOptimize(a1s);
    benchmark::DoNotOptimize(s1s);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kAttrXml.size()));
}

// Recursive-descent reconstruction of a <Member> subtree from the stream.
static auto srReadMember(xmlTextReaderPtr r, SrMember& m) -> void {
  m.id = srAttrInt(r, "id");
  m.role = srAttrStr(r, "role");
  if (xmlTextReaderIsEmptyElement(r)) {
    return;
  }
  const int depth = xmlTextReaderDepth(r);
  while (xmlTextReaderRead(r) == 1) {
    const int t = xmlTextReaderNodeType(r);
    if (t == kEndElem && xmlTextReaderDepth(r) == depth) {
      break;
    }
    if (t != kElem) {
      continue;
    }
    if (srNameIs(r, "FullName")) {
      m.full_name = srText(r);
    } else if (srNameIs(r, "Email")) {
      m.email = srText(r);
    } else if (srNameIs(r, "Skills")) {
      if (xmlTextReaderIsEmptyElement(r)) {
        continue;
      }
      const int sdepth = xmlTextReaderDepth(r);
      while (xmlTextReaderRead(r) == 1) {
        const int st = xmlTextReaderNodeType(r);
        if (st == kEndElem && xmlTextReaderDepth(r) == sdepth) {
          break;
        }
        if (st == kElem && srNameIs(r, "Skill")) {
          m.skills.push_back(srText(r));
        }
      }
    }
  }
}

static auto bmLibXml2ReaderParseOrgXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = srOpen(kOrgXml);
    SrOrg org;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem) {
        continue;
      }
      if (srNameIs(r, "Organization")) {
        org.id = srAttrInt(r, "id");
        org.name = srAttrStr(r, "name");
      } else if (srNameIs(r, "Department")) {
        SrDept& d = org.depts.emplace_back();
        d.id = srAttrInt(r, "id");
        d.name = srAttrStr(r, "name");
      } else if (srNameIs(r, "Team")) {
        SrTeam& t = org.depts.back().teams.emplace_back();
        t.id = srAttrInt(r, "id");
        t.name = srAttrStr(r, "name");
      } else if (srNameIs(r, "Member")) {
        SrMember& m = org.depts.back().teams.back().members.emplace_back();
        srReadMember(r, m);
      }
    }
    benchmark::DoNotOptimize(org);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kOrgXml.size()));
}

static auto bmLibXml2ReaderParseTreeXml(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = srOpen(kTreeXml);
    TreeNode root;
    std::vector<TreeNode*> stack{&root};
    while (xmlTextReaderRead(r) == 1) {
      const int t = xmlTextReaderNodeType(r);
      if (t == kElem && srNameIs(r, "Node")) {
        TreeNode& child = stack.back()->children.emplace_back();
        if (!xmlTextReaderIsEmptyElement(r)) {
          stack.push_back(&child);
        }
      } else if (t == kEndElem && srNameIs(r, "Node")) {
        stack.pop_back();
      }
    }
    benchmark::DoNotOptimize(root);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kTreeXml.size()));
}

static auto bmLibXml2ReaderParseCatalog(benchmark::State& state) -> void {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = srOpen(kCatalogXml);
    Catalog catalog;
    Book* cur = nullptr;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem) {
        continue;
      }
      if (srNameIs(r, "book")) {
        cur = &catalog.books.emplace_back();
        cur->id = srAttrStr(r, "id");
      } else if (cur != nullptr) {
        if (srNameIs(r, "author")) {
          cur->author = srText(r);
        } else if (srNameIs(r, "title")) {
          cur->title = srText(r);
        } else if (srNameIs(r, "genre")) {
          cur->genre = srText(r);
        } else if (srNameIs(r, "price")) {
          cur->price = srText(r);
        } else if (srNameIs(r, "publish_date")) {
          cur->publish_date = srText(r);
        } else if (srNameIs(r, "description")) {
          cur->description = srText(r);
        }
      }
    }
    benchmark::DoNotOptimize(catalog);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kCatalogXml.size()));
}

BENCHMARK(bmLibXml2ReaderParseFlatXml);
BENCHMARK(bmLibXml2ReaderParseDeepXml);
BENCHMARK(bmLibXml2ReaderParseAttrXml);
BENCHMARK(bmLibXml2ReaderParseSmallXml);
BENCHMARK(bmLibXml2ReaderParseLargeXml);
BENCHMARK(bmLibXml2ReaderParseOrgXml);
BENCHMARK(bmLibXml2ReaderParseTreeXml);
BENCHMARK(bmLibXml2ReaderParseCommentHeavyXml);
BENCHMARK(bmLibXml2ReaderParseCatalog);
#endif  // LIGHTNINGXML_HAS_LIBXML2

BENCHMARK_MAIN();