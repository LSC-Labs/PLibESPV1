#ifndef DEBUG_LSC_JSON
    #undef DEBUGINFOS
#endif
#include "JsonNode.h"
#include "LSCUtils.h"
#include "DevelopmentHelper.h"

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
    Elements.clear();
}

JsonNode * CJsonNode::getParentNode() {
    return(m_pParentNode);
}

void CJsonNode::setParentNode(JsonNode * pParentNode) {
    m_pParentNode = pParentNode;
}

/**
 * @brief Find a CJsonNode with the name
 * The name is either a node name (unlimited size) or
 * a path to the node name like "data.const.size".
 * The path elements are limited to 256 bytes in length per element.
 * In the sample "const" length may not exceed 256 bytes
 * @retunrs the node or nullptr if not found.
 */
CJsonNode* CJsonNode::find(const char* pszName) {
    CJsonNode* pResult = nullptr;
    if(pszName) {
        int nDeliIdx = LSC::indexOf(pszName,'.');
        // If a '.' is in the name, scan for the first name and handover to the next
        if(nDeliIdx > -1) {
            char szBuffer[256];
            strncpy(szBuffer,pszName,nDeliIdx);
            szBuffer[nDeliIdx] = '\0';
            CJsonNode *pSubNode = find(szBuffer);
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

CJsonNode* CJsonNode::createJsonPathToElement(const char * pszName) {
    CJsonNode *pResult = this;
    int nDeliIdx = LSC::indexOf(pszName,'.');
    if(nDeliIdx > -1) {
        char szBuffer[256];
        strncpy(szBuffer,pszName,nDeliIdx);
        szBuffer[nDeliIdx] = '\0';
        CJsonNode *pSubNode = getObject(szBuffer,true);
        pResult = pSubNode->createJsonPathToElement(&pszName[nDeliIdx + 1]);
    }
    return(pResult);
}

bool CJsonNode::getNameFromJsonPath(const char * pszName, String & strName) {
    strName = pszName;
    int nLastIdx = LSC::lastIndexOf(pszName,'.');
    if(nLastIdx > -1) strName = &pszName[nLastIdx + 1];
    return(nLastIdx > -1);
}


CJsonNode & CJsonNode::operator[](const char *pszName) {
    return(*getElement(pszName,true));
}

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

#pragma region set the value

void CJsonNode::setNodeValueType(bool bWriteWithQuotes) { 
    clear();
    m_nObjectType = ELEMENT_TYPE::VALUE; 
    m_bWriteValueWithQuotes = bWriteWithQuotes; 
}

CJsonNode* CJsonNode::setValue(const char* pszValue) {
    this->m_strValue = (char *) pszValue;
    setNodeValueType(true);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

CJsonNode* CJsonNode::setValue(String & strValue) {
    this->m_strValue = (char *) strValue.c_str();
    setNodeValueType(true);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}


CJsonNode* CJsonNode::setValue(bool bValue) {
    this->m_strValue = bValue ? "true" : "false";
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

CJsonNode* CJsonNode::setValue(int nValue) {
    // to avoid to be interpreted as character by string class, print to buffer first...
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%d",nValue);
    this->m_strValue = szBuffer;
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

CJsonNode* CJsonNode::setValue(unsigned int unValue) {
    // to avoid to be interpreted as character by string class, print to buffer first...
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%u",unValue);
    this->m_strValue = szBuffer;
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

CJsonNode* CJsonNode::setValue(long lValue) {
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%ld",lValue);
    this->m_strValue = szBuffer;
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

CJsonNode* CJsonNode::setValue(unsigned long ulValue) {
    char szBuffer[80];
    snprintf(szBuffer,sizeof(szBuffer),"%lu",ulValue);
    this->m_strValue = szBuffer;
    setNodeValueType(false);
    DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    return(this);
}

/**
 * @brief set the value, but only if value is not NAN..
 */
CJsonNode* CJsonNode::setValue(float fValue) {
    if(fValue != NAN) {
        char szBuffer[256];
        snprintf(szBuffer,sizeof(szBuffer),"%f",fValue);
        this->m_strValue = szBuffer;
        setNodeValueType(false);
        DEBUG_INFOS("JSON: -> setting quoted:(%d): %s == %s",m_bWriteValueWithQuotes,Name.c_str(),m_strValue.c_str());
    }
    return(this);
}

CJsonNode* CJsonNode::setValue(const char* pszName, const char * pszValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(pszValue));
}

CJsonNode* CJsonNode::setValue(const char* pszName, String &  strValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(strValue));
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
    if(fValue != NAN) {
        pNode->setValue(fValue);
    }
    return(pNode);
}

CJsonNode* CJsonNode::setValue(const char* pszName, long lValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(lValue));
}

CJsonNode* CJsonNode::setValue(const char* pszName, unsigned long ulValue) {
    CJsonNode * pNode = getElement(pszName,true);
    return(pNode->setValue(ulValue));
}

#pragma endregion

#pragma region get the value 

String & CJsonNode::getValueAsString(String & strDefault) {
    return(m_strValue.c_str() == nullptr ? strDefault : m_strValue);
}

String & CJsonNode::getValueAsString(const char *pszName, String & strDefault) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->getValueAsString(strDefault): strDefault);
}


const char* CJsonNode::getValue() {
    return(m_strValue.c_str());
}

const char* CJsonNode::getValueAsCharPointer(const char *pszDefault) {
    const char *pszResult = m_strValue.c_str();
    return(pszResult ? pszResult : pszDefault);
}

const char* CJsonNode::getValueAsCharPointer(const char *pszName, const char *pszDefault) {
    CJsonNode *pNode = find(pszName);
    return(pNode ? pNode->getValueAsCharPointer(pszDefault) : pszDefault );
}

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

unsigned long CJsonNode::getValueAsUnsignedLong( unsigned long ulDefault) {
    unsigned long nResult = ulDefault;
    if (isNumberValue()) nResult = atol(m_strValue.c_str());
    return(nResult);
}
unsigned long CJsonNode::getValueAsUnsignedLong(const char* pszName, unsigned long ulDefault) {
    CJsonNode* pNode = find(pszName);
    unsigned long nResult = pNode ? pNode->getValueAsUnsignedLong(ulDefault) : ulDefault;
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


bool CJsonNode::storeValueIf(String & strTarget) {
    bool bResult = false;
    if(m_strValue.c_str() != nullptr) {
        strTarget = m_strValue.c_str();
        bResult = true;
    }
    return(bResult);
}

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

bool CJsonNode::storeValueIf(const char* pszName, String & strTarget) {
    bool bResult = false;
    CJsonNode* pNode = find(pszName);
    if (pNode) bResult = pNode->storeValueIf(strTarget);
    return(bResult);
}

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
    CJsonNode* pPath = bCreateIfNotExist ? createJsonPathToElement(pszName) : this;
    CJsonNode* pNode = pPath->find(pszName);
    if (pNode == nullptr && bCreateIfNotExist) {
        String strName;
        getNameFromJsonPath(pszName,strName);
        pNode = new CJsonNode(strName.c_str(), ELEMENT_TYPE::OBJECT);
        pNode->setParentNode(this);
        pPath->Elements.push_back(pNode);
    } 
    // Clear and convert to object if wrong type when CreateIfNotExist is set to true
    if(pNode && pNode->m_nObjectType != ELEMENT_TYPE::OBJECT && bCreateIfNotExist) {
        DEBUG_INFOS("JSON: -> converting %d > %d (to object)",pNode->m_nObjectType, ELEMENT_TYPE::OBJECT);
        pNode->clear();
        pNode->m_nObjectType = ELEMENT_TYPE::OBJECT;
    }
    return((pNode && pNode->getType() == ELEMENT_TYPE::OBJECT) ? pNode : nullptr);
}

/**
 * @brief Create an (empty) element
 * Will not guarantee valid JSON structure - use only at right place
 * - if a node exists with this name, it will be cleared()
 */
CJsonNode* CJsonNode::createElement(const char *pszName) {
    CJsonNode *pNode = nullptr;
    if(pszName) pNode = find(pszName);
    if(!pNode) {
        pNode = new CJsonNode();
        pNode->setParentNode(this);
    }
    else pNode->clear();
    pNode->m_nObjectType = ELEMENT_TYPE::VALUE;
    Elements.push_back(pNode);
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
    CJsonNode* pPath = bCreateIfNotExist ? createJsonPathToElement(pszName) : this;
    CJsonNode* pNode = pPath->find(pszName);
    if (pNode == nullptr && bCreateIfNotExist) {
        String strName;
        getNameFromJsonPath(pszName,strName);
        pNode = new CJsonNode(strName.c_str(), ELEMENT_TYPE::ARRAY);
        pNode->setParentNode(this);
        pPath->Elements.push_back(pNode);
    }
    // Clear and convert to object if wrong type when CreateIfNotExist is set to true
    if(pNode && pNode->m_nObjectType != ELEMENT_TYPE::ARRAY && bCreateIfNotExist) {
        pNode->clear();
        pNode->m_nObjectType = ELEMENT_TYPE::ARRAY;
    }
    return(pNode && pNode->getType() == ELEMENT_TYPE::ARRAY ? pNode : nullptr);
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
    DEBUG_FUNC_START_PARMS("%s,%d",pszName,bCreateIfNotExist);
    CJsonNode* pPath = bCreateIfNotExist ? createJsonPathToElement(pszName) : this;
    CJsonNode* pNode = pPath->find(pszName);
    if (pNode == nullptr && bCreateIfNotExist) {
        String strName;
        getNameFromJsonPath(pszName,strName);
        pNode = new CJsonNode(strName.c_str(), ELEMENT_TYPE::VALUE);
        pNode->setParentNode(this);
        pPath->Elements.push_back(pNode);
    }
    
    // Clear and convert to object if wrong type when CreateIfNotExist is set to true
    if(pNode && (pNode->m_nObjectType != ELEMENT_TYPE::VALUE) && bCreateIfNotExist) {
        pNode->clear();
        pNode->m_nObjectType = ELEMENT_TYPE::VALUE;
    }

    DEBUG_FUNC_END();
    return(pNode  && (pNode->getType() == ELEMENT_TYPE::VALUE) ? pNode : nullptr);
}
#pragma endregion

#pragma information about this node 

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
#pragma endregion

#pragma region node serialization


const char* CJsonNode::getAsJsonText() {
    String strData;
    strData.reserve(1024);
    m_strSerializationCache = serializeNode(strData,-1);
    return(m_strSerializationCache.c_str());
}

const char* CJsonNode::getAsJsonTextPretty() {
    String strData;
    strData.reserve(1024 + 256);
    m_strSerializationCache = serializeNode(strData,0);
    return(m_strSerializationCache.c_str());
}

void CJsonNode::writeIdentPrefixString(String & strResultString, int nIdentDeep) {
    while(nIdentDeep-- > 0) strResultString += "    ";
}
const char* CJsonNode::serializeNode(String & strResultString, int nIdentDeep) {
    DEBUG_FUNC_START();
    const char *pszKeyValDeli   = nIdentDeep > -1 ? ": " : ":";
    const char *pszPost         = nIdentDeep > -1 ? "\n"  : "";
    if(nIdentDeep > -1) nIdentDeep++;
    switch(this->m_nObjectType) {
        case ELEMENT_TYPE::VALUE:
            {
                // Element type value detected - write with or without quotes...
                if (!m_bWriteValueWithQuotes) {
                    strResultString += getValueAsCharPointer("");
                }
                else {
                    strResultString += "\"";
                    strResultString += getValueAsCharPointer("");
                    strResultString += "\"";
                }
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
                    if (pChildNode->Name.length() > 0) {
                        writeIdentPrefixString(strResultString,nIdentDeep);
                        strResultString += "\"";
                        strResultString += pChildNode->Name.c_str();
                        strResultString += "\"";
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
/*
const char* CJsonNode::serialize(String & strResultString, int nIdentDeep) {

    String strPrefix = "";
    const char *pszKeyValDeli   = nIdentDeep > -1 ? " : " : ":";
    const char *pszPost         = nIdentDeep > -1 ? "\n"  : "";

    // Output with indent requested
    if(nIdentDeep > -1) {
        for(int nDeep = 0; nDeep < nIdentDeep; nDeep++ ) {
            strPrefix += "   ";
        }
        nIdentDeep++;
    }
    
    // If a name is in place, write the name an the key value delimiter...
    if (Name.length() > 0) {
        strResultString += strPrefix.c_str();
        strResultString += "\"";
        strResultString += Name.c_str();
        strResultString += "\"";
        strResultString += pszKeyValDeli;
    }
    
    switch (this->m_nObjectType) {

        case ELEMENT_TYPE::VALUE:
            {
                // Element type value detected - write with or without quotes...
                if (!m_bWriteValueWithQuotes) {
                    strResultString += getValueAsCharPointer("");
                }
                else {
                    strResultString += "\"";
                    strResultString += getValueAsCharPointer("");
                    strResultString += "\"";
                }
            }
            break;
            
        case ELEMENT_TYPE::OBJECT:
        case ELEMENT_TYPE::ARRAY: 
            {
                bool bFirstElement  = true;
                strResultString += m_nObjectType == ELEMENT_TYPE::OBJECT ? "{" : "[";
                strResultString += pszPost; // New Line if necessary...
                for(CJsonNode *pChildNode : this->Elements) {
                    if (!bFirstElement) {
                        strResultString += ",";
                        strResultString += pszPost; // New Line if pretty mode
                    }
                    pChildNode->serialize(strResultString,nIdentDeep);
                    bFirstElement  = false;
                }
                strResultString += pszPost; // New Line (if pretty mode)
                strResultString += strPrefix.c_str();
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

    return(strResultString.c_str());
}
*/

#pragma endregion

#pragma region parsing the input

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
    strValueData.clear();
    bHasQuotes = false;
    pszJsonData = LSC::skipWhite(pszJsonData);
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
            if (LSC::isWhite(*pszJsonData)) {
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
        pszJsonData = LSC::skipWhite(pszJsonData);
        switch (*pszJsonData) {
        case '{': pszJsonData++; m_nObjectType = ELEMENT_TYPE::OBJECT; break;
        case '[': pszJsonData++; m_nObjectType = ELEMENT_TYPE::ARRAY;  break;
        }
        bool bParsing = true;
        while (*pszJsonData) {
            pszJsonData = LSC::skipWhite(pszJsonData);
            switch (*pszJsonData) {
            case '{': // Sub Object detected ?
            case '[': // Sub Array detected ? or inside a string
                pActiveNode = new CJsonNode(strKeyName.c_str());
                pActiveNode->setParentNode(this);
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
                strData = "";
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
                    pActiveNode->setParentNode(this);
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


#pragma #endregion