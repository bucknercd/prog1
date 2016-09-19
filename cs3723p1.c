//mesquite room UC 2.01.24


/**********************************************************************
cs3723p1.c
Purpose:	
    This file has the function definitions for functions that allocate heap
    memory, remove references to objects, associate (create a reference) to an
    object, increment the object's ref count and print out an object
 Command Parameters:
    Description of the command parameters.  There are no commands. These
    functions are invoked by other functions
Input:
    N/A
Results:
    Depending on function called, data can be copied from the stack to the
    heap, data inside a particular function can be modfied (ref count,
    pointers), or the printing of the data of a particular node
Returns:
    all functions are void. nothing returned
Notes:
    
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cs3723p1.h"

#define ULAddr(addr) ((unsigned long) addr)

/******************** smrcAllocate **********************************
void * smrcAllocate(
	StorageManager *pMgr, short shDataSize, short shNodeType, char sbData[],
    SMResult *psmResult);
Purpose:
    allocates memory in the heap for AllocNode struct by calling smAlloc.
	sets refCount to 1, node type, and copies sbUserData from stack to
	to the heap in the correct offset of newly allocated AllocNode  
Parameters:
    I   StorageManager *pMgr	pointer,passed to smAlloc(), so it can allocate correct
    							space for new node on the heap
    I   short DataSize       	passed to smAlloc(), also use in knowing how much memory to 
    							copy from stack to the heap for node
    I   short shNodeType		used to set the node type in the newly alloc'd node
    I   char sbData             binary data in a character array, contians all data in node(on stack)
    I   SMResult *psmResult     pointer to error node , error mesg is set on this node
    O   Passed out. Value is returned through this parameter.
Notes:
    very important: userData info is being copied from the stack to our 
    virtual heap
**************************************************************************/
void * smrcAllocate(
    StorageManager *pMgr, short shDataSize, short shNodeType, char sbData[],
    SMResult *psmResult)
{
    // pAlloc pointer. point to start of alloc node
    AllocNode *pAlloc = smAlloc(pMgr, shDataSize + 3*sizeof(short));
    if (pAlloc == NULL)
    {
        psmResult->rc = RC_INVALID_ADDR;
        strcpy(
            psmResult->szErrorMessage,
            "Invalid Address: not enough memory to alloc\n");
        return NULL;
    } 
    /* set begining sections of AllocNode and copy userDate from stack to the
     * alloc'd node (heap) */
    else 
    {
        pAlloc->shRefCount = 1;
        pAlloc->shNodeType = shNodeType;
        pAlloc->shAllocSize = shDataSize + 3*sizeof(short);
        memcpy(pAlloc->sbData, sbData, shDataSize);
        // return a pointer to the user data of this allocNode 
        return ((char *)pAlloc) + 3 * sizeof(short);
    }
}

