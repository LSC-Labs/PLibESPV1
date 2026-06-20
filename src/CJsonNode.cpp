#include "JsonNode.h"
#include "LSCUtils.h"

CJsonNode::CJsonNode(const char* pszName, ELEMENT_TYPE eType) {
    Name = pszName;
    this->m_nObjectType = eType;
};

CJsonNode::CJsonNode(const char* pszName, const char* pszValue) {
    this->m_strValue = pszValue;
    this->Name = pszName;
    this->m_nObjectType = ELEMENT_TYPE::VALUE;
}

void CJsonNode::clear() {
    for (CJsonNode* pEntry : Elements) {
        delete(pEntry);
    }
}

CJsonNode* CJsonNode::find(const char* pszName) {
    CJsonNode* pResult = nullptr;
    for (CJsonNode* pEntry : Elements) {
        if (pEntry->Name == pszName) {
            pResult = pEntry;
            break;
        }
    }
    return(pResult);
}

/**
 * @brief checks if the key name exists in this node.
 * It searches for elements and objects..
 */
bool CJsonNode::exists(const char* pszName) {
    bool bFound = find(pszName) != nullptr;
    return(bFound);
}

CJsonNode::ELEMENT_TYPE CJsonNode::getType() {
    return(m_nObjectType);
}

bool CJsonNode::isJsonObject() { return(m_nObjectType == ELEMENT_TYPE::OBJECT); }
bool CJsonNode::isJsonArray() { return(m_nObjectType == ELEMENT_TYPE::ARRAY); }
bool CJsonNode::isJsonValue() { return(m_nObjectType == ELEMENT_TYPE::VALUE); }
bool CJsonNode::isJsonObject(const char *pszName) { 
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isJsonObject(): false); 
}
bool CJsonNode::isJsonArray(const char *pszName) { 
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isJsonArray(): false); 
}
bool CJsonNode::isJsonValue(const char *pszName) { 
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isJsonValue(): false);
}



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


CJsonNode* CJsonNode::setValue(const char* pszValue) {
    this->m_strValue = pszValue;
    this->m_bWriteValueWithQuotes = true;
    return(this);
}

CJsonNode* CJsonNode::setValue(bool bValue) {
    this->m_strValue = bValue ? "true" : "false";
    this->m_bWriteValueWithQuotes = false;
    return(this);
}

CJsonNode* CJsonNode::setValue(int nValue) {
    // to avoid to be interpreted as character by string class, print to buffer first...
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%d",nValue);
    this->m_strValue = szBuffer;
    this->m_bWriteValueWithQuotes = false;
    return(this);
}

CJsonNode* CJsonNode::setValue(long lValue) {
    this->m_strValue = lValue;
    this->m_bWriteValueWithQuotes = false;
    return(this);
}

CJsonNode* CJsonNode::setValue(float fValue) {
    char szBuffer[256];
    snprintf(szBuffer,sizeof(szBuffer),"%f",fValue);
    this->m_strValue = szBuffer;
    this->m_bWriteValueWithQuotes = false;
    return(this);
}

CJsonNode* CJsonNode::setValue(const char* pszName, const char * pszValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(pszValue));
}

CJsonNode* CJsonNode::setValue(const char* pszName, bool bValue) {
     CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(bValue));
}

CJsonNode* CJsonNode::setValue(const char* pszName, int nValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(nValue));
}

CJsonNode* CJsonNode::setValue(const char* pszName, float fValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(fValue));
}

CJsonNode* CJsonNode::setValue(const char* pszName, long lValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(lValue));
}

#pragma region get the value 

String & CJsonNode::getValueAsString() {
    return(m_strValue);
}

String & CJsonNode::getValueAsString(const char *pszName, String & strDefault) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->getValueAsString(): strDefault);
}


const char* CJsonNode::getValue() {
    return(m_strValue.c_str());
}

const char* CJsonNode::getValueOrDefault(const char *pszDefault) {
    const char *pszResult = m_strValue.c_str();
    return(pszResult ? pszResult : pszDefault);
}

const char* CJsonNode::getValueOrDefault(const char *pszName, const char *pszDefault) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->getValueOrDefault(pszDefault) : pszDefault );
}

const char* CJsonNode::getValue(const char* pszName) {
    const char* pszResult = nullptr;
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
    return(pszResult);
}

int CJsonNode::getValueAsInt(int nDefault) {
    int nResult = nDefault;
    if (isNumberValue()) nResult = atoi(m_strValue.c_str());
    return(nResult);
}

int CJsonNode::getValueAsInt(const char* pszName, int nDefault) {
    CJsonNode* pNode = find(pszName);
    int nResult = pNode ? pNode->getValueAsInt(nDefault) : nDefault;
    return(nResult);
}


