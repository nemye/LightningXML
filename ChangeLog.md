# Changelog

## Unreleased

### Changed
- Public entry points (`deserialize`, `serialize`, `Serializer::write`, `valueField`, `Parser::pull`) constrained with C++20 concepts
- Compile-time field-kind queries consolidated into variable templates (matching `FIELD_COUNT`/`FIELD_SEQ`); `optionalsNotRequired` reuses `anyFieldSatisfies`.
- Serializer escape scanning

### Fixed
- date/dateTime year with 19+ digits overflowed a signed accumulator (undefined behavior)

### Added
- `StrictParser` rejects control bytes outside the XML `Char` production in character data, CDATA, attribute values, comments, and PIs (`ErrorCode::ForbiddenControlChar`)
- gcov coverage CI with 80% line floor
- Tests for parser guard paths, serializer escaping, multi-byte character references, and date/time

## 1.1.0 - 2026-07-02

### Changed
- `xmlight::validate()` recurses through it's own, nested objects, containers, optionals, unique_ptrs, and variants `XmlConstraints`.
- `NormalizingParser` expands references in `xs:list` values before splitting (`string_view` items stay raw).
- xsdgen resolves `xs:include` transitively.
- Consolidated parser and codegen internals.
- Attribute parsing improved
- Reduce allocations when parsing into `std::vector`

### Fixed
- xsdgen emitted uncompilable C++ for inline types in `xs:group` definitions, facets on `xs:choice` branches, multiple choices in one type, and enum-typed `default`/`fixed` values.
- Inline `<xs:simpleType>` on attributes (enums, facets) was silently dropped to `std::string`.
- `xmlight::validate()` did not compile when instantiated in certain cases.

### Added
- Install rules and CMake package config: `find_package(LightningXML)` with `LightningXML::lightningxml`.
- Build-time test that compiles and round-trips xsdgen output from a kitchen-sink schema.
- Unity and LTO build options
- Minimal LightningXML serializer benchmarking

### Removed
- Leftover TurboXML naming and duplicated CMake/README sections.

## 1.0.0 - 2026-07-01 

Initial Release