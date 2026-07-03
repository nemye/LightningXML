#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "LightningXML.hh"
#include "test_Helpers.hh"

class LightningBasicTests : public ::testing::Test {};

TEST_F(LightningBasicTests, ParsingNested) {
  const std::string_view xml_src = R"(
<person>
  <name>Alice</name>
  <age>30</age>
  <address>
    <street>Main St</street>
    <zip>12345</zip>
  </address>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "employee", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::RootElementNotFound);

  xmlight::Parser correct_parser{xml_src};
  ASSERT_TRUE(xmlight::deserialize(correct_parser, "person", person));
  EXPECT_EQ(person.name, "Alice");
  EXPECT_EQ(person.age, 30);
  EXPECT_EQ(person.address.street, "Main St");
  EXPECT_EQ(person.address.zip, 12345);
}

TEST_F(LightningBasicTests, SkipsUnknownFields) {
  const std::string_view xml_src = R"(
<person>
  <name>Bob</name>
  <age>25</age>
  <nickname>Bobby</nickname>
  <address>
    <street>Elm Ave</street>
    <zip>99999</zip>
  </address>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "Bob");
  EXPECT_EQ(person.age, 25);
  EXPECT_EQ(person.address.street, "Elm Ave");
  EXPECT_EQ(person.address.zip, 99999);
}

TEST_F(LightningBasicTests, MissingFieldsRetainDefaults) {
  const std::string_view xml_src = R"(
<person>
  <name>Carol</name>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  std::ignore = xmlight::deserialize(parser, "person", person);
  EXPECT_EQ(person.name, "Carol");
  EXPECT_EQ(person.age, 0);
}

TEST_F(LightningBasicTests, MalformedXmlReturnsFalse) {
  const std::string_view xml_src = R"(<person><name>Dave</name>)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::UnexpectedEof);
}

TEST_F(LightningBasicTests, EmptyInputReturnsFalse) {
  xmlight::Parser parser{""};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::RootElementNotFound);
}

TEST_F(LightningBasicTests, ParsesFieldsOutOfOrder) {
  const std::string_view xml_src = R"(
<person>
  <age>42</age>
  <address>
    <zip>11111</zip>
    <street>Oak Rd</street>
  </address>
  <name>Eve</name>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "Eve");
  EXPECT_EQ(person.age, 42);
  EXPECT_EQ(person.address.street, "Oak Rd");
  EXPECT_EQ(person.address.zip, 11111);
}

TEST_F(LightningBasicTests, PullsUserDataAndIgnoresUnknownTags) {
  const std::string_view xml_src = R"(
<Users>
  <User id="42">
    <Name>Ada Lovelace</Name>
    <UnknownTag><Nested>Ignore me</Nested></UnknownTag>
    <Email>ada@example.com</Email>
  </User>
  <User id="99">
    <Name>Grace Hopper</Name>
    <Email>grace@example.com</Email>
  </User>
</Users>
)";
  xmlight::Parser parser{xml_src};
  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  ASSERT_EQ(users.items.size(), 2U);
  EXPECT_EQ(users.items[0].id, 42);
  EXPECT_EQ(users.items[0].name, "Ada Lovelace");
  EXPECT_EQ(users.items[0].email, "ada@example.com");
  EXPECT_EQ(users.items[1].id, 99);
  EXPECT_EQ(users.items[1].name, "Grace Hopper");
  EXPECT_EQ(users.items[1].email, "grace@example.com");
}

TEST_F(LightningBasicTests, DeserializesFullOrganizationHierarchy) {
  const std::string_view xml_src = R"(<?xml version="1.0"?>
<Organization id="1" name="Acme Corp">
  <Department id="10" name="Engineering">
    <Team id="100" name="Platform">
      <Member id="1001" role="Engineer">
        <FullName>Ada Lovelace</FullName>
        <Email>ada@acme.com</Email>
        <Skills>
          <Skill>C++</Skill>
          <Skill>Algorithms</Skill>
        </Skills>
      </Member>
      <Member id="1002" role="Lead">
        <FullName>Grace Hopper</FullName>
        <Email>grace@acme.com</Email>
        <Skills>
          <Skill>Compilers</Skill>
          <Skill>Leadership</Skill>
          <Skill>COBOL</Skill>
        </Skills>
      </Member>
    </Team>
  </Department>
</Organization>)";

  xmlight::Parser parser{xml_src};
  Organization org;
  ASSERT_TRUE(xmlight::deserialize(parser, "Organization", org));
  EXPECT_EQ(org.id, 1);
  EXPECT_EQ(org.name, "Acme Corp");
  ASSERT_EQ(org.departments.size(), 1U);

  const auto& eng = org.departments[0];
  EXPECT_EQ(eng.id, 10);
  EXPECT_EQ(eng.name, "Engineering");
  ASSERT_EQ(eng.teams.size(), 1U);

  const auto& platform = eng.teams[0];
  EXPECT_EQ(platform.id, 100);
  EXPECT_EQ(platform.name, "Platform");
  ASSERT_EQ(platform.members.size(), 2U);

  const auto& ada = platform.members[0];
  EXPECT_EQ(ada.id, 1001);
  EXPECT_EQ(ada.role, "Engineer");
  EXPECT_EQ(ada.full_name, "Ada Lovelace");
  EXPECT_EQ(ada.email, "ada@acme.com");
  ASSERT_EQ(ada.skills.items.size(), 2U);
  EXPECT_EQ(ada.skills.items[0], "C++");
  EXPECT_EQ(ada.skills.items[1], "Algorithms");
}

/// @brief Tests that empty string attributes are parsed as empty string_views.
TEST_F(LightningBasicTests, EmptyStringAttribute) {
  const std::string_view xml_src = R"(<Organization id="1" name=""></Organization>)";
  xmlight::Parser parser{xml_src};
  Organization org;

  ASSERT_TRUE(xmlight::deserialize(parser, "Organization", org));
  EXPECT_EQ(org.id, 1);
  EXPECT_TRUE(org.name.empty());
}

/// @brief Tests that empty numeric attributes and empty child elements
/// fall back to default values safely without failing the parse.
TEST_F(LightningBasicTests, EmptyNumericAttributeErrors) {
  // An empty value on a non-optional numeric attribute ("") is not valid
  // lexical space for the type, so it is a hard error rather than a silently
  // dropped default. (Use std::optional<int> to allow an empty/absent value.)
  // This mirrors how an empty numeric *element* is already rejected.
  const std::string_view xml_src = R"(
<Users>
  <User id="">
    <Name>Ghost</Name>
    <Email></Email>
  </User>
</Users>
)";

  xmlight::Parser parser{xml_src};
  Users users;

  EXPECT_FALSE(xmlight::deserialize(parser, "Users", users));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::InvalidNumericValue);
}

/// @brief Tests that malformed numeric data fails gracefully rather than
/// throwing or crashing.
TEST_F(LightningBasicTests, MalformedNumericDataFailsGracefully) {
  const std::string_view xml_src = R"(
<person>
  <name>John</name>
  <age>not-a-number</age>
  <address><street>123 Ave</street><zip>NaN</zip></address>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  // Deserialization of primitives returns false if parse_numeric fails.
  // Because pull() relies on read_element returning true for handled fields,
  // failing to parse 'age' causes it to return false, propagating up.
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::InvalidNumericValue);
}

/// @brief Tests that self-closing tags with attributes process the attributes
/// and terminate correctly.
TEST_F(LightningBasicTests, SelfClosingTagWithAttributes) {
  const std::string_view xml_src = R"(<Users><User id="77" Name="Self" Email="none"/></Users>)";
  xmlight::Parser parser{xml_src};
  Users users;

  // For this to work, User must be able to pull() attributes from a
  // self-closing tag. The current logic in pull() checks for ElementClose
  // immediately.
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  ASSERT_EQ(users.items.size(), 1U);
  EXPECT_EQ(users.items[0].id, 77);
  // Name and Email won't be populated because there are no child elements,
  // but the element itself should be consumed without error.
  EXPECT_TRUE(users.items[0].name.empty());
}

/// @brief Tests mixed content (text nodes alongside child elements).
/// The parser should ignore text nodes that aren't bound to a specific field.
TEST_F(LightningBasicTests, MixedContentIsIgnored) {
  const std::string_view xml_src = R"(
<person>
  Some raw text that shouldn't break the parser.
  <name>Mixer</name>
  More random text.
  <age>45</age>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "Mixer");
  EXPECT_EQ(person.age, 45);
}

/// @brief Tests that comments and CDATA between fields do not disrupt
/// deserialization.
TEST_F(LightningBasicTests, IgnoresInterleavedCommentsAndCData) {
  const std::string_view xml_src = R"(
<person>
  <name>Frank</name>
  <![CDATA[ Some raw data that the parser should ignore because it's not in a mapped field ]]>
  <age>50</age>
  </person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "Frank");
  EXPECT_EQ(person.age, 50);
}

/// @brief Enforces the zero-allocation rule that entities are NOT expanded.
TEST_F(LightningBasicTests, EntitiesRemainUnexpanded) {
  const std::string_view xml_src = R"(
<person>
  <name>AT&amp;T</name>
  <address><street>Me &lt; You</street></address>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));

  // The views should contain the raw entity strings
  EXPECT_EQ(person.name, "AT&amp;T");
  EXPECT_EQ(person.address.street, "Me &lt; You");
}

/// @brief Tests that deeply mismatched closing tags cause an immediate parse
/// failure.
TEST_F(LightningBasicTests, MismatchedClosingTagsFailCleanly) {
  const std::string_view xml_src = R"(
<person>
  <name>Alice</age> </person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::ElementMismatch);
}

/// @brief Tests that both empty tags and self-closing tags yield empty string
/// views.
TEST_F(LightningBasicTests, EmptyAndSelfClosingStringPrimitives) {
  const std::string_view xml_src = R"(
<Users>
  <User id="1"><Name></Name><Email/></User>
</Users>
)";
  xmlight::Parser parser{xml_src};
  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  ASSERT_EQ(users.items.size(), 1U);
  EXPECT_TRUE(users.items[0].name.empty());
  EXPECT_TRUE(users.items[0].email.empty());
}

/// @brief Tests that a mismatched close tag deep in the hierarchy propagates
/// failure all the way out, not just at the leaf level.
TEST_F(LightningBasicTests, DeepMismatchedClosingTagFails) {
  const std::string_view xml_src = R"(
<person>
  <address>
    <street>123 Ave</zip>
  </address>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::ElementMismatch);
}

/// @brief Tests that a close tag with no name (</>) is rejected as malformed.
TEST_F(LightningBasicTests, EmptyClosingTagNameFails) {
  const std::string_view xml_src = R"(<person></>)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::ExpectedNameInCloseTag);
}

/// @brief Tests that a stray close tag appearing before the root open tag
/// causes begin_element to fail immediately.
TEST_F(LightningBasicTests, StrayCloseTagBeforeRootFails) {
  const std::string_view xml_src = R"(</person><person></person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::RootElementNotFound);
}

/// @brief Tests that a non-xml processing instruction before the root element
/// is skipped and the document parses correctly.
TEST_F(LightningBasicTests, NonXmlProcessingInstructionIsSkipped) {
  const std::string_view xml_src =
      R"(<?xml-stylesheet type="text/xsl" href="style.xsl"?><person><name>Pat</name></person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "Pat");
}

/// @brief Tests that when a comment splits a text node inside a primitive
/// element, only the last text segment is captured (last-write-wins).
/// This is a known limitation of the zero-copy design: value() overwrites
/// 'out' on each Text token, so a comment between two text runs discards
/// the first run.
TEST_F(LightningBasicTests, CommentSplitsTextNodeLastSegmentWins) {
  const std::string_view xml_src = R"(
<person>
  <name>Al<!--comment-->ice</name>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  // "Al" is overwritten when "ice" arrives as a second Text token.
  EXPECT_EQ(person.name, "ice");
}

/// @brief Tests that an unexpected open element inside a string primitive
/// field is silently consumed and only the trailing text is captured.
/// This is a known limitation: value() has no mechanism to reject child
/// elements and will resume reading after the inner element is consumed.
TEST_F(LightningBasicTests, OpenElementInsideStringPrimitiveIsConsumed) {
  const std::string_view xml_src = R"(
<person>
  <name><unexpected/>hello</name>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  // Text before the inner element ("") is overwritten by text after it.
  EXPECT_EQ(person.name, "hello");
}

/// @brief Tests that a numeric field containing only whitespace fails to
/// parse, because std::from_chars does not accept leading whitespace.
TEST_F(LightningBasicTests, WhitespaceOnlyNumericFieldFails) {
  const std::string_view xml_src = R"(
<person>
  <name>Test</name>
  <age>   </age>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::InvalidNumericValue);
}

/// @brief Tests that a numeric field with whitespace padding around a valid
/// number fails to parse, because std::from_chars does not accept leading
/// whitespace or trailing characters after the number.
TEST_F(LightningBasicTests, WhitespacePaddedNumericFieldCollapses) {
  // XSD whitespace `collapse` applies to numeric leaves, so surrounding
  // whitespace is trimmed before the value is parsed.
  const std::string_view xml_src = R"(
<person>
  <name>Test</name>
  <age> 30 </age>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.age, 30);
}

