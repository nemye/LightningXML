/// @file test_Generated.cc
/// @brief Compiles and round-trips the code xsdgen produces from
/// test/schemas/kitchensink.xsd (generated at build time). A compile failure
/// here means the generator emitted invalid C++ for a supported construct.
#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "kitchensink_generated.hh"

namespace {

constexpr std::string_view DOC = R"(<inventory rev="r2" state="released">
  <item id="7">
    <createdBy>ian</createdBy>
    <sku>AB12</sku>
    <color>green</color>
    <qty>1</qty>
    <qty>2</qty>
    <price currency="USD">9.99</price>
    <tags>1 2 3</tags>
    <note>hello</note>
    <audit><who>bot</who></audit>
    <weight>1.5</weight>
    <origin>DE</origin>
  </item>
</inventory>)";

auto parsedDoc() -> Inventory {
  xmlight::Parser p{DOC};
  Inventory inv;
  EXPECT_TRUE(xmlight::deserialize(p, "inventory", inv));
  return inv;
}

}  // namespace

TEST(GeneratedCode, DeserializesKitchenSink) {
  const Inventory inv = parsedDoc();
  EXPECT_EQ(inv.rev, "r2");
  EXPECT_EQ(inv.state, State::released);
  ASSERT_EQ(inv.item.size(), 1U);
  const Item& it = inv.item.front();
  EXPECT_EQ(it.id, 7);
  EXPECT_EQ(it.createdBy, "ian");
  EXPECT_EQ(it.prio, Prio::High);
  EXPECT_EQ(it.sku, "AB12");
  ASSERT_TRUE(it.color.has_value());
  EXPECT_EQ(*it.color, Color::green);
  EXPECT_EQ(it.qty, (std::vector<int>{1, 2}));
  EXPECT_DOUBLE_EQ(it.price.value, 9.99);
  EXPECT_EQ(it.price.currency, "USD");
  EXPECT_EQ(it.tags, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(it.audit.who, "bot");
  EXPECT_EQ(it.variant, nullptr);
  ASSERT_TRUE(std::holds_alternative<double>(it.choice));
  EXPECT_DOUBLE_EQ(std::get<double>(it.choice), 1.5);
  ASSERT_TRUE(std::holds_alternative<std::string>(it.choice2));
  EXPECT_EQ(std::get<std::string>(it.choice2), "DE");
}

TEST(GeneratedCode, RoundTripsThroughSerializer) {
  const Inventory inv = parsedDoc();
  const std::string xml = xmlight::serialize("inventory", inv);
  xmlight::Parser p{xml};
  Inventory again;
  ASSERT_TRUE(xmlight::deserialize(p, "inventory", again));
  ASSERT_EQ(again.item.size(), 1U);
  const Item& it = again.item.front();
  EXPECT_EQ(it.sku, "AB12");
  EXPECT_EQ(it.qty, (std::vector<int>{1, 2}));
  EXPECT_EQ(it.tags, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(it.audit.who, "bot");
  ASSERT_TRUE(std::holds_alternative<std::string>(it.choice2));
  EXPECT_EQ(std::get<std::string>(it.choice2), "DE");
}

TEST(GeneratedCode, ValidatePassesOnValidDocument) {
  const Inventory inv = parsedDoc();
  EXPECT_FALSE(xmlight::validate(inv).has_value());
}

TEST(GeneratedCode, ValidateReachesNestedFacetViolations) {
  Inventory inv = parsedDoc();
  inv.item.front().sku = "waytoolongsku";
  const auto err = xmlight::validate(inv);
  ASSERT_TRUE(err.has_value());
  EXPECT_NE(err->message.find("sku"), std::string::npos) << err->message;
}

TEST(GeneratedCode, ValidateReachesNestedMaxOccursViolation) {
  Inventory inv = parsedDoc();
  inv.item.front().qty.assign(5, 1);
  EXPECT_TRUE(xmlight::validate(inv).has_value());
}

TEST(GeneratedCode, ValidateReachesNestedFixedViolation) {
  Inventory inv = parsedDoc();
  inv.item.front().prio = Prio::Low;
  EXPECT_TRUE(xmlight::validate(inv).has_value());
}
