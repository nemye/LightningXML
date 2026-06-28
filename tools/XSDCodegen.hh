/// @file XSDCodegen.hh
/// @brief Generates TurboXML XmlMetadata definitions from an XSD schema.
///
/// The schema is itself parsed with TurboXML (XSD is XML; the xs: prefix is
/// matched by local name). A practical subset is supported; anything outside it
/// is recorded as a note rather than dropped silently or treated as fatal.
#pragma once

#include <array>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "TurboXML.hh"

namespace xsd {

// ---- Parsed XSD model (populated by TurboXML) ----

struct Enumeration {
  std::string value;
};

struct Facet {
  std::string value;
};

struct Restriction {
  std::string base;
  std::vector<Enumeration> enumerations;
  std::optional<Facet> min_length;
  std::optional<Facet> max_length;
  std::optional<Facet> length;
  std::optional<Facet> pattern;
  std::optional<Facet> min_inclusive;
  std::optional<Facet> max_inclusive;
  std::optional<Facet> min_exclusive;
  std::optional<Facet> max_exclusive;
  std::optional<Facet> fraction_digits;
  std::optional<Facet> total_digits;
};

struct List {
  std::string item_type;
};

struct SimpleType {
  std::string name;
  std::optional<Restriction> restriction;
  std::optional<List> list;
};

struct Attribute {
  std::string name;
  std::string type;
  std::string use;
  std::string default_;  // NOLINT(readability-identifier-naming)
  std::string fixed;
};

struct AttributeGroupRef {
  std::string ref;
};

struct AttributeGroupDef {
  std::string name;
  std::vector<Attribute> attributes;
};

struct GroupRef {
  std::string ref;
};

struct Element;  // recursive via the content-model containers below

struct Choice {
  std::optional<std::string> max_occurs;
  std::vector<Element> elements;
};

struct Sequence {
  std::vector<Element> elements;
  std::vector<Choice> choices;
  std::vector<GroupRef> group_refs;
};

struct GroupDef {
  std::string name;
  std::optional<Sequence> sequence;
  std::optional<Choice> choice;
};

struct Extension {
  std::string base;
  std::vector<Attribute> attributes;
};

struct SimpleContent {
  std::optional<Extension> extension;
};

struct ComplexExtension {
  std::string base;
  std::optional<Sequence> sequence;
  std::optional<Choice> choice;
  std::vector<Attribute> attributes;
  std::vector<AttributeGroupRef> attribute_group_refs;
};

struct ComplexContent {
  std::optional<ComplexExtension> extension;
};

struct ComplexType {
  std::string name;
  bool mixed{};
  std::optional<Sequence> sequence;
  std::optional<Choice> choice;
  std::optional<SimpleContent> simple_content;
  std::optional<ComplexContent> complex_content;
  std::vector<Attribute> attributes;
  std::vector<AttributeGroupRef> attribute_group_refs;
};

struct Element {
  std::string name;
  std::string type;
  std::optional<int> min_occurs;
  std::optional<std::string> max_occurs;
  std::optional<ComplexType> complex_type;
  std::optional<SimpleType> simple_type;
};

struct Include {
  std::string schema_location;
};

struct Schema {
  std::vector<Element> elements;
  std::vector<ComplexType> complex_types;
  std::vector<SimpleType> simple_types;
  std::vector<AttributeGroupDef> attribute_groups;
  std::vector<GroupDef> groups;
  std::vector<Include> includes;
};

}  // namespace xsd

