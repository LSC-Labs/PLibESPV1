#ifndef DEBUG_LSC_JSON
    #undef DEBUGINFOS
#endif
#include "JsonNode.h"
#include "LSCUtils.h"
#include <math.h>
#include "DevelopmentHelper.h"

namespace { void jsonQuoted(String& out, const String& value); }

/**
 * @brief Create a JSON node with an optional name and explicit node type.
 */
CJsonNode::CJsonNode(const char* pszName, ELEMENT_TYPE eType) {
    Name = pszName;
    this->m_nObjectType = eType;
};

/**
 * @brief Create a named JSON value node initialized with text data.
 */
CJsonNode::CJsonNode(const char* pszName, const char* pszValue) {
    this->m_strValue = pszValue;
    this->Name = pszName;
    this->m_nObjectType = ELEMENT_TYPE::VALUE;
}

/**
 * @brief Delete all child nodes and reset this node to an empty container.
 */
void CJsonNode::clear() {
    for (CJsonNode* pEntry : Elements) {
        delete(pEntry);
    }
    Elements.clear();
}

/**
 * @brief Return the parent node, or nullptr for a root node.
 */
JsonNode * CJsonNode::getParentNode() {
    return(m_pParentNode);
}

/**
 * @brief Store the parent pointer used for tree navigation.
 */
void CJsonNode::setParentNode(JsonNode * pParentNode) {
    m_pParentNode = pParentNode;
}

/**
 * @brief Find a CJsonNode with the name
 * The name is either a node name (unlimited size) or
 * a path to the node name like "data.const.size".
 * @return the node or nullptr if not found.
 */
CJsonNode* CJsonNode::find(const char* pszName, bool bResolveName ) {
    CJsonNode* pResult = nullptr;
    if(pszName) {
        int nDeliIdx = bResolveName ? LSC::indexOf(pszName,'.') : -1;
        // If a '.' is in the name, scan for the first name and handover to the next
        if(nDeliIdx > -1) {
            String segment;
            for (int i = 0; i < nDeliIdx; ++i) segment += pszName[i];
            CJsonNode *pSubNode = find(segment.c_str(), false);
            if(pSubNode) {
                pResult = pSubNode->find(&pszName[nDeliIdx + 1]);
            }
        } else {
            for (CJsonNode* pEntry : Elements) {
                if (pEntry->Name == pszName) {
                    pResult = pEntry;
                    break;
                }
            }
        }
    }
    return(pResult);
}

/**
 * @brief Ensure that all object nodes before the final path element exist.
 *
 * For example, "wifi.ip.address" creates/returns the path up to "ip"; the
 * caller then creates or looks up the final "address" element.
 */
CJsonNode* CJsonNode::createJsonPathToElement(const char * pszName) {
    CJsonNode *pResult = this;
    int nDeliIdx = LSC::indexOf(pszName,'.');
    if(nDeliIdx > -1) {
        String segment;
        for (int i = 0; i < nDeliIdx; ++i) segment += pszName[i];
        CJsonNode *pSubNode = getObject(segment.c_str(),true);
        pResult = pSubNode->createJsonPathToElement(&pszName[nDeliIdx + 1]);
    }
    return(pResult);
}

/**
 * @brief Split a dotted JSON path and return the final element name.
 * @return true if the supplied name contained a path delimiter.
 */
bool CJsonNode::getNameFromJsonPath(const char * pszName, String & strName) {
    strName = pszName;
    int nLastIdx = LSC::lastIndexOf(pszName,'.');
    if(nLastIdx > -1) strName = &pszName[nLastIdx + 1];
    return(nLastIdx > -1);
}


/**
 * @brief Return an existing value element or create it on demand.
 */
CJsonNode & CJsonNode::operator[](const char *pszName) {
    return(*getElement(pszName,true));
}

/**
 * @brief String overload for value element access.
 */
CJsonNode & CJsonNode::operator[](String & strName) {
    return(*getElement(strName.c_str(),true));
}

/**
 * @brief checks if the key name exists in this node.
 * It searches for elements and objects..
 */
bool CJsonNode::exists(const char* pszName) {
    bool bFound = find(pszName) != nullptr;
    return(bFound);
}

/**
 * @brief Return the stored node type.
 */
CJsonNode::ELEMENT_TYPE CJsonNode::getType() {
    return(m_nObjectType);
}

/** @brief Return true when this node is a JSON object. */
bool CJsonNode::isJsonObject() { return(m_nObjectType == ELEMENT_TYPE::OBJECT); }
/** @brief Return true when this node is a JSON array. */
bool CJsonNode::isJsonArray() { return(m_nObjectType == ELEMENT_TYPE::ARRAY); }
/** @brief Return true when this node is a scalar JSON value. */
bool CJsonNode::isJsonValue() { return(m_nObjectType == ELEMENT_TYPE::VALUE); }
/** @brief Return true when the named child exists and is an object. */
bool CJsonNode::isJsonObject(const char *pszName) { 
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isJsonObject(): false); 
}
/** @brief Return true when the named child exists and is an array. */
bool CJsonNode::isJsonArray(const char *pszName) { 
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isJsonArray(): false); 
}
/** @brief Return true when the named child exists and is a scalar value. */
bool CJsonNode::isJsonValue(const char *pszName) { 
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isJsonValue(): false);
}

