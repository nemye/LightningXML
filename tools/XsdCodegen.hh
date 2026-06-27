/// @file XsdCodegen.hh
/// @brief Generates TurboXML XmlMetadata definitions from an XSD schema.
///
/// The schema is itself parsed with TurboXML (XSD is XML; the xs: prefix is
/// matched by local name). A practical subset is supported; anything outside it
/// is recorded as a note rather than dropped silently or treated as fatal.
#pragma once

#include <array>
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
  std::optional<Facet> minLength;
  std::optional<Facet> maxLength;
  std::optional<Facet> length;
  std::optional<Facet> pattern;
  std::optional<Facet> minInclusive;
  std::optional<Facet> maxInclusive;
  std::optional<Facet> minExclusive;
  std::optional<Facet> maxExclusive;
  std::optional<Facet> fractionDigits;
  std::optional<Facet> totalDigits;
};

struct List {
  std::string itemType;
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
  std::string default_;
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
  std::optional<std::string> maxOccurs;
  std::vector<Element> elements;
};

struct Sequence {
  std::vector<Element> elements;
  std::vector<Choice> choices;
  std::vector<GroupRef> groupRefs;
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
  std::vector<AttributeGroupRef> attributeGroupRefs;
};

struct ComplexContent {
  std::optional<ComplexExtension> extension;
};

struct ComplexType {
  std::string name;
  bool mixed{};
  std::optional<Sequence> sequence;
  std::optional<Choice> choice;
  std::optional<SimpleContent> simpleContent;
  std::optional<ComplexContent> complexContent;
  std::vector<Attribute> attributes;
  std::vector<AttributeGroupRef> attributeGroupRefs;
};

struct Element {
  std::string name;
  std::string type;
  std::optional<int> minOccurs;
  std::optional<std::string> maxOccurs;
  std::optional<ComplexType> complexType;
  std::optional<SimpleType> simpleType;
};

struct Include {
  std::string schemaLocation;
};

struct Schema {
  std::vector<Element> elements;
  std::vector<ComplexType> complexTypes;
  std::vector<SimpleType> simpleTypes;
  std::vector<AttributeGroupDef> attributeGroups;
  std::vector<GroupDef> groups;
  std::vector<Include> includes;
};

}  // namespace xsd

