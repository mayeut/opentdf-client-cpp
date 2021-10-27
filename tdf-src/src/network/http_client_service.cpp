//
//  TDF SDK
//
//  Created by Sujan Reddy on 2019/03/04.
//  Copyright 2019 Virtru Corporation
//

#include "http_client_service.h"
#include "network_util.h"
#include "crypto/crypto_utils.h"
#include "sdk_constants.h"

#include <regex>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/rfc2818_verification.hpp>

namespace virtru :: network {

    ///
    // This is code is copied from https://www.boost.org/doc/libs/1_67_0/libs/beast/doc/html/beast/examples.html
    // https://www.boost.org/doc/libs/1_67_0/libs/beast/example/http/client/async-ssl/http_client_async_ssl.cpp
    //
    // we didn't make any attempts do any cleanup.
    // TODO: Should attempt to clean it and document the functionality
    ///
    /*
    Boost Software License - Version 1.0 - August 17th, 2003

    Permission is hereby granted, free of charge, to any person or organization
            obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
            execute, and transmit the Software, and to prepare derivative works of the
            Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
            must be included in all copies of the Software, in whole or in part, and
            all derivative works of the Software, unless such copies or derivative
            works are solely in the form of machine-executable object code generated by
            a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
            SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
            ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
            DEALINGS IN THE SOFTWARE.
            */
    namespace {
        //Secure session
        class SSLSession : public std :: enable_shared_from_this < SSLSession > {
            std::string host;
            ba::ip::tcp :: resolver resolver_;
            ba::ssl::stream <ba::ip::tcp :: socket > stream_;
            bb::flat_buffer buffer_; // (Must persist between reads)
            HttpRequest req_;
            HttpResponse res_;
            ServiceCallback cb_;
            void report (ErrorCode ec ) {
                if ( cb_ ) {
                    cb_ ( ec, move ( res_ ) );
                    cb_ = nullptr;
                }
            }

        public:
            SSLSession(
                    std::string && host,
                    ba::io_context & io,
                    ba::ssl::context & ctx,
                    HttpRequest && req,
                    ServiceCallback && cb
            ):
                    host {move ( host ) },
                    resolver_ { io },
                    stream_ { io, ctx },
                    req_ { move ( req ) },
                    cb_ { move ( cb ) }
            { }
            void start(
                    std::string_view port
            ) {
                // Set SNI Hostname (many hosts need this to handshake successfully)
                if (!SSL_set_tlsext_host_name(stream_.native_handle( ), host.c_str ( ) ) )
                    return report (
                            {
                                    static_cast < int > ( :: ERR_get_error ( ) ),
                                    ba :: error :: get_ssl_category ( )
                            }
                    );
                resolver_.async_resolve (
                        host,
                        port,
                        [ self = shared_from_this ( ) ] ( auto ec, auto & results ) {
                            self -> on_resolve ( ec, results );
                        }
                );
            }
            void on_resolve (
                    bs :: error_code ec,
                    const ba :: ip :: tcp :: resolver :: results_type & results
            ) {
                if ( ec )
                    return report ( ec );
                ba :: async_connect (
                        stream_.next_layer ( ),
                        results.begin ( ),
                        results.end ( ),
                        [ self = shared_from_this ( ) ] ( auto ec, auto & /*ep*/ ) {
                            self -> on_connect ( ec );
                        }
                );
            }
            void on_connect ( bs :: error_code ec ) {
                if ( ec )
                    return report ( ec );
                stream_.set_verify_callback (
                        ba :: ssl :: rfc2818_verification { host }
                );
                stream_.async_handshake (
                        ba :: ssl :: stream_base :: client,
                        [ self = shared_from_this ( ) ] ( auto & ec ) {
                            self -> on_handshake ( ec );
                        }
                );
            }
            void on_handshake ( bs :: error_code ec ) {
                if ( ec )
                    return report ( ec );
                // Send the HTTP request to the remote host
                bb :: http :: async_write (
                        stream_,
                        req_,
                        [ self = shared_from_this ( ) ] ( auto ec, auto bytes_transferred ) {
                            self -> on_write ( ec, bytes_transferred );
                        }
                );
            }
            void on_write (
                    bs :: error_code ec,
                    std :: size_t /*bytes_transferred*/
            ) {
                if ( ec )
                    return report ( ec );
                bb :: http :: async_read (
                        stream_,
                        buffer_,
                        res_,
                        [ self = shared_from_this ( ) ] ( auto ec, auto bytes_transferred ) {
                            self -> on_read ( ec, bytes_transferred );
                        }
                );
            }
            void on_read (
                    bs :: error_code ec,
                    std :: size_t /*bytes_transferred*/
            ) {
                if ( ec )
                    return report ( ec );
                // Gracefully close the stream
                stream_.async_shutdown (
                        [ self = shared_from_this ( ) ] ( auto ec ) {
                            self -> on_shutdown ( ec );
                        }
                );
            }
            void on_shutdown ( bs :: error_code ec ) {
                // Rationale:
                // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
                if ( ec == ba :: error :: eof )
                    ec.assign ( 0, ec.category ( ) );
                // If ec is zero then the connection is closed gracefully
                report ( ec );
            }

        };