/**
 * @brief Remove and delete the direct child with the given name.
 */
void CJsonNode::remove(const char* pszName) {
    size_t nCurIndex = 0;
    for (CJsonNode* pEntry : Elements) {
        if (pEntry->Name == pszName) {
            delete(pEntry);
            Elements.erase(Elements.begin() + nCurIndex);
            break;
        }
        nCurIndex++;
    }
}

#pragma region set the value

/**
 * @brief Convert this node into a scalar value and remember quote handling.
 */
void CJsonNode::setNodeValueType(bool bWriteWithQuotes) { 
    clear();
    m_nObjectType = ELEMENT_TYPE::VALUE; 
    m_bWriteValueWithQuotes = bWriteWithQuotes; 
}

/**
 * @brief Store a quoted string value in this node.
 */
CJsonNode* CJsonNode::setValue(const char* pszValue) {
    this->m_strValue = (char *) pszValue;
    setNodeValueType(true);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

/**
 * @brief Store a quoted String value in this node.
 */
CJsonNode* CJsonNode::setValue(String & strValue) {
    this->m_strValue = (char *) strValue.c_str();
    setNodeValueType(true);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}


/**
 * @brief Store an unquoted boolean value in this node.
 */
CJsonNode* CJsonNode::setValue(bool bValue) {
    this->m_strValue = bValue ? "true" : "false";
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

/**
 * @brief Store an unquoted signed integer value in this node.
 */
CJsonNode* CJsonNode::setValue(int nValue) {
    // to avoid to be interpreted as character by string class, print to buffer first...
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%d",nValue);
    this->m_strValue = szBuffer;
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

/**
 * @brief Store an unquoted unsigned integer value in this node.
 */
CJsonNode* CJsonNode::setValue(unsigned int unValue) {
    // to avoid to be interpreted as character by string class, print to buffer first...
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%u",unValue);
    this->m_strValue = szBuffer;
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

/**
 * @brief Store an unquoted signed long value in this node.
 */
CJsonNode* CJsonNode::setValue(long lValue) {
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%ld",lValue);
    this->m_strValue = szBuffer;
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

CJsonNode* CJsonNode::setValue(signed long long llValue) {
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%lld",llValue);
    this->m_strValue = szBuffer;
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

/**
 * @brief Store an unquoted unsigned long value in this node.
 */
CJsonNode* CJsonNode::setValue(unsigned long ulValue) {
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%lu",ulValue);
    this->m_strValue = szBuffer;
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

/**
 * @brief Store an unquoted floating point value, but only if it is not NaN.
 */
CJsonNode* CJsonNode::setValue(float fValue) {
    // NaN / infinite can not be written as JSON number ("nan" is invalid JSON) => null
    // (note: "fValue != NAN" is always true - NaN is not equal to anything)
    if(isnan(fValue) || isinf(fValue)) {
        this->m_strValue = "";              // unquoted empty value is written as null
        setNodeValueType(false);
    } else {
        char szBuffer[256];
        snprintf(szBuffer,sizeof(szBuffer),"%f",fValue);
        this->m_strValue = szBuffer;
        setNodeValueType(false);
        DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    }
    return(this);
}

/**
 * @brief Set or create a named string value.
 */
CJsonNode* CJsonNode::setValue(const char* pszName, const char * pszValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(pszValue));
}

/**
 * @brief Set or create a named String value.
 */
CJsonNode* CJsonNode::setValue(const char* pszName, String &  strValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(strValue));
}


/**
 * @brief Set or create a named boolean value.
 */
CJsonNode* CJsonNode::setValue(const char* pszName, bool bValue) {
     CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(bValue));
}

/**
 * @brief Set or create a named signed integer value.
 */
CJsonNode* CJsonNode::setValue(const char* pszName, int nValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(nValue));
}

/**
 * @brief Set or create a named floating point value.
 */
CJsonNode* CJsonNode::setValue(const char* pszName, float fValue) {
    CJsonNode * pNode = getElement(pszName,true);
    if(fValue != NAN) {
        pNode->setValue(fValue);
    }
    return(pNode);
}

/**
 * @brief Set or create a named signed long value.
 */
CJsonNode* CJsonNode::setValue(const char* pszName, long lValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(lValue));
}

/**
 * @brief Set or create a named unsigned long value.
 */
CJsonNode* CJsonNode::setValue(const char* pszName, unsigned long ulValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(ulValue));
}

#pragma endregion

#pragma region get the value 

/**
 * @brief Return this node's value as String, or the supplied default.
 */
String & CJsonNode::getValueAsString(String & strDefault) {
    return(m_strValue.c_str() == nullptr ? strDefault : m_strValue);
}

/**
 * @brief Return a named child value as String, or the supplied default.
 */
String & CJsonNode::getValueAsString(const char *pszName, String & strDefault) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->getValueAsString(strDefault): strDefault);
}


/**
 * @brief Return this node's raw value text.
 */
const char* CJsonNode::getValue() {
    return(m_strValue.c_str());
}

/**
 * @brief Return this node's value as C string, or the supplied default.
 */
