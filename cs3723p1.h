/**********************************************************************
cs3723p1.h
Purpose:
    Defines constants for
        boolean values
        maximum sizes
        program return codes
        error messages
    Defines typedefs for
        MetaAttr - describes one attribute within a node type
		NodeType - describes one node type
        AllocNode represents an allocated node.  The actual size of an
            allocated item may be much larger.  The size of an allocated
            item cannot be less than the size of a FreeNode.
        StorageManager is the primary structure used by this program.
        SMResult is used by the smrc... functions to
            specify whether they executed successfully.
    Prototypes
Notes:

**********************************************************************/
#define TRUE 1
#define FALSE 0

#define MAX_KEY_SIZE 10                     // Maximum size of a key for Hash Table
#define MAX_MESSAGE_SIZE 100                // Maximum message size for smResult
#define MAX_STRING 30                       // Maximum size of strings like
                                            // node type names, attribute names
#define MAX_NODE_TYPE 5	                    // Maximum number of node types
#define MAX_NODE_ATTR 50                    // Maximum number of combined node attr
#define MAX_DATA_SZ 500                     // Maximum size of sbData
#define ERROR_PROCESSING 3                  // error during processing - exit value

#define NOT_FOUND -1                        // Could not find name in metadata

// Errors returned in the rc of SMResult
#define RC_NOT_AVAIL 901            // There isn't any free memory to handle alloc request
#define RC_CANT_FREE 902            // Attempted to free a node which isn't allocated.
#define RC_INVALID_ADDR 903         // Invalid address which isn't within heap
#define RC_ASSOC_ATTR_NOT_PTR 801   // Attribute name for ASSOC not a pointer attribute
#define RC_ASSOC_ATTR_NOT_FOUND 802 // Attribute name for ASSOC not found for the from node

// Error messages provided by the driver if it encounters problems with the input file
#define ERR_INVALID_COMMAND "INVALID COMMAND "        // Command file has invalid data
#define ERR_ALLOC  "Invalid ALLOC command argument "
#define ERR_DEREF  "Invalid DEREF command argument "
#define ERR_ASSOC  "Invalid ASSOC command argument "
#define ERR_ADDREF "Invalid ADDREF command argument "
#define ERR_PRTNODE "Invalid PRTNODE command argument "
#define ERR_NODETYPE_NOT_FOUND "Data has bad node type: "
#define ERR_HEAP    "Heap Memory Error: "

// Error messages for unexpected situations in the driver
#define ERR_NULL_AREA "Storage Mgr Begin is NULL "
#define ERR_ALGORITHM "Unexpected error in algorithm "
#define ERR_INVALID_HEXDUMP_PARM "Invalid hexDump parameter "

// MetaAttr describes an attribute within a node type
typedef struct MetaAttr
{
    short  shNodeType;                      // Type of node
    char   szAttrName[MAX_STRING+1];        // Name of the attribute
    char   cDataType;                       // Data type: S - char string, P -Ptr, D - double, I - int
    short  shSizeBytes;                     // size in bytes including zero byte for strings
    short  shOffset;
}MetaAttr;
// NodeType describes one type of node
typedef struct NodeType
{
	char szNodeTypeNm[MAX_STRING+1];
	short shBeginMetaAttr;              // Subscript in metaAttrM of first attribute for
                                        // this node type.
	short shNodeTotalSize;
}NodeType;

// AllocNode represents an allocated node.  The actual size of an allocated item may be much
// larger.
typedef struct AllocNode
{
    short shAllocSize;                 // total size of the allocated item.
                                       // This is negative for free nodes.
    short shRefCount;                  // The number of references to this allocated node.
    short shNodeType;                  // Node Type subscript.
    char  sbData[MAX_DATA_SZ];         // This is the user's data in the node.  It might
                                       // be bigger than MAX_STRING.
} AllocNode;

// StorageManager is the primary structure used by this program.
typedef struct
{
    int iHeapSize;                       // Total size of the heap memory being managed
    char *pBeginStorage;                 // Beginning of the heap memory being managed
    char *pEndStorage;                   // End address immediately after the heap memory
    char *pFreeTop;                      // Top of Free Memory
    NodeType nodeTypeM[MAX_NODE_TYPE];   // array of node types
    MetaAttr metaAttrM[MAX_NODE_ATTR];   // array of attribute meta data
} StorageManager;

// This is returned by smAllocate and smFree via the parmater list.
typedef struct
{
    int rc;                                // Return Code is 0 if it is normal.  Otheriwise,
                                           // it is not zero.  See the defined constants.
    char szErrorMessage[MAX_MESSAGE_SIZE + 1];  // If a problem is encountered, this should
                                                // explain the error.
} SMResult;

// student functions
void * smrcAllocate(
    StorageManager *pMgr, short shDataSize, short shNodeType, char sbData[],
    SMResult *psmResult);
void smrcRemoveRef(
    StorageManager *pMgr, void *pUserData, SMResult *psmResult);
void smrcAssoc(
    StorageManager *pMgr, void *pUserDataFrom, char szAttrName[],
    void *pUserDataTo, SMResult *psmResult);
void smrcAddRef(StorageManager *pMgr, void *pUserDataTo, SMResult *psmResult);
void initMemNode();

#if defined(_WIN32) || defined(_WIN64)
extern void printNode(StorageManager *pMgr, void *pUserData);
#else
extern "C" void printNode(StorageManager *pMgr, void *pUserData);
#endif


// Driver functions
void smPrintMeta(StorageManager *pMgr);

void smPrintFree(StorageManager *pMgr);
short findNodeType(StorageManager *pMgr, char szNodeTypeNm[]);
AllocNode * smAlloc(StorageManager *pMgr, short shTotalSize);
void smFree(StorageManager *pMgr, void *pUserData, SMResult *psmResult);
void smInit(StorageManager *pMgr);
void smDump(StorageManager *pMgr);

void exitError(const char *pszMessage, const char *pszDiagnosticInfo);

// Hex Dump
int hexDump(char *psbBuffer, int iBufferLength, int iBytesPerLine);

//void xxdDump(void *, int);

