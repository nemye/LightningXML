/// @file bench_TurboXML.cc
/// @brief Performance benchmarks for TurboXML's pull parser.

#include <benchmark/benchmark.h>

#include <charconv>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "Helpers.hh"
#include "TurboXML.hh"

// ---- Benchmark record types ----
//
// Record/metadata declarations used by the field-handling benchmarks below
// (required-vs-optional tracking and string normalization). Each pairs an
// xml::XmlMetadata specialization with its struct; the per-benchmark comments
// further down explain how they are compared.

// Optional fields (default): no presence tracking is emitted.
struct OptItem {
  int id{};
  std::string_view title;
  std::string_view desc;
  int status{};
};

template <>
struct xml::XmlMetadata<OptItem> {
  static constexpr auto fields = std::make_tuple(
      xml::attr_field("id", &OptItem::id), xml::field("title", &OptItem::title),
      xml::field("desc", &OptItem::desc),
      xml::field("status", &OptItem::status));
};

struct OptItemList {
  std::vector<OptItem> items;
};

template <>
struct xml::XmlMetadata<OptItemList> {
  static constexpr auto fields =
      std::make_tuple(xml::vec_field("Item", &OptItemList::items));
};

// Required fields: same layout, every field marked required (worst case).
struct ReqItem {
  int id{};
  std::string_view title;
  std::string_view desc;
  int status{};
};

template <>
struct xml::XmlMetadata<ReqItem> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("id", &ReqItem::id, true),
                      xml::field("title", &ReqItem::title, true),
                      xml::field("desc", &ReqItem::desc, true),
                      xml::field("status", &ReqItem::status, true));
};

struct ReqItemList {
  std::vector<ReqItem> items;
};

template <>
struct xml::XmlMetadata<ReqItemList> {
  static constexpr auto fields =
      std::make_tuple(xml::vec_field("Item", &ReqItemList::items, true));
};

// Owning std::string fields: exercises the raw-copy and normalization paths.
struct NormItem {
  int id{};
  std::string title;
  std::string desc;
  int status{};
};

template <>
struct xml::XmlMetadata<NormItem> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("id", &NormItem::id),
                      xml::field("title", &NormItem::title),
                      xml::field("desc", &NormItem::desc),
                      xml::field("status", &NormItem::status));
};

struct NormItemList {
  std::vector<NormItem> items;
};

template <>
struct xml::XmlMetadata<NormItemList> {
  static constexpr auto fields =
      std::make_tuple(xml::vec_field("Item", &NormItemList::items));
};

// Helper functions
static void build_tree_xml(std::string& xml, int depth, int branching) {
  if (depth <= 0) {
    xml += "<Node/>";
    return;
  }
  xml += "<Node>";
  for (int i = 0; i < branching; ++i) {
    build_tree_xml(xml, depth - 1, branching);
  }
  xml += "</Node>";
}

// Payload generators
static auto GenerateLargeXml(size_t count) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n<Users>\n";
  xml.reserve(count * 100 + 30);
  for (size_t i = 0; i < count; ++i) {
    xml += "  <User id=\"" + std::to_string(i) + "\">\n";
    xml += "    <Name>Benchmark User " + std::to_string(i) + "</Name>\n";
    xml += "    <Email>user" + std::to_string(i) + "@example.com</Email>\n";
    xml += "  </User>\n";
  }
  xml += "</Users>";
  return xml;
}

static auto GenerateFlatXml(size_t count) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n<FlatList>\n";
  xml.reserve(count * 150 + 50);
  for (size_t i = 0; i < count; ++i) {
    const std::string idx = std::to_string(i);
    xml += "  <Item id=\"" + idx + "\">\n";
    xml += "    <title>Item Title " + idx + "</title>\n";
    xml += "    <desc>Some relatively short description text here.</desc>\n";
    xml += "    <status>1</status>\n";
    xml += "  </Item>\n";
  }
  xml += "</FlatList>";
  return xml;
}

static auto GenerateDeepXml(size_t count) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n<DeepList>\n";
  xml.reserve(count * 150 + 50);
  for (size_t i = 0; i < count; ++i) {
    xml += "  <L1><L2><L3><L4><L5>\n";
    xml += "    <v>" + std::to_string(i) + "</v>\n";
    xml += "  </L5></L4></L3></L2></L1>\n";
  }
  xml += "</DeepList>";
  return xml;
}

static auto GenerateAttrXml(size_t count) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n<AttrList>\n";
  xml.reserve(count * 200 + 50);
  for (size_t i = 0; i < count; ++i) {
    const std::string idx = std::to_string(i);
    xml += "  <Item a1=\"" + idx + "\" a2=\"2\" a3=\"3\" a4=\"4\" a5=\"5\" ";
    xml += "s1=\"str1\" s2=\"str2\" s3=\"str3\" s4=\"str4\" s5=\"str5\"/>\n";
  }
  xml += "</AttrList>";
  return xml;
}