TEST_F(LightningBasicTests, ParserCanBeResetAndReused) {
  const std::string_view xml_src = R"(<person><name>Alice</name><age>30</age></person>)";

  xmlight::Parser parser{xml_src};

  Person first;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", first));
  EXPECT_EQ(first.name, "Alice");

  parser.reset();

  Person second;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", second));
  EXPECT_EQ(second.name, "Alice");
  EXPECT_EQ(second.age, 30);
}

TEST_F(LightningBasicTests, PrimitiveFastPathTruncatedCloseTag) {
  // Truncated close tag: `</name` with no `>`.
  const std::string xml = "<person><name>Alice</name";
  xmlight::Parser parser{xml};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::ExpectedCloseTagEnd);
}

TEST_F(LightningBasicTests, IgnoresTrailingDocumentContent) {
  const std::string_view xml_src = R"(
<person>
  <name>Alice</name>
</person>
<junk>should not matter</junk>
)";

  xmlight::Parser parser{xml_src};

  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));

  EXPECT_EQ(person.name, "Alice");
}

TEST_F(LightningBasicTests, SkipsDeepUnknownSubtree) {
  const std::string_view xml_src = R"(
<person>
  <name>Alice</name>
  <unknown>
    <a>
      <b>
        <c>
          <d>ignored</d>
        </c>
      </b>
    </a>
  </unknown>
  <age>30</age>
</person>
)";

  xmlight::Parser parser{xml_src};

  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));

  EXPECT_EQ(person.name, "Alice");
  EXPECT_EQ(person.age, 30);
}

/// @brief Unknown subtrees containing quoted '>' and "/>" in attribute
/// values, comments and CDATA with markup-like content, PIs, and
/// self-closing tags must be skipped without desyncing the parse.
TEST_F(LightningBasicTests, SkipsUnknownSubtreeWithTrickyContent) {
  const std::string_view xml_src = R"(
<person>
  <name>Alice</name>
  <unknown a="x > y" b='gt > inside'>
    <!-- comment with > and "quotes" and <tags> -->
    <![CDATA[ raw <blob/> with ]] almost-terminators ]]>
    <?pi target with > inside ?>
    <child attr="/>">text</child>
    <selfclosed x="y"/>
  </unknown>
  <age>30</age>
</person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "Alice");
  EXPECT_EQ(person.age, 30);
}

/// @brief Input truncated inside an unknown subtree must fail the parse.
TEST_F(LightningBasicTests, TruncatedUnknownSubtreeFails) {
  const std::string_view xml_src = R"(<person><unknown><a><b>deep)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::UnexpectedEof);
}

/// @brief An unterminated attribute quote inside an unknown subtree must
/// fail the parse rather than scan past the document end.
TEST_F(LightningBasicTests, UnterminatedQuoteInUnknownSubtreeFails) {
  const std::string_view xml_src =
      R"(<person><unknown><a attr="broken></a></unknown><name>X</name></person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::UnexpectedEof);
}

TEST_F(LightningBasicTests, SingleQuotedAttributes) {
  const std::string_view xml_src = R"(<Users><User id='123'></User></Users>)";

  xmlight::Parser parser{xml_src};

  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));

  ASSERT_EQ(users.items.size(), 1U);
  EXPECT_EQ(users.items[0].id, 123);
}

TEST_F(LightningBasicTests, MissingAttributeQuoteFails) {
  const std::string_view xml_src = R"(<Users><User id=123></User></Users>)";

  xmlight::Parser parser{xml_src};

  Users users;
  EXPECT_FALSE(xmlight::deserialize(parser, "Users", users));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::ExpectedQuotedValue);
}

TEST_F(LightningBasicTests, UnterminatedAttributeFails) {
  const std::string_view xml_src = R"(<Users><User id="123></User></Users>)";

  xmlight::Parser parser{xml_src};

  Users users;
  EXPECT_FALSE(xmlight::deserialize(parser, "Users", users));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::UnterminatedAttributeValue);
}

TEST_F(LightningBasicTests, UnterminatedCommentFails) {
  const std::string_view xml_src = R"(
<person>
  <!-- broken
  <name>Alice</name>
</person>
)";

  xmlight::Parser parser{xml_src};

  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::UnterminatedComment);
}

TEST_F(LightningBasicTests, UnterminatedCDataFails) {
  const std::string_view xml_src = R"(
<person>
  <![CDATA[broken
</person>
)";

  xmlight::Parser parser{xml_src};

  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::UnterminatedCData);
}

TEST_F(LightningBasicTests, UnterminatedProcessingInstructionFails) {
  const std::string_view xml_src = R"(<?xml-stylesheet type="text/xsl"<person></person>)";

  xmlight::Parser parser{xml_src};

  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::UnterminatedPi);
}

TEST_F(LightningBasicTests, NamespacePrefixesAreIgnoredForMatching) {
  const std::string_view xml_src = R"(
<ns:person>
  <ns:name>Alice</ns:name>
  <ns:age>30</ns:age>
</ns:person>
)";

  xmlight::Parser parser{xml_src};

  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));

  EXPECT_EQ(person.name, "Alice");
  EXPECT_EQ(person.age, 30);
}

TEST_F(LightningBasicTests, NamespacedAttributes) {
  const std::string_view xml_src = R"(<Users><User ns:id="55"></User></Users>)";

  xmlight::Parser parser{xml_src};

  Users users;

  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  ASSERT_EQ(users.items.size(), 1U);

  EXPECT_EQ(users.items[0].id, 55);
}

TEST_F(LightningBasicTests, LargeVectorOfUsers) {
  std::string xml = "<Users>";

  for (int i = 0; i < 1000; ++i) {
    xml += "<User id=\"" + std::to_string(i) + "\">";
    xml += "<Name>User</Name>";
    xml += "</User>";
  }

  xml += "</Users>";

  xmlight::Parser parser{xml};

  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));

  ASSERT_EQ(users.items.size(), 1000U);
  EXPECT_EQ(users.items.front().id, 0);
  EXPECT_EQ(users.items.back().id, 999);
}

TEST_F(LightningBasicTests, StringViewsReferenceOriginalBuffer) {
  std::string xml = "<person><name>Alice</name></person>";

  xmlight::Parser parser{xml};

  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));

  const char* begin = xml.data();
  const char* end = begin + xml.size();

  EXPECT_GE(person.name.data(), begin);
  EXPECT_LT(person.name.data(), end);
}

TEST_F(LightningBasicTests, DuplicateFieldLastValueWins) {
  const std::string_view xml_src = R"(
<person>
  <name>First</name>
  <name>Second</name>
</person>
)";

  xmlight::Parser parser{xml_src};

  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));

  EXPECT_EQ(person.name, "Second");
}

/// @brief XML tags are strictly case-sensitive. Verify that mismatched casing
/// is ignored.
TEST_F(LightningBasicTests, CaseSensitivity) {
  const std::string_view xml_src = R"(
<person>
  <NAME>Alice</NAME>
  <Age>30</Age>
</person>
)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));

  // Should remain default/empty because the struct maps to "name" and "age",
  // not "NAME" and "Age"
  EXPECT_TRUE(person.name.empty());
  EXPECT_EQ(person.age, 0);
}

/// @brief Because it's a zero-copy parser leveraging string_views, standard
/// string text nodes should preserve exact whitespace (including newlines).
TEST_F(LightningBasicTests, PreservesWhitespaceInStrings) {
  const std::string_view xml_src = R"(<person><name>
    Spaced Out
  </name></person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "\n    Spaced Out\n  ");
}

/// @brief XML 1.0 forbids duplicate attributes, but LightningXML does not detect
/// them (documented limitation -- the O(n^2) check is too costly for the
/// performance target). The document-order (first) match deterministically
/// wins.
TEST_F(LightningBasicTests, DuplicateAttributesTakesFirst) {
  const std::string_view xml_src = R"(<Users><User id="1" id="2"><Name>Bob</Name></User></Users>)";
  xmlight::Parser parser{xml_src};
  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  ASSERT_EQ(users.items.size(), 1U);
  EXPECT_EQ(users.items[0].id, 1);
}

/// @brief Tests that if the root element does not match the requested object
/// name, the parser fails gracefully right away.
TEST_F(LightningBasicTests, RootTagMismatchFails) {
  const std::string_view xml_src = R"(<alien><name>Zorg</name></alien>)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::RootElementNotFound);
}

/// @brief Verifies that attributes located directly on the root node being
/// deserialized are parsed and populated correctly.
TEST_F(LightningBasicTests, ParsesAttributesOnRootElement) {
  const std::string_view xml_src = R"(<Organization id="999" name="Global Corp"></Organization>)";
  xmlight::Parser parser{xml_src};
  Organization org;
  ASSERT_TRUE(xmlight::deserialize(parser, "Organization", org));
  EXPECT_EQ(org.id, 999);
  EXPECT_EQ(org.name, "Global Corp");
}

/// @brief Verifies deeply nested hierarchical deserialization. Tests parser's
/// recursion limit / stack handling implicitly using the test_Helpers.hh DeepList.
TEST_F(LightningBasicTests, DeepNestingDeserialization) {
  const std::string_view xml_src = R"(
<DeepList>
  <L1>
    <L2>
      <L3>
        <L4>
          <L5><v>42</v></L5>
        </L4>
      </L3>
    </L2>
  </L1>
</DeepList>
)";
  xmlight::Parser parser{xml_src};
  DeepList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "DeepList", list));
  ASSERT_EQ(list.items.size(), 1U);
  EXPECT_EQ(list.items[0].next.next.next.next.value, 42);
}

/// @brief Mixes standard tags and self-closing tags within the same vector
/// to ensure the token state machine resets cleanly between iterations.
TEST_F(LightningBasicTests, MixedSelfClosingAndStandardTagsInVector) {
  const std::string_view xml_src = R"(
<Users>
  <User id="10"><Name>Standard</Name></User>
  <User id="20"/>
  <User id="30"><Name>Standard Again</Name></User>
</Users>
)";
  xmlight::Parser parser{xml_src};
  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  ASSERT_EQ(users.items.size(), 3U);
  EXPECT_EQ(users.items[0].id, 10);
  EXPECT_EQ(users.items[1].id, 20);
  EXPECT_TRUE(users.items[1].name.empty());
  EXPECT_EQ(users.items[2].id, 30);
}

/// @brief Tag names cannot start with a number according to XML specs.
TEST_F(LightningBasicTests, InvalidTagNamesFailCleanly) {
  const std::string_view xml_src = R"(<person><123name>Bob</123name></person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  // Should fail cleanly inside `parse_element_open` since '1' is not
  // `is_name_start`
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::UnexpectedCharAfterLt);
}

/// @brief Tests malformed tags holding garbage characters where attributes are
/// expected.
TEST_F(LightningBasicTests, MalformedTagGarbageFails) {
  const std::string_view xml_src = R"(<person !@#$gar> <name>Alice</name> </person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::ExpectedAttributeName);
}

auto generateNestedXml(const size_t depth) -> std::string {
  std::string xml;
  xml.reserve(depth * 13);  // "<Node></Node>" is 13 chars
  for (size_t i = 0; i < depth; ++i) {
    xml += "<Node>";
  }
  for (size_t i = 0; i < depth; ++i) {
    xml += "</Node>";
  }
  return xml;
}

/// @brief Tests that the parser successfully evaluates exactly kMaxDepth
TEST_F(LightningBasicTests, MaxDepthBoundarySucceeds) {
  const std::string xml_src = generateNestedXml(xmlight::Parser::MAX_DEPTH);

  xmlight::Parser parser{xml_src};
  TreeNode root;

  // Should successfully parse exactly up to the limit
  ASSERT_TRUE(xmlight::deserialize(parser, "Node", root));
  ASSERT_EQ(root.children.size(), 1U);
  EXPECT_EQ(root.children[0].children.size(), 1U);
}

/// @brief Tests that the parser cleanly aborts when depth hits kMaxDepth + 1
TEST_F(LightningBasicTests, MaxDepthExceededFailsCleanly) {
  const std::string xml_src = generateNestedXml(xmlight::Parser::MAX_DEPTH + 1);

  xmlight::Parser parser{xml_src};
  TreeNode root;
  EXPECT_FALSE(xmlight::deserialize(parser, "Node", root));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::DepthExceeded);
}

/// @brief Compact deep XML with zero whitespace between tags.
/// Every open tag hits try_begin_element directly.
TEST_F(LightningBasicTests, DeepNestingNoWhitespace) {
  const std::string_view xml_src =
      R"(<DeepList><L1><L2><L3><L4><L5><v>99</v></L5></L4></L3></L2></L1></DeepList>)";
  xmlight::Parser parser{xml_src};
  DeepList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "DeepList", list));
  ASSERT_EQ(list.items.size(), 1U);
  EXPECT_EQ(list.items[0].next.next.next.next.value, 99);
}

/// @brief Multiple deep elements back-to-back without whitespace.
TEST_F(LightningBasicTests, DeepNestingMultipleNoWhitespace) {
  std::string xml = "<DeepList>";
  for (int i = 0; i < 5; ++i) {
    xml += "<L1><L2><L3><L4><L5><v>" + std::to_string(i) + "</v></L5></L4></L3></L2></L1>";
  }
  xml += "</DeepList>";

  xmlight::Parser parser{xml};
  DeepList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "DeepList", list));
  ASSERT_EQ(list.items.size(), 5U);
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(list.items[i].next.next.next.next.value, i);
  }
}