const char* CJsonNode::getValueAsCharPointer(const char *pszDefault) {
    const char *pszResult = m_strValue.c_str();
    return(pszResult ? pszResult : pszDefault);
}

/**
 * @brief Return a named child value as C string, or the supplied default.
 */
const char* CJsonNode::getValueAsCharPointer(const char *pszName, const char *pszDefault) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->getValueAsCharPointer(pszDefault) : pszDefault );
}

/**
 * @brief Return a named child value or, for value nodes, this node's value.
 */
const char* CJsonNode::getValue(const char* pszName, const char *pszDefault) {
    const char* pszResult = pszDefault;
    CJsonNode* pNode;
    switch (m_nObjectType) {
    case ELEMENT_TYPE::OBJECT:
        pNode = this->find(pszName);
        if (pNode) pszResult = pNode->getValue();
        break;

    case ELEMENT_TYPE::VALUE:
        pszResult = m_strValue.c_str();
        break;
    default:
        break;
    }
    return(pszResult ? pszResult : pszDefault);
}

/**
 * @brief Parse this node's numeric value as int.
 */
int CJsonNode::getValueAsInt(int nDefault) {
    int nResult = nDefault;
    if (isNumberValue()) nResult = atoi(m_strValue.c_str());
    return(nResult);
}

/**
 * @brief Parse a named child value as int.
 */
int CJsonNode::getValueAsInt(const char* pszName, int nDefault) {
    CJsonNode* pNode = find(pszName);
    int nResult = pNode ? pNode->getValueAsInt(nDefault) : nDefault;
    return(nResult);
}


/**
 * @brief Parse this node's numeric value as unsigned int.
 */
unsigned int CJsonNode::getValueAsUnsignedInt(unsigned int nDefault) {
    return((unsigned int) getValueAsInt((int) nDefault));
}

/**
 * @brief Parse a named child value as unsigned int.
 */
unsigned int CJsonNode::getValueAsUnsignedInt(const char* pszName, unsigned int nDefault) {
    return((unsigned int) getValueAsInt(pszName,(int) nDefault));
}

/**
 * @brief Parse this node's numeric value as long.
 */
long CJsonNode::getValueAsLong(long lDefault) {
    long nResult = lDefault;
    if (isNumberValue()) nResult = atol(m_strValue.c_str());
    return(nResult);
}

/**
 * @brief Parse a named child value as long.
 */
long CJsonNode::getValueAsLong(const char* pszName, long lDefault) {
    CJsonNode* pNode = find(pszName);
    long nResult = pNode ? pNode->getValueAsLong(lDefault) : lDefault;
    return(nResult);
}

/**
 * @brief Parse this node's numeric value as unsigned long.
 */
unsigned long CJsonNode::getValueAsUnsignedLong( unsigned long ulDefault) {
    unsigned long nResult = ulDefault;
    if (isNumberValue()) nResult = atol(m_strValue.c_str());
    return(nResult);
}
/**
 * @brief Parse a named child value as unsigned long.
 */
unsigned long CJsonNode::getValueAsUnsignedLong(const char* pszName, unsigned long ulDefault) {
    CJsonNode* pNode = find(pszName);
    unsigned long nResult = pNode ? pNode->getValueAsUnsignedLong(ulDefault) : ulDefault;
    return(nResult);
}

/**
 * @brief Parse this node's numeric value as float.
 */
float CJsonNode::getValueAsFloat(float fDefault) {
    double fResult = fDefault;
    if (isNumberValue()) fResult = atof(m_strValue.c_str());
    return(fResult);
}

/**
 * @brief Parse a named child value as float.
 */
float CJsonNode::getValueAsFloat(const char* pszName, float fDefault) {
    CJsonNode* pNode = find(pszName);
    float fResult = pNode ? pNode->getValueAsFloat(fDefault) : fDefault;
    return(fResult);
}


/**
 * @brief Parse this node's boolean value.
 */
bool CJsonNode::getValueAsBool(bool bDefault) {
    bool bResult = bDefault;
    if (isBooleanValue()) bResult = LSC::isTrueValue(m_strValue.c_str()); 
    return(bResult);
}

/**
 * @brief Parse a named child value as bool.
 */
bool CJsonNode::getValueAsBool(const char* pszName, bool bDefault) {
    CJsonNode* pNode = find(pszName);
    bool bResult = pNode ? pNode->getValueAsBool(bDefault) : bDefault;
    return(bResult);
}

#pragma endregion

#pragma region store value (if) functions

/**
 * @brief Store the value, if it is a number value
 * @result true, if value is stored, otherwise false
 */
bool CJsonNode::storeValueIf(bool* pbTarget) {
    bool bResult = false;
    if (isBooleanValue()) {
        *pbTarget = getValueAsBool(*pbTarget);
        bResult = true;
    }
    return(bResult);
}

/**
 * @brief Store the value form the given key into the target,
 * if the given key exists and it is a number.
 */
bool CJsonNode::storeValueIf(const char* pszName, bool* pbTarget) {
    bool bResult = false;
    CJsonNode* pNode = find(pszName);
    if (pNode) bResult = pNode->storeValueIf(pbTarget);
    return(bResult);
}