static auto GenerateOrgXml(size_t teams, size_t members) -> std::string {
  std::string xml = "<?xml version=\"1.0\"?>\n";
  xml += R"(<Organization id="1" name="Acme Corp">)"
         "\n";
  for (size_t d = 0; d < 2; ++d) {
    xml += "  <Department id=\"" + std::to_string(d) + "\" name=\"Dept" +
           std::to_string(d) + "\">\n";
    for (size_t t = 0; t < teams; ++t) {
      size_t tid = d * teams + t;
      xml += "    <Team id=\"" + std::to_string(tid) + "\" name=\"Team" +
             std::to_string(tid) + "\">\n";
      for (size_t m = 0; m < members; ++m) {
        std::string mid = std::to_string(tid * members + m);
        xml += "      <Member id=\"" + mid + "\" role=\"Engineer\">\n";
        xml += "        <FullName>Member " + mid + "</FullName>\n";
        xml += "        <Email>m" + mid + "@acme.com</Email>\n";
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

static auto GenerateTreeXml(int depth, int branching) -> std::string {
  std::string xml;
  xml.reserve(1 << (depth + 4));
  build_tree_xml(xml, depth, branching);
  return xml;
}

static auto GenerateCommentHeavyXml(size_t count) -> std::string {
  // ~500 byte comments to exercise memchr-accelerated scan_to_delimiter.
  const std::string filler(480, '=');
  std::string xml = "<?xml version=\"1.0\"?>\n<Users>\n";
  xml.reserve(count * 700);
  for (size_t i = 0; i < count; ++i) {
    xml += "  <!-- comment " + std::to_string(i) + " " + filler + " -->\n";
    xml += "  <User id=\"" + std::to_string(i) + "\">\n";
    xml += "    <Name>User " + std::to_string(i) + "</Name>\n";
    xml += "    <Email>u" + std::to_string(i) + "@e.com</Email>\n";
    xml += "  </User>\n";
  }
  xml += "</Users>";
  return xml;
}

static auto GenerateUnknownHeavyXml(size_t count) -> std::string {
  // Each User carries a large unmapped <Meta> subtree the parser must skip:
  // nested elements, attributes (including a quoted '>'), comments, CDATA.
  std::string xml = "<?xml version=\"1.0\"?>\n<Users>\n";
  xml.reserve(count * 700);
  for (size_t i = 0; i < count; ++i) {
    const std::string idx = std::to_string(i);
    xml += "  <User id=\"" + idx + "\">\n";
    xml += "    <Name>User " + idx + "</Name>\n";
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
    xml += "    <Email>u" + idx + "@e.com</Email>\n";
    xml += "  </User>\n";
  }
  xml += "</Users>";
  return xml;
}

// Pre-generate payloads
const std::string kFlatXml = GenerateFlatXml(2000);
const std::string kDeepXml = GenerateDeepXml(2000);
const std::string kAttrXml = GenerateAttrXml(2000);
const std::string kSmallXml = GenerateLargeXml(1);
const std::string kLargeXml = GenerateLargeXml(10000);
const std::string kOrgXml = GenerateOrgXml(20, 10);
const std::string kTreeXml = GenerateTreeXml(14, 2);
const std::string kCommentXml = GenerateCommentHeavyXml(1000);
const std::string kUnknownXml = GenerateUnknownHeavyXml(2000);
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

static void BM_ParseSmallXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kSmallXml};
    Users users;
    bool ok = xml::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kSmallXml.size()));
}

static void BM_ParseLargeXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kLargeXml};
    Users users;
    bool ok = xml::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kLargeXml.size()));
}

static void BM_ParseFlatXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kFlatXml};
    FlatList list;
    bool ok = xml::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

static void BM_ParseDeepXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kDeepXml};
    DeepList list;
    bool ok = xml::deserialize(parser, "DeepList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kDeepXml.size()));
}

static void BM_ParseAttrXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kAttrXml};
    AttrList list;
    bool ok = xml::deserialize(parser, "AttrList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kAttrXml.size()));
}

static void BM_ParseOrgXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kOrgXml};
    Organization org;
    bool ok = xml::deserialize(parser, "Organization", org);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(org);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kOrgXml.size()));
}

static void BM_ParseTreeXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kTreeXml};
    TreeNode root;
    bool ok = xml::deserialize(parser, "Node", root);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(root);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kTreeXml.size()));
}

static void BM_ParseCommentHeavyXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kCommentXml};
    Users users;
    bool ok = xml::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCommentXml.size()));
}

static void BM_ParseUnknownHeavyXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kUnknownXml};
    Users users;
    bool ok = xml::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kUnknownXml.size()));
}

static void BM_ParseCatalog(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kCatalogXml};
    Catalog catalog;
    bool ok = xml::deserialize(parser, "catalog", catalog);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(catalog);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCatalogXml.size()));
}

// ---- StrictParser across the comparison workloads ----
//
// The same workloads and output records as the default-Parser benchmarks above,
// but parsed with xml::StrictParser (normalize + strict well-formedness). On
// the std::string_view records the views stay raw and zero-copy; the extra cost
// is purely the three well-formedness scans (']]>' in content, '<' in attribute
// values, duplicate attribute names). The std::string Catalog additionally
// routes its owning fields through the normalization scan. This places a
// fully-conforming TurboXML configuration next to the validating tree parsers
// (pugixml / libxml2) on identical input.

static void BM_Strict_ParseFlatXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kFlatXml};
    FlatList list;
    bool ok = xml::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

static void BM_Strict_ParseDeepXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kDeepXml};
    DeepList list;
    bool ok = xml::deserialize(parser, "DeepList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kDeepXml.size()));
}

static void BM_Strict_ParseAttrXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kAttrXml};
    AttrList list;
    bool ok = xml::deserialize(parser, "AttrList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kAttrXml.size()));
}

static void BM_Strict_ParseSmallXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kSmallXml};
    Users users;
    bool ok = xml::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kSmallXml.size()));
}

static void BM_Strict_ParseLargeXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kLargeXml};
    Users users;
    bool ok = xml::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kLargeXml.size()));
}

static void BM_Strict_ParseOrgXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kOrgXml};
    Organization org;
    bool ok = xml::deserialize(parser, "Organization", org);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(org);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kOrgXml.size()));
}

static void BM_Strict_ParseTreeXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kTreeXml};
    TreeNode root;
    bool ok = xml::deserialize(parser, "Node", root);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(root);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kTreeXml.size()));
}

static void BM_Strict_ParseCommentHeavyXml(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kCommentXml};
    Users users;
    bool ok = xml::deserialize(parser, "Users", users);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(users);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCommentXml.size()));
}

static void BM_Strict_ParseCatalog(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kCatalogXml};
    Catalog catalog;
    bool ok = xml::deserialize(parser, "catalog", catalog);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(catalog);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCatalogXml.size()));
}

// ---- Required vs optional fields ----
//
// Two records with identical members and the identical payload (kFlatXml, a
// 2000-element <FlatList> of <Item id><title><desc><status>); the only
// difference is whether the fields are declared required. Marking a field
// required turns on per-element presence tracking (a bitmask OR at each matched
// field) plus a mask comparison at each closing tag in Parser::pull(); the
// optional variant compiles all of that away (kHasRequired == false). Parsing
// the same bytes both ways isolates that tracking overhead. Both records mark
// every field required (the worst case for the required path).

static void BM_ParseOptionalFields(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kFlatXml};
    OptItemList list;
    bool ok = xml::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

static void BM_ParseRequiredFields(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kFlatXml};
    ReqItemList list;
    bool ok = xml::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

// ---- Normalizing string content ----
//
// Same kFlatXml payload, but the record holds owning std::string fields and is
// parsed with xml::NormalizingParser. On that parser, std::string fields route
// every text run through the normalization scan (reference expansion, CR/CRLF
// folding, attribute whitespace) instead of a single zero-copy view assignment.
//
// Comparison axis vs the two benchmarks above: those use std::string_view (raw,
// zero-copy), so the delta here bundles two costs - the owning std::string copy
// AND the per-byte normalization scan. This payload is entity-free, so it
// measures the steady-state cost of the normalization machinery on plain
// content; documents carrying many &entities;/&#refs; would add expansion work
// on top. BM_ParseOwnedRawStrings isolates the owning-copy half so the
// normalization half can be read off as the difference.

