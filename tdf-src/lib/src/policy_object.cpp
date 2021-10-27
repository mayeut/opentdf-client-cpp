//
//  TDF SDK
//
//  Created by Sujan Reddy on 2019/03/05.
//  Copyright 2019 Virtru Corporation
//

#include "policy_object.h"
#include "tdf_exception.h"
#include "logger.h"
#include "sdk_constants.h"

#include <memory>
#include <tao/json.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <iostream>
#include <exception>
#include <typeinfo>
#include <stdexcept>
#include <regex>
#include <boost/exception/diagnostic_information.hpp>

namespace virtru {

    /*
     * {
     *  "uuid": "1111-2222-33333-44444-abddef-timestamp",
     *  "body": {
     *     "dataAttributes": [<Attribute Object>],
     *     "dissem": ["user-id@domain.com"]
     *  }
     * }
     */

    /// Default constructor for creating an instance of PolicyObject.
    PolicyObject::PolicyObject()  {
        m_uuid = to_string(boost::uuids::random_generator{}());
    }

    /// Constructs PolicyObject by parsing 'policyObjectStr' json string. On error
    /// throw an virtru::Exception
    PolicyObject PolicyObject::CreatePolicyObjectFromJson(const std::string& policyObjectJsonStr) {

        PolicyObject policyObject{};

        try {
            tao::json::value policyObjectJson = tao::json::from_string(policyObjectJsonStr);

            // Get uuid
            policyObject.m_uuid = policyObjectJson.as<std::string_view>(kUid);

            // Get dissems
            policyObject.m_dissems.clear();
            auto& dissmesArray = policyObjectJson[kBody][kDissem].get_array();
            for (auto& dissem : dissmesArray) {
                // NOTE: Not sure we want to check if the dissem is always the
                // email address could be cert CN
                //checkIsValidEmailAndThrow(dissem.get_string());

                policyObject.m_dissems.insert(dissem.get_string());
            }

            // Get attribute objects
            auto& attributeObjects = policyObjectJson[kBody][kDataAttributes].get_array();
            for (auto& attributeObject : attributeObjects) {
                policyObject.m_attributeObjects.emplace_back(AttributeObject(to_string(attributeObject)));
            }

        } catch (...) {
            ThrowException(boost::current_exception_diagnostic_information());
        }

        return policyObject;
    }

    /// Add dissem(user/entity) to this policy object.
    PolicyObject& PolicyObject::addDissem(const std::string& dissem) {
    
        // NOTE: Not sure we want to check if the dissem is always the
        // email address could be cert CN
        //checkIsValidEmailAndThrow(dissem);

        m_dissems.insert(dissem);
        return *this;
    }

    /// Add attribute object to this policy object.
    PolicyObject& PolicyObject::addAttributeObject(const AttributeObject& attributeObject) {
        m_attributeObjects.emplace_back(attributeObject);
        return *this;
    }

    /// Return the UUID of this policy object.
    std::string PolicyObject::getUuid() const {
        return m_uuid;
    }

    /// Return the set of all the dissems of this policy object.
    std::set<std::string> PolicyObject::getDissems() const {
        return m_dissems;
    }

    /// Return the vector of all the attribute objects of this policy object.
    std::vector<AttributeObject> PolicyObject::getAttributeObjects() const {
        return m_attributeObjects;
    }

    /// Return a json string representation of this policy object.
    std::string PolicyObject::toJsonString(bool prettyPrint) const {
        tao::json::value policy;

        policy[kUid] = m_uuid;
        auto& body = policy[kBody];

        body[kDataAttributes] = tao::json::empty_array;
        for (auto& artributeObject : m_attributeObjects) {
            body[kDataAttributes].emplace_back(tao::json::from_string(artributeObject.toJsonString(prettyPrint)));
        }

        body[kDissem] = tao::json::empty_array;
        for (auto& dissem : m_dissems) {
            body[kDissem].emplace_back(dissem);
        }

        if (prettyPrint) {
            return to_string(policy, 2);
        } else {
            return to_string(policy);
        }
    }

    /// Crete PolicyObject by copying the data(not unique identifier)
    PolicyObject PolicyObject::CopyDataFromPolicyObject(const PolicyObject& policyObject) {
        auto newPolicyObject = policyObject;
        newPolicyObject.m_uuid = to_string(boost::uuids::random_generator{}());
        return newPolicyObject;
    }

    // Provide default implementation.
    PolicyObject::~PolicyObject()  = default;
    PolicyObject::PolicyObject(const PolicyObject&) = default;
    PolicyObject& PolicyObject::operator=(const PolicyObject&) = default;
    PolicyObject::PolicyObject(PolicyObject&&) = default;
    PolicyObject& PolicyObject::operator=(PolicyObject&&) = default;
}  // namespace virtru