/**
 * @brief Copy this node's text into the target string when a value is present.
 */
bool CJsonNode::storeValueIf(String & strTarget) {
    bool bResult = false;
    if(m_strValue.c_str() != nullptr) {
        strTarget = m_strValue.c_str();
        bResult = true;
    }
    return(bResult);
}

/**
 * @brief Copy this node's text unless it matches the excluded marker value.
 */
bool CJsonNode::storeValueIfNot(String & strTarget,const char *pszIfNot) {
    bool bResult = false;
    if(m_strValue.c_str() != nullptr && pszIfNot) {
        if(m_strValue != pszIfNot) {
            strTarget = m_strValue.c_str();
            bResult = true;
        }
    }
    return(bResult);
}

/**
 * @brief Copy a named child text value into the target string when present.
 */
bool CJsonNode::storeValueIf(const char* pszName, String & strTarget) {
    bool bResult = false;
    CJsonNode* pNode = find(pszName);
    if (pNode) bResult = pNode->storeValueIf(strTarget);
    return(bResult);
}

/**
 * @brief Copy a named child text value unless it matches the excluded marker.
 */
bool CJsonNode::storeValueIfNot(const char* pszName, String & strTarget, const char *pszIfNot) {
    bool bResult = false;
    CJsonNode* pNode = find(pszName);
    if (pNode) bResult = pNode->storeValueIfNot(strTarget,pszIfNot);
    return(bResult);
}



/**
 * @brief Store the value, if it is a number value
 * @result true, if value is stored, otherwise false
 */
bool CJsonNode::storeValueIf(int* pnTarget) {
    bool bResult = false;
    if (isNumberValue()) {
        *pnTarget = getValueAsInt(*pnTarget);
        bResult = true;
    }
    return(bResult);
}

/**
 * @brief Store the value form the given key into the target,
 * if the given key exists and it is a number.
 */
bool CJsonNode::storeValueIf(const char* pszName, int* pnTarget) {
    bool bResult = false;
    CJsonNode* pNode = find(pszName);
    if (pNode) bResult = pNode->storeValueIf(pnTarget);
    return(bResult);
}

/**
 * @brief Store the value, if it is a number value
 * @result true, if value is stored, otherwise false
 */
bool CJsonNode::storeValueIf(unsigned int* pnTarget) {
    bool bResult = false;
    if (isNumberValue()) {
        *pnTarget = getValueAsUnsignedInt(*pnTarget);
        bResult = true;
    }
    return(bResult);
}

/**
 * @brief Store the value form the given key into the target,
 * if the given key exists and it is a number.
 */
bool CJsonNode::storeValueIf(const char* pszName, unsigned int* pnTarget) {
    bool bResult = false;
    CJsonNode* pNode = find(pszName);
    if (pNode) bResult = pNode->storeValueIf(pnTarget);
    return(bResult);
}

/**
 * @brief Store the value, if it is a number value
 * @result true, if value is stored, otherwise false
 */
bool CJsonNode::storeValueIf(long * pnTarget) {
    bool bResult = false;
    if (isNumberValue()) {
        *pnTarget = getValueAsLong(*pnTarget);
        bResult = true;
    }
    return(bResult);
}

/**
 * @brief Store the value form the given key into the target,
 * if the given key exists and it is a number.
 */
bool CJsonNode::storeValueIf(const char* pszName, long * pnTarget) {
    bool bResult = false;
    CJsonNode* pNode = find(pszName);
    if (pNode) bResult = pNode->storeValueIf(pnTarget);
    return(bResult);
}







/**
 * @brief Store the value, if it is a number value
 * @result true, if value is stored, otherwise false
 */
bool CJsonNode::storeValueIf(float* pfTarget) {
    bool bResult = false;
    if (isNumberValue()) {
        *pfTarget = getValueAsFloat(*pfTarget);
        bResult = true;
    }
    return(bResult);
}

/**
 * @brief Store the value form the given key into the target,
 * if the given key exists and it is a number.
 */
bool CJsonNode::storeValueIf(const char* pszName, float* pfTarget) {
    bool bResult = false;
    CJsonNode* pNode = find(pszName);
    if (pNode) bResult = pNode->storeValueIf(pfTarget);
    return(bResult);
}

#pragma endregion

#pragma region get objects, arrays and element nodes

/**
 * @brief Get a node of the given type by name or path ("a.b.c").
 *
 * The node is searched from this node along the full path. If it is not found
 * and bCreateIfNotExist is true, the missing path objects are created and the
 * node is appended to its real parent (the last path object).
 * An existing node of another type is cleared and converted when
 * bCreateIfNotExist is true, otherwise nullptr is returned.
 */
CJsonNode* CJsonNode::getTypedNode(const char* pszName, ELEMENT_TYPE eType, bool bCreateIfNotExist) {
    if(!pszName) return(nullptr);
    CJsonNode* pNode = find(pszName);
    if (pNode == nullptr && bCreateIfNotExist) {
        CJsonNode* pParent = createJsonPathToElement(pszName);
        String strName;
        getNameFromJsonPath(pszName,strName);
        pNode = new CJsonNode(strName.c_str(), eType);
        pNode->setParentNode(pParent);
        pParent->Elements.push_back(pNode);
    }
    // Clear and convert if wrong type when CreateIfNotExist is set to true
    if(pNode && pNode->m_nObjectType != eType && bCreateIfNotExist) {
        DEBUG_INFOS("JSON: -> converting %d > %d",pNode->m_nObjectType, eType);
        pNode->clear();
        pNode->m_strValue = "";
        pNode->m_bWriteValueWithQuotes = false;
        pNode->m_nObjectType = eType;
    }
    return((pNode && pNode->getType() == eType) ? pNode : nullptr);
}

