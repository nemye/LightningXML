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

// 3a: xs:complexContent extension -> struct inheritance + merged metadata.
TEST(XsdCodegen, ComplexContentExtension) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Person">
      <xs:sequence>
        <xs:element name="name" type="xs:string"/>
      </xs:sequence>
      <xs:attribute name="id" type="xs:int" use="required"/>
    </xs:complexType>
    <xs:complexType name="Employee">
      <xs:complexContent>
        <xs:extension base="Person">
          <xs:sequence>
            <xs:element name="department" type="xs:string"/>
          </xs:sequence>
          <xs:attribute name="empId" type="xs:int"/>
        </xs:extension>
      </xs:complexContent>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "struct Employee : Person")) << r.code;
  // Inherited members must NOT be re-declared in Employee's struct body.
  const auto emp_def = r.code.find("struct Employee :");
  ASSERT_NE(emp_def, std::string::npos) << r.code;
  const auto emp_end = r.code.find("\n};", emp_def);
  const std::string emp_body = r.code.substr(emp_def, emp_end - emp_def);
  EXPECT_EQ(emp_body.find("std::string name"), std::string::npos) << emp_body;
  // But metadata MUST reference them via &Employee::.
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("id", &Employee::id, true))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::field("name", &Employee::name, true))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::field("department", &Employee::department, true))")) << r.code;
}

// 3b: xs:attributeGroup inline expansion.
TEST(XsdCodegen, AttributeGroupExpansion) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:attributeGroup name="CommonAttrs">
      <xs:attribute name="id" type="xs:int" use="required"/>
      <xs:attribute name="lang" type="xs:string"/>
    </xs:attributeGroup>
    <xs:complexType name="Widget">
      <xs:attributeGroup ref="CommonAttrs"/>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "struct Widget")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("id", &Widget::id, true))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("lang", &Widget::lang))")) << r.code;
}

// 3b: xs:group inline expansion.
TEST(XsdCodegen, ElementGroupExpansion) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:group name="CommonFields">
      <xs:sequence>
        <xs:element name="title" type="xs:string"/>
        <xs:element name="desc" type="xs:string" minOccurs="0"/>
      </xs:sequence>
    </xs:group>
    <xs:complexType name="Doc">
      <xs:sequence>
        <xs:group ref="CommonFields"/>
        <xs:element name="body" type="xs:string"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "struct Doc")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::field("title", &Doc::title, true))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::field("desc", &Doc::desc))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::field("body", &Doc::body, true))")) << r.code;
}

// 3c: xs:include merges types from an external schema via loader callback.
TEST(XsdCodegen, SchemaIncludeLoader) {
  constexpr std::string_view base_xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:include schemaLocation="common.xsd"/>
    <xs:complexType name="Widget">
      <xs:sequence>
        <xs:element name="name" type="xs:string"/>
      </xs:sequence>
      <xs:attribute name="color" type="Color"/>
    </xs:complexType>
  </xs:schema>)";
  constexpr std::string_view common_xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="Color">
      <xs:restriction base="xs:string">
        <xs:enumeration value="red"/>
        <xs:enumeration value="blue"/>
      </xs:restriction>
    </xs:simpleType>
  </xs:schema>)";

  xsd::Options opts;
  opts.loader = [&](std::string_view loc) -> std::optional<std::string> {
    if (loc == "common.xsd") return std::string{common_xsd};
    return {};
  };
  const auto r = xsd::generate(base_xsd, opts);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "enum class Color")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("color", &Widget::color))")) << r.code;
}

// 4a: finite maxOccurs -> std::vector member + XmlConstraints size check.
TEST(XsdCodegen, FiniteMaxOccursConstraint) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Doc">
      <xs:sequence>
        <xs:element name="tag" type="xs:string" maxOccurs="3"/>
        <xs:element name="note" type="xs:string" maxOccurs="unbounded"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  // Both bounded and unbounded become std::vector.
  EXPECT_TRUE(has(r.code, "std::vector<std::string> tag;")) << r.code;
  EXPECT_TRUE(has(r.code, "std::vector<std::string> note;")) << r.code;
  // Only the bounded one gets a constraint check.
  EXPECT_TRUE(has(r.code, "XmlConstraints<Doc>")) << r.code;
  EXPECT_TRUE(has(r.code, "tag.size() > 3")) << r.code;
  EXPECT_FALSE(has(r.code, "note.size()")) << r.code;
}

// 4b: attribute default -> C++ default member initializer.
TEST(XsdCodegen, AttributeDefaultInitializer) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Price">
      <xs:attribute name="currency" type="xs:string" default="USD"/>
      <xs:attribute name="precision" type="xs:int" default="2"/>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, R"(std::string currency{"USD"};)")) << r.code;
  EXPECT_TRUE(has(r.code, "int precision{2};")) << r.code;
}

// 4c: attribute fixed -> initializer + XmlConstraints equality check.
TEST(XsdCodegen, AttributeFixedConstraint) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Hdr">
      <xs:attribute name="version" type="xs:string" fixed="1.0"/>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  // Member should be pre-initialised to the fixed value.
  EXPECT_TRUE(has(r.code, R"(std::string version{"1.0"};)")) << r.code;
  // XmlConstraints must enforce it.
  EXPECT_TRUE(has(r.code, "XmlConstraints<Hdr>")) << r.code;
  EXPECT_TRUE(has(r.code, R"(version != "1.0")")) << r.code;
}

