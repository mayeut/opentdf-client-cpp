# C++ OSS Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

### Added
0.1.1
- PLAT-1381 Change all TDF mentions to TDF

0.1.0
- Change version numbering in preparation for opentdf

1.2.9
- PLAT-1324 Change the library name from tdf to opentdf

1.2.8
- PLAT-1345 C++ SDK support PE authorization for OIDC
- 
1.2.7
- PLAT-1323 Remove the owner field from OIDCCredentials class
  
1.2.6
- PLAT-1273 API to add attributes to the policy and API to get the subject attributes from OIDC

1.2.5
- PLAT-1188 NanoTDF IV changed from 3 bytes to 12 bytes(Fix the container size)

1.2.4
- PLAT-1242 OIDC flow should validate request body for NanoTDF/TDF rewrap request

1.2.3
- PLAT-1234 Openstack OIDC support for NanoTDF

1.2.2
- PLAT-1223 Openstack OIDC support for TDF

1.2.1
- PLAT-1178 Updated conan remotes and version tags

1.2.0
- PLAT-1209 C# Bindings
- PLAT-1188 NanoTDF IV changed from 3 bytes to 12 bytes
- PLAT-1158 New API for encrypt/decrypt to deal with bytes
- PLAT-1146 Add Java bindings
- PLAT-1149 Interface to validate the NanoTDF schema
- SA-275 Add initial OIDC auth and KAS v2 API support
- PLAT-1031 Remove references to obsolete kas_public_key endpoint
- PLAT-1015 Don't use ECDSA by default
- PLAT-791 Dataset nano TDF first version of changes
- PLAT-617 adds `withDataAttributes` function to client to allow user to add data attributes
- PLAT-624 adds `getEntityAttributes` function to client to allow user to read entity attributes
- PLAT-445 Nano TDF first version of changes
- PLAT-700 Support for Nano TDF

1.0.5: 
- SA-177 Support for VJWT
- SA-177 Added setUser method to support VJWT
- SA-228 Avoid resetting LogLevel, add LogLevel::Current

### Fixed