/**
 * @brief get an element as object.
 * returns the named element if it is an object.
 * if bCreateIfNotExist is set to true:
 * - the node will be created if it does not exist
 * - the node will be converted if it is of wrong type (existing data will be deleted)
 * if bCreateIfNotExist is set to false:
 * - if the node exists and is of type object, the result is the node
 * - if the node exists and is of wrong type, the result is nullptr
 * - if the node does not exists, the result is nullptr
 * @param pszName Name of the element requested
 * @param bCreateIfNotExist If true, the element will be created or replaced
 * @returns the Node or nullptr if no node is in place
 */
CJsonNode* CJsonNode::getObject(const char* pszName, bool bCreateIfNotExist) {
    return(getTypedNode(pszName, ELEMENT_TYPE::OBJECT, bCreateIfNotExist));
}

/**
 * @brief Create an (empty) element
 * Will not guarantee valid JSON structure - use only at right place
 * - if a node exists with this name, it will be cleared()
 */
CJsonNode* CJsonNode::createElement(const char *pszName) {
    CJsonNode *pNode = nullptr;
    if(pszName) pNode = find(pszName,false);
    if(!pNode) {
        pNode = new CJsonNode();
        pNode->setParentNode(this);
        if(pszName) pNode->Name = pszName;
        Elements.push_back(pNode);
    }
    else {
        // Existing node is reused - it is already in Elements (no second insert,
        // otherwise it would be deleted twice by clear())
        pNode->clear();
        pNode->m_strValue = "";
    }
    pNode->m_nObjectType = ELEMENT_TYPE::VALUE;
    return(pNode);
}

/**
 * @brief Create an empty object 
 * Will not guarantee valid JSON structure - use only at right place
 * - if a node exists with this name, it will be cleared()
 */
CJsonNode* CJsonNode::createObject(const char *pszName) {
    CJsonNode *pNode = createElement(pszName);
    pNode->m_nObjectType = ELEMENT_TYPE::OBJECT;
    return(pNode);
}

/**
 * @brief Create an empty array 
 * Will not guarantee valid JSON structure - use only at right place
 * - if a node exists with this name, it will be cleared()
 */
CJsonNode* CJsonNode::createArray(const char *pszName) {
    CJsonNode *pNode = createElement(pszName);
    pNode->m_nObjectType = ELEMENT_TYPE::ARRAY;
    return(pNode);
}

/**
 * @brief get an element as array.
 * returns the named element if it is an array.
 * if bCreateIfNotExist is set to true:
 * - the node will be created if it does not exist
 * - the node will be converted if it is of wrong type (existing data will be deleted)
 * if bCreateIfNotExist is set to false:
 * - if the node exists and is of type array, the result is the node
 * - if the node exists and is of wrong type, the result is nullptr
 * - if the node does not exists, the result is nullptr
 * @param pszName Name of the element requested
 * @param bCreateIfNotExist If true, the element will be created or replaced
 * @returns the Node or nullptr if no node is in place
 */
CJsonNode* CJsonNode::getArray(const char* pszName, bool bCreateIfNotExist) {
    return(getTypedNode(pszName, ELEMENT_TYPE::ARRAY, bCreateIfNotExist));
}

/**
 * @brief get an element as array.
 * returns the named element if it is an array.
 * if bCreateIfNotExist is set to true:
 * - the node will be created if it does not exist
 * - the node will be converted if it is of wrong type (existing data will be deleted)
 * if bCreateIfNotExist is set to false:
 * - if the node exists and is of type array, the result is the node
 * - if the node exists and is of wrong type, the result is nullptr
 * - if the node does not exists, the result is nullptr
 * @param pszName Name of the element requested
 * @param bCreateIfNotExist If true, the element will be created or replaced
 * @returns the Node or nullptr if no node is in place
 */
CJsonNode* CJsonNode::getElement(const char* pszName, bool bCreateIfNotExist) {
    return(getTypedNode(pszName, ELEMENT_TYPE::VALUE, bCreateIfNotExist));
}
#pragma endregion

#pragma information about this node 

/**
 * @brief Print this node tree to Serial for diagnostics.
 * @param pszPrefixName Optional dotted prefix prepended to child names.
 */