/// @brief Namespace-prefixed deep nesting causes try_begin_element to fail
/// and fall through to normal tokenisation. Structure must still parse.
TEST_F(LightningBasicTests, DeepNestingWithNamespaceFallthrough) {
  const std::string_view xml_src = R"(
<DeepList>
  <ns:L1><ns:L2><ns:L3><ns:L4><ns:L5>
    <ns:v>42</ns:v>
  </ns:L5></ns:L4></ns:L3></ns:L2></ns:L1>
</DeepList>)";
  xmlight::Parser parser{xml_src};
  DeepList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "DeepList", list));
  ASSERT_EQ(list.items.size(), 1U);
  EXPECT_EQ(list.items[0].next.next.next.next.value, 42);
}

/// @brief Tags with attributes on an N==1 type cause try_begin_element
/// to fail (char after name is ' ', not '>'), falling through to normal
/// tokenisation. TreeNode has no AttrField so attributes are silently ignored.
TEST_F(LightningBasicTests, TreeNodeWithAttributedChildrenFallthrough) {
  const std::string_view xml_src = R"(<Node><Node id="1"><Node/></Node><Node id="2"/></Node>)";
  xmlight::Parser parser{xml_src};
  TreeNode root;
  ASSERT_TRUE(xmlight::deserialize(parser, "Node", root));
  ASSERT_EQ(root.children.size(), 2U);
  ASSERT_EQ(root.children[0].children.size(), 1U);
  EXPECT_TRUE(root.children[1].children.empty());
}

/// @brief Self-closing tags inside a tree hit the try_begin_element
/// self-closing path (<Node/> matched as name + "/>").
TEST_F(LightningBasicTests, SelfClosingViaFastPath) {
  const std::string_view xml_src = R"(<Node><Node/><Node><Node/></Node></Node>)";
  xmlight::Parser parser{xml_src};
  TreeNode root;
  ASSERT_TRUE(xmlight::deserialize(parser, "Node", root));
  ASSERT_EQ(root.children.size(), 2U);
  EXPECT_TRUE(root.children[0].children.empty());
  ASSERT_EQ(root.children[1].children.size(), 1U);
  EXPECT_TRUE(root.children[1].children[0].children.empty());
}

/// @brief Unknown sibling elements at various nesting levels must be skipped
/// even when the fast path fires for known elements.
TEST_F(LightningBasicTests, DeepNestingWithUnknownSiblings) {
  const std::string_view xml_src = R"(
<DeepList>
  <unknown>stuff</unknown>
  <L1>
    <unknown2/>
    <L2><L3><L4><L5><v>1</v></L5></L4></L3></L2>
  </L1>
  <L1><L2><L3><L4><L5><v>2</v></L5></L4></L3></L2></L1>
</DeepList>)";
  xmlight::Parser parser{xml_src};
  DeepList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "DeepList", list));
  ASSERT_EQ(list.items.size(), 2U);
  EXPECT_EQ(list.items[0].next.next.next.next.value, 1);
  EXPECT_EQ(list.items[1].next.next.next.next.value, 2);
}

/// @brief A child element whose name matches an AttrField hash must be
/// skipped without disrupting the parse (exercises the FieldKind::Attr dispatch
/// arm).
TEST_F(LightningBasicTests, ElementNameCollidesWithAttrField) {
  const std::string_view xml_src = R"(
<Users>
  <User id="42">
    <id>999</id>
    <Name>Collider</Name>
    <Email>c@c.com</Email>
  </User>
</Users>)";
  xmlight::Parser parser{xml_src};
  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  ASSERT_EQ(users.items.size(), 1U);
  EXPECT_EQ(users.items[0].id, 42);
  EXPECT_EQ(users.items[0].name, "Collider");
  EXPECT_EQ(users.items[0].email, "c@c.com");
}

/// @brief Empty vector container: root element with no matching children.
TEST_F(LightningBasicTests, EmptyVectorContainer) {
  const std::string_view xml_src = R"(<Users></Users>)";
  xmlight::Parser parser{xml_src};
  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  EXPECT_TRUE(users.items.empty());
}

/// @brief Self-closing root that holds a vector field.
TEST_F(LightningBasicTests, SelfClosingVectorRoot) {
  const std::string_view xml_src = R"(<Users/>)";
  xmlight::Parser parser{xml_src};
  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  EXPECT_TRUE(users.items.empty());
}

/// @brief All self-closing elements inside a vector.
TEST_F(LightningBasicTests, ConsecutiveSelfClosingInVector) {
  const std::string_view xml_src = R"(
<Users>
  <User id="1"/><User id="2"/><User id="3"/><User id="4"/><User id="5"/>
</Users>)";
  xmlight::Parser parser{xml_src};
  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  ASSERT_EQ(users.items.size(), 5U);
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(users.items[i].id, i + 1);
    EXPECT_TRUE(users.items[i].name.empty());
  }
}

/// @brief Empty Skills (vec of string_view primitives).
TEST_F(LightningBasicTests, VecOfPrimitivesEmpty) {
  const std::string_view xml_src = R"(<Skills></Skills>)";
  xmlight::Parser parser{xml_src};
  Skills skills;
  ASSERT_TRUE(xmlight::deserialize(parser, "Skills", skills));
  EXPECT_TRUE(skills.items.empty());
}

/// @brief Multiple primitives in a vector.
TEST_F(LightningBasicTests, VecOfPrimitivesMultiple) {
  const std::string_view xml_src = R"(
<Skills>
  <Skill>C++</Skill>
  <Skill>Rust</Skill>
  <Skill>Go</Skill>
  <Skill>Python</Skill>
</Skills>)";
  xmlight::Parser parser{xml_src};
  Skills skills;
  ASSERT_TRUE(xmlight::deserialize(parser, "Skills", skills));
  ASSERT_EQ(skills.items.size(), 4U);
  EXPECT_EQ(skills.items[0], "C++");
  EXPECT_EQ(skills.items[1], "Rust");
  EXPECT_EQ(skills.items[2], "Go");
  EXPECT_EQ(skills.items[3], "Python");
}

/// @brief Self-closing primitive element yields empty string_view.
TEST_F(LightningBasicTests, VecOfPrimitivesSelfClosing) {
  const std::string_view xml_src = R"(<Skills><Skill>A</Skill><Skill/><Skill>B</Skill></Skills>)";
  xmlight::Parser parser{xml_src};
  Skills skills;
  ASSERT_TRUE(xmlight::deserialize(parser, "Skills", skills));
  ASSERT_EQ(skills.items.size(), 3U);
  EXPECT_EQ(skills.items[0], "A");
  EXPECT_TRUE(skills.items[1].empty());
  EXPECT_EQ(skills.items[2], "B");
}

/// @brief All 10 attributes populated on AttrItem.
TEST_F(LightningBasicTests, FullAttrItemAllAttributesParsed) {
  const std::string_view xml_src = R"(
<AttrList>
  <Item a1="10" a2="20" a3="30" a4="40" a5="50"
        s1="one" s2="two" s3="three" s4="four" s5="five"/>
</AttrList>)";
  xmlight::Parser parser{xml_src};
  AttrList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "AttrList", list));
  ASSERT_EQ(list.items.size(), 1U);
  const auto& item = list.items[0];
  EXPECT_EQ(item.a1, 10);
  EXPECT_EQ(item.a2, 20);
  EXPECT_EQ(item.a3, 30);
  EXPECT_EQ(item.a4, 40);
  EXPECT_EQ(item.a5, 50);
  EXPECT_EQ(item.s1, "one");
  EXPECT_EQ(item.s2, "two");
  EXPECT_EQ(item.s3, "three");
  EXPECT_EQ(item.s4, "four");
  EXPECT_EQ(item.s5, "five");
}

/// @brief Attributes arriving out of metadata order must still bind via the
/// fallback scan (exercises the attribute document-order cursor miss path).
TEST_F(LightningBasicTests, OutOfOrderAttributesParsed) {
  const std::string_view xml_src = R"(
<AttrList>
  <Item s5="five" a1="10" s1="one" a5="50" a2="20"
        s2="two" a3="30" s4="four" a4="40" s3="three"/>
</AttrList>)";
  xmlight::Parser parser{xml_src};
  AttrList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "AttrList", list));
  ASSERT_EQ(list.items.size(), 1U);
  const auto& item = list.items[0];
  EXPECT_EQ(item.a1, 10);
  EXPECT_EQ(item.a2, 20);
  EXPECT_EQ(item.a3, 30);
  EXPECT_EQ(item.a4, 40);
  EXPECT_EQ(item.a5, 50);
  EXPECT_EQ(item.s1, "one");
  EXPECT_EQ(item.s2, "two");
  EXPECT_EQ(item.s3, "three");
  EXPECT_EQ(item.s4, "four");
  EXPECT_EQ(item.s5, "five");
}

/// @brief FlatItem: mixed attrs + child elements.
TEST_F(LightningBasicTests, FlatListParsing) {
  const std::string_view xml_src = R"(
<FlatList>
  <Item id="1">
    <title>First</title>
    <desc>Description one</desc>
    <status>0</status>
  </Item>
  <Item id="2">
    <title>Second</title>
    <desc>Description two</desc>
    <status>1</status>
  </Item>
</FlatList>)";
  xmlight::Parser parser{xml_src};
  FlatList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "FlatList", list));
  ASSERT_EQ(list.items.size(), 2U);
  EXPECT_EQ(list.items[0].id, 1);
  EXPECT_EQ(list.items[0].title, "First");
  EXPECT_EQ(list.items[0].description, "Description one");
  EXPECT_EQ(list.items[0].status, 0);
  EXPECT_EQ(list.items[1].id, 2);
  EXPECT_EQ(list.items[1].title, "Second");
  EXPECT_EQ(list.items[1].description, "Description two");
  EXPECT_EQ(list.items[1].status, 1);
}

/// @brief Reset after a failed parse must allow a clean retry.
TEST_F(LightningBasicTests, ResetAfterFailedParse) {
  const std::string xml = R"(<person><name>Alice</name><age>30</age></person>)";
  xmlight::Parser parser{xml};

  Person p1;
  EXPECT_FALSE(xmlight::deserialize(parser, "employee", p1));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::RootElementNotFound);

  parser.reset();
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::None);  // reset clears it

  Person p2;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", p2));
  EXPECT_EQ(p2.name, "Alice");
  EXPECT_EQ(p2.age, 30);
}

/// @brief Very deeply nested unknown subtree must be fully skipped.
TEST_F(LightningBasicTests, SkipsVeryDeeplyNestedUnknownSubtree) {
  std::string xml = "<person><name>Alice</name><unknown>";
  for (int i = 0; i < 50; ++i) {
    xml += "<level" + std::to_string(i) + ">";
  }
  xml += "deep";
  for (int i = 49; i >= 0; --i) {
    xml += "</level" + std::to_string(i) + ">";
  }
  xml += "</unknown><age>25</age></person>";

  xmlight::Parser parser{xml};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "Alice");
  EXPECT_EQ(person.age, 25);
}

/// @brief Multiple processing instructions before root.
TEST_F(LightningBasicTests, MultipleProcessingInstructions) {
  const std::string_view xml_src =
      R"(<?xml version="1.0"?><?xml-stylesheet type="text/xsl"?><person><name>Bob</name></person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "Bob");
}

/// @brief Comments containing isolated dashes and a "->" near-miss delimiter
/// must not cause premature termination of the comment scan. (Interior "--"
/// is a separate well-formedness error; see DoubleHyphenInsideCommentRejected.)
TEST_F(LightningBasicTests, CommentWithNearMissDelimiters) {
  const std::string_view xml_src = R"(
<person>
  <!-- tricky - dashes -> not yet > still going - almost -->
  <name>Survived</name>
  <age>1</age>
</person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(person.name, "Survived");
  EXPECT_EQ(person.age, 1);
}

/// @brief XML forbids "--" within a comment's content (WFC). LightningXML enforces
/// this in a single scanning pass and reports MalformedComment.
TEST_F(LightningBasicTests, DoubleHyphenInsideCommentRejected) {
  const std::string_view xml_src = R"(<person><!-- bad -- comment --><name>X</name></person>)";
  xmlight::Parser parser{xml_src};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::MalformedComment);
}

/// @brief Verifies that long comments interleaved with elements are
/// correctly skipped and all element data is parsed. Mirrors the
/// comment-heavy benchmark payload structure.
TEST_F(LightningBasicTests, CommentHeavyPayloadParsesCorrectly) {
  const std::string filler(480, '=');
  std::string xml = "<Users>\n";
  for (int i = 0; i < 50; ++i) {
    xml += "  <!-- comment " + std::to_string(i) + " " + filler + " -->\n";
    xml += "  <User id=\"" + std::to_string(i) + "\">\n";
    xml += "    <Name>User " + std::to_string(i) + "</Name>\n";
    xml += "    <Email>u" + std::to_string(i) + "@e.com</Email>\n";
    xml += "  </User>\n";
  }
  // Trailing comment after the last element.
  xml += "  <!-- final trailing comment " + filler + " -->\n";
  xml += "</Users>";

  xmlight::Parser parser{xml};
  Users users;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", users));
  ASSERT_EQ(users.items.size(), 50U);
  for (size_t i = 0; i < 50; ++i) {
    EXPECT_EQ(users.items[i].id, i);
    EXPECT_EQ(users.items[i].name, "User " + std::to_string(i));
    EXPECT_EQ(users.items[i].email, "u" + std::to_string(i) + "@e.com");
  }
}

