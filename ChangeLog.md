# Changelog

## 1.1.0 - 2026-07-02

### Changed
- `xmlight::validate()` recurses through it's own, nested objects, containers, optionals, `unique_ptr`s, and variants `XmlConstraints`.
- `NormalizingParser` expands references in `xs:list` values before splitting (`string_view` items stay raw).
- xsdgen resolves `xs:include` transitively.
- Consolidated parser and codegen internals.

### Fixed
- xsdgen emitted uncompilable C++ for inline types in `xs:group` definitions, facets on `xs:choice` branches, multiple choices in one type, and enum-typed `default`/`fixed` values.
- Inline `<xs:simpleType>` on attributes (enums, facets) was silently dropped to `std::string`.
- `xmlight::validate()` did not compile when instantiated.

### Added
- Install rules and CMake package config: `find_package(LightningXML)` with `LightningXML::lightningxml`.
- Build-time test that compiles and round-trips xsdgen output from a kitchen-sink schema.

### Removed
- Leftover TurboXML naming and duplicated CMake/README sections.

## 1.0.0 - 2026-07-01 

Initial Release