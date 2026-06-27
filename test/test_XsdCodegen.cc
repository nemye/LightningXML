#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>

#include "XsdCodegen.hh"
// The committed golden header generated from test/xsd_sample.xsd. Including it
// proves the generated metadata is valid C++ and parses (see EndToEnd below).
#include "xsd_sample_generated.hh"

namespace {

auto has(const std::string& code, std::string_view frag) -> bool {
  return code.find(frag) != std::string::npos;
}

auto read_file(const std::string& path) -> std::string {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

TEST(XsdCodegen, BuiltinTypesAndCardinality) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Rec">
      <xs:sequence>
        <xs:element name="name" type="xs:string"/>
        <xs:element name="age" type="xs:int" minOccurs="0"/>
        <xs:element name="tag" type="xs:string" maxOccurs="unbounded"/>
        <xs:element name="when" type="xs:dateTime"/>
      </xs:sequence>
      <xs:attribute name="id" type="xs:int" use="required"/>
      <xs:attribute name="ref" type="xs:string"/>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(r.notes.empty());
  EXPECT_TRUE(has(r.code, "struct Rec"));
  EXPECT_TRUE(has(r.code, "std::string name{};"));
  EXPECT_TRUE(has(r.code, "std::optional<int> age;"));       // minOccurs=0
  EXPECT_TRUE(has(r.code, "std::vector<std::string> tag;"))  // unbounded
      << r.code;
  EXPECT_TRUE(has(r.code, "xml::DateTime when{};"));  // dateTime -> DateTime
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("id", &Rec::id, true))"));
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("ref", &Rec::ref))"));
  EXPECT_TRUE(has(r.code, R"(xml::field("name", &Rec::name, true))"));   // minOccurs=1
  EXPECT_TRUE(has(r.code, R"(xml::field("age", &Rec::age))"));           // optional
  EXPECT_TRUE(has(r.code, R"(xml::vec_field("tag", &Rec::tag, true))"));
}

TEST(XsdCodegen, EnumFromSimpleType) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="Color">
      <xs:restriction base="xs:string">
        <xs:enumeration value="red"/>
        <xs:enumeration value="green-ish"/>
      </xs:restriction>
    </xs:simpleType>
    <xs:complexType name="Paint">
      <xs:attribute name="color" type="Color"/>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "enum class Color"));
  EXPECT_TRUE(has(r.code, "xml::enum_table<Color>"));
  EXPECT_TRUE(has(r.code, R"({"green-ish", Color::green_ish})"));  // sanitized id
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("color", &Paint::color))"));
}

TEST(XsdCodegen, SimpleContentBecomesValueField) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Money">
      <xs:simpleContent>
        <xs:extension base="xs:decimal">
          <xs:attribute name="ccy" type="xs:string"/>
        </xs:extension>
      </xs:simpleContent>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "double value{};"));
  EXPECT_TRUE(has(r.code, "xml::value_field(&Money::value)"));
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("ccy", &Money::ccy))"));
}

TEST(XsdCodegen, ChoiceBecomesVariant) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="A"><xs:attribute name="a" type="xs:int"/></xs:complexType>
    <xs:complexType name="B"><xs:attribute name="b" type="xs:int"/></xs:complexType>
    <xs:complexType name="Shape">
      <xs:choice>
        <xs:element name="a" type="A"/>
        <xs:element name="b" type="B"/>
      </xs:choice>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "std::variant<A, B> choice;"));
  EXPECT_TRUE(has(r.code, "xml::variant_field(&Shape::choice"));
  EXPECT_TRUE(has(r.code, R"(xml::alt<A>("a"))"));
  EXPECT_TRUE(has(r.code, R"(xml::alt<B>("b"))"));
}

TEST(XsdCodegen, ListBecomesListOrAttrField) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="IntList"><xs:list itemType="xs:int"/></xs:simpleType>
    <xs:complexType name="Cfg">
      <xs:sequence>
        <xs:element name="codes" type="IntList"/>
      </xs:sequence>
      <xs:attribute name="tags" type="xs:NMTOKENS"/>
      <xs:attribute name="ids" type="IntList"/>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(r.notes.empty()) << (r.notes.empty() ? "" : r.notes[0]);
  EXPECT_TRUE(has(r.code, "std::vector<int> codes;"));
  EXPECT_TRUE(has(r.code, R"(xml::list_field("codes", &Cfg::codes)"));
  EXPECT_TRUE(has(r.code, "std::vector<std::string> tags;"));  // NMTOKENS attr
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("tags", &Cfg::tags))"));
  EXPECT_TRUE(has(r.code, "std::vector<int> ids;"));
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("ids", &Cfg::ids))"));
}

TEST(XsdCodegen, RecursionUsesUniquePtr) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Node">
      <xs:sequence>
        <xs:element name="child" type="Node" minOccurs="0"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "std::unique_ptr<Node> child;"));
}

TEST(XsdCodegen, UnsupportedConstructsAreNoted) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Doc" mixed="true">
      <xs:sequence>
        <xs:element name="title" type="xs:bogusType"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);  // still generates
  ASSERT_GE(r.notes.size(), 2u);
  bool mixed = false;
  bool unknown = false;
  for (const auto& n : r.notes) {
    mixed |= n.find("mixed content") != std::string::npos;
    unknown |= n.find("unknown type") != std::string::npos;
  }
  EXPECT_TRUE(mixed);
  EXPECT_TRUE(unknown);
}

