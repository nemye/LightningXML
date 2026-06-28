#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "XSDCodegen.hh"

namespace {

auto has(const std::string& code, std::string_view frag) -> bool {
  return code.find(frag) != std::string::npos;
}

}  // namespace

TEST(XsdCodegen, BuiltinTypesAndCardinality) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  EXPECT_TRUE(has(r.code, R"(xml::attrField("id", &Rec::id, true))"));
  EXPECT_TRUE(has(r.code, R"(xml::attrField("ref", &Rec::ref))"));
  EXPECT_TRUE(has(r.code, R"(xml::field("name", &Rec::name, true))"));  // minOccurs=1
  EXPECT_TRUE(has(r.code, R"(xml::field("age", &Rec::age))"));          // optional
  EXPECT_TRUE(has(r.code, R"(xml::vecField("tag", &Rec::tag, true))"));
}

TEST(XsdCodegen, BuiltinTypeMappings) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:complexType name="Types">
      <xs:sequence>
        <xs:element name="flag" type="xs:boolean"/>
        <xs:element name="day" type="xs:date"/>
        <xs:element name="t" type="xs:time"/>
        <xs:element name="fp" type="xs:float"/>
        <xs:element name="dp" type="xs:double"/>
        <xs:element name="big" type="xs:long"/>
        <xs:element name="n" type="xs:integer"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(r.notes.empty()) << r.notes[0];
  EXPECT_TRUE(has(r.code, "bool flag{};")) << r.code;
  EXPECT_TRUE(has(r.code, "xml::Date day{};")) << r.code;
  EXPECT_TRUE(has(r.code, "xml::Time t{};")) << r.code;
  EXPECT_TRUE(has(r.code, "float fp{};")) << r.code;
  EXPECT_TRUE(has(r.code, "double dp{};")) << r.code;
  EXPECT_TRUE(has(r.code, "long big{};")) << r.code;
  EXPECT_TRUE(has(r.code, "long n{};")) << r.code;  // xs:integer -> long
}

TEST(XsdCodegen, EnumFromSimpleType) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  EXPECT_TRUE(has(r.code, "xml::enumTable<Color>"));
  EXPECT_TRUE(has(r.code, R"({"green-ish", Color::green_ish})"));  // sanitized id
  EXPECT_TRUE(has(r.code, R"(xml::attrField("color", &Paint::color))"));
}

TEST(XsdCodegen, SimpleContentBecomesValueField) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  EXPECT_TRUE(has(r.code, "xml::valueField(&Money::value)"));
  EXPECT_TRUE(has(r.code, R"(xml::attrField("ccy", &Money::ccy))"));
}

TEST(XsdCodegen, ChoiceBecomesVariant) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  EXPECT_TRUE(has(r.code, "xml::variantField(&Shape::choice"));
  EXPECT_TRUE(has(r.code, R"(xml::alt<A>("a"))"));
  EXPECT_TRUE(has(r.code, R"(xml::alt<B>("b"))"));
}

TEST(XsdCodegen, ListBecomesListOrAttrField) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  EXPECT_TRUE(has(r.code, R"(xml::listField("codes", &Cfg::codes)"));
  EXPECT_TRUE(has(r.code, "std::vector<std::string> tags;"));  // NMTOKENS attr
  EXPECT_TRUE(has(r.code, R"(xml::attrField("tags", &Cfg::tags))"));
  EXPECT_TRUE(has(r.code, "std::vector<int> ids;"));
  EXPECT_TRUE(has(r.code, R"(xml::attrField("ids", &Cfg::ids))"));
}

TEST(XsdCodegen, RecursionUsesUniquePtr) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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

// Facet capture: string length and numeric range constraints.
TEST(XsdCodegen, StringLengthFacetsGenerateConstraints) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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