// Owning std::string fields parsed with the default (non-normalizing) parser:
// the copy cost without the normalization scan, as a baseline for the delta.
static void BM_ParseOwnedRawStrings(benchmark::State& state) {
  for (auto _ : state) {
    xml::Parser parser{kFlatXml};
    NormItemList list;
    bool ok = xml::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

static void BM_ParseNormalizedFields(benchmark::State& state) {
  for (auto _ : state) {
    xml::NormalizingParser parser{kFlatXml};
    NormItemList list;
    bool ok = xml::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

static void BM_StrictParseNormalizedFields(benchmark::State& state) {
  for (auto _ : state) {
    xml::StrictParser parser{kFlatXml};
    NormItemList list;
    bool ok = xml::deserialize(parser, "FlatList", list);
    benchmark::DoNotOptimize(ok);
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

BENCHMARK(BM_ParseFlatXml);
BENCHMARK(BM_ParseDeepXml);
BENCHMARK(BM_ParseAttrXml);
BENCHMARK(BM_ParseSmallXml);
BENCHMARK(BM_ParseLargeXml);
BENCHMARK(BM_ParseOrgXml);
BENCHMARK(BM_ParseTreeXml);
BENCHMARK(BM_ParseCommentHeavyXml);
BENCHMARK(BM_ParseUnknownHeavyXml);
BENCHMARK(BM_ParseCatalog);
BENCHMARK(BM_ParseOptionalFields);
BENCHMARK(BM_ParseRequiredFields);
BENCHMARK(BM_ParseOwnedRawStrings);
BENCHMARK(BM_ParseNormalizedFields);
BENCHMARK(BM_StrictParseNormalizedFields);

BENCHMARK(BM_Strict_ParseFlatXml);
BENCHMARK(BM_Strict_ParseDeepXml);
BENCHMARK(BM_Strict_ParseAttrXml);
BENCHMARK(BM_Strict_ParseSmallXml);
BENCHMARK(BM_Strict_ParseLargeXml);
BENCHMARK(BM_Strict_ParseOrgXml);
BENCHMARK(BM_Strict_ParseTreeXml);
BENCHMARK(BM_Strict_ParseCommentHeavyXml);
BENCHMARK(BM_Strict_ParseCatalog);

#ifdef TURBOXML_HAS_PUGIXML
#include <pugixml.hpp>

static auto child_as_int(const pugi::xml_node& node, const char* name) -> int {
  const char* text = node.child_value(name);
  int out{};
  std::from_chars(text, text + std::strlen(text), out);
  return out;
}

static void BM_Pugi_ParseSmallXml(benchmark::State& state) {
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
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kSmallXml.size()));
}

static void BM_Pugi_ParseLargeXml(benchmark::State& state) {
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
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kLargeXml.size()));
}

static void BM_Pugi_ParseFlatXml(benchmark::State& state) {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kFlatXml.data(), kFlatXml.size());
    FlatList list;
    for (const auto& item : doc.child("FlatList").children("Item")) {
      FlatItem it;
      it.id = item.attribute("id").as_int();
      it.title = item.child_value("title");
      it.description = item.child_value("desc");
      it.status = child_as_int(item, "status");
      list.items.push_back(it);
    }
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

static void BM_Pugi_ParseDeepXml(benchmark::State& state) {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kDeepXml.data(), kDeepXml.size());
    std::vector<int> values;
    for (const auto& l1 : doc.child("DeepList").children("L1")) {
      values.push_back(child_as_int(
          l1.child("L2").child("L3").child("L4").child("L5"), "v"));
    }
    benchmark::DoNotOptimize(values);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kDeepXml.size()));
}

static void BM_Pugi_ParseAttrXml(benchmark::State& state) {
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
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kAttrXml.size()));
}

static void BM_Pugi_ParseOrgXml(benchmark::State& state) {
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
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kOrgXml.size()));
}

static void pugi_build_tree(TreeNode& node, const pugi::xml_node& xn) {
  for (const auto& child : xn.children("Node")) {
    node.children.emplace_back();
    pugi_build_tree(node.children.back(), child);
  }
}

static void BM_Pugi_ParseTreeXml(benchmark::State& state) {
  for (auto _ : state) {
    pugi::xml_document doc;
    doc.load_buffer(kTreeXml.data(), kTreeXml.size());
    TreeNode root;
    pugi_build_tree(root, doc.child("Node"));
    benchmark::DoNotOptimize(root);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kTreeXml.size()));
}

static void BM_Pugi_ParseCommentHeavyXml(benchmark::State& state) {
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
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCommentXml.size()));
}

static void BM_Pugi_ParseCatalog(benchmark::State& state) {
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
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCatalogXml.size()));
}

BENCHMARK(BM_Pugi_ParseFlatXml);
BENCHMARK(BM_Pugi_ParseDeepXml);
BENCHMARK(BM_Pugi_ParseAttrXml);
BENCHMARK(BM_Pugi_ParseSmallXml);
BENCHMARK(BM_Pugi_ParseLargeXml);
BENCHMARK(BM_Pugi_ParseOrgXml);
BENCHMARK(BM_Pugi_ParseTreeXml);
BENCHMARK(BM_Pugi_ParseCommentHeavyXml);
BENCHMARK(BM_Pugi_ParseCatalog);
#endif  // TURBOXML_HAS_PUGIXML

// ============================================================================
// Comparison benchmarks: RapidXML and libxml2
// ----------------------------------------------------------------------------
// All three comparison libraries are timed on the identical payloads and emit
// the identical output structures (the same FlatItem/Organization/etc. records,
// or the same id/name/email vectors) as the TurboXML benchmarks above, so the
// only thing that differs is the parsing strategy and its feature set. Read the
// four together as a feature/performance spectrum:
//
//   * TurboXML (default Parser) - zero-copy, non-normalizing, non-validating.
//     Views point straight into the source; no entity decoding, no DOM. Fewest
//     features, no allocation for string fields.
//   * RapidXML - in-situ DOM. Parses destructively into a *mutable* buffer and
//     leaves zero-copy pointers into it; decodes the predefined entities. We
//     copy the source into a fresh buffer inside the timed loop because the
//     parse is destructive -- the analogue of pugixml's internal load_buffer
//     copy, kept inside timing for parity.
//   * pugixml - builds an owning DOM (load_buffer copies the source), decodes
//     entities, normalizes. We then walk the tree to fill the structs.
//   * libxml2 - the feature-rich end: copies every string into the tree,
//     decodes entities, and fully validates well-formedness.
//
// To avoid charging the comparison parsers for work TurboXML also skips, the
// string accessors below return pointers into each library's own tree (no extra
// copy beyond what the parse already did), matching pugixml's child_value().
// ============================================================================