void CJsonNode::dump(const char* pszPrefixName) {
    String strPrefix = pszPrefixName ? pszPrefixName : "";
    if (strPrefix.length() > 0) strPrefix += ".";

    for (CJsonNode* pEntry : Elements) {
        switch (pEntry->m_nObjectType) {
        case ELEMENT_TYPE::VALUE:
            if (pEntry->Name.length() > 0) SerialPrintf("%s%s == %s\n", strPrefix.c_str(), pEntry->Name.c_str(), pEntry->m_strValue.c_str());
            else  SerialPrintf("%s\n", pEntry->m_strValue.c_str());
            break;
        case ELEMENT_TYPE::OBJECT:
            pEntry->dump((strPrefix + pEntry->Name).c_str());
            break;
        case ELEMENT_TYPE::ARRAY:
            SerialPrintf("%s%s == [\n", strPrefix.c_str(), pEntry->Name.c_str());
            pEntry->dump();
            SerialPrintf("]\n");
        }

    }
}


/**
 * @brief Return true if this node contains a recognized boolean literal.
 */
bool CJsonNode::isBooleanValue() {
    bool bIsBoolean = LSC::isTrueValue(m_strValue.c_str()) || LSC::isFalseValue(m_strValue.c_str());
    return(bIsBoolean);
}

/**
 * @brief Return true if a named child contains a recognized boolean literal.
 */
bool CJsonNode::isBooleanValue(const char *pszName) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isBooleanValue() : false);
}
/**
 * @brief Return true if a named child contains a numeric literal.
 */
bool CJsonNode::isNumberValue(const char *pszName) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isNumberValue() : false);
}
/**
 * @brief Return true if this node contains a simple integer/float literal.
 */
bool CJsonNode::isNumberValue() {
    bool bResult = false;
    unsigned int nDotCounter = 0;
    const char* pszString = m_strValue.c_str();
    if (pszString && *pszString) {
        // Allow optional '+' or '-' at the start (but still false if only char.
        if (*pszString == '+' || *pszString == '-') pszString++;
        while (*pszString) {
            if ((*pszString >= '0' && *pszString <= '9') || *pszString == '.') {
                if (*pszString == '.') nDotCounter++;
                bResult = true;
            }
            else { bResult = false; break; }
            pszString++;
        }

    }
    return (bResult && nDotCounter < 2); // Must have at least one digit and only 0 or 1 dot inside.
}
#pragma endregion

#pragma region node serialization


/**
 * @brief Serialize this node tree as compact JSON.
 *
 * The returned pointer belongs to the internal serialization cache and remains
 * valid until the next serialization call on this node.
 */
const char* CJsonNode::getAsJsonText() {
    String strData;
    strData.reserve(1024);
    m_strSerializationCache = serializeNode(strData,-1);
    return(m_strSerializationCache.c_str());
}

/**
 * @brief Serialize this node tree as pretty-printed JSON.
 *
 * The returned pointer belongs to the internal serialization cache and remains
 * valid until the next serialization call on this node.
 */
const char* CJsonNode::getAsJsonTextPretty() {
    String strData;
    strData.reserve(1024 + 256);
    m_strSerializationCache = serializeNode(strData,0);
    return(m_strSerializationCache.c_str());
}

/**
 * @brief Append indentation spaces used by pretty JSON serialization.
 */
void CJsonNode::writeIdentPrefixString(String & strResultString, int nIdentDeep) {
    while(nIdentDeep-- > 0) strResultString += "    ";
}

/**
 * @brief Recursively serialize this node into the provided string buffer.
 * @param strResultString Output buffer receiving JSON text.
 * @param nIdentDeep -1 for compact mode, otherwise current indentation level.
 * @return Pointer to the output buffer's C string.
 */
const char* CJsonNode::serializeNode(String & strResultString, int nIdentDeep) {
    DEBUG_FUNC_START();
    const char *pszKeyValDeli   = nIdentDeep > -1 ? ": " : ":";
    const char *pszPost         = nIdentDeep > -1 ? "\n"  : "";
    if(nIdentDeep > -1) nIdentDeep++;
    switch(this->m_nObjectType) {
        case ELEMENT_TYPE::VALUE:
            {
                if (m_bWriteValueWithQuotes) jsonQuoted(strResultString, m_strValue);
                else if (m_strValue.length() == 0) strResultString += "null";
                else strResultString += m_strValue;
            }
            break;

        case ELEMENT_TYPE::OBJECT:
        case ELEMENT_TYPE::ARRAY:
            {
                bool bFirstElement  = true;
                // strResultString += strPrefix.c_str();
                strResultString += m_nObjectType == ELEMENT_TYPE::OBJECT ? "{" : "[";
                strResultString += pszPost; // New Line if necessary...
                for(CJsonNode *pChildNode : this->Elements) {
                    if (!bFirstElement) {
                        strResultString += ",";
                        strResultString += pszPost; // New Line if pretty mode
                    }
                    // If a name is in place, write the name an the key value delimiter...
                    if (m_nObjectType == ELEMENT_TYPE::OBJECT) {
                        writeIdentPrefixString(strResultString,nIdentDeep);
                        jsonQuoted(strResultString, pChildNode->Name);
                        strResultString += pszKeyValDeli;
                    }
                    pChildNode->serializeNode(strResultString,nIdentDeep);
                    bFirstElement  = false;
                }
                strResultString += pszPost; // New Line (if pretty mode)
                writeIdentPrefixString(strResultString, nIdentDeep -1);
                strResultString += m_nObjectType == ELEMENT_TYPE::OBJECT ? "}" : "]";
            }
            break;
    
        default:
            {
                DEBUG_INFO("JSON:: ####### unexpected data found #######");
                DEBUG_INFOS("  --> Node Type  : %d",m_nObjectType);
                DEBUG_INFOS("  --> Node Value : %s",m_strValue.c_str());
                DEBUG_INFOS("  --> Current result:\n%s",strResultString.c_str());
            }
            break;
    
    }
    DEBUG_FUNC_END();
    return(strResultString.c_str());
}