template <>
struct xml::XmlMetadata<xsd::Enumeration> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("value", &xsd::Enumeration::value));
};
template <>
struct xml::XmlMetadata<xsd::Facet> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("value", &xsd::Facet::value));
};
template <>
struct xml::XmlMetadata<xsd::Restriction> {
  static constexpr auto fields = std::make_tuple(
      xml::attr_field("base", &xsd::Restriction::base),
      xml::vec_field("enumeration", &xsd::Restriction::enumerations),
      xml::field("minLength", &xsd::Restriction::minLength),
      xml::field("maxLength", &xsd::Restriction::maxLength),
      xml::field("length", &xsd::Restriction::length),
      xml::field("pattern", &xsd::Restriction::pattern),
      xml::field("minInclusive", &xsd::Restriction::minInclusive),
      xml::field("maxInclusive", &xsd::Restriction::maxInclusive),
      xml::field("minExclusive", &xsd::Restriction::minExclusive),
      xml::field("maxExclusive", &xsd::Restriction::maxExclusive),
      xml::field("fractionDigits", &xsd::Restriction::fractionDigits),
      xml::field("totalDigits", &xsd::Restriction::totalDigits));
};
template <>
struct xml::XmlMetadata<xsd::List> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("itemType", &xsd::List::itemType));
};
template <>
struct xml::XmlMetadata<xsd::SimpleType> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("name", &xsd::SimpleType::name),
                      xml::field("restriction", &xsd::SimpleType::restriction),
                      xml::field("list", &xsd::SimpleType::list));
};
template <>
struct xml::XmlMetadata<xsd::Attribute> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("name", &xsd::Attribute::name),
                      xml::attr_field("type", &xsd::Attribute::type),
                      xml::attr_field("use", &xsd::Attribute::use),
                      xml::attr_field("default", &xsd::Attribute::default_));
};
template <>
struct xml::XmlMetadata<xsd::AttributeGroupRef> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("ref", &xsd::AttributeGroupRef::ref));
};
template <>
struct xml::XmlMetadata<xsd::AttributeGroupDef> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("name", &xsd::AttributeGroupDef::name),
                      xml::vec_field("attribute", &xsd::AttributeGroupDef::attributes));
};
template <>
struct xml::XmlMetadata<xsd::GroupRef> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("ref", &xsd::GroupRef::ref));
};
template <>
struct xml::XmlMetadata<xsd::Choice> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("maxOccurs", &xsd::Choice::maxOccurs),
                      xml::vec_field("element", &xsd::Choice::elements));
};
template <>
struct xml::XmlMetadata<xsd::Sequence> {
  static constexpr auto fields =
      std::make_tuple(xml::vec_field("element", &xsd::Sequence::elements),
                      xml::vec_field("choice", &xsd::Sequence::choices),
                      xml::vec_field("group", &xsd::Sequence::groupRefs));
};
template <>
struct xml::XmlMetadata<xsd::GroupDef> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("name", &xsd::GroupDef::name),
                      xml::field("sequence", &xsd::GroupDef::sequence),
                      xml::field("choice", &xsd::GroupDef::choice));
};
template <>
struct xml::XmlMetadata<xsd::Extension> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("base", &xsd::Extension::base),
                      xml::vec_field("attribute", &xsd::Extension::attributes));
};
template <>
struct xml::XmlMetadata<xsd::SimpleContent> {
  static constexpr auto fields = std::make_tuple(
      xml::field("extension", &xsd::SimpleContent::extension));
};
template <>
struct xml::XmlMetadata<xsd::ComplexExtension> {
  static constexpr auto fields = std::make_tuple(
      xml::attr_field("base", &xsd::ComplexExtension::base),
      xml::field("sequence", &xsd::ComplexExtension::sequence),
      xml::field("choice", &xsd::ComplexExtension::choice),
      xml::vec_field("attribute", &xsd::ComplexExtension::attributes),
      xml::vec_field("attributeGroup", &xsd::ComplexExtension::attributeGroupRefs));
};
template <>
struct xml::XmlMetadata<xsd::ComplexContent> {
  static constexpr auto fields = std::make_tuple(
      xml::field("extension", &xsd::ComplexContent::extension));
};
template <>
struct xml::XmlMetadata<xsd::ComplexType> {
  static constexpr auto fields = std::make_tuple(
      xml::attr_field("name", &xsd::ComplexType::name),
      xml::attr_field("mixed", &xsd::ComplexType::mixed),
      xml::field("sequence", &xsd::ComplexType::sequence),
      xml::field("choice", &xsd::ComplexType::choice),
      xml::field("simpleContent", &xsd::ComplexType::simpleContent),
      xml::field("complexContent", &xsd::ComplexType::complexContent),
      xml::vec_field("attribute", &xsd::ComplexType::attributes),
      xml::vec_field("attributeGroup", &xsd::ComplexType::attributeGroupRefs));
};
template <>
struct xml::XmlMetadata<xsd::Element> {
  static constexpr auto fields = std::make_tuple(
      xml::attr_field("name", &xsd::Element::name),
      xml::attr_field("type", &xsd::Element::type),
      xml::attr_field("minOccurs", &xsd::Element::minOccurs),
      xml::attr_field("maxOccurs", &xsd::Element::maxOccurs),
      xml::field("complexType", &xsd::Element::complexType),
      xml::field("simpleType", &xsd::Element::simpleType));
};
template <>
struct xml::XmlMetadata<xsd::Include> {
  static constexpr auto fields =
      std::make_tuple(xml::attr_field("schemaLocation", &xsd::Include::schemaLocation));
};
template <>
struct xml::XmlMetadata<xsd::Schema> {
  static constexpr auto fields = std::make_tuple(
      xml::vec_field("element", &xsd::Schema::elements),
      xml::vec_field("complexType", &xsd::Schema::complexTypes),
      xml::vec_field("simpleType", &xsd::Schema::simpleTypes),
      xml::vec_field("attributeGroup", &xsd::Schema::attributeGroups),
      xml::vec_field("group", &xsd::Schema::groups),
      xml::vec_field("include", &xsd::Schema::includes));
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

inline auto local_name(std::string_view s) -> std::string_view {
  const auto pos = s.rfind(':');
  return pos == std::string_view::npos ? s : s.substr(pos + 1);
}

inline auto is_ident_start(char c) -> bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
inline auto is_ident_char(char c) -> bool {
  return is_ident_start(c) || (c >= '0' && c <= '9');
}

// Turn an XML name into a valid C++ identifier (invalid chars -> '_').
inline auto sanitize(std::string_view name) -> std::string {
  std::string out;
  out.reserve(name.size() + 1);
  for (char c : name) {
    out.push_back(is_ident_char(c) ? c : '_');
  }
  if (out.empty() || !is_ident_start(out.front())) {
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
inline auto builtin_type(std::string_view xsd_type) -> std::string {
  static const std::unordered_map<std::string_view, std::string_view> kMap = {
      {"string", "std::string"},        {"normalizedString", "std::string"},
      {"token", "std::string"},         {"anyURI", "std::string"},
      {"NMTOKEN", "std::string"},       {"Name", "std::string"},
      {"NCName", "std::string"},        {"ID", "std::string"},
      {"IDREF", "std::string"},         {"language", "std::string"},
      {"boolean", "bool"},              {"int", "int"},
      {"integer", "long"},              {"long", "long"},
      {"short", "short"},               {"byte", "signed char"},
      {"unsignedInt", "unsigned"},      {"unsignedLong", "unsigned long"},
      {"unsignedShort", "unsigned short"}, {"nonNegativeInteger", "long"},
      {"positiveInteger", "long"},      {"decimal", "double"},
      {"double", "double"},             {"float", "float"},
      {"date", "xml::Date"},            {"time", "xml::Time"},
      {"dateTime", "xml::DateTime"}};
  const auto it = kMap.find(local_name(xsd_type));
  return it == kMap.end() ? std::string{} : std::string{it->second};
}

}  // namespace detail

// ---- Generator ----

class Generator {
 public:
  explicit Generator(const Schema& schema) : schema_(schema) {}

  auto run() -> GenResult {
    index_named_types();
    for (const auto& st : schema_.simpleTypes) collect_simple_type(st.name, st);
    for (const auto& ct : schema_.complexTypes)
      collect_complex_type(detail::capitalize(ct.name), ct);
    for (const auto& el : schema_.elements) collect_element_inline(el);
    order_structs();
    // Build constraint specializations before emit() so needs_regex_ is known.
    for (const auto& s : structs_) emit_struct_constraints(s);
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

  // Deduplicated: fields_of() is computed several times (ordering, struct, and
  // metadata emission), so the same diagnostic can be produced more than once.
  void note(std::string msg) {
    if (note_seen_.insert(msg).second) notes_.push_back(std::move(msg));
  }

  void index_named_types() {
    for (const auto& ct : schema_.complexTypes) {
      if (!ct.name.empty()) named_complex_[ct.name] = &ct;
    }
    for (const auto& st : schema_.simpleTypes) {
      if (!st.name.empty()) named_simple_[st.name] = &st;
    }
    for (const auto& ag : schema_.attributeGroups) {
      if (!ag.name.empty()) named_attr_groups_[ag.name] = &ag;
    }
    for (const auto& g : schema_.groups) {
      if (!g.name.empty()) named_groups_[g.name] = &g;
    }
  }

  auto collect_simple_type(std::string_view xml_name, const SimpleType& st)
      -> std::string {
    if (!st.restriction || st.restriction->enumerations.empty()) {
      return {};
    }
    const std::string cpp = unique_name(detail::capitalize(xml_name));
    EnumDef e;
    e.cpp_name = cpp;
    for (const auto& en : st.restriction->enumerations) {
      e.tokens.push_back(en.value);
    }
    enums_.push_back(std::move(e));
    return cpp;
  }

  void collect_complex_type(std::string cpp_name, const ComplexType& ct) {
    if (emitted_.count(cpp_name)) return;
    emitted_.insert(cpp_name);

    std::string parent_xsd;
    if (ct.complexContent && ct.complexContent->extension) {
      const std::string& base = ct.complexContent->extension->base;
      parent_xsd = std::string{detail::local_name(base)};
      if (auto it = named_complex_.find(parent_xsd); it != named_complex_.end()) {
        collect_complex_type(detail::capitalize(parent_xsd), *it->second);
      } else {
        note("complexContent base '" + base + "' not found in schema");
      }
    }

    structs_.push_back({cpp_name, &ct, parent_xsd});

    if (ct.mixed) {
      note("mixed content on complexType '" + ct.name +
           "' is unsupported; text between child elements is ignored");
    }

    // Collect inline types from wherever they live in this CT.
    auto collect_seq = [&](const Sequence& seq) {
      for (const auto& el : seq.elements) collect_element_inline(el);
      for (const auto& ch : seq.choices) {
        for (const auto& el : ch.elements) collect_element_inline(el);
      }
    };

    if (ct.complexContent && ct.complexContent->extension) {
      const auto& ext = *ct.complexContent->extension;
      if (ext.sequence) collect_seq(*ext.sequence);
      if (ext.choice) {
        for (const auto& el : ext.choice->elements) collect_element_inline(el);
      }
    } else {
      if (ct.sequence) collect_seq(*ct.sequence);
      if (ct.choice) {
        for (const auto& el : ct.choice->elements) collect_element_inline(el);
      }
    }
  }

  void collect_element_inline(const Element& el) {
    if (el.complexType) {
      collect_complex_type(unique_name(detail::capitalize(el.name)), *el.complexType);
    } else if (el.simpleType) {
      collect_simple_type(el.name, *el.simpleType);
    }
  }

  auto unique_name(std::string base) -> std::string {
    std::string name = base;
    int n = 2;
    while (used_names_.count(name)) name = base + std::to_string(n++);
    used_names_.insert(name);
    return name;
  }

  // ---- type resolution ----

  struct Resolved {
    std::string cpp;
    bool is_struct{};
    bool is_list{};
  };

  auto resolve_list_item(std::string_view itemType) -> std::string {
    if (std::string b = detail::builtin_type(itemType); !b.empty()) return b;
    const std::string key{detail::local_name(itemType)};
    if (auto it = named_simple_.find(key); it != named_simple_.end()) {
      if (it->second->restriction &&
          !it->second->restriction->enumerations.empty()) {
        return detail::capitalize(key);
      }
      return base_builtin(it->second->restriction
                              ? it->second->restriction->base
                              : "string");
    }
    return "std::string";
  }

  static auto builtin_list(std::string_view type) -> bool {
    const auto n = detail::local_name(type);
    return n == "NMTOKENS" || n == "IDREFS" || n == "ENTITIES";
  }

  auto resolve_element_type(const Element& el) -> Resolved {
    if (el.complexType) return {detail::capitalize(el.name), true, false};
    if (el.simpleType) {
      if (el.simpleType->list)
        return {resolve_list_item(el.simpleType->list->itemType), false, true};
      if (std::string e = enum_name_for_inline(el); !e.empty())
        return {e, false, false};
      if (el.simpleType->restriction)
        return {base_builtin(el.simpleType->restriction->base), false, false};
      note("untyped inline simpleType on '" + el.name + "' -> std::string");
      return {"std::string", false, false};
    }
    return resolve_named_type(el.type, el.name);
  }

  auto enum_name_for_inline(const Element& el) -> std::string {
    if (!el.simpleType || !el.simpleType->restriction ||
        el.simpleType->restriction->enumerations.empty())
      return {};
    return detail::capitalize(el.name);
  }

  auto base_builtin(std::string_view base) -> std::string {
    if (std::string b = detail::builtin_type(base); !b.empty()) return b;
    if (auto it = named_simple_.find(std::string{detail::local_name(base)});
        it != named_simple_.end() && it->second->restriction) {
      return base_builtin(it->second->restriction->base);
    }
    return "std::string";
  }

  auto resolve_named_type(std::string_view type, std::string_view ctx) -> Resolved {
    if (type.empty()) {
      note("element '" + std::string{ctx} + "' has no type -> std::string");
      return {"std::string", false, false};
    }
    if (builtin_list(type)) return {"std::string", false, true};
    if (std::string b = detail::builtin_type(type); !b.empty()) return {b, false, false};
    const std::string key{detail::local_name(type)};
    if (auto it = named_complex_.find(key); it != named_complex_.end())
      return {detail::capitalize(key), true, false};
    if (auto it = named_simple_.find(key); it != named_simple_.end()) {
      if (it->second->list)
        return {resolve_list_item(it->second->list->itemType), false, true};
      if (it->second->restriction && !it->second->restriction->enumerations.empty())
        return {detail::capitalize(key), false, false};
      return {base_builtin(it->second->restriction ? it->second->restriction->base
                                                   : "string"),
              false, false};
    }
    note("unknown type '" + std::string{type} + "' on '" + std::string{ctx} +
         "' -> std::string");
    return {"std::string", false, false};
  }

  // ---- cardinality ----

  static auto is_unbounded(const Element& el) -> bool {
    return el.maxOccurs && (*el.maxOccurs == "unbounded" ||
                            (!el.maxOccurs->empty() && *el.maxOccurs != "0" &&
                             *el.maxOccurs != "1"));
  }
  static auto min_occurs(const Element& el) -> int {
    return el.minOccurs.value_or(1);
  }

  // ---- field generation ----

  void add_attr_field(const std::string& ctx, const Attribute& a,
                      std::vector<FieldOut>& out) {
    const Resolved r = resolve_named_type(a.type, a.name);
    const std::string mem = detail::sanitize(a.name);
    const std::string suffix = (a.use == "required") ? ", true)" : ")";
    std::string decl;
    if (r.is_list) {
      decl = "std::vector<" + r.cpp + "> " + mem + ";";
    } else if (!a.default_.empty()) {
      decl = r.cpp + " " + mem + "{\"" + a.default_ + "\"};";
    } else {
      decl = r.cpp + " " + mem + "{};";
    }
    out.push_back({decl, "xml::attr_field(\"" + a.name + "\", &" + ctx + "::" + mem + suffix});
  }

  void add_element_field(const std::string& ctx, const Element& el,
                         std::vector<FieldOut>& out) {
    const Resolved r = resolve_element_type(el);
    const std::string mem = detail::sanitize(el.name);
    const std::string cpp = r.cpp;
    if (r.is_list) {
      if (is_unbounded(el)) {
        note("repeated xs:list element '" + el.name +
             "' (maxOccurs>1) is unsupported; treated as a single list");
      }
      out.push_back({"std::vector<" + cpp + "> " + mem + ";",
                     "xml::list_field(\"" + el.name + "\", &" + ctx + "::" + mem +
                         (min_occurs(el) >= 1 ? ", true)" : ")")});
      return;
    }
    if (is_unbounded(el)) {
      out.push_back({"std::vector<" + cpp + "> " + mem + ";",
                     "xml::vec_field(\"" + el.name + "\", &" + ctx + "::" + mem +
                         (min_occurs(el) >= 1 ? ", true)" : ")")});
    } else if (r.is_struct && r.cpp == ctx) {
      // Direct self-reference: break the cycle with unique_ptr.
      out.push_back({"std::unique_ptr<" + cpp + "> " + mem + ";",
                     "xml::field(\"" + el.name + "\", &" + ctx + "::" + mem + ")"});
    } else if (min_occurs(el) == 0) {
      out.push_back({"std::optional<" + cpp + "> " + mem + ";",
                     "xml::field(\"" + el.name + "\", &" + ctx + "::" + mem + ")"});
    } else {
      out.push_back({cpp + " " + mem + "{};",
                     "xml::field(\"" + el.name + "\", &" + ctx + "::" + mem + ", true)"});
    }
  }

  void add_choice_fields(const std::string& ctx, const Choice& ch,
                         std::vector<FieldOut>& out) {
    std::vector<std::string> types;
    std::vector<std::string> alts;
    std::unordered_set<std::string> seen;
    for (const auto& el : ch.elements) {
      const Resolved r = resolve_element_type(el);
      if (!seen.insert(r.cpp).second) {
        note("xs:choice in '" + ctx + "' has two branches of the same C++ type ('" +
             r.cpp + "'); the choice was skipped (std::variant needs distinct types)");
        return;
      }
      types.push_back(r.cpp);
      alts.push_back("xml::alt<" + r.cpp + ">(\"" + el.name + "\")");
    }
    if (types.empty()) return;

    std::string variant = "std::variant<";
    for (size_t i = 0; i < types.size(); ++i)
      variant += types[i] + (i + 1 < types.size() ? ", " : "");
    variant += ">";

    const bool repeated =
        ch.maxOccurs && (*ch.maxOccurs == "unbounded" || *ch.maxOccurs != "1");
    const std::string member_type = repeated ? "std::vector<" + variant + ">" : variant;
    std::string alts_joined;
    for (size_t i = 0; i < alts.size(); ++i)
      alts_joined += "\n          " + alts[i] + (i + 1 < alts.size() ? "," : "");

    out.push_back({member_type + " choice;",
                   "xml::variant_field(&" + ctx + "::choice," + alts_joined + ")"});
  }

  void expand_attr_groups(const std::string& ctx,
                          const std::vector<AttributeGroupRef>& refs,
                          std::vector<FieldOut>& out) {
    for (const auto& agr : refs) {
      const std::string ref_key{detail::local_name(agr.ref)};
      if (auto it = named_attr_groups_.find(ref_key); it != named_attr_groups_.end()) {
        for (const auto& a : it->second->attributes) add_attr_field(ctx, a, out);
      } else {
        note("attributeGroup ref='" + agr.ref + "' not found in schema");
      }
    }
  }

  void expand_group_ref(const std::string& ctx, const GroupRef& gr,
                        std::vector<FieldOut>& out) {
    const std::string ref_key{detail::local_name(gr.ref)};
    if (auto it = named_groups_.find(ref_key); it != named_groups_.end()) {
      const GroupDef& gd = *it->second;
      if (gd.sequence) {
        for (const auto& el : gd.sequence->elements) add_element_field(ctx, el, out);
        for (const auto& ch : gd.sequence->choices) add_choice_fields(ctx, ch, out);
      }
      if (gd.choice) add_choice_fields(ctx, *gd.choice, out);
    } else {
      note("group ref='" + gr.ref + "' not found in schema");
    }
  }

  // Generates the own (non-inherited) fields of a complexType. ctx is the C++
  // struct name used for pointer-to-member references in metadata.
  auto own_fields_of(const std::string& ctx, const ComplexType& ct)
      -> std::vector<FieldOut> {
    std::vector<FieldOut> out;

    if (ct.simpleContent && ct.simpleContent->extension) {
      const auto& ext = *ct.simpleContent->extension;
      out.push_back({base_builtin(ext.base) + " value{};",
                     "xml::value_field(&" + ctx + "::value)"});
      for (const auto& a : ext.attributes) add_attr_field(ctx, a, out);
      return out;
    }

    if (ct.complexContent && ct.complexContent->extension) {
      const auto& ext = *ct.complexContent->extension;
      for (const auto& a : ext.attributes) add_attr_field(ctx, a, out);
      expand_attr_groups(ctx, ext.attributeGroupRefs, out);
      if (ext.sequence) {
        for (const auto& el : ext.sequence->elements) add_element_field(ctx, el, out);
        for (const auto& gr : ext.sequence->groupRefs) expand_group_ref(ctx, gr, out);
        for (const auto& ch : ext.sequence->choices) add_choice_fields(ctx, ch, out);
      }
      if (ext.choice) add_choice_fields(ctx, *ext.choice, out);
      return out;
    }

    for (const auto& a : ct.attributes) add_attr_field(ctx, a, out);
    expand_attr_groups(ctx, ct.attributeGroupRefs, out);
    if (ct.sequence) {
      for (const auto& el : ct.sequence->elements) add_element_field(ctx, el, out);
      for (const auto& gr : ct.sequence->groupRefs) expand_group_ref(ctx, gr, out);
      for (const auto& ch : ct.sequence->choices) add_choice_fields(ctx, ch, out);
    }
    if (ct.choice) add_choice_fields(ctx, *ct.choice, out);
    return out;
  }

  // Walks the complexContent extension chain and returns all inherited fields,
  // each marked inherited=true. Uses ctx for pointer-to-member names so that
  // &Child::inherited_field is emitted (valid C++ via derived-class access).
  auto inherited_fields(const std::string& ctx, const ComplexType& ct)
      -> std::vector<FieldOut> {
    if (!ct.complexContent || !ct.complexContent->extension) return {};
    const std::string base_key{
        detail::local_name(ct.complexContent->extension->base)};
    auto it = named_complex_.find(base_key);
    if (it == named_complex_.end()) return {};

    auto result = inherited_fields(ctx, *it->second);
    for (auto& f : own_fields_of(ctx, *it->second)) {
      f.inherited = true;
      result.push_back(std::move(f));
    }
    return result;
  }

  auto fields_of(const StructDef& s) -> std::vector<FieldOut> {
    auto result = inherited_fields(s.cpp_name, *s.ct);
    for (auto& f : own_fields_of(s.cpp_name, *s.ct)) result.push_back(std::move(f));
    return result;
  }

  // ---- emission ----

  void emit() {
    out_ += "/// @file Generated by TurboXML xsdgen. Do not edit by hand.\n";
    if (!notes_.empty()) {
      out_ += "///\n/// Unsupported XSD constructs (skipped):\n";
      for (const auto& n : notes_) out_ += "///   - " + n + "\n";
    }
    out_ += "#pragma once\n\n";
    out_ += "#include <memory>\n#include <optional>\n#include <string>\n";
    if (needs_regex_) out_ += "#include <regex>\n";
    out_ += "#include <variant>\n#include <vector>\n\n";
    out_ += "#include \"TurboXML.hh\"\n\n";

    for (const auto& e : enums_) emit_enum(e);

    if (!structs_.empty()) {
      for (const auto& s : structs_) out_ += "struct " + s.cpp_name + ";\n";
      out_ += "\n";
    }
    for (const auto& s : structs_) emit_struct_def(s);
    for (const auto& s : structs_) emit_struct_metadata(s);

    if (!constraints_out_.empty()) out_ += constraints_out_;

    for (const auto& el : schema_.elements) {
      const Resolved r = resolve_element_type(el);
      out_ += "// root: xml::deserialize(parser, \"" + el.name + "\", obj);  // ";
      out_ += r.cpp + "\n";
    }
  }

  void emit_enum(const EnumDef& e) {
    out_ += "enum class " + e.cpp_name + " {\n";
    for (const auto& t : e.tokens) out_ += "  " + detail::sanitize(t) + ",\n";
    out_ += "};\n";
    out_ += "template <>\nstruct xml::XmlEnumTraits<" + e.cpp_name + "> {\n";
    out_ += "  static constexpr auto values = xml::enum_table<" + e.cpp_name + ">({\n";
    for (const auto& t : e.tokens) {
      out_ += "      {\"" + t + "\", " + e.cpp_name + "::" + detail::sanitize(t) + "},\n";
    }
    out_ += "  });\n};\n\n";
  }

  void emit_struct_def(const StructDef& s) {
    out_ += "struct " + s.cpp_name;
    if (!s.parent_xsd.empty()) out_ += " : " + detail::capitalize(s.parent_xsd);
    out_ += " {\n";
    for (const auto& f : fields_of(s)) {
      if (!f.inherited && !f.member.empty()) out_ += "  " + f.member + "\n";
    }
    out_ += "};\n\n";
  }

  void emit_struct_metadata(const StructDef& s) {
    const auto fields = fields_of(s);
    out_ += "template <>\nstruct xml::XmlMetadata<" + s.cpp_name + "> {\n";
    out_ += "  static constexpr auto fields = std::make_tuple(\n";
    for (size_t i = 0; i < fields.size(); ++i) {
      out_ += "      " + fields[i].metadata + (i + 1 < fields.size() ? ",\n" : "\n");
    }
    if (fields.empty()) out_ += "      /* no fields */\n";
    out_ += "  );\n};\n\n";
  }

  // ---- constraint generation ----

  static auto has_value_facets(const Restriction& r) -> bool {
    return r.minLength || r.maxLength || r.length || r.pattern || r.minInclusive ||
           r.maxInclusive || r.minExclusive || r.maxExclusive || r.fractionDigits ||
           r.totalDigits;
  }

  static auto is_string_cpp(const std::string& cpp) -> bool {
    return cpp == "std::string" || cpp == "std::string_view";
  }

  static auto escape_str_literal(const std::string& s) -> std::string {
    std::string out;
    for (char c : s) {
      if (c == '\\') out += "\\\\";
      else if (c == '"') out += "\\\"";
      else out += c;
    }
    return out;
  }

  auto type_restriction(std::string_view type) -> const Restriction* {
    const std::string key{detail::local_name(type)};
    if (auto it = named_simple_.find(key); it != named_simple_.end()) {
      if (it->second->restriction && has_value_facets(*it->second->restriction))
        return &*it->second->restriction;
    }
    return nullptr;
  }

  auto element_restriction(const Element& el) -> const Restriction* {
    if (el.simpleType && el.simpleType->restriction &&
        has_value_facets(*el.simpleType->restriction))
      return &*el.simpleType->restriction;
    return type_restriction(el.type);
  }

  auto build_check_code(const std::string& mem, const std::string& cpp, bool is_opt,
                        const Restriction& r) -> std::string {
    std::string out;
    const std::string a = is_opt ? "v." + mem + "->" : "v." + mem + ".";
    const std::string val = is_opt ? "*v." + mem : "v." + mem;
    const std::string g = is_opt ? "v." + mem + " && " : "";

    auto emit_if = [&](const std::string& cond, const std::string& msg) {
      out += "    if (" + g + cond + ") return \"" + mem + ": " + msg + "\";\n";
    };

    if (is_string_cpp(cpp)) {
      if (r.minLength)
        emit_if(a + "size() < " + r.minLength->value,
                "minLength violation (min=" + r.minLength->value + ")");
      if (r.maxLength)
        emit_if(a + "size() > " + r.maxLength->value,
                "maxLength violation (max=" + r.maxLength->value + ")");
      if (r.length)
        emit_if(a + "size() != " + r.length->value,
                "length violation (expected=" + r.length->value + ")");
      if (r.pattern && !r.pattern->value.empty()) {
        needs_regex_ = true;
        out += "    if (" + g + "!std::regex_match(" + val + ", std::regex(\"" +
               escape_str_literal(r.pattern->value) + "\"))) return \"" + mem +
               ": pattern violation\";\n";
      }
    } else {
      if (r.minInclusive && !r.minInclusive->value.empty())
        emit_if(val + " < " + r.minInclusive->value,
                "minInclusive violation (min=" + r.minInclusive->value + ")");
      if (r.maxInclusive && !r.maxInclusive->value.empty())
        emit_if(val + " > " + r.maxInclusive->value,
                "maxInclusive violation (max=" + r.maxInclusive->value + ")");
      if (r.minExclusive && !r.minExclusive->value.empty())
        emit_if(val + " <= " + r.minExclusive->value,
                "minExclusive violation (min=" + r.minExclusive->value + ")");
      if (r.maxExclusive && !r.maxExclusive->value.empty())
        emit_if(val + " >= " + r.maxExclusive->value,
                "maxExclusive violation (max=" + r.maxExclusive->value + ")");
    }
    return out;
  }

  auto struct_constraint_body(const StructDef& s) -> std::string {
    std::string body;
    const ComplexType& ct = *s.ct;

    auto process_attr = [&](const Attribute& a) {
      const Restriction* r = type_restriction(a.type);
      if (!r) return;
      body += build_check_code(detail::sanitize(a.name),
                               resolve_named_type(a.type, a.name).cpp, false, *r);
    };
    auto process_element = [&](const Element& el) {
      if (is_unbounded(el) || (el.simpleType && el.simpleType->list)) return;
      const Restriction* r = element_restriction(el);
      if (!r) return;
      body += build_check_code(detail::sanitize(el.name), resolve_element_type(el).cpp,
                               min_occurs(el) == 0, *r);
    };

    if (ct.simpleContent && ct.simpleContent->extension) {
      const auto& ext = *ct.simpleContent->extension;
      if (const Restriction* r = type_restriction(ext.base))
        body += build_check_code("value", base_builtin(ext.base), false, *r);
      for (const auto& a : ext.attributes) process_attr(a);
      return body;
    }

    if (ct.complexContent && ct.complexContent->extension) {
      const auto& ext = *ct.complexContent->extension;
      for (const auto& a : ext.attributes) process_attr(a);
      if (ext.sequence) {
        for (const auto& el : ext.sequence->elements) process_element(el);
      }
      if (ext.choice) {
        for (const auto& el : ext.choice->elements) process_element(el);
      }
      return body;
    }

    for (const auto& a : ct.attributes) process_attr(a);
    if (ct.sequence) {
      for (const auto& el : ct.sequence->elements) process_element(el);
    }
    if (ct.choice) {
      for (const auto& el : ct.choice->elements) process_element(el);
    }
    return body;
  }

  void emit_struct_constraints(const StructDef& s) {
    std::string body = struct_constraint_body(s);
    if (body.empty()) return;
    constraints_out_ += "template <>\nstruct xml::XmlConstraints<" + s.cpp_name + "> {\n";
    constraints_out_ += "  static auto check(const " + s.cpp_name +
                        "& v) -> std::optional<std::string> {\n";
    constraints_out_ += body;
    constraints_out_ += "    return {};\n  }\n};\n\n";
  }

  // Topologically order structs so a by-value/optional member's type is defined
  // first. Inherited fields are skipped; the parent is already collected before
  // the child by collect_complex_type.
  void order_structs() {
    std::unordered_map<std::string, size_t> index;
    for (size_t i = 0; i < structs_.size(); ++i)
      index[structs_[i].cpp_name] = i;

    std::vector<std::vector<size_t>> deps(structs_.size());
    for (size_t i = 0; i < structs_.size(); ++i) {
      for (const auto& f : fields_of(structs_[i])) {
        if (f.inherited) continue;
        if (f.member.rfind("std::vector<", 0) == 0 ||
            f.member.rfind("std::unique_ptr<", 0) == 0)
          continue;
        for (const auto& [name, j] : index) {
          if (j != i && member_references(f.member, name)) deps[i].push_back(j);
        }
      }
    }
    std::vector<int> state(structs_.size(), 0);  // 0=new,1=active,2=done
    std::vector<StructDef> ordered;
    auto visit = [&](auto&& self, size_t i) -> void {
      if (state[i] == 2) return;
      if (state[i] == 1) return;  // cycle: forward decl covers it
      state[i] = 1;
      for (size_t j : deps[i]) self(self, j);
      state[i] = 2;
      ordered.push_back(structs_[i]);
    };
    for (size_t i = 0; i < structs_.size(); ++i) visit(visit, i);
    structs_ = std::move(ordered);
  }

  static auto member_references(const std::string& member,
                                const std::string& type) -> bool {
    const std::string opt = "std::optional<" + type + ">";
    return member.rfind(type + " ", 0) == 0 || member.rfind(opt, 0) == 0;
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
inline auto generate(std::string_view xsd_text, const Options& opts = {}) -> GenResult {
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
      auto src = opts.loader(inc.schemaLocation);
      if (!src) continue;
      xml::Parser ip{*src};
      Schema included;
      if (!xml::deserialize(ip, "schema", included)) continue;
      for (auto& ct : included.complexTypes) schema.complexTypes.push_back(std::move(ct));
      for (auto& st : included.simpleTypes) schema.simpleTypes.push_back(std::move(st));
      for (auto& el : included.elements) schema.elements.push_back(std::move(el));
      for (auto& ag : included.attributeGroups)
        schema.attributeGroups.push_back(std::move(ag));
      for (auto& g : included.groups) schema.groups.push_back(std::move(g));
    }
  }

  return Generator{schema}.run();
}

}  // namespace xsd