TEST(XsdCodegen, NumericRangeFacets) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="SmallInt">
      <xs:restriction base="xs:int">
        <xs:minInclusive value="0"/>
        <xs:maxInclusive value="100"/>
      </xs:restriction>
    </xs:simpleType>
    <xs:simpleType name="UnitInterval">
      <xs:restriction base="xs:double">
        <xs:minExclusive value="0"/>
        <xs:maxExclusive value="1"/>
      </xs:restriction>
    </xs:simpleType>
    <xs:complexType name="Rec">
      <xs:sequence>
        <xs:element name="score" type="SmallInt"/>
        <xs:element name="prob" type="UnitInterval"/>
      </xs:sequence>
    </xs:complexType>
  </xs:schema>)";
  const auto r = xsd::generate(xsd);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "XmlConstraints<Rec>")) << r.code;
  EXPECT_TRUE(has(r.code, "v.score < 0")) << r.code;    // minInclusive: <
  EXPECT_TRUE(has(r.code, "v.score > 100")) << r.code;  // maxInclusive: >
  EXPECT_TRUE(has(r.code, "v.prob <= 0")) << r.code;    // minExclusive: <=
  EXPECT_TRUE(has(r.code, "v.prob >= 1")) << r.code;    // maxExclusive: >=
}

TEST(XsdCodegen, InlineFacetsOnElement) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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

// 3a: xs:complexContent extension -> struct inheritance + merged metadata.
TEST(XsdCodegen, ComplexContentExtension) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  EXPECT_TRUE(has(r.code, R"(xml::attrField("id", &Employee::id, true))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::field("name", &Employee::name, true))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::field("department", &Employee::department, true))")) << r.code;
}

// 3b: xs:attributeGroup inline expansion.
TEST(XsdCodegen, AttributeGroupExpansion) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  EXPECT_TRUE(has(r.code, R"(xml::attrField("id", &Widget::id, true))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::attrField("lang", &Widget::lang))")) << r.code;
}

// 3b: xs:group inline expansion.
TEST(XsdCodegen, ElementGroupExpansion) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  const std::string_view base_xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:include schemaLocation="common.xsd"/>
    <xs:complexType name="Widget">
      <xs:sequence>
        <xs:element name="name" type="xs:string"/>
      </xs:sequence>
      <xs:attribute name="color" type="Color"/>
    </xs:complexType>
  </xs:schema>)";
  const std::string_view common_xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
    <xs:simpleType name="Color">
      <xs:restriction base="xs:string">
        <xs:enumeration value="red"/>
        <xs:enumeration value="blue"/>
      </xs:restriction>
    </xs:simpleType>
  </xs:schema>)";

  xsd::Options opts;
  opts.loader = [&](std::string_view loc) -> std::optional<std::string> {
    if (loc == "common.xsd") {
      return std::string{common_xsd};
    }
    return {};
  };
  const auto r = xsd::generate(base_xsd, opts);
  ASSERT_TRUE(r.ok);
  EXPECT_TRUE(has(r.code, "enum class Color")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::attrField("color", &Widget::color))")) << r.code;
}

// 4a: finite maxOccurs -> std::vector member + XmlConstraints size check.
TEST(XsdCodegen, FiniteMaxOccursConstraint) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  EXPECT_TRUE(has(r.code, "xml::variantField(&Mixed::choice")) << r.code;
}

// xs:complexContent extension across three levels: all levels appear in
// the deepest child's XmlMetadata, none are re-declared in its struct body.
TEST(XsdCodegen, MultiLevelComplexContentExtension) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  EXPECT_TRUE(has(r.code, R"(xml::attrField("name", &Dog::name, true))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::attrField("owner", &Dog::owner))")) << r.code;
  EXPECT_TRUE(has(r.code, R"(xml::attrField("breed", &Dog::breed))")) << r.code;
  // Dog's struct body must only declare breed.
  const auto dog_def = r.code.find("struct Dog :");
  ASSERT_NE(dog_def, std::string::npos) << r.code;
  const auto dog_end = r.code.find("\n};", dog_def);
  const std::string dog_body = r.code.substr(dog_def, dog_end - dog_def);
  EXPECT_EQ(dog_body.find("std::string name"), std::string::npos) << dog_body;
  EXPECT_EQ(dog_body.find("std::string owner"), std::string::npos) << dog_body;
  EXPECT_NE(dog_body.find("std::string breed"), std::string::npos) << dog_body;
}

// Attribute whose type is a named simpleType with facets generates a constraint.
TEST(XsdCodegen, AttributeFacetConstraint) {
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
  const std::string_view xsd = R"(<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema">
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
