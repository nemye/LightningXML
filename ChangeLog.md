# Changelog

## 2.0.0 - 2026-06-28

### Added

**Parser tiers** (`include/TurboXML.hh`)
- `xml::NormalizingParser`: EOL normalization, character-reference expansion,
  and attribute-value whitespace normalization per XML 1.0 §3.3.3.
- `xml::StrictParser`: all of the above plus rejection of `]]>` in text,
  bare `<` in attribute values, and duplicate attributes.
- Both are aliases of `BasicParser<ParserOptions>`; `xml::Parser` is unchanged.

**Conformance fixes** (`include/TurboXML.hh`)
- UTF-8 BOM (`\xEF\xBB\xBF`) stripped before parsing.
- DOCTYPE internal subset skipped with bracket-depth tracking; declarations
  containing `>` (e.g. `<!ATTLIST name CDATA "val>ue">`) no longer truncate
  the skip.
- Conformance test suite (`test/test_Conformance.cc`).

**New field types**
- `xml::listField(M C::*m)`: whitespace-delimited list into any sequence container (`xs:list`).
- `xml::valueField(M C::*m)`: element character content alongside attribute
  fields (`xs:simpleContent`, e.g. `<price currency="USD">9.99</price>`).
- `xml::variantField(M C::*m, alt<T>...)` /
  `xml::requiredVariantField(M C::*m, alt<T>...)`: maps `xs:choice` to a
  `std::variant<...>` member (or `std::vector<std::variant<...>>` for repeated
  choice). `xml::alt<T>("name")` names each alternative.

**New member types**
- `std::optional<T>` elements and attributes: engaged when present, empty when absent, omitted on serialization when empty.
- `std::unique_ptr<T>` elements: null when absent, allocated when present; supports self-referential types.
- `std::variant<Ts...>` via variant fields above.
- Enum members via `xml::XmlEnumTraits<E>` specialization;
  `xml::enumTable<E>(entries)` helper builds the name->enumerator table.
  Unknown tokens report `InvalidEnumValue`.
- Custom leaf types via `xml::XmlValueTraits<T>` (`parse` / `format` statics).
- `xml::Date`, `xml::Time`, `xml::DateTime`: XSD `date`/`time`/`dateTime` with
  timezone and fractional-second support and `std::chrono` accessors.

**Error reporting**
- `xml::ErrorCode` enum: `Parser::errorCode()` returns a specific failure reason
  after a failed `deserialize()` (e.g. `UnterminatedComment`, `ElementMismatch`,
  `RootElementNotFound`). Cleared by `reset()`.

**Constraint validation**
- `xml::XmlConstraints<T>`: specializable trait; `static check(const T&)`
  returns `std::optional<std::string>` (nullopt = valid). Default is a no-op.
- `xml::validate(obj)` -> `std::optional<xml::ValidationError>`: calls the
  trait recursively. Empty = valid.
- `xml::ValidationError`: message + field path, distinct from `xml::ErrorCode`.

**XSD code generator** (`tools/XSDCodegen.hh`, `tools/XSDGen.cc`)
- `xsd::Generator` reads an XSD schema (parsed with TurboXML) and emits C++
  struct definitions with `XmlMetadata<T>` specializations.
  - `xs:complexType` / `xs:element` / `xs:attribute` -> struct + fields
  - `xs:simpleType` enumeration -> `enum class` + `XmlEnumTraits<E>`
  - `xs:simpleContent` -> `valueField`
  - `xs:choice` -> `variantField`
  - `xs:complexContent extension` -> struct inheritance with merged
    `XmlMetadata<Child>` prepending parent fields; recursive
  - `xs:attributeGroup` / `xs:group` -> expanded inline at codegen time
  - `xs:include` -> loaded via `xsd::Options::loader` callback; CLI resolves
    paths relative to the input schema's directory
  - Facets (`minLength`, `maxLength`, `length`, `pattern`, `minInclusive`,
    `maxInclusive`, `minExclusive`, `maxExclusive`) -> `XmlConstraints<T>`
    specializations; `pattern` uses `<regex>`
  - Attribute `default` -> C++ default member initializer
  - Attribute `fixed` -> default initializer + `XmlConstraints<T>` equality check
  - Finite `maxOccurs=N` -> `XmlConstraints<T>` size check
  - Unsupported constructs reported in `Generator::Result::notes`
- CMake option `TURBOXML_BUILD_CODEGEN` (default `ON`).

### Changed
- Required-field tracking widened from a single `uint64_t` to a multiword bitmask, removing the 64-fields-per-type limit.

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
- Declarative field mapping via `XmlMetadata<T>` with `field`, `attrField`, `vecField`, and `arrField`.
- Zero-copy `std::string_view` or owning `std::string` fields; arithmetic types via `std::from_chars`.
- Compile-time FNV-1a field dispatch; extensible containers through `XmlContainerTraits`.
- Serializer with pretty/compact output and XML escaping.
- GoogleTest suite and Google Benchmark comparisons against pugixml.