#ifdef TURBOXML_HAS_RAPIDXML
#include <boost/property_tree/detail/rapidxml.hpp>

namespace rx = boost::property_tree::detail::rapidxml;
using RxNode = rx::xml_node<>;

static auto rx_child_sv(RxNode* n, const char* name) -> std::string_view {
  auto* c = n->first_node(name);
  return c ? std::string_view(c->value(), c->value_size()) : std::string_view{};
}

static auto rx_attr_sv(RxNode* n, const char* name) -> std::string_view {
  auto* a = n->first_attribute(name);
  return a ? std::string_view(a->value(), a->value_size()) : std::string_view{};
}

static auto rx_attr_int(RxNode* n, const char* name) -> int {
  auto* a = n->first_attribute(name);
  int out{};
  if (a) std::from_chars(a->value(), a->value() + a->value_size(), out);
  return out;
}

static auto rx_child_int(RxNode* n, const char* name) -> int {
  auto* c = n->first_node(name);
  int out{};
  if (c) std::from_chars(c->value(), c->value() + c->value_size(), out);
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
static auto rx_buffer(const std::string& src) -> std::vector<char> {
  std::vector<char> buf(src.begin(), src.end());
  buf.push_back('\0');
  return buf;
}

template <int Flags>
static void rx_run_users(benchmark::State& state, const std::string& src) {
  for (auto _ : state) {
    auto buf = rx_buffer(src);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    auto* root = doc.first_node("Users");
    for (auto* u = root->first_node("User"); u; u = u->next_sibling("User")) {
      ids.push_back(rx_attr_int(u, "id"));
      names.push_back(rx_child_sv(u, "Name"));
      emails.push_back(rx_child_sv(u, "Email"));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(src.size()));
}

template <int Flags>
static void rx_run_flat(benchmark::State& state) {
  for (auto _ : state) {
    auto buf = rx_buffer(kFlatXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    FlatList list;
    auto* root = doc.first_node("FlatList");
    for (auto* item = root->first_node("Item"); item;
         item = item->next_sibling("Item")) {
      FlatItem it;
      it.id = rx_attr_int(item, "id");
      it.title = rx_child_sv(item, "title");
      it.description = rx_child_sv(item, "desc");
      it.status = rx_child_int(item, "status");
      list.items.push_back(it);
    }
    benchmark::DoNotOptimize(list);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

template <int Flags>
static void rx_run_deep(benchmark::State& state) {
  for (auto _ : state) {
    auto buf = rx_buffer(kDeepXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    std::vector<int> values;
    auto* root = doc.first_node("DeepList");
    for (auto* l1 = root->first_node("L1"); l1; l1 = l1->next_sibling("L1")) {
      auto* l5 =
          l1->first_node("L2")->first_node("L3")->first_node("L4")->first_node(
              "L5");
      values.push_back(rx_child_int(l5, "v"));
    }
    benchmark::DoNotOptimize(values);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kDeepXml.size()));
}

template <int Flags>
static void rx_run_attr(benchmark::State& state) {
  for (auto _ : state) {
    auto buf = rx_buffer(kAttrXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    std::vector<int> a1s, a2s, a3s, a4s, a5s;
    std::vector<std::string_view> s1s, s2s, s3s, s4s, s5s;
    auto* root = doc.first_node("AttrList");
    for (auto* item = root->first_node("Item"); item;
         item = item->next_sibling("Item")) {
      a1s.push_back(rx_attr_int(item, "a1"));
      a2s.push_back(rx_attr_int(item, "a2"));
      a3s.push_back(rx_attr_int(item, "a3"));
      a4s.push_back(rx_attr_int(item, "a4"));
      a5s.push_back(rx_attr_int(item, "a5"));
      s1s.push_back(rx_attr_sv(item, "s1"));
      s2s.push_back(rx_attr_sv(item, "s2"));
      s3s.push_back(rx_attr_sv(item, "s3"));
      s4s.push_back(rx_attr_sv(item, "s4"));
      s5s.push_back(rx_attr_sv(item, "s5"));
    }
    benchmark::DoNotOptimize(a1s);
    benchmark::DoNotOptimize(s1s);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kAttrXml.size()));
}

static void rx_build_org_member(OrgMember& member, RxNode* mn) {
  member.id = rx_attr_int(mn, "id");
  member.role = rx_attr_sv(mn, "role");
  member.full_name = rx_child_sv(mn, "FullName");
  member.email = rx_child_sv(mn, "Email");
  auto* skills = mn->first_node("Skills");
  for (auto* sn = skills->first_node("Skill"); sn;
       sn = sn->next_sibling("Skill")) {
    member.skills.items.push_back(
        std::string_view(sn->value(), sn->value_size()));
  }
}

template <int Flags>
static void rx_run_org(benchmark::State& state) {
  for (auto _ : state) {
    auto buf = rx_buffer(kOrgXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    Organization org;
    auto* org_node = doc.first_node("Organization");
    org.id = rx_attr_int(org_node, "id");
    org.name = rx_attr_sv(org_node, "name");
    for (auto* dn = org_node->first_node("Department"); dn;
         dn = dn->next_sibling("Department")) {
      OrgDepartment dept;
      dept.id = rx_attr_int(dn, "id");
      dept.name = rx_attr_sv(dn, "name");
      for (auto* tn = dn->first_node("Team"); tn;
           tn = tn->next_sibling("Team")) {
        OrgTeam team;
        team.id = rx_attr_int(tn, "id");
        team.name = rx_attr_sv(tn, "name");
        for (auto* mn = tn->first_node("Member"); mn;
             mn = mn->next_sibling("Member")) {
          OrgMember member;
          rx_build_org_member(member, mn);
          team.members.push_back(member);
        }
        dept.teams.push_back(team);
      }
      org.departments.push_back(dept);
    }
    benchmark::DoNotOptimize(org);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kOrgXml.size()));
}

static void rx_build_tree(TreeNode& node, RxNode* xn) {
  for (auto* child = xn->first_node("Node"); child;
       child = child->next_sibling("Node")) {
    node.children.emplace_back();
    rx_build_tree(node.children.back(), child);
  }
}

template <int Flags>
static void rx_run_tree(benchmark::State& state) {
  for (auto _ : state) {
    auto buf = rx_buffer(kTreeXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    TreeNode root;
    rx_build_tree(root, doc.first_node("Node"));
    benchmark::DoNotOptimize(root);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kTreeXml.size()));
}

template <int Flags>
static void rx_run_catalog(benchmark::State& state) {
  for (auto _ : state) {
    auto buf = rx_buffer(kCatalogXml);
    rx::xml_document<> doc;
    doc.parse<Flags>(buf.data());
    Catalog catalog;
    auto* root = doc.first_node("catalog");
    for (auto* node = root->first_node("book"); node;
         node = node->next_sibling("book")) {
      Book& b = catalog.books.emplace_back();
      b.id = std::string(rx_attr_sv(node, "id"));
      b.author = std::string(rx_child_sv(node, "author"));
      b.title = std::string(rx_child_sv(node, "title"));
      b.genre = std::string(rx_child_sv(node, "genre"));
      b.price = std::string(rx_child_sv(node, "price"));
      b.publish_date = std::string(rx_child_sv(node, "publish_date"));
      b.description = std::string(rx_child_sv(node, "description"));
    }
    benchmark::DoNotOptimize(catalog);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCatalogXml.size()));
}

// parse_default (full tree + entity decoding)
static void BM_RapidXml_ParseFlatXml(benchmark::State& s) { rx_run_flat<0>(s); }
static void BM_RapidXml_ParseDeepXml(benchmark::State& s) { rx_run_deep<0>(s); }
static void BM_RapidXml_ParseAttrXml(benchmark::State& s) { rx_run_attr<0>(s); }
static void BM_RapidXml_ParseSmallXml(benchmark::State& s) {
  rx_run_users<0>(s, kSmallXml);
}
static void BM_RapidXml_ParseLargeXml(benchmark::State& s) {
  rx_run_users<0>(s, kLargeXml);
}
static void BM_RapidXml_ParseOrgXml(benchmark::State& s) { rx_run_org<0>(s); }
static void BM_RapidXml_ParseTreeXml(benchmark::State& s) { rx_run_tree<0>(s); }
static void BM_RapidXml_ParseCommentHeavyXml(benchmark::State& s) {
  rx_run_users<0>(s, kCommentXml);
}
static void BM_RapidXml_ParseCatalog(benchmark::State& s) {
  rx_run_catalog<0>(s);
}

// parse_fastest (no data nodes / no entity translation / no terminators)
static void BM_RapidXmlFast_ParseFlatXml(benchmark::State& s) {
  rx_run_flat<rx::parse_fastest>(s);
}
static void BM_RapidXmlFast_ParseDeepXml(benchmark::State& s) {
  rx_run_deep<rx::parse_fastest>(s);
}
static void BM_RapidXmlFast_ParseAttrXml(benchmark::State& s) {
  rx_run_attr<rx::parse_fastest>(s);
}
static void BM_RapidXmlFast_ParseSmallXml(benchmark::State& s) {
  rx_run_users<rx::parse_fastest>(s, kSmallXml);
}
static void BM_RapidXmlFast_ParseLargeXml(benchmark::State& s) {
  rx_run_users<rx::parse_fastest>(s, kLargeXml);
}
static void BM_RapidXmlFast_ParseOrgXml(benchmark::State& s) {
  rx_run_org<rx::parse_fastest>(s);
}
static void BM_RapidXmlFast_ParseTreeXml(benchmark::State& s) {
  rx_run_tree<rx::parse_fastest>(s);
}
static void BM_RapidXmlFast_ParseCommentHeavyXml(benchmark::State& s) {
  rx_run_users<rx::parse_fastest>(s, kCommentXml);
}
static void BM_RapidXmlFast_ParseCatalog(benchmark::State& s) {
  rx_run_catalog<rx::parse_fastest>(s);
}

BENCHMARK(BM_RapidXml_ParseFlatXml);
BENCHMARK(BM_RapidXml_ParseDeepXml);
BENCHMARK(BM_RapidXml_ParseAttrXml);
BENCHMARK(BM_RapidXml_ParseSmallXml);
BENCHMARK(BM_RapidXml_ParseLargeXml);
BENCHMARK(BM_RapidXml_ParseOrgXml);
BENCHMARK(BM_RapidXml_ParseTreeXml);
BENCHMARK(BM_RapidXml_ParseCommentHeavyXml);
BENCHMARK(BM_RapidXml_ParseCatalog);

BENCHMARK(BM_RapidXmlFast_ParseFlatXml);
BENCHMARK(BM_RapidXmlFast_ParseDeepXml);
BENCHMARK(BM_RapidXmlFast_ParseAttrXml);
BENCHMARK(BM_RapidXmlFast_ParseSmallXml);
BENCHMARK(BM_RapidXmlFast_ParseLargeXml);
BENCHMARK(BM_RapidXmlFast_ParseOrgXml);
BENCHMARK(BM_RapidXmlFast_ParseTreeXml);
BENCHMARK(BM_RapidXmlFast_ParseCommentHeavyXml);
BENCHMARK(BM_RapidXmlFast_ParseCatalog);
#endif  // TURBOXML_HAS_RAPIDXML

#ifdef TURBOXML_HAS_LIBXML2
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlreader.h>

// Find the first element child of `parent` named `name` (nullptr if none).
static auto x2_child(xmlNode* parent, const char* name) -> xmlNode* {
  for (xmlNode* c = parent->children; c != nullptr; c = c->next) {
    if (c->type == XML_ELEMENT_NODE && xmlStrcmp(c->name, BAD_CAST name) == 0) {
      return c;
    }
  }
  return nullptr;
}

// Text content of an element, as a pointer into the tree (no extra allocation),
// matching pugixml's child_value(). Returns the first text/CDATA child.
static auto x2_text(xmlNode* n) -> const char* {
  if (n == nullptr) return "";
  for (xmlNode* c = n->children; c != nullptr; c = c->next) {
    if (c->type == XML_TEXT_NODE || c->type == XML_CDATA_SECTION_NODE) {
      return reinterpret_cast<const char*>(c->content);
    }
  }
  return "";
}

// Attribute value as a pointer into the tree (no extra allocation), unlike
// xmlGetProp which copies and must be freed.
static auto x2_attr(xmlNode* n, const char* name) -> const char* {
  for (xmlAttr* a = n->properties; a != nullptr; a = a->next) {
    if (xmlStrcmp(a->name, BAD_CAST name) == 0) {
      return a->children != nullptr
                 ? reinterpret_cast<const char*>(a->children->content)
                 : "";
    }
  }
  return "";
}

static auto x2_text_int(xmlNode* n) -> int {
  const char* t = x2_text(n);
  int out{};
  std::from_chars(t, t + std::strlen(t), out);
  return out;
}

static auto x2_attr_int(xmlNode* n, const char* name) -> int {
  const char* t = x2_attr(n, name);
  int out{};
  std::from_chars(t, t + std::strlen(t), out);
  return out;
}

static void BM_LibXml2_ParseSmallXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kSmallXml.data(), static_cast<int>(kSmallXml.size()),
                      "bench.xml", nullptr, 0);
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* u = root->children; u != nullptr; u = u->next) {
      if (u->type != XML_ELEMENT_NODE || xmlStrcmp(u->name, BAD_CAST "User"))
        continue;
      ids.push_back(x2_attr_int(u, "id"));
      names.push_back(x2_text(x2_child(u, "Name")));
      emails.push_back(x2_text(x2_child(u, "Email")));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kSmallXml.size()));
}

