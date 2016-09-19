/**********************************************************************
    cs3723p1Driver.c by Larry Clark
Purpose:
    This program demonstrates Storage Management software responsible 
    for reference counts. 
Command Parameters:
    p1 < infile > outfile
    The stream input file contains commands.  There are commands 
    which specify comments, allocating memory, dereferencing memory, 
    adding references, associating nodes to other nodes, and printing 
    nodes. 
   
    Commands in the input file:
    
    * comment       
        This is just a comment which helps explain what is being tested.
    
    ALLOC key nodeTypeNm val1, val2, ...
        Establish a user data node of the specified type.  The data (val1, val2, ...) will be 
        placed in the user data node.  This command invokes smrcAllocate which returns a user data 
        pointer to allocated memory.  Key is used as an entry in a Hash Table which has a 
        corresponding value set to the user data pointer.
    DEREF key NULL
        Frees the memory referenced by the key.  This command invokes smrcRemoveRef.  If NULL
        is also specified, it sets the hash table entry's value to NULL.
    ADDREF newKey oldKey
        Adds a reference to th oldKey's node using newKey as the hash table entry.  
        This invokes smrcAddRef.
    ASSOC keyFrom attrName keyTo
        The user data for the specified keyFrom will be changed to point to the pointer
        referenced by keyTo.  If the node referenced by keyFrom already has a value
        for the specified attribute, that existing referenced node must be dereferenced.
        Note that keyTo might be NULL, causing the attribute to change to NULL.
    PRTNODE key
        Invokes printNode for the node referenced by the specified key.  
    PRTALL
        Prints all nodes in the memory.
    DUMP 
        Dumps all the nodes in the memory.

Results:
    All output is written to stdout.
    Prints the metadata.
    Prints each of the commands and their command parameters.  Some of the commands
    (PRTNODE, PRTALL, DUMP) also print information.

Returns:
    0       Normal
    3       Processing Error

Notes:
   
**********************************************************************/
// If compiling using visual studio, tell the compiler not to give its warnings
// about the safety of scanf and printf
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cs3723p1.h"

#define MAX_TOKEN_SIZE 50		// largest token size for tokenizer
#define MAX_BUFFER_SZ 100       // size of input buffer

// prototypes only used by the driver
void processCommands(StorageManager *pMgr, FILE *pfileCommand);
int getSimpleToken(char szInput[], const char szDelims[]
    , int *piBufferPosition, char szToken[]);
void setData(StorageManager *pMgr, short shNodeType, char szInput[], char sbData[]);
void initMetadata(StorageManager *pMgr);
void printMeta(StorageManager *pMgr);

// Simple macro for converting addresses to unsigned long
#define ULAddr(addr) ((unsigned long) addr)

// If on Windows, don't use extern "C" in calling file.
// g++ compiler requires the extern "C".
#if defined(_WIN32) || defined(_WIN64)
extern void *getHash(const char *szKey);
extern void putHash(const char *szKey, void *value);
extern void eraseAll();
extern void printAll(StorageManager *pMgr);
#else
extern "C" void *getHash(const char *szKey);
extern "C" void putHash(const char *szKey, void *value);
extern "C" void eraseAll();
extern "C" void printAll(StorageManager *pMgr);
#endif

int main(int argc, char *argv[])
{
    StorageManager storageManager;

    // Set up the storage manager and our metadata
    smInit(&storageManager);
    initMetadata(&storageManager);
    
    // Print the metadata
    printMeta(&storageManager);
    
    // Process commands for manipulating user data nodes
    processCommands(&storageManager, stdin);
    free(storageManager.pBeginStorage);
    printf("\n");
    return 0;
}
/******************** smInit **************************************
    void smInit(StorageManager *pMgr)
Purpose:
    Initializes the heap and corresponding attributes in the 
    storage manager.
Parameters:
    O StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
Returns:
    n/a
Notes:
    - Uses malloc to actually allocate memory for the heap.
**************************************************************************/
#define HEAP_SIZE 600
void smInit(StorageManager *pMgr)
{
    pMgr->pBeginStorage = (char *)malloc(HEAP_SIZE);
    if (pMgr->pBeginStorage == NULL)
        exitError(ERR_HEAP, "malloc failed to allocate heap ");
    pMgr->iHeapSize = HEAP_SIZE;
    pMgr->pEndStorage = pMgr->pBeginStorage + HEAP_SIZE;
    pMgr->pFreeTop = pMgr->pBeginStorage;
}