/******************** smrcRemoveRef **********************************
void smrcRemoveRef(StorageManager *pMgr, void *pUserData, SMResult *psmResult);
Purpose:
    
Parameters:
	I   StorageManager *pMgr	pointer, used to access metadata to loop thru
								different attributes; also passed to smFree()
								for error purposes (our virtual heap boundaries)
	I   void *pUserData 		pointer to the userData section of the node;
								passed to smFree so smFree knows what addr to free,
								also used to calculate our attr offsets, and to know 
								where to make an AllocNode pointer point to
	I   SMResult *psmResult		pointer to error struct, an error message may be set
								in this struct
Notes:
	a double pointer(ppNew) is used to point to another pointer at a certain 
	offset. It is needed so when we dereference it we can see what address it 
	points to. If it was just a single pointer we could't get the right information.
**************************************************************************/
void smrcRemoveRef(StorageManager *pMgr, void *pUserData, SMResult *psmResult)
{
	int i;				// incrementer variable
	char **ppNew;   	// double pointer. used to point to ptr attribute
	char szMesg[100];	// char array for our error message
	short type;			// node type 
	AllocNode *pAlloc;	// AllocNode pointer. used to access parts of our node

	// make pAlloc point to begining of our allocNode node
    pAlloc = (AllocNode *)(((char *)pUserData) - 3*sizeof(short));
	type = pAlloc->shNodeType;

	// if the node already has zero references to it, set the error code and return
    if (pAlloc->shRefCount == 0) 
    {
    	psmResult->rc = 1;
		sprintf(
			szMesg,
			"No reference to this node. %08lX cannot be freed.\n",
                        ULAddr(pUserData));
    	strcpy(psmResult->szErrorMessage, szMesg);
    	return;
    }
    psmResult->rc = 0;

    // lower reference count of the node
    pAlloc->shRefCount--;

    // if ref count is now zero ?
    if (pAlloc->shRefCount == 0)
    {

    	// free the node and (possibly) set appropriate error code
    	smFree(pMgr, pUserData, psmResult);

    	/* starting at beginning metaAttribute subscript, iterate thru all attributes
		 *  while the node type is the correct type */
    	for(i = pMgr->nodeTypeM[type].shBeginMetaAttr;
    		pMgr->metaAttrM[i].shNodeType == type; i++)
    	{
    		MetaAttr n = pMgr->metaAttrM[i];
    		if (n.cDataType == 'P')
    		{
    			ppNew = (char **)((char *)pUserData + n.shOffset);
    			if (*ppNew == NULL)
    				continue;
    			else 
    			    smrcRemoveRef(pMgr, *ppNew, psmResult);
    		}
    	}
    } 
    else if (pAlloc->shRefCount > 0) 
    {
    	return;
    }
}

/******************** Name of Function **********************************
function prototype 
Purpose:
    Explain what the function does including a brief overview of what it
    returns.
Parameters:
    List each parameter on a separate line including data type name and 
    description.  Each item should begin with whether the parameter is passed 
    in, out or both:
    I   Passed in.  Value isn’t modified by subroutine.
    O   Passed out. Value is returned through this parameter.
    I/O Modified. Original value is used, but this subroutine modifies it.
Notes:
    Include any special assumptions.  If global variables are referenced, 
    reference them.  Explain critical parts of the algorithm.
Return value:
    List the values returned by the function.  Do not list returned 
    parameters.  Remove if void.
**************************************************************************/
void smrcAssoc(
    StorageManager *pMgr, void *pUserDataFrom, char szAttrName[],
    void *pUserDataTo, SMResult *psmResult)
{
	int i;              // incrementer
	char **ppNew;
    short type;
    AllocNode *pAlloc, *pToAlloc;
	pAlloc = (AllocNode *)(((char *)pUserDataFrom) - 3 * sizeof(short));
	pToAlloc = (AllocNode *)(((char *)pUserDataTo) - 3 * sizeof(short));
	type = pAlloc->shNodeType;
	for(i = pMgr->nodeTypeM[type].shBeginMetaAttr;
    		pMgr->metaAttrM[i].shNodeType == type; i++)
	{
		MetaAttr n = pMgr->metaAttrM[i];
		if ( (strcmp(szAttrName, n.szAttrName) == 0 ) &&
                        n.cDataType == 'P')
		{
			psmResult->rc = 0;
			ppNew = (char **)((char *)pUserDataFrom + n.shOffset);
			if (*ppNew != NULL)
				smrcRemoveRef(pMgr, *ppNew, psmResult);
			memcpy(ppNew, &pUserDataTo, sizeof(ppNew));
			pToAlloc->shRefCount++;
		} 
		else if ( (strcmp(szAttrName, n.szAttrName) == 0 ) &&
                        n.cDataType != 'P') 
		{
                        printf("data type = %c\n", n.cDataType);
			psmResult->rc = RC_ASSOC_ATTR_NOT_PTR;
			strcpy(
		               psmResult->szErrorMessage,
		               "This attribute is not a pointer.\n");	
		}
	}
}