static void BM_LibXml2_ParseLargeXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kLargeXml.data(), static_cast<int>(kLargeXml.size()),
                      "bench.xml", nullptr, 0);
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* u = root->children; u != nullptr; u = u->next) {
      if (u->type != XML_ELEMENT_NODE || xmlStrcmp(u->name, BAD_CAST "User"))
        continue;
      ids.push_back(x2_attr_int(u, "id"));
      names.push_back(x2_text(x2_child(u, "Name")));
      emails.push_back(x2_text(x2_child(u, "Email")));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kLargeXml.size()));
}

static void BM_LibXml2_ParseFlatXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kFlatXml.data(), static_cast<int>(kFlatXml.size()),
                      "bench.xml", nullptr, 0);
    FlatList list;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* item = root->children; item != nullptr; item = item->next) {
      if (item->type != XML_ELEMENT_NODE ||
          xmlStrcmp(item->name, BAD_CAST "Item"))
        continue;
      FlatItem it;
      it.id = x2_attr_int(item, "id");
      it.title = x2_text(x2_child(item, "title"));
      it.description = x2_text(x2_child(item, "desc"));
      it.status = x2_text_int(x2_child(item, "status"));
      list.items.push_back(it);
    }
    benchmark::DoNotOptimize(list);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

static void BM_LibXml2_ParseDeepXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kDeepXml.data(), static_cast<int>(kDeepXml.size()),
                      "bench.xml", nullptr, 0);
    std::vector<int> values;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* l1 = root->children; l1 != nullptr; l1 = l1->next) {
      if (l1->type != XML_ELEMENT_NODE || xmlStrcmp(l1->name, BAD_CAST "L1"))
        continue;
      xmlNode* l5 =
          x2_child(x2_child(x2_child(x2_child(l1, "L2"), "L3"), "L4"), "L5");
      values.push_back(x2_text_int(x2_child(l5, "v")));
    }
    benchmark::DoNotOptimize(values);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kDeepXml.size()));
}