template<>
struct xml::XmlMetadata<xsd::Enumeration> {
  static constexpr auto fields = std::make_tuple(xml::attrField("value", &xsd::Enumeration::value));
};
template<>
struct xml::XmlMetadata<xsd::Facet> {
  static constexpr auto fields = std::make_tuple(xml::attrField("value", &xsd::Facet::value));
};
template<>
struct xml::XmlMetadata<xsd::Restriction> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("base", &xsd::Restriction::base),
                      xml::vecField("enumeration", &xsd::Restriction::enumerations),
                      xml::field("minLength", &xsd::Restriction::min_length),
                      xml::field("maxLength", &xsd::Restriction::max_length),
                      xml::field("length", &xsd::Restriction::length),
                      xml::field("pattern", &xsd::Restriction::pattern),
                      xml::field("minInclusive", &xsd::Restriction::min_inclusive),
                      xml::field("maxInclusive", &xsd::Restriction::max_inclusive),
                      xml::field("minExclusive", &xsd::Restriction::min_exclusive),
                      xml::field("maxExclusive", &xsd::Restriction::max_exclusive),
                      xml::field("fractionDigits", &xsd::Restriction::fraction_digits),
                      xml::field("totalDigits", &xsd::Restriction::total_digits));
};
template<>
struct xml::XmlMetadata<xsd::List> {
  static constexpr auto fields = std::make_tuple(xml::attrField("itemType", &xsd::List::item_type));
};
template<>
struct xml::XmlMetadata<xsd::SimpleType> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("name", &xsd::SimpleType::name),
                      xml::field("restriction", &xsd::SimpleType::restriction),
                      xml::field("list", &xsd::SimpleType::list));
};
template<>
struct xml::XmlMetadata<xsd::Attribute> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("name", &xsd::Attribute::name),
                      xml::attrField("type", &xsd::Attribute::type),
                      xml::attrField("use", &xsd::Attribute::use),
                      xml::attrField("default", &xsd::Attribute::default_),
                      xml::attrField("fixed", &xsd::Attribute::fixed));
};
template<>
struct xml::XmlMetadata<xsd::AttributeGroupRef> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("ref", &xsd::AttributeGroupRef::ref));
};
template<>
struct xml::XmlMetadata<xsd::AttributeGroupDef> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("name", &xsd::AttributeGroupDef::name),
                      xml::vecField("attribute", &xsd::AttributeGroupDef::attributes));
};
template<>
struct xml::XmlMetadata<xsd::GroupRef> {
  static constexpr auto fields = std::make_tuple(xml::attrField("ref", &xsd::GroupRef::ref));
};
template<>
struct xml::XmlMetadata<xsd::Choice> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("maxOccurs", &xsd::Choice::max_occurs),
                      xml::vecField("element", &xsd::Choice::elements));
};
template<>
struct xml::XmlMetadata<xsd::Sequence> {
  static constexpr auto fields =
      std::make_tuple(xml::vecField("element", &xsd::Sequence::elements),
                      xml::vecField("choice", &xsd::Sequence::choices),
                      xml::vecField("group", &xsd::Sequence::group_refs));
};
template<>
struct xml::XmlMetadata<xsd::GroupDef> {
  static constexpr auto fields = std::make_tuple(xml::attrField("name", &xsd::GroupDef::name),
                                                 xml::field("sequence", &xsd::GroupDef::sequence),
                                                 xml::field("choice", &xsd::GroupDef::choice));
};
template<>
struct xml::XmlMetadata<xsd::Extension> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("base", &xsd::Extension::base),
                      xml::vecField("attribute", &xsd::Extension::attributes));
};
template<>
struct xml::XmlMetadata<xsd::SimpleContent> {
  static constexpr auto fields =
      std::make_tuple(xml::field("extension", &xsd::SimpleContent::extension));
};
template<>
struct xml::XmlMetadata<xsd::ComplexExtension> {
  static constexpr auto fields = std::make_tuple(
      xml::attrField("base", &xsd::ComplexExtension::base),
      xml::field("sequence", &xsd::ComplexExtension::sequence),
      xml::field("choice", &xsd::ComplexExtension::choice),
      xml::vecField("attribute", &xsd::ComplexExtension::attributes),
      xml::vecField("attributeGroup", &xsd::ComplexExtension::attribute_group_refs));
};
template<>
struct xml::XmlMetadata<xsd::ComplexContent> {
  static constexpr auto fields =
      std::make_tuple(xml::field("extension", &xsd::ComplexContent::extension));
};
template<>
struct xml::XmlMetadata<xsd::ComplexType> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("name", &xsd::ComplexType::name),
                      xml::attrField("mixed", &xsd::ComplexType::mixed),
                      xml::field("sequence", &xsd::ComplexType::sequence),
                      xml::field("choice", &xsd::ComplexType::choice),
                      xml::field("simpleContent", &xsd::ComplexType::simple_content),
                      xml::field("complexContent", &xsd::ComplexType::complex_content),
                      xml::vecField("attribute", &xsd::ComplexType::attributes),
                      xml::vecField("attributeGroup", &xsd::ComplexType::attribute_group_refs));
};
template<>
struct xml::XmlMetadata<xsd::Element> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("name", &xsd::Element::name),
                      xml::attrField("type", &xsd::Element::type),
                      xml::attrField("minOccurs", &xsd::Element::min_occurs),
                      xml::attrField("maxOccurs", &xsd::Element::max_occurs),
                      xml::field("complexType", &xsd::Element::complex_type),
                      xml::field("simpleType", &xsd::Element::simple_type));
};
template<>
struct xml::XmlMetadata<xsd::Include> {
  static constexpr auto fields =
      std::make_tuple(xml::attrField("schemaLocation", &xsd::Include::schema_location));
};
template<>
struct xml::XmlMetadata<xsd::Schema> {
  static constexpr auto fields =
      std::make_tuple(xml::vecField("element", &xsd::Schema::elements),
                      xml::vecField("complexType", &xsd::Schema::complex_types),
                      xml::vecField("simpleType", &xsd::Schema::simple_types),
                      xml::vecField("attributeGroup", &xsd::Schema::attribute_groups),
                      xml::vecField("group", &xsd::Schema::groups),
                      xml::vecField("include", &xsd::Schema::includes));
};