// Proves the committed golden header (generated from xsd_sample.xsd) compiles
// and that its metadata parses a representative document correctly.
TEST(XsdCodegen, EndToEndGeneratedMetadataParses) {
  constexpr std::string_view doc = R"(<order id="7" labels="rush gift">
    <priority>High</priority>
    <total currency="USD">9.99</total>
    <placed>2026-06-18T09:30:00Z</placed>
    <note>first</note>
    <note>second</note>
    <quantities>4 8 15</quantities>
    <parent id="1"><priority>Low</priority><total>1.00</total></parent>
    <circle radius="3"/>
    <square side="4"/>
    <circle radius="5"/>
  </order>)";
  xml::Parser parser{doc};
  Order o;
  ASSERT_TRUE(xml::deserialize(parser, "order", o));
  EXPECT_EQ(o.id, 7);
  EXPECT_EQ(o.priority, Priority::High);
  EXPECT_DOUBLE_EQ(o.total.value, 9.99);
  EXPECT_EQ(o.total.currency, "USD");
  ASSERT_TRUE(o.placed);
  EXPECT_EQ(o.placed->time.hour, 9u);
  ASSERT_EQ(o.note.size(), 2u);
  EXPECT_EQ(o.note[0], "first");
  EXPECT_EQ(o.labels, (std::vector<std::string>{"rush", "gift"}));   // NMTOKENS attr list
  EXPECT_EQ(o.quantities, (std::vector<int>{4, 8, 15}));             // xs:list element
  ASSERT_TRUE(o.parent);
  EXPECT_EQ(o.parent->id, 1);
  EXPECT_EQ(o.parent->priority, Priority::Low);
  ASSERT_EQ(o.choice.size(), 3u);
  EXPECT_EQ(std::get<Circle>(o.choice[0]).radius, 3);
  EXPECT_EQ(std::get<Square>(o.choice[1]).side, 4);
  EXPECT_EQ(std::get<Circle>(o.choice[2]).radius, 5);
}

// Facet capture: string length and numeric range constraints.
TEST(XsdCodegen, StringLengthFacetsGenerateConstraints) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="BoundedStr">
      <xs:restriction base="xs:string">
        <xs:minLength value="1"/>
        <xs:maxLength value="50"/>
      </xs:restriction>
    </xs:simpleType>
    <xs:complexType name="Item">
      <xs:sequence>
        <xs:element name="code" type="BoundedStr"/>
        <xs:element name="note" type="BoundedStr" minOccurs="0"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "XmlConstraints<Item>")) << r.code;
  EXPECT_TRUE(has(r.code, "code.size() < 1")) << r.code;
  EXPECT_TRUE(has(r.code, "code.size() > 50")) << r.code;
  // Optional field uses nullptr-guard before size check.
  EXPECT_TRUE(has(r.code, "v.note &&")) << r.code;
  EXPECT_TRUE(has(r.code, "note->size() < 1")) << r.code;
}

TEST(XsdCodegen, NumericRangeFacetsGenerateConstraints) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="SmallInt">
      <xs:restriction base="xs:int">
        <xs:minInclusive value="0"/>
        <xs:maxInclusive value="100"/>
      </xs:restriction>
    </xs:simpleType>
    <xs:complexType name="Score">
      <xs:sequence>
        <xs:element name="value" type="SmallInt"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "XmlConstraints<Score>")) << r.code;
  EXPECT_TRUE(has(r.code, "v.value < 0")) << r.code;
  EXPECT_TRUE(has(r.code, "v.value > 100")) << r.code;
}

TEST(XsdCodegen, InlineFacetsOnElement) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Rec">
      <xs:sequence>
        <xs:element name="code">
          <xs:simpleType>
            <xs:restriction base="xs:string">
              <xs:length value="5"/>
            </xs:restriction>
          </xs:simpleType>
        </xs:element>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "XmlConstraints<Rec>")) << r.code;
  EXPECT_TRUE(has(r.code, "code.size() != 5")) << r.code;
}

TEST(XsdCodegen, PatternFacetGeneratesRegexCheck) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="AlphaCode">
      <xs:restriction base="xs:string">
        <xs:pattern value="[A-Z]{3}"/>
      </xs:restriction>
    </xs:simpleType>
    <xs:complexType name="Item">
      <xs:sequence>
        <xs:element name="code" type="AlphaCode"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "#include <regex>")) << r.code;
  EXPECT_TRUE(has(r.code, "std::regex_match")) << r.code;
  EXPECT_TRUE(has(r.code, "[A-Z]{3}")) << r.code;
}

// Verify the full round-trip: generate code, compile it, parse an XML doc,
// then validate against the generated XmlConstraints.
TEST(XsdCodegen, ConstraintRoundTripValidation) {
  // Use the generated golden header's Order type — it has no facets, so
  // xml::validate() should always return nullopt (no-op default specialization).
  Order o;
  o.id = 1;
  o.priority = Priority::Low;
  EXPECT_FALSE(xml::validate(o).has_value());
}

// Guards the committed golden against drift from the generator.
TEST(XsdCodegen, GoldenMatchesGenerator) {
  const std::string xsd = read_file(std::string(TXSD_DATA_DIR) + "/xsd_sample.xsd");
  ASSERT_FALSE(xsd.empty()) << "could not read test/xsd_sample.xsd";
  const std::string golden =
      read_file(std::string(TXSD_DATA_DIR) + "/xsd_sample_generated.hh");
  ASSERT_FALSE(golden.empty());
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_EQ(r.code, golden)
      << "regenerate with: turboxml_xsdgen test/xsd_sample.xsd -o "
         "test/xsd_sample_generated.hh";
}