unsigned int CJsonNode::getValueAsUnsignedInt(unsigned int nDefault) {
    return((unsigned int) getValueAsInt((int) nDefault));
}

unsigned int CJsonNode::getValueAsUnsignedInt(const char* pszName, unsigned int nDefault) {
    return((unsigned int) getValueAsInt(pszName,(int) nDefault));
}

long CJsonNode::getValueAsLong(long lDefault) {
    long nResult = lDefault;
    if (isNumberValue()) nResult = atol(m_strValue.c_str());
    return(nResult);
}

long CJsonNode::getValueAsLong(const char* pszName, long lDefault) {
    CJsonNode* pNode = find(pszName);
    long nResult = pNode ? pNode->getValueAsLong(lDefault) : lDefault;
    return(nResult);
}



float CJsonNode::getValueAsFloat(float fDefault) {
    double fResult = fDefault;
    if (isNumberValue()) fResult = atof(m_strValue.c_str());
    return(fResult);
}

float CJsonNode::getValueAsFloat(const char* pszName, float fDefault) {
    CJsonNode* pNode = find(pszName);
    float fResult = pNode ? pNode->getValueAsFloat(fDefault) : fDefault;
    return(fResult);
}


bool CJsonNode::getValueAsBool(bool bDefault) {
    bool bResult = bDefault;
    if (isBooleanValue()) bResult = LSC::isTrueValue(m_strValue.c_str()); 
    return(bResult);
}

bool CJsonNode::getValueAsBool(const char* pszName, bool bDefault) {
    CJsonNode* pNode = find(pszName);
    bool bResult = pNode ? pNode->getValueAsBool(bDefault) : bDefault;
    return(bResult);
}

#pragma endregion


#pragma region Store if functions

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
    CJsonNode* pNode = find(pszName);
    if (pNode == nullptr && bCreateIfNotExist) {
        pNode = new CJsonNode(pszName, ELEMENT_TYPE::OBJECT);
        this->Elements.push_back(pNode);
    }
    // Clear and convert to object if wrong type when CreateIfNotExist is set to true
    if(pNode && pNode->m_nObjectType != ELEMENT_TYPE::OBJECT && bCreateIfNotExist) {
        pNode->clear();
        pNode->m_nObjectType = ELEMENT_TYPE::OBJECT;
    }
    return((pNode && pNode->getType() == ELEMENT_TYPE::OBJECT) ? pNode : nullptr);
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
    CJsonNode* pNode = find(pszName);
    if (pNode == nullptr && bCreateIfNotExist) {
        pNode = new CJsonNode(pszName, ELEMENT_TYPE::ARRAY);
        this->Elements.push_back(pNode);
    }
    // Clear and convert to object if wrong type when CreateIfNotExist is set to true
    if(pNode && pNode->m_nObjectType != ELEMENT_TYPE::ARRAY && bCreateIfNotExist) {
        pNode->clear();
        pNode->m_nObjectType = ELEMENT_TYPE::ARRAY;
    }
    return(pNode->getType() == ELEMENT_TYPE::ARRAY ? pNode : nullptr);
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
    CJsonNode* pNode = find(pszName);
    if (pNode == nullptr && bCreateIfNotExist) {
        pNode = new CJsonNode(pszName, ELEMENT_TYPE::VALUE);
        this->Elements.push_back(pNode);
    }
    // Clear and convert to object if wrong type when CreateIfNotExist is set to true
    if(pNode && pNode->m_nObjectType != ELEMENT_TYPE::VALUE && bCreateIfNotExist) {
        pNode->clear();
        pNode->m_nObjectType = ELEMENT_TYPE::VALUE;
    }
    return(pNode->getType() == ELEMENT_TYPE::VALUE ? pNode : nullptr);
}

void CJsonNode::dump(const char* pszPrefixName) {
    String strPrefix = pszPrefixName;
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

bool CJsonNode::isBooleanValue() {
    bool bIsBoolean = LSC::isTrueValue(m_strValue.c_str()) || LSC::isFalseValue(m_strValue.c_str());
    return(bIsBoolean);
}

bool CJsonNode::isBooleanValue(const char *pszName) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isBooleanValue() : false);
}
bool CJsonNode::isNumberValue(const char *pszName) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->isNumberValue() : false);
}
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