#pragma endregion

#pragma region parsing the input

/// @brief Parse a scalar JSON token from the input string.
/// @param pszJsonData Input string positioned at the value start.
/// @param strValueData Parsed value text is written here.
/// @param bHasQuotes Set to true when the token was enclosed in quotes.
/// @return Pointer to the delimiter that stopped parsing (for example ',', '}', ']').
namespace {
const char* jsonWhite(const char* p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    return p;
}
bool jsonHex4(const char*& p, unsigned& value) {
    value = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned char c = *p;
        unsigned digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else return false;
        ++p;
        value = value * 16 + digit;
    }
    return true;
}
/// @brief Append a single (Latin-1) byte as UTF-8 sequence.
void jsonLatin1(String& out, unsigned char c) {
    if (c < 0x80) out += char(c);
    else { out += char(0xc0 | (c >> 6)); out += char(0x80 | (c & 63)); }
}

/// @brief Parse a quoted JSON string.
/// @param lenient true: legacy mode - raw control characters, invalid UTF-8 (read as Latin-1)
///        and unknown escape sequences are accepted (as written by older versions).
bool jsonString(const char*& p, String& out, bool lenient = false) {
    if (*p != '"') return false;
    ++p;
    while (*p && *p != '"') {
        unsigned char c = *p++;
        if (c < 0x20) {
            if (!lenient) return false;
            out += char(c);
            continue;
        }
        if (c != '\\') {
            if (c < 0x80) { out += char(c); continue; }
            unsigned code;
            unsigned count;
            unsigned minimum;
            bool valid = true;
            if (c >= 0xc2 && c <= 0xdf) { code = c & 31; count = 1; minimum = 0x80; }
            else if (c >= 0xe0 && c <= 0xef) { code = c & 15; count = 2; minimum = 0x800; }
            else if (c >= 0xf0 && c <= 0xf4) { code = c & 7; count = 3; minimum = 0x10000; }
            else { valid = false; count = 0; code = 0; minimum = 0; }
            const char *pSeq = p;           // first continuation byte
            for (unsigned i = 0; valid && i < count; ++i) {
                unsigned char next = *p;
                if ((next & 0xc0) != 0x80) { valid = false; break; }
                ++p;
                code = (code << 6) | (next & 63);
            }
            if (valid && (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))) valid = false;
            if (valid) {
                out += char(c);
                for (const char *q = pSeq; q < p; ++q) out += *q;
            } else {
                if (!lenient) return false;
                p = pSeq;                   // byte is not UTF-8 - take it as Latin-1
                jsonLatin1(out, c);
            }
            continue;
        }
        char escape = *p;
        if (!escape) return false;
        ++p;
        switch (escape) {
        case '"': case '\\': case '/': out += escape; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
            unsigned code;
            if (!jsonHex4(p, code)) return false;
            if (code >= 0xd800 && code <= 0xdbff) {
                if (*p != '\\') return false;
                ++p;
                if (*p != 'u') return false;
                ++p;
                unsigned low;
                if (!jsonHex4(p, low) || low < 0xdc00 || low > 0xdfff) return false;
                code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
            } else if (code >= 0xdc00 && code <= 0xdfff) return false;
            // Values are exposed as C strings; embedded NUL cannot be represented safely.
            if (!code) return false;
            if (code < 0x80) out += char(code);
            else if (code < 0x800) {
                out += char(0xc0 | (code >> 6)); out += char(0x80 | (code & 63));
            } else if (code < 0x10000) {
                out += char(0xe0 | (code >> 12)); out += char(0x80 | ((code >> 6) & 63)); out += char(0x80 | (code & 63));
            } else {
                out += char(0xf0 | (code >> 18)); out += char(0x80 | ((code >> 12) & 63));
                out += char(0x80 | ((code >> 6) & 63)); out += char(0x80 | (code & 63));
            }
            break;
        }
        default:
            if (!lenient) return false;
            out += escape;          // legacy: unknown escape => the character itself
            break;
        }
    }
    if (*p != '"') return false;
    ++p;
    return true;
}
void jsonQuoted(String& out, const String& value) {
    static const char hex[] = "0123456789abcdef";
    out += '"';
    for (size_t i = 0; i < value.length(); ++i) {
        unsigned char c = value[i];
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c < 0x20) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
        else out += char(c);
    }
    out += '"';
}
}

namespace {
/// @brief Legacy delimiter of an unquoted token.
bool jsonIsBareEnd(char c) {
    return c == 0 || c == ',' || c == '}' || c == ']' || c == ':' || c == ' ' || c == '\t' || c == '\r' || c == '\n';
}
}