/******************** Name of Function **********************************
function prototype 
Purpose:
    Explain what the function does including a brief overview of what it
    returns.
Parameters:
    List each parameter on a separate line including data type name and 
    description.  Each item should begin with whether the parameter is passed 
    in, out or both:
    I   Passed in.  Value isn’t modified by subroutine.
    O   Passed out. Value is returned through this parameter.
    I/O Modified. Original value is used, but this subroutine modifies it.
Notes:
    Include any special assumptions.  If global variables are referenced, 
    reference them.  Explain critical parts of the algorithm.
Return value:
    List the values returned by the function.  Do not list returned 
    parameters.  Remove if void.
**************************************************************************/
void smrcAddRef(
    StorageManager *pMgr, void *pUserDataTo, SMResult *psmResult)
{
	AllocNode *pAlloc;      // ptr to AllocNode to access AllocNode val

        /* make pAlloc point to  pUserDataTo address - 6 (if short ==2 ) */
    pAlloc = (AllocNode *)(((char *)pUserDataTo) - 3 * sizeof(short));
	pAlloc->shRefCount++;
}

/******************** Name of Function **********************************
function prototype 
Purpose:
    Explain what the function does including a brief overview of what it
    returns.
Parameters:
    List each parameter on a separate line including data type name and 
    description.  Each item should begin with whether the parameter is passed 
    in, out or both:
    I   Passed in.  Value isn’t modified by subroutine.
    O   Passed out. Value is returned through this parameter.
    I/O Modified. Original value is used, but this subroutine modifies it.
Notes:
    Include any special assumptions.  If global variables are referenced, 
    reference them.  Explain critical parts of the algorithm.
Return value:
    List the values returned by the function.  Do not list returned 
    parameters.  Remove if void.
**************************************************************************/
extern "C" void printNode(StorageManager *pMgr, void *pUserData)
{ 

    AllocNode *pAlloc;  // ptr to AllocNode to access AllocNode vals
    char **ppPtr;       // ptr to access pointer val
    char *szPtr;        // ptr to a char array to access string val
    int *piPtr;         // ptr to an integer to access integer val
    int i;              // incrementer variable
    double *pdPtr;      // ptr to a double to access double val

    /* make pAlloc point to  pUserData address - 6 (if short ==2 ) */
    pAlloc = (AllocNode *)(((char *)pUserData) - 3 * sizeof(short));
    printf(
        "%s\t%s\t%s\t%s\t\t%s\n", "Alloc Address", "Size", "Node Type",
        "Ref Cnt", "Data Address");
    printf(
        "%08lX\t%d\t%d\t\t%d\t\t%08lX\n", ULAddr(pAlloc), pAlloc->shAllocSize,
        pAlloc->shNodeType, pAlloc->shRefCount, ULAddr(pUserData));
    printf("\n\t\t%s\t%s\t%s\n", "Attr Name", "Type", "Value");

    /* set i to the passed in nodeType's beginning attr index. For each attr
     * index, if that index's nodeType == the passed in nodeType, then
     * increment i (go thru the metaData attrs).   */
    for(i = pMgr->nodeTypeM[pAlloc->shNodeType].shBeginMetaAttr;
        pMgr->metaAttrM[i].shNodeType == pAlloc->shNodeType; i++)
    {
        MetaAttr n = pMgr->metaAttrM[i];
        
        /* check the data types. Print the data using the deferencing of 
         * their respective pointers. ( a double pointer in case of 
         * 'pointer' types */
        switch(n.cDataType)
        {
            case 'P':
                ppPtr = (char **)((char *)pUserData + n.shOffset);
                printf(
                    "\t\t%-10s\t%c\t%08lX\n", n.szAttrName, n.cDataType,
                    ULAddr(*ppPtr));
                break;
            case 'S':
                szPtr = (char *)pUserData + n.shOffset;
                printf("\t\t%-10s\t%c\t%s\n", n.szAttrName, n.cDataType, szPtr);
                break;
            case 'I':
                piPtr = (int *)((char *)pUserData + n.shOffset);
                printf(
                    "\t\t%-10s\t%c\t%d\n", n.szAttrName, n.cDataType, *piPtr);
                break;
            case 'D':
                pdPtr = (double *)((char *)pUserData + n.shOffset);
                printf(
                    "\t\t%-10s\t%c\t%f\n", n.szAttrName, n.cDataType, *pdPtr);
                break;
        }
    }
}
