#include <SimpleJsonNode.h>


void CSimpleJsonNode::clear() {
    for (CElementEntry* pEntry : tKeyValues) {
        std::cout << "Removing : " << pEntry->Name << "\n";
        delete(pEntry);
    }
    for (CSimpleJsonNode* pNode : this->tSubNodes) {
        delete(pNode);
    }
}

void CSimpleJsonNode::dump() {
    for (CElementEntry* pEntry : tKeyValues) {
        std::cout << pEntry->Name << " == " << pEntry->Value << "\n";
    }
}


/**
 * @brief checks if the key name exists in this node.
 * It searches for elements and objects..
 */
bool CSimpleJsonNode::exists(const char* pszName) {
    bool bFound = false;
    for (CElementEntry* pEntry : tKeyValues) {
        if (pEntry->Name == pszName) {
            bFound = true;
            break;
        }
    }
    if(!bFound) {
        for (CSimpleJsonNode* pEntry : this->tSubNodes) {
            if (pEntry->Name == pszName) {
                bFound = true;
                break;
            }
        }
    }
    return(bFound);
}

bool CSimpleJsonNode::hasObject(const char * pszName) {
    return(getObject(pszName) != nullptr);
}

bool CSimpleJsonNode::hasElement(const char * pszName) {
    return(getElement(pszName) != nullptr);
}

CSimpleJsonNode * CSimpleJsonNode::getObject(const char *pszName) {
    CSimpleJsonNode *pResultNode = nullptr;
    for(CSimpleJsonNode *pNode : this->tSubNodes) {
        if(pNode && pNode->Name == pszName) { pResultNode = pNode; break; }
    }
    return(pResultNode);
}

 CSimpleJsonNode * CSimpleJsonNode::createOrGetObject(const char *pszName) {
    CSimpleJsonNode *pResultNode = getObject(pszName);
    if(pszName && !pResultNode) {
        pResultNode = new CSimpleJsonNode(pszName);
        tSubNodes.push_back(pResultNode);
    }
    return(pResultNode);
}

CSimpleJsonNode::CElementEntry * CSimpleJsonNode::getElement(const char * pszName) {
    CSimpleJsonNode::CElementEntry * pResult = nullptr;
    for (CElementEntry * pEntry : tKeyValues) {
        if (pEntry->Name == pszName) {
            pResult = pEntry; 
            break;
        }
    }
    return(pResult);
}

/**
 * @brief: Get the value of a Json Element.
 * 
 */
const char * CSimpleJsonNode::getValue(const char* pszName, const char *pszDefault) {
    const char* pszResult = pszDefault;
    for (CElementEntry* pEntry : tKeyValues) {
        if (pEntry->Name == pszName) {
            pszResult = pEntry->Value.c_str(); 
            break;
        }
    }
    return(pszResult);
}

const char* CSimpleJsonNode::parse(const char* pszJsonData) {
    // int nOpenBrackets = 0;
    // bool bScanName = true;
    bool bEscapeIsActive = false;
    bool bStringIsActive = false;
    std::string strName;
    std::string strValue;
    std::string strData;
    strData.reserve(512);
    // char szData[512] = { 0 };
    // char szName[40] = { 0 };
    // char szValue[512] = { 0 };
    // std::string strData;
    // unsigned int nDataPos = 0;
    while (*pszJsonData) {
        // printf("%c", *pszJsonData);
        switch (*pszJsonData) {
        case ' ':
        case '\r':
        case '\n':
        case '\t': 
            if (bStringIsActive) strData += *pszJsonData;
            break;

        case '{': // Opening bracket at this level ?
            if (strName.length() > 0) {
                CSimpleJsonNode* pSubJson = new CSimpleJsonNode(strName.c_str());
                tSubNodes.push_back(pSubJson);
                pszJsonData = pSubJson->parse(pszJsonData);
            }
            break;

        case '\\':
            if (!bEscapeIsActive) bEscapeIsActive = true;
            else { strData += '\\'; bEscapeIsActive = false; }
            break;

        case '"':
            if (bEscapeIsActive) { strData += '"'; bEscapeIsActive = false;  }
            else { bStringIsActive = !bStringIsActive; };
            break;

        case '}': // Closing bracket at this level
        case ',': // Close Element...
            if (bStringIsActive) strData += *pszJsonData;
            else {
                strValue = strData;
                strData.clear();
                // strncpy(szValue, szData, nDataPos);
                // szValue[nDataPos] = '\0';
                // nDataPos = 0;
            }
            // Todo - store the element...
            // CNodeEntry * pEntry = new CNodeEntry(strName, strValue);
            this->tKeyValues.push_back(new CElementEntry(strName, strValue));

            // printf("Setting value >>%s<< == >>%s<<\n", strName.c_str(), strValue.c_str());
            
            // this->oKeyValues.set(szName, szValue);
            break;

        case ':':
            if (bStringIsActive) strData += ':';
            else {
                strName = strData;
                strData.clear();
                // strncpy(szName, szData, nDataPos);
                // szName[nDataPos] = '\0';
                // nDataPos = 0;
            }
            break;
        default:
            // Scan the text...
            strData += *pszJsonData;
            break;
        }
        pszJsonData++;
    }
    return(pszJsonData);
}