namespace xsd {

struct Options {
  // If provided, called with each xs:include schemaLocation to load external
  // schema text. Return nullopt to silently skip an include.
  std::function<std::optional<std::string>(std::string_view)> loader;
};

struct GenResult {
  std::string code;
  std::vector<std::string> notes;
  bool ok{};
};

namespace detail {

// ---- small string helpers ----

inline auto localName(std::string_view s) -> std::string_view {
  const auto pos = s.rfind(':');
  return pos == std::string_view::npos ? s : s.substr(pos + 1);
}

inline auto isIdentStart(char c) -> bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
inline auto isIdentChar(char c) -> bool {
  return isIdentStart(c) || (c >= '0' && c <= '9');
}

// Turn an XML name into a valid C++ identifier (invalid chars -> '_').
inline auto sanitize(std::string_view name) -> std::string {
  std::string out;
  out.reserve(name.size() + 1);
  for (const char c : name) {
    out.push_back(isIdentChar(c) ? c : '_');
  }
  if (out.empty() || !isIdentStart(out.front())) {
    out.insert(out.begin(), '_');
  }
  return out;
}

inline auto capitalize(std::string_view name) -> std::string {
  std::string out = sanitize(name);
  if (!out.empty() && out.front() >= 'a' && out.front() <= 'z') {
    out.front() = static_cast<char>(out.front() - 'a' + 'A');
  }
  return out;
}

// XSD built-in -> C++ type, or "" if not a known built-in.
inline auto builtinType(std::string_view xsd_type) -> std::string {
  static const std::unordered_map<std::string_view, std::string_view> k_map = {
      {"string", "std::string"},
      {"normalizedString", "std::string"},
      {"token", "std::string"},
      {"anyURI", "std::string"},
      {"NMTOKEN", "std::string"},
      {"Name", "std::string"},
      {"NCName", "std::string"},
      {"ID", "std::string"},
      {"IDREF", "std::string"},
      {"language", "std::string"},
      {"boolean", "bool"},
      {"int", "int"},
      {"integer", "long"},
      {"long", "long"},
      {"short", "short"},
      {"byte", "signed char"},
      {"unsignedInt", "unsigned"},
      {"unsignedLong", "unsigned long"},
      {"unsignedShort", "unsigned short"},
      {"nonNegativeInteger", "long"},
      {"positiveInteger", "long"},
      {"decimal", "double"},
      {"double", "double"},
      {"float", "float"},
      {"date", "xml::Date"},
      {"time", "xml::Time"},
      {"dateTime", "xml::DateTime"}};
  const auto it = k_map.find(localName(xsd_type));
  return it == k_map.end() ? std::string{} : std::string{it->second};
}

}  // namespace detail

// ---- Generator ----

class Generator {
 public:
  explicit Generator(const Schema& schema) : schema_(schema) {}

  auto run() -> GenResult {
    indexNamedTypes();
    for (const auto& st : schema_.simple_types) {
      collectSimpleType(st.name, st);
    }
    for (const auto& ct : schema_.complex_types) {
      collectComplexType(detail::capitalize(ct.name), ct);
    }
    for (const auto& el : schema_.elements) {
      collectElementInline(el);
    }
    orderStructs();
    // Build constraint specializations before emit() so needs_regex_ is known.
    for (const auto& s : structs_) {
      emitStructConstraints(s);
    }
    emit();
    GenResult r;
    r.code = std::move(out_);
    r.notes = std::move(notes_);
    r.ok = true;
    return r;
  }

 private:
  struct StructDef {
    std::string cpp_name;
    const ComplexType* ct;
    std::string parent_xsd;  // XSD key (named_complex_ key) of the base type; "" if none
  };
  struct EnumDef {
    std::string cpp_name;
    std::vector<std::string> tokens;  // original XML token spellings
  };
  struct FieldOut {
    std::string member;    // C++ member declaration, e.g. "std::string name;"
    std::string metadata;  // metadata call, e.g. xml::field("name", &T::name)
    bool inherited{};      // from a base class; omit from struct body but keep in metadata
  };

  // Deduplicated: fieldsOf() is computed several times (ordering, struct, and
  // metadata emission), so the same diagnostic can be produced more than once.
  auto note(std::string msg) -> void {
    if (note_seen_.insert(msg).second) {
      notes_.push_back(std::move(msg));
    }
  }

  auto indexNamedTypes() -> void {
    for (const auto& ct : schema_.complex_types) {
      if (!ct.name.empty()) {
        named_complex_[ct.name] = &ct;
      }
    }
    for (const auto& st : schema_.simple_types) {
      if (!st.name.empty()) {
        named_simple_[st.name] = &st;
      }
    }
    for (const auto& ag : schema_.attribute_groups) {
      if (!ag.name.empty()) {
        named_attr_groups_[ag.name] = &ag;
      }
    }
    for (const auto& g : schema_.groups) {
      if (!g.name.empty()) {
        named_groups_[g.name] = &g;
      }
    }
  }

  auto collectSimpleType(std::string_view xml_name, const SimpleType& st) -> std::string {
    if (!st.restriction || st.restriction->enumerations.empty()) {
      return {};
    }
    const std::string cpp = uniqueName(detail::capitalize(xml_name));
    EnumDef e;
    e.cpp_name = cpp;
    for (const auto& en : st.restriction->enumerations) {
      e.tokens.push_back(en.value);
    }
    enums_.push_back(std::move(e));
    return cpp;
  }

  auto collectComplexType(const std::string& cpp_name, const ComplexType& ct) -> void {
    if (emitted_.contains(cpp_name)) {
      return;
    }
    emitted_.insert(cpp_name);

    std::string parent_xsd;
    if (ct.complex_content && ct.complex_content->extension) {
      const std::string& base = ct.complex_content->extension->base;
      parent_xsd = std::string{detail::localName(base)};
      if (auto it = named_complex_.find(parent_xsd); it != named_complex_.end()) {
        collectComplexType(detail::capitalize(parent_xsd), *it->second);
      } else {
        note("complexContent base '" + base + "' not found in schema");
      }
    }

    structs_.emplace_back(cpp_name, &ct, parent_xsd);

    if (ct.mixed) {
      note("mixed content on complexType '" + ct.name +
           "' is unsupported; text between child elements is ignored");
    }

    // Collect inline types from wherever they live in this CT.
    auto collect_seq = [&](const Sequence& seq) {
      for (const auto& el : seq.elements) {
        collectElementInline(el);
      }
      for (const auto& ch : seq.choices) {
        for (const auto& el : ch.elements) {
          collectElementInline(el);
        }
      }
    };

    if (ct.complex_content && ct.complex_content->extension) {
      const auto& ext = *ct.complex_content->extension;
      if (ext.sequence) {
        collect_seq(*ext.sequence);
      }
      if (ext.choice) {
        for (const auto& el : ext.choice->elements) {
          collectElementInline(el);
        }
      }
    } else {
      if (ct.sequence) {
        collect_seq(*ct.sequence);
      }
      if (ct.choice) {
        for (const auto& el : ct.choice->elements) {
          collectElementInline(el);
        }
      }
    }
  }

