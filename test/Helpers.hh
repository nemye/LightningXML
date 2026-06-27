#pragma once
#include <string_view>
#include <vector>

#include "TurboXML.hh"

struct Skills {
  std::vector<std::string_view> items;
};

template<>
struct xml::XmlMetadata<Skills> {
  static constexpr auto fields = std::make_tuple(xml::vecField("Skill", &Skills::items));
};

struct OrgMember {
  int id{0};
  std::string_view role;
  std::string_view full_name;
  std::string_view email;
  Skills skills;
};

template<>
struct xml::XmlMetadata<OrgMember> {
  static constexpr auto fields = std::make_tuple(xml::attrField("id", &OrgMember::id),
                                                 xml::attrField("role", &OrgMember::role),
                                                 xml::field("FullName", &OrgMember::full_name),
                                                 xml::field("Email", &OrgMember::email),
                                                 xml::field("Skills", &OrgMember::skills));
};

struct OrgTeam {
  int id{0};
  std::string_view name;
  std::vector<OrgMember> members;
};

template<>
struct xml::XmlMetadata<OrgTeam> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("id", &OrgTeam::id), xml::attrField("name", &OrgTeam::name),
                      xml::vecField("Member", &OrgTeam::members));
};

struct OrgDepartment {
  int id{0};
  std::string_view name;
  std::vector<OrgTeam> teams;
};

template<>
struct xml::XmlMetadata<OrgDepartment> {
  static constexpr auto fields = std::make_tuple(xml::attrField("id", &OrgDepartment::id),
                                                 xml::attrField("name", &OrgDepartment::name),
                                                 xml::vecField("Team", &OrgDepartment::teams));
};

struct Organization {
  int id{0};
  std::string_view name;
  std::vector<OrgDepartment> departments;
};

template<>
struct xml::XmlMetadata<Organization> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("id", &Organization::id),
                      xml::attrField("name", &Organization::name),
                      xml::vecField("Department", &Organization::departments));
};

struct User {
  int id{0};
  std::string_view name;
  std::string_view email;
};

template<>
struct xml::XmlMetadata<User> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("id", &User::id), xml::field("Name", &User::name),
                      xml::field("Email", &User::email));
};

struct Users {
  std::vector<User> items;
};

template<>
struct xml::XmlMetadata<Users> {
  static constexpr auto fields = std::make_tuple(xml::vecField("User", &Users::items));
};

struct Address {
  std::string_view street;
  int zip{};
};

template<>
struct xml::XmlMetadata<Address> {
  static constexpr auto fields =
      std::make_tuple(xml::field("street", &Address::street), xml::field("zip", &Address::zip));
};

struct Person {
  std::string_view name;
  int age{};
  Address address;
};

template<>
struct xml::XmlMetadata<Person> {
  static constexpr auto fields =
      std::make_tuple(xml::field("name", &Person::name), xml::field("age", &Person::age),
                      xml::field("address", &Person::address));
};

struct FlatItem {
  int id{};
  std::string_view title{};
  std::string_view description{};
  int status{};
};

template<>
struct xml::XmlMetadata<FlatItem> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("id", &FlatItem::id), xml::field("title", &FlatItem::title),
                      xml::field("desc", &FlatItem::description),
                      xml::field("status", &FlatItem::status));
};

struct FlatList {
  std::vector<FlatItem> items;
};

template<>
struct xml::XmlMetadata<FlatList> {
  static constexpr auto fields = std::make_tuple(xml::vecField("Item", &FlatList::items));
};

struct DeepL5 {
  int value{};
};
template<>
struct xml::XmlMetadata<DeepL5> {
  static constexpr auto fields = std::make_tuple(xml::field("v", &DeepL5::value));
};

struct DeepL4 {
  DeepL5 next;
};
template<>
struct xml::XmlMetadata<DeepL4> {
  static constexpr auto fields = std::make_tuple(xml::field("L5", &DeepL4::next));
};

struct DeepL3 {
  DeepL4 next;
};
template<>
struct xml::XmlMetadata<DeepL3> {
  static constexpr auto fields = std::make_tuple(xml::field("L4", &DeepL3::next));
};

struct DeepL2 {
  DeepL3 next;
};
template<>
struct xml::XmlMetadata<DeepL2> {
  static constexpr auto fields = std::make_tuple(xml::field("L3", &DeepL2::next));
};