/******************** initMetadata **************************************
    void initMetadata(StorageManager *pMgr)
Purpose:
    Initializes the nodeTypeM and metaAttrM arrays with metadata.
Parameters:
    O StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
Notes:
   
**************************************************************************/
void initMetadata(StorageManager *pMgr)
{
#define SHCUST 0   // node type for Customer
#define SHLINE 1   // node type for LineItem
    // The following macro gives values to the size and offset attributes
#define SIZE_OFFSET(ptr,attr) \
    ((short)sizeof(attr)), (short)(((char *)&attr) - ((char *) ptr))

    struct LineItem
    {
        char szProductId[10];
        int  iQtyRequest;
        double dCost;
        struct LineItem *pNextItem;
    } lineItem;
    struct Customer
    {
        char szCustomerId[12];
        char szName[20];
        struct LineItem *pFirstItem;
        struct Customer *pNextCustomer;
        double dBalance;
    } customer;
    char *pCustomer = (char *)&customer;
    char *pLineItem = (char *)&lineItem;
    int iN = 0;
    // nodeTypeM contains metadata about each node type which will be copied to storage manager
    NodeType nodeTypeM[MAX_NODE_TYPE] =
    {
        { "Customer", 0, sizeof(struct Customer) }
        , { "LineItem", 5, sizeof(struct LineItem) }
        , { "", 0 } // sentinel
    };
    // metaAttrM contains metadata about each user data attribute which will be copied
    // to storage manager.  This is using the excellent initialization capability of C.
    MetaAttr metaAttrM[MAX_NODE_ATTR] =
    {
        { SHCUST, "customerId", 'S', SIZE_OFFSET(pCustomer, customer.szCustomerId) }
        , { SHCUST, "name", 'S', SIZE_OFFSET(pCustomer, customer.szName) }
        , { SHCUST, "pFirstItem", 'P', SIZE_OFFSET(pCustomer, customer.pFirstItem) }
        , { SHCUST, "pNextCust", 'P', SIZE_OFFSET(pCustomer, customer.pNextCustomer) }
        , { SHCUST, "balance", 'D', SIZE_OFFSET(pCustomer, customer.dBalance) }
        , { SHLINE, "productId", 'S', SIZE_OFFSET(pLineItem, lineItem.szProductId) }
        , { SHLINE, "iQtyReq", 'I', SIZE_OFFSET(pLineItem, lineItem.iQtyRequest) }
        , { SHLINE, "dCost", 'D', SIZE_OFFSET(pLineItem, lineItem.dCost) }
        , { SHLINE, "pNextItem", 'P', SIZE_OFFSET(pLineItem, lineItem.pNextItem) }
        , { -1, "END_OF_ATTR" } // sentinel
    };
    memcpy(pMgr->nodeTypeM, nodeTypeM, sizeof(nodeTypeM));
    memcpy(pMgr->metaAttrM, metaAttrM, sizeof(metaAttrM));
}
/******************** printMeta **************************************
    void printMeta(StorageManager *pMgr)
Purpose:
    Prints metadata about each node type.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
Notes:
   
**************************************************************************/
void printMeta(StorageManager *pMgr)
{
    int iN;
    int iAt;
    printf("Metadata\n");
    printf("%-10s %-12s %8s\n", "Node Type", "Beg Attr Sub", "Total Sz");
    // Loop for each node type.  The end is marked by a name which is an empty string.
    for (iN = 0; pMgr->nodeTypeM[iN].szNodeTypeNm[0] != '\0'; iN++)
    {
        printf("%-10s %4d%8s %4d\n"
            , pMgr->nodeTypeM[iN].szNodeTypeNm
            , pMgr->nodeTypeM[iN].shBeginMetaAttr
            , " "
            , pMgr->nodeTypeM[iN].shNodeTotalSize);
        printf("\t\t%-14s %-4s %-6s %-4s\n"
            , "Attribute Name"
            , "Type"
            , "Offset"
            , "Size");
        // The attributes for the node type begin at its shBeginMetaAttr subscript and continue while
        // the shNodeType is this node's node type (which is the node type's subscript in
        // the nodeTypeM array).
        for (iAt = pMgr->nodeTypeM[iN].shBeginMetaAttr; pMgr->metaAttrM[iAt].shNodeType == iN; iAt++)
        {
            printf("\t\t%-14s   %c  %6i %4i\n"
                , pMgr->metaAttrM[iAt].szAttrName
                , pMgr->metaAttrM[iAt].cDataType            
                , pMgr->metaAttrM[iAt].shOffset
                , pMgr->metaAttrM[iAt].shSizeBytes);
        }

    }
}
/******************** processCommands **************************************
    void processCommands(StorageManager *pMgr, FILE *pfileCommand)
Purpose:
    Reads the Command file to process commands.  There are several types of
    records (see the program header for more information).
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
    I FILE *pfileCommand    command stream input
Notes:
    This calls functions inside:
        hashAPi
        hexDump64
        cs3723p1 
**************************************************************************/
void processCommands(StorageManager *pMgr, FILE *pfileCommand)
{
    // variables for command processing
    char szInputBuffer[MAX_BUFFER_SZ+1];    // input buffer for a single text line
    char szCommand[MAX_TOKEN_SIZE + 1];     // command
    int bGotToken;                          // TRUE if getSimpleToken got a token
    int iBufferPosition;                    // This is used by getSimpleToken to 
                                            // track parsing position within input buffer

    // variables used for the buffer passed to hexdump 
    int iBytesPerLine = 40;                 // number of bytes to be displayed per line
                                            // by hexDump
    int iScanfCnt;                          // number of values returned by sscanf
    
    int iBytesToSend = 0;                   // number of bytes sent to hexDump
    int iLines = 0;                         // number of lines returned from hexDump

    SMResult smResult = { 0, "" };

    //  get command data until EOF
    while (fgets(szInputBuffer, MAX_BUFFER_SZ, pfileCommand) != NULL)
    {
        // if the line is just a line feed, ignore it
        if (szInputBuffer[0] == '\n')
            continue;

        // get the command
        iBufferPosition = 0;                // reset buffer position
        bGotToken = getSimpleToken(szInputBuffer, " \n\r", &iBufferPosition, szCommand);
        if (! bGotToken)
            exitError(ERR_INVALID_COMMAND, szInputBuffer);

        // see if the command is a comment
        if (szCommand[0]== '*')
        {
            printf("%s", szInputBuffer);
            continue;       // it was just a comment
        }
        memset(&smResult, '\0', sizeof(SMResult));  // in case the smrc functions didn't
        printf(">>> %s", szInputBuffer);

        if (strcmp(szCommand, "ALLOC") == 0)
        {   // ALLOC key nodeTypeNm val1, val2, ...
            char szKey[MAX_KEY_SIZE + 1];
            char szNodeTypeNm[MAX_STRING + 1];
            char szRemainingInput[MAX_DATA_SZ + 1]; // Used for a node's data values
            char sbData[MAX_DATA_SZ];
            void *pUserData = NULL;
            iScanfCnt = sscanf(&szInputBuffer[iBufferPosition]
                , "%10s %10s %100[^\n]"
                , szKey
                , szNodeTypeNm
                , szRemainingInput);
            if (iScanfCnt <  3)
                exitError(ERR_ALLOC, szInputBuffer);
            // get the node type (i.e., subscript in the nodeTypeM array
            short shNodeType = findNodeType(pMgr, szNodeTypeNm);
            if (shNodeType == NOT_FOUND)
                exitError(ERR_NODETYPE_NOT_FOUND, szNodeTypeNm);
            short shSize = pMgr->nodeTypeM[shNodeType].shNodeTotalSize;
            // Set up the data attributes in the user data
            setData(pMgr, shNodeType, szRemainingInput, sbData);
            // Invoke the Ref Count Storage Manager Allocate function
            pUserData = smrcAllocate(pMgr, shSize, shNodeType, sbData, &smResult);

            // If it allocated memory, save the memory address with the key
            if (pUserData == NULL)
            {    // Did not allocate memory
                printf("\t!!! Memory not allocated\n");
                printf("\t\tsmAllocate rc=%d, %s\n"
                    , smResult.rc
                    , smResult.szErrorMessage);
            }
            else
                putHash(szKey, pUserData);  // record where it was placed
        }
        else if (strcmp(szCommand, "DEREF") == 0)
        {   // FREE key    This can have an optional NULL.
            char szKey[MAX_KEY_SIZE + 1];
            char *pUserData;
            char szNULL[MAX_STRING + 1] = "";

            // get the key from the input
            iScanfCnt = sscanf(&szInputBuffer[iBufferPosition]
                , "%10s %4s"
                , szKey
                , szNULL);
            // was the key in it?
            if (iScanfCnt <  1)
                exitError(ERR_DEREF, szInputBuffer);
            
            // get the address to free 
            pUserData = (char *)getHash(szKey);
            if (pUserData == NULL)
            {
                printf("*** getHash returned NULL\n");
                continue;
            }
            else
            {    // Invoke the Storage Manager Free function
                smrcRemoveRef(pMgr, pUserData, &smResult);
                if (smResult.rc != 0)
                {
                    printf("\t\tsmrcRemoveRef rc=%d, %s\n"
                        , smResult.rc
                        , smResult.szErrorMessage);
                }
                else if (strcmp(szNULL, "NULL") == 0)
                    putHash(szKey, NULL);
            }
        }
        else if (strcmp(szCommand, "ASSOC") == 0)
        {   // ASSOC keyFrom attrName keyTo
            char szKeyFrom[MAX_KEY_SIZE + 1];
            char szKeyTo[MAX_KEY_SIZE + 1];
            char szAttrNm[MAX_STRING + 1];
            void *pFromData = NULL;
            void *pToData = NULL;
            iScanfCnt = sscanf(&szInputBuffer[iBufferPosition]
                , "%10s %10s %10s"
                , szKeyFrom
                , szAttrNm
                , szKeyTo);
            if (iScanfCnt < 3)
                exitError(ERR_ASSOC, szInputBuffer);
            // get the user address of the from 
            pFromData = (char *)getHash(szKeyFrom);
            if (pFromData == NULL)
            {
                printf("*** getHash for 'from' returned NULL\n");
                continue;
            }
            // get the user address of the to 
            pToData = (char *)getHash(szKeyTo);
            if (pToData == NULL)
            {
                printf("*** getHash for 'to' returned NULL\n");
                continue;
            }
            smrcAssoc(pMgr, pFromData, szAttrNm, pToData, &smResult);
            if (smResult.rc != 0)
            {
                printf("\t\tsmrcAssoc rc=%d, %s\n"
                    , smResult.rc
                    , smResult.szErrorMessage);
            }
        }
        else if (strcmp(szCommand, "ADDREF") == 0)
        {   // ASSOC keyFrom attrName keyTo
            char szNewKey[MAX_KEY_SIZE + 1];
            char szOldKey[MAX_KEY_SIZE + 1];
            void *pUserData = NULL;
            iScanfCnt = sscanf(&szInputBuffer[iBufferPosition]
                , "%10s %10s"
                , szNewKey
                , szOldKey);
            if (iScanfCnt < 2)
                exitError(ERR_ADDREF, szInputBuffer);
            // get the address of the existing node 
            pUserData = (char *)getHash(szOldKey);
            if (pUserData == NULL)
            {
                printf("*** getHash returned NULL\n");
                continue;
            }
            smrcAddRef(pMgr, pUserData, &smResult);
            if (smResult.rc != 0)
            {
                printf("\t\tsmrcAddRef rc=%d, %s\n"
                    , smResult.rc
                    , smResult.szErrorMessage);
            }
            else
                putHash(szNewKey, pUserData);  // record where it was placed
        }
        else if (strcmp(szCommand, "DUMP") == 0)
            smDump(pMgr);
        else if (strcmp(szCommand, "PRTNODE") == 0)
        {   // PRTNODE key
            char szKey[MAX_KEY_SIZE + 1];
            char *pUserData;

            // get the key from the input
            iScanfCnt = sscanf(&szInputBuffer[iBufferPosition]
                , "%10s"
                , szKey);
            // was the key in it?
            if (iScanfCnt <  1)
                exitError(ERR_PRTNODE, szInputBuffer);

            // get the address to free 
            pUserData = (char *)getHash(szKey);
            if (pUserData == NULL)
            {
                printf("*** getHash returned NULL\n");
                continue;
            }
            else
            {
                printNode(pMgr, pUserData);
            }
        }
        else if (strcmp(szCommand, "PRTALL") == 0)
        {
            // Print all the nodes having a reference in the hash table
            printAll(pMgr);
            printf("\n");
        }
        else
            exitError(ERR_INVALID_COMMAND, szCommand);
    }
    printf("\n");;
}
/******************** findNodeType **************************************
    short findNodeType(StorageManager *pMgr, char szNodeTypeNm[])
Purpose:
    Searches the storage manager's nodeTypeM array to find the 
    specified node type and returns its subscript.  If not found
    it returns NOT_FOUND.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
    I char szNodeTypeNm[]   Node type to find.
Returns:
    Subscript of the node type in the nodeTypeM array.
    NOT_FOUND (-1) is it isn't found.
**************************************************************************/
short findNodeType(StorageManager *pMgr, char szNodeTypeNm[])
{
    int iN;
    // Loop for each node type.  The end is marked by a name which is an empty string.
    for (iN = 0; pMgr->nodeTypeM[iN].szNodeTypeNm[0] != '\0'; iN++)
    {
        if (strcmp(pMgr->nodeTypeM[iN].szNodeTypeNm, szNodeTypeNm) == 0)
            return iN;
    }
    return NOT_FOUND;
}
/******************** setData **************************************
    void setData(StorageManager *pMgr, short shNodeType
        , char szInput[], char sbData[])
Purpose:
    Uses metadata to set the attributes of user data based on comma-
    separated input text.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
    I short shNodeType      Node type of the user data
    I char szInput[]        String containing the comma separated input text
    O char sbData[]         Binary data set by this function
Returns:
    n/a
Notes:
    Assumes that shNodeType is a valid subscript in the nodeTypeM array.
**************************************************************************/
void setData(StorageManager *pMgr, short shNodeType, char szInput[], char sbData[])
{
    int iAt;                        // control variable representing subscript in metaAttrM
    char szToken[MAX_STRING];       // token returned by getSimpleToken
    int iBufferPos = 0;             // current buffer position used by getSimpleToken
    MetaAttr *pAttr;                // slightly simplifies referencing item in metaAttrM
    int iScanfCnt;                  // returned by sscanf
    int iValue;                     // integer value when attribute is an int
    double dValue;                  // double value when attribute is a double
    char *pszMemAtOffset;           // pointer into user data if this attribute is a string
    int *piMemAtOffset;             // pointer into user data if this attribute is an int
    void **ppNode;                  // pointer into user data if this attribute is a pointer
    double *pdMemAtOffset;          // pointer into user data if this attribute is a double
    int iLen;                       // helps with checking too long of a string value
    // zero out the user data
    memset(sbData, '\0', pMgr->nodeTypeM[shNodeType].shNodeTotalSize);
    // Loop through each of the user data's attributes.  The subscripts start with
    // shBeginMetaAttr from nodeTypeM and end when the corresponding metaAttr's node type is
    // different.
    for (iAt = pMgr->nodeTypeM[shNodeType].shBeginMetaAttr; pMgr->metaAttrM[iAt].shNodeType == shNodeType; iAt++)
    {
        pAttr = &(pMgr->metaAttrM[iAt]);   // slightly simplify the reference in the metaAttrM array
        // get the next token from the input
        if (!getSimpleToken(szInput, ",\n\r",&iBufferPos, szToken))
            return;
        
        // set the data based on the attribute data type
        switch (pAttr->cDataType)
        {
            case 'P':  // pointer
                // The value in the data must state NULL
                if (strcmp(szToken, "NULL")!= 0)
                    exitError(ERR_ALLOC, szToken);
                // get the attribute's address based on its offset
                ppNode = (void **) &(sbData[pAttr->shOffset]);
                *ppNode = NULL;                 // assign it NULL
                break;
            case 'S':  // char string
                // check for too long of a value
                iLen = strlen(szToken);
                if (iLen > pAttr->shSizeBytes - 1)
                    exitError(ERR_ALLOC, szToken);
                // get the attribute's address based on its offset
                pszMemAtOffset = (char *) &(sbData[pAttr->shOffset]);
                strcpy(pszMemAtOffset, szToken);
                break;
            case 'I':  // int
                // Convert the token to an int
                iScanfCnt = sscanf(szToken, "%d", &iValue);
                if (iScanfCnt < 1)
                    exitError(ERR_ALLOC, szToken);
                // get the attribute's address based on its offset
                piMemAtOffset = (int *) &(sbData[pAttr->shOffset]);
                *piMemAtOffset = iValue;
                break;
            case 'D':  // double
                // Convert the token to a double
                iScanfCnt = sscanf(szToken, "%lf", &dValue);
                if (iScanfCnt < 1)
                    exitError(ERR_ALLOC, szToken);
                // get the attribute's address based on its offset
                pdMemAtOffset = (double *)&(sbData[pAttr->shOffset]);
                *pdMemAtOffset = dValue;
                break;
            default:
                exitError(ERR_ALGORITHM, "Invalid metadata type");
        }
    }
}
/******************** smDump **************************************
    void smDump(StorageManager *pMgr)
Purpose:
    Uses hexDump to dump each node in the heap. 
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
Returns:
    n/a
Notes:
    - The heap begins at pBeginStorage and active memory ends at pFreeTop
    - Uses hexDump64.c's hexDump function.
**************************************************************************/
void smDump(StorageManager *pMgr)
{
    int iBytesToSend;
    int iBytesPerLine = 20;
    char *pCh;
    short shTempSize;
    AllocNode *pAlloc;
    // Print each item from the beginning of the heap to the end
    printf("\t%-8s %-5s %4s\n", "Address", "Mem", "Size");
    for (pCh = pMgr->pBeginStorage; pCh < pMgr->pFreeTop; pCh += shTempSize)
    {
        pAlloc = (AllocNode *)pCh;
        shTempSize = pAlloc->shAllocSize;

        // See if it is an allocated item or a free item
        if (shTempSize < 0)
        {   // It is a free item
            shTempSize = -shTempSize;  // negate it so we can use it
            printf("\t%08lX %-5s %4d\n"
                , ULAddr(pAlloc), "Free", shTempSize);
            iBytesToSend = shTempSize - sizeof(short);
            hexDump(pCh + sizeof(short), iBytesToSend, iBytesPerLine);
        }
        else
        {  // It is an allocated Item
            printf("\t%08lX %-5s %4d\n", ULAddr(pAlloc), "Alloc", shTempSize);
            iBytesToSend = shTempSize - sizeof(short);
            hexDump(pCh + sizeof(short), iBytesToSend, iBytesPerLine);
        }
    }
}
/******************** smAlloc **************************************
    AllocNode *smAlloc(StorageManager *pMgr, short shTotalSize)
Purpose:
    To allocate memory, it simply advances a pointer in the heap.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
    I short shTotalSize     Total amount of memory desired which includes
                            memory for the user data, length, node type,
                            and reference count.
Returns:
    - Memory address of the allocated memory.  This is NOT the user data address.
    - NULL if memory could not be allocated. 
Notes:
    - The heap begins at pBeginStorage and active memory ends at pFreeTop
**************************************************************************/
AllocNode *smAlloc(StorageManager *pMgr, short shTotalSize)
{
    AllocNode *pAlloc;
    // Make certain there is still heap memory to allocate
    if (pMgr->pFreeTop == NULL || pMgr->pFreeTop >= pMgr->pEndStorage)
        return NULL;
    pAlloc = (AllocNode *) pMgr->pFreeTop;
    // if the allocated memory would extend past the end of the heap,
    // return NULL
    if (pMgr->pFreeTop + shTotalSize >= pMgr->pEndStorage)
        return NULL;
    pMgr->pFreeTop += shTotalSize;
    pAlloc->shAllocSize = shTotalSize;
    return pAlloc;
}
/******************** smFree **************************************
    void smFree(StorageManager *pMgr, void *pUserData, SMResult *psmResult)
Purpose:
    For this assignment, smFree doesn't actually free the memory.  Instead,
    it simply changes the sign on shAllocSize.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
    I/O void *pUserData     Points to the user data (which is after the beginning 
                            of the allocated node)
    O SMResult *psmResult   Result structure containing a return code and
                            an error message.  For normal execution, 
                            the rc should be set to zero.
Returns:
    n/a
Notes:
    - If the referenced address is outside the heap, it returns an error via
      psmResult.
    - If the size is negative or zero, this shouldn't be freed so it returns
      an error via psmResult.
**************************************************************************/
void smFree(StorageManager *pMgr, void *pUserData, SMResult *psmResult)
{
    // The user data is preceded by three short intgers: 
    //    shAllocSize, shRefCount, and shNodeType.
    // To get the allocated node's address from the user data address, we must
    // subtract the size of three short integers.

    AllocNode *pAlloc = (AllocNode *)(((char *)pUserData) - 3 * sizeof(short));
    psmResult->rc = 0;

    // If the referenced address is outside the heap, it returns an error via
    // psmResult.
    if ((char *)pAlloc < pMgr->pBeginStorage || (char *) pAlloc >= pMgr->pEndStorage)
    {
        psmResult->rc = RC_INVALID_ADDR;
        sprintf(psmResult->szErrorMessage, "Address (%p) is not in the heap ", pAlloc);
        return;
    }
    // If the size is negative (already free) or zero, this shouldn't be freed so 
    // it returns an error via psmResult.
    if (pAlloc->shAllocSize <= 0)
    {
        psmResult->rc = RC_CANT_FREE;
        strcpy(psmResult->szErrorMessage, "Not a valid allocated node");
        return;
    }

    // Since it is free, indicate that with a negative address.
    pAlloc->shAllocSize = -(pAlloc->shAllocSize);
}
/*
** This Hex Dump can be used instead of calling the real
** hexDump from smDump when using Visual Studio.  Rename this to hexDump.
** Please remember to remove it when you run your code on a fox server.
*/
int dumbHexDump(char *sbBuffer, int iBufferLength, int iBytesPerLine)
{
    printf("\t\t\t, %s", sbBuffer);  // print the data
    return 1;	// arbitrary value and meaningless in this case
}