  auto collectElementInline(const Element& el) -> void {
    if (el.complex_type) {
      collectComplexType(uniqueName(detail::capitalize(el.name)), *el.complex_type);
    } else if (el.simple_type) {
      collectSimpleType(el.name, *el.simple_type);
    }
  }

  auto uniqueName(const std::string& base) -> std::string {
    std::string name = base;
    int n = 2;
    while (used_names_.contains(name)) {
      name = std::format("{}{}", base, n++);
    }
    used_names_.insert(name);
    return name;
  }

  // ---- type resolution ----

  struct Resolved {
    std::string cpp;
    bool is_struct{};
    bool is_list{};
  };

  auto resolveListItem(std::string_view item_type) const -> std::string {
    if (std::string b = detail::builtinType(item_type); !b.empty()) {
      return b;
    }
    const std::string key{detail::localName(item_type)};
    if (auto it = named_simple_.find(key); it != named_simple_.end()) {
      if (it->second->restriction && !it->second->restriction->enumerations.empty()) {
        return detail::capitalize(key);
      }
      return baseBuiltin(it->second->restriction ? it->second->restriction->base : "string");
    }
    return "std::string";
  }

  static auto builtinList(std::string_view type) -> bool {
    const auto n = detail::localName(type);
    return n == "NMTOKENS" || n == "IDREFS" || n == "ENTITIES";
  }

  auto resolveElementType(const Element& el) -> Resolved {
    if (el.complex_type) {
      return {detail::capitalize(el.name), true, false};
    }
    if (el.simple_type) {
      if (el.simple_type->list) {
        return {resolveListItem(el.simple_type->list->item_type), false, true};
      }
      if (const std::string e = enumNameForInline(el); !e.empty()) {
        return {e, false, false};
      }
      if (el.simple_type->restriction) {
        return {baseBuiltin(el.simple_type->restriction->base), false, false};
      }
      note("untyped inline simpleType on '" + el.name + "' -> std::string");
      return {"std::string", false, false};
    }
    return resolveNamedType(el.type, el.name);
  }

  static auto enumNameForInline(const Element& el) -> std::string {
    if (!el.simple_type || !el.simple_type->restriction ||
        el.simple_type->restriction->enumerations.empty()) {
      return {};
    }
    return detail::capitalize(el.name);
  }

  auto baseBuiltin(std::string_view base) const -> std::string {
    if (std::string b = detail::builtinType(base); !b.empty()) {
      return b;
    }
    if (auto it = named_simple_.find(std::string{detail::localName(base)});
        it != named_simple_.end() && it->second->restriction) {
      return baseBuiltin(it->second->restriction->base);
    }
    return "std::string";
  }

  auto resolveNamedType(std::string_view type, std::string_view ctx) -> Resolved {
    if (type.empty()) {
      note("element '" + std::string{ctx} + "' has no type -> std::string");
      return {"std::string", false, false};
    }
    if (builtinList(type)) {
      return {"std::string", false, true};
    }
    if (const std::string b = detail::builtinType(type); !b.empty()) {
      return {b, false, false};
    }
    const std::string key{detail::localName(type)};
    if (auto it = named_complex_.find(key); it != named_complex_.end()) {
      return {detail::capitalize(key), true, false};
    }
    if (auto it = named_simple_.find(key); it != named_simple_.end()) {
      if (it->second->list) {
        return {resolveListItem(it->second->list->item_type), false, true};
      }
      if (it->second->restriction && !it->second->restriction->enumerations.empty()) {
        return {detail::capitalize(key), false, false};
      }
      return {baseBuiltin(it->second->restriction ? it->second->restriction->base : "string"),
              false, false};
    }
    note("unknown type '" + std::string{type} + "' on '" + std::string{ctx} + "' -> std::string");
    return {"std::string", false, false};
  }

  // ---- cardinality ----

  static auto isUnbounded(const Element& el) -> bool {
    return el.max_occurs &&
           (*el.max_occurs == "unbounded" ||
            (!el.max_occurs->empty() && *el.max_occurs != "0" && *el.max_occurs != "1"));
  }
  static auto minOccursOf(const Element& el) -> int { return el.min_occurs.value_or(1); }

  // Returns the finite max_occurs bound when it's an integer > 1; empty otherwise.
  static auto finiteMaxOccurs(const Element& el) -> std::optional<int> {
    if (!el.max_occurs || el.max_occurs->empty() || *el.max_occurs == "unbounded") {
      return {};
    }
    try {
      const int n = std::stoi(*el.max_occurs);
      return n > 1 ? std::optional<int>{n} : std::optional<int>{};
    } catch (...) {
      return {};
    }
  }

  // ---- field generation ----