static void BM_LibXml2_ParseAttrXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kAttrXml.data(), static_cast<int>(kAttrXml.size()),
                      "bench.xml", nullptr, 0);
    std::vector<int> a1s, a2s, a3s, a4s, a5s;
    std::vector<std::string_view> s1s, s2s, s3s, s4s, s5s;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* item = root->children; item != nullptr; item = item->next) {
      if (item->type != XML_ELEMENT_NODE ||
          xmlStrcmp(item->name, BAD_CAST "Item"))
        continue;
      a1s.push_back(x2_attr_int(item, "a1"));
      a2s.push_back(x2_attr_int(item, "a2"));
      a3s.push_back(x2_attr_int(item, "a3"));
      a4s.push_back(x2_attr_int(item, "a4"));
      a5s.push_back(x2_attr_int(item, "a5"));
      s1s.push_back(x2_attr(item, "s1"));
      s2s.push_back(x2_attr(item, "s2"));
      s3s.push_back(x2_attr(item, "s3"));
      s4s.push_back(x2_attr(item, "s4"));
      s5s.push_back(x2_attr(item, "s5"));
    }
    benchmark::DoNotOptimize(a1s);
    benchmark::DoNotOptimize(s1s);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kAttrXml.size()));
}

static void x2_build_org_member(OrgMember& member, xmlNode* mn) {
  member.id = x2_attr_int(mn, "id");
  member.role = x2_attr(mn, "role");
  member.full_name = x2_text(x2_child(mn, "FullName"));
  member.email = x2_text(x2_child(mn, "Email"));
  xmlNode* skills = x2_child(mn, "Skills");
  for (xmlNode* sn = skills->children; sn != nullptr; sn = sn->next) {
    if (sn->type == XML_ELEMENT_NODE &&
        xmlStrcmp(sn->name, BAD_CAST "Skill") == 0)
      member.skills.items.push_back(x2_text(sn));
  }
}

static void BM_LibXml2_ParseOrgXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kOrgXml.data(), static_cast<int>(kOrgXml.size()),
                      "bench.xml", nullptr, 0);
    Organization org;
    xmlNode* org_node = xmlDocGetRootElement(doc);
    org.id = x2_attr_int(org_node, "id");
    org.name = x2_attr(org_node, "name");
    for (xmlNode* dn = org_node->children; dn != nullptr; dn = dn->next) {
      if (dn->type != XML_ELEMENT_NODE ||
          xmlStrcmp(dn->name, BAD_CAST "Department"))
        continue;
      OrgDepartment dept;
      dept.id = x2_attr_int(dn, "id");
      dept.name = x2_attr(dn, "name");
      for (xmlNode* tn = dn->children; tn != nullptr; tn = tn->next) {
        if (tn->type != XML_ELEMENT_NODE ||
            xmlStrcmp(tn->name, BAD_CAST "Team"))
          continue;
        OrgTeam team;
        team.id = x2_attr_int(tn, "id");
        team.name = x2_attr(tn, "name");
        for (xmlNode* mn = tn->children; mn != nullptr; mn = mn->next) {
          if (mn->type != XML_ELEMENT_NODE ||
              xmlStrcmp(mn->name, BAD_CAST "Member"))
            continue;
          OrgMember member;
          x2_build_org_member(member, mn);
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
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kOrgXml.size()));
}