// xs:choice with maxOccurs="unbounded" -> std::vector<std::variant<...>>.
TEST(XsdCodegen, RepeatedChoiceBecomesVectorVariant) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="A"><xs:attribute name="a" type="xs:int"/></xs:complexType>
    <xs:complexType name="B"><xs:attribute name="b" type="xs:int"/></xs:complexType>
    <xs:complexType name="Mixed">
      <xs:choice maxOccurs="unbounded">
        <xs:element name="a" type="A"/>
        <xs:element name="b" type="B"/>
      </xs:choice>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "std::vector<std::variant<A, B>> choice;")) << r.code;
  EXPECT_TRUE(has(r.code, "xml::variant_field(&Mixed::choice")) << r.code;
}

// xs:complexContent extension across three levels: all levels appear in
// the deepest child's XmlMetadata, none are re-declared in its struct body.
TEST(XsdCodegen, MultiLevelComplexContentExtension) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Animal">
      <xs:attribute name="name" type="xs:string" use="required"/>
    </xs:complexType>
    <xs:complexType name="Pet">
      <xs:complexContent>
        <xs:extension base="Animal">
          <xs:attribute name="owner" type="xs:string"/>
        </xs:extension>
      </xs:complexContent>
    </xs:complexType>
    <xs:complexType name="Dog">
      <xs:complexContent>
        <xs:extension base="Pet">
          <xs:attribute name="breed" type="xs:string"/>
        </xs:extension>
      </xs:complexContent>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "struct Pet : Animal")) << r.code;
  EXPECT_TRUE(has(r.code, "struct Dog : Pet")) << r.code;
  // Dog's metadata must cover all three levels.
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("name", &Dog::name, true))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("owner", &Dog::owner))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::attr_field("breed", &Dog::breed))")) << r.code;
  // Dog's struct body must only declare breed.
  const auto dog_def = r.code.find("struct Dog :");
  ASSERT_NE(dog_def, std::string::npos) << r.code;
  const auto dog_end = r.code.find("\n};", dog_def);
  const std::string dog_body = r.code.substr(dog_def, dog_end - dog_def);
  EXPECT_EQ(dog_body.find("std::string name"), std::string::npos) << dog_body;
  EXPECT_EQ(dog_body.find("std::string owner"), std::string::npos) << dog_body;
  EXPECT_NE(dog_body.find("std::string breed"), std::string::npos) << dog_body;
}

// minExclusive / maxExclusive emit <= / >= comparisons (strict bounds).
TEST(XsdCodegen, ExclusiveRangeFacetsGenerateConstraints) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="UnitInterval">
      <xs:restriction base="xs:double">
        <xs:minExclusive value="0"/>
        <xs:maxExclusive value="1"/>
      </xs:restriction>
    </xs:simpleType>
    <xs:complexType name="Prob">
      <xs:sequence>
        <xs:element name="p" type="UnitInterval"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "XmlConstraints<Prob>")) << r.code;
  EXPECT_TRUE(has(r.code, "v.p <= 0")) << r.code;
  EXPECT_TRUE(has(r.code, "v.p >= 1")) << r.code;
}

// Attribute whose type is a named simpleType with facets generates a constraint.
TEST(XsdCodegen, AttributeFacetConstraint) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="ShortCode">
      <xs:restriction base="xs:string">
        <xs:maxLength value="5"/>
      </xs:restriction>
    </xs:simpleType>
    <xs:complexType name="Tag">
      <xs:attribute name="code" type="ShortCode"/>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "XmlConstraints<Tag>")) << r.code;
  EXPECT_TRUE(has(r.code, "code.size() > 5")) << r.code;
}

// Inline simpleType with enumerations on an element generates an enum class
// scoped to that element name.
TEST(XsdCodegen, InlineEnumOnElement) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Traffic">
      <xs:sequence>
        <xs:element name="signal">
          <xs:simpleType>
            <xs:restriction base="xs:string">
              <xs:enumeration value="red"/>
              <xs:enumeration value="amber"/>
              <xs:enumeration value="green"/>
            </xs:restriction>
          </xs:simpleType>
        </xs:element>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "enum class Signal")) << r.code;
  EXPECT_TRUE(has(r.code, "Signal signal{};")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::field("signal", &Traffic::signal, true))")) << r.code;
}

// A top-level xs:element emits a root-deserialize comment in the output.
TEST(XsdCodegen, TopLevelElementGeneratesRootComment) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Item">
      <xs:attribute name="id" type="xs:int"/>
    </xs:complexType>
    <xs:element name="item" type="Item"/>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, R"(// root: xml::deserialize(parser, "item", obj);)")) << r.code;
}

// simpleContent whose base is a constrained simpleType generates a value constraint.
TEST(XsdCodegen, SimpleContentConstraint) {
  constexpr std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="PosDouble">
      <xs:restriction base="xs:double">
        <xs:minInclusive value="0"/>
      </xs:restriction>
    </xs:simpleType>
    <xs:complexType name="Amount">
      <xs:simpleContent>
        <xs:extension base="PosDouble">
          <xs:attribute name="ccy" type="xs:string"/>
        </xs:extension>
      </xs:simpleContent>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "XmlConstraints<Amount>")) << r.code;
  EXPECT_TRUE(has(r.code, "v.value < 0")) << r.code;
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