  auto addAttrField(const std::string& ctx, const Attribute& a,
                    std::vector<FieldOut>& out) -> void {
    const Resolved r = resolveNamedType(a.type, a.name);
    const std::string mem = detail::sanitize(a.name);
    const std::string suffix = (a.use == "required") ? ", true)" : ")";
    std::string decl;
    const std::string& init_val = !a.fixed.empty() ? a.fixed : a.default_;
    if (r.is_list) {
      decl = "std::vector<" + r.cpp + "> " + mem + ";";
    } else if (!init_val.empty()) {
      if (isStringCpp(r.cpp)) {
        decl = r.cpp + " " + mem + "{\"" + escapeStrLiteral(init_val) + "\"};";
      } else {
        decl = r.cpp + " " + mem + "{" + init_val + "};";
      }
    } else {
      decl = r.cpp + " " + mem + "{};";
    }
    out.push_back({decl, "xml::attrField(\"" + a.name + "\", &" + ctx + "::" + mem + suffix});
  }

  auto addElementField(const std::string& ctx, const Element& el,
                       std::vector<FieldOut>& out) -> void {
    const Resolved r = resolveElementType(el);
    const std::string mem = detail::sanitize(el.name);
    const std::string cpp = r.cpp;
    if (r.is_list) {
      if (isUnbounded(el)) {
        note("repeated xs:list element '" + el.name +
             "' (maxOccurs>1) is unsupported; treated as a single list");
      }
      out.push_back({"std::vector<" + cpp + "> " + mem + ";",
                     "xml::listField(\"" + el.name + "\", &" + ctx + "::" + mem +
                         (minOccursOf(el) >= 1 ? ", true)" : ")")});
      return;
    }
    if (isUnbounded(el)) {
      out.push_back({"std::vector<" + cpp + "> " + mem + ";",
                     "xml::vecField(\"" + el.name + "\", &" + ctx + "::" + mem +
                         (minOccursOf(el) >= 1 ? ", true)" : ")")});
    } else if (r.is_struct && r.cpp == ctx) {
      // Direct self-reference: break the cycle with unique_ptr.
      out.push_back({"std::unique_ptr<" + cpp + "> " + mem + ";",
                     "xml::field(\"" + el.name + "\", &" + ctx + "::" + mem + ")"});
    } else if (minOccursOf(el) == 0) {
      out.push_back({"std::optional<" + cpp + "> " + mem + ";",
                     "xml::field(\"" + el.name + "\", &" + ctx + "::" + mem + ")"});
    } else {
      out.push_back({cpp + " " + mem + "{};",
                     "xml::field(\"" + el.name + "\", &" + ctx + "::" + mem + ", true)"});
    }
  }

  auto addChoiceFields(const std::string& ctx, const Choice& ch,
                       std::vector<FieldOut>& out) -> void {
    std::vector<std::string> types;
    std::vector<std::string> alts;
    std::unordered_set<std::string> seen;
    for (const auto& el : ch.elements) {
      const Resolved r = resolveElementType(el);
      if (!seen.insert(r.cpp).second) {
        note("xs:choice in '" + ctx + "' has two branches of the same C++ type ('" + r.cpp +
             "'); the choice was skipped (std::variant needs distinct types)");
        return;
      }
      types.push_back(r.cpp);
      alts.push_back("xml::alt<" + r.cpp + ">(\"" + el.name + "\")");
    }
    if (types.empty()) {
      return;
    }

    std::string variant = "std::variant<";
    for (size_t i = 0; i < types.size(); ++i) {
      variant += types[i] + (i + 1 < types.size() ? ", " : "");
    }
    variant += ">";

    const bool repeated = ch.max_occurs && (*ch.max_occurs == "unbounded" || *ch.max_occurs != "1");
    const std::string member_type = repeated ? "std::vector<" + variant + ">" : variant;
    std::string alts_joined;
    for (size_t i = 0; i < alts.size(); ++i) {
      alts_joined += "\n          " + alts[i] + (i + 1 < alts.size() ? "," : "");
    }

    out.push_back(
        {member_type + " choice;", "xml::variantField(&" + ctx + "::choice," + alts_joined + ")"});
  }

  auto expandAttrGroups(const std::string& ctx, const std::vector<AttributeGroupRef>& refs,
                        std::vector<FieldOut>& out) -> void {
    for (const auto& agr : refs) {
      const std::string ref_key{detail::localName(agr.ref)};
      if (auto it = named_attr_groups_.find(ref_key); it != named_attr_groups_.end()) {
        for (const auto& a : it->second->attributes) {
          addAttrField(ctx, a, out);
        }
      } else {
        note("attributeGroup ref='" + agr.ref + "' not found in schema");
      }
    }
  }

  auto expandGroupRef(const std::string& ctx, const GroupRef& gr,
                      std::vector<FieldOut>& out) -> void {
    const std::string ref_key{detail::localName(gr.ref)};
    if (auto it = named_groups_.find(ref_key); it != named_groups_.end()) {
      const GroupDef& gd = *it->second;
      if (gd.sequence) {
        for (const auto& el : gd.sequence->elements) {
          addElementField(ctx, el, out);
        }
        for (const auto& ch : gd.sequence->choices) {
          addChoiceFields(ctx, ch, out);
        }
      }
      if (gd.choice) {
        addChoiceFields(ctx, *gd.choice, out);
      }
    } else {
      note("group ref='" + gr.ref + "' not found in schema");
    }
  }