/// @brief Parse into std::string fields - data must survive after the
/// source buffer is destroyed.
TEST_F(LightningBasicTests, StringFieldsMaterializeData) {
  OwnedPerson person;
  {
    // Source buffer scoped - will be destroyed before assertions.
    const std::string xml =
        "<person><name>Alice</name><age>30</age>"
        "<email>alice@example.com</email></person>";
    xmlight::Parser parser{xml};
    ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
    // xml is destroyed here
  }
  EXPECT_EQ(person.name, "Alice");
  EXPECT_EQ(person.age, 30);
  EXPECT_EQ(person.email, "alice@example.com");
}

/// @brief std::string attributes are materialized, not views.
TEST_F(LightningBasicTests, StringAttrsMaterializeData) {
  OwnedUser user;
  {
    const std::string xml = R"(<OwnedUser id="42" role="admin"><Name>Bob</Name></OwnedUser>)";
    xmlight::Parser parser{xml};
    ASSERT_TRUE(xmlight::deserialize(parser, "OwnedUser", user));
  }
  EXPECT_EQ(user.id, 42);
  EXPECT_EQ(user.role, "admin");
  EXPECT_EQ(user.name, "Bob");
}

/// @brief Vector of std::string - each element is an owned copy.
TEST_F(LightningBasicTests, VecOfStringMaterializesData) {
  OwnedList list;
  {
    const std::string xml =
        "<OwnedList><Tag>Alpha</Tag><Tag>Beta</Tag><Tag>Gamma</Tag></"
        "OwnedList>";
    xmlight::Parser parser{xml};
    ASSERT_TRUE(xmlight::deserialize(parser, "OwnedList", list));
  }
  ASSERT_EQ(list.tags.size(), 3U);
  EXPECT_EQ(list.tags[0], "Alpha");
  EXPECT_EQ(list.tags[1], "Beta");
  EXPECT_EQ(list.tags[2], "Gamma");
}

/// @brief Self-closing element yields empty std::string.
TEST_F(LightningBasicTests, SelfClosingStringFieldIsEmpty) {
  OwnedPerson person;
  const std::string xml = "<person><name/><age>25</age><email/></person>";
  xmlight::Parser parser{xml};
  ASSERT_TRUE(xmlight::deserialize(parser, "person", person));
  EXPECT_TRUE(person.name.empty());
  EXPECT_EQ(person.age, 25);
  EXPECT_TRUE(person.email.empty());
}

/// @brief Exact fill: element count == array capacity.
TEST_F(LightningBasicTests, ArrFieldExactFill) {
  const std::string_view xml_src = R"(
<FixedSkills>
  <Skill>C++</Skill>
  <Skill>Rust</Skill>
  <Skill>Go</Skill>
</FixedSkills>)";
  xmlight::Parser parser{xml_src};
  FixedSkills skills;
  ASSERT_TRUE(xmlight::deserialize(parser, "FixedSkills", skills));
  EXPECT_EQ(skills.items[0], "C++");
  EXPECT_EQ(skills.items[1], "Rust");
  EXPECT_EQ(skills.items[2], "Go");
}

/// @brief Underfill: fewer elements than capacity - remaining slots keep
/// defaults.
TEST_F(LightningBasicTests, ArrFieldUnderfill) {
  const std::string_view xml_src = R"(
<FixedSkills>
  <Skill>Python</Skill>
</FixedSkills>)";
  xmlight::Parser parser{xml_src};
  FixedSkills skills;
  ASSERT_TRUE(xmlight::deserialize(parser, "FixedSkills", skills));
  EXPECT_EQ(skills.items[0], "Python");
  EXPECT_TRUE(skills.items[1].empty());
  EXPECT_TRUE(skills.items[2].empty());
}

/// @brief Overfill: more elements than capacity - extras silently skipped.
TEST_F(LightningBasicTests, ArrFieldOverfill) {
  const std::string_view xml_src = R"(
<FixedSkills>
  <Skill>A</Skill>
  <Skill>B</Skill>
  <Skill>C</Skill>
  <Skill>D</Skill>
  <Skill>E</Skill>
</FixedSkills>)";
  xmlight::Parser parser{xml_src};
  FixedSkills skills;
  ASSERT_TRUE(xmlight::deserialize(parser, "FixedSkills", skills));
  EXPECT_EQ(skills.items[0], "A");
  EXPECT_EQ(skills.items[1], "B");
  EXPECT_EQ(skills.items[2], "C");
}

/// @brief Empty container.
TEST_F(LightningBasicTests, ArrFieldEmpty) {
  const std::string_view xml_src = R"(<FixedSkills></FixedSkills>)";
  xmlight::Parser parser{xml_src};
  FixedSkills skills;
  ASSERT_TRUE(xmlight::deserialize(parser, "FixedSkills", skills));
  EXPECT_TRUE(skills.items[0].empty());
  EXPECT_TRUE(skills.items[1].empty());
  EXPECT_TRUE(skills.items[2].empty());
}

/// @brief Self-closing root with arrField.
TEST_F(LightningBasicTests, ArrFieldSelfClosingRoot) {
  const std::string_view xml_src = R"(<FixedSkills/>)";
  xmlight::Parser parser{xml_src};
  FixedSkills skills;
  ASSERT_TRUE(xmlight::deserialize(parser, "FixedSkills", skills));
  EXPECT_TRUE(skills.items[0].empty());
}

/// @brief Arr field mixed with attr and element fields (N>1 dispatch table
/// path).
TEST_F(LightningBasicTests, ArrFieldMixedWithOtherFields) {
  const std::string_view xml_src = R"(
<MixedRecord id="7">
  <Name>Alice</Name>
  <Score>95</Score>
  <Score>87</Score>
  <Score>92</Score>
</MixedRecord>)";
  xmlight::Parser parser{xml_src};
  MixedRecord rec;
  ASSERT_TRUE(xmlight::deserialize(parser, "MixedRecord", rec));
  EXPECT_EQ(rec.id, 7);
  EXPECT_EQ(rec.name, "Alice");
  EXPECT_EQ(rec.scores[0], 95);
  EXPECT_EQ(rec.scores[1], 87);
  EXPECT_EQ(rec.scores[2], 92);
  EXPECT_EQ(rec.scores[3], 0);
}

/// @brief Arr field with overflow in N>1 type - extras skipped cleanly.
TEST_F(LightningBasicTests, ArrFieldMixedOverflow) {
  const std::string_view xml_src = R"(
<MixedRecord id="1">
  <Score>1</Score><Score>2</Score><Score>3</Score><Score>4</Score>
  <Score>5</Score><Score>6</Score>
  <Name>Bob</Name>
</MixedRecord>)";
  xmlight::Parser parser{xml_src};
  MixedRecord rec;
  ASSERT_TRUE(xmlight::deserialize(parser, "MixedRecord", rec));
  EXPECT_EQ(rec.id, 1);
  EXPECT_EQ(rec.name, "Bob");
  EXPECT_EQ(rec.scores[0], 1);
  EXPECT_EQ(rec.scores[1], 2);
  EXPECT_EQ(rec.scores[2], 3);
  EXPECT_EQ(rec.scores[3], 4);
}

/// @brief An unknown element whose name extends a mapped field's name
/// ("titles" vs "title") must not be matched by the document-order fast
/// path; it is skipped and all mapped siblings parse correctly.
TEST_F(LightningBasicTests, UnknownPrefixNamedSiblingIsSkipped) {
  const std::string_view xml_src = R"(
<FlatList>
  <Item id="1">
    <titles>fake</titles>
    <title>Real</title>
    <desc>D</desc>
    <status>2</status>
  </Item>
</FlatList>)";
  xmlight::Parser parser{xml_src};
  FlatList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "FlatList", list));
  ASSERT_EQ(list.items.size(), 1U);
  EXPECT_EQ(list.items[0].id, 1);
  EXPECT_EQ(list.items[0].title, "Real");
  EXPECT_EQ(list.items[0].description, "D");
  EXPECT_EQ(list.items[0].status, 2);
}

/// @brief Fields arriving out of metadata order across consecutive items
/// must parse correctly, including when document order is later restored
/// (exercises hint misses and re-synchronisation).
TEST_F(LightningBasicTests, OutOfOrderThenInOrderItems) {
  const std::string_view xml_src = R"(
<FlatList>
  <Item id="1">
    <status>5</status>
    <title>A</title>
    <desc>da</desc>
  </Item>
  <Item id="2">
    <desc>db</desc>
    <status>6</status>
    <title>B</title>
  </Item>
  <Item id="3">
    <title>C</title>
    <desc>dc</desc>
    <status>7</status>
  </Item>
</FlatList>)";
  xmlight::Parser parser{xml_src};
  FlatList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "FlatList", list));
  ASSERT_EQ(list.items.size(), 3U);
  EXPECT_EQ(list.items[0].title, "A");
  EXPECT_EQ(list.items[0].description, "da");
  EXPECT_EQ(list.items[0].status, 5);
  EXPECT_EQ(list.items[1].title, "B");
  EXPECT_EQ(list.items[1].description, "db");
  EXPECT_EQ(list.items[1].status, 6);
  EXPECT_EQ(list.items[2].title, "C");
  EXPECT_EQ(list.items[2].description, "dc");
  EXPECT_EQ(list.items[2].status, 7);
}

/// @brief Bool fields accept the XML Schema boolean lexical space
/// ("true", "false", "1", "0") as both attributes and elements.
TEST_F(LightningBasicTests, BoolFieldsAllLexicalForms) {
  {
    const std::string_view xml_src =
        R"(<Toggle enabled="true"><active>1</active><verbose>false</verbose></Toggle>)";
    xmlight::Parser parser{xml_src};
    Toggle t;
    ASSERT_TRUE(xmlight::deserialize(parser, "Toggle", t));
    EXPECT_TRUE(t.enabled);
    EXPECT_TRUE(t.active);
    EXPECT_FALSE(t.verbose);
  }
  {
    const std::string_view xml_src =
        R"(<Toggle enabled="0"><active>false</active><verbose>true</verbose></Toggle>)";
    xmlight::Parser parser{xml_src};
    Toggle t;
    t.enabled = true;
    ASSERT_TRUE(xmlight::deserialize(parser, "Toggle", t));
    EXPECT_FALSE(t.enabled);
    EXPECT_FALSE(t.active);
    EXPECT_TRUE(t.verbose);
  }
}

/// @brief Invalid bool element text fails the parse, mirroring numeric fields.
TEST_F(LightningBasicTests, BoolFieldRejectsInvalidText) {
  const std::string_view xml_src = R"(<Toggle enabled="1"><active>yes</active></Toggle>)";
  xmlight::Parser parser{xml_src};
  Toggle t;
  EXPECT_FALSE(xmlight::deserialize(parser, "Toggle", t));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::InvalidNumericValue);
}

/// @brief An unparseable bool attribute leaves the default value, consistent
/// with how numeric attributes fail silently.
TEST_F(LightningBasicTests, BoolAttrInvalidErrors) {
  // A malformed non-optional boolean attribute is a hard error rather than a
  // silently dropped default.
  const std::string_view xml_src =
      R"(<Toggle enabled="maybe"><active>1</active><verbose>0</verbose></Toggle>)";
  xmlight::Parser parser{xml_src};
  Toggle t;
  EXPECT_FALSE(xmlight::deserialize(parser, "Toggle", t));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::InvalidNumericValue);
}

/// @brief Bools serialize as "true"/"false" and round-trip.
TEST_F(LightningBasicTests, SerializerBoolRoundTrip) {
  Toggle t;
  t.enabled = true;
  t.active = false;
  t.verbose = true;

  const std::string xml = xmlight::serialize("Toggle", t);
  EXPECT_NE(xml.find("enabled=\"true\""), std::string::npos);
  EXPECT_NE(xml.find("<active>false</active>"), std::string::npos);

  xmlight::Parser parser{xml};
  Toggle out;
  ASSERT_TRUE(xmlight::deserialize(parser, "Toggle", out));
  EXPECT_TRUE(out.enabled);
  EXPECT_FALSE(out.active);
  EXPECT_TRUE(out.verbose);
}

TEST_F(LightningBasicTests, SerializerPrimitiveFields) {
  Person person;
  person.name = "Alice";
  person.age = 30;
  person.address.street = "Main St";
  person.address.zip = 12345;

  const std::string xml = xmlight::serialize("person", person);

  xmlight::Parser parser{xml};
  Person out;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", out));
  EXPECT_EQ(out.name, "Alice");
  EXPECT_EQ(out.age, 30);
  EXPECT_EQ(out.address.street, "Main St");
  EXPECT_EQ(out.address.zip, 12345);
}

TEST_F(LightningBasicTests, SerializerAttributeFields) {
  User user;
  user.id = 99;
  user.name = "Grace Hopper";
  user.email = "grace@example.com";

  const std::string xml = xmlight::serialize("User", user);

  xmlight::Parser parser{xml};
  User out;
  ASSERT_TRUE(xmlight::deserialize(parser, "User", out));
  EXPECT_EQ(out.id, 99);
  EXPECT_EQ(out.name, "Grace Hopper");
  EXPECT_EQ(out.email, "grace@example.com");
}

