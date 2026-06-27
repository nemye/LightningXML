# Changelog

## 2.0.0 - 2026-06-27

XML 1.0 fifth edition conformance, optional XSD 1.0 constraint validation, and
a substantially expanded code generator.

### Added
- **XML 1.0 conformance** (non-validating processor):
  - UTF-8 BOM (`\xEF\xBB\xBF`) is stripped before parsing.
  - DOCTYPE internal subset is correctly skipped with bracket-depth tracking so
    `<!ENTITY ...>` and `<!ATTLIST ...>` declarations containing `>` no longer
    truncate the skip.
  - Conformance test suite (`test/test_Conformance.cc`).
- **XSD constraint validation**:
  - `xml::XmlConstraints<T>`: specializable trait with a `check(const T&)`
    static returning `std::optional<std::string>`. The default is a no-op.
  - `xml::validate(obj)` → `std::optional<xml::ValidationError>`: calls the
    trait and wraps the result. Empty = valid; `err->message` = first violation.
  - `xml::ValidationError`: a named struct distinct from `xml::ErrorCode` so
    parser errors and schema constraint violations can be handled independently.
- **`xsdgen` expansions** (`tools/XSDCodegen.hh`):
  - `xs:complexContent extension`: emits `struct Child : Parent` with a merged
    `XmlMetadata<Child>` that prepends parent fields; multi-level inheritance
    is handled recursively.
  - `xs:attributeGroup` / `xs:group`: expanded inline at codegen time; no
    runtime representation.
  - `xs:include schemaLocation="..."`: loaded and merged before code generation
    via a `std::function` loader callback in `xsd::Options`; the CLI resolves
    paths relative to the input schema's directory.
  - XSD facets (`minLength`, `maxLength`, `length`, `pattern`, `minInclusive`,
    `maxInclusive`, `minExclusive`, `maxExclusive`): emitted as
    `XmlConstraints<T>` specializations; `pattern` uses `<regex>` (included only
    when needed).
  - Attribute `default`: emits a C++ default member initializer.
  - Attribute `fixed`: emits a default initializer *and* an `XmlConstraints<T>`
    equality check.
  - Finite `maxOccurs=N` (N > 1): emits `std::vector<T>` + an
    `XmlConstraints<T>` size check (`v.field.size() > N`).
- `xsdgen` tool (`tools/`): generates `XmlMetadata` definitions from an XSD
  schema (parsed with TurboXML itself). Maps complexTypes/elements/attributes,
  enumerations, simpleContent, `xs:choice`, built-ins (incl. date/time), and
  recursion to the library's features; unsupported constructs are reported as
  notes. Built behind `TURBOXML_BUILD_CODEGEN` (on by default).
- `xml::ErrorCode` enum and `Parser::error_code()`: a failed `deserialize()`
  now reports a specific reason (e.g. `UnterminatedComment`, `ElementMismatch`,
  `RootElementNotFound`). `reset()` clears it.
- Enum fields via `xml::XmlEnumTraits<E>`: maps XML token spellings (e.g.
  `xs:enumeration`) to C++ enumerators for element and attribute fields, with
  round-trip serialization. An unknown token reports `InvalidEnumValue`.
- `xml::value_field`: captures an element's own text into a member while
  attribute fields still apply (XSD `simpleContent`, e.g.
  `<price currency="USD">9.99</price>`).
- `std::unique_ptr<T>` element members: an optional, possibly recursive child;
  null when absent, allocated when present, omitted on serialization when null.
- Variant fields via `xml::variant_field` / `required_variant_field` and
  `xml::alt<T>("name")`: maps `xs:choice` to a `std::variant<...>` member (or a
  `std::vector<std::variant<...>>` for a repeated/interleaved choice). The matched
  element selects the alternative; the serializer emits the active one.
- Built-in date/time value types `xml::Date`, `xml::Time`, `xml::DateTime` (XSD
  `date`/`time`/`dateTime`, with timezone and fractional seconds), each with
  `std::chrono` accessors; malformed input reports `InvalidValue`.
- `xml::XmlValueTraits<T>` customization point: parse/format any leaf type to and
  from its XML text form (the date types are built-in specializations).
- `std::optional<T>` element and attribute members: engaged when present, left
  empty when absent, and omitted on serialization when empty. Marking an optional
  field `required` is a compile-time error.

### Changed
- Required-field presence is tracked in a multiword mask instead of a single
  `uint64_t`, removing the 64-fields-per-type ceiling.
- `xsdgen` tool to generate TurboXML C++ definitions for provided XSD.

## 1.2.0 - 2026-06-12

Performance release. No API changes.

### Changed
- Document-order field hint: the opening-tag byte-compare fast path now covers all types, skipping tokenization and field lookup for schema-ordered XML (+18-47% throughput on element-heavy workloads).
- Document-order attribute cursor: attribute fields match at a running cursor instead of per-field linear scans (+8% on attribute-heavy workloads).
- Raw skip scanner: unmapped subtrees are skipped with a quote/comment/CDATA-aware byte scan instead of full tokenization (+19% on unknown-heavy content; new benchmark added).

## 1.1.0 - 2026-06-12

### Added
- `bool` field support: parsed from `true`/`false`/`1`/`0`, serialized as `true`/`false`.

### Changed
- Simplified internals: unified element dispatch (removed the N==1 special case and `handle_element`), merged duplicated serializer escape/number helpers and field factories.

### Fixed
- Skipped subtrees nested deeper than 65,535 levels no longer desync the parser (depth counter widened).
- CMake configure no longer fails when `TURBOXML_BUILD_TESTS` or `TURBOXML_BUILD_BENCHMARKS` is `OFF`.
- README corrections: CMake target is `TurboXML::turboxml`, Quick Start example now compiles.

## 1.0.0 - 2026-06-10

Initial release.

- Header-only C++20 XML pull-parser, deserializer, and serializer (`TurboXML.hh`).
- Declarative field mapping via `XmlMetadata<T>` with `field`, `attr_field`, `vec_field`, and `arr_field`.
- Zero-copy `std::string_view` or owning `std::string` fields; arithmetic types via `std::from_chars`.
- Compile-time FNV-1a field dispatch; extensible containers through `XmlContainerTraits`.
- Serializer with pretty/compact output and XML escaping.
- GoogleTest suite and Google Benchmark comparisons against pugixml.