const char* CJsonNode::parseValue(const char* p, String& out, bool& quoted, bool lenient) {
    out.clear();
    quoted = false;
    if (!p) return nullptr;
    p = jsonWhite(p);
    if (*p == '"') {
        quoted = true;
        return jsonString(p, out, lenient) ? p : nullptr;
    }
    // Legacy: an empty value ("key": ,) - stored as null
    if (lenient && (*p == ',' || *p == '}' || *p == ']')) return p;

    const char* start = p;
    bool valid = true;
    if (*p == '-' || (*p >= '0' && *p <= '9')) {
        if (*p == '-') ++p;
        if (*p == '0') ++p;
        else {
            if (*p < '1' || *p > '9') valid = false;
            while (*p >= '0' && *p <= '9') ++p;
        }
        if (valid && *p == '.') {
            ++p;
            if (*p < '0' || *p > '9') valid = false;
            while (*p >= '0' && *p <= '9') ++p;
        }
        if (valid && (*p == 'e' || *p == 'E')) {
            ++p;
            if (*p == '+' || *p == '-') ++p;
            if (*p < '0' || *p > '9') valid = false;
            while (*p >= '0' && *p <= '9') ++p;
        }
    } else {
        const char* literal = *p == 't' ? "true" : *p == 'f' ? "false" : *p == 'n' ? "null" : nullptr;
        if (!literal) valid = false;
        while (valid && *literal) { if (*p != *literal) valid = false; else { ++p; ++literal; } }
    }
    // Legacy: the token must end here - otherwise it is an unquoted text (i.e. 1.2.3, +5, on)
    if (valid && lenient && !jsonIsBareEnd(*p)) valid = false;
    if (!valid) {
        if (!lenient) return nullptr;
        // Legacy: take the unquoted text up to the next delimiter as string value
        p = start;
        while (!jsonIsBareEnd(*p)) ++p;
        if (p == start) return nullptr;
        quoted = true;
    }
    while (start != p) out += *start++;
    return p;
}

bool CJsonNode::parseNode(const char*& p, unsigned depth, bool lenient) {
    p = jsonWhite(p);
    if (*p != '{' && *p != '[') {
        m_nObjectType = VALUE;
        const char* end = parseValue(p, m_strValue, m_bWriteValueWithQuotes, lenient);
        if (!end) return false;
        p = end;
        return true;
    }
    if (depth >= 32) return false;
    bool object = *p++ == '{';
    m_nObjectType = object ? OBJECT : ARRAY;
    char close = object ? '}' : ']';
    p = jsonWhite(p);
    // Legacy: leading / repeated commas ([,1,,2]) are skipped
    while (lenient && *p == ',') p = jsonWhite(p + 1);
    if (*p == close) { ++p; return true; }
    for (;;) {
        String key;
        if (object) {
            if (*p == '"') {
                if (!jsonString(p, key, lenient)) return false;
            } else {
                // Legacy: unquoted key name
                if (!lenient) return false;
                const char* start = p;
                while (!jsonIsBareEnd(*p)) ++p;
                if (p == start) return false;
                while (start != p) key += *start++;
            }
            p = jsonWhite(p);
            if (*p != ':') return false;
            ++p;
            // Ambiguous duplicate keys are rejected; dots remain literal key characters.
            // Legacy: older versions could write a key twice - the last one wins.
            if (find(key.c_str(), false)) {
                if (!lenient) return false;
                remove(key.c_str());
            }
        }
        CJsonNode* child = new CJsonNode(key.c_str());
        child->setParentNode(this);
        Elements.push_back(child);
        if (!child->parseNode(p, depth + 1, lenient)) return false;
        p = jsonWhite(p);
        if (*p == close) { ++p; return true; }
        if (*p != ',') return false;
        p = jsonWhite(p + 1);
        if (lenient) {
            // Legacy: trailing / repeated commas ({"a":1,} or [1,,2])
            while (*p == ',') p = jsonWhite(p + 1);
            if (*p == close) { ++p; return true; }
        }
    }
}

size_t CJsonNode::s_nLastParseErrorOffset = 0;

const char* CJsonNode::parse(const char* input, bool lenient) {
    s_nLastParseErrorOffset = 0;
    if (!input) return nullptr;
    const char* origin = input;
    // Legacy: UTF-8 byte order mark (i.e. file edited with a windows editor)
    if (lenient && (unsigned char) input[0] == 0xef && (unsigned char) input[1] == 0xbb && (unsigned char) input[2] == 0xbf) input += 3;
    CJsonNode parsed;
    const char* end = input;
    if (!parsed.parseNode(end, 0, lenient)) { s_nLastParseErrorOffset = (size_t)(end - origin); return nullptr; }
    // Strict: nothing may follow the document. Legacy: content behind the document is ignored.
    if (!lenient && *jsonWhite(end)) { s_nLastParseErrorOffset = (size_t)(jsonWhite(end) - origin); return nullptr; }
    clear();
    Elements.swap(parsed.Elements);
    m_strValue = parsed.m_strValue;
    m_nObjectType = parsed.m_nObjectType;
    m_bWriteValueWithQuotes = parsed.m_bWriteValueWithQuotes;
    m_strSerializationCache.clear();
    for (CJsonNode* child : Elements) child->setParentNode(this);
    // Preserve the historical closing-delimiter return for container roots.
    return m_nObjectType == VALUE ? end : end - 1;
}