        //Non secure session
        class Session : public std :: enable_shared_from_this < Session > {
            std::string host;
            ba::ip::tcp :: resolver resolver_;
            ba::ip::tcp ::socket stream_;
            bb::flat_buffer buffer_; // (Must persist between reads)
            HttpRequest req_;
            HttpResponse res_;
            ServiceCallback cb_;
            void report (ErrorCode ec ) {
                if ( cb_ ) {
                    cb_ ( ec, move ( res_ ) );
                    cb_ = nullptr;
                }
            }

        public:
            Session(
                    std::string && host,
                    ba::io_context & io,
                    HttpRequest && req,
                    ServiceCallback && cb
            ):
                    host {move ( host ) },
                    resolver_ { io },
                    stream_ { io },
                    req_ { move ( req ) },
                    cb_ { move ( cb ) }
            { }
            void start(
                    std::string_view port
            ) {
                resolver_.async_resolve (
                        host,
                        port,
                        [ self = shared_from_this ( ) ] ( auto ec, auto & results ) {
                            self -> on_resolve ( ec, results );
                        }
                );
            }
            void on_resolve (
                    bs :: error_code ec,
                    const ba :: ip :: tcp :: resolver :: results_type & results
            ) {
                if ( ec )
                    return report ( ec );
                ba :: async_connect (
                        stream_,
                        results.begin ( ),
                        results.end ( ),
                        [ self = shared_from_this ( ) ] ( auto ec, auto & /*ep*/ ) {
                            self -> on_connect ( ec );
                        }
                );
            }
            void on_connect ( bs :: error_code ec ) {
                if ( ec )
                    return report ( ec );

                // Send the HTTP request to the remote host
                bb :: http :: async_write (
                        stream_,
                        req_,
                        [ self = shared_from_this ( ) ] ( auto ec, auto bytes_transferred ) {
                            self -> on_write ( ec, bytes_transferred );
                        }
                );
            }

            void on_write (
                    bs :: error_code ec,
                    std :: size_t /*bytes_transferred*/
            ) {
                if ( ec )
                    return report ( ec );

                bb :: http :: async_read (
                        stream_,
                        buffer_,
                        res_,
                        [ self = shared_from_this ( ) ] ( auto ec, auto bytes_transferred ) {
                            self -> on_read ( ec, bytes_transferred );
                        }
                );
            }
            void on_read (
                    bs :: error_code ec,
                    std :: size_t /*bytes_transferred*/
            ) {
                if ( ec )
                    return report ( ec );

                // Gracefully close the stream
                stream_.shutdown (ba::ip::tcp::socket::shutdown_both, ec);
                return report (ec);
            }

        };
    } // namespace

    /// Create an instance of Service and returns a unique_ptr. On error it throws exception
    std::unique_ptr<Service> Service::Create(const std::string& url,
                                             std::string_view sdkConsumerCertAuthority,
                                             const std::string& clientKeyFileName, 
                                             const std::string& clientCertFileName) {
        
        // TODO: May want to support in future.
        // std::regex urlRegex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
        std::regex urlRegex("(http|https):\\/\\/([^\\/ :]+):?([^\\/ ]*)(\\/?[^ ]*)");
        std::cmatch what;
        if(!regex_match(url.c_str(), what, urlRegex)) {
            std::string errorMsg{"Failed to parse url, expected:'(http|https)//<domain>/<target>' actual:"};
            errorMsg.append(url);
            ThrowException(std::move(errorMsg));
        }
        
        return std::unique_ptr<Service>(new Service(std::string(what[1].first, what[1].second), // schema
                                                    std::string(what[2].first, what[2].second), // host
                                                    std::string(what[3].first, what[3].second), // port
                                                    std::string(what[4].first, what[4].second), // target
                                                    sdkConsumerCertAuthority, clientKeyFileName, 
                                                    clientCertFileName));
    }