TEST_F(LightningBasicTests, SerializerVecField) {
  Users users;
  users.items.push_back({1, "Ada", "ada@example.com"});
  users.items.push_back({2, "Grace", "grace@example.com"});

  const std::string xml = xmlight::serialize("Users", users);

  xmlight::Parser parser{xml};
  Users out;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", out));
  ASSERT_EQ(out.items.size(), 2U);
  EXPECT_EQ(out.items[0].id, 1);
  EXPECT_EQ(out.items[0].name, "Ada");
  EXPECT_EQ(out.items[1].id, 2);
  EXPECT_EQ(out.items[1].name, "Grace");
}

TEST_F(LightningBasicTests, SerializerArrField) {
  FixedSkills skills;
  skills.items[0] = "C++";
  skills.items[1] = "Rust";
  skills.items[2] = "Go";

  const std::string xml = xmlight::serialize("FixedSkills", skills);

  xmlight::Parser parser{xml};
  FixedSkills out;
  ASSERT_TRUE(xmlight::deserialize(parser, "FixedSkills", out));
  EXPECT_EQ(out.items[0], "C++");
  EXPECT_EQ(out.items[1], "Rust");
  EXPECT_EQ(out.items[2], "Go");
}

TEST_F(LightningBasicTests, SerializerEmptyVec) {
  const Users users;
  const std::string xml = xmlight::serialize("Users", users);

  xmlight::Parser parser{xml};
  Users out;
  ASSERT_TRUE(xmlight::deserialize(parser, "Users", out));
  EXPECT_TRUE(out.items.empty());
}

TEST_F(LightningBasicTests, SerializerAttrOnlyTypeIsSelfClosing) {
  AttrItem item;
  item.a1 = 1;
  item.a2 = 2;
  item.s1 = "hello";

  const std::string xml = xmlight::serialize("Item", item);
  EXPECT_NE(xml.find("/>"), std::string::npos);

  xmlight::Parser parser{xml};
  AttrItem out;
  ASSERT_TRUE(xmlight::deserialize(parser, "Item", out));
  EXPECT_EQ(out.a1, 1);
  EXPECT_EQ(out.s1, "hello");
}

TEST_F(LightningBasicTests, SerializerEscapesSpecialChars) {
  Person person;
  person.name = "AT&T";
  person.age = 1;

  const std::string xml = xmlight::serialize("person", person);
  EXPECT_NE(xml.find("&amp;"), std::string::npos);

  xmlight::Parser parser{xml};
  Person out;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", out));
  EXPECT_EQ(out.name, "AT&amp;T");
}

TEST_F(LightningBasicTests, SerializerCompactMode) {
  Person person;
  person.name = "Bob";
  person.age = 25;

  const std::string pretty = xmlight::serialize<true>("person", person);
  const std::string compact = xmlight::serialize<false>("person", person);

  EXPECT_NE(pretty.find('\n'), std::string::npos);
  EXPECT_EQ(compact.find('\n'), std::string::npos);

  xmlight::Parser parser{compact};
  Person out;
  ASSERT_TRUE(xmlight::deserialize(parser, "person", out));
  EXPECT_EQ(out.name, "Bob");
  EXPECT_EQ(out.age, 25);
}

TEST_F(LightningBasicTests, SerializerRoundTripOrganization) {
  Organization org;
  org.id = 1;
  org.name = "Acme";
  OrgDepartment dept;
  dept.id = 10;
  dept.name = "Eng";
  OrgTeam team;
  team.id = 100;
  team.name = "Platform";
  OrgMember member;
  member.id = 1001;
  member.role = "Engineer";
  member.full_name = "Ada";
  member.email = "ada@acme.com";
  member.skills.items = {"C++", "Rust"};
  team.members.push_back(member);
  dept.teams.push_back(team);
  org.departments.push_back(dept);

  const std::string xml = xmlight::serialize("Organization", org);

  xmlight::Parser parser{xml};
  Organization out;
  ASSERT_TRUE(xmlight::deserialize(parser, "Organization", out));
  ASSERT_EQ(out.departments.size(), 1U);
  ASSERT_EQ(out.departments[0].teams.size(), 1U);
  ASSERT_EQ(out.departments[0].teams[0].members.size(), 1U);
  EXPECT_EQ(out.departments[0].teams[0].members[0].full_name, "Ada");
  ASSERT_EQ(out.departments[0].teams[0].members[0].skills.items.size(), 2U);
  EXPECT_EQ(out.departments[0].teams[0].members[0].skills.items[0], "C++");
}

struct ReqRecord {
  int id{};               // required attribute
  std::string_view name;  // required element
  std::string_view note;  // optional element
};

template<>
struct xmlight::XmlMetadata<ReqRecord> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("id", &ReqRecord::id, true),
                                                 xmlight::field("name", &ReqRecord::name, true),
                                                 xmlight::field("note", &ReqRecord::note));
};

struct ReqList {
  std::vector<std::string_view> items;  // required: at least one
};

template<>
struct xmlight::XmlMetadata<ReqList> {
  static constexpr auto fields = std::make_tuple(xmlight::vecField("item", &ReqList::items, true));
};

struct ReqParent {
  ReqRecord child;  // required nested object
};

template<>
struct xmlight::XmlMetadata<ReqParent> {
  static constexpr auto fields =
      std::make_tuple(xmlight::field("ReqRecord", &ReqParent::child, true));
};

/// @brief All required fields present, optional one absent -> success.
TEST_F(LightningBasicTests, RequiredFieldsAllPresentOptionalAbsent) {
  const std::string_view xml_src = R"(<ReqRecord id="7"><name>Ada</name></ReqRecord>)";
  xmlight::Parser parser{xml_src};
  ReqRecord rec;
  ASSERT_TRUE(xmlight::deserialize(parser, "ReqRecord", rec));
  EXPECT_EQ(rec.id, 7);
  EXPECT_EQ(rec.name, "Ada");
  EXPECT_TRUE(rec.note.empty());
}

/// @brief Optional field may also be present.
TEST_F(LightningBasicTests, RequiredFieldsWithOptionalPresent) {
  const std::string_view xml_src =
      R"(<ReqRecord id="7"><name>Ada</name><note>hi</note></ReqRecord>)";
  xmlight::Parser parser{xml_src};
  ReqRecord rec;
  ASSERT_TRUE(xmlight::deserialize(parser, "ReqRecord", rec));
  EXPECT_EQ(rec.note, "hi");
}

/// @brief Out-of-order required fields still satisfy the mask.
TEST_F(LightningBasicTests, RequiredFieldsOutOfOrder) {
  const std::string_view xml_src =
      R"(<ReqRecord id="7"><note>hi</note><name>Ada</name></ReqRecord>)";
  xmlight::Parser parser{xml_src};
  ReqRecord rec;
  ASSERT_TRUE(xmlight::deserialize(parser, "ReqRecord", rec));
  EXPECT_EQ(rec.name, "Ada");
  EXPECT_EQ(rec.note, "hi");
}

/// @brief Missing a required child element -> MissingRequiredField.
TEST_F(LightningBasicTests, MissingRequiredElementFails) {
  const std::string_view xml_src = R"(<ReqRecord id="7"><note>hi</note></ReqRecord>)";
  xmlight::Parser parser{xml_src};
  ReqRecord rec;
  EXPECT_FALSE(xmlight::deserialize(parser, "ReqRecord", rec));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::MissingRequiredField);
}

/// @brief Missing a required attribute -> MissingRequiredField.
TEST_F(LightningBasicTests, MissingRequiredAttributeFails) {
  const std::string_view xml_src = R"(<ReqRecord><name>Ada</name></ReqRecord>)";
  xmlight::Parser parser{xml_src};
  ReqRecord rec;
  EXPECT_FALSE(xmlight::deserialize(parser, "ReqRecord", rec));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::MissingRequiredField);
}

/// @brief A self-closing element cannot satisfy a required child element,
/// even though its required attribute is present.
TEST_F(LightningBasicTests, SelfClosingMissingRequiredElementFails) {
  const std::string_view xml_src = R"(<ReqRecord id="7"/>)";
  xmlight::Parser parser{xml_src};
  ReqRecord rec;
  EXPECT_FALSE(xmlight::deserialize(parser, "ReqRecord", rec));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::MissingRequiredField);
}

/// @brief A required container with zero matching children fails.
TEST_F(LightningBasicTests, RequiredContainerEmptyFails) {
  const std::string_view xml_src = R"(<ReqList></ReqList>)";
  xmlight::Parser parser{xml_src};
  ReqList list;
  EXPECT_FALSE(xmlight::deserialize(parser, "ReqList", list));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::MissingRequiredField);
}

/// @brief A required container with at least one child succeeds (exercises the
/// N==1 fast-path bit set).
TEST_F(LightningBasicTests, RequiredContainerNonEmptySucceeds) {
  const std::string_view xml_src = R"(<ReqList><item>a</item><item>b</item></ReqList>)";
  xmlight::Parser parser{xml_src};
  ReqList list;
  ASSERT_TRUE(xmlight::deserialize(parser, "ReqList", list));
  ASSERT_EQ(list.items.size(), 2U);
  EXPECT_EQ(list.items[0], "a");
}

/// @brief A required nested object that is absent fails at the parent level.
TEST_F(LightningBasicTests, RequiredNestedObjectMissingFails) {
  const std::string_view xml_src = R"(<ReqParent></ReqParent>)";
  xmlight::Parser parser{xml_src};
  ReqParent p;
  EXPECT_FALSE(xmlight::deserialize(parser, "ReqParent", p));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::MissingRequiredField);
}

/// @brief A required nested object that is present but itself incomplete
/// propagates MissingRequiredField from the inner pull().
TEST_F(LightningBasicTests, RequiredNestedObjectIncompleteFails) {
  const std::string_view xml_src = R"(<ReqParent><ReqRecord id="1"></ReqRecord></ReqParent>)";
  xmlight::Parser parser{xml_src};
  ReqParent p;
  EXPECT_FALSE(xmlight::deserialize(parser, "ReqParent", p));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::MissingRequiredField);
}

/// @brief A required nested object, fully populated, succeeds.
TEST_F(LightningBasicTests, RequiredNestedObjectCompleteSucceeds) {
  const std::string_view xml_src =
      R"(<ReqParent><ReqRecord id="1"><name>Ada</name></ReqRecord></ReqParent>)";
  xmlight::Parser parser{xml_src};
  ReqParent p;
  ASSERT_TRUE(xmlight::deserialize(parser, "ReqParent", p));
  EXPECT_EQ(p.child.id, 1);
  EXPECT_EQ(p.child.name, "Ada");
}

/// @brief Exactly kMaxAttributesPerElement attributes is accepted (boundary).
TEST_F(LightningBasicTests, MaxAttributesBoundaryAccepted) {
  std::string xml = "<User";
  xml.reserve(xmlight::Parser::MAX_ATTRIBUTES_PER_ELEMENT * 5 + 16);
  for (size_t i = 0; i < xmlight::Parser::MAX_ATTRIBUTES_PER_ELEMENT; ++i) {
    xml += R"( a="")";
  }
  xml += "/>";
  xmlight::Parser parser{xml};
  User user;
  EXPECT_TRUE(xmlight::deserialize(parser, "User", user));
}

/// @brief One attribute past the cap is rejected with TooManyAttributes.
TEST_F(LightningBasicTests, TooManyAttributesRejected) {
  std::string xml = "<User";
  xml.reserve(xmlight::Parser::MAX_ATTRIBUTES_PER_ELEMENT * 5 + 16);
  for (size_t i = 0; i <= xmlight::Parser::MAX_ATTRIBUTES_PER_ELEMENT; ++i) {
    xml += R"( a="")";
  }
  xml += "/>";
  xmlight::Parser parser{xml};
  User user;
  EXPECT_FALSE(xmlight::deserialize(parser, "User", user));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::TooManyAttributes);
}

/// @brief A deeply nested *unknown* subtree beyond kMaxDepth is rejected by
/// skip_element (depth parity with the recursive descent path) rather than
/// silently skipped.
TEST_F(LightningBasicTests, UnknownSubtreeDepthLimited) {
  const int over = xmlight::Parser::MAX_DEPTH + 5;
  std::string xml = "<person><name>A</name><unknown>";
  for (int i = 0; i < over; ++i) {
    xml += "<n>";
  }
  for (int i = 0; i < over; ++i) {
    xml += "</n>";
  }
  xml += "</unknown><age>1</age></person>";
  xmlight::Parser parser{xml};
  Person person;
  EXPECT_FALSE(xmlight::deserialize(parser, "person", person));
  EXPECT_EQ(parser.errorCode(), xmlight::ErrorCode::DepthExceeded);
}

// Normalization & reference expansion (NormalizingParser, opt-in)
//
// Normalization is gated on BasicParser<true> (xmlight::NormalizingParser) and only
// applies to owning std::string fields; std::string_view fields stay raw and
// zero-copy. The default xmlight::Parser compiles these paths away entirely.

struct NormRecord {
  std::string body;      // element text: expanded + EOL-normalized
  std::string attr;      // attribute value: expanded + whitespace-normalized
  std::string_view raw;  // string_view stays raw even under normalization
};

template<>
struct xmlight::XmlMetadata<NormRecord> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("a", &NormRecord::attr),
                                                 xmlight::field("body", &NormRecord::body),
                                                 xmlight::field("raw", &NormRecord::raw));
};