static void x2_build_tree(TreeNode& node, xmlNode* xn) {
  for (xmlNode* c = xn->children; c != nullptr; c = c->next) {
    if (c->type == XML_ELEMENT_NODE &&
        xmlStrcmp(c->name, BAD_CAST "Node") == 0) {
      node.children.emplace_back();
      x2_build_tree(node.children.back(), c);
    }
  }
}

static void BM_LibXml2_ParseTreeXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kTreeXml.data(), static_cast<int>(kTreeXml.size()),
                      "bench.xml", nullptr, 0);
    TreeNode root;
    x2_build_tree(root, xmlDocGetRootElement(doc));
    benchmark::DoNotOptimize(root);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kTreeXml.size()));
}

static void BM_LibXml2_ParseCommentHeavyXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kCommentXml.data(), static_cast<int>(kCommentXml.size()),
                      "bench.xml", nullptr, 0);
    std::vector<int> ids;
    std::vector<std::string_view> names, emails;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* u = root->children; u != nullptr; u = u->next) {
      if (u->type != XML_ELEMENT_NODE || xmlStrcmp(u->name, BAD_CAST "User"))
        continue;
      ids.push_back(x2_attr_int(u, "id"));
      names.push_back(x2_text(x2_child(u, "Name")));
      emails.push_back(x2_text(x2_child(u, "Email")));
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCommentXml.size()));
}

static void BM_LibXml2_ParseCatalog(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlDoc* doc =
        xmlReadMemory(kCatalogXml.data(), static_cast<int>(kCatalogXml.size()),
                      "bench.xml", nullptr, 0);
    Catalog catalog;
    xmlNode* root = xmlDocGetRootElement(doc);
    for (xmlNode* node = root->children; node != nullptr; node = node->next) {
      if (node->type != XML_ELEMENT_NODE ||
          xmlStrcmp(node->name, BAD_CAST "book"))
        continue;
      Book& b = catalog.books.emplace_back();
      b.id = x2_attr(node, "id");
      b.author = x2_text(x2_child(node, "author"));
      b.title = x2_text(x2_child(node, "title"));
      b.genre = x2_text(x2_child(node, "genre"));
      b.price = x2_text(x2_child(node, "price"));
      b.publish_date = x2_text(x2_child(node, "publish_date"));
      b.description = x2_text(x2_child(node, "description"));
    }
    benchmark::DoNotOptimize(catalog);
    xmlFreeDoc(doc);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCatalogXml.size()));
}

BENCHMARK(BM_LibXml2_ParseFlatXml);
BENCHMARK(BM_LibXml2_ParseDeepXml);
BENCHMARK(BM_LibXml2_ParseAttrXml);
BENCHMARK(BM_LibXml2_ParseSmallXml);
BENCHMARK(BM_LibXml2_ParseLargeXml);
BENCHMARK(BM_LibXml2_ParseOrgXml);
BENCHMARK(BM_LibXml2_ParseTreeXml);
BENCHMARK(BM_LibXml2_ParseCommentHeavyXml);
BENCHMARK(BM_LibXml2_ParseCatalog);

// ---- libxml2 streaming reader (xmlTextReader) ----
//
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

auto sr_name_is(xmlTextReaderPtr r, const char* name) -> bool {
  return xmlStrcmp(xmlTextReaderConstName(r), BAD_CAST name) == 0;
}

auto sr_attr_str(xmlTextReaderPtr r, const char* name) -> std::string {
  xmlChar* v = xmlTextReaderGetAttribute(r, BAD_CAST name);
  std::string out = (v != nullptr) ? reinterpret_cast<const char*>(v) : "";
  if (v != nullptr) xmlFree(v);
  return out;
}

auto sr_attr_int(xmlTextReaderPtr r, const char* name) -> int {
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
auto sr_text(xmlTextReaderPtr r) -> std::string {
  if (xmlTextReaderIsEmptyElement(r)) return {};
  std::string out;
  const int depth = xmlTextReaderDepth(r);
  while (xmlTextReaderRead(r) == 1) {
    const int t = xmlTextReaderNodeType(r);
    if (t == kEndElem && xmlTextReaderDepth(r) == depth) break;
    if (t == kText || t == kCData) {
      const xmlChar* v = xmlTextReaderConstValue(r);
      if (v != nullptr) out += reinterpret_cast<const char*>(v);
    }
  }
  return out;
}

auto sr_text_int(xmlTextReaderPtr r) -> int {
  const std::string s = sr_text(r);
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

auto sr_open(const std::string& src) -> xmlTextReaderPtr {
  return xmlReaderForMemory(src.data(), static_cast<int>(src.size()),
                            "bench.xml", nullptr, 0);
}

}  // namespace

static void sr_run_users(benchmark::State& state, const std::string& src) {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = sr_open(src);
    std::vector<int> ids;
    std::vector<std::string> names, emails;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem) continue;
      if (sr_name_is(r, "User")) {
        ids.push_back(sr_attr_int(r, "id"));
      } else if (sr_name_is(r, "Name")) {
        names.push_back(sr_text(r));
      } else if (sr_name_is(r, "Email")) {
        emails.push_back(sr_text(r));
      }
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(names);
    benchmark::DoNotOptimize(emails);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(src.size()));
}

static void BM_LibXml2Reader_ParseSmallXml(benchmark::State& state) {
  sr_run_users(state, kSmallXml);
}
static void BM_LibXml2Reader_ParseLargeXml(benchmark::State& state) {
  sr_run_users(state, kLargeXml);
}
static void BM_LibXml2Reader_ParseCommentHeavyXml(benchmark::State& state) {
  sr_run_users(state, kCommentXml);
}

static void BM_LibXml2Reader_ParseFlatXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = sr_open(kFlatXml);
    std::vector<int> ids, statuses;
    std::vector<std::string> titles, descs;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem) continue;
      if (sr_name_is(r, "Item")) {
        ids.push_back(sr_attr_int(r, "id"));
      } else if (sr_name_is(r, "title")) {
        titles.push_back(sr_text(r));
      } else if (sr_name_is(r, "desc")) {
        descs.push_back(sr_text(r));
      } else if (sr_name_is(r, "status")) {
        statuses.push_back(sr_text_int(r));
      }
    }
    benchmark::DoNotOptimize(ids);
    benchmark::DoNotOptimize(titles);
    benchmark::DoNotOptimize(descs);
    benchmark::DoNotOptimize(statuses);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kFlatXml.size()));
}