  // Generates the own (non-inherited) fields of a complexType. ctx is the C++
  // struct name used for pointer-to-member references in metadata.
  auto ownFieldsOf(const std::string& ctx, const ComplexType& ct) -> std::vector<FieldOut> {
    std::vector<FieldOut> out;

    if (ct.simple_content && ct.simple_content->extension) {
      const auto& ext = *ct.simple_content->extension;
      out.push_back({baseBuiltin(ext.base) + " value{};", "xml::valueField(&" + ctx + "::value)"});
      for (const auto& a : ext.attributes) {
        addAttrField(ctx, a, out);
      }
      return out;
    }

    if (ct.complex_content && ct.complex_content->extension) {
      const auto& ext = *ct.complex_content->extension;
      for (const auto& a : ext.attributes) {
        addAttrField(ctx, a, out);
      }
      expandAttrGroups(ctx, ext.attribute_group_refs, out);
      if (ext.sequence) {
        for (const auto& el : ext.sequence->elements) {
          addElementField(ctx, el, out);
        }
        for (const auto& gr : ext.sequence->group_refs) {
          expandGroupRef(ctx, gr, out);
        }
        for (const auto& ch : ext.sequence->choices) {
          addChoiceFields(ctx, ch, out);
        }
      }
      if (ext.choice) {
        addChoiceFields(ctx, *ext.choice, out);
      }
      return out;
    }

    for (const auto& a : ct.attributes) {
      addAttrField(ctx, a, out);
    }
    expandAttrGroups(ctx, ct.attribute_group_refs, out);
    if (ct.sequence) {
      for (const auto& el : ct.sequence->elements) {
        addElementField(ctx, el, out);
      }
      for (const auto& gr : ct.sequence->group_refs) {
        expandGroupRef(ctx, gr, out);
      }
      for (const auto& ch : ct.sequence->choices) {
        addChoiceFields(ctx, ch, out);
      }
    }
    if (ct.choice) {
      addChoiceFields(ctx, *ct.choice, out);
    }
    return out;
  }

  // Walks the complexContent extension chain and returns all inherited fields,
  // each marked inherited=true. Uses ctx for pointer-to-member names so that
  // &Child::inherited_field is emitted (valid C++ via derived-class access).
  auto inheritedFields(const std::string& ctx, const ComplexType& ct) -> std::vector<FieldOut> {
    if (!ct.complex_content || !ct.complex_content->extension) {
      return {};
    }
    const std::string base_key{detail::localName(ct.complex_content->extension->base)};
    auto it = named_complex_.find(base_key);
    if (it == named_complex_.end()) {
      return {};
    }

    auto result = inheritedFields(ctx, *it->second);
    for (auto& f : ownFieldsOf(ctx, *it->second)) {
      f.inherited = true;
      result.push_back(std::move(f));
    }
    return result;
  }

  auto fieldsOf(const StructDef& s) -> std::vector<FieldOut> {
    auto result = inheritedFields(s.cpp_name, *s.ct);
    for (auto& f : ownFieldsOf(s.cpp_name, *s.ct)) {
      result.push_back(std::move(f));
    }
    return result;
  }

  // ---- emission ----

  auto emit() -> void {
    out_ += "/// @file Generated by TurboXML xsdgen. Do not edit by hand.\n";
    if (!notes_.empty()) {
      out_ += "///\n/// Unsupported XSD constructs (skipped):\n";
      for (const auto& n : notes_) {
        out_ += "///   - " + n + "\n";
      }
    }
    out_ += "#pragma once\n\n";
    out_ += "#include <memory>\n#include <optional>\n#include <string>\n";
    if (needs_regex_) {
      out_ += "#include <regex>\n";
    }
    out_ += "#include <variant>\n#include <vector>\n\n";
    out_ += "#include \"TurboXML.hh\"\n\n";

    for (const auto& e : enums_) {
      emitEnum(e);
    }

    if (!structs_.empty()) {
      for (const auto& s : structs_) {
        out_ += "struct " + s.cpp_name + ";\n";
      }
      out_ += "\n";
    }
    for (const auto& s : structs_) {
      emitStructDef(s);
    }
    for (const auto& s : structs_) {
      emitStructMetadata(s);
    }

    if (!constraints_out_.empty()) {
      out_ += constraints_out_;
    }

    for (const auto& el : schema_.elements) {
      const Resolved r = resolveElementType(el);
      out_ += "// root: xml::deserialize(parser, \"" + el.name + "\", obj);  // ";
      out_ += r.cpp + "\n";
    }
  }

  auto emitEnum(const EnumDef& e) -> void {
    out_ += "enum class " + e.cpp_name + " {\n";
    for (const auto& t : e.tokens) {
      out_ += "  " + detail::sanitize(t) + ",\n";
    }
    out_ += "};\n";
    out_ += "template <>\nstruct xml::XmlEnumTraits<" + e.cpp_name + "> {\n";
    out_ += "  static constexpr auto values = xml::enumTable<" + e.cpp_name + ">({\n";
    for (const auto& t : e.tokens) {
      out_ += "      {\"" + t + "\", " + e.cpp_name + "::" + detail::sanitize(t) + "},\n";
    }
    out_ += "  });\n};\n\n";
  }

  auto emitStructDef(const StructDef& s) -> void {
    out_ += "struct " + s.cpp_name;
    if (!s.parent_xsd.empty()) {
      out_ += " : " + detail::capitalize(s.parent_xsd);
    }
    out_ += " {\n";
    for (const auto& f : fieldsOf(s)) {
      if (!f.inherited && !f.member.empty()) {
        out_ += "  " + f.member + "\n";
      }
    }
    out_ += "};\n\n";
  }

