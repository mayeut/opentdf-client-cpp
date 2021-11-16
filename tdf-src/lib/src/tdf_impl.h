//
//  TDF SDK
//
//  Created by Sujan Reddy on 2019/04/29
//  Copyright 2019 Virtru Corporation
//

#ifndef VIRTRU_TDF_IMPL_H
#define VIRTRU_TDF_IMPL_H

#include "crypto/bytes.h"
#include "tdf_constants.h"
#include "tdf_libarchive_writer.h"
#include "tdf_libarchive_reader.h"
#include "libxml2_deleters.h"

#include <boost/filesystem.hpp>
#include "nlohmann/json.hpp"

namespace virtru {

    using namespace virtru::crypto;

    /// Forward declaration
    class TDFBuilder;
    class SplitKey;
    class TDFZIPReader;

    /// Implementation of the core functionality of tdf.
    class TDFImpl {
    public: /// Interface.
        /// Constructor
        /// \param tdfBuilder - The TDFBuilder instance.
        explicit TDFImpl(TDFBuilder& tdfBuilder);

        /// Destructor
        ~TDFImpl() = default;

        /// Encrypt the file to tdf format.
        /// \param inFilepath - The file on which the encryption is performed.
        /// \param outFilepath - The file path of the tdf after successful encryption.
        void encryptFile(const std::string& inFilepath,
                         const std::string& outFilepath);

        /// Encrypt the stream data to tdf format.
        /// \param inStream - The stream containing a data to be encrypted.
        /// \param outStream - The stream containing the encrypted data.
        void encryptStream(std::istream& inStream, std::ostream& outStream);

        /// Encrypt the data that is retrieved from the source callback.
        /// \param sourceCb - A source callback to retrieve the data to be encrypted.
        /// \param sinkCb - A sink callback with the encrypted data.
        void encryptData(TDFDataSourceCb sourceCb, TDFDataSinkCb sinkCb);

        /// Decrypt the tdf file.
        /// \param inFilepath - The tdf file on which the decryption is performed.
        /// \param outFilepath - The file path of the file after successful decryption.
        void decryptFile(const std::string& inFilepath,
                         const std::string& outFilepath);
        
        /// Decrypt the tdf stream data.
        /// \param inStream - The stream containing a tdf data to be decrypted.
        /// \param outStream - The stream containing plain data.
        void decryptStream(std::istream& inStream, std::ostream& outStream);

        /// Decrypt the data that is retrieved from the source callback.
        /// \param sourceCb - A source callback to retrieve the data to be decrypted.
        /// \param sinkCb - A sink callback with the decrypted data.
        void decryptData(TDFDataSourceCb sourceCb, TDFDataSinkCb sinkCb);

        /// Return the policy uuid from the tdf file.
        /// \param tdfFilePath - The tdf file path
        /// \return - Return a uuid of the policy.
        std::string getPolicyUUID(const std::string& tdfFilePath);

        /// Return the policy uuid from the tdf input stream.
        /// \param inStream - The stream containing a tdf data.
        /// \return - Return a uuid of the policy.
        /// NOTE: virtru::exception will be thrown if there is issues while retrieving the policy uuid.
        std::string getPolicyUUID(std::istream&  inStream);
        
        /// Sync the tdf file, with symmetric wrapped key and Policy Object.
        /// \param encryptedTdfFilepath - The file path to the tdf.
        void sync(const std::string& encryptedTdfFilepath) const;

    private:
        /// Encrypt the data in the input stream to tdf format and return the manifest.
        /// \param inputStream - The input steam
        /// \param dataSize - The amount of data in the input stream.
        /// \param sinkCB - A data sink callback which is called with tdf data.
        /// \return string - The manifest of the tdf.
        std::string encryptStream(std::istream& inputStream, std::streampos dataSize, DataSinkCb&& sinkCB);

        /// Decrypt the data in TDFArchiveReader and write to the out stream.
        /// \param tdfArchiveReader - The TDF archive reader instance.
        /// \param outStream - The stream containing the decrypted data.
        void decryptStream(TDFArchiveReader& tdfArchiveReader, DataSinkCb&& sinkCB);

    private:
        /// Generate a signature of the payload base on integrity algorithm.
        /// \param payload - A payload data
        /// \param splitkey - SplitKey object holding the wrapped key.
        /// \param alg - Integrity algorithm to be used for performing signature.
        /// \return string - Result of the signature calculation.
        std::string getSignature(Bytes payload, SplitKey& splitkey, IntegrityAlgorithm alg) const ;
        