    /// Constructor
    Service::Service(std::string&& schema, std::string&& host, std::string&& port,
                     std::string&& target, std::string_view sdkConsumerCertAuthority, const std::string& clientKeyFileName, 
                                               const std::string& clientCertFileName)
                     : m_schema{std::move(schema)}, m_host{std::move(host)} {

        // Use default ca's if the consumer of the sdk didn't provide one.
        auto certAuthority = sdkConsumerCertAuthority;

        // Load cert authority
        ErrorCode errorCode;
        if (m_schema == "https") {
            m_secure = true;
            if (certAuthority.empty()) {
                certAuthority = gloablCertAuthority;
                m_sslContext.add_certificate_authority(ba::buffer(certAuthority), errorCode);
            }
            else
            {
                m_sslContext.use_certificate_file(clientCertFileName, ba::ssl::context_base::file_format::pem);
                m_sslContext.use_private_key_file(clientKeyFileName, ba::ssl::context_base::file_format::pem); 
                m_sslContext.load_verify_file(static_cast<std::string>(certAuthority));
            }
            
        }
        else if (m_schema == "http") {
            m_secure = false;
        }

        if (errorCode) {
            ThrowBoostNetworkException(errorCode.message(), errorCode.value());
        }

        m_request.target(std::move(target));

        if (!port.empty()) {
            m_schema = std::move(port);
        }
    }

    /// Add a HTTP request header.
    void Service::AddHeader(const std::string& key, const std::string& value) {
        if (IsLogLevelDebug()) {
            std::string debugMsg;

            // Redact sensitive data
            std::string scrubbedValue;
            if (key == kAuthorizationKey) {
                scrubbedValue = value.substr(0,16) + "...\"";
            }
            else
            {
                scrubbedValue = value;
            }

            debugMsg = "AddHeader key=\"" + key + "\" Value=\"" + scrubbedValue + "\"";
            LogDebug(debugMsg);
        }
        m_request.set(key, value);
    }

    /// Execute a get request and on completion the callback is executed.
    void Service::ExecuteGet(IOContext& ioContext, ServiceCallback&& callback) {

        m_request.method(HttpVerb::get);

        if (isSSLSocket()) {
            std::make_shared<SSLSession>(move(m_host), ioContext, m_sslContext,
                                         move(m_request), move(callback))->start(m_schema);
        }
        else {
            std::make_shared<Session>(move(m_host), ioContext,
                                      move(m_request), move(callback))->start(m_schema);
        }
    }

    /// Execute a post request and on completion the callback is executed.
    void Service::ExecutePost(std::string&& body, IOContext& ioContext, ServiceCallback&& callback) {
        m_request.method(HttpVerb::post);

        m_request.body() = body;
        m_request.prepare_payload();

        if (isSSLSocket()) {
            std::make_shared<SSLSession>(move(m_host), ioContext, m_sslContext,
                                         move(m_request), move(callback))->start(m_schema);
        }
        else {
            std::make_shared<Session>(move(m_host), ioContext,
                                      move(m_request), move(callback))->start(m_schema);
        }
    }

    /// Execute a patch request and on completion the callback is executed.
    void Service::ExecutePatch(std::string&& body, IOContext& ioContext, ServiceCallback&& callback) {
        m_request.method(HttpVerb::patch);

        m_request.body() = body;
        m_request.prepare_payload();

        if (isSSLSocket()) {
            std::make_shared<SSLSession>(move(m_host), ioContext, m_sslContext,
                                         move(m_request), move(callback))->start(m_schema);
        }
        else {
            std::make_shared<Session>(move(m_host), ioContext,
                                      move(m_request), move(callback))->start(m_schema);
        }
    }

    /// Return the target of the current service.
    std::string Service::getTarget() const {
        auto target = m_request.target();
        return std::string{target.begin(), target.end()};
    }
}  // namespace virtru::network