  auto emitStructMetadata(const StructDef& s) -> void {
    const auto fields = fieldsOf(s);
    out_ += "template <>\nstruct xml::XmlMetadata<" + s.cpp_name + "> {\n";
    out_ += "  static constexpr auto fields = std::make_tuple(\n";
    for (size_t i = 0; i < fields.size(); ++i) {
      out_ += "      " + fields[i].metadata + (i + 1 < fields.size() ? ",\n" : "\n");
    }
    if (fields.empty()) {
      out_ += "      /* no fields */\n";
    }
    out_ += "  );\n};\n\n";
  }

  // ---- constraint generation ----

  static auto hasValueFacets(const Restriction& r) -> bool {
    return r.min_length || r.max_length || r.length || r.pattern || r.min_inclusive ||
           r.max_inclusive || r.min_exclusive || r.max_exclusive || r.fraction_digits ||
           r.total_digits;
  }

  static auto isStringCpp(const std::string& cpp) -> bool {
    return cpp == "std::string" || cpp == "std::string_view";
  }

  static auto escapeStrLiteral(const std::string& s) -> std::string {
    std::string out;
    for (const char c : s) {
      if (c == '\\') {
        out += "\\\\";
      } else if (c == '"') {
        out += "\\\"";
      } else {
        out += c;
      }
    }
    return out;
  }

  auto typeRestriction(std::string_view type) const -> const Restriction* {
    const std::string key{detail::localName(type)};
    if (auto it = named_simple_.find(key); it != named_simple_.end()) {
      if (it->second->restriction && hasValueFacets(*it->second->restriction)) {
        return &*it->second->restriction;
      }
    }
    return nullptr;
  }

  auto elementRestriction(const Element& el) const -> const Restriction* {
    if (el.simple_type && el.simple_type->restriction &&
        hasValueFacets(*el.simple_type->restriction)) {
      return &*el.simple_type->restriction;
    }
    return typeRestriction(el.type);
  }

  auto buildCheckCode(const std::string& mem, const std::string& cpp, bool is_opt,
                      const Restriction& r) -> std::string {
    std::string out;
    const std::string a = is_opt ? "v." + mem + "->" : "v." + mem + ".";
    const std::string val = is_opt ? "*v." + mem : "v." + mem;
    const std::string g = is_opt ? "v." + mem + " && " : "";

    auto emit_if = [&](const std::string& cond, const std::string& msg) {
      out += "    if (" + g + cond + ") return \"" + mem + ": " + msg + "\";\n";
    };

    if (isStringCpp(cpp)) {
      if (r.min_length) {
        emit_if(a + "size() < " + r.min_length->value,
                "minLength violation (min=" + r.min_length->value + ")");
      }
      if (r.max_length) {
        emit_if(a + "size() > " + r.max_length->value,
                "maxLength violation (max=" + r.max_length->value + ")");
      }
      if (r.length) {
        emit_if(a + "size() != " + r.length->value,
                "length violation (expected=" + r.length->value + ")");
      }
      if (r.pattern && !r.pattern->value.empty()) {
        needs_regex_ = true;
        out += "    { static const std::regex pat(\"" + escapeStrLiteral(r.pattern->value) +
               "\"); if (" + g + "!std::regex_match(" + val + ", pat)) return \"" + mem +
               ": pattern violation\"; }\n";
      }
    } else {
      if (r.min_inclusive && !r.min_inclusive->value.empty()) {
        emit_if(val + " < " + r.min_inclusive->value,
                "minInclusive violation (min=" + r.min_inclusive->value + ")");
      }
      if (r.max_inclusive && !r.max_inclusive->value.empty()) {
        emit_if(val + " > " + r.max_inclusive->value,
                "maxInclusive violation (max=" + r.max_inclusive->value + ")");
      }
      if (r.min_exclusive && !r.min_exclusive->value.empty()) {
        emit_if(val + " <= " + r.min_exclusive->value,
                "minExclusive violation (min=" + r.min_exclusive->value + ")");
      }
      if (r.max_exclusive && !r.max_exclusive->value.empty()) {
        emit_if(val + " >= " + r.max_exclusive->value,
                "maxExclusive violation (max=" + r.max_exclusive->value + ")");
      }
    }
    return out;
  }

  auto structConstraintBody(const StructDef& s) -> std::string {
    std::string body;
    const ComplexType& ct = *s.ct;

    auto process_attr = [&](const Attribute& a) {
      const std::string mem = detail::sanitize(a.name);
      if (!a.fixed.empty()) {
        const Resolved res = resolveNamedType(a.type, a.name);
        if (isStringCpp(res.cpp)) {
          body += "    if (v." + mem + " != \"" + escapeStrLiteral(a.fixed) + "\") return \"" +
                  mem + ": fixed value violation\";\n";
        } else {
          body += "    if (v." + mem + " != " + a.fixed + ") return \"" + mem +
                  ": fixed value violation\";\n";
        }
      }
      const Restriction* r = typeRestriction(a.type);
      if (r == nullptr) {
        return;
      }
      body += buildCheckCode(mem, resolveNamedType(a.type, a.name).cpp, false, *r);
    };
    auto process_element = [&](const Element& el) {
      const std::string mem = detail::sanitize(el.name);
      if (const auto n = finiteMaxOccurs(el)) {
        body +=
            std::format("    if (v.{}.size() > {}) return \"{}: maxOccurs violation (max={})\";\n",
                        mem, *n, mem, *n);
      }
      if (isUnbounded(el) || (el.simple_type && el.simple_type->list)) {
        return;
      }
      const Restriction* r = elementRestriction(el);
      if (r == nullptr) {
        return;
      }
      body += buildCheckCode(mem, resolveElementType(el).cpp, minOccursOf(el) == 0, *r);
    };

    if (ct.simple_content && ct.simple_content->extension) {
      const auto& ext = *ct.simple_content->extension;
      if (const Restriction* r = typeRestriction(ext.base)) {
        body += buildCheckCode("value", baseBuiltin(ext.base), false, *r);
      }
      for (const auto& a : ext.attributes) {
        process_attr(a);
      }
      return body;
    }

    if (ct.complex_content && ct.complex_content->extension) {
      const auto& ext = *ct.complex_content->extension;
      for (const auto& a : ext.attributes) {
        process_attr(a);
      }
      if (ext.sequence) {
        for (const auto& el : ext.sequence->elements) {
          process_element(el);
        }
      }
      if (ext.choice) {
        for (const auto& el : ext.choice->elements) {
          process_element(el);
        }
      }
      return body;
    }

    for (const auto& a : ct.attributes) {
      process_attr(a);
    }
    if (ct.sequence) {
      for (const auto& el : ct.sequence->elements) {
        process_element(el);
      }
    }
    if (ct.choice) {
      for (const auto& el : ct.choice->elements) {
        process_element(el);
      }
    }
    return body;
  }