struct DeepL1 {
  DeepL2 next;
};
template<>
struct xml::XmlMetadata<DeepL1> {
  static constexpr auto fields = std::make_tuple(xml::field("L2", &DeepL1::next));
};

struct DeepList {
  std::vector<DeepL1> items;
};
template<>
struct xml::XmlMetadata<DeepList> {
  static constexpr auto fields = std::make_tuple(xml::vecField("L1", &DeepList::items));
};

struct AttrItem {
  int a1{}, a2{}, a3{}, a4{}, a5{};
  std::string_view s1, s2, s3, s4, s5;
};

template<>
struct xml::XmlMetadata<AttrItem> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("a1", &AttrItem::a1), xml::attrField("a2", &AttrItem::a2),
                      xml::attrField("a3", &AttrItem::a3), xml::attrField("a4", &AttrItem::a4),
                      xml::attrField("a5", &AttrItem::a5), xml::attrField("s1", &AttrItem::s1),
                      xml::attrField("s2", &AttrItem::s2), xml::attrField("s3", &AttrItem::s3),
                      xml::attrField("s4", &AttrItem::s4), xml::attrField("s5", &AttrItem::s5));
};

struct AttrList {
  std::vector<AttrItem> items;
};

template<>
struct xml::XmlMetadata<AttrList> {
  static constexpr auto fields = std::make_tuple(xml::vecField("Item", &AttrList::items));
};

struct TreeNode {
  std::vector<TreeNode> children;
};

template<>
struct xml::XmlMetadata<TreeNode> {
  static constexpr auto fields = std::make_tuple(xml::vecField("Node", &TreeNode::children));
};

struct OwnedPerson {
  std::string name;
  int age{};
  std::string email;
};

template<>
struct xml::XmlMetadata<OwnedPerson> {
  static constexpr auto fields =
      std::make_tuple(xml::field("name", &OwnedPerson::name), xml::field("age", &OwnedPerson::age),
                      xml::field("email", &OwnedPerson::email));
};

struct OwnedUser {
  int id{};
  std::string role;
  std::string name;
};

template<>
struct xml::XmlMetadata<OwnedUser> {
  static constexpr auto fields = std::make_tuple(xml::attrField("id", &OwnedUser::id),
                                                 xml::attrField("role", &OwnedUser::role),
                                                 xml::field("Name", &OwnedUser::name));
};

struct OwnedList {
  std::vector<std::string> tags;
};

template<>
struct xml::XmlMetadata<OwnedList> {
  static constexpr auto fields = std::make_tuple(xml::vecField("Tag", &OwnedList::tags));
};

struct Book {
  std::string id;
  std::string author;
  std::string title;
  std::string genre;
  std::string price;
  std::string publish_date;
  std::string description;
};

template<>
struct xml::XmlMetadata<Book> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("id", &Book::id), xml::field("author", &Book::author),
                      xml::field("title", &Book::title), xml::field("genre", &Book::genre),
                      xml::field("price", &Book::price),
                      xml::field("publish_date", &Book::publish_date),
                      xml::field("description", &Book::description));
};

struct Catalog {
  std::vector<Book> books;
};

template<>
struct xml::XmlMetadata<Catalog> {
  static constexpr auto fields = std::make_tuple(xml::vecField("book", &Catalog::books));
};

struct FixedSkills {
  std::array<std::string_view, 3> items{};
};

template<>
struct xml::XmlMetadata<FixedSkills> {
  static constexpr auto fields = std::make_tuple(xml::arrField("Skill", &FixedSkills::items));
};

struct Toggle {
  bool enabled{};
  bool active{};
  bool verbose{};
};

template<>
struct xml::XmlMetadata<Toggle> {
  static constexpr auto fields = std::make_tuple(xml::attrField("enabled", &Toggle::enabled),
                                                 xml::field("active", &Toggle::active),
                                                 xml::field("verbose", &Toggle::verbose));
};

struct MixedRecord {
  int id{};
  std::string_view name;
  std::array<int, 4> scores{};
};

template<>
struct xml::XmlMetadata<MixedRecord> {
  static constexpr auto fields = std::make_tuple(xml::attrField("id", &MixedRecord::id),
                                                 xml::field("Name", &MixedRecord::name),
                                                 xml::arrField("Score", &MixedRecord::scores));
};