        /// Generate a signature of the payload base on integrity algorithm.
        /// \param payload - A payload data
        /// \param splitkey - SplitKey object holding the wrapped key.
        /// \param alg - Integrity algorithm as string to be used for performing signature.
        /// \return string - Result of the signature calculation.
        std::string getSignature(Bytes payload, SplitKey& splitkey, const std::string& alg) const;

        /// Build an Upsert v2 payload based on the manifest
        /// \param requestPayload - A payload object with policy and key access already added
        /// \return string - The signed jwt
        std::string buildUpsertV2Payload(nlohmann::json& requestPayload) const;

        /// Build an Upsert v1 payload based on the manifest
        /// \param requestPayload - A payload object with policy and key access already added
        void buildUpsertV1Payload(nlohmann::json& requestPayload) const;

        /// Build a Rewrap v2 payload based on the manifest
        /// \param requestPayload - A payload object with policy and key access already added
        /// \return string - The signed jwt
        std::string buildRewrapV2Payload(nlohmann::json& requestPayload) const;

        /// Build a Rewrap v1 payload based on the manifest
        /// \param requestPayload - A payload object with policy and key access already added
        void buildRewrapV1Payload(nlohmann::json& requestPayload) const;

        /// Upsert the key information.
        /// \param manifest - The manifest of the tdf.
        /// \param ignoreKeyAccessType - If true skips the key access type before
        /// syncing.
        // ignoreType if true skips the key access type check when syncing
        void upsert(nlohmann::json& manifest, bool ignoreKeyAccessType = false) const ;

        /// Unwrap the key from the manifest.
        /// \param manifest - Manifest of the encrypted tdf
        /// \return - Wrapped key.
        WrappedKey unwrapKey(nlohmann::json& manifest) const;

        /// Parse the response and retrieve the wrapped key.
        /// \param unwrapResponse - The response string from '/rewrap'
        /// \return - Wrapped key.
        WrappedKey getWrappedKey(const std::string& unwrapResponse) const;

        /// Generate a html tdf type file.
        /// \param manifest - Manifest of the tdf file.
        /// \param inputStream - input stream of tdf data.
        /// \param outStream  - output stream, on success it hold the .html tdf data.
        inline void generateHtmlTdf(const std::string& manifest,
                                    std::istream& inputStream,
                                    std::ostream& outStream);

        /// Return tdf zip data by parsing html tdf file.
        /// \param htmlTDFFilepath - A tdf file in .html format.
        /// \param manifestData - If true return manifest data otherwise return tdf zip data.
        /// \return - TDF zip data.
        std::vector<std::uint8_t> getTDFZipData(const std::string& htmlTDFFilepath,
                                                bool manifestData = false) const;

        /// Return tdf zip data by parsing html tdf file.
        /// \param bytes - The payload of the html.
        /// \param manifestData - If true return manifest data otherwise return tdf zip data.
        /// \return - TDF zip data.
        std::vector<std::uint8_t> getTDFZipData(Bytes bytes, bool manifestData = false);

        /// Return tdf zip data from XMLDoc object.
        /// \param xmlDocPtr - The unique ptr of XMLDoc object.
        /// \param manifestData - If true return manifest data otherwise return tdf zip data.
        /// \return - TDF zip data.
        std::vector<std::uint8_t> getTDFZipData(XMLDocFreePtr xmlDocPtr, bool manifestData) const;

        /// Check if the given tdf file is encrypted with zip protocol.
        /// \param inTdfFilePath - The tdf file path
        /// \return True if the tdf file is encrypted with zip protocol.
        bool isZipFormat(const std::string& inTdfFilePath) const;
        
        /// Check if the given input stream data is encrypted with zip protocol.
        /// \param inStream - The stream holding the tdf data
        /// \return True if the input stream data is encrypted with zip protocol.
        bool isZipFormat(std::istream& tdfInStream) const;

        /// Retrive the policy uuid(id) from the manifest.
        /// \param manifestStr - The tdf manifest.
        /// \return String - The policy id.
        std::string getPolicyIdFromManifest(const std::string& manifestStr) const;

    private: /// Data

        TDFBuilder& m_tdfBuilder;
        std::vector<std::uint8_t> m_zipReadBuffer;
        std::vector<std::uint8_t> m_encodeBufferSize;
    };

} // namespace virtru

#endif // VIRTRU_TDF_IMPL_H