/******************** getSimpleToken **************************************
 int getSimpleToken(char szInput[], char szDelims[]
     , int *piBufferPosition, char szToken[]) 
 Purpose:
    Returns the next token in a string.  The delimiter(s) for the token is  
    passed as a parameter.  To help positioning for a subsequent call, it 
    also returns the next position in the buffer.
Parameters:
    I   char szInput[]          input buffer to be parsed
    I   char szDelims[]         delimiters for tokens
    I/O int *piBufferPosition   Position to begin looking for the next token.
                                This is also used to return the next 
                                position to look for a token (which will
                                follow the delimiter).
    O   char szToken[]          Returned token.  
Returns:
    Functionally:
        TRUE - a token is returned
        FALSE - no more tokens
    iBufferPosition parm - the next position for parsing
    szToken parm - the returned token.  If not found, it will be an empty string.
Notes:
    - If the token is larger than MAX_TOKEN_SIZE, we return a truncated value.
**************************************************************************/

int getSimpleToken(char szInput[], const char szDelims[]
    , int *piBufferPosition, char szToken[]) 
{
    int iDelimPos;                      // found position of delim
    int iCopy;                          // number of characters to copy
    
    // check for past the end of the string
    if (*piBufferPosition >= (int) strlen(szInput))
    {
        szToken[0] = '\0';              // mark it empty
        return FALSE;                   // no more tokens
    }

    // get the position of the first delim, relative to the iBufferPosition
    iDelimPos = strcspn(&szInput[*piBufferPosition], szDelims);

    // see if we have more characters than target token, if so, trunc
    if (iDelimPos > MAX_TOKEN_SIZE)
        iCopy = MAX_TOKEN_SIZE;             // truncated size
    else
        iCopy = iDelimPos;
    
    // copy the token into the target token variable
    memcpy(szToken, &szInput[*piBufferPosition], iCopy);
    szToken[iCopy] = '\0';              // null terminate

    // advance the position
    *piBufferPosition += iDelimPos + 1;  
    return TRUE;
}

/******************** exitError *****************************
void exitError(char *pszMessage, char *pszDiagnosticInfo)
Purpose:
    Prints an error message and diagnostic to stderr.  Exits with
    ERROR_PROCESSING.
Parameters:
    I char *pszMessage              error message to print
    I char *pszDiagnosticInfo       supplemental diagnostic information
Notes:
    This routine causes the program to exit.
**************************************************************************/
void exitError(const char *pszMessage, const char *pszDiagnosticInfo)
{
    fprintf(stderr, "Error: %s %s\n"
        , pszMessage
        , pszDiagnosticInfo);
    exit(ERROR_PROCESSING);
}