struct NormText {
  std::string v;
};

template<>
struct xmlight::XmlMetadata<NormText> {
  static constexpr auto fields = std::make_tuple(xmlight::field("v", &NormText::v));
};

/// @brief The five predefined entities expand in element text.
TEST_F(LightningBasicTests, NormalizePredefinedEntities) {
  const std::string_view src =
      R"(<NormText><v>a &amp; b &lt; c &gt; d &apos; e &quot; f</v></NormText>)";
  xmlight::NormalizingParser p{src};
  NormText t;
  ASSERT_TRUE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(t.v, "a & b < c > d ' e \" f");
}

/// @brief Decimal and hex character references expand, including a multi-byte
/// code point encoded as UTF-8.
TEST_F(LightningBasicTests, NormalizeCharacterReferences) {
  const std::string_view src = R"(<NormText><v>&#65;&#x42;&#x2764;</v></NormText>)";
  xmlight::NormalizingParser p{src};
  NormText t;
  ASSERT_TRUE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(t.v,
            std::string("AB") + "\xE2\x9D\xA4");  // U+2764 HEAVY BLACK HEART
}

/// @brief CDATA content is copied literally: references inside it are NOT
/// expanded, and it concatenates with surrounding text runs.
TEST_F(LightningBasicTests, NormalizeCDataLiteralAndConcatenated) {
  const std::string_view src = R"(<NormText><v>x &amp; <![CDATA[y &amp; z]]> w</v></NormText>)";
  xmlight::NormalizingParser p{src};
  NormText t;
  ASSERT_TRUE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(t.v, "x & y &amp; z w");
}

/// @brief CR and CRLF in element text normalize to a single LF; tabs survive.
TEST_F(LightningBasicTests, NormalizeLineEndings) {
  const std::string src = "<NormText><v>a\r\nb\rc\td</v></NormText>";
  xmlight::NormalizingParser p{src};
  NormText t;
  ASSERT_TRUE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(t.v, "a\nb\nc\td");
}

/// @brief Attribute values: literal whitespace (tab/newline) becomes a space,
/// but whitespace introduced via a character reference is preserved literally.
TEST_F(LightningBasicTests, NormalizeAttributeWhitespace) {
  const std::string src = "<NormRecord a=\"x\ty\nz&#9;w\"><body>b</body><raw>r</raw></NormRecord>";
  xmlight::NormalizingParser p{src};
  NormRecord r;
  ASSERT_TRUE(xmlight::deserialize(p, "NormRecord", r));
  EXPECT_EQ(r.attr, "x y z\tw");  // literal tab/LF -> space; &#9; stays a tab
}

/// @brief std::string_view fields are unaffected by normalization (raw bytes),
/// even on the normalizing parser, while sibling std::string fields expand.
TEST_F(LightningBasicTests, NormalizeLeavesStringViewRaw) {
  const std::string_view src =
      R"(<NormRecord a="ok"><body>m &amp; n</body><raw>p &amp; q</raw></NormRecord>)";
  xmlight::NormalizingParser p{src};
  NormRecord r;
  ASSERT_TRUE(xmlight::deserialize(p, "NormRecord", r));
  EXPECT_EQ(r.body, "m & n");     // owning std::string -> expanded
  EXPECT_EQ(r.raw, "p &amp; q");  // string_view -> raw, zero-copy
}

/// @brief The default parser performs no expansion: owning std::string fields
/// receive raw bytes, preserving byte-for-byte fidelity (opt-in by design).
TEST_F(LightningBasicTests, DefaultParserDoesNotNormalize) {
  const std::string_view src = R"(<NormText><v>a &amp; b</v></NormText>)";
  xmlight::Parser p{src};
  NormText t;
  ASSERT_TRUE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(t.v, "a &amp; b");
}

/// @brief An undefined entity (no DTD, not one of the five predefined) is a
/// hard error on the normalizing path, in element text and in attributes.
TEST_F(LightningBasicTests, NormalizeUndefinedEntityFailsInText) {
  const std::string_view src = R"(<NormText><v>a &bogus; b</v></NormText>)";
  xmlight::NormalizingParser p{src};
  NormText t;
  EXPECT_FALSE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::UndefinedEntity);
}

TEST_F(LightningBasicTests, NormalizeUndefinedEntityFailsInAttribute) {
  const std::string_view src =
      R"(<NormRecord a="x &bogus; y"><body>b</body><raw>r</raw></NormRecord>)";
  xmlight::NormalizingParser p{src};
  NormRecord r;
  EXPECT_FALSE(xmlight::deserialize(p, "NormRecord", r));
  EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::UndefinedEntity);
}

/// @brief A malformed or out-of-range character reference fails with
/// InvalidCharRef.
TEST_F(LightningBasicTests, NormalizeInvalidCharRefFails) {
  const std::string_view src = R"(<NormText><v>&#xZZ;</v></NormText>)";
  xmlight::NormalizingParser p{src};
  NormText t;
  EXPECT_FALSE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::InvalidCharRef);
}

/// @brief A character reference to a code point outside the XML Char production
/// (here NUL) is rejected as InvalidCharRef.
TEST_F(LightningBasicTests, NormalizeForbiddenCodePointFails) {
  const std::string_view src = R"(<NormText><v>&#0;</v></NormText>)";
  xmlight::NormalizingParser p{src};
  NormText t;
  EXPECT_FALSE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::InvalidCharRef);
}

// Strict (fully-conforming) parser: no false positives
//
// StrictParser enforces the three WFCs (rejection cases live in the
// conformance suite). These guard against false positives and confirm it still
// normalizes (StrictParser = normalize + strict).

/// @brief A CDATA section's terminating "]]>" must NOT be flagged as the
/// forbidden CharData sequence (the check runs on text, not CDATA content).
TEST_F(LightningBasicTests, StrictAcceptsCDataTerminator) {
  const std::string_view xml_src = R"(<NormText><v><![CDATA[x]]></v></NormText>)";
  xmlight::StrictParser p{xml_src};
  NormText t;
  ASSERT_TRUE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(t.v, "x");
}

/// @brief "]]" in text without a following '>' is well-formed and accepted.
TEST_F(LightningBasicTests, StrictAcceptsBracketsWithoutClose) {
  const std::string_view xml_src = R"(<NormText><v>a ]] b</v></NormText>)";
  xmlight::StrictParser p{xml_src};
  NormText t;
  ASSERT_TRUE(xmlight::deserialize(p, "NormText", t));
  EXPECT_EQ(t.v, "a ]] b");
}

/// @brief StrictParser is fully conforming: it still expands references and
/// normalizes into owning std::string fields.
TEST_F(LightningBasicTests, StrictParserNormalizes) {
  const std::string_view xml_src =
      R"(<NormRecord a="x&#9;y"><body>m &amp; n</body><raw>r</raw></NormRecord>)";
  xmlight::StrictParser p{xml_src};
  NormRecord r;
  ASSERT_TRUE(xmlight::deserialize(p, "NormRecord", r));
  EXPECT_EQ(r.body, "m & n");  // entity expanded
  EXPECT_EQ(r.attr, "x\ty");   // &#9; preserved as a literal tab
}

//
// Library extensions: lifted field ceiling, enums, value fields, recursion.
//

// Enumerations via XmlEnumTraits (string tokens)
enum class Priority : uint8_t { Low, Medium, High };
template<>
struct xmlight::XmlEnumTraits<Priority> {
  static constexpr auto values = xmlight::enumTable<Priority>(
      {{"Low", Priority::Low}, {"Medium", Priority::Medium}, {"High", Priority::High}});
};

struct Task {
  Priority priority{};  // attribute
  Priority level{};     // child element
};
template<>
struct xmlight::XmlMetadata<Task> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("priority", &Task::priority),
                                                 xmlight::field("level", &Task::level));
};

TEST_F(LightningBasicTests, EnumFieldRoundTrip) {
  const std::string_view src = R"(<Task priority="High"><level>Medium</level></Task>)";
  xmlight::Parser p{src};
  Task t;
  ASSERT_TRUE(xmlight::deserialize(p, "Task", t));
  EXPECT_EQ(t.priority, Priority::High);
  EXPECT_EQ(t.level, Priority::Medium);
  const std::string out = xmlight::serialize<false>("Task", t);
  EXPECT_EQ(out, R"(<Task priority="High"><level>Medium</level></Task>)");
}

TEST_F(LightningBasicTests, EnumUnknownTokenFails) {
  const std::string_view src = R"(<Task priority="High"><level>Wizard</level></Task>)";
  xmlight::Parser p{src};
  Task t;
  EXPECT_FALSE(xmlight::deserialize(p, "Task", t));
  EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::InvalidEnumValue);
}

// Value field (XSD simpleContent)
struct Money {
  std::string_view currency;  // attribute
  double amount{};            // element's own text
};
template<>
struct xmlight::XmlMetadata<Money> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("currency", &Money::currency),
                                                 xmlight::valueField(&Money::amount));
};
struct Invoice {
  Money total;
};
template<>
struct xmlight::XmlMetadata<Invoice> {
  static constexpr auto fields = std::make_tuple(xmlight::field("total", &Invoice::total));
};

TEST_F(LightningBasicTests, ValueFieldNumericRoundTrip) {
  const std::string_view src = R"(<Invoice><total currency="USD">9.99</total></Invoice>)";
  xmlight::Parser p{src};
  Invoice inv;
  ASSERT_TRUE(xmlight::deserialize(p, "Invoice", inv));
  EXPECT_EQ(inv.total.currency, "USD");
  EXPECT_DOUBLE_EQ(inv.total.amount, 9.99);
  const std::string out = xmlight::serialize<false>("Invoice", inv);
  EXPECT_EQ(out, R"(<Invoice><total currency="USD">9.99</total></Invoice>)");
}

struct Measure {
  std::string unit;  // attribute
  std::string text;  // required value
};
template<>
struct xmlight::XmlMetadata<Measure> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("unit", &Measure::unit),
                                                 xmlight::valueField(&Measure::text, true));
};
struct MeasureDoc {
  Measure m;
};
template<>
struct xmlight::XmlMetadata<MeasureDoc> {
  static constexpr auto fields = std::make_tuple(xmlight::field("m", &MeasureDoc::m));
};

TEST_F(LightningBasicTests, ValueFieldStringNormalized) {
  const std::string_view src = R"(<MeasureDoc><m unit="kg">a &amp; b</m></MeasureDoc>)";
  xmlight::NormalizingParser p{src};
  MeasureDoc d;
  ASSERT_TRUE(xmlight::deserialize(p, "MeasureDoc", d));
  EXPECT_EQ(d.m.unit, "kg");
  EXPECT_EQ(d.m.text, "a & b");  // entity expanded into the value
}

TEST_F(LightningBasicTests, ValueFieldRequiredEmptyFails) {
  // Self-closing and empty both lack text -> required value field is missing.
  for (const std::string_view src :
       {std::string_view(R"(<MeasureDoc><m unit="kg"/></MeasureDoc>)"),
        std::string_view(R"(<MeasureDoc><m unit="kg"></m></MeasureDoc>)")}) {
    xmlight::Parser p{src};
    MeasureDoc d;
    EXPECT_FALSE(xmlight::deserialize(p, "MeasureDoc", d));
    EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::MissingRequiredField);
  }
}

// Recursion via std::unique_ptr
struct Section {
  std::string_view title;
  std::unique_ptr<Section> sub;  // optional, recursive
};
template<>
struct xmlight::XmlMetadata<Section> {
  static constexpr auto fields = std::make_tuple(xmlight::field("title", &Section::title),
                                                 xmlight::field("sub", &Section::sub));
};

TEST_F(LightningBasicTests, UniquePtrRecursionChain) {
  const std::string_view src = R"(<Section><title>A</title><sub><title>B</title>)"
                               R"(<sub><title>C</title></sub></sub></Section>)";
  xmlight::Parser p{src};
  Section s;
  ASSERT_TRUE(xmlight::deserialize(p, "Section", s));
  EXPECT_EQ(s.title, "A");
  ASSERT_TRUE(s.sub);
  EXPECT_EQ(s.sub->title, "B");
  ASSERT_TRUE(s.sub->sub);
  EXPECT_EQ(s.sub->sub->title, "C");
  EXPECT_FALSE(s.sub->sub->sub);
  const std::string out = xmlight::serialize<false>("Section", s);
  EXPECT_EQ(out, R"(<Section><title>A</title><sub><title>B</title>)"
                 R"(<sub><title>C</title></sub></sub></Section>)");
}

TEST_F(LightningBasicTests, UniquePtrAbsentChildOmitted) {
  const std::string_view src = R"(<Section><title>solo</title></Section>)";
  xmlight::Parser p{src};
  Section s;
  ASSERT_TRUE(xmlight::deserialize(p, "Section", s));
  EXPECT_EQ(s.title, "solo");
  EXPECT_FALSE(s.sub);
  const std::string out = xmlight::serialize<false>("Section", s);
  EXPECT_EQ(out, R"(<Section><title>solo</title></Section>)");  // no <sub>
}