const char* CJsonNode::getAsJsonText() {
    bool bIsFirstElement = false;
    m_strDataCache.clear();
    if (Name.length() > 0) {
        if (Name.length() > 0) m_strDataCache += "\"" + Name + "\":";
    }
    switch (this->m_nObjectType) {
    case ELEMENT_TYPE::VALUE:
        // if (isBooleanValue() || isNumberValue()) m_strDataCache += m_strValue;
        if (!m_bWriteValueWithQuotes) m_strDataCache += m_strValue;
        else m_strDataCache += "\"" + m_strValue + "\"";
        break;
    case ELEMENT_TYPE::OBJECT:
        bIsFirstElement = true;
        m_strDataCache += "{";
        for (CJsonNode* pNode : this->Elements) {
            if (!bIsFirstElement) m_strDataCache += ",";
            m_strDataCache += pNode->getAsJsonText();
            bIsFirstElement = false;
        }
        m_strDataCache += "}";
        break;

    case ELEMENT_TYPE::ARRAY:
        bIsFirstElement = true;
        m_strDataCache += "[";
        for (CJsonNode* pNode : this->Elements) {
            if (!bIsFirstElement) m_strDataCache += ",";
            if (pNode->getType() == ELEMENT_TYPE::VALUE) m_strDataCache += pNode->getAsJsonText();
            else m_strDataCache += pNode->getAsJsonText();
            bIsFirstElement = false;
        }
        m_strDataCache += "]";
    }
    return(m_strDataCache.c_str());
}

inline bool CJsonNode::isWhite(const char c) {
    return(
        c == ' ' ||  // space
        c == '\t' ||  // horizontal tab
        c == '\n' ||  // newline
        c == '\v' ||  // vertical tab
        c == '\f' ||  // form feed
        c == '\r'   // carriage return
        );
}
/// <summary>
/// skipWhite skips the white characters.
/// As ther could be also special chars like "Umlaute" in german,
/// isspace() cannot handle this chars, so check step by step...
/// </summary>
/// <param name="psz"></param>
/// <returns></returns>
inline const char* CJsonNode::skipWhite(const char* psz) {
    while (psz && isWhite(*psz)) psz++;
    return(psz);
}

/// <summary>
/// parse a value from input string
/// </summary>
/// <param name="pszJsonData">The input string to be parsed</param>
/// <param name="strValueData">Found value will be appended here</param>
/// <returns>Pointer to the last character the causes the stop of parsing (",{}[]...")</returns>
const char* CJsonNode::parseValue(const char* pszJsonData, String& strValueData, bool& bHasQuotes) {
    bool bEscapeIsActive = false;
    bool bStringIsActive = false;
    bool bStopParsing = false;
    bHasQuotes = false;
    pszJsonData = skipWhite(pszJsonData);
    while (pszJsonData && *pszJsonData) {
        switch (*pszJsonData) {
        case '"':
            if (bEscapeIsActive) { strValueData += '"'; bEscapeIsActive = false; }
            else { bStringIsActive = !bStringIsActive; };
            if (bStringIsActive) bHasQuotes = true;
            break;

        case '\\':
            if (!bEscapeIsActive) bEscapeIsActive = true;
            else { strValueData += '\\'; bEscapeIsActive = false; }
            break;

            // Delimiters detected... terminate or into the string
        case '[': // start of an array
        case ']': // end of an array
        case '{': // start of a new object
        case '}': // end of object
        case ':': // key value assignment
        case ',': // or a close element delimiter...
            if (bStringIsActive) strValueData += *pszJsonData;
            else bStopParsing = true;
            break;

        default:
            // If a quoted string is active, insert data as is.
            // If it is non quoted, ignore all non white spaces
            // TODO: Optimize, cause only numbers and bools are outside a string.
            if (isWhite(*pszJsonData)) {
                if (bStringIsActive)    strValueData += *pszJsonData;
            }
            else                      strValueData += *pszJsonData;
            break;

        }
        if (bStopParsing) break;
        pszJsonData++;
    }
    return(pszJsonData);
}
/*
const char* CJsonNode::parseArray(const char* pszJsonData) {
    String strData;
    CJsonNode* pActiveNode;
    pszJsonData = skipWhite(pszJsonData);
    if (*pszJsonData == '[') {
        this->m_nObjectType = ELEMENT_TYPE::ARRAY;
        bool bParsing = true;
        pszJsonData++;
        while (*pszJsonData) {
            pszJsonData = skipWhite(pszJsonData);
            switch (*pszJsonData) {
            case '{': // Sub Array detected ? or inside a string
                pActiveNode = new CJsonNode(strData.c_str());
                pszJsonData = pActiveNode->parseObject(pszJsonData);
                m_tSubNodes.push_back(pActiveNode);
                break;

            case '[': // Sub Array detected ? or inside a string
                pActiveNode = new CJsonNode(strData.c_str());
                pszJsonData = pActiveNode->parseArray(pszJsonData);
                m_tSubNodes.push_back(pActiveNode);
                break;

                // Following elements are not expected here 
                // seems as json data is malformed...
            case '}':
            case ']':
            case ',':
            case ':':
                bParsing = false;
                break;

            default:
                // parse the data...
                // If the result is a ':' it is an assignment, a key value entry.. store it.
                bool bValueIsQuoted = true;
                pszJsonData = parseValue(pszJsonData, strData, bValueIsQuoted);
                if (*pszJsonData == ':') bParsing = false;
                else {
                    pActiveNode = new CJsonNode("", strData.c_str());
                    pActiveNode->m_bWriteValueWithQuotes = bValueIsQuoted;
                    m_tSubNodes.push_back(pActiveNode);
                    strData.clear();
                }
                break;
            }
            if (!bParsing) break;
            pszJsonData++;
        }
    }
    return(pszJsonData);
}

const char* CJsonNode::parseObject(const char* pszJsonData) {
    String strData;
    String strKeyName;
    CJsonNode* pActiveNode;
    pszJsonData = skipWhite(pszJsonData);
    if (*pszJsonData == '{') {
        this->m_nObjectType = ELEMENT_TYPE::OBJECT;
        bool bParsing = true;
        pszJsonData++;
        while (*pszJsonData) {
            pszJsonData = skipWhite(pszJsonData);
            switch (*pszJsonData) {
            case '{': // Sub Object detected ?
                pActiveNode = new CJsonNode(strKeyName.c_str());
                pszJsonData = pActiveNode->parseObject(pszJsonData);
                m_tSubNodes.push_back(pActiveNode);
                break;

            case '[': // Sub Array detected ? or inside a string
                pActiveNode = new CJsonNode(strKeyName.c_str());
                pszJsonData = pActiveNode->parseArray(pszJsonData);
                m_tSubNodes.push_back(pActiveNode);
                break;

                // Following elements are not expected here 
                // seems as json data is malformed...
            case '}':
            case ']':
            case ',':
                bParsing = false;
                break;

            default:
                // parse the data...
                // If the result is a ':' it is an assignment, a key value entry.. store it.
                bool bValueIsQuoted;
                pszJsonData = parseValue(pszJsonData, strData, bValueIsQuoted);
                if (*pszJsonData == ':') { strKeyName = strData; }
                else {
                    pActiveNode = new CJsonNode(strKeyName.c_str(), strData.c_str());
                    pActiveNode->m_bWriteValueWithQuotes = bValueIsQuoted;
                    m_tSubNodes.push_back(pActiveNode);
                    strKeyName.clear();
                }
                strData.clear();
                break;
            }
            if (!bParsing) break;
            pszJsonData++;
        }
    }
    return(pszJsonData);
}
*/