static void BM_LibXml2Reader_ParseDeepXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = sr_open(kDeepXml);
    std::vector<int> values;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) == kElem && sr_name_is(r, "v"))
        values.push_back(sr_text_int(r));
    }
    benchmark::DoNotOptimize(values);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kDeepXml.size()));
}

static void BM_LibXml2Reader_ParseAttrXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = sr_open(kAttrXml);
    std::vector<int> a1s, a2s, a3s, a4s, a5s;
    std::vector<std::string> s1s, s2s, s3s, s4s, s5s;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem || !sr_name_is(r, "Item")) continue;
      a1s.push_back(sr_attr_int(r, "a1"));
      a2s.push_back(sr_attr_int(r, "a2"));
      a3s.push_back(sr_attr_int(r, "a3"));
      a4s.push_back(sr_attr_int(r, "a4"));
      a5s.push_back(sr_attr_int(r, "a5"));
      s1s.push_back(sr_attr_str(r, "s1"));
      s2s.push_back(sr_attr_str(r, "s2"));
      s3s.push_back(sr_attr_str(r, "s3"));
      s4s.push_back(sr_attr_str(r, "s4"));
      s5s.push_back(sr_attr_str(r, "s5"));
    }
    benchmark::DoNotOptimize(a1s);
    benchmark::DoNotOptimize(s1s);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kAttrXml.size()));
}

// Recursive-descent reconstruction of a <Member> subtree from the stream.
static void sr_read_member(xmlTextReaderPtr r, SrMember& m) {
  m.id = sr_attr_int(r, "id");
  m.role = sr_attr_str(r, "role");
  if (xmlTextReaderIsEmptyElement(r)) return;
  const int depth = xmlTextReaderDepth(r);
  while (xmlTextReaderRead(r) == 1) {
    const int t = xmlTextReaderNodeType(r);
    if (t == kEndElem && xmlTextReaderDepth(r) == depth) break;
    if (t != kElem) continue;
    if (sr_name_is(r, "FullName")) {
      m.full_name = sr_text(r);
    } else if (sr_name_is(r, "Email")) {
      m.email = sr_text(r);
    } else if (sr_name_is(r, "Skills")) {
      if (xmlTextReaderIsEmptyElement(r)) continue;
      const int sdepth = xmlTextReaderDepth(r);
      while (xmlTextReaderRead(r) == 1) {
        const int st = xmlTextReaderNodeType(r);
        if (st == kEndElem && xmlTextReaderDepth(r) == sdepth) break;
        if (st == kElem && sr_name_is(r, "Skill"))
          m.skills.push_back(sr_text(r));
      }
    }
  }
}

static void BM_LibXml2Reader_ParseOrgXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = sr_open(kOrgXml);
    SrOrg org;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem) continue;
      if (sr_name_is(r, "Organization")) {
        org.id = sr_attr_int(r, "id");
        org.name = sr_attr_str(r, "name");
      } else if (sr_name_is(r, "Department")) {
        SrDept& d = org.depts.emplace_back();
        d.id = sr_attr_int(r, "id");
        d.name = sr_attr_str(r, "name");
      } else if (sr_name_is(r, "Team")) {
        SrTeam& t = org.depts.back().teams.emplace_back();
        t.id = sr_attr_int(r, "id");
        t.name = sr_attr_str(r, "name");
      } else if (sr_name_is(r, "Member")) {
        SrMember& m = org.depts.back().teams.back().members.emplace_back();
        sr_read_member(r, m);
      }
    }
    benchmark::DoNotOptimize(org);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kOrgXml.size()));
}

static void BM_LibXml2Reader_ParseTreeXml(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = sr_open(kTreeXml);
    TreeNode root;
    std::vector<TreeNode*> stack{&root};
    while (xmlTextReaderRead(r) == 1) {
      const int t = xmlTextReaderNodeType(r);
      if (t == kElem && sr_name_is(r, "Node")) {
        TreeNode& child = stack.back()->children.emplace_back();
        if (!xmlTextReaderIsEmptyElement(r)) stack.push_back(&child);
      } else if (t == kEndElem && sr_name_is(r, "Node")) {
        stack.pop_back();
      }
    }
    benchmark::DoNotOptimize(root);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kTreeXml.size()));
}

static void BM_LibXml2Reader_ParseCatalog(benchmark::State& state) {
  xmlInitParser();
  for (auto _ : state) {
    xmlTextReaderPtr r = sr_open(kCatalogXml);
    Catalog catalog;
    Book* cur = nullptr;
    while (xmlTextReaderRead(r) == 1) {
      if (xmlTextReaderNodeType(r) != kElem) continue;
      if (sr_name_is(r, "book")) {
        cur = &catalog.books.emplace_back();
        cur->id = sr_attr_str(r, "id");
      } else if (cur != nullptr) {
        if (sr_name_is(r, "author"))
          cur->author = sr_text(r);
        else if (sr_name_is(r, "title"))
          cur->title = sr_text(r);
        else if (sr_name_is(r, "genre"))
          cur->genre = sr_text(r);
        else if (sr_name_is(r, "price"))
          cur->price = sr_text(r);
        else if (sr_name_is(r, "publish_date"))
          cur->publish_date = sr_text(r);
        else if (sr_name_is(r, "description"))
          cur->description = sr_text(r);
      }
    }
    benchmark::DoNotOptimize(catalog);
    xmlFreeTextReader(r);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(state.iterations() *
                          static_cast<int64_t>(kCatalogXml.size()));
}

BENCHMARK(BM_LibXml2Reader_ParseFlatXml);
BENCHMARK(BM_LibXml2Reader_ParseDeepXml);
BENCHMARK(BM_LibXml2Reader_ParseAttrXml);
BENCHMARK(BM_LibXml2Reader_ParseSmallXml);
BENCHMARK(BM_LibXml2Reader_ParseLargeXml);
BENCHMARK(BM_LibXml2Reader_ParseOrgXml);
BENCHMARK(BM_LibXml2Reader_ParseTreeXml);
BENCHMARK(BM_LibXml2Reader_ParseCommentHeavyXml);
BENCHMARK(BM_LibXml2Reader_ParseCatalog);
#endif  // TURBOXML_HAS_LIBXML2

BENCHMARK_MAIN();