  auto emitStructConstraints(const StructDef& s) -> void {
    const std::string body = structConstraintBody(s);
    if (body.empty()) {
      return;
    }
    constraints_out_ += "template <>\nstruct xml::XmlConstraints<" + s.cpp_name + "> {\n";
    constraints_out_ +=
        "  static auto check(const " + s.cpp_name + "& v) -> std::optional<std::string> {\n";
    constraints_out_ += body;
    constraints_out_ += "    return {};\n  }\n};\n\n";
  }

  // Topologically order structs so a by-value/optional member's type is defined
  // first. Inherited fields are skipped; the parent is already collected before
  // the child by collectComplexType.
  auto orderStructs() -> void {
    std::unordered_map<std::string, size_t> index;
    for (size_t i = 0; i < structs_.size(); ++i) {
      index[structs_[i].cpp_name] = i;
    }

    std::vector<std::vector<size_t>> deps(structs_.size());
    for (size_t i = 0; i < structs_.size(); ++i) {
      for (const auto& f : fieldsOf(structs_[i])) {
        if (f.inherited) {
          continue;
        }
        if (f.member.starts_with("std::vector<") || f.member.starts_with("std::unique_ptr<")) {
          continue;
        }
        for (const auto& [name, j] : index) {
          if (j != i && memberReferences(f.member, name)) {
            deps[i].push_back(j);
          }
        }
      }
    }
    std::vector<int> state(structs_.size(), 0);  // 0=new,1=active,2=done
    std::vector<StructDef> ordered;
    auto visit = [&](auto&& self, size_t i) -> void {
      if (state[i] == 2) {
        return;
      }
      if (state[i] == 1) {
        return;
      }  // cycle: forward decl covers it
      state[i] = 1;
      for (const size_t j : deps[i]) {
        self(self, j);
      }
      state[i] = 2;
      ordered.push_back(structs_[i]);
    };
    for (size_t i = 0; i < structs_.size(); ++i) {
      visit(visit, i);
    }
    structs_ = std::move(ordered);
  }

  static auto memberReferences(const std::string& member, const std::string& type) -> bool {
    const std::string opt = "std::optional<" + type + ">";
    return member.starts_with(type + " ") || member.starts_with(opt);
  }

  const Schema& schema_;
  std::unordered_map<std::string, const ComplexType*> named_complex_;
  std::unordered_map<std::string, const SimpleType*> named_simple_;
  std::unordered_map<std::string, const AttributeGroupDef*> named_attr_groups_;
  std::unordered_map<std::string, const GroupDef*> named_groups_;
  std::vector<StructDef> structs_;
  std::vector<EnumDef> enums_;
  std::unordered_set<std::string> emitted_;
  std::unordered_set<std::string> used_names_;
  std::vector<std::string> notes_;
  std::unordered_set<std::string> note_seen_;
  std::string out_;
  std::string constraints_out_;
  bool needs_regex_{false};
};

/// @brief Generates TurboXML metadata source from XSD schema text.
/// @param xsd_text Full text of the XSD document.
/// @param opts Options including an optional include-file loader callback.
[[nodiscard]] inline auto generate(std::string_view xsd_text,
                                   const Options& opts = {}) -> GenResult {
  xml::Parser parser{xsd_text};
  Schema schema;
  if (!xml::deserialize(parser, "schema", schema)) {
    GenResult r;
    r.ok = false;
    r.notes.push_back("failed to parse the XSD as <schema> XML");
    return r;
  }

  if (opts.loader) {
    for (const auto& inc : schema.includes) {
      auto src = opts.loader(inc.schema_location);
      if (!src) {
        continue;
      }
      xml::Parser ip{*src};
      Schema included;
      if (!xml::deserialize(ip, "schema", included)) {
        continue;
      }
      for (auto& ct : included.complex_types) {
        schema.complex_types.push_back(std::move(ct));
      }
      for (auto& st : included.simple_types) {
        schema.simple_types.push_back(std::move(st));
      }
      for (auto& el : included.elements) {
        schema.elements.push_back(std::move(el));
      }
      for (auto& ag : included.attribute_groups) {
        schema.attribute_groups.push_back(std::move(ag));
      }
      for (auto& g : included.groups) {
        schema.groups.push_back(std::move(g));
      }
    }
  }

  return Generator{schema}.run();
}

}  // namespace xsd