// More than 64 fields (multiword required mask)
struct Wide72 {
  int a0{};
  int a1{};
  int a2{};
  int a3{};
  int a4{};
  int a5{};
  int a6{};
  int a7{};
  int a8{};
  int a9{};
  int a10{};
  int a11{};
  int a12{};
  int a13{};
  int a14{};
  int a15{};
  int a16{};
  int a17{};
  int a18{};
  int a19{};
  int a20{};
  int a21{};
  int a22{};
  int a23{};
  int a24{};
  int a25{};
  int a26{};
  int a27{};
  int a28{};
  int a29{};
  int a30{};
  int a31{};
  int a32{};
  int a33{};
  int a34{};
  int a35{};
  int a36{};
  int a37{};
  int a38{};
  int a39{};
  int a40{};
  int a41{};
  int a42{};
  int a43{};
  int a44{};
  int a45{};
  int a46{};
  int a47{};
  int a48{};
  int a49{};
  int a50{};
  int a51{};
  int a52{};
  int a53{};
  int a54{};
  int a55{};
  int a56{};
  int a57{};
  int a58{};
  int a59{};
  int a60{};
  int a61{};
  int a62{};
  int a63{};
  int a64{};
  int a65{};
  int a66{};
  int a67{};
  int a68{};
  int a69{};
  int a70{};
  int a71{};
};
template<>
struct xmlight::XmlMetadata<Wide72> {
  static constexpr auto fields = std::make_tuple(
      xmlight::attrField("a0", &Wide72::a0, true), xmlight::attrField("a1", &Wide72::a1),
      xmlight::attrField("a2", &Wide72::a2), xmlight::attrField("a3", &Wide72::a3),
      xmlight::attrField("a4", &Wide72::a4), xmlight::attrField("a5", &Wide72::a5),
      xmlight::attrField("a6", &Wide72::a6), xmlight::attrField("a7", &Wide72::a7),
      xmlight::attrField("a8", &Wide72::a8), xmlight::attrField("a9", &Wide72::a9),
      xmlight::attrField("a10", &Wide72::a10), xmlight::attrField("a11", &Wide72::a11),
      xmlight::attrField("a12", &Wide72::a12), xmlight::attrField("a13", &Wide72::a13),
      xmlight::attrField("a14", &Wide72::a14), xmlight::attrField("a15", &Wide72::a15),
      xmlight::attrField("a16", &Wide72::a16), xmlight::attrField("a17", &Wide72::a17),
      xmlight::attrField("a18", &Wide72::a18), xmlight::attrField("a19", &Wide72::a19),
      xmlight::attrField("a20", &Wide72::a20), xmlight::attrField("a21", &Wide72::a21),
      xmlight::attrField("a22", &Wide72::a22), xmlight::attrField("a23", &Wide72::a23),
      xmlight::attrField("a24", &Wide72::a24), xmlight::attrField("a25", &Wide72::a25),
      xmlight::attrField("a26", &Wide72::a26), xmlight::attrField("a27", &Wide72::a27),
      xmlight::attrField("a28", &Wide72::a28), xmlight::attrField("a29", &Wide72::a29),
      xmlight::attrField("a30", &Wide72::a30), xmlight::attrField("a31", &Wide72::a31),
      xmlight::attrField("a32", &Wide72::a32), xmlight::attrField("a33", &Wide72::a33),
      xmlight::attrField("a34", &Wide72::a34), xmlight::attrField("a35", &Wide72::a35),
      xmlight::attrField("a36", &Wide72::a36), xmlight::attrField("a37", &Wide72::a37),
      xmlight::attrField("a38", &Wide72::a38), xmlight::attrField("a39", &Wide72::a39),
      xmlight::attrField("a40", &Wide72::a40), xmlight::attrField("a41", &Wide72::a41),
      xmlight::attrField("a42", &Wide72::a42), xmlight::attrField("a43", &Wide72::a43),
      xmlight::attrField("a44", &Wide72::a44), xmlight::attrField("a45", &Wide72::a45),
      xmlight::attrField("a46", &Wide72::a46), xmlight::attrField("a47", &Wide72::a47),
      xmlight::attrField("a48", &Wide72::a48), xmlight::attrField("a49", &Wide72::a49),
      xmlight::attrField("a50", &Wide72::a50), xmlight::attrField("a51", &Wide72::a51),
      xmlight::attrField("a52", &Wide72::a52), xmlight::attrField("a53", &Wide72::a53),
      xmlight::attrField("a54", &Wide72::a54), xmlight::attrField("a55", &Wide72::a55),
      xmlight::attrField("a56", &Wide72::a56), xmlight::attrField("a57", &Wide72::a57),
      xmlight::attrField("a58", &Wide72::a58), xmlight::attrField("a59", &Wide72::a59),
      xmlight::attrField("a60", &Wide72::a60), xmlight::attrField("a61", &Wide72::a61),
      xmlight::attrField("a62", &Wide72::a62), xmlight::attrField("a63", &Wide72::a63),
      xmlight::attrField("a64", &Wide72::a64, true), xmlight::attrField("a65", &Wide72::a65),
      xmlight::attrField("a66", &Wide72::a66), xmlight::attrField("a67", &Wide72::a67),
      xmlight::attrField("a68", &Wide72::a68), xmlight::attrField("a69", &Wide72::a69),
      xmlight::attrField("a70", &Wide72::a70), xmlight::attrField("a71", &Wide72::a71, true));
};

static auto wideXml(int omit) -> std::string {
  std::string s = "<Wide72";
  for (int i = 0; i < 72; ++i) {
    if (i == omit) {
      continue;
    }
    s += " a" + std::to_string(i) + "=\"" + std::to_string(i) + "\"";
  }
  s += "/>";
  return s;
}

TEST_F(LightningBasicTests, MoreThan64FieldsAllPresent) {
  const std::string src = wideXml(-1);
  xmlight::Parser p{src};
  Wide72 w;
  ASSERT_TRUE(xmlight::deserialize(p, "Wide72", w));
  EXPECT_EQ(w.a0, 0);
  EXPECT_EQ(w.a64, 64);
  EXPECT_EQ(w.a71, 71);
}

TEST_F(LightningBasicTests, MoreThan64FieldsMissingRequiredSecondWord) {
  // a64 is required and lives in the second mask word; dropping it fails.
  const std::string src = wideXml(64);
  xmlight::Parser p{src};
  Wide72 w;
  EXPECT_FALSE(xmlight::deserialize(p, "Wide72", w));
  EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::MissingRequiredField);
}

// Date/time value types and variant (xs:choice) fields.
struct Event {
  xmlight::Date day;        // attribute
  xmlight::DateTime stamp;  // element
  xmlight::Time at;         // element
};
template<>
struct xmlight::XmlMetadata<Event> {
  static constexpr auto fields =
      std::make_tuple(xmlight::attrField("day", &Event::day),
                      xmlight::field("stamp", &Event::stamp), xmlight::field("at", &Event::at));
};

TEST_F(LightningBasicTests, DateTimeRoundTripAndChrono) {
  const std::string_view src = R"(<Event day="2026-06-18"><stamp>2026-06-18T09:30:00Z</stamp>)"
                               R"(<at>23:59:59.5+02:00</at></Event>)";
  xmlight::Parser p{src};
  Event e;
  ASSERT_TRUE(xmlight::deserialize(p, "Event", e));
  EXPECT_EQ(e.day, (xmlight::Date{2026, 6, 18}));
  EXPECT_EQ(e.stamp.time.hour, 9U);
  EXPECT_TRUE(e.stamp.time.tz.has_value());
  EXPECT_EQ(e.at.nanosecond, 500000000U);
  EXPECT_EQ(e.at.tz, std::chrono::minutes{120});
  // chrono accessors
  EXPECT_EQ(e.day.toSysDays(), std::chrono::sys_days{std::chrono::year{2026} / 6 / 18});
  EXPECT_EQ(e.stamp.toSysTime(),  // UTC instant
            std::chrono::sys_days{std::chrono::year{2026} / 6 / 18} + std::chrono::hours{9} +
                std::chrono::minutes{30});
  EXPECT_EQ(xmlight::serialize<false>("Event", e), src);
}

TEST_F(LightningBasicTests, DateTimeInvalidValue) {
  // Month 13 in an element is a hard parse failure.
  const std::string_view src = R"(<Event day="2026-06-18"><stamp>2026-13-01T00:00:00</stamp>)"
                               R"(<at>00:00:00</at></Event>)";
  xmlight::Parser p{src};
  Event e;
  EXPECT_FALSE(xmlight::deserialize(p, "Event", e));
  EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::InvalidValue);
}

// Variant (xs:choice)
struct VCircle {
  int r{};
};
template<>
struct xmlight::XmlMetadata<VCircle> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("r", &VCircle::r));
};
struct VSquare {
  int s{};
};
template<>
struct xmlight::XmlMetadata<VSquare> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("s", &VSquare::s));
};

struct Shape {
  std::variant<VCircle, VSquare> kind;
};
template<>
struct xmlight::XmlMetadata<Shape> {
  static constexpr auto fields = std::make_tuple(xmlight::requiredVariantField(
      &Shape::kind, xmlight::alt<VCircle>("circle"), xmlight::alt<VSquare>("square")));
};

TEST_F(LightningBasicTests, VariantSingleBothBranches) {
  {
    const std::string_view src = R"(<Shape><square s="7"/></Shape>)";
    xmlight::Parser p{src};
    Shape sh;
    ASSERT_TRUE(xmlight::deserialize(p, "Shape", sh));
    ASSERT_EQ(sh.kind.index(), 1U);
    EXPECT_EQ(std::get<VSquare>(sh.kind).s, 7);
    EXPECT_EQ(xmlight::serialize<false>("Shape", sh), src);
  }
  {
    const std::string_view src = R"(<Shape><circle r="3"/></Shape>)";
    xmlight::Parser p{src};
    Shape sh;
    ASSERT_TRUE(xmlight::deserialize(p, "Shape", sh));
    ASSERT_EQ(sh.kind.index(), 0U);
    EXPECT_EQ(std::get<VCircle>(sh.kind).r, 3);
    EXPECT_EQ(xmlight::serialize<false>("Shape", sh), src);
  }
}

TEST_F(LightningBasicTests, VariantRequiredMissing) {
  const std::string_view src = R"(<Shape><triangle/></Shape>)";
  xmlight::Parser p{src};
  Shape sh;
  EXPECT_FALSE(xmlight::deserialize(p, "Shape", sh));
  EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::MissingRequiredField);
}

struct VDoc {
  std::vector<std::variant<VCircle, VSquare>> body;
};
template<>
struct xmlight::XmlMetadata<VDoc> {
  static constexpr auto fields = std::make_tuple(xmlight::variantField(
      &VDoc::body, xmlight::alt<VCircle>("circle"), xmlight::alt<VSquare>("square")));
};

TEST_F(LightningBasicTests, VariantRepeatedInterleaved) {
  const std::string_view src = R"(<VDoc><circle r="1"/><square s="2"/><circle r="3"/></VDoc>)";
  xmlight::Parser p{src};
  VDoc d;
  ASSERT_TRUE(xmlight::deserialize(p, "VDoc", d));
  ASSERT_EQ(d.body.size(), 3U);
  EXPECT_EQ(d.body[0].index(), 0U);
  EXPECT_EQ(std::get<VCircle>(d.body[0]).r, 1);
  EXPECT_EQ(d.body[1].index(), 1U);
  EXPECT_EQ(std::get<VSquare>(d.body[1]).s, 2);
  EXPECT_EQ(std::get<VCircle>(d.body[2]).r, 3);
  EXPECT_EQ(xmlight::serialize<false>("VDoc", d), src);
}

// std::optional fields (present => engaged, absent => nullopt).
struct OptAddr {
  std::string_view city;
};
template<>
struct xmlight::XmlMetadata<OptAddr> {
  static constexpr auto fields = std::make_tuple(xmlight::field("city", &OptAddr::city));
};

struct OptPerson {
  std::optional<int> age;                // attribute (scalar inner)
  std::optional<std::string_view> nick;  // element (scalar inner)
  std::optional<OptAddr> addr;           // element (object inner)
};
template<>
struct xmlight::XmlMetadata<OptPerson> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("age", &OptPerson::age),
                                                 xmlight::field("nick", &OptPerson::nick),
                                                 xmlight::field("addr", &OptPerson::addr));
};

TEST_F(LightningBasicTests, OptionalAllPresentRoundTrip) {
  const std::string_view src =
      R"(<OptPerson age="30"><nick>Al</nick><addr><city>NYC</city></addr></OptPerson>)";
  xmlight::Parser p{src};
  OptPerson pe;
  ASSERT_TRUE(xmlight::deserialize(p, "OptPerson", pe));
  ASSERT_TRUE(pe.age);
  EXPECT_EQ(*pe.age, 30);
  ASSERT_TRUE(pe.nick);
  EXPECT_EQ(*pe.nick, "Al");
  ASSERT_TRUE(pe.addr);
  EXPECT_EQ(pe.addr->city, "NYC");
  EXPECT_EQ(xmlight::serialize<false>("OptPerson", pe), src);
}

TEST_F(LightningBasicTests, OptionalAbsentStaysEmptyAndOmitted) {
  // No attribute, no child elements: every optional stays nullopt and the
  // serializer omits them.
  const std::string_view src = R"(<OptPerson age="5"></OptPerson>)";
  xmlight::Parser p{src};
  OptPerson pe;
  ASSERT_TRUE(xmlight::deserialize(p, "OptPerson", pe));
  ASSERT_TRUE(pe.age);
  EXPECT_EQ(*pe.age, 5);
  EXPECT_FALSE(pe.nick);
  EXPECT_FALSE(pe.addr);
  EXPECT_EQ(xmlight::serialize<false>("OptPerson", pe), src);  // no <nick>/<addr>
}