/**
 * @brief Parsing a Json text string
 * - either a native assignment of a var like key:value
 * - a json object, starting with '{'
 * - or an array, starting with '['
 */
const char* CJsonNode::parse(const char* pszJsonData) {
    if (pszJsonData) {
        String strData;         // KeyValue - Data buffer
        String strKeyName;      // KeyValue - KeyName buffer
        CJsonNode* pActiveNode; // Used to create child elements
        pszJsonData = skipWhite(pszJsonData);
        switch (*pszJsonData) {
        case '{': pszJsonData++; m_nObjectType = ELEMENT_TYPE::OBJECT; break;
        case '[': pszJsonData++; m_nObjectType = ELEMENT_TYPE::ARRAY;  break;
        }
        bool bParsing = true;
        while (*pszJsonData) {
            pszJsonData = skipWhite(pszJsonData);
            switch (*pszJsonData) {
            case '{': // Sub Object detected ?
            case '[': // Sub Array detected ? or inside a string
                pActiveNode = new CJsonNode(strKeyName.c_str());
                pszJsonData = pActiveNode->parse(pszJsonData);
                Elements.push_back(pActiveNode);
                break;

                // Following elements are not expected here or are terminating the object / value. 
            case '}':
            case ']':
            case ',':
                bParsing = false;
                break;

                // Default is parsing a value (array) or a key:value (object)
            default:
                bool bValueIsQuoted;
                pszJsonData = parseValue(pszJsonData, strData, bValueIsQuoted);
                if (*pszJsonData == ':') {
                    // If it is an array, the token ':' may not come in place => stop parsing
                    // If the result is a ':' it is an assignment, it's a key value entry.. store it,
                    // otherwise, this was the name...
                    if (m_nObjectType == ELEMENT_TYPE::ARRAY) bParsing = false;
                    else strKeyName = strData;
                }
                else {
                    pActiveNode = new CJsonNode(m_nObjectType == ELEMENT_TYPE::ARRAY ? "" : strKeyName.c_str(), strData.c_str());
                    pActiveNode->m_bWriteValueWithQuotes = bValueIsQuoted;
                    Elements.push_back(pActiveNode);
                    strKeyName.clear();
                }
                strData.clear();
                break;
            }
            if (!bParsing) break;
            pszJsonData++;
        }
    }
    return(pszJsonData);
}