// xs:list fields (whitespace-separated values in one element / attribute).
enum class Grade : uint8_t { A, B, C };
template<>
struct xmlight::XmlEnumTraits<Grade> {
  static constexpr auto values =
      xmlight::enumTable<Grade>({{"A", Grade::A}, {"B", Grade::B}, {"C", Grade::C}});
};

struct ListRec {
  std::vector<std::string> tags;  // attribute list
  std::vector<int> dims;          // element list
  std::array<int, 3> fixed{};     // fixed element list (overflow skipped)
  std::vector<Grade> grades;      // enum element list
};
template<>
struct xmlight::XmlMetadata<ListRec> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("tags", &ListRec::tags),
                                                 xmlight::listField("dims", &ListRec::dims),
                                                 xmlight::listField("fixed", &ListRec::fixed),
                                                 xmlight::listField("grades", &ListRec::grades));
};

TEST_F(LightningBasicTests, ListElementsAndAttributesRoundTrip) {
  const std::string_view src =
      R"(<ListRec tags="a b c"><dims>1 2 3 4</dims><fixed>10 20 30 40</fixed>)"
      R"(<grades>A C B</grades></ListRec>)";
  xmlight::Parser p{src};
  ListRec r;
  ASSERT_TRUE(xmlight::deserialize(p, "ListRec", r));
  EXPECT_EQ(r.tags, (std::vector<std::string>{"a", "b", "c"}));
  EXPECT_EQ(r.dims, (std::vector<int>{1, 2, 3, 4}));
  EXPECT_EQ(r.fixed[0], 10);
  EXPECT_EQ(r.fixed[2], 30);  // 40 overflow skipped
  EXPECT_EQ(r.grades, (std::vector<Grade>{Grade::A, Grade::C, Grade::B}));
  // Round-trips to canonical (single-space) list form; fixed array drops
  // overflow.
  EXPECT_EQ(xmlight::serialize<false>("ListRec", r),
            R"(<ListRec tags="a b c"><dims>1 2 3 4</dims><fixed>10 20 30</fixed>)"
            R"(<grades>A C B</grades></ListRec>)");
}

TEST_F(LightningBasicTests, ListEmptyAndSelfClosing) {
  const std::string_view src = R"(<ListRec tags=""><dims/><fixed></fixed></ListRec>)";
  xmlight::Parser p{src};
  ListRec r;
  ASSERT_TRUE(xmlight::deserialize(p, "ListRec", r));
  EXPECT_TRUE(r.tags.empty());
  EXPECT_TRUE(r.dims.empty());
  EXPECT_TRUE(r.grades.empty());
}

TEST_F(LightningBasicTests, ListBadTokenFails) {
  const std::string_view src = R"(<ListRec><dims>1 x 3</dims></ListRec>)";
  xmlight::Parser p{src};
  ListRec r;
  EXPECT_FALSE(xmlight::deserialize(p, "ListRec", r));
  EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::InvalidNumericValue);
}

TEST_F(LightningBasicTests, ListExtraWhitespaceIgnored) {
  const std::string_view src =
      R"(<ListRec><dims>  1   2	3
      4 </dims></ListRec>)";
  xmlight::Parser p{src};
  ListRec r;
  ASSERT_TRUE(xmlight::deserialize(p, "ListRec", r));
  EXPECT_EQ(r.dims, (std::vector<int>{1, 2, 3, 4}));
}

// Robustness / XSD-conformance for typed leaf & attribute values

namespace {
struct RobScalars {
  int count{};
  double ratio{};
  bool flag{};
};
struct RobDate {
  xmlight::Date date{};
};
struct RobAttr {
  int n{};
};
struct RobOptAttr {
  std::optional<int> n;
};
}  // namespace

template<>
struct xmlight::XmlMetadata<RobScalars> {
  static constexpr auto fields = std::make_tuple(xmlight::field("count", &RobScalars::count),
                                                 xmlight::field("ratio", &RobScalars::ratio),
                                                 xmlight::field("flag", &RobScalars::flag));
};
template<>
struct xmlight::XmlMetadata<RobDate> {
  static constexpr auto fields = std::make_tuple(xmlight::field("date", &RobDate::date));
};
template<>
struct xmlight::XmlMetadata<RobAttr> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("n", &RobAttr::n));
};
template<>
struct xmlight::XmlMetadata<RobOptAttr> {
  static constexpr auto fields = std::make_tuple(xmlight::attrField("n", &RobOptAttr::n));
};

template<typename P>
static void expectCount(std::string_view xml, int want) {
  P p{xml};
  RobScalars o{};
  ASSERT_TRUE(xmlight::deserialize(p, "RobScalars", o)) << "ec=" << static_cast<int>(p.errorCode());
  EXPECT_EQ(o.count, want);
}
template<typename P>
static void expectCountError(std::string_view xml, xmlight::ErrorCode want) {
  P p{xml};
  RobScalars o{};
  EXPECT_FALSE(xmlight::deserialize(p, "RobScalars", o));
  EXPECT_EQ(p.errorCode(), want);
}

TEST_F(LightningBasicTests, RobustNumericSurroundingWhitespace) {
  const std::string_view xml = "<RobScalars><count> 42 </count></RobScalars>";
  expectCount<xmlight::Parser>(xml, 42);
  expectCount<xmlight::NormalizingParser>(xml, 42);
  expectCount<xmlight::StrictParser>(xml, 42);
}

TEST_F(LightningBasicTests, RobustNumericNewlineIndentation) {
  const std::string_view xml = "<RobScalars><count>\n    42\n  </count></RobScalars>";
  expectCount<xmlight::Parser>(xml, 42);
  expectCount<xmlight::NormalizingParser>(xml, 42);
  expectCount<xmlight::StrictParser>(xml, 42);
}

TEST_F(LightningBasicTests, RobustLeadingPlus) {
  expectCount<xmlight::Parser>("<RobScalars><count>+42</count></RobScalars>", 42);
  xmlight::Parser p{"<RobScalars><ratio>+3.5</ratio></RobScalars>"};
  RobScalars o{};
  ASSERT_TRUE(xmlight::deserialize(p, "RobScalars", o));
  EXPECT_DOUBLE_EQ(o.ratio, 3.5);
}

TEST_F(LightningBasicTests, RobustCommentSplitNumericReadWhole) {
  expectCount<xmlight::Parser>("<RobScalars><count>4<!--x-->2</count></RobScalars>", 42);
  expectCount<xmlight::NormalizingParser>("<RobScalars><count>4<!--x-->2</count></RobScalars>", 42);
  expectCount<xmlight::StrictParser>("<RobScalars><count>4<!--x-->2</count></RobScalars>", 42);
  expectCount<xmlight::Parser>("<RobScalars><count>1<!--a-->2<!--b-->3</count></RobScalars>", 123);
  expectCount<xmlight::Parser>("<RobScalars><count>1<?pi ?>2</count></RobScalars>", 12);
}

TEST_F(LightningBasicTests, RobustNumericInCData) {
  expectCount<xmlight::Parser>("<RobScalars><count><![CDATA[42]]></count></RobScalars>", 42);
  expectCount<xmlight::NormalizingParser>("<RobScalars><count><![CDATA[42]]></count></RobScalars>",
                                          42);
}

TEST_F(LightningBasicTests, RobustCharRefInTypedLeaf) {
  const std::string_view xml = "<RobScalars><count>4&#50;</count></RobScalars>";
  expectCount<xmlight::NormalizingParser>(xml, 42);
  expectCount<xmlight::StrictParser>(xml, 42);
  expectCountError<xmlight::Parser>(xml, xmlight::ErrorCode::InvalidNumericValue);
}

TEST_F(LightningBasicTests, RobustBoolSurroundingWhitespace) {
  xmlight::Parser p{"<RobScalars><flag> true </flag></RobScalars>"};
  RobScalars o{};
  ASSERT_TRUE(xmlight::deserialize(p, "RobScalars", o));
  EXPECT_TRUE(o.flag);
}

TEST_F(LightningBasicTests, RobustDateSurroundingWhitespace) {
  xmlight::Parser p{"<RobDate><date> 2026-06-28 </date></RobDate>"};
  RobDate o{};
  ASSERT_TRUE(xmlight::deserialize(p, "RobDate", o));
  EXPECT_EQ(o.date, (xmlight::Date{2026, 6, 28}));
}

TEST_F(LightningBasicTests, RobustTypedAttribute) {
  {
    xmlight::Parser p{"<RobAttr n=' 42 '/>"};
    RobAttr o{};
    ASSERT_TRUE(xmlight::deserialize(p, "RobAttr", o));
    EXPECT_EQ(o.n, 42);
  }
  {
    xmlight::Parser p{"<RobAttr n='abc'/>"};
    RobAttr o{};
    EXPECT_FALSE(xmlight::deserialize(p, "RobAttr", o));
    EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::InvalidNumericValue);
  }
  {
    xmlight::Parser p{"<RobAttr n=''/>"};
    RobAttr o{};
    EXPECT_FALSE(xmlight::deserialize(p, "RobAttr", o));
    EXPECT_EQ(p.errorCode(), xmlight::ErrorCode::InvalidNumericValue);
  }
}

TEST_F(LightningBasicTests, RobustOptionalTypedAttribute) {
  {
    xmlight::Parser p{"<RobOptAttr n=' 42 '/>"};
    RobOptAttr o{};
    ASSERT_TRUE(xmlight::deserialize(p, "RobOptAttr", o));
    ASSERT_TRUE(o.n.has_value());
    EXPECT_EQ(*o.n, 42);
  }
  {
    xmlight::Parser p{"<RobOptAttr n='abc'/>"};
    RobOptAttr o{};
    ASSERT_TRUE(xmlight::deserialize(p, "RobOptAttr", o));
    EXPECT_FALSE(o.n.has_value());
  }
  {
    xmlight::Parser p{"<RobOptAttr n=''/>"};
    RobOptAttr o{};
    ASSERT_TRUE(xmlight::deserialize(p, "RobOptAttr", o));
    EXPECT_FALSE(o.n.has_value());
  }
}

struct ValItem {
  std::string sku;
};
template<>
struct xmlight::XmlMetadata<ValItem> {
  static constexpr auto fields = std::make_tuple(xmlight::field("sku", &ValItem::sku));
};
template<>
struct xmlight::XmlConstraints<ValItem> {
  static auto check(const ValItem& v) -> std::optional<std::string> {
    if (v.sku.size() > 4) {
      return "sku: maxLength violation";
    }
    return {};
  }
};

struct ValOrder {
  int qty{};
  std::vector<ValItem> items;
  std::optional<ValItem> gift;
  std::unique_ptr<ValOrder> parent;
  std::variant<ValItem, int> pick{0};
};
template<>
struct xmlight::XmlMetadata<ValOrder> {
  static constexpr auto fields = std::make_tuple(
      xmlight::attrField("qty", &ValOrder::qty), xmlight::vecField("item", &ValOrder::items),
      xmlight::field("gift", &ValOrder::gift), xmlight::field("parent", &ValOrder::parent),
      xmlight::variantField(&ValOrder::pick, xmlight::alt<ValItem>("pickItem"),
                            xmlight::alt<int>("pickInt")));
};
template<>
struct xmlight::XmlConstraints<ValOrder> {
  static auto check(const ValOrder& v) -> std::optional<std::string> {
    if (v.qty < 0) {
      return "qty: minInclusive violation";
    }
    return {};
  }
};

TEST_F(LightningBasicTests, ValidateChecksOwnConstraintsFirst) {
  ValOrder o;
  o.qty = -1;
  o.items.push_back({"TOOLONG"});
  const auto err = xmlight::validate(o);
  ASSERT_TRUE(err.has_value());
  EXPECT_EQ(err->message, "qty: minInclusive violation");
}

TEST_F(LightningBasicTests, ValidateRecursesIntoContainers) {
  ValOrder o;
  o.items.push_back({"OK"});
  o.items.push_back({"TOOLONG"});
  const auto err = xmlight::validate(o);
  ASSERT_TRUE(err.has_value());
  EXPECT_EQ(err->message, "sku: maxLength violation");
}

TEST_F(LightningBasicTests, ValidateRecursesIntoOptionalAndUniquePtr) {
  ValOrder o;
  o.gift = ValItem{"TOOLONG"};
  EXPECT_TRUE(xmlight::validate(o).has_value());

  ValOrder p;
  p.parent = std::make_unique<ValOrder>();
  p.parent->qty = -5;
  EXPECT_TRUE(xmlight::validate(p).has_value());
}

TEST_F(LightningBasicTests, ValidateRecursesIntoVariants) {
  ValOrder o;
  o.pick = ValItem{"TOOLONG"};
  EXPECT_TRUE(xmlight::validate(o).has_value());
  o.pick = 7;
  EXPECT_FALSE(xmlight::validate(o).has_value());
}

TEST_F(LightningBasicTests, ValidatePassesWhenAllNestedValid) {
  ValOrder o;
  o.qty = 3;
  o.items.push_back({"ABCD"});
  o.gift = ValItem{"OK"};
  o.parent = std::make_unique<ValOrder>();
  EXPECT_FALSE(xmlight::validate(o).has_value());
}
